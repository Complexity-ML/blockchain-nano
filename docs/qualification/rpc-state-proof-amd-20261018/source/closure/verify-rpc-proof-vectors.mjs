import assert from 'node:assert/strict';
import {readFileSync,statSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {dirname,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
const [input,inputSha,moduleFile,...extra]=process.argv.slice(2);
assert.equal(extra.length,0);assert(input&&moduleFile);assert.match(inputSha,/^[0-9a-f]{64}$/);
assert.equal(process.versions.node,'25.9.0');
const sha=b=>createHash('sha256').update(b).digest('hex');
const pinned=(file,hash,size)=>{const b=readFileSync(file);assert.equal(b.length,size,file);assert.equal(sha(b),hash,file);return b;};
pinned(moduleFile,'b59224b4a64231d493ccf21b98d4c5ddb5f5ce3ec9889b792190d0e0e0b090ce',5340);
pinned(resolve(dirname(moduleFile),'account-proof.mjs'),'2a07c2f6d0602250e85a71b563161fbbaafd5c8cac7d612f3c3c8c02909d31c9',2439);
pinned(resolve(dirname(moduleFile),'../hardhat/package.json'),'39a64e23c4cde8a0a0c3db20da85f0ac5c5e4508d9fe118c8a8c42fd0d1a2e8d',550);
assert(statSync(input).size>0&&statSync(input).size<=16*1024*1024);
const raw=readFileSync(input);assert(raw.length<=16*1024*1024);assert.equal(sha(raw),inputSha);
const text=new TextDecoder('utf-8',{fatal:true}).decode(raw);assert(text.endsWith('\n'));
const lines=text.slice(0,-1).split('\n');assert(lines.every(line=>line.length>0));
const records=lines.map(line=>JSON.parse(line));assert.equal(records.length,30);
assert(records.every(r=>r&&typeof r==='object'&&!Array.isArray(r)));
const vectors=records.filter(r=>r.scenario==='proof-vector');assert.equal(vectors.length,24);
const reports=records.filter(r=>r.scenario!=='proof-vector');
assert.deepEqual(reports.map(r=>r.scenario),[
 'get-proof-native-mpt-shapes-and-canonical-rlp','get-proof-account-storage-and-object-selectors',
 'get-proof-errors-configurable-budgets-and-cancellation','get-proof-pinned-reorg-real-retention-and-reopen',
 'get-proof-http-websocket-capacity-and-shutdown','get-proof-summary']);
assert(reports.every(r=>r.passed===true));const summary=records.at(-1);
for(const [key,value] of Object.entries({scenario:'get-proof-summary',groups:5,expectedRefusals:63,proofVectors:24,selectedGroup:'all',externalNetwork:false,consensusModified:false,productionQualified:false}))assert.equal(summary[key],value,key);
const {verifyProofVector}=await import(pathToFileURL(resolve(moduleFile)).href);
assert.equal(typeof verifyProofVector,'function');
const results=vectors.map((vector,index)=>{
 assert.deepEqual(Object.keys(vector).sort(),['address','expectedPresent','expectedValues','keys','response','root','scenario']);
 assert.equal(typeof vector.expectedPresent,'boolean');assert(Array.isArray(vector.keys)&&Array.isArray(vector.expectedValues));
 assert.equal(vector.expectedValues.length,vector.keys.length);
 const result=verifyProofVector(vector);assert.deepEqual(Object.keys(result).sort(),['exists','values']);
 assert.equal(result.exists,vector.expectedPresent);assert.deepEqual(result.values,vector.expectedValues);
 return {index,root:vector.root,address:vector.address,exists:result.exists,values:result.values};
});
console.log(JSON.stringify({format:'nano-rpc-proof-vectors-independent-check-v1',passed:true,inputSha256:inputSha,verifiedVectors:results.length,results,consensusRootAuthenticated:false,productionQualified:false}));
