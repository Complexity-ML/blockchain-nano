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

const results=[];
try{for(const fork of ['Paris','Shanghai']){
 const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-estimate-time-')),children=[];
 try{
  const wallet=new Wallet(key(10)),validator=new Wallet(key(1)).address.toLowerCase(),probe='0x00000000000000000000000000000000000040ad';
  // Skip writes in the last recorded second; otherwise update timestamp and counter.
  const code='0x6000544214601557426000556001546001016001555b00';
  const timestamp=BigInt(Math.floor(Date.now()/1000)-10),balance=10n**24n;
  const alloc={[validator]:{balance:q(balance)},[wallet.address.toLowerCase()]:{balance:q(balance)},[probe]:{code,storage:{[q(0)]:q(timestamp),[q(1)]:q(1)}}};
  const genesis={timestamp:q(timestamp),config:{chainId:1337,consensus:'pos',posSignatureVersion:1,bftVersion:1,bootstrapValidator:validator,evmFork:fork},alloc};
  fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(out,fork+'-genesis.json'),JSON.stringify(genesis,null,2));
  fs.writeFileSync(path.join(temp,'node.json'),JSON.stringify({bft:{key_env:'TEST_BFT_KEY',journal:path.join(temp,'votes')}}));
  const n=start(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0','127.0.0.1',path.join(temp,'node.json')],{TEST_BFT_KEY:key(1)},path.join(out,fork+'-nano.log'));children.push(n.child);const nu=await n.ready;
  const a=start(anvil,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--base-fee','1000000000','--timestamp',timestamp.toString(),'--no-mining'],{},path.join(out,fork+'-anvil.log'));children.push(a.child);const au=await a.ready;
  await ok(au,'anvil_setBalance',[wallet.address,q(balance)]);await ok(au,'anvil_setCode',[probe,code]);
  await ok(au,'anvil_setStorageAt',[probe,q(0),q(timestamp).slice(2).padStart(64,'0').replace(/^/,'0x')]);await ok(au,'anvil_setStorageAt',[probe,q(1),'0x'+'1'.padStart(64,'0')]);
  const request={from:wallet.address,to:probe,gasPrice:q(2000000000n)},estimates={};
  for(const [name,url] of [['nano',nu],['anvil',au]]){
   assert.equal(BigInt((await ok(url,'eth_getBlockByNumber',['latest',false])).timestamp),timestamp);
   estimates[name]={default:await ok(url,'eth_estimateGas',[request]),latest:await ok(url,'eth_estimateGas',[request,'latest']),pending:await rpc(url,'eth_estimateGas',[request,'pending'])};
   assert.equal(await ok(url,'eth_call',[{...request,gas:estimates[name].latest},'latest']),'0x');
  }
  async function waitReceipt(url,hash){for(let i=0;i<200;i++){const r=await ok(url,'eth_getTransactionReceipt',[hash]);if(r)return r;await new Promise(r=>setTimeout(r,100));}throw Error('receipt timeout');}
  assert.equal(BigInt(estimates.nano.pending.result),31039n);
  const transactions=[];
  for(const [nonce,gas] of [[0,BigInt(estimates.nano.default)],[1,BigInt(estimates.nano.pending.result)]]){
   const raw=await wallet.signTransaction({type:0,chainId:1337,nonce,to:probe,gasLimit:gas,gasPrice:2000000000n}),hash=keccak256(raw);
   assert.equal(await ok(nu,'eth_sendRawTransaction',[raw]),hash);const nr=await waitReceipt(nu,hash),nb=await ok(nu,'eth_getBlockByHash',[nr.blockHash,false]);
   await ok(au,'evm_setNextBlockTimestamp',[Number(BigInt(nb.timestamp))]);assert.equal(await ok(au,'eth_sendRawTransaction',[raw]),hash);await ok(au,'evm_mine',[]);const ar=await waitReceipt(au,hash);
   const states=[];for(const url of [nu,au])states.push({last:await ok(url,'eth_getStorageAt',[probe,q(0),'latest']),counter:await ok(url,'eth_getStorageAt',[probe,q(1),'latest'])});
   assert.equal(nr.status,ar.status);assert.equal(nr.gasUsed,ar.gasUsed);assert.deepEqual(states[0],states[1]);
   assert.equal(nr.status,nonce===0?'0x0':'0x1');assert.equal(BigInt(states[0].counter),nonce===0?1n:2n);
   transactions.push({raw,hash,gas:q(gas),timestamp:nb.timestamp,nano:nr,anvil:ar,state:states[0],matches:true});
  }
  results.push({fork,estimates,transactions});
 }finally{for(const child of children.reverse())await stop(child);fs.rmSync(temp,{recursive:true,force:true});}
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion:version,scope:'Same signed timestamp-dependent transactions and inclusion timestamps on Nano and Anvil; no assumption that latest estimates predict future block execution.',results},null,2)+'\n');}
assert.equal(results.length,2);console.log('Paris/Shanghai timestamp-dependent estimates and four same-raw transactions compared');
