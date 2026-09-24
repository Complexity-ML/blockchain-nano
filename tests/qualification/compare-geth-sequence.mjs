import {readFileSync,writeFileSync,mkdirSync,readdirSync} from 'node:fs';
import {resolve,basename} from 'node:path';
import {createRequire} from 'node:module';
import {spawnSync,execFileSync} from 'node:child_process';
import assert from 'node:assert/strict';
const require=createRequire(new URL('../hardhat/package.json',import.meta.url));
const {encodeRlp,decodeRlp,keccak256,zeroPadValue,toBeHex}=require('ethers');
const [geth,nanoBinary,inputs,output]=process.argv.slice(2);assert(geth&&nanoBinary&&inputs&&output,'usage: geth nano inputs output');
const write=(path,value)=>writeFileSync(path,JSON.stringify(value,null,2)+'\n');
const read=path=>JSON.parse(readFileSync(path));
const quantity=n=>'0x'+BigInt(n??0).toString(16);
function canonical(state){
 const result={};
 for(const [addr,a] of Object.entries(state).sort()){
  const balance=quantity(a.balance),nonce=quantity(a.nonce),code=a.code??'0x';
  const storage=Object.fromEntries(Object.entries(a.storage??{}).map(([k,v])=>[zeroPadValue(toBeHex(BigInt(k.startsWith('0x')?k:'0x'+k)),32),zeroPadValue(toBeHex(BigInt(v.startsWith('0x')?v:'0x'+v)),32)]).filter(([,v])=>BigInt(v)!==0n).sort());
  if(balance==='0x0'&&nonce==='0x0'&&code==='0x'){assert.equal(Object.keys(storage).length,0,'empty account with storage');continue;}
  result[(addr.startsWith('0x')?addr:'0x'+addr).toLowerCase()]={balance,nonce,code,storage};
 }
 return result;
}
mkdirSync(output,{recursive:true});const version=execFileSync(geth,['--version'],{encoding:'utf8'}).trim();const rows=[];
const groups=new Map();
for(const file of readdirSync(inputs).filter(f=>f.endsWith('-input.json'))){const stem=basename(file,'-input.json'),key=stem.replace(/-\d+$/,'');if(!groups.has(key))groups.set(key,[]);groups.get(key).push({file,stem,nonce:Number(stem.match(/-(\d+)$/)[1])});}
for(const [group,cases] of groups){
 cases.sort((a,b)=>a.nonce-b.nonce);assert.equal(cases[0].nonce,0);cases.forEach((c,i)=>assert.equal(c.nonce,i,'noncontiguous transaction sequence'));
 // Explicit synthetic common genesis: only funded/code/nonce accounts, never inferred trie presence.
 // Subsequent prestates are never refreshed from Anvil or from the other engine.
 const initial=canonical(read(resolve(inputs,cases[0].file)).pre);
 const groupDir=resolve(output,group);mkdirSync(groupDir,{recursive:true});write(resolve(groupDir,'common-initial-state.json'),initial);
 let nanoState=structuredClone(initial),gethState=structuredClone(initial);
 for(const {file,stem} of cases){
  const original=read(resolve(inputs,file)),b=original.block,raw=original.rawTransaction,dir=resolve(output,stem);mkdirSync(dir,{recursive:true});
  const input={...original,pre:nanoState};write(resolve(dir,'nano-input.json'),input);
  const n=spawnSync(nanoBinary,[],{input:JSON.stringify(input),encoding:'utf8',timeout:15000});write(resolve(dir,'nano-process.json'),{status:n.status,stdout:n.stdout,stderr:n.stderr,error:n.error?.message});assert.ifError(n.error);assert.equal(n.status,0,n.stderr);
  const actual=JSON.parse(n.stdout);write(resolve(dir,'nano-result.json'),actual);nanoState=actual.post;
  const env={currentCoinbase:b.miner,currentGasLimit:b.gasLimit,currentNumber:b.number,currentTimestamp:b.timestamp,currentRandom:b.mixHash,currentBaseFee:b.baseFeePerGas,currentExcessBlobGas:b.excessBlobGas,parentBeaconBlockRoot:b.parentBeaconBlockRoot,withdrawals:['Paris','Berlin'].includes(original.fork)?undefined:[],blockHashes:{[quantity(BigInt(b.number)-1n)]:b.parentHash}};
  const tx=Number.parseInt(raw.slice(2,4),16)>=0xc0?decodeRlp(raw):raw;
  for(const [name,value] of [['alloc.json',gethState],['env.json',env],['txs.rlp',encodeRlp([tx])]])write(resolve(dir,name),value);
  const args=['t8n','--state.fork='+original.fork,'--state.chainid=1337','--state.reward=-1','--input.alloc='+resolve(dir,'alloc.json'),'--input.env='+resolve(dir,'env.json'),'--input.txs='+resolve(dir,'txs.rlp'),'--output.basedir='+dir,'--output.alloc=post.json','--output.result=result.json','--output.body=body.rlp'];
  const g=spawnSync(geth,args,{encoding:'utf8',timeout:15000});write(resolve(dir,'geth-process.json'),{args,status:g.status,stdout:g.stdout,stderr:g.stderr,error:g.error?.message});assert.ifError(g.error);assert.equal(g.status,0,g.stderr);
  const result=read(resolve(dir,'result.json'));gethState=read(resolve(dir,'post.json'));
  let error;
  try{
   assert.equal(result.rejected?.length??0,0);assert.equal(result.receipts.length,1);const ref=result.receipts[0];ref.type??='0x0';ref.logs??=[];
   assert.equal(ref.transactionHash,keccak256(raw));assert.equal(actual.receipt.transactionHash,keccak256(raw));
   for(const field of ['transactionHash','type','status','gasUsed','cumulativeGasUsed','logsBloom'])assert.equal(actual.receipt[field],ref[field],field);
   const logs=r=>r.logs.map(l=>({address:l.address,topics:l.topics,data:l.data}));assert.deepEqual(logs(actual.receipt),logs(ref));
   assert.deepEqual(canonical(nanoState),canonical(gethState),'complete supplied poststate differs');
   assert.equal(actual.stateRoot,result.stateRoot,'computed poststate root differs');
  }catch(e){error=e.message;}
  const row={case:stem,fork:original.fork,status:error?'mismatch':'matched',nanoGas:actual.receipt.gasUsed,gethGas:result.receipts?.[0]?.gasUsed,nanoRoot:actual.stateRoot,gethRoot:result.stateRoot,...(error?{error}:{})};rows.push(row);console.log(JSON.stringify(row));
 }
}
const mismatched=rows.filter(r=>r.status!=='matched').length;const summary={matched:rows.length-mismatched,mismatched,sequences:groups.size,version,scope:'Independent sequential execution from explicit common synthetic state; same signed bytes and supplied block context; complete supplied poststate and root; no header/consensus or global Anvil state-root validation'};write(resolve(output,'summary.json'),{summary,results:rows});console.log(JSON.stringify({summary}));process.exitCode=mismatched?1:0;
