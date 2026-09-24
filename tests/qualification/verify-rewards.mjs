import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');
assert.equal((text.match(/Test Passed\./g)||[]).length,6);
for(const name of ['ethereum_official_phase0_rewards','ethereum_official_phase0_ffg','ethereum_official_registry_epoch_updates','ethereum_official_effective_balance_updates','ethereum_official_epoch_maintenance','consensus_slashing_epoch'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='official-phase0-rewards');assert.equal(rows.length,1);
for(const [key,value] of Object.entries({cases:19,validatorsCompared:1216,changedBalances:946,duplicateControls:19,genesisCases:2,leakCases:4,cancellations:275,localRejections:11,atomicOverflows:1,saturations:64,fullEpochTransition:false,activeInConsensus:false}))assert.equal(rows[0][key],value);
console.log(JSON.stringify({verified:true,...rows[0],productionQualified:false},null,2));
