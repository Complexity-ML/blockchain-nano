import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(x=>x.startsWith('{')).map(x=>JSON.parse(x));
const summary=rows.filter(x=>x.scenario==='ethereum-committee-primitive');assert.equal(summary.length,1);
assert.equal(summary[0].officialCases,240);assert.equal(summary[0].invalidParametersRejected,5);assert.equal(summary[0].activeInConsensus,false);
const cases=rows.filter(x=>x.scenario==='committee-partition');assert.deepEqual(cases.map(x=>x.validators),[255,256,257,1001,100000,1000000]);
for(const x of cases){assert.equal(x.committees,2048);assert.equal(x.allAssignedOnce,true);assert.equal(x.scalarDifferential,true);assert(x.milliseconds>0);}
assert(text.includes('Test Passed.'));
console.log(JSON.stringify({verified:true,officialCases:240,upstream:'ethereum/consensus-spec-tests@bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908',maximumPartitionSize:1000000,scope:'Permutation primitive and balanced partition only. Official mainnet vectors selected for counts <=100 plus scalar differential/bucket boundaries and partition invariants up to 1 million. Not epoch lifecycle, secure randomness, BLS aggregation or consensus activation.'},null,2));
