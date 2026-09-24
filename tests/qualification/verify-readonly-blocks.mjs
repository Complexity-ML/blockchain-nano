import {readFileSync,readdirSync} from 'node:fs';
import {resolve,relative} from 'node:path';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
import {parseReport} from './audit-evidence.mjs';
const campaign=resolve(process.argv[2]);
const root=resolve(process.argv[3]??'.');
const fixtureDir=resolve(root,'tests/fixtures/ethereum-blocks');
const expected=new Set(),inventory=[];
for(const name of readdirSync(fixtureDir).filter(n=>n.endsWith('.json')).sort()){
 const path=resolve(fixtureDir,name),data=readFileSync(path);
 inventory.push({path:relative(root,path),sha256:createHash('sha256').update(data).digest('hex')});
 for(const [name,test] of Object.entries(JSON.parse(data))){
  const id=JSON.stringify([name,test.network,0]);assert(!expected.has(id));expected.add(id);
 }
}
const report=parseReport(readFileSync(resolve(campaign,'blocks.jsonl'),'utf8'));
assert.deepEqual(report.issues,[]);
assert.equal(readFileSync(resolve(campaign,'exit-code.txt'),'utf8').trim(),'0');
assert.equal(report.counts.matched,expected.size);
assert.equal(report.counts.mismatched+report.counts.unsupported+report.counts.errors,0);
assert.equal(report.index.size,expected.size);
for(const id of expected)assert(report.index.has(id),`missing ${id}`);
assert.equal(report.summary.files,inventory.length);
const recorded=JSON.parse(readFileSync(resolve(campaign,'fixture-inventory.json'),'utf8'));
assert.deepEqual(recorded,inventory);
console.log(JSON.stringify({verified:true,files:inventory.length,cases:expected.size,scope:'Exact current checked-in block corpus only. No claim about omitted upstream fixtures, block/network activation, or production readiness.'},null,2));
