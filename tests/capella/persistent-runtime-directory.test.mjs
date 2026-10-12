import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,realpath,rm,stat,symlink,writeFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import {persistentRuntimeDirectory} from './persistent-runtime-directory.mjs';
const repo=fileURLToPath(new URL('../../',import.meta.url));
const controller=join(repo,'tests/capella/restored-three-node-run.mjs');
const temporaryError=/inside system temporary storage/;
const exists=async p=>{try{await stat(p);return true;}catch(error){if(error.code==='ENOENT')return false;throw error;}};
async function tempFixture(root,fn){let directory;try{await stat(root);}catch(error){if(error.code==='ENOENT')return null;throw error;}try{directory=await mkdtemp(join(root,'nano-runtime-location-'));return await fn(directory);}finally{if(directory)await rm(directory,{recursive:true,force:true});}}
async function durableFixture(fn){const directory=await mkdtemp(join(repo,'.nano-runtime-persistent-'));try{return await fn(directory);}finally{await rm(directory,{recursive:true,force:true});}}
for(const root of [...new Set(['/tmp','/private/tmp','/var/tmp',tmpdir()])])test('refuses actual system temporary root and descendants '+root,async context=>{
 if(!(await exists(root))){context.skip('Temporary root does not exist on this platform');return;}
 await assert.rejects(persistentRuntimeDirectory(root),temporaryError);
 await tempFixture(root,async directory=>{const child=join(directory,'nested','prepared');await mkdir(child,{recursive:true});await assert.rejects(persistentRuntimeDirectory(directory),temporaryError);await assert.rejects(persistentRuntimeDirectory(child),temporaryError);});
});
test('real aliases resolve to the same forbidden temporary storage',async context=>{
 if(!(await exists('/tmp'))||!(await exists('/private/tmp'))){context.skip('Platform does not expose both temporary aliases');return;}
 const a=await realpath('/tmp'),b=await realpath('/private/tmp');
 if(a===b){await assert.rejects(persistentRuntimeDirectory(a),temporaryError);await assert.rejects(persistentRuntimeDirectory(b),temporaryError);}
 else{await assert.rejects(persistentRuntimeDirectory('/tmp'),temporaryError);await assert.rejects(persistentRuntimeDirectory('/private/tmp'),temporaryError);}
});
test('persistent directories containing tmp or temporary in their names remain accepted',async()=>durableFixture(async directory=>{
 for(const name of ['tmp','tmp-runtime','temporary-data','state-not-in-tmp']){const child=join(directory,name,'prepared');await mkdir(child,{recursive:true});assert.equal(await persistentRuntimeDirectory(child),await realpath(child));}
}));
test('durable symlink into temporary storage is refused by its real target',async()=>durableFixture(async directory=>tempFixture(tmpdir(),async temporary=>{
 const link=join(directory,'durable-name');await symlink(temporary,link,'dir');await assert.rejects(persistentRuntimeDirectory(link),temporaryError);
 const descendant=join(temporary,'child');await mkdir(descendant);await assert.rejects(persistentRuntimeDirectory(join(link,'child')),temporaryError);
})));
test('symlink to persistent storage returns its canonical existing directory',async()=>durableFixture(async directory=>{
 const target=join(directory,'tmp-word-persistent');await mkdir(target);const link=join(directory,'alias');await symlink(target,link,'dir');assert.equal(await persistentRuntimeDirectory(link),await realpath(target));
}));
test('missing or non-directory preparation is refused without creation',async()=>durableFixture(async directory=>{
 const missing=join(directory,'missing');await assert.rejects(persistentRuntimeDirectory(missing),{code:'ENOENT'});assert.equal(await exists(missing),false);
 const file=join(directory,'file');await writeFile(file,'fixture');await assert.rejects(persistentRuntimeDirectory(file),/existing directory/);
}));
test('controller rejects temporary preparation before campaign mkdir and all process work',async()=>tempFixture(tmpdir(),async directory=>{
 const run=spawnSync(process.execPath,[controller,'fixture@127.0.0.1','a'.repeat(40),directory],{cwd:repo,env:{...process.env,PATH:''},encoding:'utf8',timeout:5000});
 assert.equal(run.error,undefined);assert.equal(run.status,1);assert.match(run.stderr,temporaryError);assert.equal(await exists(join(directory,'campaign')),false);assert.equal(run.stdout,'');
}));
test('controller rejects durable symlink into temporary storage before campaign mkdir',async()=>durableFixture(async directory=>tempFixture(tmpdir(),async temporary=>{
 const link=join(directory,'runtime');await symlink(temporary,link,'dir');const run=spawnSync(process.execPath,[controller,'fixture@127.0.0.1','a'.repeat(40),link],{cwd:repo,env:{...process.env,PATH:''},encoding:'utf8',timeout:5000});
 assert.equal(run.error,undefined);assert.equal(run.status,1);assert.match(run.stderr,temporaryError);assert.equal(await exists(join(temporary,'campaign')),false);assert.equal(run.stdout,'');
})));
