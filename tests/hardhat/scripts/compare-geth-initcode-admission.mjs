import {createRequire} from 'node:module';
const {Wallet,keccak256}=createRequire(import.meta.url)('ethers');
import {spawn,execFileSync} from 'node:child_process';
import fs from 'node:fs';import os from 'node:os';import path from 'node:path';import assert from 'node:assert/strict';
const binary=process.env.NANO_BINARY,geth=process.env.GETH_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;
assert(binary&&geth&&out&&process.argv.length>2,'binaries, output and prior campaign directories required');
fs.mkdirSync(out,{recursive:false});const referenceVersion=execFileSync(geth,['version'],{encoding:'utf8'}).trim();
async function rpc(url,method,params){const r=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params}),signal:AbortSignal.timeout(10000)});assert(r.ok);return r.json();}
const indexingWaits=[];
async function ok(url,method,params){for(let attempt=0;attempt<50;attempt++){const r=await rpc(url,method,params);if(r.error?.message==='transaction indexing is in progress'&&['eth_getTransactionByHash','eth_getTransactionReceipt'].includes(method)){indexingWaits.push({method,attempt,error:r.error});await new Promise(resolve=>setTimeout(resolve,100));continue;}assert(!r.error,JSON.stringify(r));return r.result;}throw Error('Geth indexing remained unavailable for five seconds');}
function start(exe,args,env,log){const child=spawn(exe,args,{env:{...process.env,...env},stdio:['ignore','pipe','pipe']});let text='';const stream=fs.createWriteStream(log);child.stdout.on('data',d=>{text+=d;stream.write(d);});child.stderr.on('data',d=>{text+=d;stream.write(d);child.stdout.emit('data',Buffer.alloc(0));});child.once('close',()=>stream.end());const ready=new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Startup timeout')),10000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',()=>{clearTimeout(timer);reject(Error('Exited before ready'));});child.stdout.on('data',()=>{const m=text.match(/(?:listening on |endpoint=)127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(timer);resolve('http://127.0.0.1:'+m[1]);}});});return {child,ready};}
async function stop(child){if(child.exitCode!==null||child.signalCode!==null)return;const done=new Promise(r=>child.once('exit',r));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await done;clearTimeout(timer);}
const results=[];
try{for(const campaign of process.argv.slice(2)){

 for(const fork of ['Shanghai']){
  const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-geth-rpc-')),children=[];
  try{
   const genesis=JSON.parse(fs.readFileSync(path.join(campaign,fork+'-genesis.json'))),label=path.basename(campaign)+'-'+fork;
   for(const i of [1,2,3])genesis.alloc[new Wallet('0x'+i.toString(16).padStart(64,'0')).address.toLowerCase()]={balance:'0xd3c21bcecceda1000000'};
   fs.writeFileSync(path.join(temp,'nano.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(temp,'node.json'),JSON.stringify({bft:{key_env:'TEST_BFT_KEY',journal:path.join(temp,'votes')}}));
   const config={chainId:1337,homesteadBlock:0,eip150Block:0,eip155Block:0,eip158Block:0,byzantiumBlock:0,constantinopleBlock:0,petersburgBlock:0,istanbulBlock:0,berlinBlock:0,londonBlock:0,terminalTotalDifficulty:0,...(fork==='Shanghai'?{shanghaiTime:0}:{})};
   const ggenesis={config,alloc:Object.fromEntries(Object.entries(genesis.alloc).map(([address,account])=>[address,{balance:"0x0",...account}])),nonce:'0x0',timestamp:'0x0',extraData:'0x',gasLimit:'0x1c9c380',difficulty:'0x0',baseFeePerGas:'0x3b9aca00'};
   const gp=path.join(temp,'geth.json');fs.writeFileSync(gp,JSON.stringify(ggenesis));fs.writeFileSync(path.join(out,label+'-genesis.json'),JSON.stringify(ggenesis,null,2));
   const datadir=path.join(temp,'geth');const fd=fs.openSync(path.join(out,label+'-init.log'),'w');try{execFileSync(geth,['--datadir',datadir,'init',gp],{stdio:['ignore',fd,fd]});}finally{fs.closeSync(fd);}
   const n=start(binary,['serve',path.join(temp,'nano.json'),path.join(temp,'nano-db'),'0','127.0.0.1'],{},path.join(out,label+'-nano.log'));children.push(n.child);const nu=await n.ready;
   const g=start(geth,['--datadir',datadir,'--networkid','1337','--maxpeers','0','--nodiscover','--port','0','--ipcdisable','--authrpc.port','0','--http','--http.addr','127.0.0.1','--http.port','0','--http.api','eth,net,web3,txpool'],{},path.join(out,label+'-geth.log'));children.push(g.child);const gu=await g.ready;
   for(const url of [nu,gu])assert.equal(await ok(url,'eth_blockNumber',[]),'0x0');
   for(const type of [0,1,2]){
    const wallet=new Wallet('0x'+(type+1).toString(16).padStart(64,'0'));
    const fees=type===2?{maxFeePerGas:4000000000n,maxPriorityFeePerGas:1000000000n}:{gasPrice:4000000000n};
    for(const size of [49153,49152]){
     const raw=await wallet.signTransaction({chainId:1337,type,nonce:0,gasLimit:3000000,...fees,data:'0x5f5ff3'+'00'.repeat(size-3)}),hash=keccak256(raw),expectedAccepted=size===49152;
     fs.writeFileSync(path.join(out,'type-'+type+'-size-'+size+'-raw.json'),JSON.stringify({raw,hash,type,size,expectedAccepted},null,2));
     const [nano,reference]=await Promise.all([rpc(nu,'eth_sendRawTransaction',[raw]),rpc(gu,'eth_sendRawTransaction',[raw])]);
     const snapshots=[];
     for(const url of [nu,gu])snapshots.push({balance:await ok(url,'eth_getBalance',[wallet.address,'latest']),nonce:await ok(url,'eth_getTransactionCount',[wallet.address,'latest']),pendingNonce:await ok(url,'eth_getTransactionCount',[wallet.address,'pending']),transaction:await rpc(url,'eth_getTransactionByHash',[hash]),receipt:await rpc(url,'eth_getTransactionReceipt',[hash]),pool:await ok(url,'txpool_content',[])});
     const responseMatches=expectedAccepted?nano.result===hash&&reference.result===hash:Boolean(nano.error&&reference.error)&&/initcode|init code/i.test(nano.error.message)&&/initcode|init code/i.test(reference.error.message);
     const containsHash=v=>Boolean(v&&typeof v==='object'&&(v.hash===hash||Object.values(v).some(containsHash)));
     const unavailable=r=>r.error?.message==='transaction indexing is in progress';
     const stateMatches=snapshots.every(s=>BigInt(s.balance)===10n**24n&&s.nonce==='0x0'&&BigInt(s.pendingNonce)===(expectedAccepted?1n:0n)&&(s.receipt.result===null||unavailable(s.receipt))&&containsHash(s.pool)===expectedAccepted&&(expectedAccepted?s.transaction.result?.hash===hash:s.transaction.result===null||unavailable(s.transaction)));
     results.push({fork,type,initcodeBytes:size,expectedAccepted,hash,nano,geth:reference,snapshots,matches:responseMatches&&stateMatches});
    }
   }
   for(const url of [nu,gu])assert.equal(await ok(url,'eth_blockNumber',[]),'0x0');
   for(const [address,account] of Object.entries(genesis.alloc))for(const url of [nu,gu]){assert.equal(BigInt(await ok(url,'eth_getBalance',[address,'latest'])),BigInt(account.balance??0));assert.equal(BigInt(await ok(url,'eth_getTransactionCount',[address,'latest'])),0n);if(account.code)assert.equal(await ok(url,'eth_getCode',[address,'latest']),account.code);}
  }finally{for(const c of children.reverse())await stop(c);fs.rmSync(temp,{recursive:true,force:true});}
 }
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion,indexingWaits,scope:'Fresh local Nano and Geth HTTP RPC from corresponding genesis allocations. Checks hash, rejection reason, canonical balances/nonces, pending lookup/nonce, and same-nonce valid recovery. Same signed oversized and maximum-size Shanghai creations sent to both local pools; inclusion not tested. Missing-hash/receipt lookups may report Geth indexing unavailable at height zero; exact responses preserved, pool membership verified via txpool_content. No public network.',total:results.length,matched:results.filter(r=>r.matches).length,results},null,2)+'\n');}
assert.equal(results.length,6);assert(results.every(r=>r.matches),'Geth HTTP RPC divergences retained');console.log(results.length+' HTTP RPC comparisons passed');
