import {test} from 'node:test';
import assert from 'node:assert/strict';
import {verifyPartitionLoad} from './partition-load-evidence.mjs';
const hash=n=>'0x'+n.toString(16).padStart(64,'0');
function evidence(){
 const event=(name,time,extra={})=>({event:name,time,...extra});
 const transactions=Array.from({length:96},(_,i)=>({hash:hash(i+1)}));
 const baseline=Object.fromEntries(['native','local','remote'].map(n=>[n,{nonce:'0x1',balance:'0x0'}]));
 const states=Object.fromEntries(['native','local','remote'].map(n=>[n,{nonce:'0x61',balance:'0x60',storage:'0x2a'}]));
 const anchor={number:'0x5',hash:hash(999)};
 const observations=Object.fromEntries(['native','local','remote'].map(n=>[n,{anchor,finalized:anchor,blocks:{'0x4':hash(998)},
  receipts:Object.fromEntries(transactions.map((t,i)=>[t.hash,{transactionHash:t.hash,blockNumber:'0x4',blockHash:hash(998),
   transactionIndex:'0x'+i.toString(16),gasUsed:'0x5208',cumulativeGasUsed:'0x'+((i+1)*21000).toString(16),status:'0x1',logs:[],contractAddress:null}]))}]));
 return {
  fixture:{transactions,baseline},
  run:[event('partition-disconnected',1000),event('partition-workload-start',1001),
   event('partition-observation',1002,{rows:[{name:'vps-docker',peers:'0x0'}]}),
   event('partition-workload-finished',1100),event('partition-reconnected',97000),event('partition-recovery-bound',98000),
   event('passed',200000,{validators:['native.log','docker-mac.log','vps.log'].map(file=>({file,healthy:true}))})],
  load:[...transactions.map((t,i)=>event('submitted',1003+i,{hash:t.hash,index:i})),
   event('receipt-observation',99000,{observations}),event('load-passed',100000,{elapsedMs:99000,anchor,states})],
  stability:{firstEpoch:'3',lastEpoch:'6',nodes:['mac-native','mac-docker','vps-docker']}
 };
}
test('complete partition workload evidence passes',()=>assert.equal(verifyPartitionLoad(evidence()).transactions,96));
for(const [name,mutate] of [
 ['submission before cut',e=>e.load[0].time=999],
 ['missing transaction',e=>e.load.splice(0,1)],
 ['wrong submitted hash',e=>e.load[0].hash=hash(555)],
 ['short cut',e=>e.run[4].time=96999],
 ['no isolation',e=>e.run[2].rows[0].peers='0x1'],
 ['retained failure',e=>e.run.push({time:200001,event:'failed'})],
 ['late recovery',e=>{e.run[5].time=300000;e.run[6].time=400000;}],
 ['wrong balance',e=>e.load.at(-1).states.remote.balance='0x61'],
 ['wrong nonce',e=>e.load.at(-1).states.local.nonce='0x62'],
 ['wrong storage',e=>e.load.at(-1).states.native.storage='0x0'],
 ['missing state evidence',e=>delete e.load.at(-1).states.remote],
 ['insufficient stability',e=>e.stability.lastEpoch='5'],
 ['unhealthy validator',e=>e.run.at(-1).validators[0].healthy=false],
 ['duplicate validator',e=>e.run.at(-1).validators[0].file='vps.log'],
])test(name+' is rejected',()=>{const e=evidence();mutate(e);assert.throws(()=>verifyPartitionLoad(e));});
