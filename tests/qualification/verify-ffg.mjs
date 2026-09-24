import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,7);
for(const name of ['ethereum_official_phase0_ffg','ethereum_official_registry_epoch_updates','ethereum_committee_primitive','consensus_authenticated_randao','ethereum_official_randao_projection','consensus_phase0_proposer','consensus_selected_randao'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));const found=rows.filter(r=>r.scenario==='official-phase0-ffg');assert.equal(found.length,1);
for(const [key,value] of Object.entries({cases:10,changedCheckpoints:9,pendingAttestations:173,duplicateControls:10,slashedControls:10,targetControls:10,thresholdControls:4,localRejections:17,cancellations:226,fullEpochTransition:false,activeInConsensus:false}))assert.equal(found[0][key],value);
console.log(JSON.stringify({verified:true,...found[0],productionQualified:false},null,2));
