import test from 'node:test';
import assert from 'node:assert/strict';
import {inspectRegistry} from './regression-inventory.mjs';
const entry=(name,properties=[{name:'TIMEOUT',value:60}])=>({name,command:['/fixture/'+name],properties});
test('inventory exposes every unselected test and never claims runtime qualification',()=>{
 const report=inspectRegistry({tests:[entry('a'),entry('b',[]),entry('c')]},['a'],p=>p!=='/fixture/c');
 assert.equal(report.totalTests,3);assert.equal(report.selectedTests,1);assert.equal(report.completeSelection,false);
 assert.deepEqual(report.unselectedTests,['b','c']);assert.deepEqual(report.withoutExplicitTimeout,['b']);assert.deepEqual(report.missingExecutables,['c']);assert.equal(report.sumExplicitTimeoutSeconds,120);assert.equal(report.runtimeTestsExecuted,0);assert.equal(report.qualification,'inventory-only');
});
test('missing, duplicated and foreign registrations fail closed',()=>{
 for(const registry of [{tests:[]},{tests:[entry('a'),entry('a')]},{tests:[{name:'a',properties:[]}]}])assert.throws(()=>inspectRegistry(registry,[],()=>true));
 for(const selection of [['a','a'],['foreign']])assert.throws(()=>inspectRegistry({tests:[entry('a')]},selection,()=>true));
});
test('all CTest mechanisms that permit skipped tests are rejected',()=>{
 for(const name of ['DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION'])assert.throws(()=>inspectRegistry({tests:[entry('a',[{name,value:false}])]},['a'],()=>true),/skip property forbidden/);
});
test('full selection still reports missing binaries and invalid execution budgets',()=>{
 const report=inspectRegistry({tests:[entry('a')]},['a'],()=>false);assert.equal(report.completeSelection,true);assert.deepEqual(report.missingExecutables,['a']);
 for(const value of [0,-1,'60',NaN])assert.throws(()=>inspectRegistry({tests:[entry('a',[{name:'TIMEOUT',value}])]},['a'],()=>true),/Invalid CTest timeout/);
 assert.throws(()=>inspectRegistry({tests:[entry('a',[{name:'TIMEOUT',value:60},{name:'TIMEOUT',value:60}])]},['a'],()=>true),/duplicate property/);
});
