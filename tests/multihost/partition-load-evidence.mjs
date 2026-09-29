import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {resolve,join} from 'node:path';
import {pathToFileURL} from 'node:url';
import {verifyLoadReceipts,loadNodes} from './load-receipts.mjs';

export function verifyPartitionLoad({run,load,fixture,stability}) {
 const one=(rows,event)=>{const selected=rows.filter(r=>r.event===event);assert.equal(selected.length,1,'Expected one '+event);return selected[0];};
 for(const rows of [run,load]) {
  assert.ok(rows.every(r=>Number.isSafeInteger(r.time)),'Missing timestamps');
  assert.ok(rows.every((r,i)=>i===0||r.time>=rows[i-1].time),'Regressing timestamps');
  assert.ok(!rows.some(r=>/failed|error|deadline/.test(r.event)&&r.event!=='warming-error'),'Failure recorded');
 }
 const cut=one(run,'partition-disconnected'),start=one(run,'partition-workload-start');
 const end=one(run,'partition-workload-finished'),reconnect=one(run,'partition-reconnected');
 const recovery=one(run,'partition-recovery-bound'),passed=one(run,'passed');
 assert.ok(cut.time<=start.time&&start.time<=end.time&&end.time<=reconnect.time,'Workload outside partition');
 assert.ok(reconnect.time-cut.time>=96000,'Partition too short');
 assert.ok(recovery.time>=reconnect.time&&recovery.time-reconnect.time<=180000,'Recovery deadline exceeded');
 assert.ok(passed.time>=recovery.time,'Passed before recovery');
 assert.equal(fixture.transactions.length,96,'Expected 96 transactions');
 const hashes=fixture.transactions.map(t=>t.hash);
 const submissions=load.filter(r=>r.event==='submitted');
 assert.equal(submissions.length,96,'Missing submissions');
 assert.deepEqual(submissions.map(r=>r.index),Array.from({length:96},(_,i)=>i),'Submission order mismatch');
 assert.deepEqual(submissions.map(r=>r.hash),hashes,'Submitted hashes differ');
 assert.ok(submissions.every(r=>r.time>=start.time&&r.time<=end.time),'Submission outside effective cut');
 const isolated=run.filter(r=>r.event==='partition-observation'&&r.time>=cut.time&&r.time<reconnect.time);
 assert.ok(isolated.some(r=>r.rows.some(n=>n.name==='vps-docker'&&BigInt(n.peers)===0n)),'No peer isolation observed');
 const final=one(load,'load-passed');
 assert.ok(final.time>=recovery.time&&final.time<=passed.time,'Invalid workload completion order');
 assert.ok(final.elapsedMs>0&&final.elapsedMs<600000,'Workload deadline exceeded');
 const observation=load.filter(r=>r.event==='receipt-observation').at(-1);
 assert.ok(observation&&observation.time<=final.time,'Missing final receipts');
 const verdict=verifyLoadReceipts(hashes,observation.observations);
 assert.equal(verdict.complete,true,'Receipts not finalized');
 assert.deepEqual(final.anchor,verdict.anchor,'Final state anchor mismatch');
 for(const node of loadNodes) {
  assert.deepEqual(fixture.baseline[node],fixture.baseline.native,'Baseline mismatch');
  assert.equal(BigInt(final.states[node].nonce),BigInt(fixture.baseline.native.nonce)+96n,'Final nonce mismatch');
  assert.equal(BigInt(final.states[node].balance),BigInt(fixture.baseline.native.balance)+96n,'Final balance mismatch');
  assert.equal(BigInt(final.states[node].storage),42n,'Final storage mismatch');
 }
 assert.ok(BigInt(stability.lastEpoch)-BigInt(stability.firstEpoch)>=3n,'Insufficient post-load finality');
 assert.deepEqual([...stability.nodes].sort(),['mac-docker','mac-native','vps-docker'],'Stability nodes mismatch');
 assert.equal(passed.validators.length,3,'Missing validator verdicts');
 assert.deepEqual(passed.validators.map(v=>v.file).sort(),['docker-mac.log','native.log','vps.log'],'Validator identities mismatch');
 assert.ok(passed.validators.every(v=>v.healthy===true),'Unhealthy validator');
 return {transactions:96,partitionMs:reconnect.time-cut.time,recoveryMs:recovery.time-reconnect.time,
  workloadMs:final.elapsedMs,anchor:final.anchor,states:final.states,stability};
}

if(process.argv[1]&&import.meta.url===pathToFileURL(resolve(process.argv[1])).href) {
 const directory=process.argv[2];assert.ok(directory,'Campaign directory required');
 const json=async name=>JSON.parse(await readFile(join(directory,name),'utf8'));
 const lines=async name=>(await readFile(join(directory,name),'utf8')).trim().split('\n').map(JSON.parse);
 console.log(JSON.stringify(verifyPartitionLoad({run:await lines('run.jsonl'),load:await lines('load.jsonl'),
  fixture:await json('load-transactions.json'),stability:await json('observations/result.json')}),null,2));
}
