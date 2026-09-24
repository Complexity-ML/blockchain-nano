import {spawn,execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../..');
const {Wallet,keccak256,Interface,getCreateAddress}=createRequire(path.join(root,'tests/hardhat/package.json'))('ethers');
const binary=process.env.NANO_BINARY,anvil=process.env.ANVIL_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;
assert(binary&&anvil&&out,'NANO_BINARY, ANVIL_BINARY, NANO_DIAGNOSTIC_REPORT_DIR required');
fs.mkdirSync(out,{recursive:false});
const version=execFileSync(anvil,['--version'],{encoding:'utf8'}).trim();
const q=n=>'0x'+BigInt(n).toString(16),key=n=>'0x'+BigInt(n).toString(16).padStart(64,'0');
async function rpc(url,method,params){const r=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params}),signal:AbortSignal.timeout(10000)});assert(r.ok);return r.json();}
async function ok(url,method,params){const r=await rpc(url,method,params);assert(!r.error,JSON.stringify(r));return r.result;}
function start(exe,args,env,log){const child=spawn(exe,args,{env:{...process.env,...env},stdio:['ignore','pipe','pipe']});let text='';const stream=fs.createWriteStream(log);child.stdout.on('data',d=>{text+=d;stream.write(d);});child.stderr.on('data',d=>stream.write(d));child.once('close',()=>stream.end());const ready=new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Startup timeout')),10000);child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',()=>{clearTimeout(timer);reject(Error('Exited before ready'));});child.stdout.on('data',()=>{const m=text.match(/listening on 127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(timer);resolve('http://127.0.0.1:'+m[1]);}});});return {child,ready};}
async function stop(child){if(child.exitCode!==null||child.signalCode!==null)return;const done=new Promise(r=>child.once('exit',r));child.kill('SIGTERM');const timer=setTimeout(()=>child.kill('SIGKILL'),5000);await done;clearTimeout(timer);}
const artifact=JSON.parse(fs.readFileSync(path.join(root,'tests/hardhat/artifacts/contracts/DeveloperGovernance.sol/DeveloperVotes.json'))),abi=new Interface(artifact.abi),results=[];
const forks=(process.env.NANO_DIAGNOSTIC_FORKS??'Paris,Shanghai').split(',');assert(forks.length&&forks.every(f=>['Paris','Shanghai'].includes(f))&&new Set(forks).size===forks.length);
try {for(const fork of forks)for(const type of [0,1,2]){
 const temp=fs.mkdtempSync(path.join(os.tmpdir(),'nano-votes-compare-')),children=[],label=fork+'-'+type;
 try {
  const wallet=new Wallet(key(10)),recipient=new Wallet(key(11)).address,address=getCreateAddress({from:wallet.address,nonce:0}),balance=10n**24n;
  const genesis={timestamp:q(1000),gasLimit:q(30000000),config:{chainId:1337,consensus:'pow',evmFork:fork},alloc:{[wallet.address]:{balance:q(balance)}}};
  fs.writeFileSync(path.join(temp,'genesis.json'),JSON.stringify(genesis));fs.writeFileSync(path.join(temp,'node.json'),JSON.stringify({dev_mode:true}));fs.writeFileSync(path.join(out,label+'-genesis.json'),JSON.stringify(genesis,null,2));
  const n=start(binary,['serve',path.join(temp,'genesis.json'),path.join(temp,'db'),'0','127.0.0.1',path.join(temp,'node.json')],{},path.join(out,label+'-nano.log'));children.push(n.child);const nu=await n.ready;
  const a=start(anvil,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--base-fee','1000000000','--gas-limit','30000000','--timestamp','1000','--no-mining'],{},path.join(out,label+'-anvil.log'));children.push(a.child);const au=await a.ready;
  const urls=[nu,au];await ok(au,'anvil_setBalance',[wallet.address,q(balance)]);
  const fees=type===2?{maxFeePerGas:4000000000n,maxPriorityFeePerGas:1000000000n}:{gasPrice:2000000000n};
  const logs=r=>r.logs.map(l=>({address:l.address.toLowerCase(),topics:l.topics,data:l.data}));
  async function state(url){const call=async(name,args)=>ok(url,'eth_call',[{to:address,data:abi.encodeFunctionData(name,args)},'latest']);return {senderBalance:await ok(url,'eth_getBalance',[wallet.address,'latest']),senderNonce:await ok(url,'eth_getTransactionCount',[wallet.address,'latest']),ownerTokens:await call('balanceOf',[wallet.address]),recipientTokens:await call('balanceOf',[recipient]),ownerVotes:await call('getVotes',[wallet.address]),delegation:await call('delegates',[wallet.address]),supply:await call('totalSupply',[])};}
  async function send(nonce,data,gasLimit,to){
   const raw=await wallet.signTransaction({type,chainId:1337,nonce,gasLimit,...fees,data,to}),hash=keccak256(raw),receipts=[],headers=[];
   for(const url of urls){assert.equal(await ok(url,'eth_sendRawTransaction',[raw]),hash);const parent=await ok(url,'eth_getBlockByNumber',['latest',false]);await ok(url,'evm_setNextBlockTimestamp',[Number(BigInt(parent.timestamp)+12n)]);if(url===au)await ok(url,'anvil_setNextBlockBaseFeePerGas',[headers[0].baseFeePerGas]);await ok(url,'evm_mine',[]);const receipt=await ok(url,'eth_getTransactionReceipt',[hash]);assert(receipt);receipts.push(receipt);headers.push(await ok(url,'eth_getBlockByHash',[receipt.blockHash,false]));if(url===nu){const fee=BigInt(parent.baseFeePerGas),used=BigInt(parent.gasUsed),target=BigInt(parent.gasLimit)/2n;const change=fee*(used>target?used-target:target-used)/target/8n;const expected=used===target?fee:used>target?fee+(change>0n?change:1n):fee-change;assert.equal(BigInt(headers[0].baseFeePerGas),expected,'Nano parent-derived base fee');}}
   fs.writeFileSync(path.join(out,label+'-nonce-'+nonce+'-gas-'+gasLimit+'-execution.json'),JSON.stringify({raw,hash,receipts,headers},null,2)+'\n');
   for(const field of ['status','type','gasUsed','effectiveGasPrice','transactionHash'])assert.equal(receipts[0][field],receipts[1][field],field);
   assert.equal(receipts[0].contractAddress?.toLowerCase()??null,receipts[1].contractAddress?.toLowerCase()??null,'contract address bytes');
   for(const field of ['number','timestamp','baseFeePerGas'])assert.equal(headers[0][field],headers[1][field],field);
   assert.deepEqual(logs(receipts[0]),logs(receipts[1]));
   const states=await Promise.all(urls.map(state));assert.deepEqual(states[0],states[1],'selected balances/nonces/votes differ');
   return {raw,hash,receipts,headers,states};
  }
  const deployment=await send(0,artifact.bytecode,3000000);assert.equal(deployment.receipts[0].status,'0x1');
  const delegation=await send(1,abi.encodeFunctionData('delegate',[wallet.address]),200000,address);assert.equal(delegation.receipts[0].status,'0x1');
  const request={from:wallet.address,to:address,data:abi.encodeFunctionData('transfer',[recipient,250000]),type:q(type),...Object.fromEntries(Object.entries(fees).map(([k,v])=>[k,q(v)]))};
  const before=await Promise.all(urls.map(state));assert.deepEqual(before[0],before[1]);
  const estimates=[];
  for(const url of urls)estimates.push({default:await ok(url,'eth_estimateGas',[request]),latest:await ok(url,'eth_estimateGas',[request,'latest']),pending:await ok(url,'eth_estimateGas',[request,'pending'])});
  assert.deepEqual(await Promise.all(urls.map(state)),before,'simulation changed canonical state');
  for(const e of estimates){assert.equal(e.default,e.pending);assert(BigInt(e.latest)<BigInt(e.default));}
  for(const source of ['nano-default','anvil-default','explicit-latest']){
   const snapshots=await Promise.all(urls.map(url=>ok(url,'evm_snapshot',[])));
   const quoted=source==='anvil-default'?estimates[1].default:source==='explicit-latest'?estimates[0].latest:estimates[0].default;
   const execution=await send(2,request.data,BigInt(quoted),address),success=source!=='explicit-latest';
   assert.equal(execution.receipts[0].status,success?'0x1':'0x0');
   for(const s of execution.states){assert.equal(BigInt(s.senderNonce),3n);assert.equal(BigInt(s.ownerTokens),success?750000n:1000000n);assert.equal(BigInt(s.recipientTokens),success?250000n:0n);assert.equal(BigInt(s.ownerVotes),success?750000n:1000000n);}
   if(!success){assert.equal(execution.receipts[0].gasUsed,quoted);assert.deepEqual(execution.receipts[0].logs,[]);}
   results.push({fork,type,source,request,estimates,quoted,deployment,delegation,before,...execution,expectedSuccess:success,matches:true});
   for(let i=0;i<urls.length;i++)assert.equal(await ok(urls[i],'evm_revert',[snapshots[i]]),true);
   assert.deepEqual(await Promise.all(urls.map(state)),before,'snapshot did not restore selected state');
  }
 }finally{for(const child of children.reverse())await stop(child);fs.rmSync(temp,{recursive:true,force:true});}
}}finally{fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({referenceVersion:version,scope:'Same signed ERC20Votes creations/delegations/transfers, controlled local development mining on Nano and Anvil; explicit latest failure preserved. Anvil next base fee explicitly aligned to Nano; Nano parent-derived fee independently checked. Selected account/token state and receipts, not complete state roots, production consensus or BFT.',results},null,2)+'\n');}
assert.equal(results.length,forks.length*9);console.log(`${results.length} signed transfer comparisons passed (${forks.length*6} default-budget successes and ${forks.length*3} intentional latest-budget failures)`);
