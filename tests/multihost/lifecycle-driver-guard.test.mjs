// ROOT-only negative integration test. No actual campaign inputs or network.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,lstat,open,rm,realpath} from 'node:fs/promises';
import {constants} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,dirname,resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
const message='Fresh lifecycle runtime driver is unavailable; managed submission and owner-local terminal audit are required';
const sourcePins={
 'run.mjs':'220a6cdb741690f9a7c0a7e29f0c7476e922c6df4a6156e86476ad39546e35c6',
 'prepare.mjs':'3dc631707379fb30160e518cfbded8d638f641c41b0c2451272050a268090ed7',
 'lifecycle.mjs':'3eedb790f38e9dcf3f3c56a53cfb44f6b86d9e54bdfef76597d8136c9d11c214'
};
const sha=b=>createHash('sha256').update(b).digest('hex');
const fields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'];
const material=s=>fields.map(k=>String(s[k]));
async function stable(path){
 const f=await open(path,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
 try{const before=await f.stat({bigint:true});assert(before.isFile()&&before.nlink===1n&&before.size>0n&&before.size<=1048576n);const bytes=await f.readFile();assert.equal(BigInt(bytes.length),before.size);assert.deepEqual(material(await f.stat({bigint:true})),material(before));assert.deepEqual(material(await lstat(path,{bigint:true})),material(before));return {sha256:sha(bytes),material:material(before)};}finally{await f.close();}
}
const absent=async path=>{try{await lstat(path);}catch(e){if(e.code==='ENOENT')return;throw e;}assert.fail('Forbidden fixture path was created: '+path);};
test('actual lifecycle driver refuses specifically before absent seed, output or network',{timeout:15000},async()=>{
 const run=resolve(process.argv[2]??fileURLToPath(new URL('./run.mjs',import.meta.url))),source=dirname(run),before={};
 for(const [leaf,expected] of Object.entries(sourcePins)){before[leaf]=await stable(join(source,leaf));assert.equal(before[leaf].sha256,expected,'Exact frozen source required');}
 const parent=await realpath(await mkdtemp(join(tmpdir(),'nano-lifecycle-unavailable-'))),missing=[];
 try{
  const artifact=path=>{missing.push(path);return {path,sha256:'d'.repeat(64),bytes:1};};
  const names=['mac-native','mac-docker','vps-docker'],owners={};
  for(let i=0;i<3;++i){const name=names[i],core=join(parent,name+'-build'),network=join(parent,name+'-network'),roles={nano:'nano',p2p:'nano-p2p',bootstrapAudit:'nano-capella-isolated-bootstrap-audit',stoppedCapture:'nano-capella-isolated-stopped-capture',lifecycleTool:'nano-capella-deposit-audit'};
   if(i===0)Object.assign(roles,{exporter:'nano-capella-transition-tests',bootstrapImport:'nano-capella-isolated-bootstrap-import'});
   // /outputs locators are guest-only: test absence only of this NEW fixture.
   const selected=path=>({path,sha256:'d'.repeat(64),bytes:1});
   owners[name]={host:i===2?'ubuntu@127.0.0.1':'mac',kind:['native-macos-arm64','linux-arm64','linux-amd64'][i],image:i===0?null:i===1?'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14',uid:i===2?1000:501,gid:i===2?1000:20,coreBuildRoot:core,networkBuildRoot:network,coreArchive:selected(i===0?core+'/libnano_core.a':'/outputs/libnano_core.a'),programs:Object.fromEntries(Object.entries(roles).map(([role,leaf])=>[role,selected((i===0?(role==='p2p'?network:core):role==='p2p'?'/network-build':'/outputs')+'/'+leaf)])),admission:artifact(join(parent,name+'-missing-admission.json'))};missing.push(core,network);
  }
  const id=(owner,suffix,digit)=>({owner,keyEnv:'NANO_TEST_CAPELLA_NETWORK_KEY_'+suffix,publicKey:'0x'+digit.repeat(96)}),q=n=>'0x'+BigInt(n).toString(16);
  const spec={format:'nano-fresh-capella-three-owner-campaign-v1',sourceBaseRevision:'dff9df7fb8fcef8dd2275ea0f92a7570f14fbc63',overlayManifest:artifact(join(parent,'missing-overlay.json')),owners,fixture:{seedFile:join(parent,'missing-seed'),ledgerDirectory:join(parent,'missing-ledger'),legacyPublicIdentity:artifact(join(parent,'missing-legacy.json'))},clock:{maximumStartupElapsedMs:'0x1',maximumDowntimeMs:'0x1',maximumSteps:'0x1'},backupRestore:false,productionQualified:false,lifecycle:{format:'nano-fresh-validator-lifecycle-v1',identities:{voluntaryExit:id('vps-docker','A','1'),slashing:id('mac-native','B','2')},deadlines:{registrationMs:100000,activationMs:100000,finalityMs:10000,restoreMs:10000,actionMs:30000,auditMs:30000,maximumEpochsAfterActivation:400,maximumRunMs:20000000},resources:{maximumWallSeconds:q(30),maximumHistoryBlocks:q(4000),maximumArchiveBytes:q(50000000),maximumIndexBytes:q(500000),maximumInventoryFiles:q(10000),maximumInventoryBytes:q(512*1024*1024)},syntheticActionAuthorized:true,intentionallyAuthorConflictingHeaders:true,privateVpsLifecycleTransferAuthorized:false}};
  const output=join(parent,'missing-output');missing.push(spec.fixture.seedFile,spec.fixture.ledgerDirectory,output);for(const p of missing)await absent(p);
  const file=join(parent,'synthetic-spec.json'),bytes=Buffer.from(JSON.stringify(spec));await writeFile(file,bytes,{flag:'wx',mode:0o600});const specBefore=await stable(file);
  const env=Object.fromEntries(Object.entries(process.env).filter(([k])=>!k.startsWith('NANO_TEST_')));
  const child=spawnSync(process.execPath,['--max-old-space-size=32','--max-semi-space-size=1',run,'ubuntu@127.0.0.1',spec.sourceBaseRevision,output,'--fresh-campaign',file,'--fresh-campaign-sha',sha(bytes)],{env,encoding:'utf8',timeout:10000,maxBuffer:262144});
  assert.ifError(child.error);assert.equal(child.signal,null);assert.equal(child.status,1);assert.equal(child.stdout,'');assert(child.stderr.includes(message),'A parser/import/ENOENT refusal is not this guard');assert(!child.stderr.includes('ENOENT'));
  for(const p of missing)await absent(p);assert.deepEqual(await stable(file),specBefore);for(const leaf of Object.keys(sourcePins))assert.deepEqual(await stable(join(source,leaf)),before[leaf]);
 }finally{await rm(parent,{recursive:true,force:true});}
});
