import {verifyAccountProof} from './account-proof.mjs';
import {readFileSync,writeFileSync,mkdirSync,readdirSync,existsSync} from 'node:fs';
import {resolve,basename} from 'node:path';
import {createRequire} from 'node:module';
import {spawnSync,execFileSync} from 'node:child_process';
import assert from 'node:assert/strict';
const require=createRequire(new URL('../hardhat/package.json',import.meta.url));
const {encodeRlp,decodeRlp,keccak256,zeroPadValue,toBeHex}=require('ethers');
const [binary,inputs,output]=process.argv.slice(2);assert(binary&&inputs&&output,'usage: geth-evm input-directory output-directory');
mkdirSync(output,{recursive:true});
const version=execFileSync(binary,['--version'],{encoding:'utf8'}).trim();const rows=[];
for(const file of readdirSync(inputs).filter(f=>f.endsWith('-input.json'))){
 const input=JSON.parse(readFileSync(resolve(inputs,file)));const stem=basename(file,'-input.json');
 const dir=resolve(output,stem);mkdirSync(dir,{recursive:true});
 const alloc=structuredClone(input.pre);
 if(process.env.NANO_USE_ACCOUNT_PROOFS==='1'&&input.rawTransaction.startsWith('0x04')){
  const prefix=stem.replace(/-\d+$/,'');const perTransactionProof=resolve(inputs,stem+'-authority-proof.json');const proofPath=existsSync(perTransactionProof)?perTransactionProof:resolve(inputs,prefix+'-authority-proof.json');
  if(existsSync(proofPath)){
   const proof=JSON.parse(readFileSync(proofPath));
   const nonce=Number(stem.match(/-(\d+)$/)[1]);
   const parentPath=resolve(inputs,prefix+'-'+(nonce-1)+'-input.json');
   const parent=JSON.parse(readFileSync(parentPath)).block;
   assert.equal(input.block.parentHash,parent.hash,'wrong proof parent');
   const checked=verifyAccountProof(parent.stateRoot,proof.address,proof.accountProof);
   const address=proof.address.toLowerCase();const a=alloc[address];assert(a,'proof address not tracked');
   if(!checked.exists){
    assert.equal(BigInt(a.balance),0n);assert.equal(BigInt(a.nonce),0n);assert.equal(a.code,'0x');assert.equal(Object.keys(a.storage).length,0);
    delete alloc[address];
   }
   writeFileSync(resolve(dir,'account-proof-check.json'),JSON.stringify({proofPath,parentPath,parentHash:parent.hash,stateRoot:parent.stateRoot,address,...checked,removedFromAlloc:!checked.exists},null,2)+'\n');
  }
 }

 for(const a of Object.values(alloc)){a.storage=Object.fromEntries(Object.entries(a.storage).map(([k,v])=>[zeroPadValue(toBeHex(BigInt(k)),32),zeroPadValue(toBeHex(BigInt(v)),32)]));}
 const b=input.block;
 const env={currentCoinbase:b.miner,currentGasLimit:b.gasLimit,currentNumber:b.number,currentTimestamp:b.timestamp,currentRandom:b.mixHash,currentBaseFee:b.baseFeePerGas,currentExcessBlobGas:b.excessBlobGas,parentBeaconBlockRoot:b.parentBeaconBlockRoot,withdrawals:[],blockHashes:{[toBeHex(BigInt(b.number)-1n)]:b.parentHash}};
 const raw=input.rawTransaction;
 const tx=Number.parseInt(raw.slice(2,4),16)>=0xc0?decodeRlp(raw):raw;
 for(const [name,value] of [['alloc.json',alloc],['env.json',env],['txs.rlp',encodeRlp([tx])]])writeFileSync(resolve(dir,name),JSON.stringify(value,null,2)+'\n');
 const args=['t8n','--state.fork='+input.fork,'--state.chainid=1337','--state.reward=-1','--input.alloc='+resolve(dir,'alloc.json'),'--input.env='+resolve(dir,'env.json'),'--input.txs='+resolve(dir,'txs.rlp'),'--output.basedir='+dir,'--output.alloc=post.json','--output.result=result.json','--output.body=body.rlp'];
 const run=spawnSync(binary,args,{encoding:'utf8',timeout:15000});writeFileSync(resolve(dir,'process.json'),JSON.stringify({args,status:run.status,stdout:run.stdout,stderr:run.stderr,error:run.error?.message},null,2)+'\n');
 let error;
 try{
 assert.ifError(run.error);assert.equal(run.status,0,run.stderr);
 const result=JSON.parse(readFileSync(resolve(dir,'result.json')));assert.equal(result.rejected?.length??0,0);assert.equal(result.receipts.length,1);
 const reference=result.receipts[0];reference.type??='0x0';reference.logs??=[];const nano=JSON.parse(readFileSync(resolve(inputs,stem+'-nano.json')));
 assert.equal(reference.transactionHash,keccak256(raw));
 for(const field of ['transactionHash','status','type','gasUsed','cumulativeGasUsed','logsBloom'])assert.equal(reference[field],nano.receipt[field],field);
 assert.deepEqual(reference.logs.map(l=>({address:l.address,topics:l.topics,data:l.data})),nano.receipt.logs.map(l=>({address:l.address,topics:l.topics,data:l.data})));
 const post=JSON.parse(readFileSync(resolve(dir,'post.json')));
 for(const address of new Set([...Object.keys(alloc),...Object.keys(nano.post)])){
 const ref=post[address]??post[address.slice(2)]??{};const actual=nano.post[address]??{};
 for(const field of ['balance','nonce'])assert.equal(BigInt(ref[field]??0),BigInt(actual[field]??0),address+' '+field);
 assert.equal(ref.code??'0x',actual.code??'0x',address+' code');
 for(const slot of [0,1,2]){const key=zeroPadValue(toBeHex(slot),32);assert.equal(BigInt(ref.storage?.[key]??ref.storage?.[key.slice(2)]??0),BigInt(actual.storage?.[toBeHex(slot)]??actual.storage?.['0x'+slot.toString(16)]??0),address+' storage '+slot);}
 }
 }catch(e){error=e.message;}
 const row={case:stem,fork:input.fork,status:error?'mismatch':'matched',...(error?{error}:{})};rows.push(row);console.log(JSON.stringify(row));
}
const mismatched=rows.filter(r=>r.status!=='matched').length;
writeFileSync(resolve(output,'summary.json'),JSON.stringify({version,matched:rows.length-mismatched,mismatched,results:rows},null,2)+'\n');
console.log(JSON.stringify({summary:{matched:rows.length-mismatched,mismatched,version,scope:'Signed transaction replay and tracked account/slot comparison; no consensus validation'}}));process.exitCode=mismatched?1:0;
