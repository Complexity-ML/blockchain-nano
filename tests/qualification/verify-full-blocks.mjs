import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,5);
for(const name of ['ethereum_official_full_blocks','ethereum_official_full_slots','ethereum_official_beacon_state_root','consensus_phase0_registry_operations','consensus_phase0_ordered_epoch'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='official-phase0-full-blocks');assert.equal(rows.length,1);
for(const [key,value] of Object.entries({valid:29,invalid:16,acceptedBlocks:97,serializedBytesCompared:530412,stateRootsVerified:true,authenticatedWrongRootRejections:1,cancellationPoints:7,mutations:8,configurationRejections:4,activeInConsensus:false}))assert.equal(rows[0][key],value);
console.log(JSON.stringify({verified:true,...rows[0],productionQualified:false},null,2));
