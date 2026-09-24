import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const dir=process.argv[2];
const initial=readFileSync(resolve(dir,'tests-full.log'),'utf8');assert.equal((initial.match(/Test Passed\./g)||[]).length,3);
for(const name of ['staking','staking_paris','staking_query_rules'])assert(initial.includes(`Testing: ${name}\n`));
const final=readFileSync(resolve(dir,'total-budget-full.log'),'utf8');assert(final.includes('Test Passed.'));
const rows=final.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));assert.equal(rows.length,1);const r=rows[0];assert.equal(r.scenario,'staking-query-rules');assert.equal(r.invalidConfigurationsRejected,10);for(const key of ['identityBound','reopenMismatchRejected','gasBudgetEnforced'])assert.equal(r[key],true);
console.log(JSON.stringify({verified:true,...r,scope:'Versioned genesis staking query budgets; invalid configs, identity binding, database mismatch, per-call and total-budget failure without input-state changes. Existing staking regressions pass. Does not implement pagination or unlimited registry capacity.'},null,2));
