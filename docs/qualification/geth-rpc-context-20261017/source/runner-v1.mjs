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
const forks=[], differences=[], readinessObservations=[], heldControls=[];
const NODE_KEY_HEX='0'.repeat(63)+'1'; // Existing public secp256k1 fixture key 1, never an account.
const PUBLIC_JWT_HEX='0x'+'11'.repeat(32)+'\n';
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
function saveText(name,text){
  const file=path.join(out,name),fd=fs.openSync(file,'wx',0o600);try{fs.writeFileSync(fd,text);fs.fsyncSync(fd);}finally{fs.closeSync(fd);}
  const d=fs.openSync(out,'r');try{fs.fsyncSync(d);}finally{fs.closeSync(d);}
  const held=material(file,524288);delete held.bytesValue;heldControls.push({path:file,held});return file;
}
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
  catch{if(p.exitCode===null&&p.signalCode===null){p.kill('SIGKILL');let last;try{await Promise.race([ended,new Promise((_,reject)=>{last=setTimeout(()=>reject(Error('owned Geth did not join')),2000);})]);}finally{clearTimeout(last);}}}
  finally{clearTimeout(timer);}
  assert(p.oracleClosed,'owned Geth stdout/stderr did not close');if(child===p)child=undefined;if(p.spawnError)throw p.spawnError;
}
async function rpc(method,params){
  check();assert(child&&!child.spawnError&&child.exitCode===null&&child.signalCode===null,'owned Geth is not running');
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
function spawnOwned(label,args,rpcPort){
  check();assert(!child,'previous Geth child not joined');childLog=fs.openSync(path.join(out,label+'.log'),'wx',0o600);let logged=0;
  save(label+'-argv.json',{program:geth,argv:args,cwd:out,publicFixtureNodeKey:true});
  const p=spawn(geth,args,{cwd:out,stdio:['ignore','pipe','pipe'],env:{PATH:'/usr/bin:/bin',HOME:out,LANG:'C',LC_ALL:'C',GOMAXPROCS:'2'}});
  child=p;const fd=childLog;p.port=rpcPort;p.once('error',e=>{p.spawnError=e;});p.closedPromise=new Promise(resolve=>p.once('close',()=>{p.oracleClosed=true;resolve();}));
  const log=b=>{logged+=b.length;if(logged>LOG_BYTES){p.spawnError=Error('Geth log cap');if(p.exitCode===null&&p.signalCode===null)p.kill('SIGTERM');return;}try{let off=0;while(off<b.length){const n=fs.writeSync(fd,b,off,b.length-off);assert(n>0);off+=n;}}catch(e){p.spawnError=e;if(p.exitCode===null&&p.signalCode===null)p.kill('SIGTERM');}};
  p.stdout.on('data',log);p.stderr.on('data',log);return p;
}
async function initGenesis(label,args){
  const p=spawnOwned(label,args);let timer;try{await Promise.race([p.closedPromise,new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('Geth init deadline')),Math.max(1,Math.min(20000,deadline-performance.now())));})]);}finally{clearTimeout(timer);}
  save(label+'-exit.json',{exitCode:p.exitCode,signal:p.signalCode,spawnError:p.spawnError?String(p.spawnError.message):null});
  assert(!p.spawnError&&p.exitCode===0&&p.signalCode===null,'Geth init did not exit zero');await stopChild();closeChildLog();
}
function genesis(fork,fixture){
  assert(fixture.genesisInput.config.chainId===1337&&fixture.genesisInput.config.evmFork===fork);
  assert.equal(quantity(fixture.header.gasLimit),30000000n);assert.equal(quantity(fixture.header.baseFeePerGas),1000000000n);assert.equal(quantity(fixture.header.timestamp),0n);
  const config={chainId:1337,homesteadBlock:0,eip150Block:0,eip155Block:0,eip158Block:0,byzantiumBlock:0,constantinopleBlock:0,petersburgBlock:0,istanbulBlock:0,muirGlacierBlock:0,berlinBlock:0,londonBlock:0,arrowGlacierBlock:0,grayGlacierBlock:0,terminalTotalDifficulty:0,mergeNetsplitBlock:0,ethash:{}};
  if(fork==='Shanghai')config.shanghaiTime=0;else assert.equal(fork,'Paris');
  const alloc=structuredClone(fixture.genesisInput.alloc);
  for(const [a,x]of Object.entries(alloc)){address(a);quantity(x.balance??'0x0');quantity(x.nonce??'0x0');data(x.code??'0x');assert(!x.storage||Object.keys(x.storage).length===0,'unsupported nonempty fixture storage');}
  return {config,nonce:'0x0',timestamp:'0x0',extraData:'0x',gasLimit:'0x1c9c380',difficulty:'0x0',mixHash:'0x'+'00'.repeat(32),coinbase:'0x'+'00'.repeat(20),baseFeePerGas:'0x3b9aca00',alloc};
}
async function verifyAlloc(fixture){
  for(const [a,x]of Object.entries(fixture.genesisInput.alloc)){
    assert.equal(quantity(await value('eth_getBalance',[a,'latest'])),quantity(x.balance??'0x0'));assert.equal(quantity(await value('eth_getTransactionCount',[a,'latest'])),quantity(x.nonce??'0x0'));assert.equal(data(await value('eth_getCode',[a,'latest'])),data(x.code??'0x'));
    assert.equal(data(await value('eth_getStorageAt',[a,'0x0','latest'])),'0x'+'00'.repeat(32));
  }
}
async function startFork(fork,fixture){
  check();const observation={fork,observedClientVersion:null,lastReadinessError:null,attempts:0,ready:false};readinessObservations.push(observation);
  const label='geth-'+fork.toLowerCase(),datadir=path.join(out,label+'-data');fs.mkdirSync(datadir,{mode:0o700});
  const genesisPath=saveText(label+'-genesis.json',JSON.stringify(genesis(fork,fixture),null,2)+'\n');
  const jwtPath=saveText(label+'-public-jwt.hex',PUBLIC_JWT_HEX);
  const configPath=saveText(label+'-config.toml',`[Eth]
NetworkId = 1337
EthDiscoveryURLs = []
SnapDiscoveryURLs = []
[Node]
IPCPath = ""
WSHost = ""
USB = false
ExternalSigner = ""
[Node.P2P]
MaxPeers = 0
NoDial = true
NoDiscovery = true
DiscoveryV4 = false
DiscoveryV5 = false
ListenAddr = ""
DiscAddr = ""
BootstrapNodes = []
BootstrapNodesV5 = []
StaticNodes = []
TrustedNodes = []
[Node.HTTPTimeouts]
ReadTimeout = 3000000000
ReadHeaderTimeout = 3000000000
WriteTimeout = 5000000000
IdleTimeout = 5000000000
`);
  const common=['--config',configPath,'--datadir',datadir,'--nodekeyhex',NODE_KEY_HEX,'--authrpc.jwtsecret',jwtPath,'--ipcdisable','--discovery.v4=false','--discovery.v5=false','--bootnodes','','--discovery.dns','','--nat','none','--maxpeers','0','--networkid','1337','--state.scheme','hash','--cache','64','--verbosity','2'];
  await initGenesis(label+'-init',[...common,'init',genesisPath]);
  const n=await port();const p=spawnOwned(label+'-run',[...common,'--syncmode','full','--gcmode','archive','--snapshot=false','--rpc.gascap','30000000','--rpc.evmtimeout','2s','--http','--http.addr','127.0.0.1','--http.port',String(n),'--http.api','eth,web3','--http.vhosts','localhost,127.0.0.1','--authrpc.addr','127.0.0.1','--authrpc.port','0','--authrpc.vhosts','localhost,127.0.0.1'],n);
  const readyUntil=Math.min(deadline,performance.now()+15000);let version,ready=false;
  while(performance.now()<readyUntil){check();if(p.spawnError)throw p.spawnError;if(p.exitCode!==null||p.signalCode!==null)throw Error('Geth exited before readiness');try{
    ++observation.attempts;version=await value('web3_clientVersion',[]);observation.observedClientVersion=typeof version==='string'?version.slice(0,256):null;
    const match=typeof version==='string'&&version.length<=256&&/^Geth\/v1\.16\.0-stable-57f8971b(?:-[0-9]{8})?\/linux-(arm64|amd64)\/go[0-9]+(?:\.[0-9]+){1,2}$/.exec(version);
    assert(match,'unexpected Geth 1.16.0 stable/57f8971b client version');assert.equal(match[1],process.arch==='arm64'?'arm64':'amd64');assert.equal(quantity(await value('eth_chainId',[])),1337n);ready=true;observation.ready=true;break;
  }catch(e){observation.lastReadinessError=String(e.message).slice(0,2048);await new Promise(r=>setTimeout(r,25));}}
  assert(ready,'Geth 1.16.0 readiness: '+JSON.stringify(observation));
  const header=await value('eth_getBlockByNumber',['0x0',false]);assert.equal(quantity(await value('eth_blockNumber',[])),0n);assert.equal(quantity(header.number),0n);assert.equal(quantity(header.difficulty),0n);assert.equal(quantity(header.gasLimit),30000000n);assert.equal(quantity(header.baseFeePerGas),1000000000n);assert.equal(quantity(header.timestamp),0n);
  assert.equal(data(header.miner),'0x'+'00'.repeat(20));assert.equal(data(header.mixHash),'0x'+'00'.repeat(32));assert(!header.blobGasUsed&&!header.excessBlobGas,'future blob fork must remain inactive');
  await verifyAlloc(fixture);
  return {fork,clientVersion:version,chainId:'1337',blockNumber:'0x0',difficulty:'0x0',baseFeePerGas:header.baseFeePerGas,gasLimit:header.gasLimit,genesisHeader:header,allocationsVerified:Object.keys(fixture.genesisInput.alloc).length,nanoFixtureDifficulty:fixture.header.difficulty,difficultyAdaptedForPostMergeContext:true,p2pDisabledByConfiguration:true,devOrMiningEnabled:false};
}
async function compare(record,index){
  const actual=await rpc(record.method,record.params),expected=record.response;
  const result={index,fork:record.fork,method:record.method,params:record.params,candidate:expected,geth:actual,candidateClass:category(expected),gethClass:category(actual),differences:[],replays:[]};
  const difference=(kind,detail,qualitative=true)=>{const d={index,fork:record.fork,method:record.method,kind,detail,affectsQualitativeComparison:qualitative};result.differences.push(d);differences.push(d);};
  try{
  if(result.candidateClass!==result.gethClass||result.candidateClass==='unclassifiedRpcError')difference('resultClass',{candidate:result.candidateClass,geth:result.gethClass});
  else if(result.candidateClass==='success'){
    if(record.method==='eth_call'){if(data(actual.result)!==data(expected.result))difference('returnData',{candidate:expected.result,geth:actual.result});}
    else if(record.method==='eth_estimateGas'){
      const c=quantity(expected.result),a=quantity(actual.result);const requested=record.params[0].gas;
      const cap=requested!=null&&quantity(requested)>=21000n?quantity(requested)<30000000n?quantity(requested):30000000n:30000000n;
      if(c<=0n||a<=0n||c>cap||a>cap)difference('estimateBounds',{candidate:String(c),geth:String(a),cap:String(cap)});
      for(const gas of new Set([expected.result,actual.result])){const replay=await rpc('eth_call',[{...record.params[0],gas},'latest']);result.replays.push({gas,response:replay});if(category(replay)!=='success')difference('estimateReplay',{gas,response:replay});}
      if(c!==a){++quoteDifferences;difference('estimateGasQuote',{candidate:expected.result,geth:actual.result},false);}
    }else{
      const e=expected.result,a=actual.result;
      if(canonical(accessList(e.accessList))!==canonical(accessList(a.accessList)))difference('accessList',{candidate:e.accessList,geth:a.accessList});
      if(quantity(e.gasUsed)!==quantity(a.gasUsed))difference('accessListGasUsed',{candidate:e.gasUsed,geth:a.gasUsed});
      if(Object.hasOwn(e,'error')!==Object.hasOwn(a,'error'))difference('accessListExecution',{candidate:e.error??null,geth:a.error??null});
      else if(Object.hasOwn(e,'error')){const ec=errorClass({code:0,message:e.error}),ac=errorClass({code:0,message:a.error});if(ec!==ac||ec==='unclassifiedRpcError')difference('accessListExecutionClass',{candidate:ec,geth:ac,candidateMessage:e.error,gethMessage:a.error});}
    }
  }
  }catch(e){difference('comparisonValueError',String(e.message).slice(0,2048));}
  const policy=feePolicy(record)&&result.gethClass==='success'&&result.differences.length===1&&result.differences[0].kind==='resultClass';if(policy)++policyDifferences;
  result.documentedAccessListZeroFeePolicy=policy;
  result.passed=result.differences.every(d=>!d.affectsQualitativeComparison)||policy;
  return result;
}

assert.equal(process.platform,'linux');assert(['arm64','x64'].includes(process.arch),'Linux arm64/amd64 Node required');
assert.equal(process.argv.length,3,'usage: GETH=/absolute/geth OUT=/new/dir node runner.mjs candidate-events.jsonl');
const input=path.resolve(process.argv[2]),geth=process.env.GETH,out=process.env.OUT;
assert(geth&&path.isAbsolute(geth)&&fs.realpathSync(geth)===geth,'physical GETH path');assert(out&&path.isAbsolute(out));
assert.equal(fs.realpathSync(path.dirname(out)),path.dirname(out),'physical output parent');
const heldInput=material(input,4*1048576),heldGeth=material(geth,128*1048576);
const events=loadEvents(heldInput.bytesValue);delete heldInput.bytesValue;delete heldGeth.bytesValue;
fs.mkdirSync(out,{mode:0o700});trace=fs.openSync(path.join(out,'comparisons.jsonl'),'wx',0o600);
try{
  save('inputs-public.json',{format:'nano-rpc-context-geth-input-pins-v1',events:publicPin(heldInput),geth:publicPin(heldGeth),expectedCases:258,profiles:['Paris','Shanghai'],sdkOrHistoricalAuthorityRecovered:false});
  for(const fork of ['Paris','Shanghai']){
    const fixture=events[0].fixtures[fork];for(const event of events)assert.equal(canonical(event.fixtures[fork]),canonical(fixture),'fixture must be identical across groups');
      const setup=await startFork(fork,fixture);const before=await value('eth_getBlockByNumber',['latest',false]);let count=0;
      for(const event of events)for(const row of event.caseRecords)if(row.fork===fork){check();let r;try{r=await compare(row,compared);}catch(e){r={index:compared,fork,method:row.method,params:row.params,candidate:row.response,comparisonError:String(e.message).slice(0,2048),passed:false};}emitRecord(r);if(!r.passed)++unexpected;++compared;++count;}
      assert.equal(count,129);const after=await value('eth_getBlockByNumber',['latest',false]);assert.equal(before.hash,after.hash,'replay must not mine or change header');
      // Readback only. No reset, repair, transaction, mine or snapshot RPC exists.
      assert.equal(quantity(await value('eth_blockNumber',[])),0n);assert.equal(canonical(before),canonical(after),'complete observed genesis header changed');await verifyAlloc(fixture);
      forks.push({...setup,cases:count,headerUnchanged:true,allocatedAccountsUnchanged:true});fs.fsyncSync(trace);
      await stopChild();closeChildLog();
  }
  assert.equal(compared,258);
}catch(e){failure=String(e.message).slice(0,4096);process.exitCode=1;}
finally{
  try{await stopChild();}catch(e){cleanupErrors.push(String(e.message));failure??='owned child cleanup failed';process.exitCode=1;}
  try{closeChildLog();}catch(e){cleanupErrors.push(String(e.message));failure??='child log close failed';process.exitCode=1;}
  try{same(input,heldInput,4*1048576);same(geth,heldGeth,128*1048576);for(const c of heldControls)same(c.path,c.held,524288);}catch(e){failure??='input/program final fence: '+String(e.message);process.exitCode=1;}
  try{closeDurably(trace);}catch(e){failure??='comparison trace close failed';process.exitCode=1;}finally{trace=undefined;}
  const completed=!failure&&compared===258&&forks.length===2,passed=completed&&unexpected===0&&cleanupErrors.length===0;
  const summary={format:'nano-rpc-context-geth-oracle-v1',passed,completed,compared,expectedCases:258,forks,readinessObservations,unexpectedComparisons:unexpected,documentedAccessListZeroFeePolicyComparisons:policyDifferences,estimateQuoteDifferences:quoteDifferences,exactAllComparedValuesEqual:passed&&differences.length===0,differences,firstFailure:failure,cleanupErrors,elapsedMs:Math.round(performance.now()-start),maximumRunMs:MAX_MS,productionQualified:false,globalSdkQualified:false,authorExecuted:false};
  save('result-public.json',summary);console.log(JSON.stringify(summary));if(!passed)process.exitCode=1;
}
