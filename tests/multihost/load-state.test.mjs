import test from 'node:test';
import assert from 'node:assert/strict';
import {collectFinalizedLoadState} from './load-state.mjs';
const hash=n=>'0x'+n.toString(16).padStart(64,'0');
const floor={number:'0x3c',hash:hash(60)},anchor={number:'0x48',hash:hash(72)};
function fixture(overrides={}){const calls=[];return {calls,rpc:async(node,method,args)=>{calls.push({node,method,args});if(overrides.rpc){const x=overrides.rpc(node,method,args);if(x!==undefined)return x;}
 if(method==='eth_getBlockByNumber')return args[0]==='finalized'?anchor:args[0]===floor.number?floor:anchor;
 return method==='eth_getTransactionCount'?'0x61':method==='eth_getBalance'?'0x60':hash(42);}};}
const options=f=>({rpc:f.rpc,receiptAnchor:floor,sender:'sender',target:'target'});
test('states use a fresh common finalized anchor beyond complete receipts',async()=>{
 const f=fixture(),result=await collectFinalizedLoadState(options(f));assert.equal(result.complete,true);assert.deepEqual(result.anchor,anchor);assert.deepEqual(result.receiptAnchor,floor);
 const reads=f.calls.filter(x=>x.method!=='eth_getBlockByNumber');assert.equal(reads.length,9);assert.ok(reads.every(x=>x.args.at(-1)===anchor.number));
});
test('different finalized heights remain pending before state reads',async()=>{
 const f=fixture({rpc:(n,m,a)=>n==='remote'&&m==='eth_getBlockByNumber'&&a[0]==='finalized'?{number:'0x49',hash:hash(73)}:undefined});assert.equal((await collectFinalizedLoadState(options(f))).complete,false);assert.ok(f.calls.every(x=>x.method==='eth_getBlockByNumber'));
});
test('exact pruning availability retries without treating unknown failures as pending',async()=>{
 const f=fixture({rpc:(n,m)=>{if(m==='eth_getBalance')throw Error('{"code":-32000,"message":"State unavailable"}');}});assert.equal((await collectFinalizedLoadState(options(f))).reason,'state-pruned-during-read');
 for(const text of ['{"code":-32603,"message":"Internal error"}','{"code":-32000,"message":"Other error"}','{"code":-32000,"message":"State unavailable","data":"extra"}','broken']){
  const f=fixture({rpc:(n,m)=>{if(m==='eth_getBalance')throw Error(text);}});await assert.rejects(collectFinalizedLoadState(options(f)));
 }
});
for(const [name,rpc] of [
 ['same height conflicting finality',(n,m,a)=>n==='remote'&&m==='eth_getBlockByNumber'&&a[0]==='finalized'?{...anchor,hash:hash(99)}:undefined],
 ['regressing finality',(n,m,a)=>m==='eth_getBlockByNumber'&&a[0]==='finalized'?{number:'0x3b',hash:hash(59)}:undefined],
 ['changed verified receipt anchor',(n,m,a)=>m==='eth_getBlockByNumber'&&a[0]===floor.number?{...floor,hash:hash(99)}:undefined],
 ['changed state anchor',(n,m,a)=>m==='eth_getBlockByNumber'&&a[0]===anchor.number?{...anchor,hash:hash(99)}:undefined],
 ['noncanonical state value',(n,m)=>m==='eth_getBalance'?'0x060':undefined]
])test(name+' fails',async()=>{const f=fixture({rpc});await assert.rejects(collectFinalizedLoadState(options(f)));});

test('different heights cannot hide a changed verified receipt anchor',async()=>{
 const f=fixture({rpc:(n,m,a)=>m==='eth_getBlockByNumber'?(a[0]===floor.number?{...floor,hash:hash(99)}:n==='remote'&&a[0]==='finalized'?{number:'0x49',hash:hash(73)}:undefined):undefined});
 await assert.rejects(collectFinalizedLoadState(options(f)));
});

// Mirror the actual RPC wire format rather than an integer-shaped stub.
test('padded storage words preserve zero, 42 and the full uint256 range',async()=>{
 for(const storage of [hash(0),hash(42),'0x'+'f'.repeat(64)]){
  const f=fixture({rpc:(n,m)=>m==='eth_getStorageAt'?storage:undefined});
  const result=await collectFinalizedLoadState(options(f));assert.equal(result.complete,true);
  for(const state of Object.values(result.states))assert.equal(state.storage,storage);
 }
});
test('short, long, odd and nonhex storage DATA fail instead of passing as quantities',async()=>{
 for(const storage of ['0x2a','0x0','0x','0x'+'0'.repeat(63),'0x'+'0'.repeat(65),'0x'+'0'.repeat(62)+'gg']){
  const f=fixture({rpc:(n,m)=>m==='eth_getStorageAt'?storage:undefined});
  await assert.rejects(collectFinalizedLoadState(options(f)),/Invalid storage word/);
 }
});
test('padded nonce remains an invalid QUANTITY even though storage permits padding',async()=>{
 const f=fixture({rpc:(n,m)=>m==='eth_getTransactionCount'?hash(97):undefined});
 await assert.rejects(collectFinalizedLoadState(options(f)),/Invalid state quantity/);
});
