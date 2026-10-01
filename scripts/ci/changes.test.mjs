import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,mkdir,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {execFileSync,execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {selectChanges,coreRequired} from './changes.mjs';
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
