import {test} from 'node:test';
import assert from 'node:assert/strict';
import {observeRecovery} from './recovery-observation.mjs';
import {PartitionRecoveryDeadline} from './partition.mjs';
test('normal recovery completes its observation',async()=>{
 const calls=[];await observeRecovery({partition:async()=>calls.push('partition'),observe:async()=>calls.push('observe'),record:async e=>calls.push(e.event)});
 assert.deepEqual(calls,['partition','observe']);
});
test('late stable finality preserves original deadline failure',async()=>{
 const calls=[],error=new PartitionRecoveryDeadline(async()=>calls.push('recovered'));
 await assert.rejects(observeRecovery({partition:async()=>{throw error;},observe:async()=>calls.push('observe'),record:async e=>calls.push(e.event)}),e=>e===error);
 assert.deepEqual(calls,['partition-recovery-deadline-missed','recovered','observe','late-finality-passed']);
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
 await assert.rejects(observeRecovery({partition:async()=>{throw new PartitionRecoveryDeadline(async()=>{});},observe:async()=>{throw error;},record:async e=>events.push(e.event)}),e=>e===error);
 assert.deepEqual(events,['partition-recovery-deadline-missed','late-finality-failed']);
});

test('late recovery must complete before stability observation',async()=>{
 let observed=false;const failure=Error('Late recovery observation deadline exceeded');
 const original=new PartitionRecoveryDeadline(async()=>{throw failure;});
 await assert.rejects(observeRecovery({partition:async()=>{throw original;},observe:async()=>{observed=true;},record:async()=>{}}),e=>e===failure);
 assert.equal(observed,false);
});
test('missing late recovery observer cannot skip the recovery stage',async()=>{
 let observed=false;
 await assert.rejects(observeRecovery({partition:async()=>{throw new PartitionRecoveryDeadline();},observe:async()=>{observed=true;},record:async()=>{}}),/unavailable/);
 assert.equal(observed,false);
});
test('deadline event retains original monotonic evidence when late stability succeeds',async()=>{
 const evidence={window:'partition-recovery-bound',recoveryElapsedMs:180127.25,recoveryLimitMs:180000,deadlineExceededByMs:127.25,lastRead:{completeReadElapsedMs:1706.5,advancedCommonFinality:true}};
 const events=[],original=new PartitionRecoveryDeadline(async()=>{},evidence);
 await assert.rejects(observeRecovery({partition:async()=>{throw original;},observe:async()=>{},record:async e=>events.push(e)}),e=>e===original);
 assert.deepEqual(events[0].recoveryObservation,evidence);
 assert.equal(events.at(-1).deadlineStillFailed,true);
});
