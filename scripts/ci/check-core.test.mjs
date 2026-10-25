import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,mkdir,readFile,rm,copyFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,resolve,dirname} from 'node:path';
import {execFile,execFileSync} from 'node:child_process';
import {promisify} from 'node:util';
const exec=promisify(execFile),repo=process.cwd(),gate=join(repo,'scripts/ci/check-core.cmake');
const expected=['core','chain','rpc_queue_expiry','rpc_simulation_context','rpc','rpc_state_block_id','rpc_get_proof','rpc_state_read_cost','ethereum_consensus_bls_verify','ethereum_consensus_bls_fast_aggregate','storage_execution_checkpoint','consensus_capella_payload_paired_state','consensus_capella_signed_transition','consensus_capella_clock_diagnostics','consensus_capella_published_signed_inventory','consensus_capella_publication_cancel_boundary','consensus_local_monotonic_time','consensus_capella_duty_projection_availability','consensus_isolated_stopped_capture_guards','consensus_capella_sync_adverse','consensus_registry_cancellation','consensus_capella_bounded_canonical_audit','consensus_fresh_lifecycle_canonical_stream'];
const stateRegistrations={
 rpc_state_block_id:{program:'nano-rpc-state-block-id-tests',argument:'--state-readers-self-test',label:'qualification_rpc_state_block_id'},
 rpc_get_proof:{program:'nano-rpc-get-proof-tests',argument:'--get-proof-self-test',label:'qualification_rpc_state_proof'},
 rpc_state_read_cost:{program:'nano-rpc-state-read-cost-tests',argument:'--state-read-cost-self-test',label:'qualification_rpc_state_proof'}
};
const replayRegistrations={
 consensus_registry_cancellation:{program:'nano-consensus-cancellation-tests',timeout:60,labels:['qualification_consensus_ethereum']},
 consensus_capella_bounded_canonical_audit:{program:'nano-capella-bounded-audit-tests',timeout:600,labels:['consensus','capella','audit','recovery']},
 consensus_fresh_lifecycle_canonical_stream:{program:'nano-capella-canonical-stream-tests',timeout:300,labels:['qualification_consensus_lifecycle']}
};
async function fixture(fn){
 const dir=await mkdtemp(join(tmpdir(),'nano-core-selection-'));
 execFileSync('git',['init','-q',dir]);
 execFileSync('git',['-C',dir,'-c','user.name=Nano CI fixture','-c','user.email=fixture@example.invalid','commit','--allow-empty','-qm','fixture']);
 try{
  const executable=join(dir,'nano-capella-isolated-stopped-capture');await writeFile(executable,'#!/bin/sh\nexit 0\n',{mode:0o700});
  const availability=join(dir,'nano-capella-duty-availability-tests');await writeFile(availability,'#!/bin/sh\nexit 0\n',{mode:0o700});
  const simulation=join(dir,'nano-rpc-simulation-context-tests');await writeFile(simulation,'#!/bin/sh\nexit 0\n',{mode:0o700});
  const legacy=join(dir,'nano-rpc-tests');await writeFile(legacy,'#!/bin/sh\nexit 0\n',{mode:0o700});
  const adverse=join(dir,'nano-capella-sync-adverse-tests');await writeFile(adverse,'#!/bin/sh\nexit 0\n',{mode:0o700});
  await mkdir(join(dir,'tests/fixtures'),{recursive:true});await writeFile(join(dir,'tests/fixtures/python-rpc.json'),'{}\n');
  for(const {program}of Object.values(stateRegistrations))await writeFile(join(dir,program),'#!/bin/sh\nexit 0\n',{mode:0o700});
  for(const {program}of Object.values(replayRegistrations))await writeFile(join(dir,program),'#!/bin/sh\nexit 0\n',{mode:0o700});
  const registry=()=>({tests:expected.map(name=>({name,
   command:replayRegistrations[name]?[join(dir,replayRegistrations[name].program)]:name==='consensus_capella_sync_adverse'?[adverse,'--self-test']:stateRegistrations[name]?[join(dir,stateRegistrations[name].program),stateRegistrations[name].argument]:name==='rpc'?[legacy]:name==='rpc_simulation_context'?[simulation,'--simulation-context-self-test']:name==='consensus_isolated_stopped_capture_guards'?[executable,'--self-test']:name==='consensus_capella_duty_projection_availability'?[availability,'--self-test']:['/usr/bin/true'],
   properties:replayRegistrations[name]?[{name:'TIMEOUT',value:replayRegistrations[name].timeout},{name:'LABELS',value:[...replayRegistrations[name].labels]}]:name==='consensus_capella_sync_adverse'?[{name:'TIMEOUT',value:180},{name:'LABELS',value:['qualification_consensus_sync_adverse']}]:stateRegistrations[name]?[{name:'TIMEOUT',value:90},{name:'LABELS',value:[stateRegistrations[name].label]}]:name==='rpc'?[{name:'TIMEOUT',value:120},{name:'WORKING_DIRECTORY',value:dir}]:name==='rpc_simulation_context'?[{name:'TIMEOUT',value:90},{name:'LABELS',value:['qualification_rpc_simulation_context']}]:name==='consensus_isolated_stopped_capture_guards'?[{name:'TIMEOUT',value:15},{name:'LABELS',value:['qualification_consensus_recovery_tooling']}]:name==='consensus_capella_duty_projection_availability'?[{name:'TIMEOUT',value:70},{name:'LABELS',value:['qualification_consensus_duty_availability']}]:[{name:'TIMEOUT',value:120}]}))});
  const check=async data=>{const path=join(dir,'registry.json');await writeFile(path,JSON.stringify(data));return exec('cmake',['-DREGISTRY='+path,'-P',gate],{timeout:10000});};
  await fn({dir,registry,check});
 }finally{await rm(dir,{recursive:true,force:true});}
}
test('exact twenty-three daily registrations validate without claiming full C++ qualification',async()=>fixture(async({registry,check})=>assert.match((await check(registry())).stdout,/Verified 23 daily core/)));
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
 for(const change of [x=>x.command[1]='--capture',x=>x.command[0]='/usr/bin/true',x=>x.properties[0].value=16,x=>x.properties[1].value=['other'],x=>x.command.push('extra')]){const r=registry();change(r.tests.find(x=>x.name==='consensus_isolated_stopped_capture_guards'));await assert.rejects(check(r),/Isolated capture self-test/);}
 const r=registry();r.tests[0].command[0]='/missing/nano';await assert.rejects(check(r),/executable missing/);
}));
test('duty availability registration cannot substitute a different program, mode, label or budget',async()=>fixture(async({registry,check})=>{
 for(const change of [x=>x.command[1]='--pure63',x=>x.command[0]='/usr/bin/true',x=>x.properties[0].value=71,x=>x.properties[1].value=['other'],x=>x.properties[1].value.push('extra'),x=>x.command.push('extra')]){
  const r=registry();change(r.tests.find(x=>x.name==='consensus_capella_duty_projection_availability'));await assert.rejects(check(r),/Duty availability self-test/);
 }
}));
test('RPC simulation context registration binds its program, dedicated selector, label and ninety-second budget',async()=>fixture(async({registry,check})=>{
 for(const change of [x=>x.command[1]='--self-test',x=>x.command[0]='/usr/bin/true',x=>x.properties[0].value=91,x=>x.properties[1].value=['other'],x=>x.properties[1].value.push('extra'),x=>x.command.push('extra')]){
  const r=registry();change(r.tests.find(x=>x.name==='rpc_simulation_context'));await assert.rejects(check(r),/RPC simulation context self-test/);
 }
 const r=registry();r.tests=r.tests.filter(x=>x.name!=='rpc_simulation_context');await assert.rejects(check(r),/count differs/);
}));
test('new state reader and proof registrations bind every program, selector, label and ninety-second budget',async()=>fixture(async({registry,check})=>{
 for(const name of Object.keys(stateRegistrations)){
  for(const change of [x=>x.command[0]='/usr/bin/true',x=>x.command[1]='--self-test',x=>x.command.pop(),x=>x.command.push('extra'),x=>x.properties[0].value=91,x=>x.properties[1].value=['other'],x=>x.properties[1].value.push('extra')]){
   const r=registry();change(r.tests.find(x=>x.name===name));await assert.rejects(check(r),/RPC state read\/proof self-test/);
  }
  const r=registry();r.tests=r.tests.filter(x=>x.name!==name);await assert.rejects(check(r),/count differs/);
 }
}));
test('adverse sync registration cannot be omitted or substituted by another program, mode, label or budget',async()=>fixture(async({registry,check})=>{
 for(const change of [x=>x.command[0]='/usr/bin/true',x=>x.command[1]='--help',x=>x.command.pop(),x=>x.command.push('extra'),x=>x.properties[0].value=181,x=>x.properties[1].value=['other'],x=>x.properties[1].value.push('extra')]){
  const r=registry();change(r.tests.find(x=>x.name==='consensus_capella_sync_adverse'));await assert.rejects(check(r),/Capella adverse sync self-test/);
 }
 const r=registry();r.tests=r.tests.filter(x=>x.name!=='consensus_capella_sync_adverse');await assert.rejects(check(r),/count differs/);
}));
test('legacy RPC registration keeps its real program, no selector, fixture cwd and 120-second budget',async()=>fixture(async({registry,check})=>{
 for(const change of [x=>x.command[0]='/usr/bin/true',x=>x.command.push('--self-test'),x=>x.properties[0].value=121,x=>x.properties[1].value='/missing/source',x=>x.properties.pop(),x=>x.properties[1].value=123]){
  const r=registry();change(r.tests.find(x=>x.name==='rpc'));await assert.rejects(check(r),/Legacy RPC command|working directory invalid/);
 }
 const r=registry();r.tests=r.tests.filter(x=>x.name!=='rpc');await assert.rejects(check(r),/count differs/);
}));
test('all three replay/cancellation registrations require their own no-argument driver, exact budget and labels',async()=>fixture(async({registry,check})=>{
 for(const [name,registration]of Object.entries(replayRegistrations)){
  const changes=[
   x=>x.command[0]='/usr/bin/true',
   x=>x.command.push('--self-test'),
   x=>x.properties[0].value=registration.timeout+1,
   x=>x.properties[0].value=registration.timeout-1,
   x=>x.properties[1].value=[],
   x=>x.properties[1].value=['other'],
   x=>x.properties[1].value.push(registration.labels[0]),
   x=>x.properties.push({name:'DISABLED',value:false})
  ];
  for(const change of changes){
   const r=registry();change(r.tests.find(x=>x.name===name));
   await assert.rejects(check(r),/Canonical replay\/cancellation|skip property forbidden/);
  }
  const r=registry();r.tests=r.tests.filter(x=>x.name!==name);
  await assert.rejects(check(r),/count differs/);
 }
}));
test('bounded canonical audit labels are an exact set and the old deposit-auditor selector cannot stand in for the thin stream driver',async()=>fixture(async({registry,check})=>{
 const reordered=registry();reordered.tests.find(x=>x.name==='consensus_capella_bounded_canonical_audit').properties[1].value.reverse();
 assert.match((await check(reordered)).stdout,/Verified 23 daily core/);
 for(const labels of [['consensus','capella','audit','audit'],['consensus','capella','audit','other'],['consensus','capella','audit',3],['audit;capella','','consensus','recovery'],['audit','capella','consensus','recovery;']]){
  const r=registry();r.tests.find(x=>x.name==='consensus_capella_bounded_canonical_audit').properties[1].value=labels;
  await assert.rejects(check(r),/Canonical replay/);
 }
 const oldSelector=registry(),stream=oldSelector.tests.find(x=>x.name==='consensus_fresh_lifecycle_canonical_stream');
 stream.command.push('--fresh-stream-self-test');
 await assert.rejects(check(oldSelector),/Canonical replay\/cancellation command/);
}));
test('real daily shell builds and hashes every added driver and selects all twenty-three reviewed names',async()=>{
 const source=await readFile(join(repo,'scripts/ci/core.sh'),'utf8'),lines=source.split('\n');
 const build=lines.filter(line=>line.startsWith('cmake --build ')&&line.includes(' --target '));assert.equal(build.length,1);
 const targets=build[0].split(' --target ')[1].split(' 2>&1')[0].split(/\s+/);
 const hashes=lines.filter(line=>line.startsWith('sha256sum ')&&line.endsWith(' > ci-reports/executables.sha256'));assert.equal(hashes.length,1);
 for(const {program}of Object.values(replayRegistrations)){
  assert.equal(targets.filter(x=>x===program).length,1);
  assert.equal(hashes[0].split('"$build_dir/'+program+'"').length,2);
 }
 const patterns=lines.filter(line=>line.startsWith("pattern='"));assert.equal(patterns.length,1);
 const selection=patterns[0].slice("pattern='^(".length,-")$'".length).split('|');
 assert.deepEqual(selection,expected);assert.equal(new Set(selection).size,23);
});

// Exercise the real shell's result gate with injected build/CTest adapters.
// These are harness controls: no C++ test, database or validator runs here.
test('daily shell refuses a green CTest exit with a skipped or incomplete JUnit report',async()=>fixture(async({dir,registry})=>{
 const realCmake=execFileSync('which',['cmake'],{encoding:'utf8'}).trim();
 await mkdir(join(dir,'bin'));await mkdir(join(dir,'scripts/ci'),{recursive:true});await mkdir(join(dir,'build'));
 for(const name of ['check-core.cmake','verify-results.sh'])await copyFile(join(repo,'scripts/ci',name),join(dir,'scripts/ci',name));
 const coreSource=await readFile(join(repo,'scripts/ci/core.sh'),'utf8');
 const buildLine=coreSource.split('\n').find(line=>line.startsWith('cmake --build ')&&line.includes(' --target '));
 assert(buildLine,'reviewed core build command missing');
 const targets=buildLine.split(' --target ')[1].split(' 2>&1')[0].split(/\s+/);
 assert(targets.length>0&&targets.every(name=>/^nano(?:-[A-Za-z0-9_-]+)?$/.test(name)),'unexpected fixture target');
 // The build adapter does not compile. Materialize its named products so an
 // unrelated missing executable cannot masquerade as the JUnit refusal.
 for(const name of targets)await writeFile(join(dir,'build',name),'#!/bin/sh\necho "Unexpected post-JUnit fixture executable" >&2\nexit 97\n',{mode:0o700});
 await writeFile(join(dir,'build/libnano-test-sync-fault.so'),'injected fixture library');
 assert.equal(await readFile(join(dir,'build/libnano-test-sync-fault.so'),'utf8'),'injected fixture library');
 await writeFile(join(dir,'fixture-registry.json'),JSON.stringify(registry()));
 await writeFile(join(dir,'bin/cmake'),'#!/bin/sh\ncase "$1" in -DREGISTRY=*) exec "$NANO_FIXTURE_CMAKE" "$@" ;; *) exit 0 ;; esac\n',{mode:0o700});
 await writeFile(join(dir,'bin/ctest'),'#!/bin/sh\ncase " $* " in *--show-only=json-v1*) cat fixture-registry.json ;; *) printf "ctest executed\n" > fixture-ctest-executed; cp fixture.xml ci-reports/core.xml ;; esac\n',{mode:0o700});
 const environment={...process.env,PATH:join(dir,'bin')+':'+dirname(process.execPath)+':'+process.env.PATH,CXX:process.execPath,NANO_FIXTURE_CMAKE:realCmake};
 for(const key of ['LD_PRELOAD','LD_LIBRARY_PATH','DYLD_INSERT_LIBRARIES','DYLD_LIBRARY_PATH'])delete environment[key];
 const all=expected.map(name=>'<testcase name="'+name+'" status="run"/>');
 const scenarios=[
  {name:'incomplete',cases:all.slice(0,-1),reason:'missing, duplicated or foreign JUnit result'},
  {name:'skipped',cases:all.map((x,i)=>i===0?x.replace('/>','><skipped/></testcase>'):x),reason:'skipped/non-executed result: core'}
 ];
 for(const scenario of scenarios){
  const xml='<testsuite tests="'+expected.length+'" failures="0">'+scenario.cases.join('')+'</testsuite>';
  await writeFile(join(dir,'fixture.xml'),xml);
  for(const name of ['fixture-ctest-executed','ci-reports/core.xml'])await rm(join(dir,name),{force:true});
  let failure;
  try{await exec('bash',['-x',join(repo,'scripts/ci/core.sh'),'build'],{cwd:dir,env:environment,timeout:10000});}catch(error){failure=error;}
  const capture=async name=>{try{return (await readFile(join(dir,name),'utf8')).slice(-8192);}catch{return '<not created>';}};
  const diagnostic=JSON.stringify({scenario:scenario.name,code:failure?.code,signal:failure?.signal,
   stdout:String(failure?.stdout??'').slice(-8192),stderr:String(failure?.stderr??'').slice(-16384),
   selectionAudit:await capture('ci-reports/selection-audit.log'),resultAudit:await capture('ci-reports/core-result-audit.json')},null,2);
  assert(failure,'shell accepted an invalid JUnit report\n'+diagnostic);
  assert.equal(failure.code,1,diagnostic);
  assert.equal(await capture('fixture-ctest-executed'),'ctest executed\n',diagnostic);
  assert.equal(await capture('ci-reports/core.xml'),xml,diagnostic);
  assert.match(await capture('ci-reports/selection-audit.log'),/Verified 23 daily core/,diagnostic);
  assert(String(failure.stderr).split('\n').includes(scenario.reason),diagnostic);
 }
}));
test('operations job refuses a missing mandatory recovery test before globs can hide it',async()=>fixture(async({dir})=>{
 const missing='tests/capella/isolated-fixture-domain.test.mjs';
 // Isolate this refusal from the required-list order and shell glob contents.
 for(const name of ['scripts/operations/stopped-backup.test.mjs','tests/capella/isolated-bootstrap-plan.test.mjs','tests/capella/isolated-runtime-observation.test.mjs','tests/capella/pinned-json-input.test.mjs','tests/capella/isolated-core-source.test.mjs','scripts/ci/check-core.test.mjs']){
  await mkdir(join(dir,name,'..'),{recursive:true});await writeFile(join(dir,name),'// Presence-only fixture; this module must not be executed.\n');
 }
 let failure;
 try{await exec('sh',[join(repo,'scripts/ci/operations.sh')],{cwd:dir,env:{...process.env,PATH:dirname(process.execPath)+':'+process.env.PATH},timeout:10000});}catch(error){failure=error;}
 const diagnostic=JSON.stringify({code:failure?.code,signal:failure?.signal,stdout:String(failure?.stdout??'').slice(-8192),stderr:String(failure?.stderr??'').slice(-8192)},null,2);
 assert(failure,'operations accepted a missing mandatory test\n'+diagnostic);
 assert.equal(failure.code,1,diagnostic);
 assert.equal(failure.stderr,'Missing required recovery test: '+missing+'\n',diagnostic);
}));
