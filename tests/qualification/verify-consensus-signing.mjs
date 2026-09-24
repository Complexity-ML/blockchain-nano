import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='consensus-signing-roots');
assert.equal(rows.length,1);const r=rows[0];assert.equal(r.forkDataCases,5);assert.equal(r.signingDataCases,5);assert.equal(r.activeInConsensus,false);assert(text.includes('Test Passed.'));
console.log(JSON.stringify({verified:true,...r,scope:'SSZ ForkData and SigningData roots and domain construction. Not integrated with validator registry, epoch schedule or consensus votes.'},null,2));
