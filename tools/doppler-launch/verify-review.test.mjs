import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {preview} from './preview.mjs';
import {verifyUnsignedReview} from './verify-review.mjs';

const configuration=JSON.parse(await readFile(new URL('./example.synthetic.json',import.meta.url)));
const persisted=v=>JSON.parse(JSON.stringify(v,(_,x)=>typeof x==='bigint'?x.toString():x));
let encoded;
const fixture=async()=>structuredClone(await(encoded??=preview(structuredClone(configuration)).then(persisted)));

test('offline verifier accepts the persisted SDK review without RPC or readiness claims',async()=>{
 const review=await fixture();
 const oldFetch=globalThis.fetch;globalThis.fetch=()=>{throw Error('Unexpected network access');};
 try{
  const result=await verifyUnsignedReview(review,configuration);
  assert.equal(result.reviewIntegrityVerified,true);assert.equal(result.calldataHash,review.calldataHash);
  assert.deepEqual(result.transaction,review.transaction);assert.deepEqual(result.predicted,review.predicted);
  for(const key of ['simulationVerified','nativeAssetRelationshipVerified','signed','broadcast','productionReady'])assert.equal(result[key],false);
 }finally{globalThis.fetch=oldFetch;}
});

test('persisted review mutations cannot replace approved configuration or SDK encoding',async(t)=>{
 const cases=[
  ['configuration',r=>r.config.initialSupply='123'],
  ['native relationship description',r=>r.nativeAssetRelationship='changed backing claim'],
  ['chain',r=>r.transaction.chainId=1],
  ['sender',r=>r.transaction.from='0x'+'22'.repeat(20)],
  ['recipient',r=>r.transaction.to='0x'+'22'.repeat(20)],
  ['value',r=>r.transaction.value='1'],
  ['calldata',r=>r.transaction.data+='00'],
  ['calldata digest',r=>r.calldataHash='0x'+'00'.repeat(32)],
  ['token prediction',r=>r.predicted.tokenAddress='0x'+'22'.repeat(20)],
  ['hook prediction',r=>r.predicted.hookAddress='0x'+'22'.repeat(20)],
  ['create parameters',r=>r.createParams.initialSupply='123'],
  ['hidden transaction field',r=>r.transaction.nonce='1'],
  ['unknown envelope field',r=>r.wallet='unexpected'],
  ['SDK version',r=>r.sdkVersion='another'],
  ['claimed production readiness',r=>r.productionReady=true],
 ];
 for(const[name,mutate]of cases)await t.test(name,async()=>{
  const review=await fixture();mutate(review);await assert.rejects(verifyUnsignedReview(review,configuration));
 });
});

test('review and approved terms are detached before the SDK await',async()=>{
 const review=await fixture(),approved=structuredClone(configuration),original=structuredClone(review);
 const pending=verifyUnsignedReview(review,approved);
 review.transaction.data='0x';review.predicted.tokenAddress='0x'+'22'.repeat(20);approved.initialSupply='123';
 const result=await pending;
 assert.deepEqual(result.transaction,original.transaction);assert.deepEqual(result.predicted,original.predicted);
 assert.equal(review.transaction.data,'0x');assert.equal(approved.initialSupply,'123');
});

test('retained successful or rejected simulation is never requalified offline',async()=>{
 for(const status of ['unsigned-preview','simulation-rejected']){
  const review=await fixture();review.status=status;
  review.simulation={performed:true,success:status!=='simulation-rejected',referenceHashPinned:true,gas:'123'};
  const result=await verifyUnsignedReview(review,configuration);
  assert.equal(result.reviewIntegrityVerified,true);assert.equal(result.retainedStatus,status);
  assert.equal(result.simulationVerified,false);assert.equal(result.productionReady,false);
 }
});
