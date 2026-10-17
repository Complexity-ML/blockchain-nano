#!/usr/bin/env node
// Package two already qualified files. Integrity does not authenticate their provenance.
import {constants} from 'node:fs';
import {open,lstat,realpath,mkdtemp,mkdir,writeFile,chmod,rm,rmdir} from 'node:fs/promises';
import {resolve,isAbsolute,join} from 'node:path';
import {tmpdir} from 'node:os';
import {fileURLToPath,pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {execFile} from 'node:child_process';
import {promisify,isDeepStrictEqual} from 'node:util';
const exec=promisify(execFile),ROOT=fileURLToPath(new URL('../../',import.meta.url));
const MAX=256*1024*1024,META=1024*1024,NINE=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const sha=b=>createHash('sha256').update(b).digest('hex'),hash=s=>typeof s==='string'&&/^[0-9a-f]{64}$/.test(s);
const need=(ok,message)=>{if(!ok)throw Error(message);},row=s=>NINE.map(k=>String(s[k]));
const exact=(o,keys)=>need(o&&typeof o==='object'&&!Array.isArray(o)&&Object.keys(o).sort().join('|')===[...keys].sort().join('|'),'Unexpected descriptor fields');
const NOTICES={'LICENSE.ethereumjs':'third_party/opcodes/LICENSE.ethereumjs','LICENSE.evm-codes':'third_party/opcodes/LICENSE.evm-codes','bip39-LICENSE':'third_party/bip39/LICENSE','blst-LICENSE':'third_party/ckzg/blst/LICENSE','ckzg-LICENSE':'third_party/ckzg/LICENSE','json-COPYRIGHTS':'third_party/notices/json-COPYRIGHTS','json-LICENSE.MIT':'third_party/notices/json-LICENSE.MIT','libp2p-LICENSE-APACHE':'third_party/notices/libp2p-LICENSE-APACHE','libp2p-LICENSE-MIT':'third_party/notices/libp2p-LICENSE-MIT','mcl-COPYRIGHT':'third_party/mcl/COPYRIGHT','secp256k1-LICENSE':'third_party/secp256k1/COPYING'};
const RECIPE='87d6cad6284d491f2076cc7850576fc110318041a47ed20f6d2b0fd53bbead1f',NOTICE_MANIFEST='6eea677d963f83cedc3c60088eb176e8f88b7592316945f31998f1fb57eb1d73';
export function validateArtifacts(input){
 const d=structuredClone(input);exact(d,['format','runtimeBase','architecture','sourceRevision','sourceEvidence','executables','maximumArtifactBytes']);
 need(d.format==='nano-candidate-artifacts-v1'&&/^sha256:[0-9a-f]{64}$/.test(d.runtimeBase)&&['arm64','amd64'].includes(d.architecture)&&/^[0-9a-f]{40}$/.test(d.sourceRevision),'Invalid candidate identity');
 need(Number.isSafeInteger(d.maximumArtifactBytes)&&d.maximumArtifactBytes>0&&d.maximumArtifactBytes<=MAX,'Artifact budget rejected');
 exact(d.executables,['nano','nano-p2p']);exact(d.sourceEvidence,['file','sha256']);
 for(const e of [d.sourceEvidence,...Object.values(d.executables)])need(typeof e.file==='string'&&isAbsolute(e.file)&&resolve(e.file)===e.file&&hash(e.sha256),'Physical absolute artifact path and hash required');
 for(const e of Object.values(d.executables)){exact(e,['file','sha256','bytes']);need(Number.isSafeInteger(e.bytes)&&e.bytes>0,'Executable size rejected');}
 need(d.executables.nano.bytes+d.executables['nano-p2p'].bytes<d.maximumArtifactBytes,'Artifact budget rejected');return d;
}
async function physical(file){need(isAbsolute(file)&&resolve(file)===file&&await realpath(file)===file,'Artifact path must be physical');}
async function bytes(fd,limit){
 const chunks=[];let count=0;for(;;){const b=Buffer.alloc(Math.min(65536,limit-count+1));const {bytesRead:n}=await fd.read(b,0,b.length,count);if(!n)break;count+=n;need(count<=limit,'Artifact read budget');chunks.push(b.subarray(0,n));}return Buffer.concat(chunks);
}
function guardError(message,file,expected,actualFd,actualPath,expectedSha,actualSha){
 const object=v=>v?Object.fromEntries(NINE.map((k,i)=>[k,v[i]])):null;
 const differentFields=NINE.filter((_,i)=>expected?.[i]!==actualFd?.[i]||expected?.[i]!==actualPath?.[i]);
 return Error(message+': '+JSON.stringify({file,differentFields,expectedNine:object(expected),actualFdNine:object(actualFd),actualPathNine:object(actualPath),expectedSha256:expectedSha,actualSha256:actualSha}));
}
async function hold(file,want,limit,newIid=false){
 need(newIid||want&&hash(want.sha256),'Known input SHA required');const admissionStarted=newIid?performance.now():0;await physical(file);const fd=await open(file,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
 try{
  const observe=async()=>{try{await physical(file);const f=await fd.stat({bigint:true}),p=await lstat(file,{bigint:true});need(f.isFile()&&p.isFile(),'Regular artifact required');return {f:row(f),p:row(p)};}catch(e){throw Error('Artifact observation failed: '+JSON.stringify({file,code:e.code??null,reason:e.message}));}};
  const read=async()=>{try{return await bytes(fd,limit);}catch(e){throw Error('Artifact read failed: '+JSON.stringify({file,code:e.code??null,reason:e.message}),{cause:e});}};
  const first=await fd.stat({bigint:true});need(first.isFile()&&first.size>0n&&first.size<=BigInt(limit),'Regular bounded artifact required: '+file);
  const initial=await observe();if(!newIid&&!isDeepStrictEqual(initial.f,initial.p))throw guardError('Artifact FD/path mismatch',file,initial.f,initial.f,initial.p,want?.sha256,null);
  const body=await read(),digest=sha(body);let identity=row(first),admission=null;
  if(body.length!==Number(first.size)||want&&(digest!==want.sha256||want.bytes!==undefined&&body.length!==want.bytes))throw guardError('Artifact pin mismatch',file,identity,initial.f,initial.p,want?.sha256,digest);
  if(newIid){
   // Only the output of this operation's newly created context reaches this path.
   // Bind the first bytes once; metadata may settle before the first identity authority.
   need(/^sha256:[0-9a-f]{64}\n?$/.test(body.toString('utf8')),'Invalid new image ID: '+file);
   const began=admissionStarted;let previous=null,count=0,since=began,samples=0,metadataChanges=0;
   for(;;){
    need(performance.now()-began<30000,'New image ID admission window: '+file);
    const before=await observe(),actual=await read(),after=await observe();++samples;
    if(actual.length!==body.length||sha(actual)!==digest)throw guardError('New image ID byte pin changed',file,previous??initial.f,after.f,after.p,digest,sha(actual));
    const now=performance.now();need(now-began<30000,'New image ID admission window: '+file);
    const coherent=isDeepStrictEqual(before.f,before.p)&&isDeepStrictEqual(before.f,after.f)&&isDeepStrictEqual(after.f,after.p);
    if(coherent){
     if(previous&&isDeepStrictEqual(previous,after.f))++count;
     else{if(previous)++metadataChanges;previous=after.f;count=1;since=now;}
     if(count>=3&&now-since>=2000){identity=[...previous];admission={samples,stableSamples:count,stableSpanMs:Math.floor(now-since),metadataChanges,bytes:body.length,sha256:digest};break;}
    }else{++metadataChanges;previous=null;count=0;since=now;}
    await new Promise(resolve=>setTimeout(resolve,Math.min(1000,30000-(performance.now()-began))));
   }
  }
  const strict=(v,actualSha)=>{if(!isDeepStrictEqual(identity,v.f)||!isDeepStrictEqual(identity,v.p))throw guardError('Artifact identity changed',file,identity,v.f,v.p,digest,actualSha);};
  const fence=async()=>{strict(await observe(),null);const actualSha=sha(await read()),after=await observe();strict(after,actualSha);if(actualSha!==digest)throw guardError('Artifact hash changed',file,identity,after.f,after.p,digest,actualSha);};
  // This compares the admitted stable row; it never takes a replacement fourth row.
  await fence();return {file,fd,body,identity,sha256:digest,bytes:body.length,fence,admission};
 }catch(e){await fd.close().catch(()=>{});throw e;}
}
const defaultDocker=async(argv,options)=>{try{const r=await exec('docker',argv,{encoding:'utf8',maxBuffer:4*META,timeout:30000,...options});if(r.stderr)process.stderr.write(r.stderr);return r.stdout;}catch(e){if(typeof e.stderr==='string')process.stderr.write(e.stderr.slice(0,4*META));throw e;}};
export async function packageArtifacts(input,{manifestBytes,manifestSha256,runDocker=defaultDocker,repoRoot=ROOT,temporaryRoot,manifestGuard=null}={}){
 // Snapshot both objects and raw bytes before the first await; the caller's pin binds them.
 const d=validateArtifacts(input),raw=Buffer.from(manifestBytes??[]);
 need(raw.length>0&&raw.length<=META&&hash(manifestSha256)&&sha(raw)===manifestSha256&&isDeepStrictEqual(JSON.parse(raw),d),'Descriptor pin/object mismatch');
 const guards=manifestGuard?[manifestGuard]:[],sources=[],call=async(a,o)=>{const s=await runDocker([...a],o);need(typeof s==='string'&&Buffer.byteLength(s)<=4*META,'Docker response budget');return s;};
 let context,contextIdentity,ownedParent,ownedParentIdentity,first,result;
 try{
  const platform=(await call(['info','--format','{{.OSType}} {{.Architecture}}'])).trim().split(/\s+/),arch={aarch64:'arm64',x86_64:'amd64',arm64:'arm64',amd64:'amd64'}[platform[1]];
  need(platform.length===2&&platform[0]==='linux'&&arch===d.architecture,'Docker daemon platform mismatch');
  const inspect=async id=>JSON.parse(await call(['image','inspect',id,'--format','{{json .}}']));
  const base=await inspect(d.runtimeBase);need(base.Id===d.runtimeBase&&base.Architecture===d.architecture&&base.Os==='linux','Runtime base mismatch');
  let total=0;const identities=new Set();
  for(const [role,want] of [...Object.entries(d.executables),['source-evidence',d.sourceEvidence]]){
   const g=await hold(want.file,want,d.maximumArtifactBytes-total);guards.push(g);total+=g.bytes;
   const inode=g.identity.slice(0,2).join(':');need(!identities.has(inode),'Artifact identities overlap');identities.add(inode);sources.push([role,g]);
  }
  const recipe=await hold(join(repoRoot,'deploy/docker/candidate.Dockerfile'),{sha256:RECIPE},META);guards.push(recipe);
  const notices=await hold(join(repoRoot,'deploy/docker/notices.sha256'),{sha256:NOTICE_MANIFEST},META);guards.push(notices);
  const records=new Map();for(const line of notices.body.toString().trimEnd().split('\n')){const r=/^([0-9a-f]{64})  ([A-Za-z0-9._-]+)$/.exec(line);need(r&&Object.hasOwn(NOTICES,r[2])&&!records.has(r[2]),'Invalid notice inventory');records.set(r[2],r[1]);}
  need(records.size===Object.keys(NOTICES).length,'Incomplete notice inventory');
  for(const [name,path] of Object.entries(NOTICES)){const g=await hold(join(repoRoot,path),{sha256:records.get(name)},META);guards.push(g);sources.push(['notices/'+name,g]);}
  if(temporaryRoot===undefined){temporaryRoot=await mkdtemp(join(await realpath(tmpdir()),'nano-package-parent-'));ownedParent=temporaryRoot;ownedParentIdentity=row(await lstat(temporaryRoot,{bigint:true}));await chmod(temporaryRoot,0o700);}
  await physical(temporaryRoot);const parent=await lstat(temporaryRoot);need(parent.isDirectory()&&parent.uid===process.getuid()&&(parent.mode&0o022)===0,'Unsafe context parent');
  context=await mkdtemp(join(temporaryRoot,'nano-package-'));contextIdentity=row(await lstat(context,{bigint:true}));await mkdir(join(context,'notices'),{mode:0o700});
  for(const [name,g] of sources){const path=join(context,name),mode=name==='nano'||name==='nano-p2p'?0o755:name.startsWith('notices/')?0o644:0o600;await writeFile(path,g.body,{flag:'wx',mode});await chmod(path,mode);}await chmod(join(context,'notices'),0o755);
  await writeFile(join(context,'notices/notices.sha256'),notices.body,{flag:'wx',mode:0o644});await chmod(join(context,'notices/notices.sha256'),0o644);await writeFile(join(context,'candidate.Dockerfile'),recipe.body,{flag:'wx',mode:0o600});
  const baseRef='nano-candidate-base:'+d.runtimeBase.slice(7),existing=(await call(['image','ls','--no-trunc','--filter','reference='+baseRef,'--format','{{.ID}}'])).trim();
  need(!existing||existing===d.runtimeBase,'Derived runtime reference already differs');if(!existing)await call(['tag',d.runtimeBase,baseRef]);
  need((await inspect(baseRef)).Id===d.runtimeBase,'Derived base reference mismatch');for(const g of guards)await g.fence();
  await call(['build','--network','none','--pull=false','--progress','plain','--iidfile',join(context,'image-id'),'--build-arg','NANO_RUNTIME_BASE='+baseRef,'--build-arg','RUNTIME_BASE_ID='+d.runtimeBase,'--build-arg','SOURCE_REVISION='+d.sourceRevision,'--build-arg','NANO_SHA256='+d.executables.nano.sha256,'--build-arg','P2P_SHA256='+d.executables['nano-p2p'].sha256,'-f',join(context,'candidate.Dockerfile'),context],{timeout:300000});
  need((await inspect(baseRef)).Id===d.runtimeBase,'Derived base reference changed');
  const idFile=await hold(join(context,'image-id'),null,80,true);guards.push(idFile);const image=idFile.body.toString().trim();need(/^sha256:[0-9a-f]{64}$/.test(image),'Invalid built image ID');
  const final=await inspect(image);need(final.Id===image&&final.Os==='linux'&&final.Architecture===d.architecture&&final.Config?.User==='10001:10001'&&final.Config?.Labels?.['org.opencontainers.image.revision']===d.sourceRevision&&final.Config?.Labels?.['org.nanobees.runtime-base-id']===d.runtimeBase,'Built image metadata mismatch');
  const sandbox=['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','512m','--cpus','1'];
  const output=await call([...sandbox,'--entrypoint','/usr/bin/sha256sum',image,'/usr/local/bin/nano','/usr/local/bin/nano-p2p']);const seen=new Set();
  for(const line of output.trim().split('\n')){const r=/^([0-9a-f]{64})\s+\/usr\/local\/bin\/(nano|nano-p2p)$/.exec(line);need(r&&!seen.has(r[2])&&r[1]===d.executables[r[2]].sha256,'Built executable hash mismatch');seen.add(r[2]);}need(seen.size===2,'Missing built executable');
  const libs=await call([...sandbox,'--entrypoint','/usr/bin/ldd',image,'/usr/local/bin/nano','/usr/local/bin/nano-p2p']);need(libs.trim()&&!/not found/.test(libs),'Unresolved executable libraries');
  result={format:'nano-candidate-artifacts-package-result-v1',image,architecture:d.architecture,sourceRevision:d.sourceRevision,runtimeBase:d.runtimeBase,manifestSha256,sourceEvidence:{...d.sourceEvidence,bytes:sources.find(([n])=>n==='source-evidence')[1].bytes},executables:structuredClone(d.executables),artifactBytes:total,noticeCount:records.size,iidFirstAdmission:idFile.admission,integrityVerified:true,sourceProvenanceSemanticallyVerified:false,productionQualified:false};
 }catch(e){first=e;}
 finally{
  for(const g of guards){try{await g.fence();}catch(e){first??=e;}}
  for(const g of guards){try{await g.fd.close();}catch(e){first??=e;}}
  if(context)try{const s=await lstat(context,{bigint:true});need(s.isDirectory()&&row(s).slice(0,2).join(':')===contextIdentity.slice(0,2).join(':'),'Owned context identity changed');await rm(context,{recursive:true});}catch(e){first??=e;}
  if(ownedParent)try{const s=await lstat(ownedParent,{bigint:true});need(s.isDirectory()&&row(s).slice(0,2).join(':')===ownedParentIdentity.slice(0,2).join(':'),'Owned temporary parent identity changed');await rmdir(ownedParent);}catch(e){first??=e;}
 }
 if(first)throw first;return {...result,temporaryContextRemoved:true};
}
export async function packageCandidate(file,expectedSha,options={}){
 need(hash(expectedSha),'Manifest SHA256 required');const g=await hold(file,{sha256:expectedSha},META);
 try{return await packageArtifacts(JSON.parse(g.body),{...options,manifestBytes:g.body,manifestSha256:expectedSha,manifestGuard:g});}catch(e){await g.fd.close().catch(()=>{});throw e;}
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 try{need(process.argv.length===5&&process.argv[2]==='--artifacts','Usage: package-candidate.mjs --artifacts ABSOLUTE_MANIFEST MANIFEST_SHA256');console.log(JSON.stringify(await packageCandidate(process.argv[3],process.argv[4])));}catch(e){console.error(e.message);process.exitCode=1;}
}
