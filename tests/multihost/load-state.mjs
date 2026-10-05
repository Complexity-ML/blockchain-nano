import assert from 'node:assert/strict';
import {loadNodes} from './load-receipts.mjs';
const quantity=x=>{assert.match(x??'',/^0x(?:0|[1-9a-f][0-9a-f]*)$/,'Invalid state quantity');return BigInt(x);};
const block=b=>{quantity(b?.number);assert.match(b?.hash??'',/^0x[0-9a-f]{64}$/,'Invalid state anchor');return {number:b.number,hash:b.hash};};
function unavailable(error){
 try{const x=JSON.parse(error.message);return x?.code===-32000&&x.message==='State unavailable'&&Object.keys(x).length===2;}catch{return false;}
}
// Receipt collection can span epochs. Never assume its old anchor still has
// materialized state on a pruned node. Only exact state-unavailable is retryable.
export async function collectFinalizedLoadState({rpc,receiptAnchor,sender,target}){
 const floor=block(receiptAnchor);
 const heads=await Promise.all(loadNodes.map(async node=>block(await rpc(node,'eth_getBlockByNumber',['finalized',false]))));
 for(const h of heads)assert.ok(quantity(h.number)>=quantity(floor.number),'Finality regressed below verified receipts');
 for(const node of loadNodes)assert.deepEqual(block(await rpc(node,'eth_getBlockByNumber',[floor.number,false])),floor,'Verified receipt anchor changed');
 if(heads.some(h=>h.number!==heads[0].number))return {complete:false,reason:'finalized-heights-differ',heads};
 for(const h of heads)assert.deepEqual(h,heads[0],'Conflicting finalized state anchors');
 const anchor=heads[0],states={};
 for(const node of loadNodes){
  try{
   states[node]={nonce:await rpc(node,'eth_getTransactionCount',[sender,anchor.number]),balance:await rpc(node,'eth_getBalance',[target,anchor.number]),storage:await rpc(node,'eth_getStorageAt',[target,'0x0',anchor.number])};
  }catch(error){if(!unavailable(error))throw error;return {complete:false,reason:'state-pruned-during-read',node,anchor};}
  quantity(states[node].nonce);quantity(states[node].balance);
  // eth_getStorageAt returns DATA (one padded 32-byte word), not QUANTITY.
  assert.match(states[node].storage??'',/^0x[0-9a-f]{64}$/,'Invalid storage word');
  assert.deepEqual(block(await rpc(node,'eth_getBlockByNumber',[anchor.number,false])),anchor,'State anchor changed during read');
  const finalized=block(await rpc(node,'eth_getBlockByNumber',['finalized',false]));
  assert.ok(quantity(finalized.number)>=quantity(anchor.number),'State anchor no longer finalized');
  if(finalized.number===anchor.number)assert.deepEqual(finalized,anchor,'Finalized state anchor changed');
 }
 return {complete:true,anchor,receiptAnchor:floor,states};
}
