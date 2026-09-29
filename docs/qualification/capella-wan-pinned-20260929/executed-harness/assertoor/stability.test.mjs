import test from 'node:test';
import assert from 'node:assert/strict';
import {observeStableFinality} from './stability.mjs';
function fixture(change = () => {}, crossBoundary = false, unstableHead = false, endpoints = ['http://a','http://b'], settleRecoveryGaps = false, pinnedStateRoots = false) {
  let time = 0, headReads = 0;
  const records = [];
  const request = async (url, options) => {
    const endpoint = endpoints.findIndex(endpoint => url === endpoint || url.startsWith(endpoint + '/'));
    const state = {epoch:2 + Math.floor(time/4000), slot:16 + Math.floor(time/1000), root:null, hash:'0x1234'};
    change(state, endpoint, time);
    if(state.pruned && url.includes('/states/0x'))return {ok:false,status:404,json:async()=>({code:404})};
    if(crossBoundary && time === 0) state.epoch = 0;
    state.root ??= '0x'+String(state.epoch).padStart(64,'0');
    let body;
    if (options.method === 'POST') body = {result:{hash:state.hash}};
    else if (url.endsWith('/spec')) body = {data:{SLOTS_PER_EPOCH:'4',SECONDS_PER_SLOT:'1'}};
    else if (url.endsWith('/finality_checkpoints')) body = {data:{finalized:{epoch:String(state.epoch),root:state.root}}};
    else if (url.endsWith('/headers/head')) body = {data:{root:'0x'+String(unstableHead ? ++headReads : state.slot).padStart(64,'0'),header:{message:{slot:String(state.slot),state_root:state.stateRoot??('0x'+String(state.slot+100).padStart(64,'0'))}}}};
    else body = {data:{message:{body:{execution_payload:{block_hash:'0x1234',block_number:'16'}}}}};
    if(crossBoundary && time === 0 && url.endsWith('/finality_checkpoints')) time=4000;
    return {ok:true,json:async()=>body};
  };
  return {records,run:()=>observeStableFinality(endpoints,4,async r=>records.push(r),{request,settleRecoveryGaps,pinnedStateRoots,clock:()=>time,wait:async ms=>{time+=ms;}})};
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

const three = ['http://a','http://b','http://c'];
test('all three nodes must advance together',async()=>{
 const f=fixture(()=>{},false,false,three);assert.equal((await f.run()).epochs,4);
 assert.ok(f.records.filter(r=>r.samples).every(r=>r.samples.length===3));
});
test('a lagging third node cannot be ignored',async()=>{
 await assert.rejects(fixture((s,i)=>{if(i===2)s.epoch=2;},false,false,three).run(),/stalled for two epochs|Finality lag/);
});
test('a third node execution mismatch fails',async()=>{
 await assert.rejects(fixture((s,i)=>{if(i===2)s.hash='0xbad';},false,false,three).run(),/execution block unavailable/);
});
test('duplicate endpoints do not represent distinct nodes',async()=>{
 await assert.rejects(fixture(()=>{},false,false,['http://a','http://a']).run(),/distinct endpoints/);
});


test('recovery gap resets consecutive credit without rejecting valid FFG progress',async()=>{
 const f=fixture((s,i,t)=>{if(t>=4000)s.epoch++;},false,false,three,true);
 const r=await f.run();assert.equal(r.firstEpoch,'4');assert.equal(r.lastEpoch,'8');assert.equal(r.epochs,4);
 assert.equal(f.records.filter(r=>r.event==='recovery-gap-window-reset').length,1);
 assert.equal(f.records.filter(r=>r.advances>0).length,4);
});
test('repeated recovery gaps never extend the absolute deadline',async()=>{
 const f=fixture((s,i,t)=>{s.epoch=2+2*Math.floor(t/4000);},false,false,three,true);
 await assert.rejects(f.run(),/window deadline/);
 assert.equal(f.records.filter(r=>r.event==='recovery-gap-window-reset').length,6);
});
test('a gap discards advances earned before it',async()=>{
 const f=fixture((s,i,t)=>{if(t>=8000)s.epoch++;},false,false,three,true);
 const r=await f.run();assert.equal(r.firstEpoch,'5');assert.equal(r.lastEpoch,'9');
 assert.equal(f.records.find(r=>r.event==='recovery-gap-window-reset').discardedAdvances,1);
});
test('execution mismatch on the jump is still fatal',async()=>{
 const f=fixture((s,i,t)=>{if(t>=4000){s.epoch++;if(i)s.hash='0xbad';}},false,false,three,true);
 await assert.rejects(f.run(),/execution block unavailable/);
});
test('safety history survives recovery window reset',async()=>{
 const f=fixture((s,i,t)=>{if(t>=4000)s.epoch++;if(t>=5000&&i===2)s.epoch=2;},false,false,three,true);
 await assert.rejects(f.run(),/regressed/);
 assert.equal(f.records.filter(r=>r.event==='recovery-gap-window-reset').length,1);
});
test('conflict after a recovery gap remains fatal',async()=>{
 const f=fixture((s,i,t)=>{if(t>=4000)s.epoch++;if(t>=5000&&i===2)s.root='0xbad';},false,false,three,true);
 await assert.rejects(f.run(),/Conflicting finalized roots/);
});


test('immutable state selector survives continuous head changes',async()=>{
 const f=fixture(()=>{},false,true,three,true,true),r=await f.run();
 assert.equal(r.epochs,4);assert.ok(f.records.some(r=>r.event==='finality-state-bound'));
 assert.ok(!f.records.some(r=>r.event==='head-changed-during-finality-read'));
});
test('pinned reads reject malformed state roots before requesting finality',async()=>{
 await assert.rejects(fixture(s=>{s.stateRoot='0x12';},false,false,three,true,true).run(),/invalid Beacon state root/);
});
test('pinned reads still reject finalized root conflicts',async()=>{
 await assert.rejects(fixture((s,i)=>{if(i)s.root='0xbad';},false,true,three,true,true).run(),/Conflicting finalized roots/);
});
test('pinned reads still reject finalized regression',async()=>{
 await assert.rejects(fixture((s,i,t)=>{if(i&&t>=5000)s.epoch=1;},false,true,three,true,true).run(),/regressed/);
});

test('unknown or pruned pinned state fails without falling back to head',async()=>{
 await assert.rejects(fixture(s=>{s.pruned=true;},false,true,three,true,true).run(),/REST 404/);
});
