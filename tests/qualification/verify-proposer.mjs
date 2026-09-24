import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');
assert.equal((text.match(/Test Passed\./g)||[]).length,3);
for(const name of ['ethereum_committee_primitive','ethereum_official_randao_projection','consensus_phase0_proposer'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));assert.equal(rows.length,11);
const r=rows.find(r=>r.scenario==='phase0-proposer-selection');assert(r);assert.equal(r.referenceComparisons,98);assert.equal(r.rejected,10);assert.equal(r.cancellation,true);assert.equal(r.activeInConsensus,false);
assert.equal(rows.find(r=>r.scenario==='official-randao-projection').proposersMatched,3);
assert.equal(rows.find(r=>r.scenario==='ethereum-committee-primitive').officialCases,240);
const scale=rows.filter(r=>r.scenario==='proposer-registry-scale');assert.deepEqual(scale.map(r=>r.validators),[100000,1000000]);
for(const row of scale){assert.equal(row.distributedNetwork,false);assert(row.selected>=0&&row.selected<row.validators);assert(Number.isFinite(row.milliseconds)&&row.milliseconds>=0);}
console.log(JSON.stringify({verified:true,...r,scale},null,2));
