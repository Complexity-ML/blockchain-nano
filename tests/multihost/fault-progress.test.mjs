import {test} from 'node:test';
import assert from 'node:assert/strict';
import {captureFaultProgress} from './fault-progress.mjs';
const root=x=>'0x'+x.repeat(64);
const header={data:{root:root('a'),header:{message:{slot:'88',state_root:root('b')}}}};
const finality={data:{current_justified:{epoch:'10',root:root('c')},finalized:{epoch:'9',root:root('d')}}};
test('bind finality to the selected state despite a subsequent head change',async()=>{
 const paths=[];let time=100;
 const r=await captureFaultProgress(async p=>{paths.push(p);return paths.length===1?structuredClone(header):finality;},()=>time++);
 assert.deepEqual(paths,['/eth/v1/beacon/headers/head','/eth/v1/beacon/states/'+root('b')+'/finality_checkpoints']);
 assert.equal(r.headSlot,'88');assert.equal(r.finalized.epoch,'9');assert.equal(r.justified.epoch,'10');assert.equal(r.afterMs-r.beforeMs,1);
});
test('invalid head/state identity or noncanonical slot prevents a diagnostic claim',async()=>{
 for(const change of [h=>h.data.root='bad',h=>h.data.header.message.state_root='bad',h=>h.data.header.message.slot='088']){
  const h=structuredClone(header);change(h);await assert.rejects(captureFaultProgress(async()=>h),/header/);
 }
});
test('missing, malformed or pruned checkpoint is not silently ignored',async()=>{
 for(const cp of [{data:{}},{data:{...finality.data,finalized:{epoch:'-1',root:root('d')}}},null]){
  let n=0;await assert.rejects(captureFaultProgress(async()=>++n===1?header:cp));
 }
 let n=0;await assert.rejects(captureFaultProgress(async()=>{if(++n===1)return header;throw Error('state pruned');}),/pruned/);
});
test('failed reads and a backward sampling clock remain fatal',async()=>{
 await assert.rejects(captureFaultProgress(async()=>{throw Error('transport failed');}),/transport/);
 let n=0,t=10;await assert.rejects(captureFaultProgress(async()=>++n===1?header:finality,()=>t--),/clock/);
});
