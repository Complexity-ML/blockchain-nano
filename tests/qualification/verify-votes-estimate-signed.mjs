import {readFileSync} from 'node:fs';
import {createRequire} from 'node:module';
import assert from 'node:assert/strict';
const {Transaction,keccak256}=createRequire(new URL('../hardhat/package.json',import.meta.url))('ethers');
const [file,forkList='Paris,Shanghai']=process.argv.slice(2);
const report=JSON.parse(readFileSync(file)),forks=forkList.split(','),expected=new Set();
for(const fork of forks)for(const type of [0,1,2])for(const source of ['nano-default','anvil-default','explicit-latest'])expected.add(`${fork}/${type}/${source}`);
assert.equal(report.results.length,expected.size);
const logs=r=>r.logs.map(l=>({address:l.address.toLowerCase(),topics:l.topics,data:l.data}));
for(const row of report.results){
 assert(expected.delete(`${row.fork}/${row.type}/${row.source}`),'unexpected or duplicate case');
 const tx=Transaction.from(row.raw),hash=keccak256(row.raw),success=row.source!=='explicit-latest';
 assert.equal(tx.chainId,1337n);assert.equal(tx.type,row.type);assert.equal(tx.nonce,2);assert.equal(tx.gasLimit,BigInt(row.quoted));assert.equal(row.hash,hash);
 const estimate=row.source==='anvil-default'?row.estimates[1].default:row.source==='explicit-latest'?row.estimates[0].latest:row.estimates[0].default;
 assert.equal(row.quoted,estimate);
 for(const e of row.estimates){assert.equal(e.default,e.pending);assert(BigInt(e.latest)<BigInt(e.default));}
 assert.deepEqual(row.before[0],row.before[1]);assert.deepEqual(row.states[0],row.states[1]);
 assert.deepEqual(logs(row.receipts[0]),logs(row.receipts[1]));
 for(let i=0;i<2;i++){
  const r=row.receipts[i],s=row.states[i],before=row.before[i];
  assert.equal(r.transactionHash,hash);assert.equal(r.status,success?'0x1':'0x0');assert.equal(BigInt(r.type),BigInt(row.type));
  assert.equal(BigInt(s.senderNonce),3n);assert.equal(BigInt(before.senderNonce),2n);
  assert.equal(BigInt(s.senderBalance),BigInt(before.senderBalance)-BigInt(r.gasUsed)*BigInt(r.effectiveGasPrice));
  assert.equal(BigInt(s.ownerTokens),success?750000n:1000000n);assert.equal(BigInt(s.recipientTokens),success?250000n:0n);assert.equal(BigInt(s.ownerVotes),success?750000n:1000000n);assert.equal(BigInt(s.supply),1000000n);
  if(!success){assert.equal(BigInt(r.gasUsed),tx.gasLimit);assert.deepEqual(r.logs,[]);}
 }
 for(const field of ['gasUsed','effectiveGasPrice'])assert.equal(row.receipts[0][field],row.receipts[1][field]);
 for(const [stage,nonce] of [[row.deployment,0],[row.delegation,1]]){
  assert.equal(Transaction.from(stage.raw).nonce,nonce);
  for(const r of stage.receipts){assert.equal(r.transactionHash,keccak256(stage.raw));assert.equal(r.status,'0x1');}
  assert.equal(stage.receipts[0].gasUsed,stage.receipts[1].gasUsed);
 }
}
assert.equal(expected.size,0);
console.log(JSON.stringify({cases:report.results.length,defaultSuccesses:forks.length*6,expectedLatestFailures:forks.length*3,exactCoverage:true,signedIdentity:true,selectedStateAndFeeAccounting:true}));
