import test from 'node:test';
import assert from 'node:assert/strict';
import {EventEmitter} from 'node:events';
import {createCampaignCommands} from './campaign-commands.mjs';

// All children, PGIDs, signals, waits and timers below are synthetic objects.
function fixture({pid=501000001,onSignal}={}){
 let clock=0,nextTimer=1,phase='work',controller=new AbortController();
 const timers=new Map(),calls=[],kills=[],children=[];
 const end=()=>phase==='work'?50:80;
 const check=()=>{if(clock>=end()&&!controller.signal.aborted)controller.abort(Error('synthetic '+phase+' deadline'));if(controller.signal.aborted)throw controller.signal.reason;};
 const deadline={check,remaining(requested){check();assert(Number.isSafeInteger(requested)&&requested>0);const left=Math.floor(end()-clock);if(left<1)throw Error('synthetic empty command budget');return Math.min(requested,left);},get signal(){return controller.signal;},get phase(){return phase;},enterCloseout(){if(phase==='closeout')return;controller.abort(Error('synthetic work ended'));phase='closeout';controller=new AbortController();}};
 const setTimer=(callback,delay)=>{assert(Number.isSafeInteger(delay)&&delay>0);const id=nextTimer++;timers.set(id,{callback,delay});return id;};
 const clearTimer=id=>timers.delete(id);
 function spawn(program,args,options){
  const child=new EventEmitter();child.pid=pid===null?undefined:pid+children.length;child.exitCode=null;child.signalCode=null;child.stdin=new EventEmitter();child.stdout=new EventEmitter();child.stderr=new EventEmitter();child.stdin.end=input=>{child.input=input;};
  const row={child,present:child.pid!==undefined,closed:false};
  row.exit=(code=0,signal=null)=>{if(child.exitCode===null&&child.signalCode===null){child.exitCode=code;child.signalCode=signal;child.emit('exit',code,signal);}};
  row.complete=({stdout='PUBLIC stdout',stderr='',keepGroup=false,code=0,signal=null}={})=>{
   assert.equal(row.closed,false);
   if(stdout.length)child.stdout.emit('data',Buffer.from(stdout));if(row.closed)return;
   if(stderr.length)child.stderr.emit('data',Buffer.from(stderr));if(row.closed)return;
   row.present=keepGroup;row.exit(code,signal);row.closed=true;child.emit('close',child.exitCode,child.signalCode);
  };
  row.failSpawn=error=>{assert.equal(child.pid,undefined);row.present=false;row.closed=true;child.emit('error',error);child.emit('close',-2,null);};
  children.push(row);calls.push({program,args,options,row});return child;
 }
 const kill=(pgid,signal)=>{
  kills.push([pgid,signal]);assert(pgid<0,'Never signal a positive PID');const row=children.find(x=>x.child.pid===-pgid);assert(row,'Only a created group may be observed or signalled');
  if(signal===0){if(!row.present)throw Object.assign(Error('synthetic absent group'),{code:'ESRCH'});return;}
  if(onSignal)return onSignal(row,signal);
  assert.equal(signal,'SIGTERM');row.present=false;if(!row.closed)row.complete({stdout:'',code:null,signal:'SIGTERM'});
 };
 const commands=createCampaignCommands({deadline,spawn,kill,now:()=>clock,sleep:async ms=>{assert(ms>0&&ms<=25);clock+=ms;},setTimer,clearTimer});
 function fire(){assert.equal(timers.size,1);const [id,t]=[...timers][0];timers.delete(id);clock+=t.delay;t.callback();}
 return {commands,deadline,calls,kills,children,timers,fire,setNow:n=>{assert(n>=clock);clock=n;},abort:reason=>controller.abort(reason)};
}

test('command adapter clamps the timeout, owns a detached child and preserves stdout, options and known stdin',async()=>{
 const f=fixture(),input=Buffer.from('PUBLIC known stdin'),env={LANG:'C'},caller=new AbortController();
 const pending=f.commands.exec('/synthetic/tool',['--public'],{timeout:999,encoding:'buffer',maxBuffer:64,input,env,signal:caller.signal});
 assert.equal(f.calls.length,1);const {options,row}=f.calls[0];assert.equal(options.detached,true);assert.deepEqual(options.stdio,['pipe','pipe','pipe']);for(const key of ['timeout','encoding','maxBuffer','input','signal'])assert.equal(Object.hasOwn(options,key),false);assert.equal(options.env,env);assert.deepEqual(row.child.input,input);assert.notEqual(row.child.input,input);
 assert.equal([...f.timers.values()][0].delay,50);row.complete({stdout:Buffer.from('ok'),stderr:Buffer.alloc(0)});
 assert.deepEqual(await pending,{stdout:Buffer.from('ok'),stderr:Buffer.alloc(0)});assert.equal(f.timers.size,0);assert(!f.kills.some(([,signal])=>signal==='SIGTERM'||signal==='SIGKILL'));await f.commands.closeAll();
 // UTF8 chunk boundaries and byte limits are independent for the two pipes.
 const utf=fixture(),unicode=utf.commands.exec('/synthetic/tool',[],{maxBuffer:3});utf.children[0].child.stdout.emit('data',Buffer.from([0xe2,0x82]));utf.children[0].child.stdout.emit('data',Buffer.from([0xac]));utf.children[0].complete({stdout:'',stderr:'ERR'});
 assert.deepEqual(await unicode,{stdout:String.fromCharCode(0x20ac),stderr:'ERR'});await utf.commands.closeAll();
 for(const stream of ['stdout','stderr']){
  const over=fixture(),pending=over.commands.exec('/synthetic/tool',[],{maxBuffer:2,encoding:'buffer'});let failure;
  const rejected=assert.rejects(pending,error=>{failure=error;return error.code==='ERR_CHILD_PROCESS_STDIO_MAXBUFFER'&&error.message==='Fresh campaign command '+stream+' byte limit exceeded';});
  over.children[0].child[stream].emit('data',Buffer.from([0xe2,0x82,0xac]));await rejected;
  assert.deepEqual(failure[stream],Buffer.from([0xe2,0x82]));assert.equal(Object.keys(failure).includes(stream),false);assert.equal(over.children[0].closed,true);assert.equal(over.children[0].present,false);await over.commands.closeAll();
 }
 const invalid=fixture();for(const maxBuffer of [-1,0.5,Infinity,Number.MAX_SAFE_INTEGER+1])assert.throws(()=>invalid.commands.exec('/synthetic/never',[],{maxBuffer}),/finite per-stream byte limit/);assert.equal(invalid.calls.length,0);
 const late=fixture(),lateOptions={timeout:40,get input(){late.setNow(51);return Buffer.from('known late fixture');}};
 assert.throws(()=>late.commands.exec('/synthetic/never',[],lateOptions),/synthetic work deadline/);assert.equal(late.calls.length,0);assert.equal(late.timers.size,0);
});

test('caller cancellation preserves its error and waits for close plus group absence; leader exit alone is insufficient',async()=>{
 const f=fixture(),caller=new AbortController(),failure=Error('synthetic caller cancelled');let settled=false;
 const pending=f.commands.exec('/synthetic/tool',[],{timeout:40,signal:caller.signal});pending.then(()=>{settled=true;},()=>{settled=true;});
 f.children[0].exit();await Promise.resolve();assert.equal(settled,false);assert.equal(f.children[0].closed,false);assert.equal(f.children[0].present,true);
 caller.abort(failure);await assert.rejects(pending,error=>error===failure);assert.equal(f.children[0].closed,true);assert.equal(f.children[0].present,false);assert.equal(f.timers.size,0);assert(f.kills.some(([id,signal])=>id===-501000001&&signal==='SIGTERM'));await f.commands.closeAll();
 const already=fixture(),aborted=new AbortController();aborted.abort(failure);assert.throws(()=>already.commands.exec('/synthetic/never',[],{signal:aborted.signal}),error=>error===failure);assert.equal(already.calls.length,0);
 // AbortSignal reasons can be falsy: none may turn cancellation into success.
 for(const reason of [false,0,'']){
  const f=fixture(),controller=new AbortController(),pending=f.commands.exec('/synthetic/tool',[],{signal:controller.signal});let fulfilled=false,observed=Symbol('no rejection');
  const settled=pending.then(()=>{fulfilled=true;},value=>{observed=value;});controller.abort(reason);await settled;
  assert.equal(fulfilled,false);assert.equal(observed,reason);assert.equal(f.children[0].closed,true);assert.equal(f.children[0].present,false);await f.commands.closeAll();
  const early=fixture(),cancelled=new AbortController();cancelled.abort(reason);let threw=false,caught;
  try{early.commands.exec('/synthetic/never',[],{signal:cancelled.signal});}catch(error){threw=true;caught=error;}
  assert.equal(threw,true);assert.equal(caught,reason);assert.equal(early.calls.length,0);
 }
 const missing=fixture({pid:null}),spawnFailed=missing.commands.exec('/synthetic/missing',[]),missingRejected=assert.rejects(spawnFailed,error=>error.code==='ENOENT'&&error.message==='Fresh campaign command spawn failed');
 missing.children[0].failSpawn(Object.assign(Error('SENSITIVE synthetic argv'),{code:'ENOENT'}));await missingRejected;assert.deepEqual(missing.kills,[]);await missing.commands.closeAll();
 for(const stream of ['stdin','stdout','stderr']){
  const f=fixture(),pending=f.commands.exec('/synthetic/tool',[],{maxBuffer:16});f.children[0].child.stdout.emit('data',Buffer.from('prior bytes'));
  const rejected=assert.rejects(pending,error=>error.code==='EPIPE'&&error.message==='Fresh campaign command '+stream+' failed'&&error.stdout==='prior bytes'&&!error.message.includes('SENSITIVE'));
  f.children[0].child[stream].emit('error',Object.assign(Error('SENSITIVE synthetic data'),{code:'EPIPE'}));await rejected;assert.equal(f.children[0].closed,true);assert.equal(f.children[0].present,false);await f.commands.closeAll();
 }
});

test('timeout escalates only its owned group and preserves the first error when TERM fails before KILL and join',async()=>{
 const denied=Error('synthetic TERM denied'),f=fixture({onSignal:(row,signal)=>{if(signal==='SIGTERM')throw denied;assert.equal(signal,'SIGKILL');row.present=false;if(!row.closed)row.complete({stdout:'',code:null,signal:'SIGKILL'});}});
 const pending=f.commands.exec('/synthetic/tool',[],{timeout:10});f.fire();
 await assert.rejects(pending,error=>error.message==='Fresh campaign command timeout'&&error.campaignCommandSecondaryFailures.some(text=>text.includes('synthetic TERM denied')));
 assert(f.kills.some(([,signal])=>signal==='SIGTERM'));assert(f.kills.some(([,signal])=>signal==='SIGKILL'));assert.equal(f.children[0].closed,true);assert.equal(f.children[0].present,false);assert.equal(f.timers.size,0);await f.commands.closeAll();
 for(const [code,signal]of [[7,null],[null,'SIGTERM']]){
  const failed=fixture(),pending=failed.commands.exec('/synthetic/tool',[],{maxBuffer:64}),rejected=assert.rejects(pending,error=>error.code===code&&error.signal===signal&&error.message==='Fresh campaign command exited unsuccessfully'&&error.stdout==='SENSITIVE synthetic output');
  failed.children[0].complete({code,signal,stdout:'SENSITIVE synthetic output'});await rejected;assert.equal(failed.children[0].closed,true);assert.equal(failed.children[0].present,false);await failed.commands.closeAll();
 }
});

test('closeout uses a fresh signal and expired global time permits only the explicit owned-cleanup capability',async()=>{
 const f=fixture(),workSignal=f.deadline.signal;f.abort(Error('synthetic work deadline'));
 assert.throws(()=>f.commands.exec('/synthetic/never',[]),/synthetic work deadline/);assert.equal(f.calls.length,0);
 f.deadline.enterCloseout();assert.notEqual(f.deadline.signal,workSignal);assert.equal(f.deadline.signal.aborted,false);
 const closing=f.commands.exec('/synthetic/closing-audit',[],{timeout:100});assert.equal([...f.timers.values()][0].delay,80);f.children[0].complete();await closing;
 f.setNow(81);assert.throws(()=>f.commands.exec('/synthetic/late-audit',[]),/synthetic closeout deadline/);assert.equal(f.calls.length,1);
 const cleanup=f.commands.execOwnedCleanup('/synthetic/docker',['stop','--time','10','synthetic-owned'],{timeout:60000});assert.equal(f.calls.length,2);assert.equal([...f.timers.values()][0].delay,30000);f.children[1].complete();await cleanup;assert.equal(f.timers.size,0);await f.commands.closeAll();
 const work=fixture();assert.throws(()=>work.commands.execOwnedCleanup('/synthetic/never',[]),/cleanup only during closeout/);assert.equal(work.calls.length,0);
});

test('owned cleanup begun before global expiry keeps its bounded join grace while audits remain refused',async()=>{
 const f=fixture();f.deadline.enterCloseout();f.setNow(79);let settled=false;
 const pending=f.commands.execOwnedCleanup('/synthetic/docker',['stop','--time','20','synthetic-owned'],{timeout:60000});pending.then(()=>{settled=true;},()=>{settled=true;});
 assert.equal([...f.timers.values()][0].delay,30000,'Final owner stop must not be clipped to one millisecond');
 f.setNow(81);assert.throws(()=>f.deadline.check(),/synthetic closeout deadline/);await Promise.resolve();
 assert.equal(settled,false);assert.equal(f.deadline.signal.aborted,true);assert.equal(f.timers.size,1);assert(!f.kills.some(([,signal])=>signal==='SIGTERM'||signal==='SIGKILL'));
 assert.throws(()=>f.commands.exec('/synthetic/late-audit',[]),/synthetic closeout deadline/);assert.equal(f.calls.length,1);
 f.children[0].complete();await pending;assert.equal(f.timers.size,0);await f.commands.closeAll();
 const g=fixture(),caller=new AbortController(),failure=Error('synthetic cleanup caller cancelled');g.deadline.enterCloseout();
 const closing=g.commands.execOwnedCleanup('/synthetic/docker',['inspect','synthetic-owned'],{timeout:20000,signal:caller.signal});caller.abort(failure);
 await assert.rejects(closing,error=>error===failure);assert.equal(g.children[0].closed,true);assert.equal(g.children[0].present,false);await g.commands.closeAll();
});

test('reserved PIDs and unregistered persistent handles can never trigger a foreign signal',async()=>{
 for(const pid of [1,28722,process.pid]){
  const f=fixture({pid}),pending=f.commands.exec('/synthetic/tool',[],{timeout:10});f.children[0].complete({keepGroup:true});
  await assert.rejects(pending,/reserved or invalid owned PID/);assert.deepEqual(f.kills,[]);await assert.rejects(f.commands.closeAll());assert.deepEqual(f.kills,[]);
 }
 const f=fixture(),foreign=Object.assign(new EventEmitter(),{pid:501000099});await assert.rejects(f.commands.terminatePersistent(foreign,()=>true,10),/only registered persistent child/);assert.deepEqual(f.kills,[]);assert.equal(f.calls.length,0);
});
