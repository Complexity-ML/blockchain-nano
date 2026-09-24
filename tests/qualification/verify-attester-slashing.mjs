import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,11);
for(const name of ['ethereum_official_attester_slashing','ethereum_official_attester_block_operations','consensus_phase0_registry_operations','consensus_authenticated_slashing','consensus_slashing_transition','consensus_slashing_recovery','consensus_slashing_epoch','ethereum_official_proposer_slashing','ethereum_official_attestation_inclusion','ethereum_official_phase0_ffg','ethereum_official_registry_epoch_updates'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
function verify(scenario,expected){const found=rows.filter(r=>r.scenario===scenario);assert.equal(found.length,1);for(const [k,v] of Object.entries(expected))assert.equal(found[0][k],v);return found[0];}
const operations=verify('official-phase0-attester-slashing',{valid:9,invalid:21,validatorsCompared:576,validatorsSlashed:37,replayRejections:16,mutationRejections:48,atomicOverflowRejections:8,cancellations:20,syntheticLifecycleCases:1,activeInConsensus:false});
const blocks=verify('official-phase0-attester-block-operations',{valid:3,invalid:1,attesterProofs:5,balancesCompared:192,canonicalInputsUnchanged:true,fullStateTransition:false,activeInConsensus:false});
const composition=verify('phase0-all-operation-types',{valid:1,invalid:2,cancellations:36,atomicOverflowRejections:1,fullBlockTransition:false,activeInConsensus:false});
verify('phase0-registry-block-operations',{positive:3,rejected:11,unsupported:2});
console.log(JSON.stringify({verified:true,operations,blocks,composition,productionQualified:false},null,2));
