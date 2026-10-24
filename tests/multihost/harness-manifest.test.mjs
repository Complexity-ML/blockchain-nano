import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,writeFile,rm,symlink,readFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {hashHarness} from './harness-manifest.mjs';
const repo=fileURLToPath(new URL('../../',import.meta.url));
const sha=bytes=>createHash('sha256').update(bytes).digest('hex');
async function fixture(t){const root=await mkdtemp(join(tmpdir(),'nano-harness-manifest-'));t.after(()=>rm(root,{recursive:true,force:true}));await mkdir(join(root,'tests/multihost'),{recursive:true});return root;}
test('actual runner inventory covers restored audit and transitive finality/monitor dependencies',async()=>{
 const hashes=await hashHarness(repo);
 for(const path of ['run.mjs','campaign-deadline.mjs','campaign-commands.mjs','owner-recovery.mjs','rotation-owned-copy.mjs','harness-manifest.mjs','restore-offline-audit.mjs','../assertoor/stability.mjs','../../scripts/operations/finality-monitor.mjs','../../scripts/operations/durable-json.mjs','../../scripts/operations/monitor-transport.mjs','../../scripts/operations/monitor-and-deliver.mjs','../../scripts/operations/alert-delivery.mjs','backup-restore.sh'])assert.equal(hashes[path],sha(await readFile(resolve(repo,'tests/multihost',path))));
});
test('dependency-only edit changes its hash while entry hash remains identical',async t=>{
 const root=await fixture(t);await writeFile(join(root,'entry.mjs'),"import {x} from './leaf.mjs';\n");await writeFile(join(root,'leaf.mjs'),'export const x=1;');const before=await hashHarness(root,['entry.mjs']);await writeFile(join(root,'leaf.mjs'),'export const x=2;');const after=await hashHarness(root,['entry.mjs']);assert.equal(before['../../entry.mjs'],after['../../entry.mjs']);assert.notEqual(before['../../leaf.mjs'],after['../../leaf.mjs']);
});
test('multiline imports, side effects and cycles are included once',async t=>{
 const root=await fixture(t);await writeFile(join(root,'entry.mjs'),"import {\n x\n} from './leaf.mjs';\nimport './side.mjs';");await writeFile(join(root,'leaf.mjs'),"import './entry.mjs'; export const x=1;");await writeFile(join(root,'side.mjs'),'');const hashes=await hashHarness(root,['entry.mjs','leaf.mjs']);assert.equal(Object.keys(hashes).length,3);
});
test('missing imported dependency fails rather than making a partial manifest',async t=>{
 const root=await fixture(t);await writeFile(join(root,'entry.mjs'),"import './missing.mjs';");await assert.rejects(hashHarness(root,['entry.mjs']),/ENOENT/);
});
test('import outside repository is rejected',async t=>{
 const root=await fixture(t);const outside=await mkdtemp(join(tmpdir(),'nano-harness-outside-'));t.after(()=>rm(outside,{recursive:true,force:true}));await writeFile(join(outside,'leaf.mjs'),'');await writeFile(join(root,'entry.mjs'),`import '${'../'+outside.split('/').pop()+'/leaf.mjs'}';`);await assert.rejects(hashHarness(root,['entry.mjs']),/escapes/);
});
test('symlink to external source is rejected',async t=>{
 const root=await fixture(t);const outside=await mkdtemp(join(tmpdir(),'nano-harness-link-'));t.after(()=>rm(outside,{recursive:true,force:true}));await writeFile(join(outside,'leaf.mjs'),'');await symlink(join(outside,'leaf.mjs'),join(root,'leaf.mjs'));await writeFile(join(root,'entry.mjs'),"import './leaf.mjs';");await assert.rejects(hashHarness(root,['entry.mjs']),/escapes/);
});
