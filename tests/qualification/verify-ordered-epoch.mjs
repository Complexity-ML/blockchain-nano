import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,7);
for(const name of ['consensus_phase0_ordered_epoch','ethereum_official_phase0_rewards','ethereum_official_phase0_ffg','ethereum_official_registry_epoch_updates','ethereum_official_effective_balance_updates','ethereum_official_epoch_maintenance','consensus_slashing_epoch'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='phase0-ordered-epoch');assert.equal(rows.length,1);
for(const [key,value] of Object.entries({officialProjections:6,deferredHistorySequences:1,epochs:5,registryEntriesCompared:384,replayRejections:5,orderingControls:1,votingComposition:1,cancellations:594,localRejections:13,fullSlotTransition:false,stateRootVerified:false,activeInConsensus:false}))assert.equal(rows[0][key],value);
console.log(JSON.stringify({verified:true,...rows[0],productionQualified:false},null,2));
