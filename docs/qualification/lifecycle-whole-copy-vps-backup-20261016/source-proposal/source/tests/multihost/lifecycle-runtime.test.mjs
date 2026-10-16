// New protocol/opaque-FS checks only. No EVM, DB, keys, process, RPC or network.
import {test,after} from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {join} from 'node:path';
import {writeFile,readFile,readdir,symlink,link,unlink,rm,mkdir,chmod,lstat} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import {validateRotationCopySummary} from './rotation-owned-copy.mjs';
import {requireLifecycleRuntimeAuthority,lifecycleSubmissionPlan,validatePreparedLifecycleAction,validateLifecycleAcceptedProjection,validateLifecycleTerminalProjection,lifecycleCheckpoint,lifecycleRollingFinalityWindow,lifecycleOperationDigest,lifecyclePrivateDirectory,writeLifecycleKnownFile,readLifecycleKnownFile,admitLifecycleKnownFiles,projectLifecycleStoppedCapture,lifecycleStoppedCapturesReceipt,publishLifecycleStoppedCapturesBeforeClosing} from './lifecycle-runtime.mjs';
import {lifecycleWholeCopyProgram} from './lifecycle-owner.mjs';
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
 const created=invoke('create','-',12000);assert.ifError(created.error);assert.equal(created.signal,null);assert.equal(created.status,0,created.stderr);const summary=validateRotationCopySummary(JSON.parse(created.stdout),false),proofBytes=await readFile(join(output,'physical-copy.private.json')),proof=JSON.parse(proofBytes);assert.equal(digest(proofBytes),summary.copyProofSha256);assert.equal(summary.sourceFiles,files.length);
 const targets=[join(output,'data',namespace,owner),join(output,'witness',namespace,owner)];assert.deepEqual(proof.sources.map(x=>x.root),[sourceData,sourceWitness]);assert.deepEqual(proof.copies.map(x=>x.root),targets);
 const synced=JSON.parse(await readFile(trace,'utf8')),parents=[join(output,'data',namespace),join(output,'witness',namespace),join(output,'data'),join(output,'witness'),output];assert.deepEqual(synced.filter(path=>parents.includes(path)),parents,'Namespace parents must really sync before data/witness/output');for(const target of targets)assert(synced.includes(target),'Owner root must really sync');
 const sourceIds=new Set(before.flat().map(row=>row.material[0]+':'+row.material[1])),ids=new Set(sourceIds);
 for(let i=0;i<2;++i){const copied=await inventory(targets[i]);assert.deepEqual(copied.map(x=>x.path),before[i].map(x=>x.path));for(let j=0;j<copied.length;++j){const row=copied[j],original=before[i][j],id=row.material[0]+':'+row.material[1];assert.equal(row.kind,original.kind);assert.equal(ids.has(id),false,'All copied inodes must be independent');ids.add(id);assert.equal(row.material[4],String(process.getuid()));assert.equal(BigInt(row.material[2])&0o7777n,row.kind==='directory'?0o700n:0o600n);if(row.kind==='file'){assert.equal(row.material[3],'1');assert.equal(row.sha256,original.sha256);assert.equal(row.bytes,original.bytes);}}}
 assert.deepEqual([await inventory(sourceData),await inventory(sourceWitness)],before);assert.equal(await readFile(join(targets[0],'peer.key'),'utf8'),files[5][1]);assert.equal((await lstat(join(targets[0],'chain/LOCK'))).size,0);
 const verified=invoke('verify',summary.copyProofSha256,5000);assert.ifError(verified.error);assert.equal(verified.signal,null);assert.equal(verified.status,0,verified.stderr);assert.equal(validateRotationCopySummary(JSON.parse(verified.stdout),true).copyProofSha256,summary.copyProofSha256);assert.deepEqual(await readFile(join(output,'physical-copy.private.json')),proofBytes);
 await writeFile(join(targets[0],'peer.key'),'PUBLIC changed copy',{flag:'a'});const refused=invoke('verify',summary.copyProofSha256,5000);assert.ifError(refused.error);assert.equal(refused.signal,null);assert.notEqual(refused.status,0);assert.equal(refused.stdout,'');assert.match(refused.stderr,/stopped rotation physical copy refused/);assert.deepEqual([await inventory(sourceData),await inventory(sourceWitness)],before);assert.deepEqual(await readFile(join(output,'physical-copy.private.json')),proofBytes);
});
