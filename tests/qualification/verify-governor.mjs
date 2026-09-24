import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const reports=process.argv.slice(2).flatMap(file=>readFileSync(file,'utf8').split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='hardhat-governor-lifecycle'));
assert.equal(reports.length,2);assert.deepEqual(reports.map(r=>r.fork).sort(),['Paris','Shanghai']);
const failureLabels=['below-proposal-threshold','duplicate-vote','queue-without-quorum','execute-before-queue','direct-timelock-bypass','replayed-proposal-execution'];
const successLabels=['renounce-admin','delegate-owner','propose','propose-no-quorum','cast-owner-vote','transfer-after-snapshot','delegate-recipient','cast-zero-historical-weight','queue','execute'];
let successes=0;
for(const r of reports){
 assert.equal(r.openzeppelin,'5.0.2');assert.notEqual(r.proposalId,r.defeatedProposalId);
 assert.deepEqual(r.historicalVotes,['1000000','0']);assert.deepEqual(r.currentVotes,['250000','750000']);assert.deepEqual(r.countedVotes,['0','1000000','0']);assert.equal(r.quorum,'500000');
 assert.equal(r.successfulState,7);assert.equal(r.defeatedState,3);assert.equal(r.finalTargetValue,'73');
 for(const key of ['adminRenounced','governorOnlyTimelockRoles','earlyVoteAndExecutionCallsRejected','defaultGasEstimation'])assert.equal(r[key],true,key);
 assert.deepEqual(r.failedReceipts.map(v=>v.label).sort(),failureLabels.slice().sort());
 const hashes=new Set();
 for(const tx of r.failedReceipts){assert.equal(tx.status,'0x0');assert.equal(tx.logs,0);assert(BigInt(tx.gasUsed)>0n&&BigInt(tx.gasUsed)<BigInt(tx.gasLimit));assert(!hashes.has(tx.hash));hashes.add(tx.hash);}
 for(const label of successLabels)assert.equal(r.receipts.filter(tx=>tx.label===label).length,1,label);
 for(const label of ['grant-governor-role','revoke-owner-role'])assert.equal(r.receipts.filter(tx=>tx.label===label).length,3);
 for(const tx of r.receipts){assert(BigInt(tx.gasUsed)>0n&&BigInt(tx.gasUsed)<=BigInt(tx.gasLimit));assert(!hashes.has(tx.hash));hashes.add(tx.hash);}
 const block=label=>r.receipts.find(tx=>tx.label===label).blockNumber;
 assert(BigInt(block('cast-owner-vote'))>BigInt(r.snapshot));assert(block('transfer-after-snapshot')>block('cast-owner-vote'));assert(block('execute')>block('queue'));successes+=r.receipts.length;
}
console.log(JSON.stringify({profiles:2,successfulProposals:2,defeatedProposals:2,expectedFailedReceipts:12,successfulReceipts:successes,exactLifecycleCoverage:true,scope:'Checks persisted test summaries and receipt identities; on-chain assertions are in governor.ts. Not an independent EVM replay.'}));
