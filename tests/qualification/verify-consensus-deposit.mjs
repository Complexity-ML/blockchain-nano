import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='consensus-deposit-signature');assert.equal(rows.length,1);const r=rows[0];
assert.equal(r.officialRoots,5);assert.equal(r.signedCases,3);assert.equal(r.mutationsRejected,24);assert.equal(r.fundingAuthenticated,false);assert.equal(r.activeInConsensus,false);assert(text.includes('Test Passed.'));
console.log(JSON.stringify({verified:true,...r,scope:'DepositMessage SSZ roots and BLS signature proof of possession only. No authentication of execution-layer funding, deposit ordering or registry activation.'},null,2));
