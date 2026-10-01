import test from 'node:test';
import assert from 'node:assert/strict';
import {createServer} from 'node:http';
import {mkdtemp,rm,readFile,writeFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {deliver,enqueue,configuration} from './alert-delivery.mjs';
import {durableJson} from './durable-json.mjs';
import {evaluate,monitorIdentity} from './finality-monitor.mjs';
const exec=promisify(execFile),a='0x'+'a'.repeat(64);
const monitor={chainId:'1337',genesisHash:a,configName:'nano-reviewed',staleMs:100,timeoutMs:1000,maximumResponseBytes:4096,maximumEpochs:16,minimumPeers:1,nodes:[{name:'node-a',url:'http://127.0.0.1:8545/'}]};
const healthy=evaluate(monitor,[{name:'node-a',epoch:'2',root:a,peers:1}],0);
const fault=evaluate(monitor,[{name:'node-a',error:'down'}],1,healthy.state).report;
const recovered=evaluate(monitor,[{name:'node-a',epoch:'3',root:a,peers:1}],2,healthy.state).report;
async function fixture(fn,reply=()=>({status:200,body:'ok'})){
 const directory=await mkdtemp(join(tmpdir(),'nano-alerts-')),calls=[],accepted=new Set();
 const server=createServer(async(req,res)=>{let text='';for await(const chunk of req)text+=chunk;const body=JSON.parse(text);calls.push({key:req.headers['idempotency-key'],body});const answer=reply(calls.length);if(answer.status===200)accepted.add(body.eventId);res.writeHead(answer.status,answer.headers);res.end(answer.body);});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 const config={monitorIdentity:fault.identity,url:'http://127.0.0.1:'+server.address().port+'/',timeoutMs:1000,maximumResponseBytes:1024,maximumPending:4},file=join(directory,'queue.json');
 try{await fn({config,file,directory,calls,accepted});}finally{server.closeAllConnections();await new Promise(resolve=>server.close(resolve));await rm(directory,{recursive:true,force:true});}
}
test('real HTTP failure survives process restart; retry uses same id; unchanged fault deduplicates and recovery delivers',async()=>fixture(async({config,file,directory,calls,accepted})=>{
 const cp=join(directory,'config.json'),rp=join(directory,'report.json');await writeFile(cp,JSON.stringify(config));await writeFile(rp,JSON.stringify(fault));
 const invoke=async()=>{try{const r=await exec(process.execPath,['scripts/operations/alert-delivery.mjs',cp,rp,file]);return {code:0,result:JSON.parse(r.stdout)};}catch(e){return {code:e.code,result:JSON.parse(e.stdout)};}};
 assert.equal((await invoke()).code,2);assert.equal(JSON.parse(await readFile(file)).pending.length,1);
 assert.equal((await invoke()).code,0);assert.equal(calls.length,2);assert.equal(calls[0].key,calls[1].key);assert.equal(accepted.size,1);
 assert.equal((await invoke()).result.delivered,0);assert.equal(calls.length,2);
 await writeFile(rp,JSON.stringify(recovered));assert.equal((await invoke()).result.delivered,1);assert.equal(calls[2].body.healthy,true);assert.equal(accepted.size,2);
},n=>({status:n===1?503:200,body:'result'})));
test('acceptance followed by failed durable ACK replays identical identifier without duplicate receiver effect',async()=>fixture(async({config,file,calls,accepted})=>{
 let commits=0;await assert.rejects(deliver(config,fault,file,{persist:async(f,v)=>{if(++commits===2)throw Error('injected ACK commit failure');await durableJson(f,v);}}),/ACK commit/);
 assert.equal(JSON.parse(await readFile(file)).pending.length,1);await deliver(config,fault,file);assert.equal(calls.length,2);assert.equal(calls[0].key,calls[1].key);assert.equal(accepted.size,1);
}));
test('redirect is refused and event remains pending',async()=>fixture(async({config,file,calls})=>{
 assert.equal((await deliver(config,fault,file)).deliveryFailed,true);assert.equal(calls.length,1);assert.equal(JSON.parse(await readFile(file)).pending.length,1);
},()=>({status:302,headers:{Location:'http://127.0.0.1:1/'},body:'redirect'})));
test('oversized response preserves pending event',async()=>fixture(async({config,file})=>{
 assert.equal((await deliver({...config,maximumResponseBytes:8},fault,file)).deliveryFailed,true);assert.equal(JSON.parse(await readFile(file)).pending.length,1);
},()=>({status:200,body:'x'.repeat(100)})));
test('queue corruption and cross-monitor report rejected before transmission',async()=>fixture(async({config,file,calls})=>{
 await assert.rejects(deliver(config,{...fault,identity:'0'.repeat(64)},file));assert.equal(calls.length,0);
 const s=enqueue(config,fault);s.pending[0].payload.alerts=[];await writeFile(file,JSON.stringify(s));await assert.rejects(deliver(config,fault,file),/Corrupt/);assert.equal(calls.length,0);
}));
test('queue capacity fails closed without dropping already pending events',()=>{
 const c={monitorIdentity:fault.identity,url:'http://127.0.0.1/',timeoutMs:10,maximumResponseBytes:100,maximumPending:1};const s=enqueue(c,fault);assert.throws(()=>enqueue(c,recovered,s),/capacity/);assert.equal(s.pending.length,1);
});
test('initial healthy observations are quiet, public cleartext and embedded credentials rejected',()=>{
 const c={monitorIdentity:fault.identity,url:'http://127.0.0.1/',timeoutMs:10,maximumResponseBytes:100,maximumPending:1};assert.equal(enqueue(c,healthy.report).pending.length,0);
 assert.throws(()=>configuration({...c,url:'http://example.com/'}));assert.throws(()=>configuration({...c,url:'https://user:secret@example.com/'}));assert.throws(()=>enqueue(c,{...fault,healthy:true}));
});
test('timeout preserves durable pending event',async()=>fixture(async({config,file})=>{
 const r=await deliver(config,fault,file,{request:async(_u,{signal})=>{await new Promise((_,reject)=>{signal.addEventListener('abort',()=>reject(signal.reason),{once:true});setTimeout(()=>reject(Error('guard')),2000).unref();});}});
 assert.equal(r.deliveryFailed,true);assert.equal(JSON.parse(await readFile(file)).pending.length,1);
}));

test('explicit drain frees a full queue without inventing a new monitor observation',async()=>fixture(async({config,file,calls})=>{
 const bounded={...config,maximumPending:1};await durableJson(file,enqueue(bounded,fault));
 await assert.rejects(deliver(bounded,recovered,file),/capacity/);assert.equal(calls.length,0);
 assert.equal((await deliver(bounded,null,file)).delivered,1);assert.equal((await deliver(bounded,recovered,file)).delivered,1);assert.equal(calls.length,2);
}));

test('delivery pin derives from reviewed local monitor configuration',()=>{assert.equal(monitorIdentity(monitor),fault.identity);assert.notEqual(monitorIdentity({...monitor,configName:'different'}),fault.identity);});
