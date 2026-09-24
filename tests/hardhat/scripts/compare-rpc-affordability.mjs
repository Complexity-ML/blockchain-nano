import {spawn,execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../..');
const {Wallet}=createRequire(path.join(root,'tests/hardhat/package.json'))('ethers');
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
 const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-rpc-budget-'));const children=[];
 try{
  const validator=new Wallet(key(1)).address.toLowerCase(),to=new Wallet(key(99)).address;
  const cases=[];let index=10;
  for(const type of [0,1,2])for(const delta of [-1,0]){const cap=type===2?3000000000n:2000000000n;cases.push({type,delta,cap,balance:21000n*cap+5n+BigInt(delta),from:new Wallet(key(index++)).address});}
  const alloc={[validator]:{balance:q(10n**24n)}};for(const c of cases)alloc[c.from.toLowerCase()]={balance:q(c.balance)};
  const genesis={config:{chainId:1337,consensus:'pos',posSignatureVersion:1,bftVersion:1,bootstrapValidator:validator,evmFork:fork},alloc};
  fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(out,fork+'-genesis.json'),JSON.stringify(genesis,null,2));
  fs.writeFileSync(path.join(temp,'node.json'),JSON.stringify({bft:{key_env:'TEST_BFT_KEY',journal:path.join(temp,'votes')}}));
  const n=start(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0','127.0.0.1',path.join(temp,'node.json')],{TEST_BFT_KEY:key(1)},path.join(out,fork+'-nano.log'));children.push(n.child);const nu=await n.ready;
  const a=start(anvil,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--base-fee','1000000000'],{},path.join(out,fork+'-anvil.log'));children.push(a.child);const au=await a.ready;
  for(const c of cases){await ok(au,'anvil_setBalance',[c.from,q(c.balance)]);
   const base={from:c.from,to,type:q(c.type),value:'0x5',gas:'0x186a0',...(c.type===2?{maxFeePerGas:q(c.cap),maxPriorityFeePerGas:'0x3b9aca00'}:{gasPrice:q(c.cap)})};
   const variants=[['boundary',base,c.delta===0],['entire-balance',{...base,value:q(c.balance)},false],['caller-cap',{...base,gas:'0x5207'},c.delta===0],['omitted-fees',{from:c.from,to,value:'0x5'},true],['zero-fees',{from:c.from,to,value:'0x5',gasPrice:'0x0'},true]];
   for(const [name,request,expectedSuccess] of variants){const [nano,reference]=await Promise.all([rpc(nu,'eth_estimateGas',[request]),rpc(au,'eth_estimateGas',[request])]);const matches=Boolean(nano.error)===Boolean(reference.error)&&Boolean(nano.error)===!expectedSuccess&&(!expectedSuccess||BigInt(nano.result)===21000n&&BigInt(reference.result)===21000n);results.push({fork,type:c.type,delta:c.delta,name,request,expectedSuccess,nano,anvil:reference,matches});}
   for(const url of [nu,au]){assert.equal(BigInt(await ok(url,'eth_getBalance',[c.from,'latest'])),c.balance);assert.equal(BigInt(await ok(url,'eth_getTransactionCount',[c.from,'latest'])),0n);}
  }
 }finally{for(const child of children.reverse())await stop(child);fs.rmSync(temp,{recursive:true,force:true});}
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion:version,scope:'Local RPC fee affordability, Paris/Shanghai types 0/1/2. No signed transaction broadcast or public network. Error wording/codes not required equal.',total:results.length,matched:results.filter(r=>r.matches).length,results},null,2)+'\n');}
assert.equal(results.length,60);assert(results.every(r=>r.matches),'RPC affordability divergence; inspect results.json');
console.log('60/60 RPC affordability comparisons passed');
