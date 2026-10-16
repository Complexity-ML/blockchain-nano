// ROOT-only negative integration test. No actual campaign inputs or network.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,lstat,open,rm,realpath} from 'node:fs/promises';
import {constants} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,dirname,resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {performance} from 'node:perf_hooks';
const message='Fresh lifecycle runtime driver is unavailable; managed submission and owner-local terminal audit are required';
const sourcePins={
 "vps-linux-bootstrap.mjs": "318fcc6fcf0f74c9fb00b4a669a315737973c51a73e352f6ef2cfb23e5583cd9",
 "run.mjs": "b45a9784eca6e88d4bf31cf2e5d3d2117d4e4a188fe5a191db5cf4891b18ad84",
  "native-runtime-env.mjs": "e5c988c7013b0a8b86af10f0f5d51edb46123cc3e73329167bbab6b5f4869119",
 "prepare.mjs": "8cffaff8a204ec276832f96c5a283747de835ca2913bc5ebb37fadb3d3da0b21",
 "program-locators.mjs": "afb6fd53db3930389e2c5adfd6a1c9cb0d59369f5926bc33476c9c7de2829c33",
 "linux-owned-files.mjs": "44b451c7123126181c19bda000a5e688c901a4b8f3f72d4e047a1f7d93f4b869",
 "harness-manifest.mjs": "b01e069ade274ac38e612f9f8eefa7ce17e204b4764593d0371891047374c09d",
 "lifecycle.mjs": "16ceccd02e7487d4424285554d210753aa70943b39714819b32f9c158a2f4a23",
 "lifecycle-runtime.mjs": "eed5d27183d5168b9b3a5efcdb0161632f5b542486c606f9a1232678ae850a72",
 "lifecycle-owner.mjs": "8c3cbb8ee26655b9e76c5e30da308195c9beb48b59f022d3c2094942831e7d65",
 "../../docs/qualification/local-supervisor-linux-20261015/source/known-files.pl": "7dd88b7d99ab15e9f2b1e52aac796265e6ed9bed24323e66584b3f31138cc0f6",
 "../../docs/qualification/local-supervisor-linux-20261015/source/copy-bootstrap-pairs.pl": "9fcd47d3bee3530c163e00e8b6a8dd475645336ba48579db45bdc9e20bb3bdf3"
};
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
test('actual lifecycle driver gates default mode and private authorizations before absent seed/output/network',{timeout:35000},async()=>{
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
