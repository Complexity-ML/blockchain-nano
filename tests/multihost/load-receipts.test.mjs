import {test} from 'node:test';
import assert from 'node:assert/strict';
import {verifyLoadReceipts, collectLoadReceipts, loadNodes} from './load-receipts.mjs';
const tx = '0x'+'a'.repeat(64), root = '0x'+'b'.repeat(64);
const other = '0x'+'c'.repeat(64);
const receipt = {transactionHash:tx, status:'0x1', blockNumber:'0x2', blockHash:root,
  transactionIndex:'0x0', gasUsed:'0x5208', cumulativeGasUsed:'0x5208', logs:[], contractAddress:null};
function fixture() {
  return Object.fromEntries(loadNodes.map(node => [node, {
    finalized:{number:'0x3',hash:other}, anchor:{number:'0x3',hash:other},
    receipts:{[tx]:structuredClone(receipt)}, blocks:{'0x2':root}}]));
}
test('all three nodes must finalize the identical canonical successful receipt', () => {
  assert.deepEqual(verifyLoadReceipts([tx],fixture()), {
    complete:true, transactions:1, anchor:{number:'0x3',hash:other}});
});
for (const [name, mutate, error] of [
  ['failed execution', o=>o.remote.receipts[tx].status='0x0', /Transaction failed/],
  ['wrong transaction', o=>o.remote.receipts[tx].transactionHash=other, /Wrong transaction/],
  ['orphaned receipt', o=>o.remote.blocks['0x2']=other, /Noncanonical/],
  ['divergent gas', o=>o.remote.receipts[tx].gasUsed='0x5209', /Divergent/],
  ['divergent logs', o=>o.remote.receipts[tx].logs=[{data:'0x01'}], /Divergent/],
  ['conflicting finality', o=>o.remote.anchor.hash=root, /Conflicting/],
  ['unfinalized anchor', o=>o.remote.finalized.number='0x2', /Anchor not finalized/],
  ['missing node', o=>delete o.remote, /Three nodes/],
  ['missing receipt key', o=>delete o.remote.receipts[tx], /Receipt set/],
  ['malformed gas', o=>o.remote.receipts[tx].gasUsed='0x00', /Gas used/],
]) test(name+' cannot pass', () => {
  const o=fixture(); mutate(o); assert.throws(()=>verifyLoadReceipts([tx],o),error);
});
test('missing receipt and unfinalized receipt remain pending', () => {
  const o=fixture();o.remote.receipts[tx]=null;
  o.local.receipts[tx].blockNumber='0x4';
  assert.deepEqual(verifyLoadReceipts([tx],o), {complete:false,pending:[
    {node:'local',tx,reason:'not-finalized'}, {node:'remote',tx,reason:'missing'}]});
});
test('empty and duplicate workloads cannot pass', () => {
  assert.throws(()=>verifyLoadReceipts([],fixture()),/Empty/);
  assert.throws(()=>verifyLoadReceipts([tx,tx],fixture()),/Duplicate/);
});
function fakeRpc(changeAnchor=false) {
  const anchorReads={};
  return async(node, method, [tag]) => {
    if(method==='eth_getTransactionReceipt')return structuredClone(receipt);
    assert.equal(method,'eth_getBlockByNumber');
    if(tag==='finalized')return {number:node==='remote'?'0x3':'0x4',hash:other};
    if(tag==='0x3') {
      anchorReads[node]=(anchorReads[node]??0)+1;
      return {number:tag,hash:changeAnchor&&anchorReads[node]>1?root:other};
    }
    assert.equal(tag,'0x2');return {number:tag,hash:root};
  };
}
test('collector uses the lowest finalized height and checks canonical blocks',async()=>{
  const r=await collectLoadReceipts([tx],fakeRpc());
  assert.equal(r.verdict.complete,true);assert.equal(r.verdict.anchor.number,'0x3');
});
test('collector rejects a finalized anchor changing during observation',async()=>{
  await assert.rejects(collectLoadReceipts([tx],fakeRpc(true)),/Finalized anchor changed/);
});
