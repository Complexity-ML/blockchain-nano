import {spawn,execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../..');
const {Wallet,keccak256}=createRequire(path.join(root,'tests/hardhat/package.json'))('ethers');
const binary=process.env.NANO_BINARY,geth=process.env.GETH_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;
assert(binary&&geth&&out,'NANO_BINARY, GETH_BINARY, NANO_DIAGNOSTIC_REPORT_DIR required');
fs.mkdirSync(out,{recursive:false});
const version=execFileSync(geth,['version'],{encoding:'utf8'}).trim();
const q=n=>'0x'+BigInt(n).toString(16),key=n=>'0x'+BigInt(n).toString(16).padStart(64,'0');
async function rpc(url,method,params){const r=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params}),signal:AbortSignal.timeout(10000)});assert(r.ok);return r.json();}
async function ok(url,method,params){const r=await rpc(url,method,params);assert(!r.error,JSON.stringify(r));return r.result;}
function start(exe,args,env,log){const child=spawn(exe,args,{env:{...process.env,...env},stdio:['ignore','pipe','pipe']});let text='';const stream=fs.createWriteStream(log);child.stdout.on('data',d=>{text+=d;stream.write(d);});child.stderr.on('data',d=>{text+=d;stream.write(d);child.stdout.emit('data',Buffer.alloc(0));});child.once('close',()=>stream.end());const ready=new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Startup timeout')),10000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',()=>{clearTimeout(timer);reject(Error('Exited before ready'));});child.stdout.on('data',()=>{const m=text.match(/(?:listening on |endpoint=)127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(timer);resolve('http://127.0.0.1:'+m[1]);}});});return {child,ready};}
async function stop(child){if(child.exitCode!==null||child.signalCode!==null)return;const done=new Promise(r=>child.once('exit',r));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await done;clearTimeout(timer);}
const results=[],transactions=[];
try{for(const fork of ['Paris','Shanghai'])for(const type of [0,1,2]){
 const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-pool-reference-')),children=[];
 try{
  const wallet=new Wallet(key(1)),sender=wallet.address.toLowerCase(),tag=fork+'-'+type;
  const genesis={config:{chainId:1337,consensus:'pos',posSignatureVersion:1,bootstrapValidator:sender,evmFork:fork},alloc:{[sender]:{balance:q(10n**24n)}}};
  fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(out,tag+'-genesis.json'),JSON.stringify(genesis,null,2));
  const n=start(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0'],{},path.join(out,tag+'-nano.log'));children.push(n.child);const nu=await n.ready;
  const config={chainId:1337,homesteadBlock:0,eip150Block:0,eip155Block:0,eip158Block:0,byzantiumBlock:0,constantinopleBlock:0,petersburgBlock:0,istanbulBlock:0,berlinBlock:0,londonBlock:0,terminalTotalDifficulty:0,...(fork==='Shanghai'?{shanghaiTime:0}:{})};
  const ggenesis={config,alloc:genesis.alloc,nonce:'0x0',timestamp:'0x0',extraData:'0x',gasLimit:'0x1c9c380',difficulty:'0x0',baseFeePerGas:'0x3b9aca00'};
  const gp=path.join(temp,'geth.json');fs.writeFileSync(gp,JSON.stringify(ggenesis));fs.writeFileSync(path.join(out,tag+'-geth-genesis.json'),JSON.stringify(ggenesis,null,2));
  const datadir=path.join(temp,'geth'),fd=fs.openSync(path.join(out,tag+'-init.log'),'w');try{execFileSync(geth,['--datadir',datadir,'init',gp],{stdio:['ignore',fd,fd]});}finally{fs.closeSync(fd);}
  const a=start(geth,['--datadir',datadir,'--networkid','1337','--maxpeers','0','--nodiscover','--port','0','--ipcdisable','--authrpc.port','0','--http','--http.addr','127.0.0.1','--http.port','0','--http.api','eth,net,web3,txpool'],{},path.join(out,tag+'-geth.log'));children.push(a.child);const au=await a.ready;
  const balance=10n**24n,to=new Wallet(key(99)).address;
  const cases=[['future-sixty',10,balance*60n/100n],['future-excess-sixty',20,balance*60n/100n],['future-thirty',20,balance*30n/100n],['fill-excess-twenty',0,balance*20n/100n],['fill-five',0,balance*5n/100n]];
  const members=pool=>Object.entries(pool).flatMap(([category,groups])=>Object.values(groups).flatMap(group=>Object.values(group).map(tx=>({category,hash:tx.hash})))).sort((a,b)=>a.hash.localeCompare(b.hash));
  for(const [name,nonce,value] of cases){
   const raw=await wallet.signTransaction({type,chainId:1337,nonce,to,value,gasLimit:21000n,...(type===2?{maxFeePerGas:2000000000n,maxPriorityFeePerGas:1000000000n}:{gasPrice:2000000000n}),...(type>0?{accessList:[]}:{})}),hash=keccak256(raw);
   const [nano,reference]=await Promise.all([rpc(nu,'eth_sendRawTransaction',[raw]),rpc(au,'eth_sendRawTransaction',[raw])]);
   for(const r of [nano,reference])if(!r.error)assert.equal(r.result,hash,'signed envelope hash mismatch');
   const np=members(await ok(nu,'txpool_content',[])),gp=members(await ok(au,'txpool_content',[]));
   results.push({fork,type,name,nonce,value:q(value),raw,hash,nano,geth:reference,nanoPool:np,gethPool:gp,matches:Boolean(nano.error)===Boolean(reference.error)&&JSON.stringify(np)===JSON.stringify(gp)});
  }
  for(const url of [nu,au]){assert.equal(await ok(url,'eth_getTransactionCount',[sender,'latest']),'0x0');assert.equal(BigInt(await ok(url,'eth_getBalance',[sender,'latest'])),10n**24n);}
 }finally{for(const child of children.reverse())await stop(child);fs.rmSync(temp,{recursive:true,force:true});}
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion:version,scope:'Cumulative affordability, same signed types 0/1/2 on Paris/Shanghai. Transfers consume 60%, 60%, 30%, 20%, 5% of balance at nonces 10,20,20,0,0. No mining; admission and surviving hash partitions; error wording not required equal.',total:results.length,matched:results.filter(r=>r.matches).length,transactions,results},null,2)+'\n');}
assert.equal(results.length,30);assert(results.every(r=>r.matches),'Pool reference differences; inspect results.json');console.log('30/30 future-nonce affordability comparisons passed');
