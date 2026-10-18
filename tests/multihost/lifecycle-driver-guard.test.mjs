// ROOT-only negative integration test. No actual campaign inputs or network.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,lstat,open,rm,realpath,readFile} from 'node:fs/promises';
import {constants} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,dirname,resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {performance} from 'node:perf_hooks';
import * as lifecycleOutputAdmission from './lifecycle-runtime.mjs';
const message='Fresh lifecycle runtime driver is unavailable; managed submission and owner-local terminal audit are required';
const historicalSourcePins={
 "vps-linux-bootstrap.mjs": "8a5b020e44c678b66c344baaa8b5769b816fd207c4a73ee53cd72a2e8787cf0f",
 "run.mjs": "4bd4d37b443dad9604bc80cdb16245f898148cd3677eb5c9c9330a8b0dc70725",
 "terminal-evidence.mjs": "c252de8c6ba3d05714748ab9c65216d02a19c8790bf5e5c14c6a72ac7dbfc42c",
  "native-runtime-env.mjs": "e5c988c7013b0a8b86af10f0f5d51edb46123cc3e73329167bbab6b5f4869119",
 "prepare.mjs": "8cffaff8a204ec276832f96c5a283747de835ca2913bc5ebb37fadb3d3da0b21",
 "program-locators.mjs": "afb6fd53db3930389e2c5adfd6a1c9cb0d59369f5926bc33476c9c7de2829c33",
 "linux-owned-files.mjs": "70d44d56d9a9ebc6704cf6acd02e29615d21ecc61b0c29c0d095531d76628119",
 "harness-manifest.mjs": "b01e069ade274ac38e612f9f8eefa7ce17e204b4764593d0371891047374c09d",
 "lifecycle.mjs": "16ceccd02e7487d4424285554d210753aa70943b39714819b32f9c158a2f4a23",
 "lifecycle-runtime.mjs": "5806bd7236b5314d2884c53e03a674d2fc2346558a7aa3b37db383e3d3db5fc0",
 "lifecycle-owner.mjs": "ab92d7136b88b528912275fe017778608c7ad0a2c43c4252bcc74e1cb862f216",
 "../../docs/qualification/local-supervisor-linux-20261015/source/known-files.pl": "7dd88b7d99ab15e9f2b1e52aac796265e6ed9bed24323e66584b3f31138cc0f6",
 "../../docs/qualification/local-supervisor-linux-20261015/source/copy-bootstrap-pairs.pl": "9fcd47d3bee3530c163e00e8b6a8dd475645336ba48579db45bdc9e20bb3bdf3"
};
// Historical literals remain intact; only this explicitly named source generation
// selects the two new helper implementations. No old receipt is requalified.
const generation20261018=Object.freeze({
 "lifecycle-runtime.mjs":"d7b92d610a36b6b046ccdf350bbbc91d7cc4861e73b90e80d38df8ee1bab2edd",
 "lifecycle-owner.mjs":"e60ce35c0b877de95c1ffb414881343cdca6c97a19ff1dac9ca033424c1ec880"
});
const generation20261018NativeLoader=Object.freeze({...generation20261018,'run.mjs':'47e45e602ecd5242d8f6784d9244afe7488688f1ee0061b5bdf70811a97bb565','prepare.mjs':'fdebaf486276bc3fd5ccd452da9fbba063556453d480f3000f20a20ee156b92f','native-loader-guard.mjs':'7b484108927236da8f1b5373d1f8d1338bbafa14a1d9495285c9598f6d16e69b'});
const generation20261018NativeLoopback=Object.freeze({...generation20261018NativeLoader,'prepare.mjs':'a3735e0bb6d306ef83482483fee2aec44463fab23fd6b4ff53f7157c53c94297'});
Object.freeze(historicalSourcePins);
const sourcePinGeneration='generation20261018NativeLoopback';
const sourcePins=Object.freeze({...historicalSourcePins,...generation20261018NativeLoopback});
const sha=b=>createHash('sha256').update(b).digest('hex');
const fields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'];
const material=s=>fields.map(k=>String(s[k]));
async function stable(path){
 const f=await open(path,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
 try{const before=await f.stat({bigint:true});assert(before.isFile()&&before.nlink===1n&&before.size>0n&&before.size<=1048576n);const bytes=await f.readFile();assert.equal(BigInt(bytes.length),before.size);assert.deepEqual(material(await f.stat({bigint:true})),material(before));assert.deepEqual(material(await lstat(path,{bigint:true})),material(before));return {sha256:sha(bytes),material:material(before)};}finally{await f.close();}
}
// Only this test's NEW, known synthetic bytes receive an initial stability
// window. Once admitted, the original full-nine row stays strictly held.
async function writeAndAdmitFixture(path,bytes){
 const f=await open(path,'wx',0o600);try{await f.writeFile(bytes);await f.sync();}finally{await f.close();}
 const parent=await open(dirname(path),'r');try{await parent.sync();}finally{await parent.close();}
 const expectedSha=sha(bytes),expectedSize=String(bytes.length),deadline=performance.now()+5000;
 let previous,start=0,samples=0;
 for(;;){
  assert(performance.now()<deadline,'New synthetic fixture first-admission deadline');
  const row=await stable(path),now=performance.now();assert(now<=deadline,'New synthetic fixture read deadline');assert.equal(row.sha256,expectedSha,'Known new fixture SHA');assert.equal(row.material[6],expectedSize,'Known new fixture size');
  if(previous&&row.sha256===previous.sha256&&row.material.every((v,i)=>v===previous.material[i]))++samples;else{previous=row;start=now;samples=1;}
  if(samples>=3&&now-start>=2000)return previous;
  const remaining=deadline-performance.now();assert(remaining>0,'New synthetic fixture stability deadline');await new Promise(resolve=>setTimeout(resolve,Math.min(1000,remaining)));
 }
}
const absent=async path=>{try{await lstat(path);}catch(e){if(e.code==='ENOENT')return;throw e;}assert.fail('Forbidden fixture path was created: '+path);};
test('actual lifecycle driver generation20261018 gates default mode and private authorizations before absent seed/output/network',{timeout:35000},async()=>{
 assert.equal(sourcePinGeneration,'generation20261018NativeLoopback');assert.deepEqual(Object.keys(generation20261018).sort(),['lifecycle-owner.mjs','lifecycle-runtime.mjs']);
 assert.deepEqual(Object.keys(generation20261018NativeLoader).sort(),['lifecycle-owner.mjs','lifecycle-runtime.mjs','native-loader-guard.mjs','prepare.mjs','run.mjs']);
 assert.deepEqual(Object.keys(generation20261018NativeLoopback).sort(),Object.keys(generation20261018NativeLoader).sort());
 assert.deepEqual(Object.keys(generation20261018NativeLoopback).filter(leaf=>generation20261018NativeLoopback[leaf]!==generation20261018NativeLoader[leaf]),['prepare.mjs']);
 for(const [leaf,pin]of Object.entries(historicalSourcePins))assert.equal(sourcePins[leaf],Object.hasOwn(generation20261018NativeLoopback,leaf)?generation20261018NativeLoopback[leaf]:pin,'Only the explicitly named source generation may override historical pins');
 const run=resolve(process.argv[2]??fileURLToPath(new URL('./run.mjs',import.meta.url))),source=dirname(run),before={};
 for(const [leaf,expected] of Object.entries(sourcePins)){before[leaf]=await stable(join(source,leaf));assert.equal(before[leaf].sha256,expected,'Exact frozen source required');}
 const parent=await realpath(await mkdtemp(join(tmpdir(),'nano-lifecycle-unavailable-'))),missing=[];
 try{
  const artifact=path=>{missing.push(path);return {path,sha256:'d'.repeat(64),bytes:1};};
  const names=['mac-native','mac-docker','vps-docker'],owners={};
  for(let i=0;i<3;++i){const name=names[i],core=join(parent,name+'-build'),network=join(parent,name+'-network'),roles={nano:'nano',p2p:'nano-p2p',bootstrapAudit:'nano-capella-isolated-bootstrap-audit',stoppedCapture:'nano-capella-isolated-stopped-capture',lifecycleTool:'nano-capella-deposit-audit'};
   if(i===0)Object.assign(roles,{exporter:'nano-capella-transition-tests',bootstrapImport:'nano-capella-isolated-bootstrap-import'});if(i===1)roles.bootstrapImport='nano-capella-isolated-bootstrap-import';
   // /outputs locators are guest-only: test absence only of this NEW fixture.
   const selected=path=>({path,sha256:'d'.repeat(64),bytes:1});
   owners[name]={host:i===2?'ubuntu@127.0.0.1':'mac',kind:['native-macos-arm64','linux-arm64','linux-amd64'][i],image:i===0?null:i===1?'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14',uid:i===2?1000:501,gid:i===2?1000:20,coreBuildRoot:core,networkBuildRoot:network,coreArchive:selected(i===0?core+'/libnano_core.a':'/outputs/libnano_core.a'),programs:Object.fromEntries(Object.entries(roles).map(([role,leaf])=>[role,selected((i===0?(role==='p2p'?network:core):role==='p2p'?'/network-build':'/outputs')+'/'+leaf)])),admission:artifact(join(parent,name+'-missing-admission.json'))};missing.push(core,network);
  }
  const id=(owner,suffix,digit)=>({owner,keyEnv:'NANO_TEST_CAPELLA_NETWORK_KEY_'+suffix,publicKey:'0x'+digit.repeat(96)}),q=n=>'0x'+BigInt(n).toString(16);
  const spec={format:'nano-fresh-capella-three-owner-campaign-v1',sourceBaseRevision:'b5a48e0f2a05bc392f7604edb6214ee6b0078d67',overlayManifest:artifact(join(parent,'missing-overlay.json')),owners,fixture:{seedFile:join(parent,'missing-seed'),ledgerDirectory:join(parent,'missing-ledger'),legacyPublicIdentity:artifact(join(parent,'missing-legacy.json'))},clock:{maximumStartupElapsedMs:'0x1',maximumDowntimeMs:'0x1',maximumSteps:'0x1'},backupRestore:false,productionQualified:false,lifecycle:{format:'nano-fresh-validator-lifecycle-v1',identities:{voluntaryExit:id('vps-docker','A','1'),slashing:id('mac-native','B','2')},deadlines:{registrationMs:100000,activationMs:100000,finalityMs:10000,restoreMs:10000,actionMs:30000,auditMs:30000,maximumEpochsAfterActivation:400,maximumRunMs:20000000},resources:{maximumWallSeconds:q(30),maximumHistoryBlocks:q(4000),maximumArchiveBytes:q(50000000),maximumIndexBytes:q(500000),maximumInventoryFiles:q(10000),maximumInventoryBytes:q(512*1024*1024)},syntheticActionAuthorized:true,intentionallyAuthorConflictingHeaders:true,privateVpsLifecycleTransferAuthorized:false}};
  const output=join(parent,'missing-output');missing.push(spec.fixture.seedFile,spec.fixture.ledgerDirectory,output);for(const p of missing)await absent(p);
  const env=Object.fromEntries(Object.entries(process.env).filter(([k])=>!k.startsWith('NANO_TEST_')));
  const cases=[{message,edit:()=>{}},{message:'specific human authorization for private VPS transfer is required before export',edit:x=>{x.lifecycle.runtimeDriverEnabled=true;}},{message:'specific human authorization for cross-owner action/job evidence transfer is required before export',edit:x=>{x.lifecycle.runtimeDriverEnabled=true;x.lifecycle.privateVpsLifecycleTransferAuthorized=true;}}];
  for(let i=0;i<cases.length;++i){const selected=structuredClone(spec);cases[i].edit(selected);if(selected.lifecycle.runtimeDriverEnabled)for(const [name,o] of Object.entries(selected.owners))o.programs.lifecycleSubmit={path:(name==='mac-native'?o.coreBuildRoot:'/outputs')+'/nano-capella-lifecycle-submit',sha256:'d'.repeat(64),bytes:1};
   const file=join(parent,'synthetic-spec-'+i+'.json'),bytes=Buffer.from(JSON.stringify(selected)),specBefore=await writeAndAdmitFixture(file,bytes);
   const child=spawnSync(process.execPath,['--max-old-space-size=32','--max-semi-space-size=1',run,'ubuntu@127.0.0.1',selected.sourceBaseRevision,output,'--fresh-campaign',file,'--fresh-campaign-sha',sha(bytes)],{env,encoding:'utf8',timeout:4000,maxBuffer:262144});
   assert.ifError(child.error);assert.equal(child.signal,null);assert.equal(child.status,1);assert.equal(child.stdout,'');assert(child.stderr.includes(cases[i].message),'A parser/import/ENOENT refusal is not this guard');assert(!child.stderr.includes('ENOENT'));
   for(const p of missing)await absent(p);assert.deepEqual(await stable(file),specBefore);for(const leaf of Object.keys(sourcePins))assert.deepEqual(await stable(join(source,leaf)),before[leaf]);
  }
 }finally{await rm(parent,{recursive:true,force:true});}
});

// Inject only filesystem observations/time. The function under test is the exact
// save() body in run.mjs, so the parent fails causally without importing run or
// reading any campaign control/capsule. No real I/O, process or network here.
const outputMaterialFields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'];
async function newOutputFixture({samples,changeAfterSample,changeAfterWait,changeAfterAdmission,readError}={}){
 const path='/synthetic-owned-output/new.json',input=Buffer.from('{"known":true}'),creation=['1','2','33152','501','20','1',String(input.length),'100','200'],expectedSha=sha(input),held=new Map(),newFiles=new Map(),events=[];let clock=0,reads=0,openCount=0,created=false,written,currentRead;
 const stable=ctime=>({before:[...creation.slice(0,8),String(ctime)],after:[...creation.slice(0,8),String(ctime)],linked:[...creation.slice(0,8),String(ctime)],sha256:expectedSha,bytes:input.length});
 const sample=async()=>{const index=reads++;events.push('sample');if(readError)throw readError;const x=typeof samples==='function'?samples(index,stable):samples?.[Math.min(index,samples.length-1)]??stable(200);changeAfterSample?.({index,held,newFiles,path});return structuredClone(x);};
 const wait=async ms=>{assert.equal(ms,1000);events.push('wait');clock+=ms;changeAfterWait?.({held,newFiles,path,clock});};
 const need=(v,m)=>{if(!v)throw Error(m);},material=stat=>outputMaterialFields.map(k=>String(stat[k])),stat=m=>({...Object.fromEntries(outputMaterialFields.map((k,i)=>[k,BigInt(m[i])])),isFile:()=>true});
 const open=async(file,flags,mode)=>{++openCount;if(file===path&&flags==='wx'){assert.equal(mode,0o600);assert.equal(created,false);created=true;events.push('create');return {async writeFile(b){written=Buffer.from(b);events.push('write');},async sync(){events.push('file-sync');},async stat(){return stat(creation);},async close(){events.push('file-close');}};}if(file===path){assert.equal(typeof flags,'number');currentRead=await sample();let stats=0,offset=0;return {async stat(){return stat(stats++===0?currentRead.before:currentRead.after);},async read(buffer,start,length){const n=Math.min(length,input.length-offset);input.copy(buffer,start,offset,offset+n);offset+=n;return {bytesRead:n};},async close(){events.push('read-close');}};}assert.equal(file,'/synthetic-owned-output');assert.equal(flags,'r');return {async sync(){events.push('parent-sync');},async close(){events.push('parent-close');}};};
 const source=await readFile(new URL('./run.mjs',import.meta.url),'utf8');
 const extract=(start,end)=>{const at=source.indexOf(start);assert(at>=0&&source.indexOf(start,at+1)===-1);const stop=source.indexOf(end,at);assert(stop>at);return source.slice(at,stop).trim();};
 const saveCode=extract(' async function save(path,value){','\n const spec='),observeCode=extract(' async function observe(path,expected,body=false,admit=true){','\n async function fence(){');
 const deps={open,dirname:p=>p.slice(0,p.lastIndexOf('/')),resolve:p=>p,realpath:async p=>p,lstat:async()=>stat(currentRead.linked),fsConstants:constants,createHash,canonical:v=>JSON.stringify(v),material,materialFields:outputMaterialFields,materialChanges:(a,b)=>outputMaterialFields.filter((_,i)=>a[i]!==b[i]),rowChanges:()=>[],held,newFiles,publicExportProbePaths:new Set(),sha,need,inputNeed:need,admitNewLifecycleOutput:async(file,pin,m,options)=>{const result=await lifecycleOutputAdmission.admitNewLifecycleOutput(file,pin,m,{...options,sample,now:()=>clock,wait});changeAfterAdmission?.({held,newFiles,path});return result;}};
 const save=Function(...Object.keys(deps),observeCode+'\n'+saveCode+'\nreturn save;')(...Object.values(deps));
 return {path,input,creation,stable,held,newFiles,events,save,expectedSha,get reads(){return reads;},get clock(){return clock;},get openCount(){return openCount;},get written(){return written;}};
}
test('new output admission: actual save rejects transient ctime sample before stable first admission',async()=>{
 const f=await newOutputFixture({samples:(i,stable)=>{const x=stable(i===0?200:201);if(i===0){x.after[8]='201';x.linked[8]='201';}return x;}}),bytes=await f.save(f.path,f.input);
 assert.deepEqual(bytes,f.input);assert.deepEqual(f.written,f.input);assert.equal(f.reads,4);assert.equal(f.clock,3000);assert.equal(f.held.size,0);assert.deepEqual(f.newFiles.get(f.path),{sha256:f.expectedSha,bytes:f.input.length});assert.deepEqual(f.events.slice(0,6),['create','write','file-sync','file-close','parent-sync','parent-close']);
});
test('new output admission: three strict samples span two seconds and ctime change resets the run',async()=>{
 const f=await newOutputFixture({samples:(i,stable)=>stable(i<2?200:201)});await f.save(f.path,f.input);assert.equal(f.reads,5);assert.equal(f.clock,4000);
 const clean=await newOutputFixture();await clean.save(clean.path,clean.input);assert.equal(clean.reads,3);assert.equal(clean.clock,2000);assert.equal(clean.newFiles.size,1);
});
test('new output admission: every known-byte and creation-identity mismatch refuses without retry',async()=>{
 for(const changed of ['sha256','bytes',...outputMaterialFields.slice(0,8)]){
  const f=await newOutputFixture({samples:(i,stable)=>{const x=stable(201);x.after[8]='202';x.linked[8]='202';if(changed==='sha256')x.sha256='f'.repeat(64);else if(changed==='bytes')++x.bytes;else{const at=outputMaterialFields.indexOf(changed);for(const row of [x.before,x.after,x.linked])row[at]=String(BigInt(row[at])+1n);}return x;}});
  await assert.rejects(f.save(f.path,f.input),/NEW output (known SHA\/bytes differ|creation identity changed)/);assert.equal(f.reads,1);assert.equal(f.clock,0);assert.equal(f.newFiles.size,0);assert.equal(f.held.size,0);
 }
});
test('new output admission: retained inputs or outputs cannot be refreshed even during prospective sampling',async()=>{
 for(const kind of ['held','newFiles']){
  const f=await newOutputFixture(),original={sha256:'a'.repeat(64),bytes:1,material:['old']};f[kind].set(f.path,original);await assert.rejects(f.save(f.path,f.input),/NEW save cannot replace retained/);assert.equal(f.openCount,0);assert.equal(f[kind].get(f.path),original);
  const race=await newOutputFixture({changeAfterSample:({index,held,newFiles,path})=>{if(index===0)(kind==='held'?held:newFiles).set(path,original);}});await assert.rejects(race.save(race.path,race.input),/NEW output already retained/);assert.equal(race.reads,1);assert.equal(race[kind].get(race.path),original);
  const resolved=await newOutputFixture({changeAfterAdmission:({held,newFiles,path})=>(kind==='held'?held:newFiles).set(path,original)});await assert.rejects(resolved.save(resolved.path,resolved.input),/NEW save cannot replace retained/);assert.equal(resolved.reads,3);assert.equal(resolved[kind].get(resolved.path),original);
 }
 const waited=await newOutputFixture({changeAfterWait:({held,path})=>held.set(path,{old:true})});await assert.rejects(waited.save(waited.path,waited.input),/NEW output already retained/);assert.equal(waited.reads,1);
});
test('new output admission: ongoing ctime churn is bounded and unrelated read errors retain their identity',async()=>{
 const f=await newOutputFixture({samples:(i,stable)=>{const x=stable(200+i);x.after[8]=String(201+i);x.linked[8]=String(201+i);return x;}});await assert.rejects(f.save(f.path,f.input),/NEW output first-admission deadline/);assert.equal(f.clock,29000);assert.equal(f.reads,30);assert.equal(f.newFiles.size,0);
 const failure=Error('injected no-follow/FD error'),failed=await newOutputFixture({readError:failure});await assert.rejects(failed.save(failed.path,failed.input),e=>e===failure);assert.equal(failed.reads,1);assert.equal(failed.clock,0);assert.equal(failed.newFiles.size,0);
});
