import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {observeBaseFinalizedReceipt} from './finality.mjs';
const tx='0x'+'a'.repeat(64),block='0x'+'b'.repeat(64),final='0x'+'c'.repeat(64),emitter='0x'+'d'.repeat(40),topic='0x'+'e'.repeat(64),code='0x60006000';
const event={emitter,topics:[topic],data:'0x1234',logIndex:'0x2',codeSha256:createHash('sha256').update(Buffer.from(code.slice(2),'hex')).digest('hex')};
function fixture(mutator=()=>{}){
 const calls=[],counts=new Map();const saved={transactionHash:tx,blockHash:block,blockNumber:'0xa',transactionIndex:'0x1',status:'0x1',logs:[{address:emitter,topics:[topic],data:'0x1234',logIndex:'0x2',transactionIndex:'0x1',transactionHash:tx,blockHash:block,blockNumber:'0xa',removed:false}]};
 const rpc={request:async({method,params})=>{
  calls.push({method,params});const key=method+JSON.stringify(params);const nth=(counts.get(key)??0)+1;counts.set(key,nth);let value;
  if(method==='eth_chainId')value='0x2105';
  if(method==='eth_getTransactionReceipt')value=structuredClone(saved);
  if(method==='eth_getBlockByNumber')value=params[0]==='finalized'?{number:'0xb',hash:final}:params[0]==='0xa'?{number:'0xa',hash:block}:{number:'0xb',hash:final};
  if(method==='eth_getCode')value=code;
  return mutator({method,params,nth,value})??value;
 }};return {rpc,calls};
}
test('finalized canonical event is observed with explicit mint prohibition',async()=>{
 const {rpc,calls}=fixture();const result=await observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event});assert.equal(result.eligibleForMint,false);assert.equal(result.cryptographicFinalityVerified,false);assert.equal(result.rpcFinalizedObservation,true);assert.equal(result.sourceEventKey,`8453:${tx}:0x2`);
 assert.deepEqual(calls.find(c=>c.method==='eth_getCode').params,[emitter,{blockHash:block,requireCanonical:true}]);assert.ok(!calls.some(c=>JSON.stringify(c).includes('"latest"')||JSON.stringify(c).includes('"safe"')));
});
const attacks={
 wrongChain:o=>o.method==='eth_chainId'?'0x1':undefined,
 chainChanged:o=>o.method==='eth_chainId'&&o.nth===2?'0x1':undefined,
 notFinalized:o=>o.method==='eth_getBlockByNumber'&&o.params[0]==='finalized'?{number:'0x9',hash:final}:undefined,
 wrongTransaction:o=>o.method==='eth_getTransactionReceipt'?{...o.value,transactionHash:'0x'+'f'.repeat(64)}:undefined,
 failedReceipt:o=>o.method==='eth_getTransactionReceipt'?{...o.value,status:'0x0'}:undefined,
 removedLog:o=>{if(o.method==='eth_getTransactionReceipt'){o.value.logs[0].removed=true;return o.value;}},
 logBlockMismatch:o=>{if(o.method==='eth_getTransactionReceipt'){o.value.logs[0].blockHash=final;return o.value;}},
 duplicatedLog:o=>{if(o.method==='eth_getTransactionReceipt'){o.value.logs.push({...o.value.logs[0]});return o.value;}},
 wrongEmitter:o=>{if(o.method==='eth_getTransactionReceipt'){o.value.logs[0].address='0x'+'a'.repeat(40);return o.value;}},
 wrongPayload:o=>{if(o.method==='eth_getTransactionReceipt'){o.value.logs[0].data='0x5678';return o.value;}},
 wrongTopic:o=>{if(o.method==='eth_getTransactionReceipt'){o.value.logs[0].topics=[block];return o.value;}},
 noncanonicalQuantity:o=>o.method==='eth_getTransactionReceipt'?{...o.value,blockNumber:'0x0a'}:undefined,
 missingCode:o=>o.method==='eth_getCode'?'0x':undefined,
 changedCode:o=>o.method==='eth_getCode'?'0x6001':undefined,
 receiptReorg:o=>o.method==='eth_getBlockByNumber'&&o.params[0]==='0xa'&&o.nth===2?{number:'0xa',hash:final}:undefined,
 finalizedReorg:o=>o.method==='eth_getBlockByNumber'&&o.params[0]==='0xb'?{number:'0xb',hash:block}:undefined,
 finalizedRegression:o=>o.method==='eth_getBlockByNumber'&&o.params[0]==='finalized'&&o.nth===2?{number:'0x9',hash:final}:undefined,
 receiptChanged:o=>{if(o.method==='eth_getTransactionReceipt'&&o.nth===2){o.value.logs[0].data='0x5678';return o.value;}},
 excessiveLogs:o=>{if(o.method==='eth_getTransactionReceipt'){o.value.logs=Array(257).fill(o.value.logs[0]);return o.value;}}
};
for(const [name,mutator] of Object.entries(attacks))test(`source gate rejects ${name}`,async()=>{const {rpc}=fixture(mutator);await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event}));});
test('unsupported EIP-1898 is fatal without a block-number fallback',async()=>{const {rpc,calls}=fixture();const request=rpc.request;rpc.request=o=>{if(o.method==='eth_getCode')throw Error('Unsupported canonical block hash');return request(o);};await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event}),/Unsupported/);assert.equal(calls.filter(c=>c.method==='eth_getBlockByNumber'&&c.params[0]==='0xa').length,1);});
test('invalid reviewed event is rejected before RPC access',async()=>{for(const bad of [{...event,codeSha256:''},{...event,topics:[]},{...event,logIndex:'0x02'},{...event,emitter:'0x'+'0'.repeat(40)}]){const {rpc,calls}=fixture();await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event:bad}));assert.equal(calls.length,0);}});

test('finalized hash replacement at the same height is rejected',async()=>{const {rpc}=fixture(o=>o.method==='eth_getBlockByNumber'&&o.params[0]==='finalized'&&o.nth===2?{number:'0xb',hash:block}:undefined);await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event}),/same height/);});
test('advancing finalized head is accepted only with its matching canonical header',async()=>{
 for(const corrupted of [false,true]){const {rpc}=fixture(o=>{
  if(o.method==='eth_getBlockByNumber'&&o.params[0]==='finalized'&&o.nth===2)return {number:'0xc',hash:final};
  if(o.method==='eth_getBlockByNumber'&&o.params[0]==='0xc')return {number:'0xc',hash:corrupted?block:final};
 });
 if(corrupted)await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event}),/not canonical/);
 else assert.equal((await observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event})).eligibleForMint,false);
 }
});

test('reviewed event remains fixed when the caller changes its configuration during RPC',async()=>{
 const reviewed=structuredClone(event);
 const {rpc}=fixture(o=>{
  if(o.method==='eth_chainId'&&o.nth===1)reviewed.data='0x5678';
  if(o.method==='eth_getTransactionReceipt'){o.value.logs[0].data='0x5678';return o.value;}
 });
 await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event:reviewed}),/reviewed event/);
});
test('shared receipt cache cannot erase evidence of a changed event',async()=>{
 let shared;
 const {rpc}=fixture(o=>{
  if(o.method==='eth_getTransactionReceipt'){
   if(o.nth===1)shared=o.value;
   else shared.logs[0].data='0x5678';
   return shared;
  }
 });
 await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event}),/receipt changed/);
});
test('shared header cache cannot rewrite the saved finalized checkpoint',async()=>{
 const shared={number:'0xb',hash:final};
 const {rpc}=fixture(o=>{
  if(o.method==='eth_getBlockByNumber'&&o.params[0]!=='0xa')return shared;
  if(o.method==='eth_getCode')shared.number='0x9';
 });
 await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event}),/checkpoint changed/);
});

// New dedicated regression: the old observer accepts all three inconsistencies.
test('advanced finalized responses stay fixed across the final canonical-header await',async t=>{
 for(const attack of ['advanced-header','second-receipt','parallel-fulfillment'])await t.test(`advanced finalized: ${attack}`,async()=>{
  let advanced,second,canonicalReads=0,deferredMutations=0;
  const {rpc,calls}=fixture(o=>{
   if(o.method==='eth_getBlockByNumber'&&o.params[0]==='finalized'&&o.nth===2){
    advanced={number:'0xc',hash:attack==='parallel-fulfillment'?block:final};return advanced;
   }
   if(o.method==='eth_getTransactionReceipt'&&o.nth===2){
    second=o.value;
    if(attack==='second-receipt')second.logs[0].data='0x5678';
    return second;
   }
   if(o.method==='eth_chainId'&&o.nth===2&&attack==='parallel-fulfillment'){
    // This response remains pending after finalized has fulfilled. A later cache
    // refresh must not erase that already-received noncanonical finalized hash.
    return new Promise(resolve=>setImmediate(()=>{++deferredMutations;advanced.hash=final;resolve('0x2105');}));
   }
   if(o.method==='eth_getBlockByNumber'&&o.params[0]==='0xc'){
    ++canonicalReads;
    if(attack==='advanced-header'){
     // Cache overwrites the earlier finalized response to match a different hash.
     advanced.hash=block;return {number:'0xc',hash:block};
    }
    if(attack==='second-receipt'){
     // Cache erases an event discrepancy present in the second receipt.
     second.logs[0].data='0x1234';
    }
    return {number:'0xc',hash:final};
   }
  });
  await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event}),
   attack==='second-receipt'?/Source receipt changed during observation/:/Advanced finalized source checkpoint is not canonical/);
  assert.equal(canonicalReads,1);
  assert.equal(deferredMutations,attack==='parallel-fulfillment'?1:0);
  assert.equal(calls.filter(c=>c.method==='eth_getTransactionReceipt').length,2);
  assert.ok(calls.every(c=>!/^eth_(send|sign)/.test(c.method)));
 });
});


// Each old final-fence Promise is fulfilled before the next synchronous cache
// refresh, but its .then projection has not run yet. Old code accepts all three.
test('synchronous RPC cache collisions cannot erase already fulfilled divergences',async t=>{
 for(const attack of ['canonical-header','finalized-header','second-receipt'])await t.test(`synchronous collision: ${attack}`,async()=>{
  let shared,collisions=0,active=0,peak=0;
  const {rpc,calls}=fixture(o=>{
   if(attack==='canonical-header'){
    if(o.method==='eth_getBlockByNumber'&&o.params[0]==='0xa'&&o.nth===2){shared={number:'0xa',hash:final};return shared;}
    if(o.method==='eth_getTransactionReceipt'&&o.nth===2){shared.hash=block;++collisions;}
   }
   if(attack==='finalized-header'){
    if(o.method==='eth_getBlockByNumber'&&o.params[0]==='0xb'){shared={number:'0xb',hash:block};return shared;}
    if(o.method==='eth_getBlockByNumber'&&o.params[0]==='finalized'&&o.nth===2){shared.hash=final;++collisions;}
   }
   if(attack==='second-receipt'){
    if(o.method==='eth_getTransactionReceipt'&&o.nth===2){shared=o.value;shared.logs[0].data='0x5678';return shared;}
    if(o.method==='eth_chainId'&&o.nth===2){shared.logs[0].data='0x1234';++collisions;}
   }
  });
  const request=rpc.request;rpc.request=async value=>{++active;peak=Math.max(peak,active);try{return await request(value);}finally{--active;}};
  const expected=attack==='canonical-header'?/Canonical source block changed during observation/:attack==='finalized-header'?/Finalized source checkpoint changed during observation/:/Source receipt changed during observation/;
  await assert.rejects(observeBaseFinalizedReceipt(rpc,{transactionHash:tx,event}),expected);
  assert.equal(collisions,1,'The later request must really mutate the original response');assert.equal(active,0);assert.equal(peak,1,'No observer RPC may overlap the previous response projection');assert.equal(calls.length,10,'The same ten requests are retained without retries');
  assert.deepEqual(calls.map(c=>c.method),['eth_chainId','eth_getBlockByNumber','eth_getTransactionReceipt','eth_getBlockByNumber','eth_getCode','eth_getBlockByNumber','eth_getBlockByNumber','eth_getBlockByNumber','eth_getTransactionReceipt','eth_chainId']);
  assert.ok(calls.every(c=>!/^eth_(send|sign)/.test(c.method)));assert.ok(calls.every(c=>!JSON.stringify(c).includes('"latest"')&&!JSON.stringify(c).includes('"safe"')));
 });
});
