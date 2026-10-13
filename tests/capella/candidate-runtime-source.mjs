import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {posix} from 'node:path';
import {verifyApprovedCandidateCheckout,sourceApprovalPolicy,qualifiedArchiveObjectMembers} from './candidate-source-authority.mjs';
export const candidateSourcePins=Object.freeze({revision:sourceApprovalPolicy.coreSourceRevision,buildCmakeVariants:Object.freeze(['20bff51087f0772434eac798fec4bf1eda4ca6b57c4b2b036d4ae2a24310f8f5','28fe657bbf1a54ad5663f64024d95f90eeb8a5e5fdd25943f0a684042e66f0ec'])});
export const digest=b=>createHash('sha256').update(b).digest('hex');
const hex=x=>typeof x==='string'&&/^[0-9a-f]{64}$/.test(x);
const same=(a,b,label)=>assert.deepEqual(a,b,label);
function parsed(bytes,pin,label){assert.ok(hex(pin),label+' explicit external SHA256 pin required');assert.ok(Buffer.isBuffer(bytes)&&bytes.length<=2*1024*1024,label+' bounded public bytes required');assert.equal(digest(bytes),pin,label+' SHA256 differs');return JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(bytes));}
const indexed=rows=>new Map(rows.map(row=>[row.path,row]));
export function projectMakeDependencies(bytes,cwd,sourceRoot){
 assert.ok(typeof cwd==='string'&&cwd.startsWith('/')&&typeof sourceRoot==='string'&&sourceRoot.startsWith('/'),'Compiler cwd/sourceRoot required');
 const slash=String.fromCharCode(92),text=new TextDecoder('utf-8',{fatal:true}).decode(bytes).replaceAll(slash+'\r\n','').replaceAll(slash+'\n','');
 const colon=text.indexOf(':');assert.ok(colon>0,'Compiler depfile rule missing');
 const tokens=[];let token='',escape=false;
 for(const c of text.slice(colon+1)){
  if(escape){token+=c;escape=false;continue;}if(c.charCodeAt(0)===92){escape=true;continue;}
  assert.notEqual(c,':','Multiple/unhandled compiler dependency rules');
  if(/\s/.test(c)){if(token)tokens.push(token);token='';}else token+=c;
 }
 assert.ok(!escape,'Trailing compiler dependency escape');if(token)tokens.push(token);
 const root=posix.resolve(sourceRoot),paths=[];
 for(const token of tokens){const path=posix.resolve(cwd,token);if(path.startsWith(root+'/'))paths.push(path.slice(root.length+1));}
 return [...new Set(paths)].sort();
}

function shellWords(bytes){
 const text=new TextDecoder('utf-8',{fatal:true}).decode(bytes);assert.ok(!text.includes('$')&&!text.includes('`'),'Unexpanded link command refused');
 const words=[];let word='',quote=null,escape=false;
 for(const c of text){if(escape){word+=c;escape=false;continue;}if(c.charCodeAt(0)===92&&quote!=="'"){escape=true;continue;}if(quote){if(c===quote)quote=null;else word+=c;continue;}if(c==='"'||c==="'"){quote=c;continue;}if(/\s/.test(c)){if(word)words.push(word);word='';}else word+=c;}
 assert.ok(!quote&&!escape,'Unterminated link command');if(word)words.push(word);
 return words;
}
export function linkedCoreArchives(bytes,cwd){
 assert.ok(typeof cwd==='string'&&cwd.startsWith('/'),'Actual link cwd required');
 const matches=shellWords(bytes).filter(x=>x.includes('libnano_core.a'));assert.ok(matches.every(x=>posix.basename(x)==='libnano_core.a'&&!x.startsWith('-')),'Unsupported core archive link operand');return matches.map(x=>posix.resolve(cwd,x));
}


export function archiveMemberObservationKey(owner,archive,ordinal,name){return owner+'\0ar-member\0'+archive.path+'\0'+archive.sha256+'\0'+ordinal+'\0'+name;}
export function actualConsumerLinkObjects(bytes,cwd){
 const words=shellWords(bytes);assert.equal(words.filter(x=>x==='-o').length,1,'One actual consumer link output required');
 const at=words.indexOf('-o');assert.ok(at+1<words.length&&!words[at+1].startsWith('-'),'Actual consumer link output absent');
 const operands=words.filter(x=>x.endsWith('.o'));assert.ok(operands.every(x=>!x.startsWith('-')),'Unsupported object link operand');
 return {objects:operands.map(x=>posix.resolve(cwd,x)),output:posix.resolve(cwd,words[at+1])};
}
function verifyArchiveObjectProvenance(proof,archive,members,owner,publicInputs,observations){
 const names=proof.coreUnits.map(x=>posix.basename(x.object.path));assert.equal(new Set(names).size,139,'Ambiguous duplicate archive object basenames refused');
 assert.equal(new Set(members).size,139,'Duplicate archive member names refused');
 const inventory=JSON.parse(new TextDecoder('utf8',{fatal:true}).decode(metadata(proof.archiveMemberHashInventory,publicInputs,owner+' actual member bytes inventory')));
 assert.equal(inventory.format,'nano-ar-member-sha256-inventory-v1','Actual archive member byte inventory format required');same(inventory.archive,archive,'Member inventory archive identity differs');
 assert.equal(inventory.archiveSha256Before,archive.sha256,'Member collector initial archive changed');assert.equal(inventory.archiveSha256After,archive.sha256,'Member collector final archive changed');
 assert.equal(inventory.collectionMethod,'ar-p-member-bytes','Actual member byte hash collection required');assert.equal(inventory.readOnly,true,'Member collection must be read only');
 assert.ok(Array.isArray(inventory.members)&&inventory.members.length===139,'All139 actual member hashes required');
 const byName=new Map(proof.coreUnits.map(x=>[posix.basename(x.object.path),x]));
 for(let ordinal=0;ordinal<members.length;ordinal++){
  const name=members[ordinal],row=inventory.members[ordinal],unit=byName.get(name);assert.ok(unit,'Unbound archive member name');
  assert.equal(row.ordinal,ordinal,'Archive member ordinal differs');assert.equal(row.name,name,'Archive member sequence/name differs');assert.equal(row.objectPath,unit.object.path,'Member object path differs');
  assert.ok(Number.isSafeInteger(unit.object.bytes)&&unit.object.bytes>0,'Observed object bytes required');assert.equal(row.bytes,unit.object.bytes,'Member/object byte length differs');assert.equal(row.sha256,unit.object.sha256,'Member bytes differ from compiled object');
  assert.equal(observations.get(archiveMemberObservationKey(owner,archive,ordinal,name)),row.sha256,'Independent actual member-byte observation differs');
 }
}

function artifact(row,owner,observations,label){
 assert.ok(row&&typeof row.path==='string'&&row.path.startsWith('/')&&hex(row.sha256),label+' artifact shape');
 assert.equal(observations.get(owner+'\0'+row.path),row.sha256,label+' actual artifact observation differs');
}
function metadata(row,publicInputs,label){
 assert.ok(row&&typeof row.path==='string'&&hex(row.sha256),label+' public binding required');
 const bytes=publicInputs.get(row.path);assert.ok(Buffer.isBuffer(bytes)&&bytes.length<=2*1024*1024,label+' bounded public bytes absent');
 assert.equal(digest(bytes),row.sha256,label+' metadata differs');return bytes;
}
// All observations must be produced by trusted read-only collectors immediately
// before launch. This pure layer never reads a key, launches a node, or obtains
// its own external authority from the manifest it is verifying.
export function verifyCandidateRuntimeBindings({authorityBytes,expectedAuthoritySha256,coreReferenceBytes,compileReferenceBytes,sourceReferenceBytes,checkout,baselineCmakeBytes,currentCmakeBytes,assetsBytes,expectedAssetsSha256,publicInputs,observations,profileBytes,expectedProfileSha256,templateBytes,expectedTemplateSha256}){
 const source=verifyApprovedCandidateCheckout({authorityBytes,expectedAuthoritySha256,coreReferenceBytes,compileReferenceBytes,sourceReferenceBytes,checkout,baselineCmakeBytes,currentCmakeBytes});
 assert.ok(publicInputs instanceof Map&&observations instanceof Map,'Independent metadata and artifact observations required');
 const ref=parsed(compileReferenceBytes,source.compileSourceReferenceSha256,'Qualified compilation input reference'),files=indexed(ref.files);
 const assets=parsed(assetsBytes,expectedAssetsSha256,'New qualified assets');
 assert.equal(assets.format,'nano-isolated-candidate-runtime-assets-v2','Legacy assets cannot authorize candidate');
 assert.equal(assets.repositoryRevision,source.controllerRepoRevision,'Assets controller checkout differs');
 assert.equal(assets.testSourceRevision,source.testSourceRevision,'Assets portable test identity differs');
 assert.equal(assets.coreRevision,candidateSourcePins.revision,'Old core revision cannot authorize new ABI');
 assert.equal(assets.sourceReferenceSha256,source.sourceReferenceSha256,'Assets controller source closure differs');
 assert.equal(assets.compileSourceReferenceSha256,source.compileSourceReferenceSha256,'Assets compilation source closure differs');
 assert.equal(assets.cmakeBaselineSha256,source.cmake.baselineSha256,'Candidate revision is not a CMake baseline');
 assert.equal(assets.currentCmakeSha256,source.cmake.currentSha256);
 assert.equal(assets.profileSha256,expectedProfileSha256,'Profile external identity differs');parsed(profileBytes,expectedProfileSha256,'Unchanged consensus profile');
 assert.equal(assets.templateSha256,expectedTemplateSha256,'Runtime template external identity differs');parsed(templateBytes,expectedTemplateSha256,'Unchanged runtime template');
 same(Object.keys(assets.assets).sort(),['mac-docker','mac-native','vps-docker'],'Exactly three owner assets required');
 const consumerNames=['nano','bootstrapAudit','stoppedCapture','pausedCapture'];
 const consumerSources={nano:'src/main.cpp',bootstrapAudit:'tests/capella/isolated_bootstrap_audit.cpp',stoppedCapture:'tests/capella/isolated_stopped_capture.cpp',pausedCapture:'tests/capella/paused_capture.cpp'};
 const summary=[];
 for(const [owner,a] of Object.entries(assets.assets)){
  assert.equal(a.executionEnvironment,{'mac-native':'native-macos-arm64','mac-docker':'linux-arm64','vps-docker':'linux-amd64'}[owner]);
  for(const role of ['nano','p2p','coreArchive'])artifact(a[role],owner,observations,owner+'/'+role);
  same(Object.keys(a.helpers).sort(),consumerNames.slice(1).sort(),owner+' exact coherent helpers required');
  for(const [role,value] of Object.entries(a.helpers))artifact(value,owner,observations,owner+'/'+role);
  assert.equal(new Set([a.nano.path,a.p2p.path,a.coreArchive.path,...Object.values(a.helpers).map(x=>x.path)]).size,6,'Owner artifact aliases refused');
  const proof=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(metadata(a.buildBinding,publicInputs,owner+' coherent build')));
  assert.equal(proof.format,'nano-coherent-candidate-build-binding-v1');assert.equal(proof.owner,owner);assert.equal(proof.candidateRevision,candidateSourcePins.revision);
  assert.equal(proof.sourceReferenceSha256,source.compileSourceReferenceSha256);assert.ok(candidateSourcePins.buildCmakeVariants.includes(proof.buildCmakeSha256),'Unreviewed compilation CMake variant');
  same(proof.archive,a.coreArchive,'Archive binding differs from coherent build');
  assert.equal(proof.dutyHeaderSha256,files.get('include/nano/consensus_capella_duty.hpp').sha256,'Old duty ABI');
  assert.equal(proof.reactorHeaderSha256,files.get('include/nano/consensus_capella_reactor.hpp').sha256,'Old reactor ABI');
  same(proof.coreUnits.map(x=>x.source).sort(),ref.expectedCoreTranslationUnits,'All139 canonical core compiler units required');
  same(Object.keys(proof.consumers).sort(),[...consumerNames].sort(),'All four new-core consumers required');
  assert.ok(typeof proof.sourceRoot==='string'&&proof.sourceRoot.startsWith('/'),'Actual compiler source root required');
  const {objects:members}=qualifiedArchiveObjectMembers(metadata(proof.archiveMembers,publicInputs,owner+' actual archive members'));
  same([...members].sort(),proof.coreUnits.map(x=>posix.basename(x.object.path)).sort(),'Archive139 members differ from compiled objects');
  verifyArchiveObjectProvenance(proof,a.coreArchive,members,owner,publicInputs,observations);
  assert.equal(new Set(proof.coreUnits.map(x=>x.object.path)).size,139,'Core object aliases refused');
  const consumed=[];
  for(const unit of [...proof.coreUnits,...Object.entries(proof.consumers).map(([name,x])=>({...x,name}))]){
   assert.equal(unit.archiveSha256,a.coreArchive.sha256,'Consumer compiled/linked against another archive ABI');
   const flags=metadata(unit.flags,publicInputs,owner+' flags').toString('utf8');
   const commandBytes=metadata(unit.command,publicInputs,owner+' compile command'),command=commandBytes.toString('utf8'),argv=shellWords(commandBytes);
   assert.equal(argv.filter(x=>x==='-c').length,1,'Actual compile source mode required');assert.equal(argv.filter(x=>x==='-o').length,1,'Actual compile output mode required');
   assert.equal(posix.resolve(unit.cwd,argv[argv.indexOf('-c')+1]),posix.resolve(proof.sourceRoot,unit.source),'Actual compiler source differs');
   assert.equal(posix.resolve(unit.cwd,argv[argv.indexOf('-o')+1]),unit.object.path,'Actual compiler object differs');
   assert.ok(!flags.includes('misc/she')&&!command.includes('misc/she'),'Ignored MCL inputs mentioned by compiler');
   assert.ok(Array.isArray(unit.dependencies)&&unit.dependencies.length>0,'Actual compiler dependencies required');
   const rawDependencies=projectMakeDependencies(metadata(unit.depfile,publicInputs,owner+' actual depfile'),unit.cwd,proof.sourceRoot);
   same(unit.dependencies.map(x=>x.path).sort(),rawDependencies,'Actual depfile/project dependency list differs');
   assert.ok(unit.dependencies.some(x=>x.path===unit.source),'Source missing from compiler dependencies');
   for(const d of unit.dependencies){
    assert.ok(typeof d.path==='string'&&hex(d.sha256),'Dependency binding shape');
    assert.ok(!ref.knownUnconsumedIgnoredPaths.includes(d.path)&&!d.path.startsWith('third_party/mcl/misc/she/'),'Ignored MCL dependency consumed');
    const bound=files.get(d.path);assert.ok(bound,'Unreviewed project dependency: '+d.path);assert.equal(d.sha256,bound.sha256,'Compiler dependency/source SHA differs');consumed.push(d.path);
   }
   artifact(unit.object,owner,observations,owner+' fresh object');
   if(unit.name){
    assert.equal(unit.source,consumerSources[unit.name],'Renamed/substituted consumer source');
    const role=unit.name==='nano'?a.nano:a.helpers[unit.name];same(unit.binary,role,'Consumer binary differs');
    same(unit.linkedCoreArchives,[a.coreArchive],'Consumer link contains absent/mixed core archives');
    const linkBytes=metadata(unit.link,publicInputs,owner+' link command');
    same(linkedCoreArchives(linkBytes,unit.linkCwd),[a.coreArchive.path],'Actual link command used absent/mixed/old core archive');
    assert.ok(Array.isArray(unit.linkObjects)&&unit.linkObjects.length>0,'All actual direct link objects required');
    assert.equal(new Set(unit.linkObjects.map(x=>x.path)).size,unit.linkObjects.length,'Duplicate direct link object paths refused');
    const linked=actualConsumerLinkObjects(linkBytes,unit.linkCwd);same(linked.objects,unit.linkObjects.map(x=>x.path),'Actual consumer link object list differs');assert.equal(linked.output,unit.binary.path,'Actual consumer link output differs');
    const own=unit.linkObjects.filter(x=>x.path===unit.object.path);assert.equal(own.length,1,'Observed consumer object absent from link');same(own[0],unit.object,'Consumed object differs from compiled consumer');
    for(const object of unit.linkObjects)artifact(object,owner,observations,owner+' actual direct link object');

   }
  }
  summary.push({owner,archiveSha256:a.coreArchive.sha256,coreCompilerUnits:139,coherentConsumers:4,projectDependencyBindings:consumed.length,ignoredMclConsumptionObserved:false});
 }
 return {format:'nano-candidate-runtime-source-binding-v1',source,newAssetsSha256:expectedAssetsSha256,ownerBindings:summary,profileSha256:expectedProfileSha256,templateSha256:expectedTemplateSha256,ignoredNonconsumptionVerified:true,startupAllowanceActivated:false,genesisClockReset:false,runtimeEnabled:false,actualRuntimeOrFinalityQualified:false,productionQualified:false};
}

// Controller integration policy. These pins come only from a separately
// reviewed operator input, never from the assets/ref that they authorize.
export function verifyCandidateRuntimeApproval(bytes,expectedSha256){
 const value=parsed(bytes,expectedSha256,'External private-runtime input approval');
 assert.equal(value.format,'nano-private-restored-candidate-input-approval-v1');
 assert.equal(value.intent,'private-restored-campaign');
 assert.equal(value.startupMaximumDowntimeMs,'0x6ddd00');
 assert.equal(value.startupMaximumSteps,'0x3c');
 const row=(x,label)=>{assert.ok(x&&typeof x.path==='string'&&x.path.startsWith('/')&&hex(x.sha256),label+' explicit external path/SHA required');};
 for(const name of ['sourceAuthority','coreReference','compileReference','sourceReference','baselineCmake','assets','profile','template'])row(value[name],name);
 assert.ok(Array.isArray(value.publicInputs)&&value.publicInputs.length>0,'Pinned public compiler inputs required');
 const keys=new Set();for(const input of value.publicInputs){row(input,'Public compiler input');assert.ok(typeof input.key==='string'&&input.key.length>0&&!keys.has(input.key),'Unique compiler metadata key required');keys.add(input.key);}
 assert.deepEqual(Object.keys(value.memberObservations).sort(),['mac-docker','mac-native','vps-docker']);
 for(const input of Object.values(value.memberObservations))row(input,'Separately pinned actual ar-p observations');
 const r=value.recovery;assert.ok(r,'Captured commitment pins required');
 for(const name of ['captureIndex','preparation','copyReceipt','bootstrapPins','bootstrapManifest','bootstrapPlan'])row(r[name],name);
 assert.ok(hex(r.sourceInventorySha256),'External bootstrap inventory digest required; no raw inventory load');
 assert.deepEqual(Object.keys(r.owners).sort(),['mac-docker','mac-native','vps-docker']);
 for(const [owner,o] of Object.entries(r.owners))for(const name of ['sourceCapture','destinationCapture','physicalAudit','helperAuditExecution'])row(o[name],owner+'/'+name);
 row(value.relaySanity,'Relay protocol qualification');
 return {approval:value,approvalSha256:expectedSha256,runtimeEnabled:false,startupAllowanceActivated:false,productionQualified:false};
}
export function candidateArtifactRequests(assets,publicInputs){
 assert.equal(assets.format,'nano-isolated-candidate-runtime-assets-v2');
 assert.ok(publicInputs instanceof Map);const requests={};
 for(const [owner,a] of Object.entries(assets.assets)){
  const proof=JSON.parse(metadata(a.buildBinding,publicInputs,owner+' compiled binding').toString('utf8'));
  const mapping=proof.namespaceMapping;
  assert.ok(mapping&&typeof a.buildRoot==='string'&&posix.resolve(a.buildRoot)===a.buildRoot,'Actual compiled namespace and canonical buildRoot required before observations');
  assert.equal(mapping.physicalBuildRoot,a.buildRoot,'Physical buildRoot differs from pinned compiler binding');
  assert.ok(typeof proof.sourceRoot==='string'&&posix.resolve(proof.sourceRoot)===proof.sourceRoot,'Canonical compiled sourceRoot required');
  assert.equal(mapping.sourceRoot,proof.sourceRoot,'Source root differs from pinned compiler binding');
  assert.equal(mapping.compilerAndLinkerBuildRoot,owner==='mac-native'?a.buildRoot:'/outputs','Compiler/linker namespace differs from actual observation mounts');
  const rows=[a.nano,a.p2p,a.coreArchive,...Object.values(a.helpers),...proof.coreUnits.map(x=>x.object),...Object.values(proof.consumers).flatMap(x=>[x.object,...x.linkObjects])];
  const byPath=new Map();
  for(const row of rows){assert.ok(row&&typeof row.path==='string'&&row.path.startsWith('/')&&hex(row.sha256),'Actual artifact request identity required');const old=byPath.get(row.path);if(old){assert.equal(old.sha256,row.sha256,'Artifact alias has conflicting SHA');if(old.bytes!==undefined&&row.bytes!==undefined)assert.equal(old.bytes,row.bytes,'Artifact alias has conflicting length');}else byPath.set(row.path,row);}
  requests[owner]=[...byPath.values()];
 }
 assert.deepEqual(Object.keys(requests).sort(),['mac-docker','mac-native','vps-docker']);return requests;
}
export function verifyCandidateRecoveryHelperExecution(receipt,{owner,reportSha256,sourceCaptureSha256,destinationCaptureSha256,bootstrapHelperSha256,captureHelperSha256}){
 assert.equal(receipt.format,'nano-qualified-candidate-restoration-helper-execution-v1');
 assert.equal(receipt.owner,owner);assert.equal(receipt.exitCode,0);assert.equal(receipt.reportSha256,reportSha256);
 assert.equal(receipt.sourceCaptureSha256,sourceCaptureSha256);assert.equal(receipt.destinationCaptureSha256,destinationCaptureSha256);
 assert.equal(receipt.bootstrapHelperSha256Before,bootstrapHelperSha256);assert.equal(receipt.bootstrapHelperSha256After,bootstrapHelperSha256);
 assert.equal(receipt.captureHelperSha256Before,captureHelperSha256);assert.equal(receipt.captureHelperSha256After,captureHelperSha256);
 assert.equal(receipt.readOnly,true);assert.equal(receipt.newSignaturesCreated,0);assert.equal(receipt.runtimeStarted,false);
 // The mandatory externally pinned receipt is an execution provenance input,
 // not a new authority inferred from these fields or a synthetic fixture.
 return {owner,reportSha256,helperBindingsVerified:true,runtimeEnabled:false,productionQualified:false};
}
