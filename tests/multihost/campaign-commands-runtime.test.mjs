// Only processes created by this test, inside its Docker PID namespace.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {setTimeout as sleep} from 'node:timers/promises';
import {createCampaignDeadline} from './campaign-deadline.mjs';
import {createCampaignCommands} from './campaign-commands.mjs';

function owned(){
 const deadline=createCampaignDeadline({startedAt:performance.now(),maximumRunMs:20000,closeoutReserveMs:2000}),children=[];
 let readyResolve,exitResolve;
 const ready=new Promise(resolve=>{readyResolve=resolve;}),exited=new Promise(resolve=>{exitResolve=resolve;});
 const commands=createCampaignCommands({deadline,spawn:(program,args,options)=>{
  const child=spawn(program,args,options);children.push(child);
  child.stdout?.on('data',bytes=>{if(String(bytes).includes('ready'))readyResolve();});
  child.once('exit',exitResolve);return child;
 }});
 return {deadline,commands,children,ready,exited,async dispose(){try{await commands.closeAll();}finally{deadline.dispose();}}};
}
async function beforeLimit(promise,ms){
 const until=performance.now()+ms;let done=false,value,error;
 promise.then(v=>{done=true;value=v;},e=>{done=true;error=e;});
 while(!done&&performance.now()<until)await sleep(10);
 assert(done,'owned child observation deadline');if(error)throw error;return value;
}
const absent=child=>assert.throws(()=>process.kill(-child.pid,0),{code:'ESRCH'});

test('real owned command joins its process group and captures exact stdin/stdout/stderr',{timeout:10000},async()=>{
 const c=owned();try{
  const result=await c.commands.exec(process.execPath,['-e',"let s='';process.stdin.setEncoding('utf8');process.stdin.on('data',b=>s+=b);process.stdin.on('end',()=>{process.stdout.write(s);process.stderr.write('known stderr');});"],{timeout:5000,maxBuffer:1024,input:Buffer.from('known synthetic input')});
  assert.equal(result.stdout,'known synthetic input');assert.equal(result.stderr,'known stderr');assert.equal(c.children.length,1);absent(c.children[0]);
 }finally{await c.dispose();}
});

test('real owned command preserves caller cancellation and joins a child that ignores TERM via KILL',{timeout:15000},async()=>{
 const c=owned(),controller=new AbortController(),reason=Error('known caller cancellation');try{
  const promise=c.commands.exec(process.execPath,['-e',"process.on('SIGTERM',()=>{});process.stdout.write('ready');setInterval(()=>{},1000);"],{timeout:10000,maxBuffer:1024,signal:controller.signal});
  // The assertion handler is attached before cancellation to retain the exact error.
  const rejected=assert.rejects(promise,e=>e===reason);
  rejected.catch(()=>{});
  await beforeLimit(c.ready,5000);controller.abort(reason);await rejected;
  assert.equal(c.children[0].signalCode,'SIGKILL');absent(c.children[0]);
  assert(reason.campaignCommandSecondaryFailures.some(s=>s.includes('required SIGKILL')));
 }finally{await c.dispose();}
});

test('real owned command does not treat leader exit as closure while a same-group descendant holds stdio',{timeout:15000},async()=>{
 const c=owned(),controller=new AbortController(),reason=Error('known descendant cancellation');try{
  const source="const {spawn}=require('node:child_process');spawn(process.execPath,['-e','setInterval(()=>{},1000)'],{stdio:'inherit'});process.stdout.write('ready');process.exit(0);";
  const promise=c.commands.exec(process.execPath,['-e',source],{timeout:10000,maxBuffer:1024,signal:controller.signal});let settled=false;promise.then(()=>{settled=true;},()=>{settled=true;});
  const rejected=assert.rejects(promise,e=>e===reason);
  rejected.catch(()=>{});
  await beforeLimit(c.ready,5000);await beforeLimit(c.exited,5000);assert.equal(c.children[0].exitCode,0);assert.equal(settled,false);
  // The group is still present even though its leader has exited.
  assert.equal(process.kill(-c.children[0].pid,0),true);
  controller.abort(reason);await rejected;absent(c.children[0]);
 }finally{await c.dispose();}
});

test('real owned command timeout terminates and joins its own process group',{timeout:10000},async()=>{
 const c=owned();try{
  await assert.rejects(c.commands.exec(process.execPath,['-e','setInterval(()=>{},1000)'],{timeout:1000,maxBuffer:1024}),/Fresh campaign command timeout/);
  assert.equal(c.children.length,1);absent(c.children[0]);
 }finally{await c.dispose();}
});
