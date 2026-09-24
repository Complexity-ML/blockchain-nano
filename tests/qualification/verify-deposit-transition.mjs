import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,11);
for(const name of ['ethereum_official_deposit_transition','consensus_slashing_transition','consensus_slashing_recovery','consensus_slashing_epoch','ethereum_official_slashings','ethereum_official_proposer_slashing','ethereum_official_block_proposer_operations','ethereum_official_voluntary_exit','ethereum_official_deneb_voluntary_exit','consensus_phase0_block_authentication','ethereum_official_randao_projection'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));const r=rows.find(r=>r.scenario==='official-phase0-deposits');assert(r);
for(const [key,value] of Object.entries({valid:15,invalid:2,admitted:6,topups:5,ignoredInvalidSignature:4,validatorsCompared:966,snapshotRoundtrips:15,invalidSnapshots:12,consumedPositionRejections:15,branchMutationsRejected:45,atomicOverflowRejections:11,activeInConsensus:false}))assert.equal(r[key],value);
assert.equal(rows.find(r=>r.scenario==='slashing-snapshot-recovery').rejectedSnapshots,22);
console.log(JSON.stringify({verified:true,...r,registrySnapshotVersion:3},null,2));
