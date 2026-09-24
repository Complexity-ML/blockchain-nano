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
const results=[],transactions=[];
try{for(const fork of ['Paris','Shanghai'])for(const type of [0,1,2]){
 const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-pool-reference-')),children=[];
 try{
  const wallet=new Wallet(key(1)),sender=wallet.address.toLowerCase(),tag=fork+'-'+type;
  const genesis={config:{chainId:1337,consensus:'pos',posSignatureVersion:1,bootstrapValidator:sender,evmFork:fork},alloc:{[sender]:{balance:q(10n**24n)}}};
  fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(out,tag+'-genesis.json'),JSON.stringify(genesis,null,2));
  const n=start(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0'],{},path.join(out,tag+'-nano.log'));children.push(n.child);const nu=await n.ready;
  const a=start(anvil,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--base-fee','1000000000','--no-mining'],{},path.join(out,tag+'-anvil.log'));children.push(a.child);const au=await a.ready;
  await ok(au,'anvil_setBalance',[sender,q(10n**24n)]);
  const accepted=new Map();
  const hashes=groups=>Object.values(groups).flatMap(group=>Object.values(group).map(tx=>tx.hash.toLowerCase())).sort();
  async function snapshot(name){
   const views=[];for(const url of [nu,au])views.push({status:await ok(url,'txpool_status',[]),nonce:await ok(url,'eth_getTransactionCount',[sender,'pending']),content:await ok(url,'txpool_content',[])});
   let nonce=0;while(accepted.has(nonce))++nonce;
   const pending=[...accepted].filter(([n])=>n<nonce).map(([,h])=>h).sort(),queued=[...accepted].filter(([n])=>n>=nonce).map(([,h])=>h).sort();
   const expected={status:{pending:q(pending.length),queued:q(queued.length)},nonce:q(nonce),pending,queued};
   const normalized=views.map(v=>({status:v.status,nonce:v.nonce,pending:hashes(v.content.pending),queued:hashes(v.content.queued)}));
   const equal=v=>JSON.stringify(v)===JSON.stringify(expected);
   results.push({fork,type,name,expected,nano:normalized[0],anvil:normalized[1],matches:equal(normalized[0])&&equal(normalized[1]),raw:views});
  }
  await snapshot('empty');
  for(const nonce of [10,2,0,1]){
   const tx={type,chainId:1337,nonce,to:sender,value:1n,gasLimit:21000n,...(type===2?{maxFeePerGas:2000000000n,maxPriorityFeePerGas:1000000000n}:{gasPrice:2000000000n}),...(type>0?{accessList:[]}:{})};
   const raw=await wallet.signTransaction(tx),hash=keccak256(raw);
   const responses=await Promise.all([ok(nu,'eth_sendRawTransaction',[raw]),ok(au,'eth_sendRawTransaction',[raw])]);
   transactions.push({fork,type,nonce,raw,hash,responses});assert(responses.every(h=>h===hash),'signed envelope hash mismatch');accepted.set(nonce,hash);await snapshot('after-'+nonce);
  }
  for(const url of [nu,au]){assert.equal(await ok(url,'eth_getTransactionCount',[sender,'latest']),'0x0');assert.equal(BigInt(await ok(url,'eth_getBalance',[sender,'latest'])),10n**24n);}
 }finally{for(const child of children.reverse())await stop(child);fs.rmSync(temp,{recursive:true,force:true});}
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion:version,scope:'Signed type 0/1/2 nonce-gap pool classification. No mining, Paris/Shanghai. Counter quantities, pending nonce and exact pending/queued hashes; no cumulative affordability claim.',total:results.length,matched:results.filter(r=>r.matches).length,transactions,results},null,2)+'\n');}
assert.equal(results.length,30);assert(results.every(r=>r.matches),'Pool reference differences; inspect results.json');console.log('30/30 signed nonce-gap pool comparisons passed');
