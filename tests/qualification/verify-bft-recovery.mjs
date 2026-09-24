import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const [campaign]=process.argv.slice(2);assert(campaign);
const read=p=>readFileSync(resolve(campaign,p),'utf8');
const executions=JSON.parse(read('resumed/results.json'));
const expected=new Set();for(const fork of ['paris','shanghai'])for(const scenario of ['restart','load_crash','stake_change_crash','two_thirds'])expected.add(`bft_${scenario}_${fork}_network`);
const initial='bft_restart_paris_network';expected.delete(initial);
assert.equal(executions.length,7);
for(const run of executions){assert(expected.delete(run.name),'unexpected or duplicate execution');assert.equal(run.code,0);assert.equal(run.signal,null);}
assert.equal(expected.size,0);
const runs=[{name:initial,file:'interrupted-ctest-full.log'},...executions.map(e=>({name:e.name,file:'resumed/'+e.name+'-full.log'}))];
const results=[];
for(const run of runs){
 const text=read(run.file);assert(text.includes('Testing: '+run.name));assert(text.includes('Test Passed.'));
 const fork=run.name.includes('_paris_')?'Paris':'Shanghai';
 const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
 let row,kind;
 if(run.name.includes('_load_crash_')){
  kind='load-crash';const finals=rows.filter(r=>r.scenario===kind);assert.equal(finals.length,1);row=finals[0];
  assert.equal(row.transactions,360);assert.equal(row.workers,4);assert.equal(row.reopenedPeers,4);assert(row.confirmationsObservedDuringSend>0);assert(row.certifiedTransactionBlocks>0);
  assert(BigInt(row.crash.offlineBlockNumber)>BigInt(row.crash.heightBeforeCrash));assert(row.crash.submittedAtRestart<360);assert(row.crash.submittedAtRelayReady<360);
  const offline=rows.filter(r=>r.scenario==='load-crash-offline-quorum');assert.equal(offline.length,1);assert(offline[0].completedSenders<4);assert.equal(offline[0].blockNumber,row.crash.offlineBlockNumber);
 }else if(run.name.includes('_stake_change_crash_')){
  kind='stake-change';const finals=rows.filter(r=>r.scenario===kind);assert.equal(finals.length,1);row=finals[0];
  assert.equal(row.restartMode,'SIGKILL');assert.equal(row.confirmedSigkillExits,4);assert.deepEqual(row.oldWeights,[1,1,1,1]);assert.deepEqual(row.newWeights,[4,1,1,1]);
  assert.equal(row.certificateSigners,2);assert.equal(row.healedPeers,4);assert.equal(row.reopenedPeers,4);assert.equal(row.restartedReactors,4);assert.equal(row.verifiedJournals,4);assert.equal(row.restartRecipientBalance,'0x3');
  assert(BigInt(row.restartBlockNumber)>BigInt(row.finalBlockNumber));assert(BigInt(row.finalBlockNumber)>BigInt(row.activationHeight));assert.equal(row.journalRecovery.length,4);
  assert.deepEqual(row.journalRecovery.map(r=>r.peer).sort(),[0,1,2,3]);
  const set=row.journalRecovery[0].afterSet;assert(/^0x[0-9a-f]{64}$/i.test(set));
  for(const j of row.journalRecovery){assert.equal(BigInt(j.after),BigInt(j.tip)+1n);assert.equal(j.afterSet,set);}
 }else if(run.name.includes('_two_thirds_')){
  kind='two-thirds';const finals=rows.filter(r=>r.scenario===kind);assert.equal(finals.length,1);row=finals[0];
  assert.deepEqual(row.weights,[3,1,1,1]);assert.equal(row.totalWeight,6);assert.equal(row.signedWeight,5);assert.equal(row.certificateSigners,3);assert.equal(row.reconnectedPeers,4);
  assert(text.includes('Initial sub-quorum groups remained without finality'));
 }else{
  kind='restart';const finals=rows.filter(r=>r.restarted===true);assert.equal(finals.length,1);row=finals[0];
  assert.equal(row.partitioned,true);assert.equal(row.validators,4);assert.equal(row.heightVerified,'0x1');
  assert(text.includes('Two isolated halves cannot finalize'));assert.equal(text.split('Restart reproduces exact reserved vote').length-1,4);
 }
 assert.equal(row.fork,fork);results.push({name:run.name,kind,fork,verified:true});
}
console.log(JSON.stringify({verified:true,cases:results.length,results,scope:'Current-binary local recovery cases. Native assertions verify receipts, certificates, state and journals; this verifier checks exact case coverage and recorded outcomes, not independent consensus execution.'},null,2));
