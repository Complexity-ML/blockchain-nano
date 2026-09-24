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
const rebroadcast=process.env.NANO_TEST_REBROADCAST_SUFFIX==='1';
const results=[],transactions=[];
try{for(const fork of ['Paris','Shanghai'])for(const type of [0,1,2]){
 const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-pool-reference-')),children=[];
 try{
  const wallet=new Wallet(key(1)),producer=new Wallet(key(99)),sender=wallet.address.toLowerCase(),to=new Wallet(key(98)).address,tag=fork+'-'+type;
  const genesis={config:{chainId:1337,consensus:'pos',posSignatureVersion:1,bootstrapValidator:producer.address,evmFork:fork},alloc:{[sender]:{balance:q(10n**24n)}}};
  fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(out,tag+'-genesis.json'),JSON.stringify(genesis,null,2));
  const keyfile=path.join(temp,'producer.json');fs.writeFileSync(keyfile,await producer.encrypt('local-test-password'));
  const cfg=path.join(temp,'node.json');fs.writeFileSync(cfg,JSON.stringify({dev_mode:true,coinbase:producer.address,wallets:[{path:keyfile,password_env:'NANO_FIXTURE_PASSWORD'}]}));
  const n=start(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0','127.0.0.1',cfg],{NANO_FIXTURE_PASSWORD:'local-test-password'},path.join(out,tag+'-nano.log'));children.push(n.child);const nu=await n.ready;
  const a=start(anvil,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--base-fee','1000000000','--no-mining'],{},path.join(out,tag+'-anvil.log'));children.push(a.child);const au=await a.ready;
  await ok(au,'anvil_setBalance',[sender,q(10n**24n)]);
  await ok(au,'anvil_setCoinbase',[producer.address]);
  const make=async(nonce,fraction)=>wallet.signTransaction({type,chainId:1337,nonce,to,value:10n**24n*BigInt(fraction)/100n,gasLimit:21000n,...(type===2?{maxFeePerGas:2000000000n,maxPriorityFeePerGas:1000000000n}:{gasPrice:2000000000n}),...(type>0?{accessList:[]}:{})});
  const raws=await Promise.all([make(1,60),make(2,5),make(0,60),make(1,5)]),hashes=raws.map(keccak256);
  for(const raw of raws.slice(0,3))for(const url of [nu,au])assert.equal(await ok(url,'eth_sendRawTransaction',[raw]),keccak256(raw));
  async function mineBoth(){await ok(nu,'evm_mine',[]);const block=await ok(nu,'eth_getBlockByNumber',['latest',false]);await ok(au,'anvil_setNextBlockBaseFeePerGas',[block.baseFeePerGas]);await ok(au,'evm_setNextBlockTimestamp',[Number(BigInt(block.timestamp))]);await ok(au,'evm_mine',[]);}
  const partition=pool=>Object.entries(pool).flatMap(([category,groups])=>Object.values(groups).flatMap(group=>Object.values(group).map(tx=>({category,hash:tx.hash})))).sort((a,b)=>a.hash.localeCompare(b.hash));
  async function snapshot(stage){const views=[];for(const url of [nu,au]){const receipts=[];for(const hash of hashes){const r=await ok(url,'eth_getTransactionReceipt',[hash]);receipts.push(r?{transactionHash:r.transactionHash,status:r.status,gasUsed:r.gasUsed,effectiveGasPrice:r.effectiveGasPrice}:null);}views.push({balance:await ok(url,'eth_getBalance',[sender,'latest']),received:await ok(url,'eth_getBalance',[to,'latest']),nonce:await ok(url,'eth_getTransactionCount',[sender,'latest']),pool:partition(await ok(url,'txpool_content',[])),receipts});}results.push({fork,type,stage,raws,hashes,nano:views[0],anvil:views[1],matches:JSON.stringify(views[0])===JSON.stringify(views[1])});}
  await mineBoth();await snapshot('after-first-block');
  const replacement=await Promise.all([rpc(nu,'eth_sendRawTransaction',[raws[3]]),rpc(au,'eth_sendRawTransaction',[raws[3]])]);transactions.push({fork,type,raw:raws[3],responses:replacement});
  if(replacement.every(r=>r.result===hashes[3])){
   if(rebroadcast){
    // Preserve the baseline pool divergence; equalize only execution inputs.
    const response=await rpc(au,'eth_sendRawTransaction',[raws[1]]);
    transactions.push({fork,type,stage:'explicit-anvil-suffix-rebroadcast',raw:raws[1],response});
    assert.equal(response.result,hashes[1]);
   }
   await mineBoth();await snapshot('after-recovery-block');
  }
 }finally{for(const child of children.reverse())await stop(child);fs.rmSync(temp,{recursive:true,force:true});}
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({rebroadcastSuffixToAnvil:rebroadcast,referenceVersion:version,scope:'Same signed overcommitted suffix and cheaper recovery on Nano/Anvil Paris/Shanghai types 0/1/2. Nano uses signed bootstrap blocks via loopback-only dev RPC. Anvil block base fee and timestamp aligned to Nano; compare balances, nonces, normalized receipts and pool hash partitions.',total:results.length,matched:results.filter(r=>r.matches).length,transactions,results},null,2)+'\n');}
assert.equal(results.length,12);
if(rebroadcast){
 for(const row of results){
  if(row.stage==='after-first-block'){
   const {pool:nanoPool,...nanoState}=row.nano,{pool:anvilPool,...anvilState}=row.anvil;
   assert.deepEqual(nanoState,anvilState);
   assert.deepEqual(nanoPool,[{category:'queued',hash:row.hashes[1]}]);
   assert.deepEqual(anvilPool,[]);
  }else assert.deepEqual(row.nano,row.anvil);
 }
 console.log('6/6 first-block execution matches with retained pool divergence; 6/6 recovery matches after explicit Anvil suffix rebroadcast');
}else{
 assert(results.every(r=>r.matches),'Pool reference differences; inspect results.json');
 console.log('12/12 post-block execution comparisons passed');
}
