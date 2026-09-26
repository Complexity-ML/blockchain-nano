// Bounded local test only. Explicit container IDs are mandatory; never auto-select production.
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {createRequire} from 'node:module';
import {mkdir,writeFile} from 'node:fs/promises';
const require=createRequire(new URL('../hardhat/package.json',import.meta.url));
const {Wallet,keccak256}=require('ethers');
const exec=promisify(execFile), [a,b,out]=process.argv.slice(2);
if(!a||!b||!out||a===b)throw Error('Usage: node crash-recovery.mjs nano-0--ID nano-1--ID output-directory');
const evidence={scenario:'Kurtosis synthetic Beacon process crash under bounded SSTORE workload',started:new Date().toISOString(),events:[],passed:false};
await mkdir(out,{recursive:true});
const save=()=>writeFile(`${out}/result.json`,JSON.stringify(evidence,null,2)+'\n');
const event=(name,details={})=>{evidence.events.push({name,time:new Date().toISOString(),...details});console.log(JSON.stringify(evidence.events.at(-1)));};
async function docker(...args){return (await exec('docker',args,{timeout:20000,maxBuffer:4*1024*1024})).stdout;}
async function footprint(node){
 const files=await docker('exec',node,'find','/var/lib/nano','-type','f','-exec','stat','-c','%s %b %n','{}','+');
 const groups={};
 for(const line of files.trim().split('\n')){const m=/^(\d+) (\d+) (.*)$/.exec(line);if(!m)throw Error('Invalid stat record');const path=m[3],kind=path.endsWith('.log')?'wal':path.endsWith('.sst')?'sst':'other';const g=groups[kind]??={logicalBytes:0,allocatedBytes:0,files:0};g.logicalBytes+=Number(m[1]);g.allocatedBytes+=Number(m[2])*512;g.files++;}
 return {sampledAt:new Date().toISOString(),groups,duKiB:Number((await docker('exec',node,'du','-sk','/var/lib/nano')).split(/\s+/)[0])};
}
async function rpc(node,method,params=[]){const d=JSON.parse(await docker('exec',node,'curl','-fsS','--max-time','10','-H','Content-Type: application/json','--data',JSON.stringify({jsonrpc:'2.0',id:1,method,params}),'http://127.0.0.1:8545'));if(d.error)throw Error(JSON.stringify(d.error));return d.result;}
const pause=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,label,seconds=180){const deadline=Date.now()+seconds*1000;let error;while(Date.now()<deadline){try{const value=await fn();if(value)return value;}catch(e){error=e.message;}await pause(1000);}throw Error(`${label} timeout: ${error??'condition unmet'}`);}
try{
 const info=[];
 for(const [index,node] of [a,b].entries()){
  if(!node.startsWith(`nano-${index}--`))throw Error('Unexpected test container name');
  const i=JSON.parse(await docker('inspect',node))[0];
  if(i.Config.Image!=='blockchain-nano:local'||!i.Config.Cmd.join(' ').includes('/node/restart.json'))throw Error('Container is not the witnessed devnet');
  if(!i.Config.Labels||!JSON.stringify(i.Config.Labels).includes('kurtosis'))throw Error('Not a Kurtosis container');
  if(await rpc(node,'eth_chainId')!=='0x539')throw Error('Not synthetic chain 1337');
  info.push({id:i.Id,image:i.Image,volumes:i.Mounts.filter(m=>m.Destination==='/var/lib/nano').map(m=>m.Name)});
 }
 evidence.containers=info;
 evidence.footprintBefore=[await footprint(a),await footprint(b)];
 const base=await until(async()=>{const x=await rpc(a,'eth_getBlockByNumber',['finalized',false]),y=await rpc(b,'eth_getBlockByNumber',['finalized',false]);return x&&BigInt(x.number)>1n&&x.hash===y?.hash?x:false;},'baseline finality');
 const wallet=new Wallet('0x'+(777n).toString(16).padStart(64,'0')),target='0x0000000000000000000000000000000000001234';
 const startNonce=Number(BigInt(await rpc(a,'eth_getTransactionCount',[wallet.address,'latest'])));
 const startValue=BigInt(await rpc(a,'eth_getStorageAt',[target,'0x0','latest']));
 const raws=[],hashes=[];
 for(let i=0;i<24;i++){const raw=await wallet.signTransaction({chainId:1337,type:2,nonce:startNonce+i,to:target,gasLimit:100000,maxFeePerGas:2000000000n,maxPriorityFeePerGas:100000000n,value:0});raws.push(raw);hashes.push(keccak256(raw));}
 for(let i=0;i<24;i++){const result=await rpc(a,'eth_sendRawTransaction',[raws[i]]);if(result!==hashes[i])throw Error('Transaction hash mismatch');}
 event('workload-submitted',{transactions:24,startNonce,startValue:startValue.toString(),baselineFinalized:base.number});
 // No claim that the signal lands inside a RocksDB write: externally timed process failure.
 await docker('kill','--signal=KILL',a);
 const dead=JSON.parse(await docker('inspect',a))[0];
 if(dead.State.Running||dead.State.ExitCode!==137)throw Error('Expected SIGKILL exit 137');
 event('killed',{exitCode:dead.State.ExitCode,physicalPowerLoss:false,exactWriteBoundary:false});
 await pause(2000);await docker('start',a);
 await until(()=>rpc(a,'eth_chainId'),'RPC restart',30);
 const after=JSON.parse(await docker('inspect',a))[0];
 if(after.Id!==info[0].id||JSON.stringify(after.Mounts.filter(m=>m.Destination==='/var/lib/nano').map(m=>m.Name))!==JSON.stringify(info[0].volumes))throw Error('Persistent data volume replaced');
 if((await rpc(a,'eth_getBlockByNumber',[base.number,false])).hash!==base.hash)throw Error('Finalized prefix changed');
 event('reopened',{sameContainer:true,sameVolume:true,finalizedPrefixPreserved:true});
 // Mempool persistence is not promised: rebroadcast the SAME signed transactions.
 for(const raw of raws){try{await rpc(a,'eth_sendRawTransaction',[raw]);}catch(e){event('rebroadcast-reply',{reply:e.message});}}
 await until(async()=>{for(const hash of hashes){const r=await rpc(a,'eth_getTransactionReceipt',[hash]);if(!r)return false;if(r.status!=='0x1')throw Error('Workload transaction reverted');}return true;},'all workload receipts',120);
 const receipts=await Promise.all(hashes.map(h=>rpc(a,'eth_getTransactionReceipt',[h])));
 event('receipts-confirmed',{count:receipts.length});
 const height=receipts.reduce((m,r)=>BigInt(r.blockNumber)>m?BigInt(r.blockNumber):m,0n);
 const common=await until(async()=>{const x=await rpc(a,'eth_getBlockByNumber',['finalized',false]),y=await rpc(b,'eth_getBlockByNumber',['finalized',false]);return x&&BigInt(x.number)>=height&&BigInt(x.number)>BigInt(base.number)&&x.hash===y?.hash?x:false;},'post-crash common finality',240);
 for(const node of [a,b]){
  const value=BigInt(await rpc(node,'eth_getStorageAt',[target,'0x0',common.number]));
  const nonce=Number(BigInt(await rpc(node,'eth_getTransactionCount',[wallet.address,common.number])));
  if(value!==startValue+24n||nonce!==startNonce+24)throw Error('Lost or duplicated application effects');
 }
 evidence.receipts=receipts;evidence.finalized=common;evidence.passed=true;
 event('passed',{transactions:24,finalizedNumber:common.number,hash:common.hash,stateRoot:common.stateRoot});
}catch(e){evidence.error=e.stack;event('failed',{error:e.message});process.exitCode=1;}
finally{evidence.footprintAfter=[];for(const node of [a,b]){try{evidence.footprintAfter.push(await footprint(node));}catch(e){evidence.footprintAfter.push({error:e.message});}}for(const node of [a,b]){try{const p=await exec('docker',['logs',node],{timeout:10000,maxBuffer:4*1024*1024});await writeFile(`${out}/${node.split('--')[0]}.log`,p.stdout+p.stderr);}catch{}}await save();}
