import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,readFile,writeFile,appendFile,copyFile,lstat,readdir,realpath,symlink,rename,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {resolve,join,dirname} from 'node:path';
import {createHash} from 'node:crypto';
import {packageArtifacts,packageCandidate} from './package-candidate.mjs';
const REPO=resolve('.'),BASE='sha256:'+'1'.repeat(64),IMAGE='sha256:'+'2'.repeat(64),REV='a'.repeat(40);
const sha=b=>createHash('sha256').update(b).digest('hex'),nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
const noticePaths={'LICENSE.ethereumjs':'third_party/opcodes/LICENSE.ethereumjs','LICENSE.evm-codes':'third_party/opcodes/LICENSE.evm-codes','bip39-LICENSE':'third_party/bip39/LICENSE','blst-LICENSE':'third_party/ckzg/blst/LICENSE','ckzg-LICENSE':'third_party/ckzg/LICENSE','json-COPYRIGHTS':'third_party/notices/json-COPYRIGHTS','json-LICENSE.MIT':'third_party/notices/json-LICENSE.MIT','libp2p-LICENSE-APACHE':'third_party/notices/libp2p-LICENSE-APACHE','libp2p-LICENSE-MIT':'third_party/notices/libp2p-LICENSE-MIT','mcl-COPYRIGHT':'third_party/mcl/COPYRIGHT','secp256k1-LICENSE':'third_party/secp256k1/COPYING'};
async function absent(path){await assert.rejects(lstat(path),{code:'ENOENT'});}
async function fixture(action){
 const root=await mkdtemp(join(await realpath(tmpdir()),'nano-package-test-')),repoRoot=join(root,'repo'),temporaryRoot=join(root,'contexts');let sentinel;
 try{
  await mkdir(temporaryRoot,{mode:0o700});const keep=join(temporaryRoot,'keep');await writeFile(keep,'pre-existing unrelated file');sentinel={path:keep,row:nine(await lstat(keep,{bigint:true})),sha256:sha(await readFile(keep))};
  for(const rel of ['deploy/docker/candidate.Dockerfile','deploy/docker/notices.sha256',...Object.values(noticePaths)]){await mkdir(dirname(join(repoRoot,rel)),{recursive:true});await copyFile(join(REPO,rel),join(repoRoot,rel));}
  for(const name of ['one','two','proof'])await mkdir(join(root,name));
  const bodies={nano:Buffer.from('public synthetic nano executable'),'nano-p2p':Buffer.from('public synthetic p2p executable')},source=Buffer.from('{"opaquePublicEvidence":true}\n');
  const input={format:'nano-candidate-artifacts-v1',runtimeBase:BASE,architecture:'arm64',sourceRevision:REV,sourceEvidence:{file:join(root,'proof/source.json'),sha256:sha(source)},executables:{nano:{file:join(root,'one/nano'),sha256:sha(bodies.nano),bytes:bodies.nano.length},'nano-p2p':{file:join(root,'two/nano-p2p'),sha256:sha(bodies['nano-p2p']),bytes:bodies['nano-p2p'].length}},maximumArtifactBytes:4096};
  for(const [name,want] of Object.entries(input.executables))await writeFile(want.file,bodies[name]);await writeFile(input.sourceEvidence.file,source);
  const expected=structuredClone(input),state={calls:[],context:null,mode:null,hook:null},originals=[...Object.values(input.executables).map(e=>e.file),input.sourceEvidence.file];
  const before=await Promise.all(originals.map(async file=>({file,row:nine(await lstat(file,{bigint:true})),sha256:sha(await readFile(file))})));
  const runDocker=async argv=>{
   state.calls.push([...argv]);await state.hook?.(argv);
   if(argv[0]==='info')return 'linux aarch64\n';
   if(argv[0]==='image'&&argv[1]==='ls')return state.mode==='foreign-tag'?'sha256:'+'3'.repeat(64)+'\n':'';
   if(argv[0]==='image'&&argv[1]==='inspect')return JSON.stringify(argv[2]===IMAGE?{Id:IMAGE,Os:'linux',Architecture:'arm64',Config:{User:state.mode==='root-user'?'0:0':'10001:10001',Labels:{'org.opencontainers.image.revision':REV,'org.nanobees.runtime-base-id':BASE}}}:{Id:BASE,Os:'linux',Architecture:'arm64'});
   if(argv[0]==='tag')return '';
   if(argv[0]==='build'){
    state.context=argv.at(-1);assert.equal(argv[argv.indexOf('--network')+1],'none');assert.ok(argv.includes('--pull=false'));assert.equal(argv[argv.indexOf('--build-arg')+1],'NANO_RUNTIME_BASE=nano-candidate-base:'+BASE.slice(7));
    for(const name of ['nano','nano-p2p']){assert.deepEqual(await readFile(join(state.context,name)),bodies[name]);assert.equal((await lstat(join(state.context,name))).mode&0o777,0o755);}
    assert.deepEqual(await readFile(join(state.context,'source-evidence')),source);assert.equal((await lstat(join(state.context,'notices/notices.sha256'))).mode&0o777,0o644);
    assert.equal((await readdir(join(state.context,'notices'))).length,12);assert.ok(argv.includes('SOURCE_REVISION='+REV));
    await writeFile(argv[argv.indexOf('--iidfile')+1],IMAGE+'\n',{flag:'wx'});return '';
   }
   if(argv[0]==='run'&&argv.includes('/usr/bin/sha256sum'))return Object.entries(expected.executables).map(([name,e])=>e.sha256+'  /usr/local/bin/'+name).join('\n')+'\n';
   if(argv[0]==='run'&&argv.includes('/usr/bin/ldd'))return state.mode==='missing-library'?'libmissing.so => not found\n':'libc.so.6 => /lib/libc.so.6\n';
   throw Error('Unexpected Docker command');
  };
  const options={runDocker,repoRoot,temporaryRoot},raw=()=>Buffer.from(JSON.stringify(input)+'\n');
  const invoke=async extra=>{const manifestBytes=raw();return packageArtifacts(input,{...options,manifestBytes,manifestSha256:sha(manifestBytes),...extra});};
  const unchanged=async()=>{for(const old of before){assert.deepEqual(nine(await lstat(old.file,{bigint:true})),old.row);assert.equal(sha(await readFile(old.file)),old.sha256);}};
  await action({root,input,expected,state,options,raw,invoke,unchanged,runDocker});
 }finally{try{if(sentinel){assert.deepEqual(nine(await lstat(sentinel.path,{bigint:true})),sentinel.row);assert.equal(sha(await readFile(sentinel.path)),sentinel.sha256);}}finally{await rm(root,{recursive:true,force:true});}}
}
test('split artifacts snapshot caller intent and retain original pins under async caller mutation',async()=>fixture(async f=>{
 const b=f.raw(),known=sha(b);f.state.hook=async a=>{if(a[0]==='info'){f.input.sourceRevision='b'.repeat(40);f.input.executables.nano.file='/never-read';b.fill(0);}};
 const r=await packageArtifacts(f.input,{...f.options,manifestBytes:b,manifestSha256:known});assert.equal(r.image,IMAGE);assert.equal(r.noticeCount,11);assert.equal(r.productionQualified,false);assert.equal(r.sourceProvenanceSemanticallyVerified,false);assert.equal(r.temporaryContextRemoved,true);assert.deepEqual(r.executables,f.expected.executables);assert.equal(r.sourceRevision,REV);assert.equal(r.manifestSha256,known);await f.unchanged();await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
}));
test('the independent raw manifest pin binds the exact input object before Docker',async()=>fixture(async f=>{
 const b=f.raw();await assert.rejects(packageArtifacts(f.input,{...f.options,manifestBytes:b,manifestSha256:'0'.repeat(64)}),/Descriptor pin\/object mismatch/);const altered=structuredClone(f.input);altered.sourceRevision='b'.repeat(40);await assert.rejects(packageArtifacts(altered,{...f.options,manifestBytes:b,manifestSha256:sha(b)}),/Descriptor pin\/object mismatch/);assert.equal(f.state.calls.length,0);
}));
test('a symlink ancestor cannot substitute an executable',async()=>fixture(async f=>{
 const p=join(f.root,'alias');await symlink(join(f.root,'one'),p);f.input.executables.nano.file=join(p,'nano');await assert.rejects(f.invoke(),/physical/);assert.equal(f.state.calls.some(a=>a[0]==='build'||a[0]==='tag'),false);
}));
test('a wrong binary pin refuses before build and preserves every original',async()=>fixture(async f=>{
 f.input.executables.nano.sha256='f'.repeat(64);await assert.rejects(f.invoke(),/Artifact pin mismatch/);assert.equal(f.state.calls.some(a=>a[0]==='build'||a[0]==='tag'),false);await f.unchanged();
}));
test('source growth after copying refuses and removes only the new context',async()=>fixture(async f=>{
 f.state.hook=async a=>{if(a[0]==='build')await appendFile(f.input.executables.nano.file,'changed');};await assert.rejects(f.invoke(),/Artifact (identity|hash) changed/);await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
}));
test('same-byte inode replacement after copying cannot satisfy the original nine-field fence',async()=>fixture(async f=>{
 f.state.hook=async a=>{if(a[0]==='build'){const p=f.input.executables.nano.file;await rename(p,p+'.old');await copyFile(p+'.old',p);}};await assert.rejects(f.invoke(),/Artifact identity changed/);await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
}));
test('an unresolved final-image library fails with original sources unchanged and context removed',async()=>fixture(async f=>{
 f.state.mode='missing-library';await assert.rejects(f.invoke(),/Unresolved executable libraries/);await f.unchanged();await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
}));
test('default temporary root creates and removes a private owned parent',async()=>fixture(async f=>{
 const {temporaryRoot,...options}=f.options,b=f.raw();await packageArtifacts(f.input,{...options,manifestBytes:b,manifestSha256:sha(b)});await absent(f.state.context);await absent(dirname(f.state.context));await f.unchanged();
}));
test('the file API pins its manifest and rejects post-read mutation',async()=>fixture(async f=>{
 const path=join(f.root,'artifacts.json'),b=f.raw();await writeFile(path,b);const r=await packageCandidate(path,sha(b),f.options);assert.equal(r.image,IMAGE);
 f.state.context=null;f.state.hook=async a=>{if(a[0]==='build')await appendFile(path,' ');};await assert.rejects(packageCandidate(path,sha(b),f.options),/Artifact (identity|hash) changed/);await absent(f.state.context);await f.unchanged();
}));


test('new IID output settles metadata before its first strict identity admission',async()=>fixture(async f=>{
 const {chmod}=await import('node:fs/promises');let timer,mutation=null,changed=false,mutationError;
 f.state.hook=async a=>{if(a[0]==='build'){const iid=a[a.indexOf('--iidfile')+1];timer=setTimeout(()=>{mutation=chmod(iid,0o400).then(()=>{changed=true;},e=>{mutationError=e;});},500);}};
 try{
  const r=await f.invoke();if(mutation)await mutation;assert.equal(mutationError,undefined);assert.equal(changed,true);
  assert.ok(r.iidFirstAdmission.stableSamples>=3);assert.ok(r.iidFirstAdmission.stableSpanMs>=2000);assert.ok(r.iidFirstAdmission.metadataChanges>=1,'the fixture must actually change metadata during settling');
  assert.equal(r.iidFirstAdmission.sha256,sha(Buffer.from(IMAGE+'\n')));assert.equal(r.iidFirstAdmission.bytes,Buffer.byteLength(IMAGE+'\n'));await f.unchanged();await absent(f.state.context);
 }finally{clearTimeout(timer);if(mutation)await mutation;}
}));
