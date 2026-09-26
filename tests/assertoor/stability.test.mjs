import test from 'node:test';
import assert from 'node:assert/strict';
import {observeStableFinality} from './stability.mjs';
function fixture(change = () => {}, crossBoundary = false, unstableHead = false) {
  let time = 0, headReads = 0;
  const records = [];
  const request = async (url, options) => {
    const endpoint = url.startsWith('http://b') ? 1 : 0;
    const state = {epoch:2 + Math.floor(time/4000), slot:16 + Math.floor(time/1000), root:null, hash:'0x1234'};
    change(state, endpoint, time);
    if(crossBoundary && time === 0) state.epoch = 0;
    state.root ??= '0x'+String(state.epoch).padStart(64,'0');
    let body;
    if (options.method === 'POST') body = {result:{hash:state.hash}};
    else if (url.endsWith('/spec')) body = {data:{SLOTS_PER_EPOCH:'4',SECONDS_PER_SLOT:'1'}};
    else if (url.endsWith('/finality_checkpoints')) body = {data:{finalized:{epoch:String(state.epoch),root:state.root}}};
    else if (url.endsWith('/headers/head')) body = {data:{root:'0x'+String(unstableHead ? ++headReads : state.slot).padStart(64,'0'),header:{message:{slot:String(state.slot)}}}};
    else body = {data:{message:{body:{execution_payload:{block_hash:'0x1234',block_number:'16'}}}}};
    if(crossBoundary && time === 0 && url.endsWith('/finality_checkpoints')) time=4000;
    return {ok:true,json:async()=>body};
  };
  return {records,run:()=>observeStableFinality(['http://a','http://b'],4,async r=>records.push(r),{request,clock:()=>time,wait:async ms=>{time+=ms;}})};
}
test('four consecutive common checkpoints bind to execution',async()=>{
  const f=fixture(),result=await f.run();assert.equal(result.epochs,4);assert.equal(result.firstEpoch,'2');assert.equal(result.lastEpoch,'6');assert.equal(f.records.filter(r=>r.advances).length,4);
});
test('conflicting finalized roots cannot be retried away',async()=>{
  await assert.rejects(fixture((s,i,t)=>{if(i&&t>=1000)s.root='0xbad';}).run(),/Conflicting finalized roots/);
});
test('a previous success cannot hide stopped finality',async()=>{
  await assert.rejects(fixture(s=>{s.epoch=2;}).run(),/stalled for two epochs|Finality lag/);
});
test('finalized epoch regression fails',async()=>{
  await assert.rejects(fixture((s,i,t)=>{if(i&&t>=5000)s.epoch=1;}).run(),/regressed/);
});
test('execution mismatch fails despite common Beacon finality',async()=>{
  await assert.rejects(fixture((s,i)=>{if(i)s.hash='0xbad';}).run(),/execution block unavailable/);
});
test('missed checkpoints do not count as consecutive advances',async()=>{
  await assert.rejects(fixture((s,i,t)=>{if(t>=4000)s.epoch++;}).run(),/Skipped common/);
});

test('an epoch transition between checkpoint and head reads is sampled consistently',async()=>{
  const f=fixture(()=>{},true),result=await f.run();
  assert.equal(result.epochs,4);assert.equal(result.firstEpoch,'3');
});

test('head churn cannot cause unbounded sampling retries',async()=>{
  await assert.rejects(fixture(()=>{},false,true).run(),/Consistent finality sample unavailable/);
});
test('a finality conflict remains fatal during a head change',async()=>{
  await assert.rejects(fixture((s,i)=>{if(i)s.root='0xbad';},false,true).run(),/Conflicting finalized roots/);
});
