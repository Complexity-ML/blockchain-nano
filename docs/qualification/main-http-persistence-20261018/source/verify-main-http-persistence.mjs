import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {createRequire} from 'node:module';
import {dirname,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
const [vectorsFile,vectorsSha,rawFile,rawSha,moduleFile,...extra]=process.argv.slice(2);
assert.equal(extra.length,0);assert.equal(process.versions.node,'25.9.0');
const sha=b=>createHash('sha256').update(b).digest('hex');
const read=(file,expected,maximum)=>{const b=readFileSync(file);assert(b.length>0&&b.length<=maximum);assert.equal(sha(b),expected);return b;};
read(moduleFile,'b59224b4a64231d493ccf21b98d4c5ddb5f5ce3ec9889b792190d0e0e0b090ce',5340);
read(resolve(dirname(moduleFile),'account-proof.mjs'),'2a07c2f6d0602250e85a71b563161fbbaafd5c8cac7d612f3c3c8c02909d31c9',2439);
const packageFile=resolve(dirname(moduleFile),'../hardhat/package.json');
read(packageFile,'39a64e23c4cde8a0a0c3db20da85f0ac5c5e4508d9fe118c8a8c42fd0d1a2e8d',550);
const require=createRequire(pathToFileURL(packageFile));
const {encodeRlp,keccak256,Transaction}=require('ethers');
const {verifyProofVector}=await import(pathToFileURL(resolve(moduleFile)).href);
const raws=JSON.parse(read(rawFile,rawSha,65536));assert(Array.isArray(raws)&&raws.length===1);
const tx=Transaction.from(raws[0]);assert.equal(tx.serialized,raws[0]);
assert.equal(tx.hash,'0x8d2e4fa292b46a6ae5cc2a0481c6b1447176b84613cb88f9e76c3b8a55ed4213');
assert.equal(tx.chainId,1n);assert(tx.from&&tx.to);assert(tx.signature); // Existing signed bytes only; no signer is instantiated.
const vectors=read(vectorsFile,vectorsSha,1048576).toString().trimEnd().split('\n').map(s=>JSON.parse(s));
assert.equal(vectors.length,5,'exact initial, post-transaction, historical, second-block and reopened historical captures');
const quantity=v=>{assert.match(v,/^0x(?:0|[1-9a-f][0-9a-f]*)$/);let h=BigInt(v).toString(16);return h==='0'?'0x':`0x${h.length%2?'0':''}${h}`;};
const fixed=(v,n)=>{assert.match(v,new RegExp(`^0x[0-9a-f]{${n*2}}$`));return v;};
const headers=new Map();const outcomes=[];
for(const [index,v] of vectors.entries()){
 const h=v.header;assert(h&&typeof h==='object');
 const fields=[fixed(h.parentHash,32),fixed(h.sha3Uncles,32),fixed(h.miner.toLowerCase(),20),fixed(h.stateRoot,32),fixed(h.transactionsRoot,32),fixed(h.receiptsRoot,32),fixed(h.logsBloom,256),...['difficulty','number','gasLimit','gasUsed','timestamp'].map(k=>quantity(h[k])),h.extraData,fixed(h.mixHash,32),fixed(h.nonce,8)];
 assert.match(h.extraData,/^0x(?:[0-9a-f]{2}){0,32}$/);
 assert(h.baseFeePerGas!==undefined);fields.push(quantity(h.baseFeePerGas));
 if(h.withdrawalsRoot!==undefined)fields.push(fixed(h.withdrawalsRoot,32));
 for(const key of ['blobGasUsed','excessBlobGas','parentBeaconBlockRoot','requestsHash'])assert.equal(h[key],undefined,'This scenario qualifies only Shanghai');
 assert.equal(keccak256(encodeRlp(fields)),h.hash,'independent execution header RLP/hash');
 assert.equal(v.root,h.stateRoot,'proof must bind the actual execution header');
 const number=Number(BigInt(h.number));assert(Number.isSafeInteger(number)&&number>=0&&number<=2);
 assert.equal(number,[0,1,0,2,1][index],'exact five-capture historical/reopen sequence');
 assert.equal(v.address,'0x5cacb0cd1bea69d7453cab8dfe870a90f5fff30b');
 assert.deepEqual(v.keys,['0x0','0x1']);assert.equal(v.expectedPresent,true);
 assert.deepEqual(v.expectedValues,number===0?['0x0','0x0']:['0x0','0x67'],'fixture constants, never candidate-derived expectations');
 if(headers.has(number))assert.deepEqual(h,headers.get(number),'same historical header on reopen');else headers.set(number,h);
 const result=verifyProofVector(v);outcomes.push({index,blockNumber:number,blockHash:h.hash,...result});
}
assert.deepEqual([...headers.keys()].sort(),[0,1,2]);
for(let number=1;number<=2;++number){const h=headers.get(number),parent=headers.get(number-1);assert.equal(h.parentHash,parent.hash);assert(BigInt(h.timestamp)>BigInt(parent.timestamp));}
console.log(JSON.stringify({format:'nano-main-http-persistence-independent-v1',passed:true,verifiedVectors:vectors.length,executionHeaderHashesVerified:3,inputSha256:vectorsSha,rawInputSha256:rawSha,existingTransactionHash:tx.hash,recoveredSender:tx.from.toLowerCase(),transactionDestination:tx.to.toLowerCase(),newSignaturesCreated:0,outcomes,consensusRootAuthenticated:false,beaconFinalityQualified:false,productionQualified:false}));
