#!/usr/bin/env node
// Stopped physical copies only. Never opens a DB, invokes a command or starts a node.
import * as fs from 'node:fs/promises';
import {constants} from 'node:fs';
import {createHash,randomUUID} from 'node:crypto';
import {dirname,join,resolve,isAbsolute,relative,sep} from 'node:path';
import {fileURLToPath} from 'node:url';
import {performance} from 'node:perf_hooks';

const MAGIC=Buffer.from('NANO-STOPPED-ROOT-V1\n'),MAX_JSON=16*1024*1024;
const F=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const same=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const digest=b=>createHash('sha256').update(b).digest('hex');
const exact=(v,keys)=>v&&typeof v==='object'&&!Array.isArray(v)&&same(Object.keys(v).sort(),[...keys].sort());
const hex=v=>typeof v==='string'&&/^[0-9a-f]{64}$/.test(v);
function need(ok,code){if(!ok)throw Object.assign(Error(code),{code});}
function integer(n,minimum,maximum){return Number.isSafeInteger(n)&&n>=minimum&&n<=maximum;}
function row(s){return Object.fromEntries(F.map(k=>[k,String(s[k])]))}
function entryName(name){return typeof name==='string'&&name.length>0&&Buffer.byteLength(name)<=4096&&!isAbsolute(name)&&!name.includes('\\')&&!/[\x00-\x1f\x7f]/.test(name)&&name.split('/').every(p=>p&&p!=='.'&&p!=='..'&&Buffer.byteLength(p)<=255);}
function within(a,b){const rel=relative(a,b);return rel===''||(!rel.startsWith('..'+sep)&&rel!=='..'&&!isAbsolute(rel));}
function disjoint(a,b){return !within(a,b)&&!within(b,a);}
function limits(v){need(exact(v,['maximumEntries','maximumFileBytes','maximumTotalBytes','maximumElapsedMs']),'limits-shape');need(integer(v.maximumEntries,1,100000)&&integer(v.maximumFileBytes,1,16*1024**3)&&integer(v.maximumTotalBytes,1,1024**4)&&v.maximumFileBytes<=v.maximumTotalBytes&&integer(v.maximumElapsedMs,1000,24*3600*1000),'limits-range');return v;}
function artifact(v){need(exact(v,['path','sha256','bytes'])&&typeof v.path==='string'&&hex(v.sha256)&&integer(v.bytes,1,MAX_JSON),'proof-artifact');return v;}
export function validateCreate(c){
 need(exact(c,['format','roots','destination','proofs','limits','preconditions'])&&c.format==='nano-stopped-backup-create-v1','create-shape');
 need(exact(c.roots,['data','witness'])&&Object.values(c.roots).every(p=>typeof p==='string'&&isAbsolute(p))&&typeof c.destination==='string'&&isAbsolute(c.destination),'root-paths');
 need(exact(c.proofs,['stop','commitments']),'proofs-shape');artifact(c.proofs.stop);artifact(c.proofs.commitments);limits(c.limits);
 need(exact(c.preconditions,['allWritersStopped','sourcesMountedReadOnly'])&&c.preconditions.allWritersStopped===true&&c.preconditions.sourcesMountedReadOnly===true,'external-stop-and-readonly-required');return c;
}
function operation(l,{checkpoint=async()=>{},sync=async h=>h.sync(),filesystem=path=>fs.statfs(path,{bigint:true}),clock=()=>performance.now(),signal}={}){
 const began=clock();let end=began+l.maximumElapsedMs;const check=()=>{need(!signal?.aborted,'interrupted');need(clock()<=end,'deadline');};
 return {check,constrain(v){end=Math.min(end,began+v.maximumElapsedMs);check();},async sync(...a){check();await sync(...a);check();},async checkpoint(...a){check();await checkpoint(...a);check();},async filesystem(...a){check();const r=await filesystem(...a);check();return r;}};
}
async function physical(path,type){
 need(typeof path==='string'&&isAbsolute(path)&&resolve(path)===path&&!/[\x00-\x1f\x7f]/.test(path),'physical-absolute-path');need(await fs.realpath(path)===path,'alias-path');
 const s=await fs.lstat(path,{bigint:true});need(type==='directory'?s.isDirectory():s.isFile(),'path-type');return s;
}
async function openFile(path){
 const before=await physical(path,'file');need(before.nlink===1n,'hardlink-file');const h=await fs.open(path,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
 try{need(same(row(before),row(await h.stat({bigint:true}))),'fd-path-before');return {h,before:row(before)};}catch(e){await h.close();throw e;}
}
async function readPinned(path,{sha256,bytes},op,capture=false){
 need(hex(sha256)&&integer(bytes,0,Number.MAX_SAFE_INTEGER),'known-byte-pin');const {h,before}=await openFile(path);const hash=createHash('sha256'),chunks=[];let count=0;
 try{
  need(BigInt(before.size)===BigInt(bytes),'file-size');for(;;){op?.check();const b=Buffer.alloc(65536),{bytesRead:n}=await h.read(b,0,b.length,null);if(!n)break;count+=n;need(count<=bytes,'file-grew');hash.update(b.subarray(0,n));if(capture)chunks.push(b.subarray(0,n));}
  need(count===bytes&&hash.digest('hex')===sha256,'file-digest');need(same(before,row(await h.stat({bigint:true})))&&same(before,row(await fs.lstat(path,{bigint:true})))&&await fs.realpath(path)===path,'fd-path-after');return {material:before,body:capture?Buffer.concat(chunks):null};
 }finally{await h.close();}
}
async function hashFile(path,max,op,consume){
 const {h,before}=await openFile(path);let count=0;const hash=createHash('sha256');
 try{
  need(BigInt(before.size)<=BigInt(max),'file-budget');
  for(;;){op.check();const b=Buffer.alloc(65536),{bytesRead:n}=await h.read(b,0,b.length,null);if(!n)break;count+=n;need(count<=max&&BigInt(count)<=BigInt(before.size),'file-read-bound');const part=b.subarray(0,n);hash.update(part);if(consume)await consume(part);}
  need(BigInt(count)===BigInt(before.size)&&same(before,row(await h.stat({bigint:true})))&&same(before,row(await fs.lstat(path,{bigint:true})))&&await fs.realpath(path)===path,'file-nine-changed');return {material:before,sha256:hash.digest('hex'),bytes:count};
 }finally{await h.close();}
}
async function inventory(root,l,op,inodes=new Set()){
 await physical(root,'directory');const entries=[];let total=0,scheduled=inodes.size+1;need(scheduled<=l.maximumEntries,'entry-budget');
 async function walk(path,name){
  op.check();need(entries.length<l.maximumEntries,'entry-budget');const s=await fs.lstat(path,{bigint:true});need(s.isDirectory()||s.isFile(),'symlink-or-special');const r=row(s);
  const id=r.dev+':'+r.ino;need(!inodes.has(id),'shared-inode');inodes.add(id);
  if(s.isDirectory()){
   entries.push({path:name,kind:'directory',material:r});const names=[],directory=await fs.opendir(path,{encoding:'buffer'});
   for await(const child of directory){op.check();need(++scheduled<=l.maximumEntries,'entry-budget');const b=child.name;need(Buffer.isBuffer(b),'directory-entry-encoding');const n=b.toString('utf8');need(Buffer.from(n).equals(b)&&entryName(n)&&!n.includes('/'),'directory-entry');names.push(n);}names.sort();
   for(const child of names)await walk(join(path,child),name?name+'/'+child:child);
   need(same(r,row(await fs.lstat(path,{bigint:true})))&&await fs.realpath(path)===path,'directory-nine-changed');
  }else{
   need(s.nlink===1n,'hardlink-file');const value=await hashFile(path,l.maximumFileBytes,op);need(same(value.material,r),'file-admission-changed');total+=value.bytes;need(Number.isSafeInteger(total)&&total<=l.maximumTotalBytes,'total-budget');entries.push({path:name,kind:'file',...value});
  }
 }
 await walk(root,'');return {sourcePath:root,entries,totalBytes:total};
}
async function writeAll(h,b,op){let at=0;while(at<b.length){op.check();const {bytesWritten:n}=await h.write(b,at,b.length-at,null);need(n>0,'short-write');at+=n;}}
async function syncDirectory(path,op){
 const s=await physical(path,'directory');const h=await fs.open(path,constants.O_RDONLY|constants.O_DIRECTORY|constants.O_NOFOLLOW);
 try{need((await h.stat({bigint:true})).ino===s.ino,'directory-fd');await op.sync(h,'directory',path);need((await fs.lstat(path,{bigint:true})).ino===s.ino&&await fs.realpath(path)===path,'directory-sync-alias');}finally{await h.close();}
}
async function newFile(path,body,op){const h=await fs.open(path,constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);try{await writeAll(h,body,op);await op.sync(h,'file',path);}finally{await h.close();}await readPinned(path,{sha256:digest(body),bytes:body.length},op);}
async function newRoot(path,protectedPaths,op){
 need(isAbsolute(path)&&resolve(path)===path&&!/[\x00-\x1f\x7f]/.test(path),'new-root-path');for(const p of protectedPaths)need(disjoint(path,p),'overlapping-roots');
 const parent=dirname(path),s=await physical(parent,'directory');need(!(s.mode&0o022n)&&s.uid===BigInt(process.getuid()),'output-parent-owner-mode');
 const filesystem=await op.filesystem(parent);need(typeof filesystem.type==='bigint'&&filesystem.type!==0x01021994n&&filesystem.type!==0x858458f6n,'unsupported-volatile-filesystem');
 // mkdir is exclusive even if another process wins the absent-name race.
 await fs.mkdir(path,{mode:0o700});need(await fs.realpath(path)===path,'new-root-alias');return path;
}
async function archiveRoot(file,root,inv,op){
 const h=await fs.open(file,constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);const hash=createHash('sha256');let bytes=0;
 const write=async b=>{hash.update(b);bytes+=b.length;await writeAll(h,b,op);};
 try{
  await write(MAGIC);for(const e of inv.entries){if(e.kind!=='file')continue;const prefix=Buffer.alloc(8);prefix.writeBigUInt64BE(BigInt(e.bytes));await write(prefix);const value=await hashFile(join(root,e.path),e.bytes,op,write);need(same(value,{material:e.material,sha256:e.sha256,bytes:e.bytes}),'source-changed-during-archive');}
  await op.sync(h,'file',file);
 }finally{await h.close();}
 const known={name:file.split(sep).at(-1),sha256:hash.digest('hex'),bytes};await readPinned(file,known,op);return known;
}
async function publish(root,manifest,op){
 const body=Buffer.from(JSON.stringify(manifest)+'\n');need(body.length<=MAX_JSON,'manifest-budget');const temporary=join(root,'.manifest-pending-'+randomUUID());await newFile(temporary,body,op);
 await syncDirectory(root,op);await op.checkpoint('before-publication',{root});
 // link refuses any existing final name; rename would replace an old manifest.
 await fs.link(temporary,join(root,'manifest.json'));await fs.unlink(temporary);await op.checkpoint('after-publication-before-sync',{root});await syncDirectory(root,op);await syncDirectory(dirname(root),op);await readPinned(join(root,'manifest.json'),{sha256:digest(body),bytes:body.length},op);await op.checkpoint('after-publication-sync',{root});
 return {sha256:digest(body),bytes:body.length};
}
async function proofRead(proofs,op){const found={};for(const name of ['stop','commitments']){const a=artifact(proofs[name]);const r=await readPinned(a.path,a,op,true);found[name]={artifact:a,material:r.material,body:r.body};}return found;}
async function proofsFence(proofs,op){for(const p of Object.values(proofs)){const r=await readPinned(p.artifact.path,p.artifact,op);need(same(r.material,p.material),'proof-nine-changed');}}
export async function createBackup(input,options={}){
 const c=validateCreate(structuredClone(input)),op=operation(c.limits,options);for(const p of Object.values(c.roots))await physical(p,'directory');need(disjoint(c.roots.data,c.roots.witness),'overlapping-source-roots');
 if(options.controlPath)for(const root of Object.values(c.roots))need(disjoint(options.controlPath,root),'control-inside-source');
 const proofPaths=Object.values(c.proofs).map(p=>p.path);for(const p of proofPaths){await physical(p,'file');for(const root of Object.values(c.roots))need(disjoint(p,root),'proof-inside-source');}
 need(c.proofs.stop.path!==c.proofs.commitments.path,'independent-proof-files');const proofs=await proofRead(c.proofs,op),inodes=new Set();let initial,first;
 try{
  initial={data:await inventory(c.roots.data,c.limits,op,inodes),witness:await inventory(c.roots.witness,c.limits,op,inodes)};need(initial.data.entries.length+initial.witness.entries.length<=c.limits.maximumEntries&&initial.data.totalBytes+initial.witness.totalBytes<=c.limits.maximumTotalBytes,'combined-budget');
  const root=await newRoot(c.destination,[...Object.values(c.roots),...proofPaths,...(options.controlPath?[options.controlPath]:[])],op);await op.checkpoint('new-backup-root',{root});await fs.mkdir(join(root,'proofs'),{mode:0o700});
  const archives={};for(const name of ['data','witness'])archives[name]=await archiveRoot(join(root,name+'.nbak'),c.roots[name],initial[name],op);
  for(const name of ['stop','commitments'])await newFile(join(root,'proofs',name+'.json'),proofs[name].body,op);await syncDirectory(join(root,'proofs'),op);
  const finalInodes=new Set();for(const name of ['data','witness'])need(same(await inventory(c.roots[name],c.limits,op,finalInodes),initial[name]),'source-final-nine-or-bytes');await proofsFence(proofs,op);
  for(const name of ['data','witness'])await readPinned(join(root,archives[name].name),archives[name],op);for(const name of ['stop','commitments'])await readPinned(join(root,'proofs',name+'.json'),c.proofs[name],op);
  const manifest={format:'nano-stopped-backup-v1',roots:Object.fromEntries(['data','witness'].map(name=>[name,{...initial[name],archive:archives[name]}])),proofs:Object.fromEntries(['stop','commitments'].map(name=>[name,{file:'proofs/'+name+'.json',sha256:c.proofs[name].sha256,bytes:c.proofs[name].bytes}])),limits:c.limits,operatorAssertions:c.preconditions,sourceQuiescenceExternallyRequired:true,sourceReadOnlyExternallyRequired:true,independentProofOriginExternallyRequired:true,cppSemanticValidationRequired:true,signingAuthorized:false,runtimeStarted:false,productionQualified:false};
  await op.checkpoint('archives-synced-sources-fenced',{root});const pin=await publish(root,manifest,op);return {format:'nano-stopped-backup-created-v1',passed:true,manifest:pin,files:initial.data.entries.filter(x=>x.kind==='file').length+initial.witness.entries.filter(x=>x.kind==='file').length,bytes:initial.data.totalBytes+initial.witness.totalBytes,cppSemanticValidationRequired:true,sourceReadOnlyExternallyRequired:true,signingAuthorized:false,runtimeStarted:false,productionQualified:false};
 }catch(e){first=e;throw e;}finally{
  // A first refusal remains primary even when a later source fence also fails.
  if(initial)try{const ids=new Set();for(const name of ['data','witness'])need(same(await inventory(c.roots[name],c.limits,op,ids),initial[name]),'source-finally-changed');await proofsFence(proofs,op);}catch(e){if(!first)throw e;}
 }
}
function validateManifest(m){
 need(exact(m,['format','roots','proofs','limits','operatorAssertions','sourceQuiescenceExternallyRequired','sourceReadOnlyExternallyRequired','independentProofOriginExternallyRequired','cppSemanticValidationRequired','signingAuthorized','runtimeStarted','productionQualified'])&&m.format==='nano-stopped-backup-v1','manifest-shape');limits(m.limits);need(m.signingAuthorized===false&&m.runtimeStarted===false&&m.productionQualified===false&&m.cppSemanticValidationRequired===true&&m.sourceQuiescenceExternallyRequired===true&&m.sourceReadOnlyExternallyRequired===true&&m.independentProofOriginExternallyRequired===true,'manifest-scope');
 need(exact(m.operatorAssertions,['allWritersStopped','sourcesMountedReadOnly'])&&Object.values(m.operatorAssertions).every(v=>v===true),'manifest-preconditions');need(exact(m.roots,['data','witness'])&&exact(m.proofs,['stop','commitments']),'manifest-roles');let entries=0,total=0;
 for(const name of ['data','witness']){
  const r=m.roots[name];need(exact(r,['sourcePath','entries','totalBytes','archive'])&&typeof r.sourcePath==='string'&&isAbsolute(r.sourcePath)&&Array.isArray(r.entries)&&r.entries.length>0,'manifest-root');need(exact(r.archive,['name','sha256','bytes'])&&r.archive.name===name+'.nbak'&&hex(r.archive.sha256)&&integer(r.archive.bytes,MAGIC.length,Number.MAX_SAFE_INTEGER),'archive-pin');
  const seen=new Map();let bytes=0,archiveBytes=MAGIC.length;
  for(const e of r.entries){need(e&&['directory','file'].includes(e.kind)&&exact(e,e.kind==='directory'?['path','kind','material']:['path','kind','material','sha256','bytes']),'manifest-entry');need(typeof e.path==='string'&&(e.path===''?e.kind==='directory'&&seen.size===0:entryName(e.path))&&!seen.has(e.path),'manifest-entry-path');need(e.path===''||seen.get(e.path.includes('/')?e.path.slice(0,e.path.lastIndexOf('/')):'')==='directory','manifest-entry-parent');seen.set(e.path,e.kind);
   need(exact(e.material,F)&&F.every(k=>typeof e.material[k]==='string'&&/^-?(0|[1-9][0-9]*)$/.test(e.material[k])),'source-nine-shape');
   if(e.kind==='file'){need(hex(e.sha256)&&integer(e.bytes,0,m.limits.maximumFileBytes)&&BigInt(e.material.size)===BigInt(e.bytes)&&e.material.nlink==='1','manifest-file-pin');bytes+=e.bytes;archiveBytes+=8+e.bytes;}
  }
  need(bytes===r.totalBytes&&archiveBytes===r.archive.bytes,'manifest-root-total');entries+=r.entries.length;total+=bytes;
 }
 need(entries<=m.limits.maximumEntries&&total<=m.limits.maximumTotalBytes,'manifest-combined-budget');for(const name of ['stop','commitments']){const p=m.proofs[name];need(exact(p,['file','sha256','bytes'])&&p.file==='proofs/'+name+'.json'&&hex(p.sha256)&&integer(p.bytes,1,MAX_JSON),'manifest-proof');}return m;
}
async function readManifest(root,sha256,op){
 const rootMaterial=row(await physical(root,'directory'));need(hex(sha256),'external-manifest-pin');const p=join(root,'manifest.json'),s=await fs.lstat(p,{bigint:true});need(s.size>0n&&s.size<=BigInt(MAX_JSON),'manifest-bound');const loaded=await readPinned(p,{sha256,bytes:Number(s.size)},op,true);const m=validateManifest(JSON.parse(loaded.body)),proofDirectoryMaterial=row(await physical(join(root,'proofs'),'directory'));
 await namespace(root,['data.nbak','manifest.json','proofs','witness.nbak'],op);await namespace(join(root,'proofs'),['commitments.json','stop.json'],op);return {m,material:loaded.material,body:loaded.body,rootMaterial,proofDirectoryMaterial};
}
async function namespace(path,wanted,op){const names=[],directory=await fs.opendir(path);for await(const e of directory){op.check();need(names.length<wanted.length,'package-namespace-size');names.push(e.name);}need(same(names.sort(),wanted),'package-namespace');}
async function unpack(root,name,r,op,destination=null){
 const archive=join(root,r.archive.name),{h,before}=await openFile(archive);const hash=createHash('sha256');let total=0;
 async function get(n,consumer){let left=n;while(left){op.check();const b=Buffer.alloc(Math.min(left,65536)),{bytesRead:k}=await h.read(b,0,b.length,null);need(k>0,'archive-truncated');const part=b.subarray(0,k);hash.update(part);total+=k;left-=k;if(consumer)await consumer(part);}}
 try{
  need(BigInt(before.size)===BigInt(r.archive.bytes),'archive-size');
  const chunks=[];await get(MAGIC.length,b=>chunks.push(b));need(Buffer.concat(chunks).equals(MAGIC),'archive-magic');
  if(destination)await fs.mkdir(join(destination,name),{mode:0o700});
  for(const e of r.entries){
   if(e.kind==='directory'){if(destination&&e.path)await fs.mkdir(join(destination,name,e.path),{mode:0o700});continue;}
   const prefix=[];await get(8,b=>prefix.push(b));need(Buffer.concat(prefix).readBigUInt64BE()===BigInt(e.bytes),'archive-frame-size');const sha=createHash('sha256');let out;
   try{if(destination)out=await fs.open(join(destination,name,e.path),constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);await get(e.bytes,async b=>{sha.update(b);if(out)await writeAll(out,b,op);});need(sha.digest('hex')===e.sha256,'archived-file-digest');if(out)await op.sync(out,'file',join(destination,name,e.path));}finally{if(out)await out.close();}
  }
  const more=Buffer.alloc(1);need((await h.read(more,0,1,null)).bytesRead===0&&total===r.archive.bytes&&hash.digest('hex')===r.archive.sha256,'archive-tail-or-digest');need(same(before,row(await h.stat({bigint:true})))&&same(before,row(await fs.lstat(archive,{bigint:true})))&&await fs.realpath(archive)===archive,'archive-nine-changed');return before;
 }finally{await h.close();}
}
async function packageFence(root,loaded,rows,op){
 need(same(row(await physical(root,'directory')),loaded.rootMaterial)&&same(row(await physical(join(root,'proofs'),'directory')),loaded.proofDirectoryMaterial),'backup-directory-nine-changed');await namespace(root,['data.nbak','manifest.json','proofs','witness.nbak'],op);await namespace(join(root,'proofs'),['commitments.json','stop.json'],op);
 const manifest=await readPinned(join(root,'manifest.json'),{sha256:digest(loaded.body),bytes:loaded.body.length},op);need(same(manifest.material,loaded.material),'manifest-nine-changed');for(const [path,want] of rows){const got=await readPinned(path,want,op);need(same(got.material,want.material),'backup-nine-changed');}
}
async function inspectBackup(root,sha256,op){
 const loaded=await readManifest(root,sha256,op),rows=[];op.constrain(loaded.m.limits);
 for(const name of ['data','witness']){const a=loaded.m.roots[name].archive;rows.push([join(root,a.name),{sha256:a.sha256,bytes:a.bytes,material:await unpack(root,name,loaded.m.roots[name],op)}]);}
 for(const name of ['stop','commitments']){const p=loaded.m.proofs[name],path=join(root,p.file),r=await readPinned(path,p,op);rows.push([path,{...p,material:r.material}]);}
 await packageFence(root,loaded,rows,op);await op.checkpoint('backup-verified',{root});return {loaded,rows};
}
export async function verifyBackup(root,sha256,options={}){
 await inspectBackup(root,sha256,operation({maximumElapsedMs:24*3600000},options));return {format:'nano-stopped-backup-verified-v1',passed:true,manifestSha256:sha256,cppSemanticValidationRequired:true,signingAuthorized:false,runtimeStarted:false,productionQualified:false};
}
export async function restoreBackup(root,sha256,destination,options={}){
 // Verify every frame/hash BEFORE making the destination. The supplied pin must
 // come from the independent create receipt, never be learned from this backup.
 const op=operation({maximumElapsedMs:24*3600000},options),{loaded,rows}=await inspectBackup(root,sha256,op);let first;
 try{
  await newRoot(destination,[root,...Object.values(loaded.m.roots).map(r=>r.sourcePath)],op);await op.checkpoint('new-restore-root',{root:destination});
  for(const name of ['data','witness']){const r=loaded.m.roots[name],path=join(root,r.archive.name),material=await unpack(root,name,r,op,destination);need(same(material,rows.find(([p])=>p===path)[1].material),'original-archive-nine-changed');}
  await fs.mkdir(join(destination,'proofs'),{mode:0o700});for(const name of ['stop','commitments']){const p=loaded.m.proofs[name],path=join(root,p.file),read=await readPinned(path,p,op,true);need(same(read.material,rows.find(([p])=>p===path)[1].material),'original-proof-nine-changed');await newFile(join(destination,p.file),read.body,op);}
  const restoredIds=new Set(),content=entries=>entries.map(e=>e.kind==='directory'?{path:e.path,kind:e.kind}:{path:e.path,kind:e.kind,sha256:e.sha256,bytes:e.bytes});
  for(const name of ['data','witness']){const readback=await inventory(join(destination,name),loaded.m.limits,op,restoredIds);need(same(content(readback.entries),content(loaded.m.roots[name].entries)),'restored-content-readback');need(readback.entries.every(e=>e.material.uid===String(process.getuid())&&!(BigInt(e.material.mode)&0o022n)),'restored-owner-mode');}
  for(const name of ['data','witness'])for(const e of [...loaded.m.roots[name].entries].reverse())if(e.kind==='directory')await syncDirectory(join(destination,name,e.path),op);
  await syncDirectory(join(destination,'proofs'),op);await packageFence(root,loaded,rows,op);await op.checkpoint('restored-files-synced',{root:destination});
  const receipt={format:'nano-stopped-restore-v1',sourceManifestSha256:sha256,files:Object.values(loaded.m.roots).reduce((n,r)=>n+r.entries.filter(e=>e.kind==='file').length,0),cppSemanticValidationRequired:true,independentLatestCommitmentsRequiredBeforeSigning:true,signingAuthorized:false,runtimeStarted:false,productionQualified:false};const completion=await publish(destination,receipt,op);return {...receipt,passed:true,completion};
 }catch(e){first=e;throw e;}finally{if(rows.length)try{await packageFence(root,loaded,rows,op);}catch(e){if(!first)throw e;}}
}
export async function main(argv){
 const abort=new AbortController();const stop=()=>abort.abort();process.once('SIGINT',stop);process.once('SIGTERM',stop);
 try{
  let result;if(argv[0]==='create'&&argv.length===3){const path=argv[1];need(hex(argv[2]),'control-pin');const s=await fs.lstat(path,{bigint:true});need(s.size>0n&&s.size<=BigInt(MAX_JSON),'control-size');const input=await readPinned(path,{sha256:argv[2],bytes:Number(s.size)},null,true);result=await createBackup(JSON.parse(input.body),{signal:abort.signal,controlPath:path});const after=await readPinned(path,{sha256:argv[2],bytes:Number(s.size)},null);need(same(after.material,input.material),'control-nine-changed');}
  else if(argv[0]==='verify'&&argv.length===3)result=await verifyBackup(argv[1],argv[2],{signal:abort.signal});
  else if(argv[0]==='restore'&&argv.length===4)result=await restoreBackup(argv[1],argv[2],argv[3],{signal:abort.signal});
  else throw Object.assign(Error('Usage: stopped-backup.mjs create CONTROL SHA256 | verify BACKUP MANIFEST_SHA256 | restore BACKUP MANIFEST_SHA256 NEW_DESTINATION'),{code:'usage'});
  process.stdout.write(JSON.stringify(result)+'\n');return result;
 }finally{process.removeListener('SIGINT',stop);process.removeListener('SIGTERM',stop);}
}
if(process.argv[1]&&fileURLToPath(import.meta.url)===resolve(process.argv[1]))main(process.argv.slice(2)).catch(e=>{process.stderr.write(JSON.stringify({passed:false,reason:typeof e.code==='string'?e.code:'stopped-backup-refused',partialNamespaceRetained:true,signingAuthorized:false,productionQualified:false})+'\n');process.exitCode=1;});
