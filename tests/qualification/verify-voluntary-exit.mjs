import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,7);
for(const name of ['ethereum_official_voluntary_exit','consensus_slashing_transition','consensus_slashing_recovery','consensus_slashing_epoch','ethereum_official_proposer_slashing','consensus_block_proposer_operations','ethereum_official_block_proposer_operations'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));const r=rows.find(r=>r.scenario==='official-voluntary-exit');assert(r);
for(const [key,value] of Object.entries({valid:4,invalid:6,validatorsCompared:320,replayRejections:4,localRejections:24,atomicOverflowRejections:4,activeInConsensus:false}))assert.equal(r[key],value);
console.log(JSON.stringify({verified:true,...r},null,2));
