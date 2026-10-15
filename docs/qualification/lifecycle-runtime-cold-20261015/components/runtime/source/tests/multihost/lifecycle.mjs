// Pure authority/phase helpers. No keys, files, clocks, processes or RPC here.
const owners=['mac-native','mac-docker','vps-docker'];
const roles=['voluntaryExit','slashing'];
const rootPattern=/^0x[0-9a-f]{64}$/,publicPattern=/^0x[0-9a-f]{96}$/;
const maximumU64=(1n<<64n)-1n;
function need(ok,message){if(!ok)throw Error('Fresh lifecycle: '+message);}
function exact(value,fields,label){need(value&&typeof value==='object'&&!Array.isArray(value)&&Object.keys(value).sort().join('\0')===[...fields].sort().join('\0'),label+' exact fields');}
function positive(value,label){need(Number.isSafeInteger(value)&&value>0,label+' positive safe integer');return value;}
export function lifecycleQuantity(value,label='quantity'){
 need(typeof value==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(value),label+' canonical quantity');
 const n=BigInt(value);need(n<=maximumU64,label+' uint64');return n;
}
const quantity=n=>'0x'+n.toString(16);
const owned=value=>structuredClone(value);

export function validateLifecycleMode(value){
 const v=owned(value);if(v&&typeof v==='object')for(const k of ['privateVpsLifecycleTransferAuthorized','crossOwnerEvidenceTransferAuthorized','runtimeDriverEnabled'])if(!Object.hasOwn(v,k))v[k]=false;exact(v,['format','identities','deadlines','resources','syntheticActionAuthorized','intentionallyAuthorConflictingHeaders','privateVpsLifecycleTransferAuthorized','crossOwnerEvidenceTransferAuthorized','runtimeDriverEnabled'],'mode');for(const k of ['privateVpsLifecycleTransferAuthorized','crossOwnerEvidenceTransferAuthorized','runtimeDriverEnabled'])need(typeof v[k]==='boolean','explicit '+k+' gate');
 need(v.format==='nano-fresh-validator-lifecycle-v1'&&v.syntheticActionAuthorized===true&&v.intentionallyAuthorConflictingHeaders===true,'explicit synthetic action authority');
 exact(v.identities,roles,'identities');
 for(const role of roles){const id=v.identities[role];exact(id,['owner','keyEnv','publicKey'],role);need(owners.includes(id.owner)&&typeof id.keyEnv==='string'&&id.keyEnv.length<=128&&/^NANO_TEST_CAPELLA_NETWORK_KEY_[A-Z0-9_]*$/.test(id.keyEnv)&&publicPattern.test(id.publicKey),role+' public owner identity');}
 need(v.identities.voluntaryExit.publicKey!==v.identities.slashing.publicKey&&v.identities.voluntaryExit.keyEnv!==v.identities.slashing.keyEnv,'disjoint exit and slashing identities');
 exact(v.deadlines,['registrationMs','activationMs','finalityMs','restoreMs','actionMs','auditMs','maximumEpochsAfterActivation','maximumRunMs'],'deadlines');
 for(const [key,n] of Object.entries(v.deadlines))positive(n,key);
 need(v.deadlines.maximumRunMs<=2147483647,'finite whole-run timer bound');
 for(const key of ['registrationMs','activationMs','finalityMs','restoreMs','actionMs','auditMs'])need(v.deadlines[key]<=v.deadlines.maximumRunMs,key+' exceeds total');
 exact(v.resources,['maximumWallSeconds','maximumHistoryBlocks','maximumArchiveBytes','maximumIndexBytes','maximumInventoryFiles','maximumInventoryBytes'],'resources');
 for(const [key,n] of Object.entries(v.resources))need(lifecycleQuantity(n,key)>0n,key+' positive quantity');
 const wall=BigInt(v.resources.maximumWallSeconds);need(wall<=600n&&wall*1000n<=BigInt(v.deadlines.auditMs)&&(wall<60n?wall:60n)*1000n<=BigInt(v.deadlines.actionMs),'tool wall budget exceeds bounded audit/action phase');
 return v;
}

export function deriveLifecycleParameters(profileBytes,mode,{maximumBootstrapAgeMs=0}={}){
 need(Buffer.isBuffer(profileBytes)&&profileBytes.length>0&&profileBytes.length<=1048576,'bounded pinned profile bytes');
 const p=JSON.parse(profileBytes);exact(p,['format','common','withdrawals','genesisVersion','maximumBlsChanges'],'Capella profile');
 need(p.format==='nano-capella-profile-v1'&&Array.isArray(p.common)&&p.common.length>=12,'Capella profile');
 const c=p.common[0],a=(name,length)=>{need(Array.isArray(c?.[name])&&c[name].length===length,name+' shape');return c[name].map((x,i)=>lifecycleQuantity(x,name+i));};
 const exit=a('exit',5),registry=a('registry',6),slashing=a('slashing',10),ffg=a('ffg',2),sync=a('epochSync',5);
 for(let i=0;i<4;++i)need(exit[i]===registry[i]&&exit[i]===slashing[i],'exit/registry/slashing profile differs');
 const slots=ffg[0],slotMs=lifecycleQuantity(p.common[1],'slotMs'),seconds=lifecycleQuantity(p.common[7],'secondsPerSlot');
 need(slots>0n&&sync[0]===slots&&slotMs>0n&&seconds>0n&&slotMs===seconds*1000n,'profile clocks differ');
 need(exit[0]>0n&&exit[1]>0n&&exit[3]>0n&&exit[4]>0n&&slashing[4]>0n,'positive lifecycle rules');
 const epochMs=slots*slotMs,exitDelay=1n+exit[2]+exit[3],earliest=exit[4]+(exitDelay>slashing[4]?exitDelay:slashing[4]),v=validateLifecycleMode(mode),d=v.deadlines;
 need(Number.isSafeInteger(maximumBootstrapAgeMs)&&maximumBootstrapAgeMs>=0,'bootstrap age budget');
 need(BigInt(d.maximumEpochsAfterActivation)>=earliest,'declared horizon is shorter than real tenure and withdrawal delay');
 const ancillary=BigInt(d.registrationMs)+BigInt(d.activationMs)+6n*BigInt(d.finalityMs)+2n*BigInt(d.restoreMs)+2n*BigInt(d.actionMs)+3n*BigInt(d.auditMs);
 const minimumRun=ancillary+BigInt(d.maximumEpochsAfterActivation)*epochMs;
 need(BigInt(d.maximumRunMs)>=minimumRun,'whole-run deadline omits real lifecycle interval or stages');
 const endSlot=(BigInt(d.maximumRunMs)+BigInt(maximumBootstrapAgeMs)+slotMs-1n)/slotMs+2n;
 need(endSlot<=maximumU64&&BigInt(v.resources.maximumHistoryBlocks)>=endSlot,'history bound cannot cover fixed slot horizon');
 // Public streamer encoding: root32,parent32,slotLE8,lengthLE8,wireSha32.
 need(BigInt(v.resources.maximumIndexBytes)>=BigInt(v.resources.maximumHistoryBlocks)*112n,'scalar index budget cannot cover declared history');
 return {format:'nano-fresh-lifecycle-parameters-v1',slotsPerEpoch:quantity(slots),slotMs:quantity(slotMs),epochMs:quantity(epochMs),tenureEpochs:quantity(exit[4]),exitLookaheadEpochs:quantity(exit[2]),withdrawalDelayEpochs:quantity(exit[3]),slashingVectorEpochs:quantity(slashing[4]),earliestWithdrawalEpochsAfterActivation:quantity(earliest),maximumEpochsAfterActivation:quantity(BigInt(d.maximumEpochsAfterActivation)),maximumEndSlot:quantity(endSlot),minimumWholeRunMs:quantity(minimumRun),maximumRunMs:d.maximumRunMs,wallClockCompletionGuaranteed:false};
}

export function bindLifecycleIdentities(domain,prepared,declarations){
 const m=owned(domain),p=owned(prepared),decl=owned(declarations);exact(decl,roles,'declared identities');
 need(m?.format==='nano-isolated-capella-fixture-domain-v1'&&m.synthetic===true&&m.productionQualified===false&&rootPattern.test('0x'+m.campaignId),'fresh synthetic campaign');
 need(Number.isSafeInteger(m.registryCount)&&m.registryCount>0&&Array.isArray(m.publicKeys)&&m.publicKeys.length===m.registryCount+1,'registered plus one pending public identity');
 const pubs=new Set();for(const [index,row] of m.publicKeys.entries()){exact(row,['index','publicKey'],'manifest public key');need(row.index===index&&publicPattern.test(row.publicKey)&&!pubs.has(row.publicKey),'unique manifest identities');pubs.add(row.publicKey);}
 const pending=m.bootstrapSigningHistory?.deposit?.publicKey;need(publicPattern.test(pending)&&pending===m.publicKeys[m.registryCount].publicKey,'pending deposit public identity');
 need(Array.isArray(p.nodes)&&p.nodes.length===3,'three prepared owners');const bound=new Map(),envs=new Set();
 for(let i=0;i<owners.length;++i){const n=p.nodes[i];need(n.name===owners[i]&&Array.isArray(n.signerIdentities),'ordered signer ownership');for(const id of n.signerIdentities){exact(id,['keyEnv','publicKey'],'prepared signer identity');need(pubs.has(id.publicKey)&&!bound.has(id.publicKey)&&!envs.has(id.keyEnv)&&typeof id.keyEnv==='string'&&id.keyEnv.length<=128&&/^NANO_TEST_CAPELLA_NETWORK_KEY_[A-Z0-9_]*$/.test(id.keyEnv),'no duplicate/substituted signer');bound.set(id.publicKey,{...id,owner:n.name});envs.add(id.keyEnv);}}
 need(bound.size===pubs.size,'every public identity has exactly one owner');const result={};
 for(const role of roles){const id=decl[role];exact(id,['owner','keyEnv','publicKey'],role);const found=bound.get(id.publicKey);need(found&&found.owner===id.owner&&found.keyEnv===id.keyEnv,role+' exact independent identity join');result[role]={...found};}
 need(result.voluntaryExit.publicKey===pending&&result.slashing.publicKey!==pending&&result.slashing.keyEnv!==result.voluntaryExit.keyEnv,'pending exit and disjoint registered slashing identity');
 return {campaignId:m.campaignId,identities:result,ownership:[...bound.values()].map(owned),registryIndicesDerivedFromKeyEnv:false};
}

// These rules validate typed observations. The caller must obtain them from the
// real pinned-state endpoint or stopped RO auditor, never manufacture them.
export function validateLifecycleObservation(row,binding){
 const v=owned(row),id=binding;exact(v,['owner','keyEnv','publicKey','resolvedIndex','registered','epoch','activationEligibilityEpoch','activationEpoch','exitEpoch','withdrawableEpoch','slashed','balanceGwei','effectiveBalanceGwei','withdrawalCredentials','withdrawalRecipient'],'validator observation');need(v.owner===id.owner&&v.keyEnv===id.keyEnv&&v.publicKey===id.publicKey,'observation identity');
 need(typeof v.registered==='boolean','registered boolean');lifecycleQuantity(v.epoch,'observed epoch');
 if(!v.registered){for(const key of ['resolvedIndex','activationEligibilityEpoch','activationEpoch','exitEpoch','withdrawableEpoch','slashed','balanceGwei','effectiveBalanceGwei','withdrawalCredentials','withdrawalRecipient'])need(v[key]===null,'absent identity must not manufacture '+key);return v;}
 need(typeof v.slashed==='boolean','slashed boolean');
 need(v.resolvedIndex!==null,'registered index must not be null');lifecycleQuantity(v.resolvedIndex,'resolved registry index');
 for(const key of ['activationEligibilityEpoch','activationEpoch','exitEpoch','withdrawableEpoch','balanceGwei','effectiveBalanceGwei'])lifecycleQuantity(v[key],key);
 need(rootPattern.test(v.withdrawalCredentials),'withdrawal credentials');const credentials=v.withdrawalCredentials.slice(2);
 const recipient=credentials.startsWith('01')?'0x'+credentials.slice(24):null;need(v.withdrawalRecipient===recipient,'recipient must come from authenticated credentials');
 const activation=lifecycleQuantity(v.activationEpoch),exit=lifecycleQuantity(v.exitEpoch),withdrawable=lifecycleQuantity(v.withdrawableEpoch);
 need(activation<=exit&&exit<=withdrawable,'registry lifecycle chronology');return v;
}

// The route adapter is usable only after ROOT qualifies the new endpoint.
// A successful immutable-state selector is mandatory; a generic HTTP404 cannot
// be interpreted as an absent validator (it may mean the state was pruned).
export function lifecycleObservationFromBeacon(response,binding,query){
 exact(query,['stateRoot','epoch','publicKey'],'validator query authority');need(rootPattern.test(query.stateRoot)&&query.publicKey===binding.publicKey,'immutable selected-state/public-key query');const epoch=lifecycleQuantity(query.epoch);
 const j=owned(response);exact(j,['execution_optimistic','finalized','data'],'single-validator response');need(j.execution_optimistic===false&&typeof j.finalized==='boolean','authenticated execution/ancestry status');exact(j.data,['index','balance','status','validator'],'single-validator data');
 const decimal=(x,label)=>{need(typeof x==='string'&&/^(?:0|[1-9][0-9]*)$/.test(x),label+' canonical decimal');const n=BigInt(x);need(n<=maximumU64,label+' uint64');return quantity(n);};
 const v=j.data.validator;exact(v,['pubkey','withdrawal_credentials','effective_balance','slashed','activation_eligibility_epoch','activation_epoch','exit_epoch','withdrawable_epoch'],'validator fields');need(v.pubkey===binding.publicKey&&typeof v.slashed==='boolean','selected validator identity');
 const row={...owned(binding),resolvedIndex:decimal(j.data.index,'validator index'),registered:true,epoch:quantity(epoch),activationEligibilityEpoch:decimal(v.activation_eligibility_epoch,'eligibility epoch'),activationEpoch:decimal(v.activation_epoch,'activation epoch'),exitEpoch:decimal(v.exit_epoch,'exit epoch'),withdrawableEpoch:decimal(v.withdrawable_epoch,'withdrawable epoch'),slashed:v.slashed,balanceGwei:decimal(j.data.balance,'validator balance'),effectiveBalanceGwei:decimal(v.effective_balance,'effective balance'),withdrawalCredentials:v.withdrawal_credentials,withdrawalRecipient:typeof v.withdrawal_credentials==='string'&&v.withdrawal_credentials.startsWith('0x01')?'0x'+v.withdrawal_credentials.slice(-40):null};
 validateLifecycleObservation(row,binding);const a=lifecycleQuantity(row.activationEpoch),x=lifecycleQuantity(row.exitEpoch),w=lifecycleQuantity(row.withdrawableEpoch),balance=lifecycleQuantity(row.balanceGwei);
 const expected=epoch<a?(lifecycleQuantity(row.activationEligibilityEpoch)===maximumU64?'pending_initialized':'pending_queued'):epoch<x?(v.slashed?'active_slashed':x===maximumU64?'active_ongoing':'active_exiting'):epoch<w?(v.slashed?'exited_slashed':'exited_unslashed'):(balance===0n?'withdrawal_done':'withdrawal_possible');
 need(j.data.status===expected,'validator status differs from authenticated epoch/fields');return row;
}

export function validateLifecycleDeposit(document,domain,binding){
 const d=owned(document);exact(d,['kind','raw','hash','validatorIndex','publicKey','amountGwei','withdrawalRecipient','sender','nonce','productionQualified'],'exported deposit');
 need(d.kind==='native_deposit_transaction'&&d.productionQualified===false&&d.publicKey===binding.publicKey&&d.publicKey===domain.bootstrapSigningHistory.deposit.publicKey&&d.validatorIndex===domain.registryCount,'new pending public deposit identity');
 need(typeof d.raw==='string'&&/^0x(?:[0-9a-f]{2})+$/.test(d.raw)&&d.raw.length<=2+2*1048576&&rootPattern.test(d.hash),'bounded exported raw transaction/commitment');
 need(Number.isSafeInteger(d.amountGwei)&&d.amountGwei>0&&d.amountGwei===domain.bootstrapSigningHistory.deposit.amountGwei&&d.sender===domain.transactionSender.address&&/^0x[0-9a-f]{40}$/.test(d.sender),'new funded sender and exact deposit amount');lifecycleQuantity(d.nonce,'deposit nonce');
 const credentials=domain.bootstrapSigningHistory.deposit.withdrawalCredentials;need(rootPattern.test(credentials)&&credentials.startsWith('0x01')&&d.withdrawalRecipient==='0x'+credentials.slice(-40),'exported execution withdrawal credentials');
 // C++ closing replay, not a Node SHA3 approximation, authenticates raw/hash.
 return {rawTransaction:d.raw,transactionHash:d.hash,amountGwei:quantity(BigInt(d.amountGwei)),withdrawalCredentials:credentials};
}

export function requireLifecycleTenure(row,binding,parameters){
 const v=validateLifecycleObservation(row,binding);need(v.registered,'unregistered voluntary exit');
 const epoch=lifecycleQuantity(v.epoch),activation=lifecycleQuantity(v.activationEpoch),exit=lifecycleQuantity(v.exitEpoch);
 need(!v.slashed&&activation!==maximumU64&&epoch>=activation&&exit===maximumU64,'untouched active voluntary-exit identity');
 need(epoch-activation>=lifecycleQuantity(parameters.tenureEpochs),'actual tenure not reached');
 need(epoch-activation<=lifecycleQuantity(parameters.maximumEpochsAfterActivation),'fixed lifecycle horizon missed');return v;
}

export function requireExactFullWithdrawal(event,binding){
 const v=owned(event);exact(v,['publicKey','resolvedIndex','withdrawalIndex','epoch','withdrawableEpoch','validatorBeforeGwei','validatorAfterGwei','selectedWithdrawalGwei','recipient','totalRecipientWithdrawalWei','recipientBeforeWithdrawalWei','recipientAfterWithdrawalWei','canonicalInclusionRoot','partial'],'full withdrawal');
 need(v.publicKey===binding.publicKey&&v.resolvedIndex===binding.resolvedIndex&&v.partial===false&&rootPattern.test(v.canonicalInclusionRoot),'exact finalized full-withdrawal identity');lifecycleQuantity(v.resolvedIndex,'withdrawal index');
 lifecycleQuantity(v.withdrawalIndex,'canonical withdrawal index');const amount=lifecycleQuantity(v.selectedWithdrawalGwei),before=lifecycleQuantity(v.validatorBeforeGwei),after=lifecycleQuantity(v.validatorAfterGwei);
 need(lifecycleQuantity(v.epoch)>=lifecycleQuantity(v.withdrawableEpoch)&&amount>0n&&amount===before&&after===0n,'full debit only after withdrawability');
 need(/^0x[0-9a-f]{40}$/.test(v.recipient)&&v.recipient===binding.withdrawalRecipient,'authenticated recipient');
 const wei=x=>{need(typeof x==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(x),'canonical Wei');const n=BigInt(x);need(n<(1n<<256n),'Wei uint256');return n;};
 const total=wei(v.totalRecipientWithdrawalWei);need(total>=amount*1000000000n&&wei(v.recipientAfterWithdrawalWei)-wei(v.recipientBeforeWithdrawalWei)===total,'exact withdrawal-step Wei credit');return v;
}

// Bounded semantic state only, not an authenticity oracle. Adapters must bind
// these typed inputs to real tool/RPC/checkpoint evidence before calling here.
export function createLifecycleProgress({campaignId,bindings,parameters}){
 need(rootPattern.test('0x'+campaignId)&&bindings.campaignId===campaignId,'phase campaign identity');
 const ids=owned(bindings.identities),ownership=owned(bindings.ownership),p=owned(parameters),observations=new Map(),prepared=new Map(),accepted=new Set(),jobs=new Set(),included=new Map(),payouts=new Map();
 let firstRestart=false,secondRestart=false,finished=false,lastWithdrawalIndex=-1n,verifiedWithdrawals=0;
 const roleNeed=role=>need(roles.includes(role),'known lifecycle role');
 const active=row=>row.registered&&lifecycleQuantity(row.activationEpoch)!==maximumU64&&lifecycleQuantity(row.activationEpoch)<=lifecycleQuantity(row.epoch)&&lifecycleQuantity(row.epoch)<lifecycleQuantity(row.exitEpoch);
 const fresh=()=>need(!finished,'terminal lifecycle state cannot advance');
 const hasInitialActivation=()=>{const v=observations.get('voluntaryExit');return v&&active(v);};
 const snapshot=()=>({format:'nano-fresh-lifecycle-progress-v1',campaignId,registeredRoles:[...observations].filter(([,v])=>v.registered).map(([k])=>k).sort(),firstRestart,preparedActions:prepared.size,acceptedActions:accepted.size,canonicalActions:included.size,exactFullPayouts:payouts.size,verifiedWithdrawals,secondRestart,finished,productionQualified:false});
 return {
  observe(role,value){
   fresh();roleNeed(role);const row=validateLifecycleObservation(value,ids[role]),before=observations.get(role);
   if(before){need(lifecycleQuantity(row.epoch)>=lifecycleQuantity(before.epoch),'observation epoch regressed');if(before.registered){need(row.registered&&row.resolvedIndex===before.resolvedIndex,'registered identity/index changed');for(const key of ['activationEpoch','withdrawalCredentials'])if(before[key]!==quantity(maximumU64))need(row[key]===before[key],key+' changed');}}
   if(role==='voluntaryExit'&&row.registered)need(row.slashed===false,'voluntary-exit identity was slashed');
   if(row.registered)for(const [other,prior] of observations)if(other!==role&&prior.registered)need(prior.resolvedIndex!==row.resolvedIndex,'two identities resolve to the same registry index');
   if(row.registered&&lifecycleQuantity(row.activationEpoch)!==maximumU64)need(lifecycleQuantity(row.epoch)<=lifecycleQuantity(row.activationEpoch)+lifecycleQuantity(p.maximumEpochsAfterActivation),'fixed lifecycle epoch horizon missed');
   observations.set(role,owned(row));return owned(row);
  },
  restartVerified(phase,evidence){
   fresh();const v=owned(evidence);exact(v,['owner','publicKey','dutyObservation','capturedLocalDutyIdentityVerified','independentPrimaryWitnessPinsMatched','sourceAndCopiesUnchanged','commonFinalityAdvances','newFinalizedDuty','otherValidatorsHealthy','noDuplicatePayout'],'restart evidence');
   need(v.owner===ids.voluntaryExit.owner&&v.capturedLocalDutyIdentityVerified===true&&v.independentPrimaryWitnessPinsMatched===true&&v.sourceAndCopiesUnchanged===true&&Number.isSafeInteger(v.commonFinalityAdvances)&&v.commonFinalityAdvances>=3&&v.newFinalizedDuty===true&&v.otherValidatorsHealthy===true,'independent restart, local duty and common finality evidence required');
   const local=ownership.filter(x=>x.publicKey===v.publicKey&&x.owner===v.owner);need(local.length===1,'restart duty must belong to the restored owner');const duty=validateLifecycleObservation(v.dutyObservation,local[0]);need(active(duty)&&duty.slashed===false,'restart proposal requires a currently active unslashed local identity');
   if(phase==='after-activation'){need(!firstRestart&&!secondRestart&&hasInitialActivation()&&v.publicKey===ids.voluntaryExit.publicKey,'first restart requires real activation and resumed target duty');firstRestart=true;}
   else{need(phase==='after-payout'&&firstRestart&&!secondRestart&&payouts.size===2&&v.noDuplicatePayout===true&&v.publicKey!==ids.voluntaryExit.publicKey&&v.publicKey!==ids.slashing.publicKey,'second restart requires both exact payouts, a third local active identity and no repeat');secondRestart=true;}
  },
  actionPrepared(role,value){
   fresh();roleNeed(role);const v=owned(value);exact(v,['publicKey','resolvedIndex','wireSha256','epoch'],'prepared action');
   need(firstRestart&&!secondRestart&&!prepared.has(role),'one action only after independent first restart');
   const row=observations.get(role);need(row&&active(row)&&v.publicKey===ids[role].publicKey&&v.resolvedIndex===row.resolvedIndex&&v.epoch===row.epoch&&/^[0-9a-f]{64}$/.test(v.wireSha256),'prepared action binds observed identity/epoch');
   if(role==='voluntaryExit')requireLifecycleTenure(row,ids[role],p);else need(row.slashed===false,'slashing identity already slashed');
   for(const other of prepared.values())need(other.wireSha256!==v.wireSha256,'prepared operation commitment reused');
   prepared.set(role,v);
  },
  actionAccepted(role,value){
   fresh();roleNeed(role);const v=owned(value);exact(v,['preparedWireSha256','accepted','session','sequence'],'accepted action');
   need(prepared.has(role)&&!accepted.has(role)&&v.accepted===true&&v.preparedWireSha256===prepared.get(role).wireSha256&&typeof v.session==='string'&&v.session.length>0&&lifecycleQuantity(v.sequence)>0n,'one actual accepted job bound to prepared operation');const key=v.session+'\0'+v.sequence;need(!jobs.has(key),'job identity reused across actions');jobs.add(key);accepted.add(role);
  },
  actionIncluded(role,value){
   fresh();roleNeed(role);const v=owned(value);exact(v,['publicKey','resolvedIndex','wireSha256','finalizedRoot','canonicalTransitionVerified'],'canonical action');
   need(accepted.has(role)&&!included.has(role)&&v.publicKey===ids[role].publicKey&&v.resolvedIndex===prepared.get(role).resolvedIndex&&v.wireSha256===prepared.get(role).wireSha256&&rootPattern.test(v.finalizedRoot)&&v.canonicalTransitionVerified===true,'accepted operation requires exact canonical finalized inclusion');included.set(role,v);
  },
  withdrawalVerified(role,event){
   fresh();roleNeed(role);need(included.has(role),'payout after exact action inclusion');const row=observations.get(role);need(row?.registered&&row.withdrawalRecipient!==null,'execution withdrawal credentials required');
   const v=requireExactFullWithdrawal(event,{...ids[role],resolvedIndex:row.resolvedIndex,withdrawalRecipient:row.withdrawalRecipient}),index=lifecycleQuantity(v.withdrawalIndex);
   need(index>lastWithdrawalIndex,'withdrawal index replay or noncanonical order');lastWithdrawalIndex=index;++verifiedWithdrawals;
   // New sync rewards after an exit may fund a later legitimate full sweep.
   // Keep only the first payout per identity; every later fed index stays strict.
   if(!payouts.has(role))payouts.set(role,v);
  },
  finish(evidence){
   fresh();exact(evidence,['canonicalReplayVerified','threeStoppedOwnerCaptures','allExternalPrimaryWitnessPinsMatched','otherValidatorsHealthy','historicalForkChoiceVerified'],'terminal evidence');
   need(firstRestart&&secondRestart&&included.size===2&&payouts.size===2&&evidence.canonicalReplayVerified===true&&evidence.threeStoppedOwnerCaptures===3&&evidence.allExternalPrimaryWitnessPinsMatched===true&&evidence.otherValidatorsHealthy===true&&evidence.historicalForkChoiceVerified===false,'complete bounded audit and restart evidence required');finished=true;return snapshot();
  },
  snapshot
 };
}
