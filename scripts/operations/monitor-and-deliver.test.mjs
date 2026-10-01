import test from 'node:test';
import assert from 'node:assert/strict';
import {createServer} from 'node:http';
import {mkdtemp,readFile,writeFile,rm,unlink,symlink} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {cycle} from './monitor-and-deliver.mjs';
import {run,monitorIdentity} from './finality-monitor.mjs';
const exec=promisify(execFile),a='0x'+'a'.repeat(64),b='0x'+'b'.repeat(64);
async function fixture(fn){
 const dir=await mkdtemp(join(tmpdir(),'nano-handoff-'));let unavailable=false,receiverDown=false,reads=0;const calls=[];
 const rpc=createServer(async(req,res)=>{++reads;if(unavailable){res.writeHead(503);res.end('{}');return;}let out;if(req.method==='POST'){let text='';for await(const part of req)text+=part;const m=JSON.parse(text).method;out={jsonrpc:'2.0',id:1,result:m==='eth_chainId'?'0x539':m==='net_peerCount'?'0x1':{hash:a}};}else if(req.url.endsWith('/spec'))out={data:{CONFIG_NAME:'nano-reviewed'}};else if(req.url.endsWith('/head'))out={data:{root:a,header:{message:{state_root:b}}}};else out={data:{finalized:{epoch:'2',root:a}}};res.end(JSON.stringify(out));});
 const receiver=createServer(async(req,res)=>{let text='';for await(const part of req)text+=part;calls.push(JSON.parse(text));res.writeHead(receiverDown?503:200);res.end('ok');});
 for(const s of [rpc,receiver])await new Promise(r=>s.listen(0,'127.0.0.1',r));
 const config={chainId:'1337',genesisHash:a,configName:'nano-reviewed',staleMs:60000,timeoutMs:1000,maximumResponseBytes:4096,maximumEpochs:16,minimumPeers:1,nodes:[{name:'node-a',url:'http://127.0.0.1:'+rpc.address().port+'/'}]};
 const args=['monitor.json','delivery.json','state.json','queue.json'].map(f=>join(dir,f));
 await writeFile(args[0],JSON.stringify(config));await writeFile(args[1],JSON.stringify({monitorIdentity:monitorIdentity(config),url:'http://127.0.0.1:'+receiver.address().port+'/',timeoutMs:1000,maximumResponseBytes:1024,maximumPending:4}));
 try{await fn({args,calls,config,setUnavailable:v=>unavailable=v,setReceiverDown:v=>receiverDown=v,reads:()=>reads});}finally{for(const s of [rpc,receiver]){s.closeAllConnections();await new Promise(r=>s.close(r));}await rm(dir,{recursive:true,force:true});}
}
test('SIGKILL after observation commit retains fault; verified operator lock removal resumes handoff before healthy observation',async()=>fixture(async({args,calls,setUnavailable})=>{
 await cycle(...args);setUnavailable(true);
 const script="import {cycle} from './scripts/operations/monitor-and-deliver.mjs'; await cycle(...JSON.parse(process.argv[1]),{send:async()=>process.kill(process.pid,'SIGKILL')});";
 await assert.rejects(exec(process.execPath,['--input-type=module','-e',script,JSON.stringify(args)]),e=>e.signal==='SIGKILL');
 assert.equal(JSON.parse(await readFile(args[2])).pendingDelivery.healthy,false);
 await assert.rejects(cycle(...args),e=>e.code==='EEXIST');
 await assert.rejects(run(args[0],args[2]),e=>e.code==='EEXIST');
 // Child has exited with SIGKILL, authoritative permission for fixture lock cleanup.
 await unlink(args[2]+'.lock');await assert.rejects(run(args[0],args[2]),/Pending delivery/);
 setUnavailable(false);const r=await cycle(...args);assert.equal(r.retried,true);assert.equal(r.report.healthy,true);
 assert.deepEqual(calls.map(c=>c.healthy),[false,true]);assert.equal(JSON.parse(await readFile(args[2])).pendingDelivery,undefined);
}));
test('failed receiver leaves durable queue while monitor continues; later recovery keeps same event identifier',async()=>fixture(async({args,calls,setUnavailable,setReceiverDown})=>{
 await cycle(...args);setUnavailable(true);setReceiverDown(true);const bad=await cycle(...args);assert.equal(bad.delivery.deliveryFailed,true);
 assert.equal(JSON.parse(await readFile(args[3])).pending.length,1);setUnavailable(false);setReceiverDown(false);const good=await cycle(...args);
 assert.equal(good.report.healthy,true);assert.equal(good.delivery.pending,0);assert.equal(calls[0].eventId,calls[1].eventId);assert.deepEqual(calls.map(c=>c.healthy),[false,false,true]);
}));
test('identity mismatch and corrupted persisted monitor stop before sampling or sending',async()=>fixture(async({args,calls,reads})=>{
 await cycle(...args);const before=reads(),state=JSON.parse(await readFile(args[2]));state.identity='0'.repeat(64);await writeFile(args[2],JSON.stringify(state));await assert.rejects(cycle(...args),/state\/clock/);assert.equal(reads(),before);assert.equal(calls.length,0);
 const c=JSON.parse(await readFile(args[1]));c.monitorIdentity='0'.repeat(64);await writeFile(args[1],JSON.stringify(c));await assert.rejects(cycle(...args),/identity differs/);assert.equal(reads(),before);
}));

test('path aliases and lock collisions are rejected before any state write',async()=>fixture(async({args,calls,reads})=>{
 await cycle(...args);const before=reads(),alias=join(join(args[2],'..'),'alias.json');await symlink(args[2],alias);
 for(const queue of [alias,args[2]+'.lock'])await assert.rejects(cycle(args[0],args[1],args[2],queue),/paths overlap/);
 assert.equal(reads(),before);assert.equal(calls.length,0);assert.equal(JSON.parse(await readFile(args[2])).identity.length,64);
}));
