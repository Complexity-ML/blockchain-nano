import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');
assert.equal((text.match(/Test Passed\./g)||[]).length,7);
for(const name of ['ethereum_official_proposer_slashing','consensus_authenticated_slashing','consensus_slashing_transition','consensus_slashing_recovery','consensus_slashing_epoch','ethereum_official_slashings','consensus_phase0_block_authentication'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));const r=rows.find(r=>r.scenario==='official-proposer-slashing');assert(r);
for(const [key,value] of Object.entries({valid:3,invalid:12,invalidRegistries:1,balancesCompared:192,replayRejections:6,localRejections:12,atomicOverflowRejections:3,activeInConsensus:false}))assert.equal(r[key],value);
console.log(JSON.stringify({verified:true,...r},null,2));
