import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {createInterface} from 'node:readline';
import {createRequire} from 'node:module';
import {createHash} from 'node:crypto';
import {runScenario} from './scenario.mjs';

const require = createRequire(import.meta.url);
const ethers = require(path.join(process.env.BRIDGE_DEPENDENCIES,'ethers'));
const solc = require(path.join(process.env.BRIDGE_DEPENDENCIES,'solc'));
const out = process.env.BRIDGE_OUTPUT;
assert(out && fs.statSync(out).isDirectory());
assert([undefined,'anvil-only','native-only'].includes(process.env.BRIDGE_MODE),'Unknown BRIDGE_MODE');
const stringify = x => JSON.stringify(x, (_, v) => typeof v === 'bigint' ? v.toString() : v, 2);
const save = (name, x) => fs.writeFileSync(path.join(out,name), stringify(x)+'\n',{flag:'wx',mode:0o600});
const hash = b => createHash('sha256').update(b).digest('hex');
assert.equal(solc.version().split('+')[0],'0.8.28');
assert.equal(ethers.version,'6.16.0');
const sourceRoot=process.env.BRIDGE_SOURCE;
const imported = {};
function source(name) {
  assert(!name.split('/').includes('..') && !name.startsWith('/'));
  let p;
  if(name.startsWith('@openzeppelin/contracts/')) p=path.join(process.env.BRIDGE_OZ,name.slice('@openzeppelin/contracts/'.length));
  else p=path.join(sourceRoot,name);
  const b=fs.readFileSync(p); imported[name]={sha256:hash(b),bytes:b.length};return b.toString();
}
const entry='tests/hardhat/contracts/BridgeHarness.sol';
const compilerInput={language:'Solidity',sources:{[entry]:{content:source(entry)}},settings:{optimizer:{enabled:true,runs:200},evmVersion:'paris',metadata:{bytecodeHash:'none'},outputSelection:{'*':{'*':['abi','evm.bytecode.object','evm.deployedBytecode.object']}}}};
const compiled=JSON.parse(solc.compile(JSON.stringify(compilerInput),{import:name=>{try{return {contents:source(name)}}catch(e){return {error:String(e)}}}}));
save('compiler-diagnostics.json',compiled.errors||[]);
assert(!(compiled.errors||[]).some(e=>e.severity==='error'),'Solidity compilation failed');
const artifacts=compiled.contracts[entry];
assert(artifacts && artifacts.BridgeToken && artifacts.MockProofVerifier);
save('compiler-inputs.json',{version:solc.version(),ethers:ethers.version,node:process.version,architecture:process.arch,platform:process.platform,settings:compilerInput.settings,inputs:imported,soljsonSha256:hash(fs.readFileSync(path.join(process.env.BRIDGE_DEPENDENCIES,'solc/soljson.js')))});
save('artifacts.json',artifacts);
const addresses=['0x7e5f4552091a69125d5dfcb7b8c2659029395bdf','0x2b5ad5c4795c026514f8317c7a215e218dccd6cf','0x6813eb9362372eef6200f3b1dbc3f819671cba69'];
const children=new Set(), logFiles=[];
let seq=0;
const ioTrace=fs.createWriteStream(path.join(out,'transactions.jsonl'),{flags:'wx',mode:0o600});
function record(x){ioTrace.write(JSON.stringify(x,(_,v)=>typeof v==='bigint'?v.toString():v)+'\n');}
function child(binary,args,tag) {
  const log=fs.openSync(path.join(out,tag+'.log'),'wx',0o600);logFiles.push(log);
  const p=spawn(binary,args,{stdio:['pipe','pipe',log],env:{PATH:'/usr/bin:/bin',HOME:'/tmp',LANG:'C',LC_ALL:'C'}});
  children.add(p);p.once('error',error=>{p.bridgeSpawnError=error;children.delete(p);});p.once('exit',()=>children.delete(p)); return {p,log};
}
async function stopChild(p) {
  if(p.exitCode!==null||p.signalCode!==null||p.bridgeSpawnError)return;
  const joined=new Promise(resolve=>p.once('exit',resolve));
  p.kill('SIGTERM');
  let timer;const ended=await Promise.race([joined.then(()=>true),new Promise(resolve=>{timer=setTimeout(()=>resolve(false),3000);})]);clearTimeout(timer);
  if(!ended){p.kill('SIGKILL');let killTimer;try{await Promise.race([joined,new Promise((_,reject)=>{killTimer=setTimeout(()=>reject(Error('Owned child failed to exit after KILL')),3000);})]);}finally{clearTimeout(killTimer);}}
}
const iface=name=>new ethers.Interface(artifacts[name].abi);
function txResult(r){
  return {success:r.success ?? (BigInt(r.status)===1n),gasUsed:BigInt(r.gasUsed??r.gas_used).toString(),logs:r.logs||[],contractAddress:r.contractAddress??r.contract_address,transactionHash:r.transactionHash??r.transaction_hash};
}
async function anvilBackend(){
  const ports=[19547,19548],rpc=[];
  for(let c=0;c<2;c++){
    const {p,log}=child(process.env.BRIDGE_ANVIL,['--host','127.0.0.1','--port',String(ports[c]),'--chain-id',String(31337+c),'--accounts','0','--hardfork','paris','--silent'],'anvil-'+c);
    p.stdout.on('data',b=>fs.writeSync(log,b));
    const send=async(method,params=[])=>{
      const abort=AbortSignal.timeout(10000);
      const response=await fetch('http://127.0.0.1:'+ports[c],{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:++seq,method,params}),signal:abort});
      assert(response.ok);const result=await response.json();if(result.error) throw Error(JSON.stringify(result.error));return result.result;
    };
    const deadline=Date.now()+10000;let ready=false;
    while(Date.now()<deadline){if(p.bridgeSpawnError)throw p.bridgeSpawnError;if(p.exitCode!==null||p.signalCode!==null)throw Error('Anvil exited before readiness');try{assert.equal(BigInt(await send('eth_chainId')),BigInt(31337+c));ready=true;break;}catch{await new Promise(r=>setTimeout(r,50));}}
    assert(ready,'Anvil readiness timeout');rpc.push(send);
    for(const a of addresses){await send('anvil_impersonateAccount',[a]);await send('anvil_setBalance',[a,'0xd3c21bcecceda1000000']);}
  }
  async function sendRaw(c,to,data,from=0){
    const params={from:addresses[from],data,gas:'0x1c9c380',value:'0x0'};if(to)params.to=to;
    const h=await rpc[c]('eth_sendTransaction',[params]);let r;
    const deadline=Date.now()+10000;
    while(Date.now()<deadline){r=await rpc[c]('eth_getTransactionReceipt',[h]);if(r)break;await new Promise(r=>setTimeout(r,10));}
    assert(r,'Missing actual transaction receipt');const z=txResult(r);assert(BigInt(z.gasUsed)<30000000n,'Unexpected full-frame failure');record({backend:'anvil',chain:c,to,hash:h,...z});return z;
  }
  return {name:'anvil-paris-two-chain',
    async deploy(c,name,args){const f=iface(name),z=await sendRaw(c,null,'0x'+artifacts[name].evm.bytecode.object+f.encodeDeploy(args).slice(2));if(!z.success){const e=Error('Deployment '+name+' reverted');e.constructorReverted=true;throw e;}assert(z.contractAddress);return z.contractAddress;},
    async read(c,a,n,m,args=[]){return iface(n).decodeFunctionResult(m,await rpc[c]('eth_call',[{to:a,data:iface(n).encodeFunctionData(m,args)},'latest']));},
    send(c,a,n,m,args=[],from=0){return sendRaw(c,a,iface(n).encodeFunctionData(m,args),from);},
    async events(c,a){return rpc[c]('eth_getLogs',[{address:a,fromBlock:'0x0',toBlock:'latest'}]);},
    async reload(c){const before=await rpc[c]('eth_blockNumber');await rpc[c]('anvil_mine',['0x1']);return {sameRoot:false,notStateReload:true,before,after:await rpc[c]('eth_blockNumber')};},
  };
}
async function nativeBackend(){
  const {p}=child(process.env.BRIDGE_NATIVE,[],'native-engine');
  const lines=createInterface({input:p.stdout});const pending=new Map();let count=0;
  lines.on('line',line=>{try{const z=JSON.parse(line),w=pending.get(z.id);assert(w,'Unexpected native response');pending.delete(z.id);clearTimeout(w.t);!z.ok?w.reject(Error(String(z.error))):w.resolve(z.result);}catch(e){for(const w of pending.values()){clearTimeout(w.t);w.reject(e);}pending.clear();}});
  p.once('exit',(code,signal)=>{for(const w of pending.values()){clearTimeout(w.t);w.reject(Error('Native runner exit '+code+'/'+signal));}pending.clear();});
  p.once('error',error=>{for(const w of pending.values()){clearTimeout(w.t);w.reject(error);}pending.clear();});
  p.stdin.on('error',error=>{for(const w of pending.values()){clearTimeout(w.t);w.reject(error);}pending.clear();});
  function request(c,action,fields={}){assert(count<5000);if(p.bridgeSpawnError)throw p.bridgeSpawnError;return new Promise((resolve,reject)=>{const id=++count;const t=setTimeout(()=>{pending.delete(id);reject(Error('Native request timeout '+id));},30000);pending.set(id,{resolve,reject,t});p.stdin.write(JSON.stringify({id,chain:c,action,...fields})+'\n');});}
  async function sendRaw(c,to,data,from=0){const z=txResult(await request(c,'send',{from:addresses[from],...(to?{to}:{}),data,gas:30000000,value:'0x0'}));assert(BigInt(z.gasUsed)<30000000n,'Unexpected full-frame failure');record({backend:'native',chain:c,to,...z});return z;}
  return {name:'nano-native-paris-two-state',
    async deploy(c,n,args){const f=iface(n),z=await sendRaw(c,null,'0x'+artifacts[n].evm.bytecode.object+f.encodeDeploy(args).slice(2));if(!z.success){const e=Error('Native deployment '+n+' reverted');e.constructorReverted=true;throw e;}assert(z.contractAddress);return z.contractAddress;},
    async read(c,a,n,m,args=[]){const r=await request(c,'call',{from:addresses[0],to:a,data:iface(n).encodeFunctionData(m,args),gas:30000000});assert(r.success,'Read failed '+n+'.'+m+': '+r.error);return iface(n).decodeFunctionResult(m,r.returnData);},
    send(c,a,n,m,args=[],from=0){return sendRaw(c,a,iface(n).encodeFunctionData(m,args),from);},
    async events(c,a){const r=await request(c,'events',{address:a});return r.logs;},
    async reload(c){const z=await request(c,'reload');return {...z,sameRoot:z.unchanged&&z.stateRootBefore===z.stateRoot};},
    async finish(){assert.equal(pending.size,0);const joined=new Promise(resolve=>p.once('exit',(code,signal)=>resolve({code,signal})));p.stdin.end();let timer;try{const z=await Promise.race([joined,new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('Native EOF timeout')),5000);})]);assert.equal(z.code,0);assert.equal(z.signal,null);return {exitCode:z.code,requests:count};}finally{clearTimeout(timer);}},
  };
}
let failure=null,anvilReport=null;
try{
  if(process.env.BRIDGE_MODE!=='native-only'){
    const b=await anvilBackend();const report=await runScenario({backend:b,artifacts,ethers});anvilReport=report;save('anvil-result.json',report);
    console.log('Anvil scenario complete:',report);
    // Terminate only these two owned Anvil children before starting native EVM.
    for(const p of [...children])await stopChild(p);
  }
  if(process.env.BRIDGE_MODE!=='anvil-only'){
    const b=await nativeBackend();const report=await runScenario({backend:b,artifacts,ethers});report.runner=await b.finish();save('native-result.json',report);console.log('Native scenario complete:',report);
    if(anvilReport){assert.deepEqual(report.cases,anvilReport.cases);assert.equal(report.expectedRefusals,anvilReport.expectedRefusals);assert.deepEqual(report.conservedBeforeCorruption,anvilReport.conservedBeforeCorruption);save('differential.json',{passed:true,sameCases:report.cases.length,sameExpectedRefusals:report.expectedRefusals,samePreCorruptionAccounting:true,gasEqualityClaimed:false,wholeEvmConformanceClaimed:false});}
  }
}catch(e){failure=e;save('failure.json',{message:String(e),stack:e.stack,scenarioProgress:e.scenarioProgress??null});console.error(e.stack);process.exitCode=1;}
finally{
  const cleanupErrors=[];
  for(const p of [...children]){try{await stopChild(p);}catch(e){cleanupErrors.push(String(e));failure ||= e;process.exitCode=1;}}
  for(const fd of logFiles)fs.closeSync(fd);
  await new Promise(resolve=>ioTrace.end(resolve));
  save('terminal.json',{passed:!failure&&children.size===0,ownedChildrenRemaining:children.size,cleanupErrors,productionQualified:false,authenticatedBridgeQualified:false});
}
