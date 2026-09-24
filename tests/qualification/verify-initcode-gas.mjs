import {readFileSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {createRequire} from 'node:module';
import assert from 'node:assert/strict';
const {Transaction,keccak256}=createRequire(new URL('../hardhat/package.json',import.meta.url))('ethers');
const [inputs,results,output]=process.argv.slice(2);
const read=p=>JSON.parse(readFileSync(p));
const report=read(resolve(results,'summary.json'));
const expected=new Set();
for(const fork of ['Shanghai','Cancun','Prague','Osaka'])for(const type of [0,1,2])for(const length of [31,32,33,49151,49152])expected.add(`${fork}/${type}/${length}`);
assert.equal(report.results.length,60);assert.equal(report.summary.sequences,12);
for(const row of report.results){
 const input=read(resolve(inputs,row.case+'-input.json'));
 const n=read(resolve(results,row.case,'nano-result.json'));
 const g=read(resolve(results,row.case,'result.json'));
 const tx=Transaction.from(input.rawTransaction),bytes=Buffer.from(tx.data.slice(2),'hex');
 assert.equal(bytes.subarray(0,3).toString('hex'),'5f5ff3');assert(bytes.subarray(3).every(b=>b===0));
 assert.equal(tx.to,null);assert.equal(tx.chainId,1337n);assert.equal(input.fork,row.fork);
 assert(expected.delete(`${row.fork}/${tx.type}/${bytes.length}`),'unexpected or duplicate matrix case');
 const zero=bytes.filter(b=>b===0).length,nonzero=bytes.length-zero;
 const intrinsic=53000+zero*4+nonzero*16+2*Math.ceil(bytes.length/32);
 const floor=21000+10*(zero+4*nonzero);
 const gas=['Prague','Osaka'].includes(row.fork)?Math.max(intrinsic+4,floor):intrinsic+4;
 assert.equal(row.status,'matched');assert.equal(BigInt(n.intrinsicGas),BigInt(intrinsic));
 for(const receipt of [n.receipt,g.receipts[0]]){assert.equal(receipt.transactionHash,keccak256(input.rawTransaction));assert.equal(receipt.status,'0x1');assert.equal(BigInt(receipt.gasUsed),BigInt(gas));}
 assert.equal(n.stateRoot,g.stateRoot);
}
assert.equal(expected.size,0);
const summary={cases:60,sequences:12,fullMatrix:true,gasFormulaVerified:true,scope:'Reads signed bytes independently of generator expectations; full supplied poststate comparison is performed by compare-geth-sequence.mjs; no RPC estimation or admission assertions.'};
if(output)writeFileSync(output,JSON.stringify(summary,null,2)+'\n');console.log(JSON.stringify(summary));
