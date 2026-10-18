import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,readFile,writeFile,appendFile,copyFile,lstat,readdir,realpath,symlink,rename,rm,chmod,link} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {resolve,join,dirname} from 'node:path';
import {createHash} from 'node:crypto';
import {packageArtifacts,packageCandidate,validateArtifacts} from './package-candidate.mjs';
const REPO=resolve('.'),BASE='sha256:'+'1'.repeat(64),IMAGE='sha256:'+'2'.repeat(64),REV='a'.repeat(40);
const sha=b=>createHash('sha256').update(b).digest('hex'),nine=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
const LEGACY=['nano','nano-p2p'],COMPLETE=[...LEGACY,'nano-storage-maintenance','nano-bootstrap-archive-maintenance','nano-capella-duty-readiness'];
const HASH_ARGS={nano:'NANO_SHA256','nano-p2p':'P2P_SHA256','nano-storage-maintenance':'STORAGE_MAINTENANCE_SHA256','nano-bootstrap-archive-maintenance':'BOOTSTRAP_ARCHIVE_MAINTENANCE_SHA256','nano-capella-duty-readiness':'CAPELLA_DUTY_READINESS_SHA256'};
const noticePaths={'LICENSE.ethereumjs':'third_party/opcodes/LICENSE.ethereumjs','LICENSE.evm-codes':'third_party/opcodes/LICENSE.evm-codes','bip39-LICENSE':'third_party/bip39/LICENSE','blst-LICENSE':'third_party/ckzg/blst/LICENSE','ckzg-LICENSE':'third_party/ckzg/LICENSE','json-COPYRIGHTS':'third_party/notices/json-COPYRIGHTS','json-LICENSE.MIT':'third_party/notices/json-LICENSE.MIT','libp2p-LICENSE-APACHE':'third_party/notices/libp2p-LICENSE-APACHE','libp2p-LICENSE-MIT':'third_party/notices/libp2p-LICENSE-MIT','mcl-COPYRIGHT':'third_party/mcl/COPYRIGHT','secp256k1-LICENSE':'third_party/secp256k1/COPYING'};
async function absent(path){await assert.rejects(lstat(path),{code:'ENOENT'});}
async function fixture(action,{complete=false,sourceMode=0o644}={}){
 const root=await mkdtemp(join(await realpath(tmpdir()),'nano-package-test-')),repoRoot=join(root,'repo'),temporaryRoot=join(root,'contexts');let sentinel;
 try{
  await mkdir(temporaryRoot,{mode:0o700});const keep=join(temporaryRoot,'keep');await writeFile(keep,'pre-existing unrelated file');sentinel={path:keep,row:nine(await lstat(keep,{bigint:true})),sha256:sha(await readFile(keep))};
  for(const rel of ['deploy/docker/candidate.Dockerfile','deploy/docker/notices.sha256',...Object.values(noticePaths)]){await mkdir(dirname(join(repoRoot,rel)),{recursive:true});await copyFile(join(REPO,rel),join(repoRoot,rel));}
  const roles=complete?COMPLETE:LEGACY,folders=['one','two','three','four','five'];for(const name of [...folders.slice(0,roles.length),'proof'])await mkdir(join(root,name));
  const bodies={nano:Buffer.from('public synthetic nano executable'),'nano-p2p':Buffer.from('public synthetic p2p executable')};for(const name of COMPLETE.slice(2))bodies[name]=Buffer.from('public synthetic '+name+' executable');
  const source=Buffer.from('{"opaquePublicEvidence":true}\n'),input={format:complete?'nano-candidate-artifacts-v2':'nano-candidate-artifacts-v1',runtimeBase:BASE,architecture:'arm64',sourceRevision:REV,sourceEvidence:{file:join(root,'proof/source.json'),sha256:sha(source)},executables:Object.fromEntries(roles.map((name,i)=>[name,{file:join(root,folders[i],name),sha256:sha(bodies[name]),bytes:bodies[name].length}])),maximumArtifactBytes:4096};
  for(const [name,want]of Object.entries(input.executables))await writeFile(want.file,bodies[name],{mode:sourceMode});await writeFile(input.sourceEvidence.file,source);
  const expected=structuredClone(input),state={calls:[],context:null,mode:null,hook:null,hashMode:null,badLibraryRole:null},originals=[...Object.values(input.executables).map(e=>e.file),input.sourceEvidence.file];
  const before=await Promise.all(originals.map(async file=>({file,row:nine(await lstat(file,{bigint:true})),sha256:sha(await readFile(file))})));
  const runDocker=async argv=>{
   state.calls.push([...argv]);await state.hook?.(argv);
   if(argv[0]==='info')return 'linux aarch64\n';
   if(argv[0]==='image'&&argv[1]==='ls')return state.mode==='foreign-tag'?'sha256:'+'3'.repeat(64)+'\n':'';
   if(argv[0]==='image'&&argv[1]==='inspect')return JSON.stringify(argv[2]===IMAGE?{Id:IMAGE,Os:'linux',Architecture:'arm64',Config:{User:state.mode==='root-user'?'0:0':'10001:10001',Labels:{'org.opencontainers.image.revision':REV,'org.nanobees.runtime-base-id':BASE}}}:{Id:BASE,Os:'linux',Architecture:'arm64'});
   if(argv[0]==='tag')return '';
   if(argv[0]==='build'){
    state.context=argv.at(-1);assert.equal(argv[argv.indexOf('--network')+1],'none');assert.ok(argv.includes('--pull=false'));assert.equal(argv[argv.indexOf('--target')+1],complete?'complete':'node');assert.equal(argv[argv.indexOf('--build-arg')+1],'NANO_RUNTIME_BASE=nano-candidate-base:'+BASE.slice(7));
    assert.deepEqual((await readdir(state.context)).sort(),[...roles,'notices','source-evidence','candidate.Dockerfile'].sort());
    for(const name of roles){assert.deepEqual(await readFile(join(state.context,name)),bodies[name]);assert.equal((await lstat(join(state.context,name))).mode&0o777,0o755);assert.ok(argv.includes(HASH_ARGS[name]+'='+expected.executables[name].sha256));}
    assert.deepEqual(await readFile(join(state.context,'source-evidence')),source);assert.equal((await lstat(join(state.context,'notices/notices.sha256'))).mode&0o777,0o644);
    assert.equal((await readdir(join(state.context,'notices'))).length,12);assert.ok(argv.includes('SOURCE_REVISION='+REV));
    await writeFile(argv[argv.indexOf('--iidfile')+1],IMAGE+'\n',{flag:'wx'});return '';
   }
   if(argv[0]==='run'&&argv.includes('/usr/bin/sha256sum')){
    assert.deepEqual(argv.slice(argv.indexOf('/usr/bin/sha256sum')+2),roles.map(name=>'/usr/local/bin/'+name));
    const rows=roles.map(name=>expected.executables[name].sha256+'  /usr/local/bin/'+name);
    if(state.hashMode==='missing')rows.pop();
    if(state.hashMode==='duplicate')rows.push(rows.at(-1));
    if(state.hashMode==='extra')rows.push('f'.repeat(64)+'  /usr/local/bin/not-in-manifest');
    if(state.hashMode==='wrong')rows[rows.length-1]='f'.repeat(64)+'  /usr/local/bin/'+roles.at(-1);
    return rows.join('\n')+'\n';
   }
   if(argv[0]==='run'&&argv.includes('/usr/bin/ldd')){
    const targets=argv.slice(argv.indexOf('/usr/bin/ldd')+2);assert.equal(targets.length,1,'one loader command per executable');assert.ok(roles.some(name=>targets[0]==='/usr/local/bin/'+name));
    return state.mode==='missing-library'||targets[0]==='/usr/local/bin/'+state.badLibraryRole?'libmissing.so => not found\n':'libc.so.6 => /lib/libc.so.6\n';
   }
   throw Error('Unexpected Docker command');
  };
  const options={runDocker,repoRoot,temporaryRoot},raw=()=>Buffer.from(JSON.stringify(input)+'\n');
  const invoke=async extra=>{const manifestBytes=raw();return packageArtifacts(input,{...options,manifestBytes,manifestSha256:sha(manifestBytes),...extra});};
  const unchanged=async()=>{for(const old of before){assert.deepEqual(nine(await lstat(old.file,{bigint:true})),old.row);assert.equal(sha(await readFile(old.file)),old.sha256);}};
  await action({root,input,expected,state,options,raw,invoke,unchanged,runDocker,roles,source,bodies});
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


// v2 changes artifact inventory and package target only; no real Docker or ELF
// loadability is inferred from these injected transport tests.
test('complete v2 packages all five byte pins as executable copies and preserves caller intent',async()=>fixture(async f=>{
 const raw=f.raw(),known=sha(raw);f.state.hook=async a=>{if(a[0]==='info'){f.input.sourceRevision='b'.repeat(40);f.input.executables['nano-capella-duty-readiness'].file='/never-read';raw.fill(0);}};
 const r=await packageArtifacts(f.input,{...f.options,manifestBytes:raw,manifestSha256:known});
 assert.equal(r.format,'nano-candidate-artifacts-package-result-v2');assert.equal(r.sourceRevision,REV);assert.equal(r.image,IMAGE);assert.equal(r.noticeCount,11);assert.equal(r.sourceProvenanceSemanticallyVerified,false);assert.equal(r.productionQualified,false);assert.equal(r.temporaryContextRemoved,true);assert.deepEqual(r.executables,f.expected.executables);
 const loaders=f.state.calls.filter(a=>a[0]==='run'&&a.includes('/usr/bin/ldd'));assert.equal(loaders.length,5);assert.deepEqual(loaders.map(a=>a.at(-1)),COMPLETE.map(n=>'/usr/local/bin/'+n));assert.equal(f.state.calls.filter(a=>a[0]==='build').length,1);
 await f.unchanged();await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
},{complete:true,sourceMode:0o600}));

test('v1 and v2 inventories reject a missing role, extra role or cross-version inventory before Docker',async()=>fixture(async f=>{
 for(const missing of COMPLETE){const bad=structuredClone(f.input);delete bad.executables[missing];assert.throws(()=>validateArtifacts(bad),/Unexpected descriptor fields/);}
 const extra=structuredClone(f.input);extra.executables['unreviewed-tool']=structuredClone(extra.executables.nano);assert.throws(()=>validateArtifacts(extra),/Unexpected descriptor fields/);
 const downgraded=structuredClone(f.input);downgraded.format='nano-candidate-artifacts-v1';assert.throws(()=>validateArtifacts(downgraded),/Unexpected descriptor fields/);
 for(const format of [['nano-candidate-artifacts-v2'],{version:2},null,2,true]){const bad=structuredClone(f.input);bad.format=format;assert.throws(()=>validateArtifacts(bad),/Invalid candidate identity/);}
 const unknown=structuredClone(f.input);unknown.format='nano-candidate-artifacts-v3';assert.throws(()=>validateArtifacts(unknown),/Invalid candidate identity/);assert.equal(f.state.calls.length,0);
},{complete:true}));

test('v2 binary budget sums all five roles before any Docker call',async()=>fixture(async f=>{
 f.input.maximumArtifactBytes=Object.values(f.input.executables).reduce((n,v)=>n+v.bytes,0)-1;await assert.rejects(f.invoke(),/Artifact budget rejected/);assert.equal(f.state.calls.length,0);await f.unchanged();
},{complete:true}));

test('v2 aggregate budget also includes the independent source proof',async()=>fixture(async f=>{
 f.input.maximumArtifactBytes=Object.values(f.input.executables).reduce((n,v)=>n+v.bytes,0)+f.source.length-1;await assert.rejects(f.invoke(),/Regular bounded artifact required/);assert.equal(f.state.calls.some(a=>a[0]==='tag'||a[0]==='build'),false);assert.equal(f.state.context,null);await f.unchanged();
},{complete:true}));

test('v2 refuses a hardlinked pair of maintenance roles before context assembly',async()=>fixture(async f=>{
 const a=f.input.executables['nano-bootstrap-archive-maintenance'],b=f.input.executables['nano-capella-duty-readiness'];await rm(b.file);await link(a.file,b.file);b.sha256=a.sha256;b.bytes=a.bytes;
 await assert.rejects(f.invoke(),/Artifact identities overlap/);assert.equal(f.state.calls.some(a=>a[0]==='tag'||a[0]==='build'),false);assert.equal(f.state.context,null);
},{complete:true}));

test('v2 refuses the source evidence aliasing an executable FD identity',async()=>fixture(async f=>{
 f.input.sourceEvidence={file:f.input.executables['nano-capella-duty-readiness'].file,sha256:f.input.executables['nano-capella-duty-readiness'].sha256};await assert.rejects(f.invoke(),/Artifact identities overlap/);assert.equal(f.state.calls.some(a=>a[0]==='tag'||a[0]==='build'),false);assert.equal(f.state.context,null);await f.unchanged();
},{complete:true}));

test('v2 validates the fifth input byte pin instead of only nano and transport',async()=>fixture(async f=>{
 f.input.executables['nano-capella-duty-readiness'].sha256='f'.repeat(64);await assert.rejects(f.invoke(),/Artifact pin mismatch/);assert.equal(f.state.calls.some(a=>a[0]==='tag'||a[0]==='build'),false);await f.unchanged();
},{complete:true}));

test('v2 image digest inventory refuses missing, duplicate, foreign and wrong fifth-role hashes',async()=>{
 for(const mode of ['missing','duplicate','extra','wrong'])await fixture(async f=>{
  f.state.hashMode=mode;await assert.rejects(f.invoke(),error=>error.message===(mode==='missing'?'Missing built executable':'Built executable hash mismatch'));await f.unchanged();await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
 },{complete:true});
});

test('v2 checks the loader of the fifth role and cleans the context on refusal',async()=>fixture(async f=>{
 f.state.badLibraryRole='nano-capella-duty-readiness';await assert.rejects(f.invoke(),/Unresolved executable libraries/);const loaders=f.state.calls.filter(a=>a[0]==='run'&&a.includes('/usr/bin/ldd'));assert.deepEqual(loaders.map(a=>a.at(-1)),COMPLETE.map(n=>'/usr/local/bin/'+n));await f.unchanged();await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
},{complete:true}));

test('v2 rejects a root-user final image without altering any input',async()=>fixture(async f=>{
 f.state.mode='root-user';await assert.rejects(f.invoke(),/Built image metadata mismatch/);await f.unchanged();await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
},{complete:true}));

test('v2 keeps the original fifth-role permission row strict after context copying',async()=>fixture(async f=>{
 const source=f.input.executables['nano-capella-duty-readiness'].file;f.state.hook=async a=>{if(a[0]==='build')await chmod(source,0o400);};await assert.rejects(f.invoke(),/Artifact identity changed/);assert.deepEqual(await readFile(source),f.bodies['nano-capella-duty-readiness']);await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
},{complete:true}));

test('v2 preserves a build refusal as first error while still fencing and cleaning',async()=>fixture(async f=>{
 f.state.hook=async a=>{if(a[0]==='build'){f.state.context=a.at(-1);await chmod(f.input.executables['nano-capella-duty-readiness'].file,0o400);throw Error('synthetic complete build refusal');}};
 await assert.rejects(f.invoke(),error=>error.message==='synthetic complete build refusal');await absent(f.state.context);assert.deepEqual(await readdir(f.options.temporaryRoot),['keep']);
},{complete:true}));

test('v2 refuses to delete a replacement context directory after final inspection',async()=>fixture(async f=>{
 let retained,foreign;f.state.hook=async a=>{if(a[0]==='run'&&a.includes('/usr/bin/ldd')&&a.at(-1)==='/usr/local/bin/nano-capella-duty-readiness'){
  retained=f.state.context+'.retained';await rename(f.state.context,retained);await mkdir(f.state.context,{mode:0o700});foreign=join(f.state.context,'foreign-sentinel');await writeFile(foreign,'preserve unrelated replacement');await copyFile(join(retained,'image-id'),join(f.state.context,'image-id'));
 }};
 await assert.rejects(f.invoke(),/Artifact identity changed/);assert.equal(await readFile(foreign,'utf8'),'preserve unrelated replacement');assert.equal(await readFile(join(retained,'image-id'),'utf8'),IMAGE+'\n');await f.unchanged();
},{complete:true}));
