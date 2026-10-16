// Pure injection checks only. No files, processes, keys, clocks or network.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {nativeRuntimeEnvironment} from './native-runtime-env.mjs';

test('native environment is a stable canonical snapshot and preserves approved overlays',()=>{
 const parent={TZ:'UTC',PATH:'/tools/bin:/usr/bin',HOME:'/owner',NANO_TEST_NETWORK_TRACE:'1'},before=structuredClone(parent);
 const base=nativeRuntimeEnvironment(parent);
 assert.equal(Object.getPrototypeOf(base),null);assert(Object.isFrozen(base));assert.deepEqual(Object.keys(base),['HOME','NANO_TEST_NETWORK_TRACE','PATH','TZ']);assert.deepEqual({...base},parent);assert.deepEqual(parent,before);
 parent.PATH='/changed';parent.DYLD_LIBRARY_PATH='/late';
 assert.equal(base.PATH,'/tools/bin:/usr/bin');assert.equal(Object.hasOwn(base,'DYLD_LIBRARY_PATH'),false);
 const fixture={NANO_TEST_CAPELLA_NETWORK_KEY_0:'0x'+'1'.repeat(64),NANO_TEST_ISOLATED_CAPELLA_SEED_FILE:'/fixture/seed',NANO_TEST_ISOLATED_CAPELLA_LEDGER_DIR:'/fixture/ledger',NANO_TEST_FRESH_SUBMISSION_PLAN:'/fixture/plan',NANO_TEST_FRESH_SUBMISSION_PLAN_SHA256:'a'.repeat(64)},fixtureBefore=structuredClone(fixture),launch=nativeRuntimeEnvironment(base,fixture);
 for(const [name,value] of Object.entries(fixture))assert.equal(launch[name],value);
 assert.equal(launch.PATH,base.PATH);assert.equal(Object.hasOwn(base,'NANO_TEST_CAPELLA_NETWORK_KEY_0'),false);assert.deepEqual(fixture,fixtureBefore);
 assert.throws(()=>{base.PATH='/mutated';},TypeError);
 assert.deepEqual(before,{TZ:'UTC',PATH:'/tools/bin:/usr/bin',HOME:'/owner',NANO_TEST_NETWORK_TRACE:'1'});
});

test('Darwin, selected ELF and managed-child controls refuse before the caller can act',()=>{
 const forbidden=['DYLD_LIBRARY_PATH','DYLD_INSERT_LIBRARIES','DYLD_FUTURE_CONTROL','LD_PRELOAD','LD_LIBRARY_PATH','LD_AUDIT','NANO_SUPERVISOR_PID','NANO_NETWORK_TOKEN'];let effects=0;
 for(const name of forbidden)for(const position of ['parent','overrides']){
  const parent={PATH:'/usr/bin'},overrides={},value=name==='DYLD_FUTURE_CONTROL'?'':'untrusted';(position==='parent'?parent:overrides)[name]=value;
  const beforeParent=structuredClone(parent),beforeOverrides=structuredClone(overrides);
  assert.throws(()=>{nativeRuntimeEnvironment(parent,overrides);++effects;},/Fresh native environment:/);
  assert.deepEqual(parent,beforeParent);assert.deepEqual(overrides,beforeOverrides);
 }
 assert.equal(effects,0);assert.equal(forbidden.length*2,16);
 // These are distinct, case-sensitive names. LDFLAGS/CPATH are compiler inputs,
 // not part of this runtime-loader policy; no general ELF policy is claimed.
 const allowed={LDFLAGS:'-Wl,example',CPATH:'/compile/include',LD_APPLICATION_SETTING:'value',dyld_library_path:'value'};
 assert.deepEqual({...nativeRuntimeEnvironment(allowed)},allowed);
});

test('native environment refuses noncanonical values and never mutates its inputs',()=>{
 const invalid=[undefined,null,0,false,{},[],String.fromCharCode(0)];let effects=0;
 for(const value of invalid){const input={PATH:'/usr/bin',APPLICATION_SETTING:value},before=structuredClone(input);assert.throws(()=>{nativeRuntimeEnvironment(input);++effects;},/environment values/);assert.deepEqual(input,before);}
 for(const name of ['', 'A=B', 'A\nB'])assert.throws(()=>nativeRuntimeEnvironment({[name]:'value'}),/noncanonical environment name/);
 for(const input of [null,[],false])assert.throws(()=>nativeRuntimeEnvironment(input),/environment object/);
 const inherited=Object.create({DYLD_INSERT_LIBRARIES:'/not-an-own-entry'});inherited.PATH='/usr/bin';const safe=nativeRuntimeEnvironment(inherited);assert.deepEqual({...safe},{PATH:'/usr/bin'});assert.equal(Object.getPrototypeOf(safe),null);
 assert.equal(effects,0);assert.equal(invalid.length+3+3,13);
});
