import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='ethereum-consensus-bls-verify');assert.equal(rows.length,1);
const r=rows[0];assert.equal(r.officialCases,29);assert(r.valid>0&&r.invalid>0);assert.equal(r.valid+r.invalid,29);assert.equal(r.mutationsRejected,r.valid*10);assert.equal(r.activeInConsensus,false);assert.equal(r.aggregateVerification,false);assert(text.includes('Test Passed.'));
console.log(JSON.stringify({verified:true,...r,scope:'Single BLS signature primitive against all verify vectors in ethereum/bls12-381-tests v0.1.2. Not aggregate verification, key possession, fork domains or active consensus.'},null,2));
