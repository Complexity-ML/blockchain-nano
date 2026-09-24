import {spawn,execFileSync} from 'node:child_process';
import fs from 'node:fs';import os from 'node:os';import path from 'node:path';import assert from 'node:assert/strict';
const binary=process.env.NANO_BINARY,geth=process.env.GETH_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;
assert(binary&&geth&&out&&process.argv.length>2,'binaries, output and prior campaign directories required');
fs.mkdirSync(out,{recursive:false});const referenceVersion=execFileSync(geth,['version'],{encoding:'utf8'}).trim();
async function rpc(url,method,params){const r=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params}),signal:AbortSignal.timeout(10000)});assert(r.ok);return r.json();}
async function ok(url,method,params){const r=await rpc(url,method,params);assert(!r.error,JSON.stringify(r));return r.result;}
function start(exe,args,env,log){const child=spawn(exe,args,{env:{...process.env,...env},stdio:['ignore','pipe','pipe']});let text='';const stream=fs.createWriteStream(log);child.stdout.on('data',d=>{text+=d;stream.write(d);});child.stderr.on('data',d=>{text+=d;stream.write(d);child.stdout.emit('data',Buffer.alloc(0));});child.once('close',()=>stream.end());const ready=new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Startup timeout')),10000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',()=>{clearTimeout(timer);reject(Error('Exited before ready'));});child.stdout.on('data',()=>{const m=text.match(/(?:listening on |endpoint=)127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(timer);resolve('http://127.0.0.1:'+m[1]);}});});return {child,ready};}
async function stop(child){if(child.exitCode!==null||child.signalCode!==null)return;const done=new Promise(r=>child.once('exit',r));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await done;clearTimeout(timer);}
const results=[];
try{for(const campaign of process.argv.slice(2)){
 const saved=JSON.parse(fs.readFileSync(path.join(campaign,'results.json')));
 for(const fork of ['Paris','Shanghai']){
  const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-geth-rpc-')),children=[];
  try{
   const genesis=JSON.parse(fs.readFileSync(path.join(campaign,fork+'-genesis.json'))),label=path.basename(campaign)+'-'+fork;
   fs.writeFileSync(path.join(temp,'nano.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(temp,'node.json'),JSON.stringify({bft:{key_env:'TEST_BFT_KEY',journal:path.join(temp,'votes')}}));
   const config={chainId:1337,homesteadBlock:0,eip150Block:0,eip155Block:0,eip158Block:0,byzantiumBlock:0,constantinopleBlock:0,petersburgBlock:0,istanbulBlock:0,berlinBlock:0,londonBlock:0,terminalTotalDifficulty:0,...(fork==='Shanghai'?{shanghaiTime:0}:{})};
   const ggenesis={config,alloc:Object.fromEntries(Object.entries(genesis.alloc).map(([address,account])=>[address,{balance:"0x0",...account}])),nonce:'0x0',timestamp:'0x0',extraData:'0x',gasLimit:'0x1c9c380',difficulty:'0x0',baseFeePerGas:'0x3b9aca00'};
   const gp=path.join(temp,'geth.json');fs.writeFileSync(gp,JSON.stringify(ggenesis));fs.writeFileSync(path.join(out,label+'-genesis.json'),JSON.stringify(ggenesis,null,2));
   const datadir=path.join(temp,'geth');const fd=fs.openSync(path.join(out,label+'-init.log'),'w');try{execFileSync(geth,['--datadir',datadir,'init',gp],{stdio:['ignore',fd,fd]});}finally{fs.closeSync(fd);}
   const n=start(binary,['serve',path.join(temp,'nano.json'),path.join(temp,'nano-db'),'0','127.0.0.1'],{},path.join(out,label+'-nano.log'));children.push(n.child);const nu=await n.ready;
   const g=start(geth,['--datadir',datadir,'--networkid','1337','--maxpeers','0','--nodiscover','--port','0','--ipcdisable','--authrpc.port','0','--http','--http.addr','127.0.0.1','--http.port','0','--http.api','eth,net,web3'],{},path.join(out,label+'-geth.log'));children.push(g.child);const gu=await g.ready;
   for(const url of [nu,gu])assert.equal(await ok(url,'eth_blockNumber',[]),'0x0');
   for(const row of saved.results.filter(r=>r.fork===fork)){
    const method=row.method??'eth_estimateGas';const [nano,reference]=await Promise.all([rpc(nu,method,[row.request,'latest']),rpc(gu,method,[row.request,'latest'])]);
    const matches=Boolean(nano.error)===Boolean(reference.error)&&(nano.error||method==='eth_estimateGas'||nano.result===reference.result);
    results.push({campaign:path.basename(campaign),fork,name:row.name,type:row.type,delta:row.delta,method,request:row.request,nano,geth:reference,matches:Boolean(matches)});
   }
   for(const url of [nu,gu])assert.equal(await ok(url,'eth_blockNumber',[]),'0x0');
   for(const [address,account] of Object.entries(genesis.alloc))for(const url of [nu,gu]){assert.equal(BigInt(await ok(url,'eth_getBalance',[address,'latest'])),BigInt(account.balance??0));assert.equal(BigInt(await ok(url,'eth_getTransactionCount',[address,'latest'])),0n);if(account.code)assert.equal(await ok(url,'eth_getCode',[address,'latest']),account.code);}
  }finally{for(const c of children.reverse())await stop(c);fs.rmSync(temp,{recursive:true,force:true});}
 }
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion,scope:'Fresh local Nano and Geth HTTP RPC from corresponding genesis allocations. Compares call output and estimate acceptance, not exact estimate gas. No signed broadcasts or public network.',total:results.length,matched:results.filter(r=>r.matches).length,results},null,2)+'\n');}
assert(results.length>0);assert(results.every(r=>r.matches),'Geth HTTP RPC divergences retained');console.log(results.length+' HTTP RPC comparisons passed');
