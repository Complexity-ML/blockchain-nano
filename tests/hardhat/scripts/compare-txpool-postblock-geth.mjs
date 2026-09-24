import {spawn,execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
import {createHmac} from 'node:crypto';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../..');
const {Wallet,keccak256}=createRequire(path.join(root,'tests/hardhat/package.json'))('ethers');
const binary=process.env.NANO_BINARY,geth=process.env.GETH_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;
assert(binary&&geth&&out,'NANO_BINARY, GETH_BINARY, NANO_DIAGNOSTIC_REPORT_DIR required');
fs.mkdirSync(out,{recursive:false});
const version=execFileSync(geth,['version'],{encoding:'utf8'}).trim();
const q=n=>'0x'+BigInt(n).toString(16),key=n=>'0x'+BigInt(n).toString(16).padStart(64,'0');
const auth=new Map();
async function rpc(url,method,params){const r=await fetch(url,{method:'POST',headers:{'content-type':'application/json',...(auth.has(url)?{authorization:'Bearer '+auth.get(url)()}: {})},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params}),signal:AbortSignal.timeout(10000)});assert(r.ok);return r.json();}
async function ok(url,method,params){const r=await rpc(url,method,params);assert(!r.error,JSON.stringify(r));return r.result;}
function start(exe,args,env,log){const child=spawn(exe,args,{env:{...process.env,...env},stdio:['ignore','pipe','pipe']});let text='';const stream=fs.createWriteStream(log);child.stdout.on('data',d=>{text+=d;stream.write(d);});child.stderr.on('data',d=>{text+=d;stream.write(d);child.stdout.emit('data',Buffer.alloc(0));});child.once('close',()=>stream.end());const ready=new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Startup timeout')),10000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',()=>{clearTimeout(timer);reject(Error('Exited before ready'));});child.stdout.on('data',()=>{const m=text.match(/(?:listening on |endpoint=)127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(timer);resolve('http://127.0.0.1:'+m[1]);}});});return {child,ready};}
async function stop(child){if(child.exitCode!==null||child.signalCode!==null)return;const done=new Promise(r=>child.once('exit',r));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await done;clearTimeout(timer);}
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
  const initial=await ok(nu,'eth_getBlockByNumber',['0x0',false]);
  const config={chainId:1337,homesteadBlock:0,eip150Block:0,eip155Block:0,eip158Block:0,byzantiumBlock:0,constantinopleBlock:0,petersburgBlock:0,istanbulBlock:0,berlinBlock:0,londonBlock:0,terminalTotalDifficulty:0,...(fork==='Shanghai'?{shanghaiTime:0}:{})};
  const ggenesis={config,alloc:genesis.alloc,nonce:'0x0',timestamp:'0x0',extraData:'0x',gasLimit:initial.gasLimit,difficulty:'0x0',baseFeePerGas:initial.baseFeePerGas};
  const gp=path.join(temp,'geth.json');fs.writeFileSync(gp,JSON.stringify(ggenesis));fs.writeFileSync(path.join(out,tag+'-geth-genesis.json'),JSON.stringify(ggenesis,null,2));
  const datadir=path.join(temp,'geth'),fd=fs.openSync(path.join(out,tag+'-init.log'),'w');try{execFileSync(geth,['--datadir',datadir,'init',gp],{stdio:['ignore',fd,fd]});}finally{fs.closeSync(fd);}
  const secret=Buffer.alloc(32,7),jwtfile=path.join(temp,'jwt.hex');fs.writeFileSync(jwtfile,secret.toString('hex'));
  const alog=path.join(out,tag+'-geth.log');
  const a=start(geth,['--datadir',datadir,'--networkid','1337','--maxpeers','0','--nodiscover','--port','0','--ipcdisable','--authrpc.addr','127.0.0.1','--authrpc.port','0','--authrpc.jwtsecret',jwtfile,'--http','--http.addr','127.0.0.1','--http.port','0','--http.api','eth,net,web3,txpool','--miner.gaslimit',BigInt(initial.gasLimit).toString()],{},alog);children.push(a.child);const au=await a.ready;
  let eu;for(let attempt=0;attempt<100&&!eu;attempt++){const text=fs.readFileSync(alog,'utf8');const match=text.match(/HTTP server started[^\n]*endpoint=127\.0\.0\.1:(\d+)[^\n]*auth=true/);if(match)eu='http://127.0.0.1:'+match[1];else await new Promise(r=>setTimeout(r,100));}assert(eu,'Engine endpoint found');
  auth.set(eu,()=>{const head=Buffer.from(JSON.stringify({alg:'HS256',typ:'JWT'})).toString('base64url'),body=Buffer.from(JSON.stringify({iat:Math.floor(Date.now()/1000)})).toString('base64url'),data=head+'.'+body;return data+'.'+createHmac('sha256',secret).update(data).digest('base64url');});
  const versionSuffix=fork==='Shanghai'?'V2':'V1',zero='0x'+'00'.repeat(32);
  const choice=hash=>({headBlockHash:hash,safeBlockHash:hash,finalizedBlockHash:hash});
  const ghead=await ok(au,'eth_getBlockByNumber',['latest',false]);assert.equal((await ok(eu,'engine_forkchoiceUpdated'+versionSuffix,[choice(ghead.hash),null])).payloadStatus.status,'VALID');
  const make=async(nonce,fraction)=>wallet.signTransaction({type,chainId:1337,nonce,to,value:10n**24n*BigInt(fraction)/100n,gasLimit:21000n,...(type===2?{maxFeePerGas:2000000000n,maxPriorityFeePerGas:1000000000n}:{gasPrice:2000000000n}),...(type>0?{accessList:[]}:{})});
  const raws=await Promise.all([make(1,60),make(2,5),make(0,60),make(1,5)]),hashes=raws.map(keccak256);
  for(const raw of raws.slice(0,3))for(const url of [nu,au])assert.equal(await ok(url,'eth_sendRawTransaction',[raw]),keccak256(raw));
  async function mineBoth(){
   await ok(nu,'evm_mine',[]);const block=await ok(nu,'eth_getBlockByNumber',['latest',false]);
   const parent=await ok(au,'eth_getBlockByNumber',['latest',false]);
   const built=await ok(eu,'engine_forkchoiceUpdated'+versionSuffix,[choice(parent.hash),{timestamp:block.timestamp,prevRandao:zero,suggestedFeeRecipient:producer.address,...(fork==='Shanghai'?{withdrawals:[]}:{})}]);
   assert.equal(built.payloadStatus.status,'VALID');assert(built.payloadId);
   await new Promise(r=>setTimeout(r,500));
   const response=await ok(eu,'engine_getPayload'+versionSuffix,[built.payloadId]),payload=fork==='Shanghai'?response.executionPayload:response;
   fs.writeFileSync(path.join(out,tag+'-payload-'+BigInt(payload.blockNumber)+'.json'),JSON.stringify(payload,null,2));
   assert.deepEqual(payload.transactions.map(keccak256),block.transactions,'same ordered execution inputs');
   assert.equal(payload.baseFeePerGas,block.baseFeePerGas,'same derived base fee');
   assert.equal((await ok(eu,'engine_newPayload'+versionSuffix,[payload])).status,'VALID');
   assert.equal((await ok(eu,'engine_forkchoiceUpdated'+versionSuffix,[choice(payload.blockHash),null])).payloadStatus.status,'VALID');
   // Geth pool reset follows the canonical-head event asynchronously.
   for(let attempt=0;attempt<100;attempt++){const pool=await ok(au,'txpool_content',[]);if(!JSON.stringify(pool).includes(hashes[2]))break;await new Promise(r=>setTimeout(r,20));}
  }
  const partition=pool=>Object.entries(pool).flatMap(([category,groups])=>Object.values(groups).flatMap(group=>Object.values(group).map(tx=>({category,hash:tx.hash})))).sort((a,b)=>a.hash.localeCompare(b.hash));
  async function snapshot(stage){const views=[];for(const url of [nu,au]){const receipts=[];for(const hash of hashes){const r=await ok(url,'eth_getTransactionReceipt',[hash]);receipts.push(r?{transactionHash:r.transactionHash,status:r.status,gasUsed:r.gasUsed,effectiveGasPrice:r.effectiveGasPrice}:null);}views.push({balance:await ok(url,'eth_getBalance',[sender,'latest']),received:await ok(url,'eth_getBalance',[to,'latest']),nonce:await ok(url,'eth_getTransactionCount',[sender,'latest']),pool:partition(await ok(url,'txpool_content',[])),receipts});}results.push({fork,type,stage,raws,hashes,nano:views[0],geth:views[1],matches:JSON.stringify(views[0])===JSON.stringify(views[1])});}
  await mineBoth();await snapshot('after-first-block');
  const replacement=await Promise.all([rpc(nu,'eth_sendRawTransaction',[raws[3]]),rpc(au,'eth_sendRawTransaction',[raws[3]])]);transactions.push({fork,type,raw:raws[3],responses:replacement});
  if(replacement.every(r=>r.result===hashes[3])){
   await mineBoth();await snapshot('after-recovery-block');
  }
 }finally{for(const child of children.reverse())await stop(child);fs.rmSync(temp,{recursive:true,force:true});}
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion:version,scope:'Same signed overcommitted suffix and cheaper recovery on Nano/Geth Paris/Shanghai types 0/1/2. Nano uses signed bootstrap blocks via loopback-only dev RPC. Geth Engine API V1/V2 builds blocks; derived base fee and ordered signed input hashes asserted equal to Nano; compare balances, nonces, normalized receipts and pool hash partitions.',total:results.length,matched:results.filter(r=>r.matches).length,transactions,results},null,2)+'\n');}
assert.equal(results.length,12);
assert(results.every(r=>r.matches),'Pool reference differences; inspect results.json');
console.log('12/12 Geth post-block execution and pool comparisons passed');
