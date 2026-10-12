import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,mkdir,readFile,rm,copyFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {execFile,execFileSync} from 'node:child_process';
import {promisify} from 'node:util';
const exec=promisify(execFile),repo=process.cwd(),gate=join(repo,'scripts/ci/check-core.cmake');
const expected=['core','chain','rpc_queue_expiry','ethereum_consensus_bls_verify','ethereum_consensus_bls_fast_aggregate','storage_execution_checkpoint','consensus_capella_payload_paired_state','consensus_capella_signed_transition','consensus_capella_clock_diagnostics','consensus_capella_published_signed_inventory','consensus_capella_publication_cancel_boundary','consensus_local_monotonic_time','consensus_isolated_stopped_capture_guards'];
async function fixture(fn){
 const dir=await mkdtemp(join(tmpdir(),'nano-core-selection-'));
 execFileSync('git',['init','-q',dir]);
 execFileSync('git',['-C',dir,'-c','user.name=Nano CI fixture','-c','user.email=fixture@example.invalid','commit','--allow-empty','-qm','fixture']);
 try{
  const executable=join(dir,'nano-capella-isolated-stopped-capture');await writeFile(executable,'#!/bin/sh\nexit 0\n',{mode:0o700});
  const registry=()=>({tests:expected.map(name=>({name,command:name===expected.at(-1)?[executable,'--self-test']:['/usr/bin/true'],properties:name===expected.at(-1)?[{name:'TIMEOUT',value:15},{name:'LABELS',value:['qualification_consensus_recovery_tooling']}]:[{name:'TIMEOUT',value:120}]}))});
  const check=async data=>{const path=join(dir,'registry.json');await writeFile(path,JSON.stringify(data));return exec('cmake',['-DREGISTRY='+path,'-P',gate],{timeout:10000});};
  await fn({dir,registry,check});
 }finally{await rm(dir,{recursive:true,force:true});}
}
test('exact thirteen daily registrations validate without claiming full C++ qualification',async()=>fixture(async({registry,check})=>assert.match((await check(registry())).stdout,/Verified 13 daily core/)));
test('missing, duplicated and foreign registrations cannot pass daily core gate',async()=>fixture(async({registry,check})=>{
 for(const change of [x=>x.tests.pop(),x=>x.tests[3].name=x.tests[2].name,x=>x.tests[3].name='foreign']){const r=registry();change(r);await assert.rejects(check(r),/count differs|names differ/);}
}));
test('all CTest skip properties are refused even when value is false',async()=>fixture(async({registry,check})=>{
 for(const name of ['DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION']){const r=registry();r.tests[0].properties.push({name,value:false});await assert.rejects(check(r),/skip property forbidden/);}
}));
test('green overrides cannot invert or ignore a failed positive C++ check',async()=>fixture(async({registry,check})=>{
 for(const [name,value] of [['WILL_FAIL',true],['WILL_FAIL',false],['PASS_REGULAR_EXPRESSION',['pretend pass']]]){const r=registry();r.tests[0].properties.push({name,value});await assert.rejects(check(r),/skip property forbidden/);}
}));
test('explicit positive numeric budgets and unique properties are required',async()=>fixture(async({registry,check})=>{
 for(const props of [[],[{name:'TIMEOUT',value:0}],[{name:'TIMEOUT',value:'120'}],[{name:'TIMEOUT',value:120},{name:'TIMEOUT',value:120}]]){const r=registry();r.tests[0].properties=props;await assert.rejects(check(r),/timeout|duplicate property/);}
}));
test('new capture command, helper name, label and exact fifteen-second budget are bound',async()=>fixture(async({registry,check})=>{
 for(const change of [x=>x.command[1]='--capture',x=>x.command[0]='/usr/bin/true',x=>x.properties[0].value=16,x=>x.properties[1].value=['other'],x=>x.command.push('extra')]){const r=registry();change(r.tests.at(-1));await assert.rejects(check(r),/Isolated capture self-test/);}
 const r=registry();r.tests[0].command[0]='/missing/nano';await assert.rejects(check(r),/executable missing/);
}));
// Exercise the real shell's result gate with injected build/CTest adapters.
// These are harness controls: no C++ test, database or validator runs here.
test('daily shell refuses a green CTest exit with a skipped or incomplete JUnit report',async()=>fixture(async({dir,registry})=>{
 const realCmake=execFileSync('which',['cmake'],{encoding:'utf8'}).trim();
 await mkdir(join(dir,'bin'));await mkdir(join(dir,'scripts/ci'),{recursive:true});await mkdir(join(dir,'build'));
 for(const name of ['check-core.cmake','verify-results.sh'])await copyFile(join(repo,'scripts/ci',name),join(dir,'scripts/ci',name));
 await writeFile(join(dir,'build/libnano-test-sync-fault.so'),'injected fixture library');
 await writeFile(join(dir,'fixture-registry.json'),JSON.stringify(registry()));
 await writeFile(join(dir,'bin/cmake'),'#!/bin/sh\ncase "$1" in -DREGISTRY=*) exec "$NANO_FIXTURE_CMAKE" "$@" ;; *) exit 0 ;; esac\n',{mode:0o700});
 await writeFile(join(dir,'bin/ctest'),'#!/bin/sh\ncase " $* " in *--show-only=json-v1*) cat fixture-registry.json ;; *) cp fixture.xml ci-reports/core.xml ;; esac\n',{mode:0o700});
 const environment={...process.env,PATH:join(dir,'bin')+':'+process.env.PATH,CXX:process.execPath,NANO_FIXTURE_CMAKE:realCmake};
 for(const key of ['LD_PRELOAD','LD_LIBRARY_PATH','DYLD_INSERT_LIBRARIES','DYLD_LIBRARY_PATH'])delete environment[key];
 const all=expected.map(name=>'<testcase name="'+name+'" status="run"/>');
 for(const cases of [all.slice(0,-1),all.map((x,i)=>i===0?x.replace('/>','><skipped/></testcase>'):x)]){
  await writeFile(join(dir,'fixture.xml'),'<testsuite tests="13" failures="0">'+cases.join('')+'</testsuite>');
  await assert.rejects(exec('bash',[join(repo,'scripts/ci/core.sh'),'build'],{cwd:dir,env:environment,timeout:10000}),e=>e.code!==0&&/missing, duplicated|skipped\/non-executed/.test(e.stderr));
 }
}));
test('operations job refuses a missing mandatory recovery test before globs can hide it',async()=>fixture(async({dir})=>{
 await assert.rejects(exec('sh',[join(repo,'scripts/ci/operations.sh')],{cwd:dir,timeout:10000}),e=>e.code!==0&&e.stderr.includes('Missing required recovery test: tests/capella/isolated-fixture-domain.test.mjs'));
}));
