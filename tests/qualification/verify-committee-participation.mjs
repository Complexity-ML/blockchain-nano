import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');
assert.equal((text.match(/Test Passed\./g)||[]).length,1);
assert(text.includes('Testing: consensus_committee_participation\n'));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));assert.equal(rows.length,1);
const r=rows[0];assert.equal(r.scenario,'consensus-committee-participation');assert.equal(r.validAggregates,3);assert.equal(r.rejected,20);assert.equal(r.wideWeight,true);assert.equal(r.snapshotOwned,true);assert.equal(r.activeInConsensus,false);
console.log(JSON.stringify({verified:true,...r},null,2));
