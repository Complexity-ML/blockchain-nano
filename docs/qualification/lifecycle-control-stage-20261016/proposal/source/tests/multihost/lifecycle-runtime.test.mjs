// New protocol/opaque-FS checks only. No EVM, DB, keys, process, RPC or network.
import {test,after} from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {join} from 'node:path';
import {writeFile,readFile,readdir,symlink,link,unlink,rm,mkdir,chmod,lstat} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import {validateRotationCopySummary} from './rotation-owned-copy.mjs';
import {requireLifecycleRuntimeAuthority,lifecycleSubmissionPlan,validatePreparedLifecycleAction,validateLifecycleAcceptedProjection,validateLifecycleTerminalProjection,lifecycleCheckpoint,lifecycleRollingFinalityWindow,lifecycleOperationDigest,lifecyclePrivateDirectory,writeLifecycleKnownFile,readLifecycleKnownFile,admitLifecycleKnownFiles,projectLifecycleStoppedCapture,publishLifecycleStoppedCapture,createLifecycleStopReceipts,lifecycleStoppedCapturesReceipt,publishLifecycleStoppedCapturesBeforeClosing} from './lifecycle-runtime.mjs';
import {lifecycleWholeCopyProgram,lifecycleLinuxStatx} from './lifecycle-owner.mjs';
const q=n=>'0x'+BigInt(n).toString(16),root=n=>'0x'+String(n).padStart(64,'0'),pub=n=>'0x'+String(n).padStart(96,'0'),digest=b=>createHash('sha256').update(b).digest('hex');
const names=['mac-native','mac-docker','vps-docker'];
let expectedRefusals=0;const refuses=f=>{assert.throws(f);++expectedRefusals;},rejects=async f=>{await assert.rejects(f);++expectedRefusals;};
after(()=>assert.equal(expectedRefusals,55,'All new expected refusals must be exercised'));
function fixture(){const id=(owner,n)=>({owner,keyEnv:'NANO_TEST_CAPELLA_NETWORK_KEY_'+n,publicKey:pub(n)});return {lifecycle:{format:'nano-fresh-validator-lifecycle-v1',identities:{voluntaryExit:id('vps-docker',64),slashing:id('mac-native',0)},runtimeDriverEnabled:true,privateVpsLifecycleTransferAuthorized:true,crossOwnerEvidenceTransferAuthorized:true,syntheticActionAuthorized:true,intentionallyAuthorConflictingHeaders:true,deadlines:{registrationMs:30000,activationMs:30000,finalityMs:30000,restoreMs:300000,actionMs:300000,auditMs:600000,maximumEpochsAfterActivation:400,maximumRunMs:30000000},resources:{maximumWallSeconds:q(60),maximumHistoryBlocks:q(10000),maximumArchiveBytes:q(1000000000),maximumIndexBytes:q(1120000),maximumInventoryFiles:q(20000),maximumInventoryBytes:q(1000000000)}},owners:Object.fromEntries(names.map(name=>[name,{programs:{lifecycleTool:{},lifecycleSubmit:{}}}]))};}
test('runtime and every specific private transfer authority fail closed before adapters',()=>{
 assert.equal(requireLifecycleRuntimeAuthority(fixture()).runtimeDriverEnabled,true);
 const window=lifecycleRollingFinalityWindow({slotsPerEpoch:'0x8',slotMs:'0x1770'},500000);assert.equal(window.maximumAncestry,1024);assert.equal(window.refreshEpochs,'0x39');assert.equal(window.refreshAfterSlots,'0x1c8');assert(BigInt(window.refreshAfterSlots)<BigInt(window.maximumStartDistanceSlots));refuses(()=>lifecycleRollingFinalityWindow({slotsPerEpoch:'0x200',slotMs:'0x1770'},500000));
 for(const mutate of [x=>x.lifecycle.runtimeDriverEnabled=false,x=>x.lifecycle.privateVpsLifecycleTransferAuthorized=false,x=>delete x.lifecycle.privateVpsLifecycleTransferAuthorized,x=>x.lifecycle.crossOwnerEvidenceTransferAuthorized=false,x=>delete x.lifecycle.crossOwnerEvidenceTransferAuthorized,x=>x.lifecycle.identities.voluntaryExit.owner='mac-native',x=>delete x.owners['mac-native'].programs.lifecycleSubmit,x=>delete x.owners['vps-docker'].programs.lifecycleTool]){const x=fixture();mutate(x);refuses(()=>requireLifecycleRuntimeAuthority(x));}
});
test('plan hash comes from owned known bytes and retains independent artifact pins',()=>{
 const a=(path,n)=>({path,sha256:String(n).repeat(64),bytes:112}),input={campaignId:'a'.repeat(64),owner:'vps-docker',manifest:a('/lifecycle-owner/isolated-domain.json',1),action:a('/lifecycle-results/action/action.private.json',2),realP2p:a('/network-build/nano-p2p',3),resultFile:'/lifecycle-owner/accepted-job.private.json',maximumWallSeconds:30},p=lifecycleSubmissionPlan(input);
 assert.equal(digest(p.bytes),p.sha256);assert.equal(p.bytes.length,p.size);assert.equal(JSON.parse(p.bytes).action.sha256,'2'.repeat(64));input.action.sha256='4'.repeat(64);assert.equal(JSON.parse(p.bytes).action.sha256,'2'.repeat(64));
 for(const mutate of [x=>x.owner='other',x=>x.action.bytes=0,x=>x.manifest.sha256='0x'+'1'.repeat(64),x=>x.realP2p.path='/network-build/../nano-p2p',x=>x.resultFile=x.action.path,x=>x.maximumWallSeconds=61]){const x=structuredClone(input);mutate(x);refuses(()=>lifecycleSubmissionPlan(x));}
});
function prepared(){return {format:'nano-fresh-lifecycle-action-result-v1',status:'PREPARED',passed:true,owner:'vps-docker',campaignId:'a'.repeat(64),kind:'voluntary_exit',resolvedIndex:'0x40',headRoot:root(1),preparedEpoch:'0x45',actionSha256:'b'.repeat(64),actionBytes:1200,payloadDigest:root(2),newSignaturesCreated:1,submitted:false,canonicalInclusionVerified:false,physicalCopiesUnchanged:true,productionQualified:false};}
test('PREPARED is not acceptance; accepted projection cannot leak a session or fake finality',()=>{
 const context={campaignId:'a'.repeat(64),owner:'vps-docker',kind:'voluntary_exit',publicKey:pub(64),expectedHead:{root:root(1),stateRoot:root(2),slot:'0x228'}},r=prepared();assert.equal(validatePreparedLifecycleAction(r,context).submitted,false);
 for(const mutate of [x=>x.status='accepted',x=>x.owner='mac-native',x=>x.headRoot=root(3),x=>x.newSignaturesCreated=0,x=>x.submitted=true,x=>x.canonicalInclusionVerified=true,x=>x.extra=true]){const x=prepared();mutate(x);refuses(()=>validatePreparedLifecycleAction(x,context));}
 const receipt={format:'nano-fresh-lifecycle-accepted-public-v1',accepted:true,campaignId:context.campaignId,owner:context.owner,kind:context.kind,actionSha256:r.actionSha256,receiptSha256:'c'.repeat(64),receiptBytes:600,rawJobExported:false,sessionExported:false,productionQualified:false};assert.equal(validateLifecycleAcceptedProjection(receipt,{...context,actionSha256:r.actionSha256}).accepted,true);
 for(const mutate of [x=>x.accepted=false,x=>x.actionSha256='d'.repeat(64),x=>x.sessionExported=true,x=>x.rawJobExported=true,x=>x.session=root(4)]){const x=structuredClone(receipt);mutate(x);refuses(()=>validateLifecycleAcceptedProjection(x,{...context,actionSha256:r.actionSha256}));}
});
test('operation commitment encodes actual public SSZ fields without private action export',()=>{
 const signature='0x'+'ab'.repeat(96),exit={message:{epoch:'69',validator_index:'64'},signature},wire=Buffer.alloc(112);wire.writeBigUInt64LE(69n);wire.writeBigUInt64LE(64n,8);Buffer.from(signature.slice(2),'hex').copy(wire,16);assert.equal(lifecycleOperationDigest('voluntary_exit',exit),'0x'+digest(Buffer.concat([Buffer.from([3]),wire])));
 const header={message:{slot:'550',proposer_index:'0',parent_root:root(1),state_root:root(2),body_root:root(3)},signature},slashing={signed_header_1:header,signed_header_2:structuredClone(header)};slashing.signed_header_2.message.body_root=root(4);const initial=lifecycleOperationDigest('proposer_slashing',slashing);assert.match(initial,/^0x[0-9a-f]{64}$/);slashing.signed_header_2.message.body_root=root(5);assert.notEqual(lifecycleOperationDigest('proposer_slashing',slashing),initial);
 for(const x of [{message:{epoch:'069',validator_index:'64'},signature},{message:{epoch:'69',validator_index:null},signature},{message:{epoch:'69',validator_index:'64'},signature:'0x00'}])refuses(()=>lifecycleOperationDigest('voluntary_exit',x));
});
test('opaque known-file admission refuses post-admission mutation and physical aliases',{timeout:15000},async()=>{
 const dir=await lifecyclePrivateDirectory('nano-lifecycle-protocol-fixture-');try{
  const path=join(dir,'plan.private.json'),bytes=Buffer.from('{"opaque":true}'),pin=await writeLifecycleKnownFile(path,bytes);assert.equal(Number((await lstat(path)).mode)&0o7777,0o600);const admitted=await admitLifecycleKnownFiles([pin]);await admitted.verify();
  await rejects(()=>writeLifecycleKnownFile(path,bytes));await writeFile(path,Buffer.from('{"opaque":fals}'));await rejects(()=>admitted.verify());await writeFile(path,bytes);await rejects(()=>admitted.verify());
  const alias=join(dir,'alias'),hard=join(dir,'hard');await symlink(path,alias);await rejects(()=>readLifecycleKnownFile(alias,pin));await link(path,hard);await rejects(()=>readLifecycleKnownFile(path,pin));await unlink(hard);await rejects(()=>readLifecycleKnownFile(path,{...pin,sha256:'e'.repeat(64)}));
 }finally{await rm(dir,{recursive:true,force:true});}
});
test('whole-owner copy plan refuses unsafe namespace, owner and zero inventory authority',()=>{
 const resources=fixture().lifecycle.resources;
 refuses(()=>lifecycleWholeCopyProgram('../bad','vps-docker',resources));refuses(()=>lifecycleWholeCopyProgram('isolated-capella-'+'a'.repeat(64),'other',resources));refuses(()=>lifecycleWholeCopyProgram('isolated-capella-'+'a'.repeat(64),'vps-docker',{...resources,maximumInventoryFiles:'0x0'}));
});
function terminal(){return {format:'nano-fresh-lifecycle-audit-result-v1',status:'PASS_SCOPED_FRESH_LIFECYCLE',passed:true,stage:'terminal',owner:'vps-docker',campaignId:'a'.repeat(64),headRoot:root(1),headSlot:'0xa80',requiredFinalized:{epoch:'0x14f',root:root(2)},capturedLocalDutyIdentityVerified:true,canonicalWithdrawalIndicesStrictlyIncreasing:true,exactWithdrawalAccountingVerified:true,completePrimaryWitnessClosuresVerified:true,externalCoreTokensMatched:true,requiredFinalizedAncestorVerified:true,canonicalSignedStateReplayVerified:true,physicalCopiesUnchanged:true,canonicalDepositTransactions:1,canonicalDepositOperations:1,registrationTransitions:1,canonicalActionInclusions:2,acceptedJobArtifactsBound:2,restoredCapturePairsBound:2,finalizedRestartProposals:4,fullPayouts:2,partialPayouts:0,postSecondRestartDuplicateFullPayouts:0,replayedSignedBlocks:2600,liveRecordPeak:33,liveStateSszPeakBytes:1000000,prunes:300,canonicalExecutionIndexChecks:2600,payoutExecutionReplays:2,prunedExecutionStatesReplayed:2000,reportBytes:2000,fullPayoutGwei:'0x123',reportSha256:'f'.repeat(64),historicalForkChoiceVerified:false,restorationProcessExecutionInferred:false,newSignaturesCreated:0,envRead:false,runtimeStarted:false,productionQualified:false,rawCapsuleExported:false};}
test('only terminal C++ replay closes accounting; 3 captures never mean 3 full audits',()=>{
 const context={campaignId:'a'.repeat(64),requiredFinalized:terminal().requiredFinalized};assert.deepEqual(lifecycleCheckpoint({epoch:'335',root:root(2)}),context.requiredFinalized);refuses(()=>lifecycleCheckpoint({epoch:'0335',root:root(2)}));refuses(()=>lifecycleCheckpoint({epoch:(1n<<64n).toString(),root:root(2)}));assert.equal(validateLifecycleTerminalProjection(terminal(),context).restoredCapturePairsBound,2);
 for(const mutate of [x=>x.owner='mac-native',x=>x.canonicalDepositTransactions=0,x=>x.canonicalActionInclusions=1,x=>x.fullPayouts=1,x=>x.restoredCapturePairsBound=1,x=>x.finalizedRestartProposals=3,x=>x.exactWithdrawalAccountingVerified=false,x=>x.canonicalWithdrawalIndicesStrictlyIncreasing=false,x=>x.postSecondRestartDuplicateFullPayouts=1,x=>x.rawCapsuleExported=true,x=>x.runtimeStarted=true,x=>x.productionQualified=true,x=>x.identities=[],x=>x.requiredFinalized.root=root(9)]){const x=terminal();mutate(x);refuses(()=>validateLifecycleTerminalProjection(x,context));}
});

// These groups are pure and can be selected without the earlier filesystem cases.
{
const campaignId='a'.repeat(64),root='0x'+'1'.repeat(64),names=['mac-native','mac-docker','vps-docker'];
const flags=['canonicalStoreSnapshotVerified','primaryAndWitnessBlobClosuresVerified','allObservedFileBytesAndInodesUnchanged','observedWitnessPairsAuthenticated','externalStopTokensMatched','externalWitnessTokensMatched','externalWitnessPinOriginExternallyRequired'];
const falseFlags=['privateKeysRead','envRead','journalWritesRequested','runtimeStarted','networkUsed','productionQualified'];
function captured(name){
 const pairs=name==='vps-docker'?33:16,stopSha='b'.repeat(64),captureSha='c'.repeat(64),firstCaptureSha256='d'.repeat(64),finalized={epoch:'0x3',root};
 return {name,receipt:{event:'capella-runtime-stopped',clockHealthy:true,finalized,validator:{healthy:true,failure:null,localKeys:pairs,awaitingRegistration:name==='vps-docker'?1:0,proposalsAdmitted:2,attestationsAdmitted:3,syncMessagesAdmitted:0},validatorKeys:['RAW-PRIVATE-KEY-INVENTORY'],session:'RAW-PRIVATE-SESSION'},firstCaptureSha256,stopArtifact:{path:'/RAW-PRIVATE-PATH/stop.json',sha256:stopSha,bytes:200},captureArtifact:{path:'/RAW-PRIVATE-PATH/capture.json',sha256:captureSha,bytes:1000},capture:{format:name==='vps-docker'?'nano-fresh-capella-stopped-capture-public-v1':'nano-isolated-stopped-recovery-pins-v1',passed:true,owner:name,campaignId,firstCaptureSha256,lastCaptureSha256:captureSha,externalStopReceiptSha256:stopSha,headSlot:43,finalized,primaryWitnessPairs:pairs,databasesReadOnly:2+2*pairs,...Object.fromEntries(flags.map(k=>[k,true])),...Object.fromEntries(falseFlags.map(k=>[k,false])),signerCallbacksInvoked:0,newSignaturesCreated:0,rawCapsuleExported:false,snapshot:'RAW-PRIVATE-SNAPSHOT',fullFileInodeInventories:'RAW-PRIVATE-INVENTORY',core:{snapshot:'RAW-PRIVATE-CORE'},session:'RAW-PRIVATE-SESSION'}};
}
const projected=()=>names.map(name=>projectLifecycleStoppedCapture(captured(name),{campaignId}));
test('stopped capture receipt retains real owner-local storage and only public commitments',()=>{
 const inputs=names.map(captured),before=JSON.stringify(inputs),rows=inputs.map(value=>projectLifecycleStoppedCapture(value,{campaignId})),result=lifecycleStoppedCapturesReceipt(rows.slice().reverse(),{campaignId});
 assert.equal(JSON.stringify(inputs),before);assert.deepEqual(result.owners.map(x=>x.owner),names);assert.deepEqual(result.owners.map(x=>x.artifactStorage),['mac-local','mac-local','vps-local']);assert.deepEqual(result.owners.map(x=>x.primaryWitnessPairs),[16,16,33]);assert.deepEqual(result.owners.map(x=>x.readOnlyDatabaseRoles),[34,34,68]);
 assert.equal(result.validatorPrimaryWitnessPairs,65);assert.equal(result.readOnlyDatabaseRoles,136);assert.equal(result.captureOnly,true);assert.equal(result.lifecycleClosingExecutedAtPublication,false);assert.equal(result.lifecycleTerminalQualified,false);assert.equal(result.restorationSigningAuthorized,false);assert.equal(result.productionQualified,false);assert.equal(JSON.stringify(result).includes('RAW-PRIVATE-'),false);
 assert.deepEqual(result.owners[2].capture,{sha256:'c'.repeat(64),bytes:1000});assert.deepEqual(result.owners[2].finalized,{epoch:'0x3',root});rows[0].capture.sha256='f'.repeat(64);assert.equal(result.owners[0].capture.sha256,'c'.repeat(64));
});
test('stopped capture receipt refuses missing authority, bad bindings and non-exact counters or flags',()=>{
 const changes=[v=>v.name='other',v=>v.capture.owner='mac-docker',v=>v.capture.campaignId='e'.repeat(64),v=>v.capture.passed=false,v=>v.capture.rawCapsuleExported=true,v=>v.capture.firstCaptureSha256='e'.repeat(64),v=>v.capture.externalStopReceiptSha256='e'.repeat(64),v=>v.capture.lastCaptureSha256='e'.repeat(64),v=>v.stopArtifact.bytes=0,v=>v.captureArtifact.sha256+='\n',v=>v.capture.headSlot=43.5,v=>v.capture.finalized={epoch:'0x2',root},v=>v.capture.primaryWitnessPairs=32,v=>v.capture.databasesReadOnly='68',v=>v.receipt.clockHealthy=false,v=>v.receipt.validator.failure='unhealthy',v=>v.receipt.validator.localKeys=32,v=>v.receipt.validator.attestationsAdmitted=0,v=>v.receipt.validator.awaitingRegistration=34,v=>v.capture.newSignaturesCreated=1,v=>v.capture.signerCallbacksInvoked=1,v=>delete v.capture.externalWitnessTokensMatched,v=>v.extra='RAW-PRIVATE-EXTRA'];
 for(const change of changes){const input=captured('vps-docker');change(input);assert.throws(()=>projectLifecycleStoppedCapture(input,{campaignId}));}
 for(const key of flags){const input=captured('vps-docker');input.capture[key]=false;assert.throws(()=>projectLifecycleStoppedCapture(input,{campaignId}));}
 for(const key of falseFlags){const input=captured('vps-docker');input.capture[key]=true;assert.throws(()=>projectLifecycleStoppedCapture(input,{campaignId}));}
});
test('stopped capture receipt requires all three distinct owners and rejects public extra fields',()=>{
 const rows=projected();for(const selected of [[],rows.slice(0,2),[rows[0],rows[1],rows[1]]])assert.throws(()=>lifecycleStoppedCapturesReceipt(selected,{campaignId}));
 const extras=structuredClone(rows);extras[2].privatePath='/RAW-PRIVATE-PATH';assert.throws(()=>lifecycleStoppedCapturesReceipt(extras,{campaignId}));const wrong=structuredClone(rows);wrong[2].artifactStorage='mac-local';assert.throws(()=>lifecycleStoppedCapturesReceipt(wrong,{campaignId}));
 assert.throws(()=>lifecycleStoppedCapturesReceipt(rows,{campaignId:'f'.repeat(64)}));
});
test('stopped capture receipt is awaited and remains published when lifecycle closing fails',async()=>{
 const events=[],failure=Error('missing real live lifecycle stages');let committed;
 await assert.rejects(publishLifecycleStoppedCapturesBeforeClosing(projected(),{campaignId,publish:async receipt=>{events.push('publish-start');await Promise.resolve();committed=structuredClone(receipt);events.push('published');},close:async()=>{events.push('closing');assert(committed);throw failure;}}),error=>error===failure);
 assert.deepEqual(events,['publish-start','published','closing']);assert.equal(committed.owners.length,3);assert.equal(committed.owners[2].artifactStorage,'vps-local');assert.equal(committed.lifecycleTerminalQualified,false);assert.equal(committed.productionQualified,false);
});
test('stopped capture receipt publication refusal prevents closing and missing capture prevents publication',async()=>{
 let closes=0,publishes=0;const failure=Error('publication refused');
 await assert.rejects(publishLifecycleStoppedCapturesBeforeClosing(projected(),{campaignId,publish:async()=>{++publishes;throw failure;},close:async()=>{++closes;}}),error=>error===failure);assert.equal(publishes,1);assert.equal(closes,0);
 await assert.rejects(publishLifecycleStoppedCapturesBeforeClosing(projected().slice(0,2),{campaignId,publish:async()=>{++publishes;},close:async()=>{++closes;}}));assert.equal(publishes,1);assert.equal(closes,0);
 const result=await publishLifecycleStoppedCapturesBeforeClosing(projected(),{campaignId,publish:async()=>{++publishes;},close:async()=>{++closes;return 'actual closing result';}});assert.equal(result,'actual closing result');assert.equal(closes,1);
});
test('each actual stopped capture commitment is awaited before return or a failed next stage',async()=>{
 for(const name of names){const events=[],original=captured(name),before=JSON.stringify(original);let release,committed,returned=false;const gate=new Promise(resolve=>{release=resolve;}),pending=publishLifecycleStoppedCapture(original,{campaignId,publish:async receipt=>{events.push('publish-start');committed=structuredClone(receipt);await gate;receipt.capture.sha256='f'.repeat(64);events.push('published');}});pending.then(()=>{returned=true;});await Promise.resolve();assert.equal(returned,false);assert.deepEqual(events,['publish-start']);release();const projection=await pending;events.push('returned');assert.equal(projection.capture.sha256,'c'.repeat(64));assert.equal(JSON.stringify(original),before);assert.equal(committed.owner,name);assert.equal(committed.artifactStorage,name==='vps-docker'?'vps-local':'mac-local');assert.equal(JSON.stringify(committed).includes('RAW-PRIVATE-'),false);
  const failure=Error('next control stage refused');await assert.rejects(async()=>{assert(committed);events.push('stage');throw failure;},error=>error===failure);assert.deepEqual(events,['publish-start','published','returned','stage']);assert.equal(committed.rawCapsuleIncluded,false);assert.equal(committed.productionQualified,false);
 }
 let stages=0;const failure=Error('capture publication refused');await assert.rejects(async()=>{await publishLifecycleStoppedCapture(captured('vps-docker'),{campaignId,publish:async()=>{throw failure;}});++stages;},error=>error===failure);assert.equal(stages,0);
});
test('retained stopped owner survives stage failure and all three captures publish before failed closing',async()=>{
 const tracker=createLifecycleStopReceipts(),bindings=Object.fromEntries(names.map(name=>[name,'original roots '+name])),stageFailure=Error('public genesis-state.ssz absent during stage'),closingFailure=Error('live lifecycle stages missing');let firstFailure,committed;const events=[];
 try{tracker.stopped('vps-docker',captured('vps-docker').receipt,bindings['vps-docker']);events.push('vps-stopped');throw stageFailure;}catch(error){firstFailure=error;}
 for(const name of ['mac-native','mac-docker'])tracker.stopped(name,captured(name).receipt,bindings[name]);const terminal=tracker.collect(bindings);assert.deepEqual(terminal.map(x=>x.name),names);
 const captures=terminal.map(({name,receipt})=>projectLifecycleStoppedCapture({...captured(name),receipt},{campaignId}));
 try{await publishLifecycleStoppedCapturesBeforeClosing(captures,{campaignId,publish:async value=>{await Promise.resolve();committed=value;events.push('captures-published');},close:async()=>{events.push('closing');throw closingFailure;}});}catch(error){firstFailure??=error;assert.equal(error,closingFailure);}
 assert.equal(firstFailure,stageFailure);assert.deepEqual(events,['vps-stopped','captures-published','closing']);assert.equal(committed.ownersCaptured,3);assert.equal(committed.validatorPrimaryWitnessPairs,65);assert.equal(committed.readOnlyDatabaseRoles,136);assert.equal(committed.captureOnly,true);assert.equal(committed.lifecycleTerminalQualified,false);assert.equal(committed.productionQualified,false);assert.equal(JSON.stringify(committed).includes('RAW-PRIVATE-'),false);
});
test('resuming invalidates the old stop before a new writer and cannot publish an incomplete terminal set',async()=>{
 const tracker=createLifecycleStopReceipts(),bindings=Object.fromEntries(names.map(name=>[name,'original roots '+name]));for(const name of names)tracker.stopped(name,captured(name).receipt,bindings[name]);
 tracker.resuming('vps-docker');const afterResume=tracker.collect(bindings);assert.deepEqual(afterResume.map(x=>x.name),['mac-native','mac-docker']);let publishes=0,closes=0;
 await assert.rejects(publishLifecycleStoppedCapturesBeforeClosing(afterResume.map(({name,receipt})=>projectLifecycleStoppedCapture({...captured(name),receipt},{campaignId})),{campaignId,publish:async()=>{++publishes;},close:async()=>{++closes;}}));assert.equal(publishes,0);assert.equal(closes,0);
 bindings['vps-docker']='new restored roots vps-docker';const receipt=captured('vps-docker').receipt;tracker.stopped('vps-docker',receipt,bindings['vps-docker']);receipt.validator.healthy=false;const terminal=tracker.collect(bindings);assert.equal(terminal.length,3);assert.equal(terminal[2].receipt.validator.healthy,true);terminal[2].receipt.validator.healthy=false;assert.equal(tracker.collect(bindings)[2].receipt.validator.healthy,true);
});
test('retained stopped receipt refuses changed roots, duplicate or unhealthy owner authority',()=>{
 const tracker=createLifecycleStopReceipts(),bindings=Object.fromEntries(names.map(name=>[name,'original roots '+name]));tracker.stopped('vps-docker',captured('vps-docker').receipt,bindings['vps-docker']);
 assert.throws(()=>tracker.collect({...bindings,'vps-docker':'different roots without resume'}));assert.throws(()=>tracker.stopped('vps-docker',captured('vps-docker').receipt,bindings['vps-docker']));assert.throws(()=>tracker.stopped('other',captured('vps-docker').receipt,'roots'));assert.throws(()=>tracker.resuming('other'));assert.throws(()=>tracker.collect({'vps-docker':bindings['vps-docker']}));
 tracker.resuming('vps-docker');for(const change of [r=>r.event='other',r=>r.clockHealthy=false,r=>r.validator.healthy=false,r=>r.validator.failure='failed']){const receipt=captured('vps-docker').receipt;change(receipt);assert.throws(()=>tracker.stopped('vps-docker',receipt,bindings['vps-docker']));}assert.throws(()=>tracker.stopped('vps-docker',captured('vps-docker').receipt,''));assert.equal(tracker.collect(bindings).length,0);
});
}

// Linux-only, actual generated Perl over public opaque fixtures. No Nano/DB/key.
// Relocate exactly ONE output-equality literal to a NEW private fixture root;
// all filesystem code and namespace/owner paths remain the generated program.
test('whole-owner generated Perl copies complete namespaced roots and refuses copy mutation',{skip:process.platform!=='linux',timeout:24000},async t=>{
 const namespace='isolated-capella-'+'e'.repeat(64),owner='vps-docker',base=await lifecyclePrivateDirectory('nano-lifecycle-whole-copy-');t.after(()=>rm(base,{recursive:true,force:true}));
 const sourceData=join(base,'source-data',namespace,owner),sourceWitness=join(base,'source-witness',namespace,owner),output=join(base,'output');
 for(const path of [sourceData,sourceWitness,output])await mkdir(path,{recursive:true,mode:0o700});
 const files=[
  [join(sourceData,'chain/CURRENT'),'MANIFEST-000001\n'],[join(sourceData,'chain/MANIFEST-000001'),'PUBLIC opaque core bytes'],[join(sourceData,'chain/LOCK'),''],
  [join(sourceData,'journal-32/CURRENT'),'PUBLIC journal32'],[join(sourceData,'journal-64/nested/checkpoint'),'PUBLIC journal64 nested'],[join(sourceData,'peer.key'),'PUBLIC TEST peer identity, not a private key'],
  [join(sourceWitness,'chain-witness/CURRENT'),'PUBLIC core witness'],[join(sourceWitness,'witness-32/CURRENT'),'PUBLIC witness32'],[join(sourceWitness,'witness-64/nested/checkpoint'),'PUBLIC witness64 nested']
 ];
 for(const [path,bytes] of files){await mkdir(path.slice(0,path.lastIndexOf('/')),{recursive:true,mode:0o700});await writeFile(path,bytes,{flag:'wx',mode:0o600});}
 await mkdir(join(sourceData,'chain/empty-directory'),{mode:0o700});await mkdir(join(sourceWitness,'empty-directory'),{mode:0o700});
 const fields=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
 async function inventory(root){const rows=[];async function walk(path,relative=''){const s=await lstat(path,{bigint:true});assert(s.isDirectory()||s.isFile());const row={path:relative,kind:s.isDirectory()?'directory':'file',material:fields.map(k=>String(s[k]))};if(s.isFile()){const bytes=await readFile(path);row.bytes=bytes.length;row.sha256=digest(bytes);}rows.push(row);if(s.isDirectory())for(const leaf of (await readdir(path)).sort())await walk(join(path,leaf),relative?relative+'/'+leaf:leaf);}await walk(root);return rows;}
 const before=[await inventory(sourceData),await inventory(sourceWitness)],resources={maximumInventoryFiles:'0x40',maximumInventoryBytes:'0x10000'},generated=lifecycleWholeCopyProgram(namespace,owner,resources),guard="$out eq '/rotation-audit'";
 assert.equal(generated.split(guard).length,2,'One exact virtual-output equality must remain');assert.match(output,/^[A-Za-z0-9_./-]+$/);const program=generated.replace(guard,"$out eq '"+output+"'");
 const trace=join(base,'sync-create.json');
 const syncObserver=String.raw`use IO::Handle ();use Fcntl qw(O_WRONLY O_CREAT O_EXCL);
my @test_sync_paths;my $test_real_sync=\&IO::Handle::sync;
{no warnings 'redefine';*IO::Handle::sync=sub{my($handle,@args)=@_;my $target=readlink('/proc/'.$$.'/fd/'.fileno($handle));die "test sync FD unavailable\n" unless defined($target);push @test_sync_paths,$target;return $test_real_sync->($handle,@args);};}
END{sysopen(my $log,'${trace}',O_WRONLY|O_CREAT|O_EXCL,0600)or die "test trace refused\n";print {$log} JSON::PP->new->canonical(1)->encode(\@test_sync_paths) or die "test trace write refused\n";close($log)or die "test trace close refused\n";}
`;
 const invoke=(mode,pin,timeout)=>spawnSync('/usr/bin/perl',['-e',mode==='create'?syncObserver+program:program,sourceData,sourceWitness,output,mode,pin],{cwd:base,env:{PATH:'/usr/bin:/bin',HOME:base,LANG:'C',LC_ALL:'C'},encoding:'utf8',timeout,maxBuffer:1048576});
 const created=invoke('create','-',12000);assert.ifError(created.error);assert.equal(created.signal,null);assert.equal(created.status,0,created.stderr);const summary=validateRotationCopySummary(JSON.parse(created.stdout),false),proofBytes=await readFile(join(output,'physical-copy.private.json')),proof=JSON.parse(proofBytes);assert.equal(digest(proofBytes),summary.copyProofSha256);assert.equal(summary.sourceFiles,files.length);assert.equal(proof.format,'nano-fresh-lifecycle-owner-copy-private-proof-v2');assert.equal(proof.materialEncoding,'linux-statx-nine-decimal-ns-v1');
 const targets=[join(output,'data',namespace,owner),join(output,'witness',namespace,owner)];assert.deepEqual(proof.sources.map(x=>x.root),[sourceData,sourceWitness]);assert.deepEqual(proof.copies.map(x=>x.root),targets);
 const synced=JSON.parse(await readFile(trace,'utf8')),parents=[join(output,'data',namespace),join(output,'witness',namespace),join(output,'data'),join(output,'witness'),output];assert.deepEqual(synced.filter(path=>parents.includes(path)),parents,'Namespace parents must really sync before data/witness/output');for(const target of targets)assert(synced.includes(target),'Owner root must really sync');
 const sourceIds=new Set(before.flat().map(row=>row.material[0]+':'+row.material[1])),ids=new Set(sourceIds);
 for(let i=0;i<2;++i){const copied=await inventory(targets[i]);assert.deepEqual(copied.map(x=>x.path),before[i].map(x=>x.path));for(let j=0;j<copied.length;++j){const row=copied[j],original=before[i][j],id=row.material[0]+':'+row.material[1];assert.equal(row.kind,original.kind);assert.equal(ids.has(id),false,'All copied inodes must be independent');ids.add(id);assert.equal(row.material[4],String(process.getuid()));assert.equal(BigInt(row.material[2])&0o7777n,row.kind==='directory'?0o700n:0o600n);if(row.kind==='file'){assert.equal(row.material[3],'1');assert.equal(row.sha256,original.sha256);assert.equal(row.bytes,original.bytes);}}}
 assert.deepEqual([await inventory(sourceData),await inventory(sourceWitness)],before);assert.equal(await readFile(join(targets[0],'peer.key'),'utf8'),files[5][1]);assert.equal((await lstat(join(targets[0],'chain/LOCK'))).size,0);
 const verified=invoke('verify',summary.copyProofSha256,5000);assert.ifError(verified.error);assert.equal(verified.signal,null);assert.equal(verified.status,0,verified.stderr);assert.equal(validateRotationCopySummary(JSON.parse(verified.stdout),true).copyProofSha256,summary.copyProofSha256);assert.deepEqual(await readFile(join(output,'physical-copy.private.json')),proofBytes);
 await writeFile(join(targets[0],'peer.key'),'PUBLIC changed copy',{flag:'a'});const refused=invoke('verify',summary.copyProofSha256,5000);assert.ifError(refused.error);assert.equal(refused.signal,null);assert.notEqual(refused.status,0);assert.equal(refused.stdout,'');assert.match(refused.stderr,/stopped rotation physical copy refused/);assert.deepEqual([await inventory(sourceData),await inventory(sourceWitness)],before);assert.deepEqual(await readFile(join(output,'physical-copy.private.json')),proofBytes);
});


// Kernel metadata conformance for the exact backend used by the owner program.
// Synthetic files only; Node bigint stat/lstat is an independent ns reference.
test('whole-owner statx keeps exact FD/path nine fields, one ns and closed ABI refusals',{skip:process.platform!=='linux',timeout:5000},async t=>{
 const base=await lifecyclePrivateDirectory('nano-lifecycle-statx-');t.after(()=>rm(base,{recursive:true,force:true}));assert.match(base,/^[A-Za-z0-9_./-]+$/);
 const mtime=join(base,'mtime'),renamed=join(base,'renamed'),moved=join(base,'moved'),alias=join(base,'alias'),missing=join(base,'missing');
 await writeFile(mtime,'PUBLIC statx timestamp bytes',{flag:'wx',mode:0o600});await writeFile(renamed,'PUBLIC original inode',{flag:'wx',mode:0o600});await symlink(mtime,alias);
 const touched=spawnSync('/usr/bin/touch',['-m','--date=@1700000000.123456789',mtime],{cwd:base,env:{PATH:'/usr/bin:/bin',HOME:base,LANG:'C',LC_ALL:'C'},encoding:'utf8',timeout:1000,maxBuffer:4096});assert.ifError(touched.error);assert.equal(touched.signal,null);assert.equal(touched.status,0,touched.stderr);
 const fields=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'],row=async path=>{const s=await lstat(path,{bigint:true});return fields.map(k=>String(s[k]));};
 const before=await row(mtime);
 const probe=String.raw`use strict;use warnings;use Fcntl qw(:DEFAULT);use JSON::PP ();
sub need{die "statx probe refused\n" unless $_[0]}sub tick{}
`+lifecycleLinuxStatx+String.raw`
my($mtime,$rename,$moved,$link,$missing)=@ARGV;need(@ARGV==5);
sysopen(my $mf,$mtime,O_RDONLY|O_NOFOLLOW)or die "test open refused\n";my $before_path=stat9($mtime,0);my $before_fd=stat9(fileno($mf),1);
need(system('/usr/bin/touch','-m','--date=@1700000000.123456790',$mtime)==0);my $after_path=stat9($mtime,0);my $after_fd=stat9(fileno($mf),1);close($mf)or die "test close refused\n";
sysopen(my $rf,$rename,O_RDONLY|O_NOFOLLOW)or die "test rename open refused\n";rename($rename,$moved)or die "test rename refused\n";sysopen(my $replacement,$rename,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600)or die "test replacement refused\n";print {$replacement} 'PUBLIC replacement inode' or die "test replacement write refused\n";close($replacement)or die "test replacement close refused\n";my $renamed_fd=stat9(fileno($rf),1);my $replacement_path=stat9($rename,0);close($rf)or die "test rename close refused\n";my $symlink_path=stat9($link,0);
my $raw="\0"x256;substr($raw,0,4)=pack('V',0xffffffff);substr($raw,16,12)=pack('V3',1,0,0);substr($raw,28,2)=pack('v',0100600);substr($raw,32,8)="\xff"x8;substr($raw,40,8)="\xff"x8;substr($raw,96,12)=pack('q<V',2,3);substr($raw,112,12)=pack('q<V',-1,999999999);substr($raw,136,8)=pack('V2',0xffffffff,0xffffffff);my $exact_limits=lifecycle_statx_decode($raw);
my $refusals=0;sub refused{my($f)=@_;my $ok=eval{$f->();1};need(!$ok&&$@ eq "statx probe refused\n");++$refusals;}
refused(sub{lifecycle_statx_decode(substr($raw,0,255))});
my $mask=$raw;substr($mask,0,4)=pack('V',0x3de);refused(sub{lifecycle_statx_decode($mask)});
my $mtime_bad=$raw;substr($mtime_bad,120,4)=pack('V',1000000000);refused(sub{lifecycle_statx_decode($mtime_bad)});
my $ctime_bad=$raw;substr($ctime_bad,104,4)=pack('V',1000000000);refused(sub{lifecycle_statx_decode($ctime_bad)});
for my $args(['linux','ppc64-linux',8,8,'12345678'],['darwin','x86_64-linux',8,8,'12345678'],['linux','x86_64-linux',4,8,'12345678'],['linux','x86_64-linux',8,4,'12345678'],['linux','x86_64-linux',8,8,'87654321']){refused(sub{lifecycle_statx_abi(@$args)});}
refused(sub{stat9('relative',0)});refused(sub{stat9(-1,1)});refused(sub{stat9($missing,0)});
print JSON::PP->new->canonical(1)->encode({number=>$lifecycle_statx_number,beforePath=>$before_path,beforeFd=>$before_fd,afterPath=>$after_path,afterFd=>$after_fd,renamedFd=>$renamed_fd,replacementPath=>$replacement_path,symlinkPath=>$symlink_path,exactLimits=>$exact_limits,refusals=>$refusals})."\n";
`;
 const result=spawnSync('/usr/bin/perl',['-e',probe,mtime,renamed,moved,alias,missing],{cwd:base,env:{PATH:'/usr/bin:/bin',HOME:base,LANG:'C',LC_ALL:'C'},encoding:'utf8',timeout:3000,maxBuffer:16384});assert.ifError(result.error);assert.equal(result.signal,null);assert.equal(result.status,0,result.stderr);assert.equal(result.stderr,'');const actual=JSON.parse(result.stdout);
 assert.equal(actual.number,process.arch==='x64'?332:291);assert.deepEqual(actual.beforePath,before);assert.deepEqual(actual.beforeFd,before);assert.deepEqual(actual.afterPath,await row(mtime));assert.deepEqual(actual.afterFd,actual.afterPath);assert.equal(BigInt(actual.afterPath[7])-BigInt(actual.beforePath[7]),1n);
 assert.deepEqual(actual.renamedFd,await row(moved));assert.deepEqual(actual.replacementPath,await row(renamed));assert.notEqual(actual.renamedFd[1],actual.replacementPath[1]);assert.deepEqual(actual.symlinkPath,await row(alias));assert.equal(BigInt(actual.symlinkPath[2])&0o170000n,0o120000n);
 assert.deepEqual(actual.exactLimits,['18446744073709551615','18446744073709551615','33152','1','0','0','18446744073709551615','-1','2000000003']);assert.equal(actual.refusals,12);
 for(const key of ['beforePath','beforeFd','afterPath','afterFd','renamedFd','replacementPath','symlinkPath','exactLimits'])assert(actual[key].every(value=>typeof value==='string'&&/^-?(?:0|[1-9][0-9]*)$/.test(value)),'All nine fields remain exact decimal strings');
});
