// Pure models only: fabricated public identities/observations are not crypto.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {validateLifecycleMode,deriveLifecycleParameters,bindLifecycleIdentities,validateLifecycleObservation,requireLifecycleTenure,requireExactFullWithdrawal,createLifecycleProgress,lifecycleObservationFromBeacon,validateLifecycleDeposit} from './lifecycle.mjs';
const q=n=>'0x'+BigInt(n).toString(16),root=n=>'0x'+String(n).padStart(64,'0'),pub=n=>'0x'+String(n).padStart(96,'0'),MAX=q((1n<<64n)-1n),address='0x'+'1'.repeat(40),credentials='0x01'+'0'.repeat(22)+address.slice(2);
function identity(){
 const domain={format:'nano-isolated-capella-fixture-domain-v1',synthetic:true,productionQualified:false,campaignId:'a'.repeat(64),registryCount:3,publicKeys:[0,1,2,3].map(index=>({index,publicKey:pub(index+1)})),bootstrapSigningHistory:{deposit:{publicKey:pub(4),amountGwei:32000000000,withdrawalCredentials:credentials}},transactionSender:{address:'0x'+'2'.repeat(40)}};
 const prepared={nodes:['mac-native','mac-docker','vps-docker'].map((name,i)=>({name,signerIdentities:(i===2?[2,3]:[i]).map(k=>({keyEnv:'NANO_TEST_CAPELLA_NETWORK_KEY_'+k,publicKey:pub(k+1)}))}))};
 const declarations={voluntaryExit:{...prepared.nodes[2].signerIdentities[1],owner:'vps-docker'},slashing:{...prepared.nodes[0].signerIdentities[0],owner:'mac-native'}};return {domain,prepared,declarations};
}
function mode(){return {format:'nano-fresh-validator-lifecycle-v1',identities:identity().declarations,syntheticActionAuthorized:true,intentionallyAuthorConflictingHeaders:true,deadlines:{registrationMs:100000,activationMs:100000,finalityMs:10000,restoreMs:10000,actionMs:30000,auditMs:30000,maximumEpochsAfterActivation:400,maximumRunMs:20000000},resources:{maximumWallSeconds:q(30),maximumHistoryBlocks:q(4000),maximumArchiveBytes:q(50000000),maximumIndexBytes:q(500000),maximumInventoryFiles:q(10000),maximumInventoryBytes:q(512*1024*1024)}};}
function profile(){const schedule=[q(2),q(32),q(4),q(256)];return {format:'nano-capella-profile-v1',common:[{exit:[...schedule,q(64)],registry:[...schedule,q(32000000000),q(16000000000)],slashing:[...schedule,q(64),q(64),q(512),q(8),q(1000000000),q(3)],ffg:[q(8),q(1000000000)],epochSync:[q(8),q(8),q(1),q(32000000000),q(10)]},q(6000),q(4000),q(40),q(0),q(1337),q(0),q(6),q(8388608),q(32),q(1048576),q(1024)],withdrawals:[q(32000000000),q(4),q(16)],genesisVersion:'0x00000000',maximumBlsChanges:q(4)};}
const encoded=v=>Buffer.from(JSON.stringify(v));
function setup(){const f=identity(),parameters=deriveLifecycleParameters(encoded(profile()),mode(),{maximumBootstrapAgeMs:60000}),bindings=bindLifecycleIdentities(f.domain,f.prepared,f.declarations);return {...f,parameters,bindings};}
function row(binding,{index=3,epoch=69,activation=5,...extra}={}){return {...binding,resolvedIndex:q(index),registered:true,epoch:q(epoch),activationEligibilityEpoch:q(0),activationEpoch:q(activation),exitEpoch:MAX,withdrawableEpoch:MAX,slashed:false,balanceGwei:q(32000000000),effectiveBalanceGwei:q(32000000000),withdrawalCredentials:credentials,withdrawalRecipient:address,...extra};}

test('profile rules determine the real325epoch lower bound and finite total budget',()=>{
 const {parameters:p}=setup();assert.equal(p.earliestWithdrawalEpochsAfterActivation,q(325));assert.equal(p.epochMs,q(48000));assert.equal(p.wallClockCompletionGuaranteed,false);
 for(const mutate of [m=>m.deadlines.maximumEpochsAfterActivation=324,m=>m.deadlines.maximumRunMs=15600000,m=>m.resources.maximumHistoryBlocks=q(1000),m=>m.resources.maximumIndexBytes=q(100),m=>m.resources.maximumWallSeconds=q(31)]){const m=mode();mutate(m);assert.throws(()=>deriveLifecycleParameters(encoded(profile()),m));}
 for(const mutate of [p=>p.common[7]=q(5),p=>p.common[0].registry[2]=q(5),p=>p.common[0].exit[4]='0x040',p=>p.common[0].slashing[4]=q(1024)]){const p=profile();mutate(p);assert.throws(()=>deriveLifecycleParameters(encoded(p),mode()));}
 const altered=profile();altered.common[0].exit[4]=q(128);assert.equal(deriveLifecycleParameters(encoded(altered),mode()).earliestWithdrawalEpochsAfterActivation,q(389));
});
test('synthetic action authority and two distinct roles are explicit',()=>{
 assert.equal(validateLifecycleMode(mode()).format,'nano-fresh-validator-lifecycle-v1');assert.equal(validateLifecycleMode(mode()).privateVpsLifecycleTransferAuthorized,false);assert.equal(validateLifecycleMode({...mode(),privateVpsLifecycleTransferAuthorized:true}).privateVpsLifecycleTransferAuthorized,true);
 for(const mutate of [m=>m.identities.voluntaryExit.keyEnv='FRESH_KEY_0',m=>m.syntheticActionAuthorized=false,m=>m.intentionallyAuthorConflictingHeaders=false,m=>m.identities.slashing={...m.identities.voluntaryExit},m=>m.identities.voluntaryExit.index=null,m=>m.deadlines.extra=1,m=>m.resources.maximumWallSeconds=30]){const m=mode();mutate(m);assert.throws(()=>validateLifecycleMode(m));}
});
test('pending identity joins publicKey ownership rather than index/null/keyEnv arithmetic',()=>{
 const f=identity(),bound=bindLifecycleIdentities(f.domain,f.prepared,f.declarations);assert.equal(bound.identities.voluntaryExit.publicKey,pub(4));assert.equal(bound.registryIndicesDerivedFromKeyEnv,false);
 for(const mutate of [f=>f.declarations.voluntaryExit.owner='mac-native',f=>f.declarations.voluntaryExit.keyEnv='NANO_TEST_CAPELLA_NETWORK_KEY_64',f=>f.declarations.slashing={...f.declarations.voluntaryExit},f=>f.prepared.nodes[0].signerIdentities[0].publicKey=pub(3),f=>f.prepared.nodes[0].signerIdentities[0].keyEnv='NANO_TEST_CAPELLA_NETWORK_KEY_2',f=>f.domain.campaignId='invalid',f=>f.domain.bootstrapSigningHistory.deposit.publicKey=pub(2),f=>f.domain.publicKeys[1].publicKey=pub(1)]){const f=identity();mutate(f);assert.throws(()=>bindLifecycleIdentities(f.domain,f.prepared,f.declarations));}
});
test('observations do not coerce null and exit preparation enforces actual tenure',()=>{
 const s=setup(),id=s.bindings.identities.voluntaryExit,good=row(id);assert.equal(requireLifecycleTenure(good,id,s.parameters).resolvedIndex,'0x3');
 const pending={...good,registered:false,...Object.fromEntries(['resolvedIndex','activationEligibilityEpoch','activationEpoch','exitEpoch','withdrawableEpoch','slashed','balanceGwei','effectiveBalanceGwei','withdrawalCredentials','withdrawalRecipient'].map(k=>[k,null]))};assert.equal(validateLifecycleObservation(pending,id).resolvedIndex,null);assert.throws(()=>requireLifecycleTenure(pending,id,s.parameters));
 for(const mutate of [r=>r.resolvedIndex=null,r=>r.epoch=q(68),r=>r.activationEpoch=MAX,r=>r.exitEpoch=q(69),r=>r.slashed=true,r=>r.withdrawalRecipient='0x'+'2'.repeat(40),r=>r.publicKey=pub(2),r=>r.epoch=q(500)]){const r=structuredClone(good);mutate(r);assert.throws(()=>requireLifecycleTenure(r,id,s.parameters));}
});
function payout(id,{epoch=330,amount=100n,other=7n,...extra}={}){return {publicKey:id.publicKey,resolvedIndex:id.resolvedIndex,withdrawalIndex:q(10),epoch:q(epoch),withdrawableEpoch:q(330),validatorBeforeGwei:q(amount),validatorAfterGwei:q(0),selectedWithdrawalGwei:q(amount),recipient:address,totalRecipientWithdrawalWei:q((amount+other)*1000000000n),recipientBeforeWithdrawalWei:q(900),recipientAfterWithdrawalWei:q(900n+(amount+other)*1000000000n),canonicalInclusionRoot:root(7),partial:false,...extra};}
test('withdrawal accounting uses exact selected debit and total withdrawal-step credit',()=>{
 const id={...setup().bindings.identities.voluntaryExit,resolvedIndex:q(3),withdrawalRecipient:address},v=payout(id);assert.equal(requireExactFullWithdrawal(v,id).selectedWithdrawalGwei,q(100));
 for(const mutate of [v=>v.partial=true,v=>v.epoch=q(329),v=>v.validatorAfterGwei=q(1),v=>v.selectedWithdrawalGwei=q(99),v=>v.totalRecipientWithdrawalWei=q(999),v=>v.recipientAfterWithdrawalWei=q(900+107),v=>v.recipient='0x'+'2'.repeat(40),v=>v.resolvedIndex=q(0),v=>v.canonicalInclusionRoot='bad']){const wrong=structuredClone(v);mutate(wrong);assert.throws(()=>requireExactFullWithdrawal(wrong,id));}
});
test('phase guards reject premature actions, repeated jobs and incomplete second restart',()=>{
 const s=setup(),p=createLifecycleProgress({campaignId:s.domain.campaignId,bindings:s.bindings,parameters:s.parameters}),ids=s.bindings.identities;
 p.observe('voluntaryExit',row(ids.voluntaryExit));p.observe('slashing',row(ids.slashing,{index:0,activation:0}));
 const action=role=>({publicKey:ids[role].publicKey,resolvedIndex:role==='voluntaryExit'?q(3):q(0),wireSha256:(role==='voluntaryExit'?'a':'b').repeat(64),epoch:q(69)});
 assert.throws(()=>p.actionPrepared('voluntaryExit',action('voluntaryExit')));
 const restart={owner:ids.voluntaryExit.owner,publicKey:ids.voluntaryExit.publicKey,dutyObservation:row(ids.voluntaryExit),capturedLocalDutyIdentityVerified:true,independentPrimaryWitnessPinsMatched:true,sourceAndCopiesUnchanged:true,commonFinalityAdvances:3,newFinalizedDuty:true,otherValidatorsHealthy:true,noDuplicatePayout:false};
 assert.throws(()=>p.restartVerified('after-activation',{...restart,newFinalizedDuty:false}));p.restartVerified('after-activation',restart);
 for(const role of ['voluntaryExit','slashing'])p.actionPrepared(role,action(role));assert.throws(()=>p.actionPrepared('voluntaryExit',action('voluntaryExit')));
 const accepted=role=>({preparedWireSha256:action(role).wireSha256,accepted:true,session:role,sequence:q(1)});assert.throws(()=>p.actionAccepted('voluntaryExit',{...accepted('voluntaryExit'),accepted:false}));p.actionAccepted('voluntaryExit',accepted('voluntaryExit'));
 assert.throws(()=>p.actionAccepted('slashing',{...accepted('slashing'),session:'voluntaryExit'}));p.actionAccepted('slashing',accepted('slashing'));
 const inclusion=role=>({publicKey:ids[role].publicKey,resolvedIndex:action(role).resolvedIndex,wireSha256:action(role).wireSha256,finalizedRoot:root(6),canonicalTransitionVerified:true});
 assert.throws(()=>p.actionIncluded('voluntaryExit',{...inclusion('voluntaryExit'),canonicalTransitionVerified:false}));
 for(const role of ['voluntaryExit','slashing'])p.actionIncluded(role,inclusion(role));
 assert.throws(()=>p.actionIncluded('voluntaryExit',inclusion('voluntaryExit')));
 assert.throws(()=>p.restartVerified('after-payout',{...restart,noDuplicatePayout:true}));
 for(const role of ['voluntaryExit','slashing']){
  p.observe(role,row(ids[role],{index:role==='voluntaryExit'?3:0,activation:role==='voluntaryExit'?5:0,epoch:330,exitEpoch:q(74),withdrawableEpoch:q(330),balanceGwei:q(0),slashed:role==='slashing'}));
  p.withdrawalVerified(role,payout({...ids[role],resolvedIndex:action(role).resolvedIndex},{withdrawalIndex:q(role==='voluntaryExit'?10:11)}));
 }
 assert.throws(()=>p.withdrawalVerified('voluntaryExit',payout({...ids.voluntaryExit,resolvedIndex:q(3)})));
 p.withdrawalVerified('voluntaryExit',payout({...ids.voluntaryExit,resolvedIndex:q(3)},{withdrawalIndex:q(12),amount:7n,other:0n}));
 assert.equal(p.snapshot().exactFullPayouts,2);assert.equal(p.snapshot().verifiedWithdrawals,3);
 assert.throws(()=>p.restartVerified('after-payout',{...restart,noDuplicatePayout:false}));
 const third=s.bindings.ownership.find(x=>x.publicKey===pub(3));const second={...restart,publicKey:third.publicKey,dutyObservation:row(third,{index:2,activation:0,epoch:330}),noDuplicatePayout:true};
 assert.throws(()=>p.restartVerified('after-payout',{...second,publicKey:ids.voluntaryExit.publicKey}));assert.throws(()=>p.restartVerified('after-payout',{...second,owner:'mac-native'}));assert.throws(()=>p.restartVerified('after-payout',{...second,newFinalizedDuty:false}));assert.throws(()=>p.restartVerified('after-payout',{...second,dutyObservation:{...second.dutyObservation,slashed:true}}));p.restartVerified('after-payout',second);
 const terminal={canonicalReplayVerified:true,threeStoppedOwnerCaptures:3,allExternalPrimaryWitnessPinsMatched:true,otherValidatorsHealthy:true,historicalForkChoiceVerified:false};
 assert.throws(()=>p.finish({...terminal,threeStoppedOwnerCaptures:2}));assert.equal(p.finish(terminal).finished,true);
 assert.throws(()=>p.observe('voluntaryExit',row(ids.voluntaryExit)));
});

test('single-validator adapter binds immutable state, exact decimal fields and lifecycle status',()=>{
 const id=setup().bindings.identities.voluntaryExit,query={stateRoot:root(9),epoch:q(69),publicKey:id.publicKey};
 const response={execution_optimistic:false,finalized:false,data:{index:'3',balance:'32000000000',status:'active_ongoing',validator:{pubkey:id.publicKey,withdrawal_credentials:credentials,effective_balance:'32000000000',slashed:false,activation_eligibility_epoch:'0',activation_epoch:'5',exit_epoch:((1n<<64n)-1n).toString(),withdrawable_epoch:((1n<<64n)-1n).toString()}}};
 const before=JSON.stringify(response);assert.equal(lifecycleObservationFromBeacon(response,id,query).resolvedIndex,q(3));assert.equal(JSON.stringify(response),before);
 for(const change of [j=>j.execution_optimistic=true,j=>j.data.index=null,j=>j.data.index='03',j=>j.data.balance=(1n<<64n).toString(),j=>j.data.validator.pubkey=pub(2),j=>j.data.status='withdrawal_done',j=>j.data.validator.slashed=null,j=>j.data.validator.withdrawal_credentials='0x01',j=>j.data.extra=1]){const j=structuredClone(response);change(j);assert.throws(()=>lifecycleObservationFromBeacon(j,id,query));}
 assert.throws(()=>lifecycleObservationFromBeacon(response,id,{...query,stateRoot:'head'}));assert.throws(()=>lifecycleObservationFromBeacon({code:404,message:'State not retained'},id,query));
});
test('only the freshly exported pending deposit and funded sender can be submitted',()=>{
 const {domain,bindings}=setup(),id=bindings.identities.voluntaryExit,d={kind:'native_deposit_transaction',raw:'0x0102',hash:root(2),validatorIndex:3,publicKey:id.publicKey,amountGwei:32000000000,withdrawalRecipient:address,sender:domain.transactionSender.address,nonce:'0x0',productionQualified:false};
 const evidence=validateLifecycleDeposit(d,domain,id);assert.equal(evidence.amountGwei,q(32000000000));assert.equal(evidence.rawTransaction,'0x0102');
 for(const change of [d=>d.kind='old_deposit',d=>d.validatorIndex=64,d=>d.publicKey=pub(1),d=>d.amountGwei=1,d=>d.sender='0x'+'3'.repeat(40),d=>d.withdrawalRecipient='0x'+'3'.repeat(40),d=>d.nonce=null,d=>d.productionQualified=true,d=>d.raw='0x1',d=>d.extra=1]){const bad=structuredClone(d);change(bad);assert.throws(()=>validateLifecycleDeposit(bad,domain,id));}
});

// Producer boundary: C++ public_address/checksum_address differs textually from
// Session::preview_public_identities/prepare_genesis address(...), for one key.
// This is a public address fixture only; no key, transaction or signer is used.
const exportedChecksummedSender='0x6c3Ca06387CEd1BCd4a5F920AC22C9d57FB6F047';
function exportedDepositBoundary(){
 const {domain,bindings}=setup(),id=bindings.identities.voluntaryExit;domain.transactionSender.address=exportedChecksummedSender.toLowerCase();
 const document={kind:'native_deposit_transaction',raw:'0x0102',hash:root(2),validatorIndex:3,publicKey:id.publicKey,amountGwei:32000000000,withdrawalRecipient:address,sender:exportedChecksummedSender,nonce:'0x0',productionQualified:false};
 return {domain,id,document};
}
test('exporter address boundary accepts casing of the same twenty bytes without changing the export',()=>{
 const variants=[exportedChecksummedSender,exportedChecksummedSender.toLowerCase(),'0x'+exportedChecksummedSender.slice(2).toUpperCase()];let cases=0;
 for(const sender of variants)for(const fundedSender of variants){
  const {domain,id,document}=exportedDepositBoundary();document.sender=sender;domain.transactionSender.address=fundedSender;const before=JSON.stringify({domain,document});
  assert.deepEqual(validateLifecycleDeposit(document,domain,id),{rawTransaction:document.raw,transactionHash:document.hash,amountGwei:q(32000000000),withdrawalCredentials:credentials});
  assert.equal(JSON.stringify({domain,document}),before);++cases;
 }
 assert.equal(cases,9);
});
test('exporter address boundary refuses malformed values and different address bytes on either side',()=>{
 const invalid=[null,1,{},'',exportedChecksummedSender.slice(2),'0X'+exportedChecksummedSender.slice(2),exportedChecksummedSender.slice(0,-1),exportedChecksummedSender+'0',' '+exportedChecksummedSender,exportedChecksummedSender+'\n',exportedChecksummedSender.slice(0,-1)+'g'];let refusals=0;
 for(const value of invalid)for(const side of ['export','domain','both']){
  const {domain,id,document}=exportedDepositBoundary();if(side!=='domain')document.sender=value;if(side!=='export')domain.transactionSender.address=value;
  assert.throws(()=>validateLifecycleDeposit(document,domain,id),/new funded sender and exact deposit amount/);++refusals;
 }
 for(const side of ['export','domain']){
  const {domain,id,document}=exportedDepositBoundary();if(side==='export')document.sender='0x'+'3'.repeat(40);else domain.transactionSender.address='0x'+'3'.repeat(40);
  assert.throws(()=>validateLifecycleDeposit(document,domain,id),/new funded sender and exact deposit amount/);++refusals;
 }
 assert.equal(refusals,35);
});
