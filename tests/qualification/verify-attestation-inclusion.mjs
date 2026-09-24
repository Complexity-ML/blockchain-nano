import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,7);
for(const name of ['ethereum_official_attestation_inclusion','consensus_phase0_registry_operations','ethereum_official_registry_block_operations','ethereum_official_phase0_ffg','ethereum_consensus_bls_verify','consensus_committee_participation','consensus_phase0_proposer'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
function verify(scenario,expected){const found=rows.filter(r=>r.scenario===scenario);assert.equal(found.length,1);for(const [k,v] of Object.entries(expected))assert.equal(found[0][k],v);return found[0];}
const admission=verify('official-phase0-attestation-inclusion',{valid:17,invalid:24,pendingAppended:17,mutationRejections:85,duplicateInclusions:17,capacityRejections:17,cancellations:70,configurationRejections:8,deferredBlockSequences:2,activeInConsensus:false,fullBlockTransition:false});
const block=verify('phase0-integrated-attestation-block',{valid:1,invalid:2,cancellations:29,configurationRejections:2,activeInConsensus:false,fullBlockTransition:false});
verify('official-phase0-ffg',{cases:10,pendingAttestations:173,duplicateControls:10,slashedControls:10,targetControls:10,thresholdControls:4,localRejections:17,cancellations:226});
console.log(JSON.stringify({verified:true,admission,block,productionQualified:false},null,2));
