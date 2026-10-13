import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {resolve,join,basename} from 'node:path';
import {verifyCandidateRuntimeApproval,candidateArtifactRequests,verifyCandidateRecoveryHelperExecution} from './candidate-runtime-source.mjs';

// Four new wiring cases only. No process, Docker, SSH, RPC or private input.
// The production controller is read as public source; only selected fragments
// execute with all I/O replaced by in-memory stubs. Its top level never runs.
const source=readFileSync(new URL('./isolated-three-node-run.mjs',import.meta.url),'utf8');
const sha=b=>createHash('sha256').update(b).digest('hex');
assert.equal(sha(source),'88ee823fedc79f4ae44d6ac1e51e1914b1b2d35dbe47ccedd9f0fc9a8da2ad64');
const names=['mac-native','mac-docker','vps-docker'], clone=x=>structuredClone(x), hex='a'.repeat(64);
const between=(start,end)=>{const a=source.indexOf(start),b=source.indexOf(end,a);assert.ok(a>=0&&b>a);return source.slice(a,b);};
const AsyncFunction=Object.getPrototypeOf(async function(){}).constructor;
const row=name=>({path:'/synthetic/'+name,sha256:hex});
function approval(){return {format:'nano-private-restored-candidate-input-approval-v1',intent:'private-restored-campaign',startupMaximumDowntimeMs:'0x6ddd00',startupMaximumSteps:'0x3c',...Object.fromEntries(['sourceAuthority','coreReference','compileReference','sourceReference','baselineCmake','assets','profile','template','relaySanity'].map(k=>[k,row(k)])),publicInputs:[{...row('meta'),key:'synthetic-metadata'}],memberObservations:Object.fromEntries(names.map(k=>[k,row(k+'-ar')])),recovery:{...Object.fromEntries(['captureIndex','preparation','copyReceipt','bootstrapPins','bootstrapManifest','bootstrapPlan'].map(k=>[k,row(k)])),sourceInventorySha256:hex,owners:Object.fromEntries(names.map(k=>[k,Object.fromEntries(['sourceCapture','destinationCapture','physicalAudit','helperAuditExecution'].map(j=>[j,row(k+'-'+j)]))]))}};}
const cli=between('const [workArg,target=',"assert.equal(target,'ubuntu@145.239.199.166');");
const entry=between(' if(candidateMode){\n  assert.ok(restoring', '\ntry{\n const pins');
async function entryHarness(args,bytes,restoring=true){
 const effects={reads:0,gates:0,mkdir:0,writes:0,privateReads:0,workers:0,writers:0};
 const run=new AsyncFunction('assert','process','restoring','verifyCandidateRuntimeApproval','publicRegularBytes','verifyCandidateInputs','mkdir','save','resolve','join','out',cli+'\nlet assets;\n'+entry+'\nreturn {candidateApproval,candidateBinding};');
 let error;try{await run(assert,{argv:['node','synthetic','/synthetic/work','ubuntu@145.239.199.166',...args]},restoring,verifyCandidateRuntimeApproval,async()=>{effects.reads++;return bytes;},async()=>{effects.gates++;throw Error('SYNTHETIC source gate refusal');},async()=>{effects.mkdir++;},async()=>{effects.writes++;},resolve,join,'/synthetic/out');}catch(e){error=e;}return {effects,error};
}
function fixture(){
 const assets={format:'nano-isolated-candidate-runtime-assets-v2',assets:{}},inputs=new Map(),data=new Map(),proofs={};
 for(const owner of names){
  const base=owner==='mac-native'?'/synthetic/native':'/outputs';
  const make=(name,p=base+'/'+name)=>{const bytes=Buffer.from('SYNTHETIC compiled-code observation '+owner+' '+p);data.set(owner+'\0'+p,bytes);return {path:p,sha256:sha(bytes),bytes:bytes.length};};
  const coreUnits=Array.from({length:139},(_,i)=>({object:make('CMakeFiles/nano_core.dir/core'+i+'.cpp.o')}));
  const helpers={bootstrapAudit:make('nano-capella-isolated-bootstrap-audit'),stoppedCapture:make('nano-capella-isolated-stopped-capture'),pausedCapture:make('nano-capella-paused-capture')};
  const mcl=[make('MCL/bint64.o'),make('MCL/base64.o')];
  const consumers=Object.fromEntries(['nano',...Object.keys(helpers)].map(k=>{const object=make('CMakeFiles/'+k+'.dir/main.cpp.o');return [k,{object,linkObjects:[object,...mcl]}];}));
  const proof={coreUnits,consumers,sourceRoot:"/synthetic/src",namespaceMapping:{sourceRoot:"/synthetic/src",physicalBuildRoot:owner==="mac-native"?base:"/synthetic/qualified-"+owner,compilerAndLinkerBuildRoot:base}};proofs[owner]=proof;const bytes=Buffer.from(JSON.stringify(proof)),path=owner+'-build-metadata';inputs.set(path,bytes);
  assets.assets[owner]={nano:make('nano'),p2p:make('nano-p2p',owner==='mac-native'?base+'/nano-p2p':'/build/dependency-route-'+(owner==='mac-docker'?'arm64':'amd64')+'-20261007/network/nano-p2p'),coreArchive:make('libnano_core.a'),helpers,buildBinding:{path,sha256:sha(bytes)},image:owner==='mac-docker'?'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14',buildRoot:owner==='mac-native'?base:'/synthetic/qualified-'+owner,buildMounts:[{type:'bind',source:'/synthetic/qualified-'+owner,target:'/outputs',readOnly:true},{type:'volume',source:owner==='mac-docker'?'nano-clean-arm64-build-20261002':'nano-clean-rpc-build-20261002',target:'/build',readOnly:true}]};
 }
 return {assets,inputs,data,proofs};
}
function helpers(f,requests,overrides={}){
 const calls=[],fds=new Map();let next=1;
 const st=b=>({dev:1n,ino:2n,size:BigInt(b.length),mode:0o100644n,uid:501n,gid:20n,mtimeNs:1n,ctimeNs:1n,isFile:()=>true});
 const code=between('// No caller-computed/self-declared pins.','const verifyCandidateInputs=');
 const dependencies={assert,resolve,join,basename,createHash,constants:{O_RDONLY:0,O_NOFOLLOW:0},lstatSync:()=>({isSymbolicLink:()=>false}),openSync:p=>{const b=f.data.get('mac-native\0'+p);assert.ok(b);fds.set(next,{b,offset:0});return next++;},fstatSync:fd=>st(fds.get(fd).b),readSync:(fd,b,offset,length)=>{const state=fds.get(fd),n=Math.min(length,state.b.length-state.offset);state.b.copy(b,offset,state.offset,state.offset+n);state.offset+=n;return n;},closeSync:fd=>fds.delete(fd),names,work:'/synthetic/work',fixture:'/synthetic/fixture',evidence:'/synthetic',remoteWork:'/remote/work',lstat:async()=>({isSymbolicLink:()=>false,isDirectory:()=>true,...st(Buffer.alloc(0))}),realpath:async p=>p,ssh:async()=> '1:2:41ed:1000:1000:1:1',q:x=>"'"+x+"'",docker:async(argv,remote)=>{calls.push(argv);const owner=remote?'vps-docker':'mac-docker';return requests[owner].map(r=>{const b=f.data.get(owner+'\0'+r.path);assert.ok(b);return r.path+'\t'+sha(b)+'\t1:2:'+b.length+':81a4:501:20:1:1';}).join('\n');}};
 Object.assign(dependencies,overrides);
 const h=new Function(...Object.keys(dependencies),code+'\nreturn {candidateBuildMounts,candidateBuildRootIdentity,observeCandidateArtifacts};')(...Object.values(dependencies));return {...h,calls};
}

test('candidate entry rejects missing external pins and baseline before outputs or workers',async()=>{
 const a=approval(),bytes=Buffer.from(JSON.stringify(a)),args=['--candidate-input-approval','/synthetic/approval','--candidate-input-approval-sha256',sha(bytes)];
 for(const [argv,b,restoring] of [[args.slice(0,2),bytes,true],[args.map((x,i)=>i===3?'b'.repeat(64):x),bytes,true],[args,Buffer.from(JSON.stringify({...a,intent:'changed'})),true],[args,bytes,false]]){const r=await entryHarness(argv,b,restoring);assert.ok(r.error);assert.equal(r.effects.mkdir,0);assert.equal(r.effects.writes,0);assert.equal(r.effects.workers,0);assert.equal(r.effects.writers,0);assert.equal(r.effects.privateReads,0);}
 const omitted=clone(a);delete omitted.recovery.owners['vps-docker'].destinationCapture;const b=Buffer.from(JSON.stringify(omitted));const r=await entryHarness([...args.slice(0,3),sha(b)],b);assert.ok(r.error);assert.equal(r.effects.gates,0);assert.equal(r.effects.mkdir,0);
 const valid=await entryHarness(args,bytes);assert.match(valid.error.message,/source gate refusal/);assert.equal(valid.effects.gates,1);assert.equal(valid.effects.mkdir,0);
});

test('artifact wiring covers 139 units, four consumer objects, MCL, helpers and relay; substitutions refuse',async()=>{
 const f=fixture(),requests=candidateArtifactRequests(f.assets,f.inputs);for(const owner of names){assert.equal(requests[owner].length,151);assert.equal(requests[owner].filter(x=>x.path.includes('nano_core.dir')).length,139);assert.equal(requests[owner].filter(x=>x.path.includes('/MCL/')).length,2);assert.ok(requests[owner].find(x=>x.path===f.assets.assets[owner].p2p.path));}
 const h=helpers(f,requests);assert.equal((await h.observeCandidateArtifacts(f.assets,requests)).size,453);assert.equal(h.calls.length,2);
 for(const [owner,path] of [['mac-native',requests['mac-native'][0].path],['mac-docker',f.assets.assets['mac-docker'].p2p.path],['vps-docker',f.assets.assets['vps-docker'].helpers.stoppedCapture.path]]){const altered=clone(requests);altered[owner].find(x=>x.path===path).sha256='b'.repeat(64);await assert.rejects(helpers(f,altered).observeCandidateArtifacts(f.assets,altered),/artifact differs/);}
 for(const field of ['sha256','bytes']){const altered=clone(f.assets),proof=clone(f.proofs['mac-native']);proof.consumers.nano.linkObjects[0]={...proof.consumers.nano.linkObjects[0],[field]:field==='sha256'?'b'.repeat(64):999};const b=Buffer.from(JSON.stringify(proof));altered.assets['mac-native'].buildBinding.sha256=sha(b);const inputs=new Map(f.inputs);inputs.set(altered.assets['mac-native'].buildBinding.path,b);assert.throws(()=>candidateArtifactRequests(altered,inputs),/conflicting/);}
});

test('mount and compiler namespace wiring refuses private trees and accepts only approved RO code mounts',async()=>{
 const f=fixture(),requests=candidateArtifactRequests(f.assets,f.inputs),h=helpers(f,requests),a=f.assets.assets['mac-docker'];
 assert.equal(h.candidateBuildMounts(a,'mac-docker').filter(x=>x==='--mount').length,2);
 for(const mutate of [x=>x.buildMounts.pop(),x=>x.buildMounts[1].target='/outputs',x=>x.buildMounts[0].readOnly=false,x=>x.buildMounts[0].source='/synthetic/work/data/private',x=>x.buildMounts[0].source='/synthetic/work/data',x=>x.buildMounts[0].source='/synthetic/work']){const b=clone(a);mutate(b);assert.throws(()=>h.candidateBuildMounts(b,'mac-docker'));}
 const image=clone(f.assets);image.assets['mac-docker'].image='sha256:'+'b'.repeat(64);await assert.rejects(h.observeCandidateArtifacts(image,requests));
 for(const path of ['/outputs/key.env','/data/CMakeFiles/core.o','/outputs/witness/core.o']){const r=clone(requests);r['mac-docker'][0].path=path;await assert.rejects(helpers(f,r).observeCandidateArtifacts(f.assets,r));}
 assert.ok(source.includes('...(candidateMode?candidateBuildMounts(node.a,node.owner):')); // Actual runtime construction calls the same function; never execute it.
});

test('helper hashes and captured current witness/key floors cannot be substituted by self-sealed preparation',()=>{
 const expected={owner:'vps-docker',reportSha256:hex,sourceCaptureSha256:'b'.repeat(64),destinationCaptureSha256:'c'.repeat(64),bootstrapHelperSha256:'d'.repeat(64),captureHelperSha256:'e'.repeat(64)};
 const receipt={format:'nano-qualified-candidate-restoration-helper-execution-v1',owner:expected.owner,exitCode:0,...expected,bootstrapHelperSha256Before:expected.bootstrapHelperSha256,bootstrapHelperSha256After:expected.bootstrapHelperSha256,captureHelperSha256Before:expected.captureHelperSha256,captureHelperSha256After:expected.captureHelperSha256,readOnly:true,newSignaturesCreated:0,runtimeStarted:false};
 assert.equal(verifyCandidateRecoveryHelperExecution(receipt,expected).runtimeEnabled,false);
 for(const [key,value] of [['owner','mac-native'],['exitCode',1],['newSignaturesCreated',1],['sourceCaptureSha256',hex],['destinationCaptureSha256',hex],['captureHelperSha256After',hex],['bootstrapHelperSha256Before',hex]])assert.throws(()=>verifyCandidateRecoveryHelperExecution({...receipt,[key]:value},expected));
 const code=between('   const destPath=join(work,owner+', '\n   captures[owner]=capture;');
 const run=new AsyncFunction('assert','join','work','owner','candidateMode','candidateApproval','prepared','readRequiredPinnedJson','pins','plan','capture',code);
 const work='/synthetic/work',owner='vps-docker',capture={core:{primary:{sequence:'0x2b',digest:hex},witness:{sequence:'0x2b',digest:'b'.repeat(64)}},restart:{sequence:'0x2b',digest:hex},recoveryWitnessCommitment:{sequence:'0x2b',digest:'b'.repeat(64)},validatorKeys:[{publicKeyIndex:32,publicKey:'public-synthetic',primary:{sequence:'0x2',digest:hex},witness:{sequence:'0x2',digest:hex},proposal:{slot:'0x1'},attestation:null,snapshotSha256:hex,witnessSnapshotSha256:hex}]};
 const dest={...clone(capture),passed:true,owner,manifestSha256:hex,campaignId:'synthetic',externalStopTokensMatched:true,independentWitnessCurrentIntentsMatched:true,canonicalStoreSnapshotVerified:true,allObservedFileBytesAndInodesUnchanged:true};
 const approval={recovery:{owners:{[owner]:{destinationCapture:{path:work+'/'+owner+'-restored-capture.public.json',sha256:expected.destinationCaptureSha256}}}}},prepared={destinationCaptureHashes:{[owner]:expected.destinationCaptureSha256}};
 const evaluate=(d,p=prepared)=>run(assert,join,work,owner,true,approval,p,async()=>({value:d}),{fixtureManifestSha256:hex},{campaignId:'synthetic'},capture);
 return (async()=>{await evaluate(dest);const d=clone(dest);d.core.witness.digest=hex;await assert.rejects(evaluate(d));const k=clone(dest);k.validatorKeys[0].primary.sequence='0x3';await assert.rejects(evaluate(k));await assert.rejects(evaluate(dest,{destinationCaptureHashes:{[owner]:hex}}));})();
});

test('repaired mount binds the physical buildRoot and refuses exact private roots, ancestors and symlink escape before worker',async()=>{
 const f=fixture(),requests=candidateArtifactRequests(f.assets,f.inputs),h=helpers(f,requests),a=f.assets.assets['mac-docker'];
 assert.ok(h.candidateBuildMounts(a,'mac-docker').includes('type=bind,source='+a.buildRoot+',target=/outputs,readonly'));
 for(const root of ['/synthetic/work/data','/synthetic/work','/synthetic','/synthetic/fixture','/synthetic/fixture/child','/synthetic/isolated-stopped-capture-20261012','/synthetic/work/witness']){
  const b=clone(a);b.buildRoot=root;b.buildMounts[0].source=root;assert.throws(()=>h.candidateBuildMounts(b,'mac-docker'),/private/);
 }
 const drift=clone(a);drift.buildMounts[0].source='/synthetic/other-build';assert.throws(()=>h.candidateBuildMounts(drift,'mac-docker'),/physical buildRoot/);
 const symlink=helpers(f,requests,{lstat:async p=>({isSymbolicLink:()=>p===a.buildRoot})});await assert.rejects(symlink.candidateBuildRootIdentity(a,'mac-docker'),/symlink/);assert.equal(symlink.calls.length,0);
 const escape=helpers(f,requests,{realpath:async()=>'/synthetic/work/data'});await assert.rejects(escape.candidateBuildRootIdentity(a,'mac-docker'),/physical code root/);assert.equal(escape.calls.length,0);
 const remote=clone(f.assets.assets['vps-docker']);for(const root of ['/home/ubuntu/nanobees-qualification/isolated-fixture-exporter-amd64-public-copy-repair-20261012','/home/ubuntu/nanobees-qualification/isolated-fixture-exporter-amd64-public-copy-repair-20261012/runtime-parent/private/export-v1']){remote.buildRoot=root;remote.buildMounts[0].source=root;assert.throws(()=>h.candidateBuildMounts(remote,'vps-docker'),/private/);}
 const wrongVolume=clone(a);wrongVolume.buildMounts[1].source='nano-clean-rpc-build-20261002';assert.throws(()=>h.candidateBuildMounts(wrongVolume,'mac-docker'));
 await h.candidateBuildRootIdentity(a,'mac-docker');assert.equal(h.calls.length,0);
 // No worker process is created: all identities and Docker output are stubs.
});


test('compiled namespace pins refuse coordinated asset and mount substitution before any observation worker',()=>{
 const f=fixture(),requests=candidateArtifactRequests(f.assets,f.inputs);assert.equal(requests['mac-docker'].length,151);
 for(const owner of names){
  const changed=clone(f.assets);changed.assets[owner].buildRoot='/synthetic/substitute';changed.assets[owner].buildMounts[0].source='/synthetic/substitute';
  assert.throws(()=>candidateArtifactRequests(changed,f.inputs),/Physical buildRoot differs/);
  for(const field of ['sourceRoot','compilerAndLinkerBuildRoot']){
   const proof=clone(f.proofs[owner]);proof.namespaceMapping[field]='/synthetic/substitute';const bytes=Buffer.from(JSON.stringify(proof));const inputs=new Map(f.inputs);inputs.set(f.assets.assets[owner].buildBinding.path,bytes);const a=clone(f.assets);a.assets[owner].buildBinding.sha256=sha(bytes);
   assert.throws(()=>candidateArtifactRequests(a,inputs),/root differs|namespace differs/);
  }
  const noMapping=clone(f.proofs[owner]);delete noMapping.namespaceMapping;const bytes=Buffer.from(JSON.stringify(noMapping));const inputs=new Map(f.inputs);inputs.set(f.assets.assets[owner].buildBinding.path,bytes);const a=clone(f.assets);a.assets[owner].buildBinding.sha256=sha(bytes);assert.throws(()=>candidateArtifactRequests(a,inputs),/compiled namespace/);
 }
 // No observer function is called; the binding must reject before its caller can create a worker.
});
