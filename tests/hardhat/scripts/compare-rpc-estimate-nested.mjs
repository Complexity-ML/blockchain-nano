import {spawn,execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../..');
const {Wallet,keccak256}=createRequire(path.join(root,'tests/hardhat/package.json'))('ethers');
const binary=process.env.NANO_BINARY,anvil=process.env.ANVIL_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;
assert(binary&&anvil&&out,'NANO_BINARY, ANVIL_BINARY, NANO_DIAGNOSTIC_REPORT_DIR required');
fs.mkdirSync(out,{recursive:false});
const version=execFileSync(anvil,['--version'],{encoding:'utf8'}).trim();
const q=n=>'0x'+BigInt(n).toString(16),key=n=>'0x'+BigInt(n).toString(16).padStart(64,'0');
async function rpc(url,method,params){const r=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params}),signal:AbortSignal.timeout(10000)});assert(r.ok);return r.json();}
async function ok(url,method,params){const r=await rpc(url,method,params);assert(!r.error,JSON.stringify(r));return r.result;}
function start(exe,args,env,log){const child=spawn(exe,args,{env:{...process.env,...env},stdio:['ignore','pipe','pipe']});let text='';const stream=fs.createWriteStream(log);child.stdout.on('data',d=>{text+=d;stream.write(d);});child.stderr.on('data',d=>stream.write(d));child.once('close',()=>stream.end());const ready=new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Startup timeout')),10000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',()=>{clearTimeout(timer);reject(Error('Exited before ready'));});child.stdout.on('data',()=>{const m=text.match(/listening on 127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(timer);resolve('http://127.0.0.1:'+m[1]);}});});return {child,ready};}
async function stop(child){if(child.exitCode!==null||child.signalCode!==null)return;const done=new Promise(r=>child.once('exit',r));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await done;clearTimeout(timer);}


const transactionTypes=(process.env.NANO_DIAGNOSTIC_TX_TYPES??'0').split(',').map(Number);
assert(transactionTypes.length&&transactionTypes.every(t=>[0,1,2].includes(t))&&new Set(transactionTypes).size===transactionTypes.length,'Expected unique transaction types 0,1,2');
// Independent BigInt oracle for post-London EIP-1559 (elasticity 2, denominator 8).
function nextFee(parent){
 const fee=BigInt(parent.baseFeePerGas),used=BigInt(parent.gasUsed),target=BigInt(parent.gasLimit)/2n;
 assert(target>0n);
 if(used===target)return fee;
 const change=fee*(used>target?used-target:target-used)/target/8n;
 return used>target?fee+(change>0n?change:1n):fee-change;
}
const results=[],feeTraces=[];
const leaf='0x00000000000000000000000000000000000040ad',calldata='0x'+(600000n).toString(16).padStart(64,'0');
const leafCode='0x60016000556000355b8015601557600190036008565b50602a60005260206000f3',expected='0x'+'2a'.padStart(64,'0');
try{for(const fork of ['Paris','Shanghai']){
 const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-estimate-nested-')),children=[];
 try{
  const wallet=new Wallet(key(10)),validator=new Wallet(key(1)).address.toLowerCase(),balance=10n**24n;
  const alloc={[validator]:{balance:q(balance)},[wallet.address.toLowerCase()]:{balance:q(balance)},[leaf]:{code:leafCode}},targets={};let target=leaf;
  for(let depth=1;depth<=4;depth++){const outer='0x'+(0x40ad+depth).toString(16).padStart(40,'0');alloc[outer]={code:'0x3660006000376020600060206000600073'+target.slice(2)+'5af1602f5760006000fd5b60206000f3'};target=outer;targets[depth]=target;}
  const genesis={gasLimit:q(30000000),config:{chainId:1337,consensus:'pos',posSignatureVersion:1,bftVersion:1,bootstrapValidator:validator,evmFork:fork},alloc};
  fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(out,fork+'-genesis.json'),JSON.stringify(genesis,null,2));
  fs.writeFileSync(path.join(temp,'node.json'),JSON.stringify({bft:{key_env:'TEST_BFT_KEY',journal:path.join(temp,'votes')}}));
  const n=start(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0','127.0.0.1',path.join(temp,'node.json')],{TEST_BFT_KEY:key(1)},path.join(out,fork+'-nano.log'));children.push(n.child);const nu=await n.ready;
  const a=start(anvil,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--base-fee','1000000000','--gas-limit','30000000','--timestamp','0','--no-mining'],{},path.join(out,fork+'-anvil.log'));children.push(a.child);const au=await a.ready;
  await ok(au,'anvil_setBalance',[wallet.address,q(balance)]);for(const [addr,value] of Object.entries(alloc))if(value.code)await ok(au,'anvil_setCode',[addr,value.code]);
  async function receipt(url,hash){for(let i=0;i<200;i++){const r=await ok(url,'eth_getTransactionReceipt',[hash]);if(r)return r;await new Promise(r=>setTimeout(r,100));}throw Error('receipt timeout');}
  let nonce=0,lastChecked=await ok(nu,'eth_getBlockByNumber',['0x0',false]);
  for(const type of transactionTypes)for(const blockTag of ['latest','pending'])for(const depth of [1,4])for(const quoteSource of ['nano','anvil']){
   const fees=type===2?{maxFeePerGas:2000000000n,maxPriorityFeePerGas:1000000000n}:{gasPrice:2000000000n};
   const accessList=type===0?undefined:[{address:leaf,storageKeys:['0x'+'0'.repeat(64)]}];
   const request={from:wallet.address,to:targets[depth],data:calldata,gas:q(30000000),type:q(type),...Object.fromEntries(Object.entries(fees).map(([k,v])=>[k,q(v)])),...(accessList?{accessList}:{})};
   async function canonicalState(url){return {storage:await ok(url,'eth_getStorageAt',[leaf,'0x0','latest']),balance:await ok(url,'eth_getBalance',[wallet.address,'latest']),nonce:await ok(url,'eth_getTransactionCount',[wallet.address,'latest'])};}
   const before=await Promise.all([nu,au].map(canonicalState));
   const estimates={nano:await ok(nu,'eth_estimateGas',[request,blockTag]),anvil:await ok(au,'eth_estimateGas',[request,blockTag])};
   const quoted=estimates[quoteSource];assert(BigInt(quoted)>23000000n&&BigInt(quoted)<=30000000n);
   // Each provider's quote must actually execute on both engines.
   for(const gas of Object.values(estimates))for(const url of [nu,au])assert.equal(await ok(url,'eth_call',[{...request,gas},blockTag]),expected);
   assert.deepEqual(await Promise.all([nu,au].map(canonicalState)),before,'estimation/calls mutated canonical state');
   const raw=await wallet.signTransaction({type,chainId:1337,nonce:nonce++,to:targets[depth],data:calldata,gasLimit:BigInt(quoted),...fees,...(accessList?{accessList}:{})}),hash=keccak256(raw);
   assert.equal(await ok(nu,'eth_sendRawTransaction',[raw]),hash);const nr=await receipt(nu,hash),nb=await ok(nu,'eth_getBlockByHash',[nr.blockHash,false]);
   const checkedBlocks=[];
   for(let height=BigInt(lastChecked.number)+1n;height<=BigInt(nb.number);height++){
    const current=await ok(nu,'eth_getBlockByNumber',[q(height),false]);
    assert.equal(current.parentHash,lastChecked.hash);assert.equal(BigInt(current.baseFeePerGas),nextFee(lastChecked),'Nano base fee violates independent parent-derived formula');
    checkedBlocks.push({parent:lastChecked,block:current});lastChecked=current;
   }
   const feeHistory=await ok(nu,'eth_feeHistory',['0x1',nb.number,[0,50,100]]);
   assert.deepEqual(feeHistory.baseFeePerGas,[nb.baseFeePerGas,q(nextFee(nb))]);
   assert.equal(feeHistory.oldestBlock,nb.number);
   assert.equal(feeHistory.gasUsedRatio[0],Number(BigInt(nb.gasUsed))/Number(BigInt(nb.gasLimit)));
   const effective=type===2?(BigInt(nb.baseFeePerGas)+fees.maxPriorityFeePerGas<fees.maxFeePerGas?BigInt(nb.baseFeePerGas)+fees.maxPriorityFeePerGas:fees.maxFeePerGas):fees.gasPrice;
   assert.equal(BigInt(nr.effectiveGasPrice),effective);
   assert.deepEqual(feeHistory.reward,[[q(effective-BigInt(nb.baseFeePerGas)),q(effective-BigInt(nb.baseFeePerGas)),q(effective-BigInt(nb.baseFeePerGas))]]);
   assert.equal(BigInt(await ok(nu,'eth_getBalance',[wallet.address,nb.number])),BigInt(before[0].balance)-BigInt(nr.gasUsed)*effective);
   const anvilParent=await ok(au,'eth_getBlockByNumber',['latest',false]);
   feeTraces.push({fork,type,hash,checkedBlocks,feeHistory,anvilParent,anvilNaturalNextFee:q(nextFee(anvilParent)),nanoMinedFee:nb.baseFeePerGas});
   await ok(au,'evm_setNextBlockTimestamp',[Number(BigInt(nb.timestamp))]);await ok(au,'anvil_setNextBlockBaseFeePerGas',[nb.baseFeePerGas]);assert.equal(await ok(au,'eth_sendRawTransaction',[raw]),hash);await ok(au,'evm_mine',[]);const ar=await receipt(au,hash);
   assert.equal(nr.status,'0x1');assert.equal(ar.status,'0x1');assert.equal(nr.gasUsed,ar.gasUsed);assert.equal(nr.effectiveGasPrice,ar.effectiveGasPrice);const ab=await ok(au,'eth_getBlockByHash',[ar.blockHash,false]);assert.equal(nb.baseFeePerGas,ab.baseFeePerGas);assert.deepEqual(nr.logs,ar.logs);assert.equal(nr.transactionHash,ar.transactionHash);
   const states=[];for(const url of [nu,au])states.push({storage:await ok(url,'eth_getStorageAt',[leaf,'0x0','latest']),balance:await ok(url,'eth_getBalance',[wallet.address,'latest']),nonce:await ok(url,'eth_getTransactionCount',[wallet.address,'latest'])});assert.deepEqual(states[0],states[1]);assert.equal(BigInt(states[0].storage),1n);
   results.push({fork,type,request,baseFeePerGas:nb.baseFeePerGas,blockTag,depth,quoteSource,simulationStateUnchanged:true,estimates,raw,hash,nano:nr,anvil:ar,state:states[0],matches:true});
  }
 }finally{for(const child of children.reverse())await stop(child);fs.rmSync(temp,{recursive:true,force:true});}
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion:version,feeTraces,scope:'Verified quotes and same-signed transactions in latest/pending contexts through one/four nested calls on local Nano and Anvil. Quote values need not be identical; execution status, gas consumed, logs, balances, nonce and storage must match.',results},null,2)+'\n');}
assert.equal(results.length,16*transactionTypes.length);console.log(`${results.length}/${16*transactionTypes.length} signed nested-computation comparisons passed; both providers quotes execute on both engines`);
