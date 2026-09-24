import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const dir=process.argv[2];
function suite(file,names){
 const text=readFileSync(resolve(dir,file),'utf8');
 assert.equal((text.match(/Test Passed\./g)||[]).length,names.length);
 assert(!text.includes('Test Failed.'));
 for(const name of names)assert(text.includes(`Testing: ${name}\n`),`missing ${name}`);
 return text;
}
const native=suite('native-full.log',['core','staking','staking_paris','staking_query_rules','staking_pagination','evm_readonly_state']);
suite('rpc-blobs-full.log',['rpc','rpc_shutdown','blobs']);
const rows=native.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
const records=rows.filter(r=>r.scenario==='evm-readonly-state');assert.equal(records.length,1);
const r=records[0];assert.equal(r.staticCases,20);
for(const flag of ['cancellationIsolated','mutableContextRejected','mutableParentRollback'])assert.equal(r[flag],true);
console.log(JSON.stringify({verified:true,tests:9,...r,scope:'Scoped Release regression: immutable static calls, nested calls, cancellation and mutable rollback; staking, RPC shutdown and blobs. Does not replace official fork conformance or distributed qualification.'},null,2));
