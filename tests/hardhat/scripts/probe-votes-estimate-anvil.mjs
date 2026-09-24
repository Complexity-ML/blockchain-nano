import fs from 'node:fs';
import path from 'node:path';
import {spawn,execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import assert from 'node:assert/strict';
const {Wallet,Interface,getCreateAddress,keccak256}=createRequire(import.meta.url)('ethers');
const binary=process.env.ANVIL_BINARY,out=process.env.NANO_DIAGNOSTIC_REPORT_DIR;assert(binary&&out);
fs.mkdirSync(out,{recursive:false});
const artifact=JSON.parse(fs.readFileSync(new URL('../artifacts/contracts/DeveloperGovernance.sol/DeveloperVotes.json',import.meta.url))),abi=new Interface(artifact.abi);
const version=execFileSync(binary,['--version'],{encoding:'utf8'}).trim(),wallet=new Wallet('0x'+'1'.padStart(64,'0')),recipient=new Wallet('0x'+'2'.padStart(64,'0')).address;
const address=getCreateAddress({from:wallet.address,nonce:0}),rows=[];
async function rpc(url,method,params){const r=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params}),signal:AbortSignal.timeout(10000)});assert(r.ok);const j=await r.json();assert(!j.error,JSON.stringify(j));return j.result;}
try {for(const fork of ['Paris','Shanghai']){
 const child=spawn(binary,['--host','127.0.0.1','--port','0','--chain-id','1337','--hardfork',fork.toLowerCase(),'--base-fee','1000000000'],{stdio:['ignore','pipe','pipe']});let text='';
 child.stdout.on('data',d=>text+=d);child.stderr.on('data',d=>text+=d);
 try {
  const url=await new Promise((resolve,reject)=>{const t=setTimeout(()=>reject(Error('startup timeout')),10000);child.once('error',reject);child.once('exit',()=>{clearTimeout(t);reject(Error('early exit'));});child.stdout.on('data',()=>{const m=text.match(/Listening on 127\.0\.0\.1:(\d+)/i);if(m){clearTimeout(t);resolve('http://127.0.0.1:'+m[1]);}});});
  const call=(method,params)=>rpc(url,method,params);
  await call('anvil_setBalance',[wallet.address,'0xd3c21bcecceda1000000']);
  async function send(nonce,data,gasLimit,to){const raw=await wallet.signTransaction({type:0,chainId:1337,nonce,gasPrice:2000000000n,gasLimit,data,to}),hash=keccak256(raw);assert.equal(await call('eth_sendRawTransaction',[raw]),hash);let receipt;for(let i=0;i<100;i++){receipt=await call('eth_getTransactionReceipt',[hash]);if(receipt)break;await new Promise(r=>setTimeout(r,50));}assert(receipt);return {raw,receipt};}
  const deployed=await send(0,artifact.bytecode,3000000);assert.equal(deployed.receipt.status,'0x1');
  const delegated=await send(1,abi.encodeFunctionData('delegate',[wallet.address]),200000,address);assert.equal(delegated.receipt.status,'0x1');
  const request={from:wallet.address,to:address,data:abi.encodeFunctionData('transfer',[recipient,250000]),gasPrice:'0x77359400'};
  const estimates={default:await call('eth_estimateGas',[request]),latest:await call('eth_estimateGas',[request,'latest']),pending:await call('eth_estimateGas',[request,'pending'])};
  const state=async()=>({nonce:await call('eth_getTransactionCount',[wallet.address,'latest']),owner:await call('eth_call',[{to:address,data:abi.encodeFunctionData('balanceOf',[wallet.address])},'latest']),recipient:await call('eth_call',[{to:address,data:abi.encodeFunctionData('balanceOf',[recipient])},'latest'])});
  const before=await state();assert.equal(before.nonce,'0x2');
  for(const source of ['default','latest','pending']){
   const snapshot=await call('evm_snapshot',[]);
   const execution=await send(2,request.data,BigInt(estimates[source]),address),after=await state();
   assert.equal(after.nonce,'0x3');
   const success=execution.receipt.status==='0x1';assert.equal(BigInt(after.owner),success?750000n:1000000n);assert.equal(BigInt(after.recipient),success?250000n:0n);
   rows.push({fork,source,estimates,request,deployed,delegated,before,...execution,after,success});
   assert.equal(await call('evm_revert',[snapshot]),true);assert.deepEqual(await state(),before);
  }
 } finally {
  if(child.exitCode===null&&child.signalCode===null){const done=new Promise(r=>child.once('exit',r));child.kill('SIGTERM');const t=setTimeout(()=>child.kill('SIGKILL'),5000);await done;clearTimeout(t);}
  fs.writeFileSync(path.join(out,fork+'-anvil.log'),text);
 }
}} finally {fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({version,scope:'Official Anvil only; synthetic ERC20Votes deployment, immediately following checkpoint write; snapshots restore each quoted-budget branch. Not complete chain equivalence.',rows},null,2)+'\n');}
assert.equal(rows.length,6);console.log(JSON.stringify(rows.map(({fork,source,estimates,success,receipt})=>({fork,source,estimates,success,gasUsed:receipt.gasUsed}))));
