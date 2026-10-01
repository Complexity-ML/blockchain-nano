import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,mkdir,rm,readFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {execFileSync,execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {selectChanges,coreRequired,networkRequired} from './changes.mjs';
const exec=promisify(execFile);
async function fixture(fn){const dir=await mkdtemp(join(tmpdir(),'nano-ci-git-'));const git=args=>execFileSync('git',args,{cwd:dir,encoding:'utf8'});git(['init','-q']);git(['config','user.name','Nano CI fixture']);git(['config','user.email','fixture@example.invalid']);const put=async(p,text='fixture')=>{await mkdir(join(dir,p,'..'),{recursive:true});await writeFile(join(dir,p),text);};const commit=()=>{git(['add','.']);git(['commit','-qm','fixture']);return git(['rev-parse','HEAD']).trim();};try{await fn({dir,git,put,commit});}finally{await rm(dir,{recursive:true,force:true});}}
test('actual Git history: docs-only skips C++ but code renamed into docs still triggers it',async()=>fixture(async({git,put,commit})=>{
 await put('src/core.cpp');const first=commit();await put('docs/status.md');const docs=commit();assert.equal(selectChanges({before:first},'push',docs,git).core,false);
 git(['mv','src/core.cpp','docs/retired.cpp']);const renamed=commit();const r=selectChanges({before:docs},'push',renamed,git);assert.equal(r.core,true);assert.ok(r.paths.includes('src/core.cpp'));
}));
test('initial push and actual PR merge base catch core changes; manual false stays quiet',async()=>fixture(async({git,put,commit})=>{
 await put('README.md');const base=commit();await put('include/nano.hpp');const head=commit();assert.equal(selectChanges({before:'0'.repeat(40)},'push',head,git).core,true);assert.equal(selectChanges({pull_request:{base:{sha:base}}},'pull_request',head,git).core,true);
 assert.equal(selectChanges({inputs:{core:'false'}},'workflow_dispatch',head,git).core,false);assert.equal(selectChanges({inputs:{core:'true'}},'workflow_dispatch',head,git).core,true);
}));
test('invalid baselines or unrelated checkout fail closed, without executing injected Git arguments',()=>{
 let calls=0;const git=args=>{++calls;assert.deepEqual(args,['rev-parse','HEAD']);return 'a'.repeat(40);};assert.throws(()=>selectChanges({before:'--exec=evil'},'push','a'.repeat(40),git));assert.equal(calls,1);assert.throws(()=>selectChanges({},'push','b'.repeat(40),git));assert.equal(calls,2);assert.throws(()=>coreRequired(['../src/evil']));
});
test('fixture, build recipe and dependency changes require C++; harness-only changes avoid rebuilding unchanged core',()=>{for(const p of ['tests/fixtures/case.json','tests/test_core.cpp','scripts/ci/core.sh','dependencies.lock.json','.github/workflows/qualification.yml','network/main.cpp'])assert.equal(coreRequired([p]),true);assert.equal(coreRequired(['tests/multihost/monitor.mjs','docs/CI.md']),false);});
test('live build log cannot mask compiler/configuration failure behind tee',async()=>fixture(async({dir,put,commit})=>{
 await put('README.md');commit();await mkdir(join(dir,'bin'));const cmake=join(dir,'bin/cmake');await writeFile(cmake,'#!/bin/sh\nif [ "$1" = "--version" ]; then echo fixture; exit 0; fi\necho "injected configuration failure" >&2\nexit 17\n',{mode:0o755});
 const script=join(process.cwd(),'scripts/ci/core.sh');await assert.rejects(exec('bash',[script],{cwd:dir,env:{...process.env,PATH:join(dir,'bin')+':'+process.env.PATH,CXX:process.execPath}}),e=>e.code===17&&e.stdout.includes('injected configuration failure'));
}));

test('actual Git history: transport rename triggers network; docs and core-only fixture changes skip it',async()=>fixture(async({git,put,commit})=>{
 await put('network/main.cpp');const base=commit();await put('docs/new.md');const docs=commit();assert.equal(selectChanges({before:base},'push',docs,git).network,false);
 git(['mv','network/main.cpp','docs/retired-network.cpp']);const renamed=commit();assert.equal(selectChanges({before:docs},'push',renamed,git).network,true);
 assert.equal(networkRequired(['tests/test_core.cpp','docs/CI.md']),false);assert.throws(()=>networkRequired(['../network/evil']));
}));
test('network dependencies, shared headers and workflow changes trigger network; manual dispatch stays explicit',async()=>fixture(async({git,put,commit})=>{
 await put('README.md');const head=commit();for(const p of ['network/main.cpp','include/nano/types.hpp','src/consensus.cpp','cmake/libp2p-toolchain.cmake','third_party/json.hpp','scripts/build-libp2p.sh','scripts/ci/network.sh','dependencies.lock.json','.github/workflows/qualification.yml'])assert.equal(networkRequired([p]),true);
 assert.equal(selectChanges({inputs:{core:'true',network:'false'}},'workflow_dispatch',head,git).network,false);assert.equal(selectChanges({inputs:{network:'true'}},'workflow_dispatch',head,git).network,true);
}));

test('actual CMake gate rejects empty or duplicate network registrations and a foreign dependency revision',async()=>fixture(async({dir,put})=>{
 const script=join(process.cwd(),'scripts/ci/check-network.cmake');const source=await readFile(script,'utf8');const names=/set\(expected ([^\n]+)\)/.exec(source)[1].split(' ');assert.equal(names.length,21);
 const run=mode=>exec('cmake',['-DNANO_CI_CHECK='+mode,'-P',script],{cwd:dir});
 await put('ci-reports/network-selection.json',JSON.stringify({tests:[]}));await assert.rejects(run('registry'),e=>e.code!==0&&e.stderr.includes('count differs'));
 await put('ci-reports/network-selection.json',JSON.stringify({tests:names.map((name,i)=>({name:i===20?names[0]:name,properties:[]}))}));await assert.rejects(run('registry'),e=>e.code!==0&&e.stderr.includes('names differ'));
 await put('ci-reports/network-selection.json',JSON.stringify({tests:names.map((name,i)=>({name,properties:i===0?[{name:'DISABLED',value:true}]:[]}))}));await assert.rejects(run('registry'),e=>e.code!==0&&e.stderr.includes('skip property is forbidden'));
 await put('dependencies.lock.json',JSON.stringify({'cpp-libp2p':{commit:'a'.repeat(40)}}));await put('ci-reports/libp2p-revision.txt','b'.repeat(40));await assert.rejects(run('pin'),e=>e.code!==0&&e.stderr.includes('differs from reviewed lock'));
}));
