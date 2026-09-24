import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,6);
for(const name of ['ethereum_official_deneb_voluntary_exit','ethereum_official_voluntary_exit','consensus_slashing_transition','consensus_slashing_recovery','ethereum_official_proposer_slashing','ethereum_official_block_proposer_operations'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));const r=rows.find(r=>r.scenario==='official-deneb-voluntary-exit');assert(r);
for(const [key,value] of Object.entries({valid:6,invalid:10,validatorsCompared:448,futureForkDomainChecks:6,replayRejections:6,localDomainRejections:12,configurationRejections:12,legacyProfileDifferences:2,activeInConsensus:false}))assert.equal(r[key],value);
assert.equal(rows.find(r=>r.scenario==='official-voluntary-exit').valid,4);
console.log(JSON.stringify({verified:true,...r},null,2));
