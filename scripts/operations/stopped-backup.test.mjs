import test from 'node:test';
import assert from 'node:assert/strict';
import * as fs from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {createBackup,verifyBackup,restoreBackup,validateCreate} from './stopped-backup.mjs';
const hash=b=>createHash('sha256').update(b).digest('hex');
const exists=async p=>fs.lstat(p).then(()=>true,e=>{if(e.code==='ENOENT')return false;throw e;});
async function put(p,b){await fs.mkdir(dirname(p),{recursive:true,mode:0o700});await fs.writeFile(p,b,{mode:0o600,flag:'wx'});}
async function fixture(t){
 const base=await fs.realpath(await fs.mkdtemp(join(tmpdir(),'nano-stopped-backup-test-')));t.after(()=>fs.rm(base,{recursive:true,force:true}));
 const roots={data:join(base,'data'),witness:join(base,'witness')};for(const p of Object.values(roots))await fs.mkdir(p,{mode:0o700});
 await put(join(roots.data,'chain/CURRENT'),'MANIFEST-000001\n');await put(join(roots.data,'chain/MANIFEST-000001'),'PUBLIC synthetic database bytes');await put(join(roots.data,'chain/LOCK'),'');
 await put(join(roots.data,'journal-0/checkpoint'),'PUBLIC signer record, not a private key');await put(join(roots.data,'peer.key'),'PUBLIC TEST IDENTITY ONLY');await fs.mkdir(join(roots.data,'empty-directory'),{mode:0o700});
 await put(join(roots.witness,'chain-witness/CURRENT'),'MANIFEST-000002\n');await put(join(roots.witness,'witness-0/checkpoint'),'PUBLIC independent witness record');
 const stop=Buffer.from('{"fixture":"PUBLIC stopped assertion; not measured quiescence"}\n'),commitments=Buffer.from('{"fixture":"PUBLIC independent commitments; no signing authority"}\n');
 await put(join(base,'stop.json'),stop);await put(join(base,'commitments.json'),commitments);
 const c={format:'nano-stopped-backup-create-v1',roots,destination:join(base,'backup'),proofs:{stop:{path:join(base,'stop.json'),sha256:hash(stop),bytes:stop.length},commitments:{path:join(base,'commitments.json'),sha256:hash(commitments),bytes:commitments.length}},limits:{maximumEntries:100,maximumFileBytes:1048576,maximumTotalBytes:8388608,maximumElapsedMs:30000},preconditions:{allWritersStopped:true,sourcesMountedReadOnly:true}};
 return {base,c};
}
async function snapshot(roots){
 const rows={};for(const root of Object.values(roots)){
  async function walk(p){const s=await fs.lstat(p,{bigint:true});rows[p]=Object.fromEntries(['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>[k,String(s[k])]));if(s.isFile())rows[p].sha256=hash(await fs.readFile(p));else for(const n of (await fs.readdir(p)).sort())await walk(join(p,n));}
  await walk(root);
 }return rows;
}
test('create, verify and new-only restore preserve two full roots, empty files and source nine',async t=>{
 const {base,c}=await fixture(t),before=await snapshot(c.roots),made=await createBackup(c);assert.equal(made.passed,true);assert.equal(made.signingAuthorized,false);assert.equal(made.cppSemanticValidationRequired,true);
 assert.equal((await verifyBackup(c.destination,made.manifest.sha256)).passed,true);const restored=join(base,'restored'),r=await restoreBackup(c.destination,made.manifest.sha256,restored);assert.equal(r.passed,true);assert.equal(r.independentLatestCommitmentsRequiredBeforeSigning,true);
 for(const [role,root] of Object.entries(c.roots)){
  async function compare(p,rel=''){const s=await fs.lstat(p);if(s.isFile())assert.deepEqual(await fs.readFile(p),await fs.readFile(join(restored,role,rel)));else{assert.deepEqual((await fs.readdir(p)).sort(),(await fs.readdir(join(restored,role,rel))).sort());for(const n of await fs.readdir(p))await compare(join(p,n),rel?rel+'/'+n:n);}}
  await compare(root);
 }assert.deepEqual(await snapshot(c.roots),before);assert.equal((await fs.stat(join(restored,'data/chain/LOCK'))).size,0);
});
test('create snapshots caller input before asynchronous checkpoints',async t=>{
 const {c}=await fixture(t),original=structuredClone(c),before=await snapshot(original.roots);let mutated=false;
 const made=await createBackup(c,{checkpoint:async name=>{if(name==='new-backup-root'){mutated=true;c.preconditions.sourcesMountedReadOnly=false;}}});
 assert.equal(mutated,true);assert.equal(c.preconditions.sourcesMountedReadOnly,false);assert.equal(made.passed,true);
 const manifest=JSON.parse(await fs.readFile(join(original.destination,'manifest.json'),'utf8'));assert.deepEqual(manifest.operatorAssertions,original.preconditions);
 assert.equal((await verifyBackup(original.destination,made.manifest.sha256)).passed,true);assert.deepEqual(await snapshot(original.roots),before);
});
test('stop/read-only assertions and budgets are mandatory; root overlap refuses before destination',async t=>{
 const {c}=await fixture(t);for(const key of ['allWritersStopped','sourcesMountedReadOnly']){const bad=structuredClone(c);bad.preconditions[key]=false;assert.throws(()=>validateCreate(bad));}
 const bad=structuredClone(c);bad.limits.maximumEntries=0;assert.throws(()=>validateCreate(bad));const overlap=structuredClone(c);overlap.roots.witness=overlap.roots.data;await assert.rejects(createBackup(overlap));assert.equal(await exists(c.destination),false);
});
test('symlinks, hardlinks across roots and linked input roots are refused',async t=>{
 for(const kind of ['leaf-symlink','cross-root-hardlink','root-symlink'])await t.test(kind,async st=>{
  const {base,c}=await fixture(st);if(kind==='leaf-symlink')await fs.symlink(join(c.roots.data,'peer.key'),join(c.roots.witness,'link'));
  else if(kind==='cross-root-hardlink')await fs.link(join(c.roots.data,'peer.key'),join(c.roots.witness,'same-inode'));
  else{const alias=join(base,'data-alias');await fs.symlink(c.roots.data,alias);c.roots.data=alias;}
  await assert.rejects(createBackup(c));assert.equal(await exists(c.destination),false);
 });
});
test('external manifest mismatch and old restore destination do not touch old files',async t=>{
 const {base,c}=await fixture(t),made=await createBackup(c),old=join(base,'old');await put(join(old,'sentinel'),'must remain');const before=await fs.readFile(join(old,'sentinel'));
 await assert.rejects(restoreBackup(c.destination,'0'.repeat(64),join(base,'wrong-pin')));assert.equal(await exists(join(base,'wrong-pin')),false);
 await assert.rejects(restoreBackup(c.destination,made.manifest.sha256,old),{code:'EEXIST'});assert.deepEqual(await fs.readFile(join(old,'sentinel')),before);assert.deepEqual(await fs.readdir(old),['sentinel']);
});
test('archive corruption and truncation refuse before making restore namespace',async t=>{
 for(const kind of ['corrupt','truncate'])await t.test(kind,async st=>{
  const {base,c}=await fixture(st),made=await createBackup(c),path=join(c.destination,'data.nbak');const raw=await fs.readFile(path);
  if(kind==='corrupt'){raw[raw.length-1]^=1;await fs.writeFile(path,raw);}else await fs.truncate(path,raw.length-1);
  await assert.rejects(restoreBackup(c.destination,made.manifest.sha256,join(base,'restored')));assert.equal(await exists(join(base,'restored')),false);
 });
});
test('even an externally pinned malformed manifest cannot traverse or replace parents',async t=>{
 for(const badPath of ['../escape','/absolute','chain/../../escape','chain\\escape'])await t.test(badPath,async st=>{
  const {base,c}=await fixture(st);await createBackup(c);const path=join(c.destination,'manifest.json'),manifest=JSON.parse(await fs.readFile(path));manifest.roots.data.entries.find(e=>e.kind==='file').path=badPath;const bytes=Buffer.from(JSON.stringify(manifest)+'\n');await fs.writeFile(path,bytes);
  await assert.rejects(restoreBackup(c.destination,hash(bytes),join(base,'restored')));assert.equal(await exists(join(base,'restored')),false);assert.equal(await exists(join(base,'escape')),false);
 });
});
test('source mutation after inventory is a refusal, with no completion marker',async t=>{
 const {c}=await fixture(t);await assert.rejects(createBackup(c,{checkpoint:async name=>{if(name==='new-backup-root')await fs.writeFile(join(c.roots.data,'chain/CURRENT'),'MUTATED-SOURCE\n');}}));assert.equal(await exists(join(c.destination,'manifest.json')),false);
});
test('process crash before publication leaves an incomplete package, never a committed backup',async t=>{
 const {base,c}=await fixture(t),control=join(base,'control.json');await put(control,JSON.stringify(c));const before=await snapshot(c.roots);
 const code=`import {readFile} from 'node:fs/promises';import {createBackup} from ${JSON.stringify(new URL('./stopped-backup.mjs',import.meta.url).href)};const c=JSON.parse(await readFile(process.argv[1]));await createBackup(c,{checkpoint:async n=>{if(n==='before-publication')process.exit(73);}});`;
 const child=spawnSync(process.execPath,['--input-type=module','-e',code,control],{encoding:'utf8',timeout:15000,maxBuffer:1048576,env:{PATH:'/usr/bin:/bin',HOME:base,LANG:'C',LC_ALL:'C'}});assert.equal(child.status,73,child.stderr);assert.equal(await exists(join(c.destination,'manifest.json')),false);
 await assert.rejects(verifyBackup(c.destination,'0'.repeat(64)));assert.deepEqual(await snapshot(c.roots),before);
});
test('unsupported volatile filesystems and unsupported fsync never issue a successful pin',async t=>{
 const {c}=await fixture(t);await assert.rejects(createBackup(c,{filesystem:async()=>({type:0x01021994n})}),{code:'unsupported-volatile-filesystem'});assert.equal(await exists(c.destination),false);
 await assert.rejects(createBackup(c,{sync:async()=>{throw Object.assign(Error('unsupported'),{code:'ENOTSUP'});}}),{code:'ENOTSUP'});assert.equal(await exists(join(c.destination,'manifest.json')),false);
});
test('directory fsync failure preserves the original error and the source inventory',async t=>{
 const {c}=await fixture(t),before=await snapshot(c.roots);await assert.rejects(createBackup(c,{sync:async(h,kind)=>{if(kind==='directory')throw Object.assign(Error('directory-sync-failed'),{code:'EIO'});await h.sync();}}),{code:'EIO'});assert.deepEqual(await snapshot(c.roots),before);assert.equal(await exists(join(c.destination,'manifest.json')),false);
});
test('a concurrent final manifest is never replaced by publication',async t=>{
 const {c}=await fixture(t);await assert.rejects(createBackup(c,{checkpoint:async(name,{root})=>{if(name==='before-publication')await fs.writeFile(join(root,'manifest.json'),'foreign commit',{flag:'wx'});}}),{code:'EEXIST'});assert.equal(await fs.readFile(join(c.destination,'manifest.json'),'utf8'),'foreign commit');
});
test('failure after link is explicitly uncertain, not a successful create receipt',async t=>{
 const {c}=await fixture(t);let success=false;await assert.rejects(createBackup(c,{checkpoint:async name=>{if(name==='after-publication-before-sync')throw Object.assign(Error('uncertain-directory-sync'),{code:'EIO'});}}).then(()=>{success=true;}),{code:'EIO'});assert.equal(success,false);assert.equal(await exists(join(c.destination,'manifest.json')),true);
});
test('restored bytes are read back before publishing completion',async t=>{
 const {base,c}=await fixture(t),made=await createBackup(c),restored=join(base,'restored');let corrupted=false;
 await assert.rejects(restoreBackup(c.destination,made.manifest.sha256,restored,{sync:async(h,kind,path)=>{await h.sync();if(!corrupted&&kind==='file'&&path===join(restored,'data/chain/CURRENT')){corrupted=true;await fs.writeFile(path,'BAD-READBACK\n');}}}),{code:'restored-content-readback'});
 assert.equal(corrupted,true);assert.equal(await exists(join(restored,'manifest.json')),false);assert.equal((await verifyBackup(c.destination,made.manifest.sha256)).passed,true);
});
test('a first operation refusal survives a failing final source fence',async t=>{
 const {c}=await fixture(t),original=Object.assign(Error('first failure'),{code:'first-failure'});await assert.rejects(createBackup(c,{checkpoint:async name=>{if(name==='new-backup-root'){await fs.writeFile(join(c.roots.data,'peer.key'),'mutation');throw original;}}}),e=>e===original);assert.equal(await exists(join(c.destination,'manifest.json')),false);
});
test('verify and restore share one elapsed-time budget without resetting at phase boundaries',async t=>{
 const {base,c}=await fixture(t),made=await createBackup(c);let now=0;
 await assert.rejects(restoreBackup(c.destination,made.manifest.sha256,join(base,'restored'),{clock:()=>now,checkpoint:async name=>{if(name==='backup-verified'||name==='new-restore-root')now+=20000;}}),{code:'deadline'});assert.equal(await exists(join(base,'restored/manifest.json')),false);
});
test('directory enumeration refuses its entry budget without archiving an oversized namespace',async t=>{
 const {c}=await fixture(t);c.limits.maximumEntries=2;await assert.rejects(createBackup(c),{code:'entry-budget'});assert.equal(await exists(c.destination),false);
});
