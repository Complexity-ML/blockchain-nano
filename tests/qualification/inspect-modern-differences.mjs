import {readFile,readdir} from 'node:fs/promises';
import {resolve} from 'node:path';
import {createRequire} from 'node:module';
import assert from 'node:assert/strict';
const {keccak256,Transaction}=createRequire(new URL('../hardhat/package.json',import.meta.url))('ethers');
assert(process.argv[2],'Usage: node inspect-modern-differences.mjs <Anvil campaign>');
const directory=resolve(process.argv[2]),read=async file=>JSON.parse(await readFile(resolve(directory,file),'utf8'));
const cases=[];
for(const file of (await readdir(directory)).filter(f=>f.endsWith('-input.json')).sort()){
 const stem=file.slice(0,-11),input=await read(file),nano=await read(stem+'-nano.json'),reference=await read(stem+'-anvil.json');
 const differences=[];let checks=0;
 const equal=(field,a,b)=>{checks++;if(JSON.stringify(a)!==JSON.stringify(b))differences.push({field,nano:a,anvil:b});};
 const hash=keccak256(input.rawTransaction);assert.equal(nano.receipt.transactionHash,hash);assert.equal(reference.receipt.transactionHash,hash);
 for(const field of ['transactionHash','status','type','gasUsed','cumulativeGasUsed','logsBloom'])equal('receipt.'+field,nano.receipt[field],reference.receipt[field]);
 const logs=r=>r.logs.map(l=>({address:l.address.toLowerCase(),topics:l.topics,data:l.data}));equal('receipt.logs',logs(nano.receipt),logs(reference.receipt));
 const accounts=state=>new Map(Object.entries(state).map(([a,v])=>[a.toLowerCase(),v]));const left=accounts(nano.post),right=accounts(reference.post);
 for(const address of new Set([...left.keys(),...right.keys()])){
  const a=left.get(address)??{},b=right.get(address)??{};
  for(const field of ['balance','nonce'])equal(address+'.'+field,BigInt(a[field]??0).toString(),BigInt(b[field]??0).toString());
  equal(address+'.code',(a.code??'0x').toLowerCase(),(b.code??'0x').toLowerCase());
  const slots=s=>new Map(Object.entries(s??{}).map(([k,v])=>[BigInt(k).toString(),BigInt(v).toString()]));const sa=slots(a.storage),sb=slots(b.storage);
  for(const slot of new Set([...sa.keys(),...sb.keys()]))equal(address+'.storage.'+slot,sa.get(slot)??'0',sb.get(slot)??'0');
 }
 let feeAccounting;
 if(differences.length){
  const tx=Transaction.from(input.rawTransaction),sender=tx.from.toLowerCase(),miner=input.block.miner.toLowerCase();
  const gasDelta=BigInt(nano.receipt.gasUsed)-BigInt(reference.receipt.gasUsed);
  const price=BigInt(reference.receipt.effectiveGasPrice),baseFee=BigInt(input.block.baseFeePerGas??0);
  const balances=new Map();for(const [address,amount] of [[sender,-gasDelta*price],[miner,gasDelta*(price-baseFee)]])balances.set(address,(balances.get(address)??0n)+amount);
  const explainsAll=differences.every(d=>{
   if(['receipt.gasUsed','receipt.cumulativeGasUsed'].includes(d.field))return BigInt(d.nano)-BigInt(d.anvil)===gasDelta;
   if(d.field.endsWith('.balance'))return balances.get(d.field.slice(0,-8))===BigInt(d.nano)-BigInt(d.anvil);
   return false;
  });
  feeAccounting={gasDelta:gasDelta.toString(),effectiveGasPrice:price.toString(),baseFee:baseFee.toString(),explainsAllDifferences:explainsAll};
 }
 cases.push({case:stem,fork:input.fork,transactionHash:hash,checks,differences,...(feeAccounting?{feeAccounting}:{})});
}
const report=await read('results.json');assert.equal(cases.length,report.results.length,'Missing detailed transaction');
const summary={transactions:cases.length,matched:cases.filter(r=>!r.differences.length).length,mismatched:cases.filter(r=>r.differences.length).length,checks:cases.reduce((n,r)=>n+r.checks,0)};
console.log(JSON.stringify({scope:'All compared receipt fields and union of supplied account/slot values, without stopping at first difference; missing accounts normalize to empty values, not trie presence; no global Anvil state root claim',summary,cases},null,2));
process.exitCode=summary.mismatched?1:0;
