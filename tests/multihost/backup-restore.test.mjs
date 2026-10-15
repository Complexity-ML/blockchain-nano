import test from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {mkdtempSync,mkdirSync,writeFileSync,readFileSync,readdirSync,lstatSync,symlinkSync,linkSync,chmodSync,rmSync,existsSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
const script=fileURLToPath(new URL('./backup-restore.sh',import.meta.url));
const linux={skip:process.platform!=='linux',timeout:15000};
const hash=b=>createHash('sha256').update(b).digest('hex');
function file(path,bytes){mkdirSync(dirname(path),{recursive:true});writeFileSync(path,bytes);}
function inventory(root){
 const out=[];
 function walk(path,relative){
  const s=lstatSync(path,{bigint:true});
  const row={path:relative,material:[s.dev,s.ino,s.mode,s.size,s.mtimeNs,s.ctimeNs,s.uid,s.gid,s.nlink].map(String)};
  if(s.isFile())row.sha256=hash(readFileSync(path));
  if(s.isSymbolicLink())row.link=true;
  out.push(row);
  if(s.isDirectory())for(const name of readdirSync(path).sort())walk(join(path,name),relative+'/'+name);
 }
 walk(root,'.');return out;
}
function fixture(t){
 const base=mkdtempSync(join(tmpdir(),'nano-two-root-backup-'));
 t.after(()=>rmSync(base,{recursive:true,force:true}));
 const f={base,data:join(base,'data'),witness:join(base,'witness'),output:join(base,'out'),genesis:join(base,'genesis.json'),request:join(base,'request.json'),nano:join(base,'nano-audit-stub'),stop:join(base,'stop.json'),called:join(base,'called')};
 for(const [path,bytes] of [
  [join(f.data,'chain/CURRENT'),'MANIFEST-000001\n'],[join(f.data,'chain/MANIFEST-000001'),'opaque core bytes'],
  [join(f.data,'journal-64/CURRENT'),'journal-primary\n'],[join(f.data,'journal-64/000001.log'),'opaque journal bytes'],[join(f.data,'peer.key'),'public synthetic peer bytes'],
  [join(f.witness,'chain-witness/CURRENT'),'witness-core\n'],[join(f.witness,'witness-64/CURRENT'),'witness-signing\n'],[join(f.witness,'witness-64/000001.log'),'opaque witness bytes'],
  [f.genesis,'{"synthetic":true}\n'],[f.request,'{"expectedCheckpoint":{"sequence":"0x1","digest":"0x'+'a'.repeat(64)+'"}}\n'],
  [f.stop,'{"stopNonce":"public-fixture-nonce","receipt":{"event":"capella-runtime-stopped"}}\n']
 ])file(path,bytes);
 file(f.nano,`#!/usr/bin/env bash
set -euo pipefail
test "$1" = audit-capella
test -f "$3/CURRENT"
if [ -n "\${AUDIT_EXPECT_WITNESS-}" ]; then test -f "$AUDIT_EXPECT_WITNESS/chain-witness/CURRENT"; test -f "$AUDIT_EXPECT_WITNESS/witness-64/000001.log"; fi
printf 'called\\n' > "$AUDIT_CALLED"
if [ -n "\${AUDIT_REPLACE_STOP-}" ]; then printf 'substituted stop nonce\\n' > "$AUDIT_REPLACE_STOP"; fi
if [ -n "\${AUDIT_MUTATE_COPY-}" ]; then printf 'mutation\\n' > "$AUDIT_MUTATE_COPY"; fi
printf '{"canonicalHistoryVerified":true,"testStub":true}\\n'
`);
 // The shell audit command is a test double, not a native checkpoint proof.
 chmodSync(f.nano,0o700);
 f.stopSha=hash(readFileSync(f.stop));return f;
}
function run(f,extra={}){
 return spawnSync('bash',[script,'--two-roots',f.data,f.witness,f.output,f.genesis,f.request,f.nano,f.stop,f.stopSha],{encoding:'utf8',timeout:10000,env:{...process.env,AUDIT_CALLED:f.called,AUDIT_EXPECT_WITNESS:join(f.output,'restored-witness'),...extra}});
}
function refusedUnchanged(f,invoke=()=>run(f)){
 const before=[inventory(f.data),inventory(f.witness)];
 const result=invoke();assert.equal(result.error,undefined);assert.notEqual(result.status,0,result.stdout+result.stderr);
 assert.equal(existsSync(f.called),false,'audit must not run after an admission refusal');
 assert.deepEqual([inventory(f.data),inventory(f.witness)],before);return result;
}
test('two independent roots restore all journals/witnesses and audit only the new chain',linux,t=>{
 const f=fixture(t);chmodSync(f.data,0o755);chmodSync(f.witness,0o755);
 const before=[inventory(f.data),inventory(f.witness)],result=run(f);
 assert.equal(result.status,0,result.stdout+result.stderr);assert.equal(existsSync(f.called),true);
 assert.deepEqual([inventory(f.data),inventory(f.witness)],before);
 for(const role of ['data','witness']){
  assert.deepEqual(readFileSync(join(f.output,role+'-source-before.sha256')),readFileSync(join(f.output,role+'-restored-before.sha256')));
  assert.deepEqual(readFileSync(join(f.output,role+'-source-before.stat')),readFileSync(join(f.output,role+'-source-after.stat')));
  assert.deepEqual(readFileSync(join(f.output,role+'-restored-before.stat')),readFileSync(join(f.output,role+'-restored-after.stat')));
  assert.ok(existsSync(join(f.output,role+'.tar.gz')));
 }
 assert.deepEqual(readFileSync(join(f.output,'stopped-receipt.json')),readFileSync(f.stop));
 assert.match(readFileSync(join(f.output,'passed.txt'),'utf8'),/signer\/witness authentication and resumed duties require their own checks/);
 const original=lstatSync(join(f.data,'chain/CURRENT'),{bigint:true}),copy=lstatSync(join(f.output,'restored-data/chain/CURRENT'),{bigint:true});
 assert.notDeepEqual([original.dev,original.ino],[copy.dev,copy.ino]);
});
for(const [name,alter] of [
 ['same roots',f=>{f.witness=f.data;}],
 ['nested witness',f=>{f.witness=join(f.data,'nested');file(join(f.witness,'chain-witness/CURRENT'),'nested');}],
 ['nested data',f=>{f.data=join(f.witness,'nested');file(join(f.data,'chain/CURRENT'),'nested');file(join(f.data,'peer.key'),'peer');}],
 ['root symlink',f=>{const alias=join(f.base,'data-link');symlinkSync(f.data,alias);f.data=alias;}],
 ['descendant symlink',f=>symlinkSync(f.request,join(f.witness,'unexpected-link'))],
 ['hard link between data and witness',f=>linkSync(join(f.data,'chain/CURRENT'),join(f.witness,'shared-current'))],
 ['hard link to external file',f=>linkSync(join(f.witness,'witness-64/000001.log'),join(f.base,'external-log'))],
 ['output inside data',f=>{f.output=join(f.data,'backup');}],
 ['output ancestor of witness',f=>{f.output=f.base;}],
 ['linked output parent',f=>{const alias=join(f.base,'alias');symlinkSync(f.witness,alias);f.output=join(alias,'new-output');}],
 ['receipt inside archived witness',f=>{f.stop=join(f.witness,'stop.json');file(f.stop,'independent receipt misplaced');f.stopSha=hash(readFileSync(f.stop));}],
 ['wrong external stop SHA',f=>{f.stopSha='f'.repeat(64);}],
 ['noncanonical external stop SHA',f=>{f.stopSha='0x'+f.stopSha;}]
])test('two-root admission refuses '+name+' before audit and preserves sources',linux,t=>{
 const f=fixture(t);alter(f);refusedUnchanged(f);
});
test('special FIFO is refused without opening or blocking',linux,t=>{
 const f=fixture(t);const made=spawnSync('mkfifo',[join(f.data,'chain/unexpected-fifo')]);assert.equal(made.status,0);
 refusedUnchanged(f);
});
test('a substituted stopped receipt/nonce after copying is rejected without changing sources',linux,t=>{
 const f=fixture(t),before=[inventory(f.data),inventory(f.witness)];
 const result=run(f,{AUDIT_REPLACE_STOP:f.stop});assert.notEqual(result.status,0);assert.equal(existsSync(f.called),true);
 assert.match(result.stderr,/stopped-receipt mismatch/);assert.equal(existsSync(join(f.output,'passed.txt')),false);
 assert.deepEqual([inventory(f.data),inventory(f.witness)],before);
});
test('an audit-side mutation of the restored witness is rejected without changing originals',linux,t=>{
 const f=fixture(t),before=[inventory(f.data),inventory(f.witness)];
 const result=run(f,{AUDIT_MUTATE_COPY:join(f.output,'restored-witness/witness-64/000001.log')});
 assert.notEqual(result.status,0);assert.equal(existsSync(f.called),true);assert.equal(existsSync(join(f.output,'passed.txt')),false);
 assert.deepEqual([inventory(f.data),inventory(f.witness)],before);
});
test('legacy five-argument single-root layout and result filenames remain supported',linux,t=>{
 const f=fixture(t);file(join(f.data,'core-witness/CURRENT'),'legacy-witness');
 const before=inventory(f.data),result=spawnSync('bash',[script,f.data,f.output,f.genesis,f.request,f.nano],{encoding:'utf8',timeout:10000,env:{...process.env,AUDIT_CALLED:f.called}});
 assert.equal(result.status,0,result.stdout+result.stderr);assert.deepEqual(inventory(f.data),before);
 for(const name of ['data.tar.gz','restored/core-witness/CURRENT','source-before.sha256','source-after.sha256','restored-before.sha256','restored-after.sha256','audit.json','passed.txt'])assert.equal(existsSync(join(f.output,name)),true,name);
});

test('two-root ownership option rejects any unsupported owner before audit',linux,t=>{
 const f=fixture(t);
 refusedUnchanged(f,()=>spawnSync('bash',[script,'--two-roots',f.data,f.witness,f.output,f.genesis,f.request,f.nano,f.stop,f.stopSha,'0:0'],{encoding:'utf8',timeout:10000,env:{...process.env,AUDIT_CALLED:f.called}}));
});
