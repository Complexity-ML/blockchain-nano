import {test} from 'node:test';
import assert from 'node:assert/strict';
import {observeRecovery} from './recovery-observation.mjs';
import {PartitionRecoveryDeadline} from './partition.mjs';
test('normal recovery completes its observation',async()=>{
 const calls=[];await observeRecovery({partition:async()=>calls.push('partition'),observe:async()=>calls.push('observe'),record:async e=>calls.push(e.event)});
 assert.deepEqual(calls,['partition','observe']);
});
test('late stable finality preserves original deadline failure',async()=>{
 const error=new PartitionRecoveryDeadline(),calls=[];
 await assert.rejects(observeRecovery({partition:async()=>{throw error;},observe:async()=>calls.push('observe'),record:async e=>calls.push(e.event)}),e=>e===error);
 assert.deepEqual(calls,['partition-recovery-deadline-missed','observe','late-finality-passed']);
});
test('restoration, RPC and safety errors never extend the campaign',async()=>{
 for(const message of ['Partition restoration failed','RPC unavailable','Conflicting finalized roots across partition','Partition recovery deadline exceeded']){
  const error=Error(message);let observed=false;
  await assert.rejects(observeRecovery({partition:async()=>{throw error;},observe:async()=>{observed=true;},record:async()=>{}}),e=>e===error);
  assert.equal(observed,false);
 }
});
test('late observer failure remains a failure and retains both diagnostics',async()=>{
 const error=Error('No sustained finality'),events=[];
 await assert.rejects(observeRecovery({partition:async()=>{throw new PartitionRecoveryDeadline();},observe:async()=>{throw error;},record:async e=>events.push(e.event)}),e=>e===error);
 assert.deepEqual(events,['partition-recovery-deadline-missed','late-finality-failed']);
});
