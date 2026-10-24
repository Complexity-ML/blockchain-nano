import test from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import {readFileSync,mkdtempSync,writeFileSync,lstatSync,rmSync,unlinkSync,linkSync,renameSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join as joinPath} from 'node:path';
import {vpsProposalFloor,validateVpsDutyFloorPair,vpsDutyFloorProjectionPython} from './vps-duty-floor.mjs';
import {lifecycleOwnerProjection} from './lifecycle-owner.mjs';
const sha=b=>createHash('sha256').update(b).digest('hex');
const j=v=>JSON.stringify(v),clone=v=>structuredClone(v),root=n=>'0x'+BigInt(n).toString(16).padStart(64,'0'),pub=n=>'0x'+BigInt(n+1).toString(16).padStart(96,'0'),token=n=>({sequence:'0x1',digest:root(n)});
const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':v!==null&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
const metadata=source=>{const start=source.indexOf('def need(v):'),end=source.indexOf('# Creator anchors');assert(start>=0&&end>start);return source.slice(start,end);};
// This executes the ACTUAL Python projection and ACTUAL existing owner reader
// against newly-created synthetic files, never a capsule/DB/runtime from Nano.
const python=String.raw`import os,sys,json,hashlib,stat,pathlib,re,tempfile
payload=json.loads(sys.stdin.buffer.read())
with tempfile.TemporaryDirectory(prefix='nano-vps-floor-unit-') as directory:
 root=pathlib.Path(directory);fields=('st_dev','st_ino','st_mode','st_uid','st_gid','st_nlink','st_size','st_mtime_ns','st_ctime_ns')
 exec(payload['reader'],globals());exec(payload['projection'],globals())
 request=payload['request'];paths={}
 for key,raw in payload['files'].items():
  p=root/(key+'.json');p.write_text(raw);os.chmod(p,0o644 if key=='manifest' else 0o600);paths[key]=p;request[key]['path']=str(p)
 control=payload.get('control',{})
 physical={}
 for key in ('sourceCapture','restoredCapture','stop'):
  physical[key]=sha(canon(mat(os.lstat(paths[key]))));request[key]['materialSha256']=physical[key]
 if control.get('wrongMaterial'):request['sourceCapture']['materialSha256']='0'*64
 if control.get('retimeBeforeRead'):
  p=paths['sourceCapture'];s=os.lstat(p);os.utime(p,ns=(s.st_atime_ns,s.st_mtime_ns+1000000000))
 if control.get('hardlink'):os.unlink(paths['restoredCapture']);os.link(paths['sourceCapture'],paths['restoredCapture'])
 if control.get('symlink'):os.unlink(paths['restoredCapture']);os.symlink(paths['sourceCapture'],paths['restoredCapture'])
 original_read=read;reads=0
 def read(*args,**kwargs):
  global reads
  reads+=1
  if control.get('mutateAtRead')==reads:
   p=paths['sourceCapture'];p.write_text(p.read_text()+' ')
  return original_read(*args,**kwargs)
 try:result=vps_floor_project_pair(request);print(json.dumps({'passed':True,'result':result,'reads':reads,'physicalPins':physical}))
 except BaseException:print(json.dumps({'passed':False,'reads':reads,'physicalPins':physical}))
`;
function fixture({selected=64,head=79,proposal='0x50',sameCapsules=false}={}){
 const manifest={format:'nano-isolated-capella-fixture-domain-v1',campaignId:'a'.repeat(64),registryCount:64,publicKeys:Array.from({length:65},(_,index)=>({index,publicKey:pub(index)})),genesisValidatorsRoot:root(900),bootstrapSigningHistory:{deposit:{publicKey:pub(64)}}};
 const finalized={epoch:'0x8',root:root(901)},witnessOriginSha256='b'.repeat(64),keys=Array.from({length:33},(_,offset)=>{const index=32+offset,indexField=index===manifest.registryCount?null:'0x'+index.toString(16),p={slot:proposal,root:root(9999)},snapshot=canonical({format:'nano-beacon-signing-v1',publicKey:pub(index),genesisValidatorsRoot:manifest.genesisValidatorsRoot,slotsPerEpoch:'0x8',proposal:proposal===null?null:p,attestation:null}),primary={sequence:'0x1',digest:'0x'+sha(snapshot)},witnessSnapshot=canonical({format:'nano-beacon-signing-witness-v1',previous:null,next:primary,snapshot}),witness={sequence:'0x1',digest:'0x'+sha(witnessSnapshot)};return {publicKeyIndex:index,index:indexField,publicKey:pub(index),primary,witness,witnessNext:primary,restart:primary,snapshot,witnessSnapshot,retired:false,registeredInCurrentHead:true,proposal:proposal===null?null:p};});
 const core={primary:token(100),restart:token(100),witness:token(101),witnessNext:token(100)};
 const stoppedKeys=kind=>keys.map(k=>({index:k.index,...(k.index===null?{publicKey:k.publicKey}:{}),restart:k[kind]}));
 const stop={event:'capella-runtime-stopped',clockHealthy:true,validator:{healthy:true,failure:null,localKeys:33},restart:core.primary,finalized,validatorKeys:stoppedKeys('primary'),witnessCommitments:{format:'nano-isolated-stopped-external-witness-commitments-v1',sourceCaptureSha256:witnessOriginSha256,core:core.witness,validatorKeys:stoppedKeys('witness')}};
 const capture={format:'nano-isolated-stopped-recovery-pins-v1',passed:true,owner:'vps-docker',campaignId:manifest.campaignId,manifestSha256:sha(j(manifest)),externalStopReceiptSha256:sha(j(stop)),restart:core.primary,recoveryWitnessCommitment:core.witness,core,validatorKeys:keys,headSlot:head,finalized,registryCount:65,primaryWitnessPairs:33,databasesReadOnly:68,canonicalStoreSnapshotVerified:true,primaryAndWitnessBlobClosuresVerified:true,allObservedFileBytesAndInodesUnchanged:true,observedWitnessPairsAuthenticated:true,externalStopTokensMatched:true,externalWitnessTokensMatched:true,externalWitnessPinOriginExternallyRequired:true,privateKeysRead:false,envRead:false,journalWritesRequested:false,runtimeStarted:false,networkUsed:false,productionQualified:false,signerCallbacksInvoked:0,newSignaturesCreated:0};
 const files={sourceCapture:j(capture),restoredCapture:j({...capture,...(sameCapsules?{}:{configurationSha256:'c'.repeat(64)})}),stop:j(stop),manifest:j(manifest)};
 const request={mode:'vps-duty-floor-pair',...Object.fromEntries(Object.entries(files).map(([k,b])=>[k,{path:'/owner/'+k+'.json',sha256:sha(b),bytes:Buffer.byteLength(b)}])),campaignId:manifest.campaignId,selectedPublicKey:pub(selected),selectedPreparedPublicKeyIndex:selected,selectedIsPending:selected===manifest.registryCount,resolvedIndex:'0x'+selected.toString(16),expectedHeadSlot:head,expectedFinalized:finalized,witnessOriginSha256};return {files,request};
}
function replaceDocument(f,key,change){const d=JSON.parse(f.files[key]);change(d);f.files[key]=j(d);f.request[key].sha256=sha(f.files[key]);f.request[key].bytes=Buffer.byteLength(f.files[key]);}
function project(f,control={}){const out=JSON.parse(execFileSync('python3',['-c',python],{input:j({...f,control,reader:metadata(lifecycleOwnerProjection),projection:vpsDutyFloorProjectionPython}),timeout:5000,maxBuffer:1048576,encoding:'utf8'}));for(const [key,value] of Object.entries(out.physicalPins))f.request[key].materialSha256=value;return out;}
const accepted=f=>{const r=project(f);assert.equal(r.passed,true);return validateVpsDutyFloorPair(r.result,f.request);};

test('VPS floor arithmetic consumes safe numeric C++ heads and canonical journal slots only',()=>{
 assert.equal(vpsProposalFloor(79,'0x50'),'0x50');assert.equal(vpsProposalFloor(79,'0x3c'),'0x4f');assert.equal(vpsProposalFloor(79,null),'0x4f');assert.equal(vpsProposalFloor(0,'0x0'),'0x0');
 for(const n of [-1,1.5,NaN,Infinity,Number.MAX_SAFE_INTEGER+1,'79',79n,true])assert.throws(()=>vpsProposalFloor(n,'0x1'));
 for(const slot of ['79','0x01','0X1',root(1),'0x10000000000000000',79,true,{}])assert.throws(()=>vpsProposalFloor(79,slot));
});

test('actual owner Python projects registered pending and provisioned identities without exporting signing roots',()=>{
 const pending=accepted(fixture());assert.equal(pending.source.selectedProvisionedIndex,null);assert.equal(pending.source.selectedResolvedIndex,'0x40');assert.equal(pending.source.cutoffSlot,'0x50');
 assert(!j(pending).includes(root(9999)));assert(!Object.hasOwn(pending.source,'snapshot'));assert(!Object.hasOwn(pending.source,'proposalRoot'));
 const provisioned=accepted(fixture({selected:32,head:500,proposal:'0x156'}));assert.equal(provisioned.source.selectedProvisionedIndex,'0x20');assert.equal(provisioned.source.cutoffSlot,'0x1f4');
 assert.equal(accepted(fixture({proposal:null})).source.journalProposalSlot,null);
});

test('actual owner Python refuses substituted, duplicate, unregistered and unwitnessed selected journals',()=>{
 const changes=[
  d=>{d.owner='mac-docker';},d=>{d.campaignId='d'.repeat(64);},d=>{d.manifestSha256='d'.repeat(64);},d=>{d.externalStopReceiptSha256='d'.repeat(64);},
  d=>{d.validatorKeys[32].publicKey=pub(1);},d=>{d.validatorKeys[32].registeredInCurrentHead=false;},d=>{d.validatorKeys[32].retired=true;},d=>{d.validatorKeys[32].index='0x40';},
  d=>{d.validatorKeys[31].index=null;},d=>{d.validatorKeys[31]=clone(d.validatorKeys[32]);},d=>{d.validatorKeys[32].publicKeyIndex=63;},d=>{d.validatorKeys[32].witness=token(999);},
  d=>{d.core.witnessNext=token(999);},d=>{d.headSlot=79.5;},d=>{d.headSlot=Number.MAX_SAFE_INTEGER+1;},d=>{d.validatorKeys[32].proposal={root:root(3)};},
  d=>{const row=d.validatorKeys[32];row.proposal.slot='0x00050';},d=>{const row=d.validatorKeys[32];row.witnessSnapshot=j({...JSON.parse(row.witnessSnapshot),snapshot:'{}'});}
 ];
 for(const change of changes){const f=fixture();replaceDocument(f,'sourceCapture',change);assert.equal(project(f).passed,false,String(change));}
 const wrongIndex=fixture();wrongIndex.request.resolvedIndex='0x3f';assert.equal(project(wrongIndex).passed,false);
 const fakePending=fixture({selected:32});fakePending.request.selectedIsPending=true;assert.equal(project(fakePending).passed,false);
 const absentOrigin=fixture();absentOrigin.request.witnessOriginSha256='d'.repeat(64);assert.equal(project(absentOrigin).passed,false);
 const copiedJournalAhead=fixture();replaceDocument(copiedJournalAhead,'restoredCapture',d=>{const row=d.validatorKeys[32];row.proposal.slot='0x51';row.snapshot=j({...JSON.parse(row.snapshot),proposal:row.proposal});row.witnessSnapshot=j({...JSON.parse(row.witnessSnapshot),snapshot:row.snapshot});});assert.equal(project(copiedJournalAhead).passed,false);
 const correlated=fixture();for(const key of ['sourceCapture','restoredCapture'])replaceDocument(correlated,key,d=>{const row=d.validatorKeys[32];row.proposal.slot='0x51';row.snapshot=canonical({...JSON.parse(row.snapshot),proposal:row.proposal});row.witnessSnapshot=canonical({...JSON.parse(row.witnessSnapshot),snapshot:row.snapshot});});assert.equal(project(correlated).passed,false,'both floors cannot move without the exact external primary/witness digests');
});

test('actual owner reader and parser refuse duplicate JSON, changed pins, aliases and late source modification',()=>{
 const duplicate=fixture();duplicate.files.sourceCapture=duplicate.files.sourceCapture.replace('"headSlot":79','"headSlot":78,"headSlot":79');duplicate.request.sourceCapture.sha256=sha(duplicate.files.sourceCapture);duplicate.request.sourceCapture.bytes=Buffer.byteLength(duplicate.files.sourceCapture);assert.equal(project(duplicate).passed,false);
 const nested=fixture();nested.files.sourceCapture=nested.files.sourceCapture.replace('"registeredInCurrentHead":true','"registeredInCurrentHead":false,"registeredInCurrentHead":true');nested.request.sourceCapture.sha256=sha(nested.files.sourceCapture);nested.request.sourceCapture.bytes=Buffer.byteLength(nested.files.sourceCapture);assert.equal(project(nested).passed,false);
 const wrongSha=fixture();wrongSha.request.sourceCapture.sha256='0'.repeat(64);assert.equal(project(wrongSha).passed,false);
 const wrongBytes=fixture();++wrongBytes.request.stop.bytes;assert.equal(project(wrongBytes).passed,false);
 assert.equal(project(fixture(),{wrongMaterial:true}).passed,false);assert.equal(project(fixture(),{retimeBeforeRead:true}).passed,false);
 assert.equal(project(fixture({sameCapsules:true}),{hardlink:true}).passed,false);assert.equal(project(fixture({sameCapsules:true}),{symlink:true}).passed,false);
 const changed=project(fixture(),{mutateAtRead:5});assert.equal(changed.passed,false);assert.equal(changed.reads,5);
});

test('public typed validator refuses replay, altered floor, changed identity and extra authority',()=>{
 const f=fixture(),good=accepted(f);
 for(const change of [v=>{v.source.cutoffSlot='0x4f';},v=>{v.source.selectedResolvedIndex='0x3f';},v=>{v.source.selectedPublicKey=pub(32);},v=>{v.source.rawCapsuleExported=true;},v=>{v.restored.selectedWitnessCommitmentSha256='d'.repeat(64);},v=>{v.source.proposalRoot=root(99);},v=>{v.source.headSlot='0x50';},v=>{v.source.indexFromAuthenticatedCapturedRegistry=false;},v=>{v.source.primaryWitnessPairs=32;},v=>{v.source.runtimeStarted=true;}]){const value=clone(good);change(value);assert.throws(()=>validateVpsDutyFloorPair(value,f.request));}
 const later=clone(f.request);later.stop.sha256='d'.repeat(64);assert.throws(()=>validateVpsDutyFloorPair(good,later));
 const otherCapsule=clone(f.request);otherCapsule.sourceCapture.sha256='e'.repeat(64);assert.throws(()=>validateVpsDutyFloorPair(good,otherCapsule));
});

test('real proposed caller binds prepared identity and both captures before publication and resume',async()=>{
 const source=readFileSync(new URL('./run.mjs',import.meta.url),'utf8'),marker=' async function lifecycleVpsDutyFloor(',endMarker='\n async function lifecycleRestore(',start=source.indexOf(marker),end=source.indexOf(endMarker,start);
 assert.equal(source.split(marker).length,2);assert.equal(source.split(endMarker).length,2);assert(start>=0&&end>start);
 const restoreStart=end,restoreEnd=source.indexOf('\n function lifecycleSourceOnlyCopyFence(',restoreStart);assert(restoreEnd>restoreStart);const restore=source.slice(restoreStart,restoreEnd);
 const floorCall='const dutyFloor=await lifecycleVpsDutyFloor(id,beforeProposal,captured,restored,prefix);',resume='await lifecycleResume(name,before.receipt)';assert.equal(restore.split(floorCall).length,2);assert.equal(restore.split(resume).length,2);assert(restore.indexOf(floorCall)<restore.indexOf(resume));assert(restore.includes('cutoffSlot:dutyFloor.source.cutoffSlot'));
 const matcher="resolvedIndex:'0x'+BigInt(index).toString(16)";assert.equal(source.split(matcher).length,2);
 const f=fixture(),m=JSON.parse(f.files.manifest),events=[],result=accepted(f),captured={name:'vps-docker',capturePath:f.request.sourceCapture.path,captureArtifact:f.request.sourceCapture,captureMaterialSha256:f.request.sourceCapture.materialSha256,stopArtifact:f.request.stop,stopMaterialSha256:f.request.stop.materialSha256,headSlot:79,finalized:f.request.expectedFinalized,publicCapture:{firstCaptureSha256:f.request.witnessOriginSha256}},restored={name:'vps-docker',capturePath:f.request.restoredCapture.path,captureArtifact:f.request.restoredCapture,captureMaterialSha256:f.request.restoredCapture.materialSha256};
 const need=(ok,label)=>assert(ok,label),join=(...parts)=>parts.join('/'),preparedManifest={nodes:[{name:'vps-docker',signerIdentities:[{publicKey:pub(64)}]}]};
 const build=({remote=async request=>{events.push('read-pair');assert.equal(request.selectedPreparedPublicKeyIndex,64);assert.equal(request.selectedIsPending,true);assert.equal(request.resolvedIndex,'0x40');assert.equal(request.sourceCapture.sha256,f.request.sourceCapture.sha256);assert.equal(request.restoredCapture.sha256,f.request.restoredCapture.sha256);return result;},save=async()=>events.push('save')}={})=>new Function('need','lifecycleCheck','preparedManifest','manifest','observe','join','assets','configAt','sha','lifecycleRemote','validateVpsDutyFloorPair','save','output',source.slice(start,end)+';return lifecycleVpsDutyFloor;')(need,()=>events.push('check'),preparedManifest,m,async()=>Buffer.from(f.files.manifest),join,'assets',()=>'/owner',sha,remote,validateVpsDutyFloorPair,save,'output');
 const id={owner:'vps-docker',publicKey:pub(64)},before={resolvedIndex:'0x40'};assert.equal((await build()(id,before,captured,restored,'phase')).source.cutoffSlot,'0x50');assert(events.indexOf('read-pair')<events.indexOf('save'));
 let saves=0;await assert.rejects(build({remote:async()=>{throw Error('refused');},save:async()=>++saves})(id,before,captured,restored,'phase'));assert.equal(saves,0);
 await assert.rejects(build()({...id,publicKey:pub(1)},before,captured,restored,'phase'));
});

test('entire owner dispatcher publishes original capture-pin fingerprints and consumes them in the pair route',()=>{
 assert.equal(process.getuid(),1000,'the REAL owner dispatcher requires uid1000; do not mock geteuid');
 const keys=['sourceCapture','restoredCapture','stop','manifest'],statFields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'];
 const fullNineSha=path=>sha(JSON.stringify(statFields.map(k=>String(lstatSync(path,{bigint:true})[k]))));
 const dispatch=(directory,request)=>{
  const stdout=execFileSync('python3',['-c',lifecycleOwnerProjection,directory,j(request)],{timeout:5000,maxBuffer:1048576,encoding:'utf8',env:{PATH:process.env.PATH,LANG:'C',PYTHONDONTWRITEBYTECODE:'1'}});
  const value=JSON.parse(stdout);assert.equal(value.format,'nano-fresh-lifecycle-owner-public-v1');assert(!stdout.includes(root(9999)),'no proposal signing root');assert(!stdout.includes('"snapshot"'),'no capsule snapshot');assert(!stdout.includes('witnessSnapshot'),'no witness snapshot');assert(!stdout.includes(directory),'no owner path');return value;
 };
 for(const mutation of ['none','same-bytes-substitution','hardlink-alias']){
  const f=fixture(),directory=mkdtempSync(joinPath(tmpdir(),'nano-vps-floor-dispatch-'));
  try{
   for(const key of keys){const path=joinPath(directory,key+'.json');writeFileSync(path,f.files[key],{flag:'wx',mode:0o600});f.request[key].path=path;assert.equal(f.request[key].sha256,sha(readFileSync(path)));assert.equal(f.request[key].bytes,readFileSync(path).length);}
   const original=Object.fromEntries(keys.map(key=>[key,fullNineSha(f.request[key].path)]));
   const capturePin=key=>{
    const outer=dispatch(directory,{mode:'capture-pin',capturePath:f.request[key].path,captureSha256:f.request[key].sha256,stopPath:f.request.stop.path});assert.equal(outer.passed,true);const pin=outer.result;
    assert.equal(pin.captureBytes,f.request[key].bytes);assert.equal(pin.captureMaterialSha256,original[key]);assert.equal(pin.stopSha256,f.request.stop.sha256);assert.equal(pin.stopBytes,f.request.stop.bytes);assert.equal(pin.stopMaterialSha256,original.stop);assert.equal(pin.headSlot,79);assert.deepEqual(pin.finalized,f.request.expectedFinalized);return pin;
   };
   const source=capturePin('sourceCapture'),restored=capturePin('restoredCapture');
   f.request.sourceCapture.materialSha256=source.captureMaterialSha256;f.request.restoredCapture.materialSha256=restored.captureMaterialSha256;f.request.stop.materialSha256=source.stopMaterialSha256;
   for(const key of keys)assert.equal(fullNineSha(f.request[key].path),original[key],'capture-pin never changes known input full-nine');
   if(mutation==='same-bytes-substitution'){const substitute=joinPath(directory,'substitute.json');writeFileSync(substitute,f.files.sourceCapture,{flag:'wx',mode:0o600});renameSync(substitute,f.request.sourceCapture.path);assert.equal(sha(readFileSync(f.request.sourceCapture.path)),f.request.sourceCapture.sha256);assert.notEqual(fullNineSha(f.request.sourceCapture.path),original.sourceCapture);}
   if(mutation==='hardlink-alias'){unlinkSync(f.request.restoredCapture.path);linkSync(f.request.sourceCapture.path,f.request.restoredCapture.path);}
   const pair=dispatch(directory,f.request);
   if(mutation==='none'){
    assert.equal(pair.passed,true);const result=validateVpsDutyFloorPair(pair.result,f.request);assert.equal(result.source.capsule.materialSha256,original.sourceCapture);assert.equal(result.restored.capsule.materialSha256,original.restoredCapture);assert.equal(result.source.stop.materialSha256,original.stop);assert.equal(result.source.cutoffSlot,'0x50');
    for(const key of keys)assert.equal(fullNineSha(f.request[key].path),original[key],'pair route leaves all four original full-nine rows intact');
   }else{assert.deepEqual(pair,{format:'nano-fresh-lifecycle-owner-public-v1',passed:false});}
  }finally{rmSync(directory,{recursive:true,force:true});}
 }
});
