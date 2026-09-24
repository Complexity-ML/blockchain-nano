import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
const r=rows.find(r=>r.scenario==='consensus-bls-fast-aggregate');
assert(r);assert.equal(r.officialCases,12);assert.equal(r.valid,3);assert.equal(r.invalid,9);assert.equal(r.mutationsRejected,33);assert.equal(r.possessionEstablished,false);assert.equal(r.activeInConsensus,false);
assert.equal((text.match(/Test Passed\./g)||[]).length,2);
const single=rows.find(r=>r.scenario==='ethereum-consensus-bls-verify');assert(single);assert.equal(single.officialCases,29);
console.log(JSON.stringify({verified:true,...r,scope:'FastAggregateVerify cryptographic primitive for a common message. Requires externally verified possession proofs. No validator authorization, quorum or consensus integration.'},null,2));
