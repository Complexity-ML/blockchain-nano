import fs from 'node:fs';
import path from 'node:path';
import net from 'node:net';
import {spawn} from 'node:child_process';
import {createHash} from 'node:crypto';
import {performance} from 'node:perf_hooks';
import assert from 'node:assert/strict';

// Fixed local test protocol; no fork URL, key, transaction or arbitrary RPC.
const MAX_MS=180000, RPC_MS=3000, LOG_BYTES=1048576, BODY_BYTES=262144, TRACE_BYTES=16777216;
const start=performance.now(), deadline=start+MAX_MS;
const needed=['rpc-simulation-context-fees','rpc-simulation-context-balances',
  'rpc-simulation-context-nonces','rpc-simulation-context-bounds'];
const allowed=new Set(['eth_call','eth_estimateGas','eth_createAccessList']);
const sha=b=>createHash('sha256').update(b).digest('hex');
const canonical=x=>JSON.stringify(x,(_,v)=>v&&typeof v==='object'&&!Array.isArray(v)?
  Object.fromEntries(Object.keys(v).sort().map(k=>[k,v[k]])):v);
const nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
let stopped=false, activeAbort, child, childLog, trace, failure=null, cleanupErrors=[];
let sequence=0, compared=0, unexpected=0, policyDifferences=0, quoteDifferences=0, traceBytes=0;
const forks=[], differences=[], readinessObservations=[];
function check(){assert(!stopped,'oracle interrupted');assert(performance.now()<deadline,'oracle global deadline');}
for(const signal of ['SIGINT','SIGTERM','SIGHUP'])process.once(signal,()=>{stopped=true;activeAbort?.abort();});
function material(p,cap){
  const fd=fs.openSync(p,fs.constants.O_RDONLY|fs.constants.O_NOFOLLOW);
  try{
    const before=fs.fstatSync(fd,{bigint:true});assert(before.isFile()&&before.nlink===1n&&before.size>0n&&before.size<=BigInt(cap));
    const b=Buffer.alloc(Number(before.size));let offset=0;
    while(offset<b.length){const n=fs.readSync(fd,b,offset,b.length-offset,null);assert(n>0,'truncated input');offset+=n;}
    assert.equal(fs.readSync(fd,Buffer.alloc(1),0,1,null),0,'input grew');
    const after=fs.fstatSync(fd,{bigint:true}),logical=fs.lstatSync(p,{bigint:true});
    assert.deepEqual(nine(before),nine(after));assert.deepEqual(nine(after),nine(logical));assert.equal(BigInt(b.length),before.size);
    return {material:nine(after),sha256:sha(b),bytes:b.length,bytesValue:b};
  }finally{fs.closeSync(fd);}
}
function same(p,old,cap){const now=material(p,cap);assert.deepEqual(now.material,old.material);assert.equal(now.sha256,old.sha256);}
function publicPin(x){return {material:x.material,sha256:x.sha256,bytes:x.bytes};}
function emitRecord(x){const b=Buffer.from(JSON.stringify(x)+'\n');assert(traceBytes+b.length<=TRACE_BYTES,'comparison trace cap');let offset=0;while(offset<b.length){const n=fs.writeSync(trace,b,offset,b.length-offset);assert(n>0,'comparison trace short write');offset+=n;}fs.fsyncSync(trace);traceBytes+=b.length;}
function closeDurably(fd){let first;try{fs.fsyncSync(fd);}catch(e){first=e;}try{fs.closeSync(fd);}catch(e){first??=e;}if(first)throw first;}
function closeChildLog(){if(childLog===undefined)return;const fd=childLog;childLog=undefined;closeDurably(fd);}
function save(name,x){const fd=fs.openSync(path.join(out,name),'wx',0o600);try{fs.writeFileSync(fd,JSON.stringify(x,null,2)+'\n');fs.fsyncSync(fd);}finally{fs.closeSync(fd);}const d=fs.openSync(out,'r');try{fs.fsyncSync(d);}finally{fs.closeSync(d);}}
function quantity(x){assert(typeof x==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(x),'canonical quantity');return BigInt(x);}
function address(x){assert(typeof x==='string'&&/^0x[0-9a-fA-F]{40}$/.test(x),'20-byte address');return x.toLowerCase();}
function data(x){assert(typeof x==='string'&&/^0x(?:[0-9a-fA-F]{2})*$/.test(x),'hex data');return x.toLowerCase();}
function loadEvents(bytes){
  const text=bytes.toString('utf8');let values;
  try{const v=JSON.parse(text);values=Array.isArray(v)?v:[v];}catch{values=text.trim().split('\n').map(s=>JSON.parse(s));}
  assert(values.length===4||values.length===5,'four groups with optional single summary');
  const groups=values.filter(x=>needed.includes(x?.scenario));assert.equal(groups.length,4);
  assert.deepEqual(groups.map(x=>x.scenario),needed);let total=0;
  for(const [i,g]of groups.entries()){
    assert.equal(g.passed,true);assert.equal(g.checks,[104,264,336,476][i]);assert.equal(g.expectedRefusals,[0,6,6,126][i]);
    assert(g.fixtures&&Array.isArray(g.caseRecords)&&g.caseRecords.length===[34,66,32,126][i]);
    for(const r of g.caseRecords){assert(['Paris','Shanghai'].includes(r.fork)&&allowed.has(r.method));assert(Array.isArray(r.params)&&r.params.length===2&&r.params[1]==='latest');assert(r.params[0]&&typeof r.params[0]==='object'&&!Array.isArray(r.params[0]));assert(r.response&&r.response.jsonrpc==='2.0'&&r.response.id===17);assert(Object.hasOwn(r.response,'result')!==Object.hasOwn(r.response,'error'));}
    total+=g.caseRecords.length;
  }
  assert.equal(total,258);
  for(const v of values)if(!groups.includes(v))assert(v.format==='nano-rpc-simulation-context-targeted-v1'&&v.passed===true&&v.selectedGroup==='all'&&v.groups===4&&v.checks===476&&v.expectedRefusals===126);
  return groups;
}
function errorClass(e){
  if(!e)return null;assert(Number.isInteger(e.code)&&typeof e.message==='string');
  const m=e.message.toLowerCase();
  if(e.code===-32602||/invalid (?:argument|params|type)|cannot unmarshal|unsupported (?:type|transaction)|not supported|out of range|hex string/.test(m))return 'invalidParameters';
  if(e.code===3||/execution reverted|execution failed|revert/.test(m))return 'executionFailure';
  if(/insufficient funds|available funds|gas required exceeds allowance|gas and value|funds for/.test(m))return 'insufficientFunds';
  if(/fee cap.*(?:base fee|less than)|max fee per gas less/.test(m))return 'feeCap';
  if(/nonce.*(?:limit|maximum|too high|overflow)/.test(m))return 'nonceLimit';
  if(e.code===-32002||/timeout|timed out/.test(m))return 'timeout';
  return 'unclassifiedRpcError';
}
function category(r){return Object.hasOwn(r,'error')?errorClass(r.error):'success';}
function accessList(x){
  assert(Array.isArray(x));return x.map(r=>({address:address(r.address),storageKeys:r.storageKeys.map(k=>{assert(/^0x[0-9a-fA-F]{64}$/.test(k));return k.toLowerCase();}).sort()})).sort((a,b)=>a.address.localeCompare(b.address));
}
function feePolicy(r){return r.method==='eth_createAccessList'&&r.response.error?.code===-32602&&
  /^(?:gasPrice must be nonzero after London|maxFeePerGas must be nonzero)$/.test(r.response.error.message);}
async function port(){const s=net.createServer();await new Promise((resolve,reject)=>{s.once('error',reject);s.listen(0,'127.0.0.1',resolve);});const n=s.address().port;await new Promise((resolve,reject)=>s.close(e=>e?reject(e):resolve()));return n;}
async function stopChild(){
  const p=child;if(!p)return;
  if(p.oracleClosed){if(child===p)child=undefined;if(p.spawnError)throw p.spawnError;return;}
  const ended=new Promise(resolve=>p.once('close',resolve));
  if(p.exitCode===null&&p.signalCode===null&&!p.spawnError)p.kill('SIGTERM');
  let timer;try{await Promise.race([ended,new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('TERM grace')),2000);})]);}
  catch{if(p.exitCode===null&&p.signalCode===null){p.kill('SIGKILL');let last;try{await Promise.race([ended,new Promise((_,reject)=>{last=setTimeout(()=>reject(Error('owned Anvil did not join')),2000);})]);}finally{clearTimeout(last);}}}
  finally{clearTimeout(timer);}
  assert(p.oracleClosed,'owned Anvil stdout/stderr did not close');if(child===p)child=undefined;if(p.spawnError)throw p.spawnError;
}
async function rpc(method,params){
  check();assert(child&&!child.spawnError&&child.exitCode===null&&child.signalCode===null,'owned Anvil is not running');
  const controller=new AbortController();activeAbort=controller;let timer;const id=++sequence;
  try{
    timer=setTimeout(()=>controller.abort(),Math.max(1,Math.min(RPC_MS,deadline-performance.now())));
    const response=await fetch('http://127.0.0.1:'+child.port,{method:'POST',headers:{'content-type':'application/json'},redirect:'error',signal:controller.signal,body:JSON.stringify({jsonrpc:'2.0',id,method,params})});
    assert(response.ok,'local RPC HTTP status');const reader=response.body.getReader(),chunks=[];let bytes=0;
    for(;;){const z=await reader.read();if(z.done)break;bytes+=z.value.length;assert(bytes<=BODY_BYTES,'RPC body limit');chunks.push(z.value);}
    const r=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(Buffer.concat(chunks)));assert(r.jsonrpc==='2.0'&&r.id===id);assert(Object.hasOwn(r,'result')!==Object.hasOwn(r,'error'));
    if(r.error)errorClass(r.error);check();return r;
  }catch(e){controller.abort();throw e;}finally{clearTimeout(timer);if(activeAbort===controller)activeAbort=undefined;}
}
async function value(method,params){const r=await rpc(method,params);assert(!r.error,method+': '+JSON.stringify(r.error));return r.result;}
async function startFork(fork,fixture){
  check();const observation={fork,observedClientVersion:null,lastReadinessError:null,attempts:0,ready:false};readinessObservations.push(observation);const n=await port();childLog=fs.openSync(path.join(out,'anvil-'+fork.toLowerCase()+'.log'),'wx',0o600);let logged=0;
  const p=spawn(anvil,['--host','127.0.0.1','--port',String(n),'--chain-id','1337','--accounts','0','--gas-limit','30000000','--base-fee','1000000000','--hardfork',fork.toLowerCase(),'--silent'],{stdio:['ignore','pipe','pipe'],env:{PATH:'/usr/bin:/bin',HOME:'/tmp',LANG:'C',LC_ALL:'C'}});
  child=p;const fd=childLog;p.port=n;p.once('error',e=>{p.spawnError=e;});p.once('close',()=>{p.oracleClosed=true;});
  const log=b=>{logged+=b.length;if(logged>LOG_BYTES){p.spawnError=Error('Anvil log cap');if(p.exitCode===null&&p.signalCode===null)p.kill('SIGTERM');return;}try{fs.writeSync(fd,b);}catch(e){p.spawnError=e;if(p.exitCode===null&&p.signalCode===null)p.kill('SIGTERM');}};
  p.stdout.on('data',log);p.stderr.on('data',log);
  const readyUntil=Math.min(deadline,performance.now()+10000);let version,ready=false;
  while(performance.now()<readyUntil){check();if(p.spawnError)throw p.spawnError;if(p.exitCode!==null||p.signalCode!==null)throw Error('Anvil exited before readiness');try{
    ++observation.attempts;version=await value('web3_clientVersion',[]);
    observation.observedClientVersion=typeof version==='string'?version.slice(0,256):null;
    assert(typeof version==='string'&&version.length<=256&&/^anvil\/v?1\.5\.1(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?(?:\+[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?$/.test(version),'unexpected Anvil client version');
    assert.equal(quantity(await value('eth_chainId',[])),1337n);ready=true;observation.ready=true;break;
  }catch(e){observation.lastReadinessError=String(e.message).slice(0,2048);await new Promise(r=>setTimeout(r,25));}}
  assert(ready,'Anvil 1.5.1 readiness: '+JSON.stringify(observation));
  assert(fixture.genesisInput.config.chainId===1337&&fixture.genesisInput.config.evmFork===fork);
  const header=await value('eth_getBlockByNumber',['latest',false]);assert.equal(quantity(header.gasLimit),30000000n);assert.equal(quantity(header.baseFeePerGas),1000000000n);
  assert.equal(quantity(fixture.header.gasLimit),30000000n);assert.equal(quantity(fixture.header.baseFeePerGas),1000000000n);
  for(const [a,account]of Object.entries(fixture.genesisInput.alloc)){
    address(a);assert(!account.storage||Object.keys(account.storage).length===0,'unsupported nonempty fixture storage');
    const balance=account.balance??'0x0',nonce=account.nonce??'0x0',code=account.code??'0x';quantity(balance);quantity(nonce);data(code);
    await value('anvil_setBalance',[a,balance]);await value('anvil_setNonce',[a,nonce]);await value('anvil_setCode',[a,code]);
    assert.equal(quantity(await value('eth_getBalance',[a,'latest'])),quantity(balance));assert.equal(quantity(await value('eth_getTransactionCount',[a,'latest'])),quantity(nonce));assert.equal(data(await value('eth_getCode',[a,'latest'])),data(code));
    assert.equal(data(await value('eth_getStorageAt',[a,'0x0','latest'])),'0x'+'00'.repeat(32));
  }
  return {fork,clientVersion:version,chainId:'1337',baseFeePerGas:header.baseFeePerGas,gasLimit:header.gasLimit,allocationsVerified:Object.keys(fixture.genesisInput.alloc).length};
}
async function compare(record,index){
  const actual=await rpc(record.method,record.params),expected=record.response;
  const result={index,fork:record.fork,method:record.method,params:record.params,candidate:expected,anvil:actual,candidateClass:category(expected),anvilClass:category(actual),differences:[],replays:[]};
  const difference=(kind,detail,qualitative=true)=>{const d={index,fork:record.fork,method:record.method,kind,detail,affectsQualitativeComparison:qualitative};result.differences.push(d);differences.push(d);};
  try{
  if(result.candidateClass!==result.anvilClass||result.candidateClass==='unclassifiedRpcError')difference('resultClass',{candidate:result.candidateClass,anvil:result.anvilClass});
  else if(result.candidateClass==='success'){
    if(record.method==='eth_call'){if(data(actual.result)!==data(expected.result))difference('returnData',{candidate:expected.result,anvil:actual.result});}
    else if(record.method==='eth_estimateGas'){
      const c=quantity(expected.result),a=quantity(actual.result);const requested=record.params[0].gas;
      const cap=requested!=null&&quantity(requested)>=21000n?quantity(requested)<30000000n?quantity(requested):30000000n:30000000n;
      if(c<=0n||a<=0n||c>cap||a>cap)difference('estimateBounds',{candidate:String(c),anvil:String(a),cap:String(cap)});
      for(const gas of new Set([expected.result,actual.result])){const replay=await rpc('eth_call',[{...record.params[0],gas},'latest']);result.replays.push({gas,response:replay});if(category(replay)!=='success')difference('estimateReplay',{gas,response:replay});}
      if(c!==a){++quoteDifferences;difference('estimateGasQuote',{candidate:expected.result,anvil:actual.result},false);}
    }else{
      const e=expected.result,a=actual.result;
      if(canonical(accessList(e.accessList))!==canonical(accessList(a.accessList)))difference('accessList',{candidate:e.accessList,anvil:a.accessList});
      if(quantity(e.gasUsed)!==quantity(a.gasUsed))difference('accessListGasUsed',{candidate:e.gasUsed,anvil:a.gasUsed});
      if(Object.hasOwn(e,'error')!==Object.hasOwn(a,'error'))difference('accessListExecution',{candidate:e.error??null,anvil:a.error??null});
      else if(Object.hasOwn(e,'error')){const ec=errorClass({code:0,message:e.error}),ac=errorClass({code:0,message:a.error});if(ec!==ac||ec==='unclassifiedRpcError')difference('accessListExecutionClass',{candidate:ec,anvil:ac,candidateMessage:e.error,anvilMessage:a.error});}
    }
  }
  }catch(e){difference('comparisonValueError',String(e.message).slice(0,2048));}
  const policy=feePolicy(record)&&result.anvilClass==='success'&&result.differences.length===1&&result.differences[0].kind==='resultClass';if(policy)++policyDifferences;
  result.documentedAccessListZeroFeePolicy=policy;
  result.passed=result.differences.every(d=>!d.affectsQualitativeComparison)||policy;
  return result;
}

assert.equal(process.argv.length,3,'usage: ANVIL=/absolute/anvil OUT=/new/dir node runner.mjs candidate-events.jsonl');
const input=path.resolve(process.argv[2]),anvil=process.env.ANVIL,out=process.env.OUT;
assert(anvil&&path.isAbsolute(anvil)&&fs.realpathSync(anvil)===anvil,'physical ANVIL path');assert(out&&path.isAbsolute(out));
assert.equal(fs.realpathSync(path.dirname(out)),path.dirname(out),'physical output parent');
const heldInput=material(input,4*1048576),heldAnvil=material(anvil,64*1048576);
const events=loadEvents(heldInput.bytesValue);delete heldInput.bytesValue;delete heldAnvil.bytesValue;
fs.mkdirSync(out,{mode:0o700});trace=fs.openSync(path.join(out,'comparisons.jsonl'),'wx',0o600);
try{
  save('inputs-public.json',{format:'nano-rpc-context-anvil-input-pins-v1',events:publicPin(heldInput),anvil:publicPin(heldAnvil),expectedCases:258,profiles:['Paris','Shanghai'],sdkOrHistoricalAuthorityRecovered:false});
  for(const fork of ['Paris','Shanghai']){
    const fixture=events[0].fixtures[fork];for(const event of events)assert.equal(canonical(event.fixtures[fork]),canonical(fixture),'fixture must be identical across groups');
      const setup=await startFork(fork,fixture);const before=await value('eth_getBlockByNumber',['latest',false]);let count=0;
      for(const event of events)for(const row of event.caseRecords)if(row.fork===fork){check();let r;try{r=await compare(row,compared);}catch(e){r={index:compared,fork,method:row.method,params:row.params,candidate:row.response,comparisonError:String(e.message).slice(0,2048),passed:false};}emitRecord(r);if(!r.passed)++unexpected;++compared;++count;}
      assert.equal(count,129);const after=await value('eth_getBlockByNumber',['latest',false]);assert.equal(before.hash,after.hash,'replay must not mine or change header');
      // Readback only. No reset, repair, transaction, mine or snapshot RPC exists.
      for(const [a,account]of Object.entries(fixture.genesisInput.alloc)){assert.equal(quantity(await value('eth_getBalance',[a,'latest'])),quantity(account.balance??'0x0'));assert.equal(quantity(await value('eth_getTransactionCount',[a,'latest'])),quantity(account.nonce??'0x0'));assert.equal(data(await value('eth_getCode',[a,'latest'])),data(account.code??'0x'));assert.equal(data(await value('eth_getStorageAt',[a,'0x0','latest'])),'0x'+'00'.repeat(32));}
      forks.push({...setup,cases:count,headerUnchanged:true,allocatedAccountsUnchanged:true});fs.fsyncSync(trace);
      await stopChild();closeChildLog();
  }
  assert.equal(compared,258);
}catch(e){failure=String(e.message).slice(0,4096);process.exitCode=1;}
finally{
  try{await stopChild();}catch(e){cleanupErrors.push(String(e.message));failure??='owned child cleanup failed';process.exitCode=1;}
  try{closeChildLog();}catch(e){cleanupErrors.push(String(e.message));failure??='child log close failed';process.exitCode=1;}
  try{same(input,heldInput,4*1048576);same(anvil,heldAnvil,64*1048576);}catch(e){failure??='input/program final fence: '+String(e.message);process.exitCode=1;}
  try{closeDurably(trace);}catch(e){failure??='comparison trace close failed';process.exitCode=1;}finally{trace=undefined;}
  const completed=!failure&&compared===258&&forks.length===2,passed=completed&&unexpected===0&&cleanupErrors.length===0;
  const summary={format:'nano-rpc-context-anvil-oracle-v1',passed,completed,compared,expectedCases:258,forks,readinessObservations,unexpectedComparisons:unexpected,documentedAccessListZeroFeePolicyComparisons:policyDifferences,estimateQuoteDifferences:quoteDifferences,exactAllComparedValuesEqual:passed&&differences.length===0,differences,firstFailure:failure,cleanupErrors,elapsedMs:Math.round(performance.now()-start),maximumRunMs:MAX_MS,productionQualified:false,globalSdkQualified:false,authorExecuted:false};
  save('result-public.json',summary);console.log(JSON.stringify(summary));if(!passed)process.exitCode=1;
}
