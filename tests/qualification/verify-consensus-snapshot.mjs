import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,4);
for(const name of ['consensus_phase0_snapshot_recovery','ethereum_official_full_blocks','ethereum_official_full_slots','consensus_slashing_recovery'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='phase0-slot-snapshot-recovery');assert.equal(rows.length,1);
for(const [key,value] of Object.entries({checks:12,rejectedSnapshots:25,abruptExitAfterCommit:true,resumedStateMatchesOfficial:true,powerLossQualified:false,executionConsensusBindingQualified:false,activeInConsensus:false}))assert.equal(rows[0][key],value);
console.log(JSON.stringify({verified:true,...rows[0],productionQualified:false},null,2));
