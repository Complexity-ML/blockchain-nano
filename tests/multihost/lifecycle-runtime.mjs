// Real lifecycle coordinator. Adapters below are supplied by run.mjs and perform
// actual managed-process/owner-local operations; no semantic model grants PASS.
import {createHash} from 'node:crypto';
import {open,lstat,realpath,mkdtemp,mkdir,readdir} from 'node:fs/promises';
import {constants} from 'node:fs';
import {join,resolve,dirname} from 'node:path';
import {tmpdir} from 'node:os';
import {setTimeout as sleep} from 'node:timers/promises';
import {validateLifecycleMode,deriveLifecycleParameters,bindLifecycleIdentities,lifecycleQuantity,lifecycleObservationFromBeacon,validateLifecycleDeposit,requireLifecycleTenure,validateLifecycleObservation,lifecycleProposalMs} from './lifecycle.mjs';
const need=(v,m)=>{if(!v)throw Error('Fresh lifecycle driver: '+m);};
const exact=(v,k,m)=>need(v&&typeof v==='object'&&!Array.isArray(v)&&Object.keys(v).sort().join('\0')===[...k].sort().join('\0'),m+' exact fields');
const q=n=>'0x'+BigInt(n).toString(16),root=/^0x[0-9a-f]{64}$/,shaPattern=/^[0-9a-f]{64}$/;
const sha=b=>createHash('sha256').update(b).digest('hex'),copy=v=>structuredClone(v);
const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
const fields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'],material=s=>fields.map(k=>String(s[k]));
export function requireLifecycleRuntimeAuthority(spec){
 const mode=validateLifecycleMode(spec.lifecycle);need(mode.runtimeDriverEnabled===true,'runtime branch is explicitly enabled');
 need(mode.privateVpsLifecycleTransferAuthorized===true,'specific human authorization for private VPS transfer is required before export');
 if(Object.values(mode.identities).some(x=>x.owner!=='vps-docker'))need(mode.crossOwnerEvidenceTransferAuthorized===true,'specific human authorization for cross-owner action/job evidence transfer is required before export');
 for(const name of ['mac-native','mac-docker','vps-docker'])for(const role of ['lifecycleTool','lifecycleSubmit'])need(spec.owners?.[name]?.programs?.[role],name+' independently admitted '+role+' is required');
 need(mode.identities.voluntaryExit.owner==='vps-docker','the two complete restart captures and closing replay are VPS-local');for(const k of ['maximumInventoryFiles','maximumInventoryBytes','maximumHistoryBlocks'])need(BigInt(mode.resources[k])<=BigInt(Number.MAX_SAFE_INTEGER),'JS/Perl safe declared '+k);need(mode.deadlines.actionMs>=10000,'managed child minimum deadline');return mode;
}
export function validatePreparedLifecycleAction(r,{campaignId,owner,kind,publicKey,expectedHead}){
 exact(r,['format','status','passed','owner','campaignId','kind','resolvedIndex','headRoot','preparedEpoch','actionSha256','actionBytes','payloadDigest','newSignaturesCreated','submitted','canonicalInclusionVerified','physicalCopiesUnchanged','productionQualified'],'PREPARED result');
 need(r.format==='nano-fresh-lifecycle-action-result-v1'&&r.status==='PREPARED'&&r.passed===true&&r.campaignId===campaignId&&r.owner===owner&&r.kind===kind&&r.headRoot===expectedHead.root&&shaPattern.test(r.actionSha256)&&root.test(r.payloadDigest),'prepared authority/commitment');
 need(Number.isSafeInteger(r.actionBytes)&&r.actionBytes>0&&r.actionBytes<=65536&&r.newSignaturesCreated===(kind==='voluntary_exit'?1:2)&&r.submitted===false&&r.canonicalInclusionVerified===false&&r.physicalCopiesUnchanged===true&&r.productionQualified===false,'prepared scope');lifecycleQuantity(r.resolvedIndex);lifecycleQuantity(r.preparedEpoch);need(/^0x[0-9a-f]{96}$/.test(publicKey),'prepared public identity');return copy(r);
}
export function lifecycleSubmissionPlan({campaignId,owner,manifest,action,realP2p,resultFile,maximumWallSeconds}){
 need(shaPattern.test(campaignId)&&['mac-native','mac-docker','vps-docker'].includes(owner),'plan campaign/owner');
 for(const a of [manifest,action,realP2p]){exact(a,['path','sha256','bytes'],'plan Artifact');need(typeof a.path==='string'&&resolve(a.path)===a.path&&!/[\0\r\n,]/.test(a.path)&&shaPattern.test(a.sha256)&&Number.isSafeInteger(a.bytes)&&a.bytes>0&&a.bytes<=1073741824,'plan known Artifact');}
 need(typeof resultFile==='string'&&resolve(resultFile)===resultFile&&!/[\0\r\n,]/.test(resultFile)&&![manifest.path,action.path,realP2p.path].includes(resultFile),'NEW result role');need(Number.isSafeInteger(maximumWallSeconds)&&maximumWallSeconds>=10&&maximumWallSeconds<=60,'plan child bound');
 const plan={format:'nano-fresh-lifecycle-submission-plan-v1',campaignId,owner,manifest:copy(manifest),action:copy(action),realP2p:copy(realP2p),resultFile,maximumWallSeconds};
 // This digest is made BEFORE any output is opened. It is never fabricated by
 // rereading an untrusted path. The action pin came from authenticated C++.
 const bytes=Buffer.from(canonical(plan));return {plan,bytes,sha256:sha(bytes),size:bytes.length};
}
export async function readLifecycleKnownFile(path,pin,{maximum=1048576,privateFile=true,expectedMaterial=null}={}){
 need(typeof path==='string'&&resolve(path)===path&&await realpath(path)===path,'canonical file path');const f=await open(path,constants.O_RDONLY|constants.O_NONBLOCK|constants.O_NOFOLLOW);
 try{const a=await f.stat({bigint:true});need(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<=BigInt(maximum)&&(!privateFile||a.uid===BigInt(process.getuid())&&(Number(a.mode)&0o7777)===0o600),'file type/owner/size/mode');const bytes=await f.readFile(),m=material(a);need(bytes.length===Number(a.size)&&canonical(m)===canonical(material(await f.stat({bigint:true})))&&canonical(m)===canonical(material(await lstat(path,{bigint:true}))),'FD/path changed');if(expectedMaterial)need(canonical(m)===canonical(expectedMaterial),'old admitted material changed');need(sha(bytes)===pin.sha256&&bytes.length===pin.bytes,'known bytes differ');return {bytes,material:m};}finally{await f.close();}
}
// Only this module's successful EXCL creator can open a ctime-only window.
// Admitted/failed paths never acquire a new creation authority.
const knownLifecycleCreations=new Map();
const lifecycleMaterialRow=v=>Array.isArray(v)&&v.length===9&&v.every(x=>typeof x==='string'&&/^-?(?:0|[1-9][0-9]*)$/.test(x)&&x!=='-0');
function lifecycleNewSampleStable(x,expected,creation){
 need(x&&lifecycleMaterialRow(x.before)&&lifecycleMaterialRow(x.after)&&lifecycleMaterialRow(x.linked),'NEW output sampled material');need(x.sha256===expected.sha256&&x.bytes===expected.bytes,'NEW output known SHA/bytes differ');
 for(const m of [x.before,x.after,x.linked])for(let i=0;i<8;++i)need(m[i]===creation[i],'NEW output creation identity changed: '+fields[i]);
 return canonical(x.before)===canonical(x.after)&&canonical(x.after)===canonical(x.linked);
}
export async function writeLifecycleKnownFile(path,bytes){
 need(Buffer.isBuffer(bytes)&&bytes.length>0&&bytes.length<=16*1024*1024&&resolve(path)===path,'known NEW bytes/path');const content=Buffer.from(bytes);need(await realpath(dirname(path))===dirname(path)&&!knownLifecycleCreations.has(path),'NEW known output already retained or linked');const parent=await lstat(dirname(path),{bigint:true});need(parent.isDirectory()&&parent.uid===BigInt(process.getuid())&&(Number(parent.mode)&0o7777)===0o700,'private known-output parent');const pin={path,sha256:sha(content),bytes:content.length},f=await open(path,constants.O_RDWR|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);let creation;
 try{await f.writeFile(content);await f.sync();const s=await f.stat({bigint:true});creation=material(s);need(s.isFile()&&s.nlink===1n&&s.uid===BigInt(process.getuid())&&(Number(s.mode)&0o7777)===0o600&&s.size===BigInt(content.length)&&lifecycleMaterialRow(creation),'NEW known output creator identity');}finally{await f.close();}const directory=await open(dirname(path),constants.O_RDONLY|constants.O_DIRECTORY|constants.O_NOFOLLOW);try{await directory.sync();}finally{await directory.close();}
 need(!knownLifecycleCreations.has(path),'NEW known output already retained');knownLifecycleCreations.set(path,{pin:copy(pin),creation:[...creation],phase:'created',admitted:null});return pin;
}
export async function admitLifecycleKnownFiles(rows,{checkpoint=()=>{},deadlineMs=30000}={}){
 need(Array.isArray(rows)&&rows.length>0&&rows.length<=16&&new Set(rows.map(r=>r.path)).size===rows.length,'bounded unique NEW files');need(typeof checkpoint==='function'&&Number.isSafeInteger(deadlineMs)&&deadlineMs>0&&deadlineMs<=30000,'NEW admission finite budget');
 const selected=rows.map(r=>{const pin={path:r.path,sha256:r.sha256,bytes:r.bytes},entry=knownLifecycleCreations.get(r.path);if(entry){need(canonical(entry.pin)===canonical(pin)&&['created','admitted'].includes(entry.phase),'NEW known output cannot replace old authority');}if(entry?.phase==='admitted'&&r.material!==undefined)need(canonical(r.material)===canonical(entry.admitted.material),'old admitted material changed');return {pin,entry,fresh:entry?.phase==='created',requestedMaterial:r.material===undefined?null:copy(r.material)};});for(const s of selected)if(s.fresh)s.entry.phase='sampling';
 const guard=()=>{for(const s of selected)need(s.entry?knownLifecycleCreations.get(s.pin.path)===s.entry&&s.entry.phase===(s.fresh?'sampling':'admitted'):!knownLifecycleCreations.has(s.pin.path),'NEW known output already retained');};
 const began=performance.now(),until=began+deadlineMs;let previous=null,consecutive=0,since=0,probes=0,last,lastClock=began;const time=()=>{const t=performance.now();need(Number.isFinite(t)&&t>=lastClock&&t<until,'NEW admission deadline/clock');lastClock=t;return t;};
 try{
  for(;;){guard();checkpoint();time();last=[];let stable=true;
   for(const s of selected){guard();time();const r=s.pin;let row;if(s.fresh){need(Number.isSafeInteger(r.bytes)&&r.bytes>0&&r.bytes<=1048576,'file type/owner/size/mode');const x=await sampleNewLifecycleOutput(r.path,r.bytes);stable=lifecycleNewSampleStable(x,r,s.entry.creation)&&stable;row={...r,material:[...x.after]};}else{const x=await readLifecycleKnownFile(r.path,r,{expectedMaterial:s.entry?.admitted.material??s.requestedMaterial});row={...r,material:x.material};}guard();time();last.push(row);}
   const now=time(),encoded=canonical(last);++probes;if(!stable){previous=null;consecutive=0;since=0;}else if(encoded===previous)++consecutive;else{previous=encoded;consecutive=1;since=now;}if(stable&&consecutive>=3&&now-since>=2000)break;need(until-now>1000,'NEW admission deadline');await sleep(1000);guard();
  }
  guard();time();const admitted=copy(last);for(let i=0;i<selected.length;++i){const s=selected[i];if(s.entry){if(s.fresh){s.entry.admitted=copy(admitted[i]);s.entry.phase='admitted';}}else knownLifecycleCreations.set(s.pin.path,{pin:copy(s.pin),creation:null,phase:'admitted',admitted:copy(admitted[i])});}
  const verify=async()=>{for(const r of admitted){checkpoint();await readLifecycleKnownFile(r.path,r,{expectedMaterial:r.material});}return true;};await verify();time();return {probeCount:probes,stableSpanMs:Math.floor(lastClock-since),verify};
 }catch(error){for(const s of selected)if(s.fresh)s.entry.phase='failed';throw error;}
}
// Only a just-created exclusive output may use this prospective window. No
// retained row is replaced. A ctime-changing sample is rejected, never admitted;
// all eight other creation fields and the known bytes must match on EVERY read.
async function sampleNewLifecycleOutput(path,maximum){
 need(await realpath(path)===path,'NEW output canonical path');const f=await open(path,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
 try{const a=await f.stat({bigint:true});need(a.isFile()&&a.nlink===1n&&a.size===BigInt(maximum),'NEW output file type/link/size');const h=createHash('sha256'),buffer=Buffer.alloc(65536);let bytes=0;
  for(;;){const {bytesRead}=await f.read(buffer,0,buffer.length,null);if(!bytesRead)break;bytes+=bytesRead;need(bytes<=maximum,'NEW output grew');h.update(buffer.subarray(0,bytesRead));}
  const b=await f.stat({bigint:true}),p=await lstat(path,{bigint:true});need(b.isFile()&&p.isFile()&&b.nlink===1n&&p.nlink===1n&&await realpath(path)===path,'NEW output FD/path file identity');return {before:material(a),after:material(b),linked:material(p),sha256:h.digest('hex'),bytes};
 }finally{await f.close();}
}
export async function admitNewLifecycleOutput(path,pin,creationMaterial,{isRetained,sample=sampleNewLifecycleOutput,now=()=>performance.now(),wait=ms=>sleep(ms)}={}){
 need(typeof path==='string'&&resolve(path)===path&&typeof isRetained==='function'&&typeof sample==='function'&&typeof now==='function'&&typeof wait==='function','NEW output admission arguments');
 need(pin&&shaPattern.test(pin.sha256)&&Number.isSafeInteger(pin.bytes)&&pin.bytes>0&&pin.bytes<=1073741824,'NEW output known bytes');const expected={sha256:pin.sha256,bytes:pin.bytes},creation=[...creationMaterial];
 const row=v=>Array.isArray(v)&&v.length===9&&v.every(x=>typeof x==='string'&&/^-?(?:0|[1-9][0-9]*)$/.test(x)&&x!=='-0');need(row(creation)&&creation[5]==='1'&&creation[6]===String(expected.bytes),'NEW output creation material');
 const unretained=()=>need(!isRetained(),'NEW output already retained');unretained();const began=now();need(Number.isFinite(began),'NEW output monotonic clock');const until=began+30000;let previous=null,consecutive=0,since=0,probeCount=0,unstableSamples=0,lastClock=began;
 const time=()=>{const t=now();need(Number.isFinite(t)&&t>=lastClock&&t<until,'NEW output first-admission deadline/clock');lastClock=t;return t;};
 for(;;){unretained();time();const x=await sample(path,expected.bytes);unretained();const t=time();++probeCount;
  // Shared with grouped known-file admission; the original predicates/messages
  // and this single-output protocol remain unchanged.
  const stable=lifecycleNewSampleStable(x,expected,creation),encoded=canonical(x.after);
  if(!stable){++unstableSamples;previous=null;consecutive=0;since=0;}
  else if(previous===encoded)++consecutive;else{previous=encoded;consecutive=1;since=t;}
  if(stable&&consecutive>=3&&t-since>=2000){unretained();return {sha256:expected.sha256,bytes:expected.bytes,material:[...x.after],probeCount,unstableSamples,stableSpanMs:Math.floor(t-since)};}
  const remaining=until-t;need(remaining>1000,'NEW output first-admission deadline');await wait(1000);unretained();
 }
}
export async function lifecyclePrivateDirectory(prefix='nano-lifecycle-'){
 const parent=await realpath(tmpdir()),directory=await mkdtemp(join(parent,prefix)),s=await lstat(directory,{bigint:true});need(await realpath(directory)===directory&&s.isDirectory()&&s.uid===BigInt(process.getuid())&&(Number(s.mode)&0o7777)===0o700,'NEW owner-private directory');return directory;
}
export function validateLifecycleAcceptedProjection(r,{campaignId,owner,kind,actionSha256}){
 exact(r,['format','accepted','campaignId','owner','kind','actionSha256','receiptSha256','receiptBytes','rawJobExported','sessionExported','productionQualified'],'accepted projection');need(r.format==='nano-fresh-lifecycle-accepted-public-v1'&&r.accepted===true&&r.campaignId===campaignId&&r.owner===owner&&r.kind===kind&&r.actionSha256===actionSha256&&shaPattern.test(r.receiptSha256)&&Number.isSafeInteger(r.receiptBytes)&&r.receiptBytes>0&&r.receiptBytes<=65536&&r.rawJobExported===false&&r.sessionExported===false&&r.productionQualified===false,'actual private accepted Job projection');return copy(r);
}
// Public transport evidence only: stopped captures are not a completed lifecycle.
const stoppedCaptureOwners=['mac-native','mac-docker','vps-docker'];
const stoppedCaptureFlags=['canonicalStoreSnapshotVerified','primaryAndWitnessBlobClosuresVerified','allObservedFileBytesAndInodesUnchanged','observedWitnessPairsAuthenticated','externalStopTokensMatched','externalWitnessTokensMatched','externalWitnessPinOriginExternallyRequired'];
const stoppedCaptureFalseFlags=['privateKeysRead','envRead','journalWritesRequested','runtimeStarted','networkUsed','productionQualified'];
const stoppedCaptureCounters=['localKeys','awaitingRegistration','proposalsAdmitted','attestationsAdmitted','syncMessagesAdmitted'];
const stoppedCaptureSha=v=>typeof v==='string'&&v.length===64&&shaPattern.test(v);
function stoppedCaptureArtifact(value,withPath=false){
 exact(value,withPath?['path','sha256','bytes']:['sha256','bytes'],'stopped capture artifact');need(stoppedCaptureSha(value.sha256)&&Number.isSafeInteger(value.bytes)&&value.bytes>0&&value.bytes<=16*1024*1024&&(!withPath||typeof value.path==='string'&&value.path.length>0),'stopped capture artifact pin');return {sha256:value.sha256,bytes:value.bytes};
}
function validateStoppedCapturePublic(r,campaignId){
 exact(r,['owner','campaignId','artifactStorage','firstCaptureSha256','stopReceipt','capture','primaryWitnessPairs','readOnlyDatabaseRoles','headSlot','finalized','clockHealthy','validator',...stoppedCaptureFlags,...stoppedCaptureFalseFlags,'signerCallbacksInvoked','newSignaturesCreated','rawCapsuleIncluded','snapshotBytesIncluded','privatePathsIncluded','sessionIncluded'],'stopped capture public projection');
 const pairs=r.owner==='vps-docker'?33:16;need(stoppedCaptureOwners.includes(r.owner)&&stoppedCaptureSha(campaignId)&&r.campaignId===campaignId&&r.artifactStorage===(r.owner==='vps-docker'?'vps-local':'mac-local')&&stoppedCaptureSha(r.firstCaptureSha256),'stopped capture owner/domain/storage');
 stoppedCaptureArtifact(r.stopReceipt);stoppedCaptureArtifact(r.capture);need(r.primaryWitnessPairs===pairs&&r.readOnlyDatabaseRoles===2+2*pairs&&Number.isSafeInteger(r.headSlot)&&r.headSlot>=0,'stopped capture counts/head slot');lifecycleCheckpoint(r.finalized);
 exact(r.validator,['healthy','failure',...stoppedCaptureCounters],'stopped capture validator');need(r.clockHealthy===true&&r.validator.healthy===true&&r.validator.failure===null,'stopped capture healthy stop');for(const key of stoppedCaptureCounters)need(Number.isSafeInteger(r.validator[key])&&r.validator[key]>=0,'stopped capture validator counter');need(r.validator.localKeys===pairs&&r.validator.awaitingRegistration<=pairs&&r.validator.attestationsAdmitted>0,'stopped capture selected validator inventory');
 for(const key of stoppedCaptureFlags)need(r[key]===true,'stopped capture '+key);for(const key of stoppedCaptureFalseFlags)need(r[key]===false,'stopped capture '+key);need(r.signerCallbacksInvoked===0&&r.newSignaturesCreated===0&&r.rawCapsuleIncluded===false&&r.snapshotBytesIncluded===false&&r.privatePathsIncluded===false&&r.sessionIncluded===false,'stopped capture no extra authority or raw export');return r;
}
export function projectLifecycleStoppedCapture(value,{campaignId}){
 exact(value,['name','receipt','firstCaptureSha256','stopArtifact','captureArtifact','capture'],'actual stopped capture input');const {name,receipt,capture}=value;
 need(capture&&capture.format===(name==='vps-docker'?'nano-fresh-capella-stopped-capture-public-v1':'nano-isolated-stopped-recovery-pins-v1')&&capture.passed===true&&capture.owner===name&&capture.campaignId===campaignId&&receipt?.event==='capella-runtime-stopped'&&(name!=='vps-docker'||capture.rawCapsuleExported===false),'actual stopped capture identity');
 const stop=stoppedCaptureArtifact(value.stopArtifact,true),capsule=stoppedCaptureArtifact(value.captureArtifact,true);need(capture.externalStopReceiptSha256===stop.sha256&&capture.lastCaptureSha256===capsule.sha256&&capture.firstCaptureSha256===value.firstCaptureSha256,'actual stopped capture artifact binding');
 const finalized=lifecycleCheckpoint(capture.finalized);need(canonical(finalized)===canonical(lifecycleCheckpoint(receipt.finalized)),'stopped capture independent finalized receipt');
 const result={owner:name,campaignId:capture.campaignId,artifactStorage:name==='vps-docker'?'vps-local':'mac-local',firstCaptureSha256:value.firstCaptureSha256,stopReceipt:stop,capture:capsule,primaryWitnessPairs:capture.primaryWitnessPairs,readOnlyDatabaseRoles:capture.databasesReadOnly,headSlot:capture.headSlot,finalized,clockHealthy:receipt.clockHealthy,validator:{healthy:receipt.validator?.healthy,failure:receipt.validator?.failure,...Object.fromEntries(stoppedCaptureCounters.map(k=>[k,receipt.validator?.[k]]))},...Object.fromEntries([...stoppedCaptureFlags,...stoppedCaptureFalseFlags].map(k=>[k,capture[k]])),signerCallbacksInvoked:capture.signerCallbacksInvoked,newSignaturesCreated:capture.newSignaturesCreated,rawCapsuleIncluded:false,snapshotBytesIncluded:false,privatePathsIncluded:false,sessionIncluded:false};
 return copy(validateStoppedCapturePublic(result,campaignId));
}
export async function publishLifecycleStoppedCapture(value,{campaignId,publish}){
 need(typeof publish==='function','individual stopped capture publication callback');const receipt=projectLifecycleStoppedCapture(value,{campaignId});await publish(copy(receipt));return receipt;
}
// A successful stop remains usable only for the SAME owner roots and until
// the next resume attempt. It never derives authority from a copied database.
export function createLifecycleStopReceipts(){
 const stopped=new Map(),owner=name=>need(stoppedCaptureOwners.includes(name),'stopped owner identity'),binding=value=>need(typeof value==='string'&&value.length>0&&value.length<=65536,'stopped owner root binding');
 return {
  stopped(name,receipt,rootBinding){owner(name);binding(rootBinding);need(receipt?.event==='capella-runtime-stopped'&&receipt.clockHealthy===true&&receipt.validator?.healthy===true&&receipt.validator.failure===null,'actual healthy stopped receipt');need(!stopped.has(name),'owner already has an unconsumed stopped receipt');stopped.set(name,{receipt:copy(receipt),rootBinding});},
  resuming(name){owner(name);stopped.delete(name);},
  collect(rootBindings){exact(rootBindings,stoppedCaptureOwners,'current owner root bindings');const result=[];for(const name of stoppedCaptureOwners){binding(rootBindings[name]);const entry=stopped.get(name);if(entry){need(entry.rootBinding===rootBindings[name],'stopped owner roots changed');result.push({name,receipt:copy(entry.receipt)});}}return result;}
 };
}
export function lifecycleStoppedCapturesReceipt(captures,{campaignId}){
 need(Array.isArray(captures)&&captures.length===3,'exactly three actual stopped captures required');const byOwner=new Map();for(const row of captures){validateStoppedCapturePublic(row,campaignId);need(!byOwner.has(row.owner),'duplicate stopped capture owner');byOwner.set(row.owner,row);}need(stoppedCaptureOwners.every(name=>byOwner.has(name)),'missing stopped capture owner');
 return {format:'nano-fresh-lifecycle-stopped-captures-public-v1',campaignId,owners:stoppedCaptureOwners.map(name=>copy(byOwner.get(name))),ownersCaptured:3,validatorPrimaryWitnessPairs:65,readOnlyDatabaseRoles:136,captureOnly:true,lifecycleClosingExecutedAtPublication:false,lifecycleTerminalQualified:false,restorationSigningAuthorized:false,rawCapsulesIncluded:false,snapshotBytesIncluded:false,privatePathsIncluded:false,sessionIncluded:false,productionQualified:false};
}
export async function publishLifecycleStoppedCapturesBeforeClosing(captures,{campaignId,publish,close}){
 need(typeof publish==='function'&&typeof close==='function','stopped capture publication/closing callbacks');const receipt=lifecycleStoppedCapturesReceipt(captures,{campaignId});await publish(receipt);return await close();
}
export function lifecycleCheckpoint(value){exact(value,['epoch','root'],'checkpoint');need(root.test(value.root)&&typeof value.epoch==='string'&&/^(?:0|[1-9][0-9]*|0x(?:0|[1-9a-f][0-9a-f]*))$/.test(value.epoch),'canonical checkpoint epoch/root');const epoch=BigInt(value.epoch);need(epoch<(1n<<64n),'checkpoint uint64');return {epoch:q(epoch),root:value.root};}
// The existing HTTP observer admits at most 1,024 ancestry blocks. Refresh
// through that SAME authenticated three-owner observer, never from HEAD roots.
export function lifecycleRollingFinalityWindow(parameters,finalityMs){
 const slots=lifecycleQuantity(parameters.slotsPerEpoch),slotMs=lifecycleQuantity(parameters.slotMs);need(slots>0n&&slotMs>0n&&Number.isSafeInteger(finalityMs)&&finalityMs>0,'rolling finality profile/deadline');
 const maximumAncestry=1024n,reservedSlots=3n*slots+(BigInt(finalityMs)+slotMs-1n)/slotMs+1n;need(reservedSlots<maximumAncestry,'profile/finality deadline cannot fit existing HTTP ancestry bound');const refreshEpochs=(maximumAncestry-reservedSlots)/(2n*slots);need(refreshEpochs>0n,'profile has no bounded rolling finality window');
 return {maximumAncestry:1024,refreshEpochs:q(refreshEpochs),refreshAfterSlots:q(refreshEpochs*slots),maximumStartDistanceSlots:q(maximumAncestry-reservedSlots)};
}
export function validateLifecycleTerminalProjection(r,{campaignId,requiredFinalized}){
 const booleans=['capturedLocalDutyIdentityVerified','canonicalWithdrawalIndicesStrictlyIncreasing','exactWithdrawalAccountingVerified','completePrimaryWitnessClosuresVerified','externalCoreTokensMatched','requiredFinalizedAncestorVerified','canonicalSignedStateReplayVerified','physicalCopiesUnchanged'];
 const counts=['canonicalDepositTransactions','canonicalDepositOperations','registrationTransitions','canonicalActionInclusions','acceptedJobArtifactsBound','restoredCapturePairsBound','finalizedRestartProposals','fullPayouts','partialPayouts','postSecondRestartDuplicateFullPayouts','replayedSignedBlocks','liveRecordPeak','liveStateSszPeakBytes','prunes','canonicalExecutionIndexChecks','payoutExecutionReplays','prunedExecutionStatesReplayed','reportBytes'];
 exact(r,['format','status','passed','stage','owner','campaignId','headRoot','headSlot','requiredFinalized',...booleans,...counts,'fullPayoutGwei','reportSha256','historicalForkChoiceVerified','restorationProcessExecutionInferred','newSignaturesCreated','envRead','runtimeStarted','productionQualified','rawCapsuleExported'],'terminal public result');
 need(r.format==='nano-fresh-lifecycle-audit-result-v1'&&r.status==='PASS_SCOPED_FRESH_LIFECYCLE'&&r.passed===true&&r.stage==='terminal'&&r.owner==='vps-docker'&&r.campaignId===campaignId&&canonical(r.requiredFinalized)===canonical(requiredFinalized)&&root.test(r.headRoot)&&shaPattern.test(r.reportSha256),'terminal identity');for(const k of booleans)need(r[k]===true,'terminal '+k);for(const k of counts)need(Number.isSafeInteger(r[k])&&r[k]>=0,'terminal counter '+k);
 need(r.canonicalDepositTransactions===1&&r.canonicalDepositOperations===1&&r.registrationTransitions===1&&r.canonicalActionInclusions===2&&r.acceptedJobArtifactsBound===2&&r.restoredCapturePairsBound===2&&r.finalizedRestartProposals===4&&r.fullPayouts>=2&&r.postSecondRestartDuplicateFullPayouts===0&&r.newSignaturesCreated===0&&r.historicalForkChoiceVerified===false&&r.restorationProcessExecutionInferred===false&&r.envRead===false&&r.runtimeStarted===false&&r.productionQualified===false&&r.rawCapsuleExported===false,'closing scoped replay requirements');lifecycleQuantity(r.headSlot);
 // This is the sum of strictly positive uint64 withdrawals, not one withdrawal.
 need(typeof r.fullPayoutGwei==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(r.fullPayoutGwei),'canonical full payout total');
 const totalGwei=BigInt(r.fullPayoutGwei),payoutCount=BigInt(r.fullPayouts);
 need(totalGwei>=payoutCount&&totalGwei<=payoutCount*((1n<<64n)-1n),'full payout total/count bounds');return copy(r);
}
// The sequence here is driven by real observations. In-process tests can check
// refusals/order only; they cannot qualify the adapters or a lifecycle campaign.
export async function observeLifecycleProposal({ops,id,index,checkpoint,after,ms,wholeDeadline,activationEpoch,slotsPerEpoch,maximumEpochs,now=()=>performance.now()}){
 ops.check();const began=now(),workEnd=ops.workDeadline?ops.workDeadline():wholeDeadline,until=Math.min(began+ms,wholeDeadline,workEnd);
 need(Number.isFinite(began)&&Number.isFinite(until)&&Number.isSafeInteger(ms)&&ms>0&&until>began,'bounded original proposal phase');
 const remaining=Math.floor(until-began);need(remaining>0&&typeof after==='bigint'&&after>=0n&&typeof activationEpoch==='bigint'&&activationEpoch>=0n&&typeof maximumEpochs==='bigint'&&maximumEpochs>0n&&typeof slotsPerEpoch==='bigint'&&slotsPerEpoch>0n,'usable proposal budget and fixed horizon');lifecycleQuantity(index);
 need(lifecycleQuantity(lifecycleCheckpoint(checkpoint).epoch)<=activationEpoch+maximumEpochs,'proposal anchor exceeded fixed activation horizon');
 const result=await ops.finalizedProposal(id,index,checkpoint,after,remaining,until);ops.check();const ended=now();
 need(Number.isFinite(ended)&&ended>=began&&ended<until,'proposal completed after original phase/work deadline');
 need(result&&root.test(result.root)&&lifecycleQuantity(result.slot)>after,'new finalized proposal after fixed journal/head floor');
 need(lifecycleQuantity(result.slot)/slotsPerEpoch<=activationEpoch+maximumEpochs,'proposal exceeded fixed activation horizon');
 if(Object.hasOwn(result,'resolvedIndex'))need(result.resolvedIndex===index,'proposal substituted fixed validator identity');return result;
}
export async function runFreshLifecycle({spec,domain,prepared,profileBytes,depositDocument,initialFinality,ops}){
 const mode=requireLifecycleRuntimeAuthority(spec),parameters=deriveLifecycleParameters(profileBytes,mode,{maximumBootstrapAgeMs:Number(BigInt(spec.clock.maximumDowntimeMs))}),bindings=bindLifecycleIdentities(domain,prepared,mode.identities),ids=bindings.identities;
 const proposerMs=lifecycleProposalMs(mode),deposit=validateLifecycleDeposit(depositDocument,domain,ids.voluntaryExit),rolling=lifecycleRollingFinalityWindow(parameters,Math.max(mode.deadlines.finalityMs,proposerMs)),began=performance.now(),until=began+mode.deadlines.maximumRunMs,maximumEpochs=lifecycleQuantity(parameters.maximumEpochsAfterActivation);let activation=null,headEpoch=0n,checkpoint=initialFinality,rollingSequence=0;lifecycleCheckpoint(checkpoint?.result?.lastCheckpoint);
 const check=()=>{ops.check();need(performance.now()<until,'whole-run finite horizon');const finalizedEpoch=lifecycleQuantity(lifecycleCheckpoint(checkpoint.result.lastCheckpoint).epoch);headEpoch=headEpoch>finalizedEpoch?headEpoch:finalizedEpoch;if(activation!==null)need(headEpoch<=activation+maximumEpochs,'fixed epoch horizon missed');};
 const proposal=(id,index,checkpoint,after)=>{check();return observeLifecycleProposal({ops,id,index,checkpoint,after,ms:proposerMs,wholeDeadline:until,activationEpoch:activation,slotsPerEpoch:lifecycleQuantity(parameters.slotsPerEpoch),maximumEpochs});};
 const wait=async(label,ms,callback)=>{const deadline=Math.min(until,performance.now()+ms);for(;;){check();need(performance.now()<deadline,label+' deadline');const x=await callback(deadline);check();need(performance.now()<deadline,label+' deadline');if(x)return x;await ops.sleep(Math.min(3000,Math.max(1,deadline-performance.now())));}};
 const observe=async(id,selector='head',phaseDeadline=until)=>{check();const h=await ops.header(id.owner,selector),m=h?.data?.header?.message;need(h?.execution_optimistic===false&&root.test(h?.data?.root)&&m&&root.test(m.state_root)&&/^(0|[1-9][0-9]*)$/.test(m.slot),'immutable Beacon HEAD/header');const epoch=BigInt(m.slot)/lifecycleQuantity(parameters.slotsPerEpoch);headEpoch=headEpoch>epoch?headEpoch:epoch;check();
  const baseEpoch=BigInt(checkpoint.result.lastCheckpoint.epoch),distance=BigInt(m.slot)-baseEpoch*lifecycleQuantity(parameters.slotsPerEpoch);
  if(epoch>=baseEpoch+BigInt(rolling.refreshEpochs)){
   need(distance<=BigInt(rolling.maximumStartDistanceSlots),'rolling finality window missed; existing ancestry bound is unchanged');const timeout=Math.floor(Math.min(mode.deadlines.finalityMs,phaseDeadline-performance.now(),until-performance.now()));need(timeout>0,'rolling finality phase deadline');
   checkpoint=await ops.finality(checkpoint.result.lastCheckpoint,'lifecycle-rolling-'+(++rollingSequence),timeout);lifecycleCheckpoint(checkpoint?.result?.lastCheckpoint);check();need(performance.now()<phaseDeadline,'rolling finality phase deadline');
  }
  // Missing route/state is never accepted as a synthetic unregistered row.
  const response=await ops.validator(id.owner,m.state_root,id.publicKey);if(response===null)return null;return lifecycleObservationFromBeacon(response,id,{stateRoot:m.state_root,epoch:q(epoch),publicKey:id.publicKey});};
 await ops.record({event:'lifecycle-begin',parameters,rollingFinalityWindow:rolling,productionQualified:false});
 need(await ops.submitDeposit(ids.voluntaryExit.owner,deposit.rawTransaction)===deposit.transactionHash,'fresh deposit actual transaction hash');
 let exit=await wait('real registration',mode.deadlines.registrationMs,async deadline=>{const r=await observe(ids.voluntaryExit,'head',deadline);return r?.registered?r:null;});
 exit=await wait('real activation',mode.deadlines.activationMs,async deadline=>{const r=await observe(ids.voluntaryExit,'head',deadline);return r&&BigInt(r.activationEpoch)!==(1n<<64n)-1n&&BigInt(r.epoch)>=BigInt(r.activationEpoch)&&BigInt(r.epoch)<BigInt(r.exitEpoch)?r:null;});activation=BigInt(exit.activationEpoch);
 const restartEvidence=[],actions={};
 checkpoint=await ops.finality(checkpoint.result.lastCheckpoint,'lifecycle-activation',mode.deadlines.finalityMs);
 const firstBefore=await proposal(ids.voluntaryExit,exit.resolvedIndex,checkpoint.result.lastCheckpoint,0n);
 const first=await ops.restore('after-activation',ids.voluntaryExit,firstBefore);restartEvidence.push(first);
 checkpoint=await ops.finality(first.finalized,'lifecycle-after-first-restore',mode.deadlines.finalityMs);
 first.afterProposalRoot=(await proposal(ids.voluntaryExit,exit.resolvedIndex,checkpoint.result.lastCheckpoint,BigInt(first.cutoffSlot))).root;
 await wait('real exit tenure',Number(maximumEpochs*lifecycleQuantity(parameters.epochMs)),async deadline=>{const r=await observe(ids.voluntaryExit,'head',deadline);if(!r)return null;exit=r;if(BigInt(r.epoch)<BigInt(r.activationEpoch)+BigInt(parameters.tenureEpochs))return null;requireLifecycleTenure(r,ids.voluntaryExit,parameters);return r;});
 for(const [role,kind] of [['voluntaryExit','voluntary_exit'],['slashing','proposer_slashing']]){
  const id=ids[role],observed=await observe(id);need(observed?.registered&&observed.slashed===false&&observed.withdrawalRecipient!==null,'eligible separate action identity');if(role==='voluntaryExit')requireLifecycleTenure(observed,id,parameters);
  const action=await ops.prepareAndSubmit({role,kind,id,parameters,bindings,mode});actions[role]=action;
  checkpoint=await ops.finality(checkpoint.result.lastCheckpoint,'lifecycle-'+role+'-inclusion',mode.deadlines.finalityMs);
  action.inclusionRoot=await ops.finalizedOperation(id.owner,kind,action,checkpoint.result.lastCheckpoint,mode.deadlines.finalityMs);
 }
 // Withdrawability and public payload entries schedule the final restart. The
 // closing replay must authenticate first finalized full debits and credits.
 const payoutRows=await wait('real withdrawability',Number(maximumEpochs*lifecycleQuantity(parameters.epochMs)),async deadline=>{
  const rows=[];for(const role of ['voluntaryExit','slashing']){const r=await observe(ids[role],'finalized',deadline);if(!r)return null;rows.push(r);}
  if(rows.some(r=>BigInt(r.withdrawableEpoch)===(1n<<64n)-1n||BigInt(r.epoch)<BigInt(r.withdrawableEpoch)))return null;need(rows[0].slashed===false&&rows[1].slashed===true,'separate exit/slashing effects');return rows;
 });
 // An exited sync-committee member can receive fresh rewards after its first
 // full debit. Never require its later post-state balance to stay zero.
 checkpoint=await ops.finality(checkpoint.result.lastCheckpoint,'lifecycle-payout-finality',mode.deadlines.finalityMs);
 const payoutCandidates=[];for(let i=0;i<2;++i){const role=['voluntaryExit','slashing'][i];payoutCandidates.push(await ops.finalizedWithdrawal(ids[role],payoutRows[i],checkpoint.result.lastCheckpoint,mode.deadlines.finalityMs));}
 const thirdCandidates=bindings.ownership.filter(x=>x.owner==='vps-docker'&&!Object.values(ids).some(id=>id.publicKey===x.publicKey));let third,thirdRow;
 for(const id of thirdCandidates){const r=await observe(id);if(r&&r.registered&&!r.slashed&&BigInt(r.activationEpoch)<=BigInt(r.epoch)&&BigInt(r.epoch)<BigInt(r.exitEpoch)){third=id;thirdRow=r;break;}}need(third,'third active unslashed VPS restart duty identity');
 const secondBefore=await proposal(third,thirdRow.resolvedIndex,checkpoint.result.lastCheckpoint,0n);
 const second=await ops.restore('after-payout',third,secondBefore);restartEvidence.push(second);
 checkpoint=await ops.finality(second.finalized,'lifecycle-after-second-restore',mode.deadlines.finalityMs);
 second.afterProposalRoot=(await proposal(third,thirdRow.resolvedIndex,checkpoint.result.lastCheckpoint,BigInt(second.cutoffSlot))).root;
 const evidence={requiredFinalized:lifecycleCheckpoint(checkpoint.result.lastCheckpoint),deposit,actions:Object.fromEntries(Object.entries(actions).map(([role,a])=>[role,{action:a.actionArtifact,acceptedJob:a.acceptedJobArtifact,inclusionRoot:a.inclusionRoot}])),restarts:restartEvidence.map(({cutoffSlot,beforeProposalRoot,afterProposalRoot,dutyPublicKey,sourceCapture,restoredCapture})=>({cutoffSlot,beforeProposalRoot,afterProposalRoot,dutyPublicKey,sourceCapture,restoredCapture}))};
 check();await ops.stageTerminalEvidence(evidence,{crossOwnerEvidenceTransferAuthorized:mode.crossOwnerEvidenceTransferAuthorized});check();
 // Return only a pending terminal commitment; run.mjs first stops/captures ALL
 // three owners, then invokes the mandatory in-situ closing audit and fences.
 return {format:'nano-fresh-lifecycle-runtime-pending-terminal-v1',bindings,parameters,evidence,payoutCandidatesObserved:2,restoresExecuted:2,canonicalReplayQualified:false,productionQualified:false};
}

export function lifecycleOperationDigest(kind,value){
 const dec=v=>{need(typeof v==='string'&&/^(0|[1-9][0-9]*)$/.test(v),'canonical operation decimal');const n=BigInt(v);need(n<1n<<64n,'operation uint64');const b=Buffer.alloc(8);b.writeBigUInt64LE(n);return b;};
 const hex=(v,n)=>{need(typeof v==='string'&&new RegExp('^0x[0-9a-f]{'+(n*2)+'}$').test(v),'operation byte width');return Buffer.from(v.slice(2),'hex');};
 const header=h=>{exact(h,['message','signature'],'signed header');const m=h.message;exact(m,['slot','proposer_index','parent_root','state_root','body_root'],'header');return Buffer.concat([dec(m.slot),dec(m.proposer_index),hex(m.parent_root,32),hex(m.state_root,32),hex(m.body_root,32),hex(h.signature,96)]);};let wire,tag;
 if(kind==='voluntary_exit'){exact(value,['message','signature'],'exit');exact(value.message,['epoch','validator_index'],'exit message');wire=Buffer.concat([dec(value.message.epoch),dec(value.message.validator_index),hex(value.signature,96)]);tag=3;}
 else{need(kind==='proposer_slashing','supported operation');exact(value,['signed_header_1','signed_header_2'],'slashing');wire=Buffer.concat([header(value.signed_header_1),header(value.signed_header_2)]);tag=6;}
 return '0x'+sha(Buffer.concat([Buffer.from([tag]),wire]));
}
