#!/usr/bin/env node
// ROOT-only focused FS consumer. Extracts one pinned function; never imports run.mjs.
import {open,lstat,realpath,mkdir,mkdtemp,readdir,writeFile,chmod} from 'node:fs/promises';
import {constants as fsConstants} from 'node:fs';
import {resolve,join,dirname} from 'node:path';
import {tmpdir} from 'node:os';
import {createHash,randomUUID} from 'node:crypto';
import {setTimeout as delay} from 'node:timers/promises';
import assert from 'node:assert/strict';

const began=performance.now(),deadline=began+28000;
const tick=()=>{if(performance.now()>=deadline)throw Error('Fixture deadline');};
const need=(ok,why)=>{tick();if(!ok)throw Error(why);};
const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
const sha=b=>createHash('sha256').update(b).digest('hex');
const materialFields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'];
const material=s=>materialFields.map(k=>String(s[k]));
const rowChanges=(a,b)=>[...['path','sha256','bytes'].filter(k=>a[k]!==b[k]),...materialFields.filter((_,i)=>a.material[i]!==b.material[i])];
const within=(a,b)=>a===b||a.startsWith(b+'/');
const expectedRuntime={sha256:'8e979cf22e72c12a54f9a68afbe21356a654c41c45896f64ba4b35b29a87e1b1',bytes:137116};
const emitted=[];
const emit=row=>{emitted.push(row);process.stdout.write(JSON.stringify(row)+'\n');};

// Independent positive oracle: no HELD/NEW/admission logic is reused here.
async function inspectFile(path){
 const f=await open(path,fsConstants.O_RDONLY|fsConstants.O_NOFOLLOW|fsConstants.O_NONBLOCK);
 try{const before=await f.stat({bigint:true});assert(before.isFile()&&before.nlink===1n&&before.uid===BigInt(process.getuid())&&before.size>0n&&before.size<1024n);const hash=createHash('sha256'),chunk=Buffer.alloc(1024);let bytes=0;for(;;){const {bytesRead:n}=await f.read(chunk,0,chunk.length,null);if(!n)break;bytes+=n;hash.update(chunk.subarray(0,n));}assert.equal(bytes,Number(before.size));assert.deepEqual(material(await f.stat({bigint:true})),material(before));assert.deepEqual(material(await lstat(path,{bigint:true})),material(before));return {path,sha256:hash.digest('hex'),bytes,material:material(before)};}finally{await f.close();}
}

// Same row authority and NEW-before-HELD priority as the fresh launcher. Diagnostics
// are fixed here; no fixture paths, compared values or bytes enter stdout.
function context(output){
 const held=new Map(),newFiles=new Map();
 const inputNeed=(ok,why)=>need(ok,why);
 async function observe(path,expected,body=false,admit=true){
  tick();path=resolve(path);if(newFiles.has(path)&&!held.has(path)){expected=newFiles.get(path);admit=false;}
  need(await realpath(path)===path,'Linked fixture input refused');
  const f=await open(path,fsConstants.O_RDONLY|fsConstants.O_NOFOLLOW|fsConstants.O_NONBLOCK);
  try{
   const before=await f.stat({bigint:true});need(before.isFile()&&before.nlink===1n&&before.size>0n&&before.size<=67108864n,'Fixture input bound/type');
   const hash=createHash('sha256'),chunk=Buffer.alloc(65536),chunks=[];let size=0;
   for(;;){tick();const {bytesRead:n}=await f.read(chunk,0,chunk.length,null);if(!n)break;size+=n;need(BigInt(size)<=before.size,'Fixture input grew');hash.update(chunk.subarray(0,n));if(body)chunks.push(Buffer.from(chunk.subarray(0,n)));}
   const after=await f.stat({bigint:true}),linked=await lstat(path,{bigint:true}),row={path,sha256:hash.digest('hex'),bytes:size,material:material(after)};
   inputNeed(size===Number(before.size)&&canonical(material(before))===canonical(row.material)&&canonical(row.material)===canonical(material(linked)),'Fixture input changed during FD read');
   if(expected)inputNeed(row.sha256===expected.sha256&&(expected.bytes===undefined||row.bytes===expected.bytes),'Fixture input SHA/bytes differs');
   if(held.has(path))inputNeed(canonical(held.get(path))===canonical(row),'Previously admitted fixture input changed');else if(admit)held.set(path,row);
   return body?Buffer.concat(chunks):row;
  }finally{await f.close();}
 }
 async function save(path,value){
  tick();const bytes=Buffer.isBuffer(value)?value:Buffer.from(typeof value==='string'?value:JSON.stringify(value,null,2)+'\n');
  const f=await open(path,'wx',0o600);try{await f.writeFile(bytes);await f.sync();}finally{await f.close();}
  const directory=await open(dirname(path),'r');try{await directory.sync();}finally{await directory.close();}
  const expected={sha256:sha(bytes),bytes:bytes.length};await observe(path,expected,false,false);newFiles.set(resolve(path),expected);return bytes;
 }
 return {held,newFiles,observe,save,inputNeed,output};
}

async function pinnedRuntime(path,pin,oldRow){
 const c=context(''),row=await c.observe(path,pin,false,false);
 if(oldRow)need(canonical(row)===canonical(oldRow),'Pinned runtime material changed');
 const bytes=await c.observe(path,pin,true,false),again=await c.observe(path,pin,false,false);
 need(canonical(row)===canonical(again),'Pinned runtime material changed during extraction');
 return {row,bytes};
}

async function main(){
 const args=process.argv.slice(2);need(args.length===4,'Four explicit fixture arguments required');
 const [runtimeArg,hashArg,sizeArg,parentArg]=args;
 need(hashArg===expectedRuntime.sha256&&sizeArg===String(expectedRuntime.bytes),'Exact V7 runtime pin required');
 const runtime=resolve(runtimeArg),parent=resolve(parentArg),temporary=await realpath(tmpdir()),p=await lstat(parent,{bigint:true});
 need(await realpath(parent)===parent&&p.isDirectory()&&p.uid===BigInt(process.getuid())&&(Number(p.mode)&0o7777)===0o700&&!within(parent,temporary)&&!within(temporary,parent),'Private fixture parent outside OS tmp required');
 const loaded=await pinnedRuntime(runtime,expectedRuntime),text=loaded.bytes.toString('utf8');
 const marker=' async function stageMacDockerRotationInputs(',next=' async function auditFreshRotation(terminal){',start=text.indexOf(marker),end=text.indexOf(next,start);
 need(start>=0&&end>start&&text.indexOf(marker,start+marker.length)===-1,'Unique exact staging function required');
 const extracted=text.slice(start,end).trim();need(Buffer.byteLength(extracted)===6133&&sha(Buffer.from(extracted))==='ea6f64eeee1fcbccaac86663999285a8b755e5bf9ea2240360349c74a02a343a','Exact extracted staging function required');
 const root=await mkdtemp(join(parent,'nano-rotation-input-fixture-')),cases=[];
 for(let i=0;i<5;++i){
  const base=join(root,String(i)),sources=join(base,'sources'),output=join(base,'reports');
  await mkdir(base,{mode:0o700});await mkdir(sources,{mode:0o700});await mkdir(output,{mode:0o700});
  const paths=[join(sources,'stop.json'),join(sources,'proof.json')],buffers=[Buffer.from('opaque-stop-fixture-'+i+'\n'),Buffer.from('opaque-proof-fixture-'+i+'\n')],c=context(output);
  for(let j=0;j<2;++j)await c.save(paths[j],buffers[j]);
  cases.push({paths,buffers,c});
 }
 // All source fixtures are NEW. Their single material admission follows one
 // stable whole table; no admitted source is ever refreshed after a mutation.
 const sourcePaths=cases.flatMap(c=>c.paths),settleBegan=performance.now();let previous=null,consecutive=0,series=0;
 for(;;){
  tick();const rows=[];for(const c of cases)for(const path of c.paths)rows.push(await c.c.observe(path,c.c.newFiles.get(path),false,false));
  const now=performance.now();if(previous&&canonical(previous)===canonical(rows))++consecutive;else{consecutive=1;series=now;}previous=rows;
  if(consecutive>=3&&now-series>=2000)break;
  need(now-settleBegan<6000,'New fixture sources did not settle');await delay(1000);
 }
 const selectedSourceRows=new Map(previous.map(row=>[row.path,row]));
 for(const c of cases)for(const path of c.paths){const row=await c.c.observe(path,c.c.newFiles.get(path),false,false);assert.deepEqual(row,selectedSourceRows.get(path));c.c.newFiles.delete(path);c.c.held.set(path,row);}
 const summaryKeys=['format','passed','inputFiles','knownBytesExact','sourceNineFieldsAndShaUnchanged','newIndependentFiles','privateDirectoryMode','privateFileMode','firstAdmissionProbeCount','firstAdmissionStableSpanMs','firstAdmissionElapsedMs','directoryBindReadOnly','rawInputsExported','productionQualified'].sort();
 const scenarios=['fresh_rotation_inputs_positive','fresh_rotation_inputs_source_bytes_refused','fresh_rotation_inputs_source_metadata_refused','fresh_rotation_inputs_destination_bytes_refused','fresh_rotation_inputs_namespace_refused'];
 let expectedRefusals=0;
 for(let i=0;i<cases.length;++i){
  tick();const c=cases[i],scope={resolve,join,dirname,tmpdir,mkdtemp,open,lstat,realpath,readdir,fsConstants,createHash,sha,canonical,material,materialFields,need,inputNeed:c.c.inputNeed,rowChanges,within,held:c.c.held,newFiles:c.c.newFiles,observe:c.c.observe,save:c.c.save,output:c.c.output,uuid:randomUUID(),interrupted:undefined,delay};
  const sourceBefore=await Promise.all(c.paths.map(inspectFile));
  assert.deepEqual((await readdir(dirname(c.paths[0]))).sort(),['proof.json','stop.json']);
  const sourceDirectory=await lstat(dirname(c.paths[0]),{bigint:true});assert.equal(Number(sourceDirectory.mode)&0o7777,0o700);
  const stage=Function(...Object.keys(scope),'"use strict";return ('+extracted+');')(...Object.values(scope));
  const got=await stage(...c.paths),targets=[join(got.directory,'stop.json'),join(got.directory,'proof.json')];
  assert.deepEqual(Object.keys(got.summary).sort(),summaryKeys);assert.equal(got.summary.format,'nano-fresh-rotation-mac-private-input-stage-v1');
  for(const k of ['passed','knownBytesExact','sourceNineFieldsAndShaUnchanged','newIndependentFiles','directoryBindReadOnly'])assert.equal(got.summary[k],true);
  for(const k of ['rawInputsExported','productionQualified'])assert.equal(got.summary[k],false);
  assert.equal(got.summary.inputFiles,2);assert.equal(got.summary.privateDirectoryMode,'0700');assert.equal(got.summary.privateFileMode,'0600');
  assert(Number.isInteger(got.summary.firstAdmissionProbeCount)&&got.summary.firstAdmissionProbeCount>=3);assert(Number.isInteger(got.summary.firstAdmissionStableSpanMs)&&Number.isInteger(got.summary.firstAdmissionElapsedMs)&&got.summary.firstAdmissionStableSpanMs>=2000&&got.summary.firstAdmissionElapsedMs>=got.summary.firstAdmissionStableSpanMs&&got.summary.firstAdmissionElapsedMs<=30000);
  const dir=await lstat(got.directory,{bigint:true});assert(dir.isDirectory()&&dir.uid===BigInt(process.getuid())&&(Number(dir.mode)&0o7777)===0o700);assert.deepEqual((await readdir(got.directory)).sort(),['proof.json','stop.json']);
  assert.equal(await realpath(got.directory),got.directory);assert(within(got.directory,temporary)&&!within(got.directory,c.c.output)&&!within(got.directory,dirname(c.paths[0])));
  const ids=new Set(sourceBefore.map(row=>row.material[0]+':'+row.material[1]));
  for(let j=0;j<2;++j){const row=await inspectFile(targets[j]),id=row.material[0]+':'+row.material[1];assert.equal(row.sha256,sha(c.buffers[j]));assert.equal(row.bytes,c.buffers[j].length);assert(!ids.has(id));ids.add(id);assert.equal(c.c.held.has(targets[j]),false);assert(c.c.newFiles.has(targets[j]));assert.equal(row.material[3],String(process.getuid()));assert.equal(Number(row.material[2])&0o7777,0o600);assert.deepEqual(await inspectFile(c.paths[j]),sourceBefore[j]);}
  const verified=await got.verify();assert.equal(verified.strictAfterAuditFencePassed,true);assert.deepEqual(Object.keys(verified).sort(),[...summaryKeys,'strictAfterAuditFencePassed'].sort());
  if(i===1){const b=Buffer.from(c.buffers[0]);b[0]^=1;await writeFile(c.paths[0],b);await assert.rejects(got.verify(),/Fixture input SHA\/bytes differs|Previously admitted fixture input changed/);++expectedRefusals;}
  if(i===2){await chmod(c.paths[0],0o640);await assert.rejects(got.verify(),/Previously admitted fixture input changed/);++expectedRefusals;}
  if(i===3){const b=Buffer.from(c.buffers[1]);b[0]^=1;await writeFile(targets[1],b);await assert.rejects(got.verify(),/Fixture input SHA\/bytes differs/);++expectedRefusals;}
  if(i===4){await writeFile(join(got.directory,'unexpected'),Buffer.from('opaque'),{flag:'wx',mode:0o600});await assert.rejects(got.verify(),/Exact private staged input namespace required/);++expectedRefusals;}
  // Separate fixtures ensure a source refusal cannot mask destination/namespace
  // discrimination. For those cases the original sources must still pass OLD9.
  if(i===0||i>=3)for(let j=0;j<2;++j){await c.c.observe(c.paths[j],c.c.held.get(c.paths[j]),false,false);assert.deepEqual(await inspectFile(c.paths[j]),sourceBefore[j]);}
  emit({scenario:scenarios[i],passed:true,successfulStageCalls:1,expectedRefusals:i===0?0:1,newSignaturesCreated:0,databaseOpened:false,networkStarted:false});
 }
 await pinnedRuntime(runtime,expectedRuntime,loaded.row);tick();
 assert.equal(expectedRefusals,4);assert.equal(sourcePaths.length,10);assert.equal(emitted.length,5);
 emit({scenario:'fresh_rotation_inputs_summary',passed:true,groups:5,successfulStageCalls:5,expectedRefusals,opaqueSourceFiles:10,payloadBytes:cases.reduce((n,c)=>n+c.buffers.reduce((a,b)=>a+b.length,0),0),newSignaturesCreated:0,databaseOpened:false,networkStarted:false,runtimeSourceUnchanged:true,elapsedMs:Math.ceil(performance.now()-began),productionQualified:false});
}
main().catch(()=>{process.stderr.write('{"scenario":"fresh_rotation_inputs_fixture_refused","passed":false,"detailsExported":false}\n');process.exitCode=1;});
