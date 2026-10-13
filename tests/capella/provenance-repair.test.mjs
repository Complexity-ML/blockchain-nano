import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {sourceApprovalPolicy,sha256} from './candidate-source-authority.mjs';
import {candidateSourcePins,digest,verifyCandidateRuntimeBindings,archiveMemberObservationKey} from './candidate-runtime-source.mjs';
const read=name=>readFileSync(new URL('../fixtures/candidate-source/'+name,import.meta.url));
const compileReferenceBytes=read('source-d7-observed-reference.json'),ref=JSON.parse(compileReferenceBytes),coreReferenceBytes=read('core-c672-production-reference.json'),baselineCmakeBytes=read('CMakeLists.core85.txt'),currentCmakeBytes=read('CMakeLists.candidate-c672.txt');
function sourceFixture(){
 const future=JSON.parse(compileReferenceBytes),path='tests/capella/isolated-three-node-run.mjs',row=future.files.find(x=>x.path===path),fromSha256=row.sha256,bytes=Buffer.from('// synthetic externally reviewed overlay policy model');
 row.bytes=bytes.length;row.sha256=sha256(bytes);row.gitBlobSha1=createHash('sha1').update('blob '+bytes.length+'\0').update(bytes).digest('hex');future.snapshotRevision='a'.repeat(40);
 const sourceReferenceBytes=Buffer.from(JSON.stringify(future)),authority={format:'nano-root-qualified-source-authority-v1',coreSourceRevision:sourceApprovalPolicy.coreSourceRevision,testSourceRevision:sourceApprovalPolicy.testSourceRevision,futureRepoRevision:future.snapshotRevision,sourceReferenceSha256:sha256(sourceReferenceBytes),compileSourceReferenceSha256:sha256(compileReferenceBytes),approvedControllerOverlays:[{path,fromSha256,toSha256:row.sha256}]},authorityBytes=Buffer.from(JSON.stringify(authority));
 return {authority,args:{authorityBytes,expectedAuthoritySha256:sha256(authorityBytes),compileReferenceBytes,sourceReferenceBytes,coreReferenceBytes,baselineCmakeBytes,currentCmakeBytes,checkout:{head:future.snapshotRevision,status:'',files:future.files,ignoredPaths:future.knownUnconsumedIgnoredPaths}}};
}
function fixture(){
 const source=sourceFixture(),args=source.args,publicInputs=new Map(),observations=new Map(),assets={format:'nano-isolated-candidate-runtime-assets-v2',repositoryRevision:source.authority.futureRepoRevision,testSourceRevision:sourceApprovalPolicy.testSourceRevision,coreRevision:candidateSourcePins.revision,sourceReferenceSha256:source.authority.sourceReferenceSha256,compileSourceReferenceSha256:source.authority.compileSourceReferenceSha256,cmakeBaselineSha256:digest(baselineCmakeBytes),currentCmakeSha256:digest(currentCmakeBytes),assets:{}};
 const metadata=(path,text)=>{const bytes=Buffer.from(text);publicInputs.set(path,bytes);return {path,sha256:digest(bytes)};};
 const artifact=(owner,path)=>{const fake=Buffer.from('SYNTHETIC MODEL ONLY '+owner+path),row={path,bytes:fake.length,sha256:digest(fake)};observations.set(owner+'\0'+path,row.sha256);return row;};
 const header=ref.files.find(x=>x.path==='include/nano/consensus_capella_duty.hpp'),reactor=ref.files.find(x=>x.path==='include/nano/consensus_capella_reactor.hpp');
 const sources={nano:'src/main.cpp',bootstrapAudit:'tests/capella/isolated_bootstrap_audit.cpp',stoppedCapture:'tests/capella/isolated_stopped_capture.cpp',pausedCapture:'tests/capella/paused_capture.cpp'};
 for(const owner of ['mac-native','mac-docker','vps-docker']){
  const a={executionEnvironment:{'mac-native':'native-macos-arm64','mac-docker':'linux-arm64','vps-docker':'linux-amd64'}[owner],coreArchive:artifact(owner,'/synthetic/'+owner+'/libnano_core.a'),nano:artifact(owner,'/synthetic/'+owner+'/nano'),p2p:artifact(owner,'/synthetic/'+owner+'/nano-p2p'),helpers:{}};
  for(const role of Object.keys(sources).filter(x=>x!=='nano'))a.helpers[role]=artifact(owner,'/synthetic/'+owner+'/'+role);
  const unit=source=>{
   const deps=[source,header.path].filter((x,i,a)=>a.indexOf(x)===i).sort();return {source,cwd:'/work',archiveSha256:a.coreArchive.sha256,flags:metadata(owner+'/flags/'+source,'-O3 -DNDEBUG -std=c++20'),command:metadata(owner+'/command/'+source,'clang++ -c /work/'+source+' -o /synthetic/'+owner+'/'+source+'.o'),depfile:metadata(owner+'/dep/'+source,'object.o: '+deps.map(x=>'/work/'+x).join(' ')+'\n'),dependencies:deps.map(path=>({path,sha256:ref.files.find(x=>x.path===path).sha256})),object:artifact(owner,'/synthetic/'+owner+'/'+source+'.o')};
  };
  const proof={format:'nano-coherent-candidate-build-binding-v1',owner,candidateRevision:candidateSourcePins.revision,sourceReferenceSha256:source.authority.compileSourceReferenceSha256,buildCmakeSha256:digest(currentCmakeBytes),sourceRoot:'/work',archive:a.coreArchive,dutyHeaderSha256:header.sha256,reactorHeaderSha256:reactor.sha256,coreUnits:ref.expectedCoreTranslationUnits.map(unit),consumers:{}};
  proof.archiveMembers=metadata(owner+'/actual-ar-t.txt',(owner==='mac-native'?'__.SYMDEF\n':'')+proof.coreUnits.map(x=>x.object.path.split('/').at(-1)).join('\n')+'\n');

  const memberRows=proof.coreUnits.map((u,ordinal)=>({ordinal,name:u.object.path.split('/').at(-1),objectPath:u.object.path,bytes:u.object.bytes,sha256:u.object.sha256}));
  for(const row of memberRows)observations.set(archiveMemberObservationKey(owner,a.coreArchive,row.ordinal,row.name),row.sha256);
  proof.archiveMemberHashInventory=metadata(owner+'/member-hashes.json',JSON.stringify({format:'nano-ar-member-sha256-inventory-v1',archive:a.coreArchive,archiveSha256Before:a.coreArchive.sha256,archiveSha256After:a.coreArchive.sha256,collectionMethod:'ar-p-member-bytes',readOnly:true,members:memberRows}));
  for(const [name,source] of Object.entries(sources)){const u=unit(source);u.binary=name==='nano'?a.nano:a.helpers[name];u.linkedCoreArchives=[a.coreArchive];u.linkCwd='/work';u.linkObjects=[u.object,artifact(owner,'/synthetic/'+owner+'/third_party/mcl/bint64.o'),artifact(owner,'/synthetic/'+owner+'/third_party/mcl/base64.o')];u.link=metadata(owner+'/link/'+name,'clang++ '+u.linkObjects.map(x=>x.path).join(' ')+' '+a.coreArchive.path+' -o '+u.binary.path);proof.consumers[name]=u;}
  a.buildBinding=metadata(owner+'/proof.json',JSON.stringify(proof));assets.assets[owner]=a;
 }
 const profileBytes=Buffer.from('{"rules":"unchanged synthetic profile"}'),templateBytes=Buffer.from('{"clock":{"maximumForwardStepMs":"0x1d4c0","replaySlots":"0x40"}}');
 assets.profileSha256=digest(profileBytes);assets.templateSha256=digest(templateBytes);
 Object.assign(args,{publicInputs,observations,profileBytes,expectedProfileSha256:digest(profileBytes),templateBytes,expectedTemplateSha256:digest(templateBytes)});
 const seal=()=>{args.assetsBytes=Buffer.from(JSON.stringify(assets));args.expectedAssetsSha256=digest(args.assetsBytes);};seal();
 const changeProof=(owner,fn)=>{const row=assets.assets[owner].buildBinding,p=JSON.parse(publicInputs.get(row.path));fn(p);assets.assets[owner].buildBinding=metadata(row.path,JSON.stringify(p));seal();};
 return {args,assets,seal,changeProof,metadata};
}


const verify=f=>verifyCandidateRuntimeBindings(f.args);
function changeInventory(f,owner,fn){f.changeProof(owner,p=>{const row=p.archiveMemberHashInventory,doc=JSON.parse(f.args.publicInputs.get(row.path));fn(doc);p.archiveMemberHashInventory=f.metadata(row.path,JSON.stringify(doc));});}
test('new provenance gate binds all139 member bytes and consumer plus MCL direct objects/output without enabling runtime',()=>{const r=verify(fixture());assert.equal(r.ownerBindings.length,3);assert.equal(r.runtimeEnabled,false);});
test('archive operand alone cannot hide a substituted consumer object or output',()=>{for(const mutate of [(f,p)=>{const u=p.consumers.nano;u.link=f.metadata('substituted-object','clang++ /old/substitute.o '+p.archive.path+' -o '+u.binary.path);},(f,p)=>{const u=p.consumers.nano;u.link=f.metadata('wrong-output','clang++ '+u.linkObjects.map(x=>x.path).join(' ')+' '+p.archive.path+' -o /wrong/nano');}]){const f=fixture();f.changeProof('mac-native',p=>mutate(f,p));assert.throws(()=>verify(f),/Actual consumer link object list differs|Actual consumer link output differs/);}});
test('same member names and repinned metadata cannot fake observed member bytes or object paths',()=>{for(const fn of [doc=>doc.members[0].sha256='0'.repeat(64),doc=>doc.members[0].objectPath='/old/object.o',doc=>doc.members[0].bytes++]){const f=fixture();changeInventory(f,'mac-native',fn);assert.throws(()=>verify(f),/Member bytes differ|Member object path differs|byte length differs/);}const f=fixture();f.args.observations.set(archiveMemberObservationKey('mac-native',f.assets.assets['mac-native'].coreArchive,0,ref.expectedCoreTranslationUnits[0].split('/').at(-1)+'.o'),'0'.repeat(64));assert.throws(()=>verify(f),/Independent actual member-byte observation differs/);});
test('ordinal order and duplicate basename ambiguity are refused before claiming archive binding',()=>{const f=fixture();changeInventory(f,'mac-docker',doc=>doc.members[1].ordinal=0);assert.throws(()=>verify(f),/ordinal differs/);const g=fixture();g.changeProof('mac-docker',p=>{p.coreUnits[1].object.path='/other/'+p.coreUnits[0].object.path.split('/').at(-1);p.archiveMembers=g.metadata('duplicate-ar-name',p.coreUnits.map(x=>x.object.path.split('/').at(-1)).join('\n')+'\n');});assert.throws(()=>verify(g),/Ambiguous duplicate archive object basenames/);});
test('archive collection must preserve before/after archive SHA and cannot omit actual byte inventory',()=>{const f=fixture();changeInventory(f,'vps-docker',doc=>doc.archiveSha256After='0'.repeat(64));assert.throws(()=>verify(f),/final archive changed/);const g=fixture();g.changeProof('vps-docker',p=>delete p.archiveMemberHashInventory);assert.throws(()=>verify(g),/actual member bytes inventory public binding required/);});
test('MCL direct object observation remains independent and missing or duplicate direct objects cannot be asserted away',()=>{const f=fixture();f.args.observations.delete('mac-native\0/synthetic/mac-native/third_party/mcl/bint64.o');assert.throws(()=>verify(f),/actual direct link object actual artifact observation differs/);const g=fixture();g.changeProof('mac-native',p=>p.consumers.nano.linkObjects.push(p.consumers.nano.linkObjects[0]));assert.throws(()=>verify(g),/Duplicate direct link object paths/);});
