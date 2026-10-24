// Pure contracts/coordinator. Runtime, copy, capture and audit effects stay in run.mjs.
import {basename,dirname,isAbsolute,normalize} from 'node:path';
const need=(ok,message)=>{if(!ok)throw Error('Owner recovery: '+message);};
const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
const root=/^0x[0-9a-f]{64}$/;
const quantity=v=>{need(typeof v==='string'&&/^(?:0|[1-9][0-9]*|0x(?:0|[1-9a-f][0-9a-f]*))$/.test(v),'canonical quantity');return BigInt(v);};
const checkpoint=v=>{need(v&&root.test(v.root),'checkpoint root');quantity(v.epoch);return v;};
const path=p=>need(typeof p==='string'&&isAbsolute(p)&&normalize(p)===p&&!/[\0\r\n]/.test(p),'canonical owner path');
// Additional live phases only: before/after proposals, one three-epoch finality
// observation and the complete stopped-copy/restart window for each Mac owner.
// Existing lifecycle terminal audits remain counted by deriveLifecycleParameters.
export function minimumMacOwnerRecoveryMs(deadlines){
 for(const key of ['restoreMs','finalityMs'])need(Number.isSafeInteger(deadlines?.[key])&&deadlines[key]>0,'positive recovery deadline '+key);
 return 2n*(BigInt(deadlines.restoreMs)+3n*BigInt(deadlines.finalityMs));
}
export function relocateOwnerRuntimeConfiguration(node,{oldDataRoot,oldWitnessRoot,dataRoot,witnessRoot}){
 for(const p of [oldDataRoot,oldWitnessRoot,dataRoot,witnessRoot])path(p);
 const old=[oldDataRoot,oldWitnessRoot],next=[dataRoot,witnessRoot];
 need(new Set([...old,...next]).size===4,'fresh independent owner roots');
 for(const a of [...old,...next])for(const b of [...old,...next])if(a!==b)need(!b.startsWith(a+'/'),'disjoint owner roots');
 need(basename(dataRoot)===basename(oldDataRoot)&&basename(witnessRoot)===basename(oldWitnessRoot)&&basename(dirname(dataRoot))===basename(dirname(oldDataRoot))&&basename(dirname(witnessRoot))===basename(dirname(oldWitnessRoot)),'owner/domain suffix unchanged');
 const result=structuredClone(node),c=result.capella;
 need(c&&!result.bellatrix&&Array.isArray(c.validator?.keys)&&c.validator.keys.length>0&&result.p2p_identity===oldDataRoot+'/peer.key'&&c.recoveryWitness===oldWitnessRoot+'/chain-witness','explicit existing owner configuration');
 result.p2p_identity=dataRoot+'/peer.key';c.recoveryWitness=witnessRoot+'/chain-witness';
 for(const key of c.validator.keys){
  const journal=basename(key.journal),witness=basename(key.recoveryWitness);
  need(/^journal-(0|[1-9][0-9]*)$/.test(journal)&&witness==='witness-'+journal.slice(8)&&key.journal===oldDataRoot+'/'+journal&&key.recoveryWitness===oldWitnessRoot+'/'+witness,'exact original journal/witness mapping');
  key.journal=dataRoot+'/'+journal;key.recoveryWitness=witnessRoot+'/'+witness;
 }
 return result;
}
export function compareOwnerRecoveryCaptures(source,restored){
 need(source?.passed===true&&restored?.passed===true&&['mac-native','mac-docker'].includes(source.owner),'actual Mac owner captures');
 for(const value of [source,restored]){
  need(value.format==='nano-isolated-stopped-recovery-pins-v1'&&value.primaryWitnessPairs===16&&value.databasesReadOnly===34,'complete owner role inventory');
  // Equal omissions must not authenticate complete components. The pinned C++
  // decoder remains the semantic authority; these checks bind its role bodies
  // and identities before comparing the two independently captured documents.
  const object=v=>v!==null&&typeof v==='object'&&!Array.isArray(v),pairFields=['primary','witness','witnessPrevious','witnessNext','snapshot','witnessSnapshot','restart'];
  const completeToken=token=>{need(object(token)&&Object.keys(token).sort().join(',')==='digest,sequence'&&root.test(token.digest)&&quantity(token.sequence)>0n,'complete captured token body');};
  const completePair=(pair,fields)=>{need(object(pair)&&fields.every(key=>Object.hasOwn(pair,key)),'complete captured pair body');for(const key of ['primary','witness','witnessNext','restart'])completeToken(pair[key]);if(pair.witnessPrevious!==null)completeToken(pair.witnessPrevious);need(typeof pair.snapshot==='string'&&pair.snapshot.length>0&&typeof pair.witnessSnapshot==='string'&&pair.witnessSnapshot.length>0,'captured pair snapshot bodies');};
  completePair(value.core,[...pairFields,'archive']);need(Array.isArray(value.core.archive),'captured core archive body');
  need(Array.isArray(value.validatorKeys)&&value.validatorKeys.length===16,'sixteen captured signer pair bodies');
  const first=value.owner==='mac-native'?0:16,publicKeys=new Set();
  for(let offset=0;offset<16;++offset){const row=value.validatorKeys[offset],index=first+offset;completePair(row,[...pairFields,'publicKeyIndex','index','publicKey','retired','proposal','attestation','registeredInCurrentHead']);need(row.publicKeyIndex===index&&quantity(row.index)===BigInt(index)&&/^0x[0-9a-f]{96}$/.test(row.publicKey)&&!publicKeys.has(row.publicKey),'complete unique owner signer identities');publicKeys.add(row.publicKey);}
  for(const key of ['externalStopTokensMatched','externalWitnessTokensMatched','externalWitnessPinOriginExternallyRequired','canonicalStoreSnapshotVerified','primaryAndWitnessBlobClosuresVerified','allObservedFileBytesAndInodesUnchanged'])need(value[key]===true,'authenticated unchanged capture '+key);
  need(value.newSignaturesCreated===0&&value.journalWritesRequested===false&&value.runtimeStarted===false&&value.productionQualified===false,'read-only capture scope');
 }
 for(const key of ['owner','campaignId','chainId','genesisHash','genesisValidatorsRoot','manifestSha256','profileSha256','consensusIdentity','forkDigest','externalStopReceiptSha256','restart','recoveryWitnessCommitment','core','validatorKeys','finalized','justifiedEpoch','headSlot','registryCount'])need(Object.hasOwn(source,key)&&Object.hasOwn(restored,key)&&canonical(source[key])===canonical(restored[key]),'source/copy commitment differs: '+key);
 return {owner:source.owner,primaryWitnessPairs:16,readOnlyDatabaseRoles:34,sourceAndRestoredCommitmentsEqual:true,externalWitnessTokensMatched:true,newSignaturesCreated:0,productionQualified:false};
}
export async function runMacOwnerRecoveries({initialFinality,ops}){
 need(initialFinality?.result&&ops&&['check','selectDuty','restore','finality','finalizedProposal','record'].every(k=>typeof ops[k]==='function'),'complete recovery capabilities');
 let stable=initialFinality;checkpoint(stable.result.lastCheckpoint);const rows=[];
 for(const owner of ['mac-native','mac-docker']){
  ops.check();const duty=await ops.selectDuty(owner,stable.result.lastCheckpoint);ops.check();
  need(duty?.owner===owner&&/^0x[0-9a-f]{96}$/.test(duty.publicKey)&&root.test(duty.beforeProposalRoot),'independent active owner duty');quantity(duty.index);
  const restored=await ops.restore(owner,duty);ops.check();
  need(restored?.owner===owner&&restored.sourceCapture&&restored.restoredCapture&&restored.copiesVerifiedBeforeResume===true&&restored.peerIdentityUnchanged===true,'actual complete restored owner');checkpoint(restored.finalized);const cutoff=quantity(restored.cutoffSlot);
  stable=await ops.finality(restored.finalized,'owner-recovery-'+owner);ops.check();checkpoint(stable?.result?.lastCheckpoint);
  need(stable.result.epochs===3&&BigInt(stable.result.lastEpoch)>=quantity(restored.finalized.epoch)+3n,'three-owner finality after recovery');
  const after=await ops.finalizedProposal(owner,duty,stable.result.lastCheckpoint,cutoff);ops.check();
  need(root.test(after?.root)&&after.root!==duty.beforeProposalRoot&&quantity(after.slot)>cutoff,'same-owner finalized proposal after cutoff');
  const record={owner,dutyPublicKey:duty.publicKey,dutyIndex:duty.index,beforeProposalRoot:duty.beforeProposalRoot,afterProposalRoot:after.root,cutoffSlot:restored.cutoffSlot,sourceCapture:restored.sourceCapture,restoredCapture:restored.restoredCapture,finalizedBefore:restored.finalized,stable:stable.result,peerIdentityUnchanged:true,copiesVerifiedBeforeResume:true,productionQualified:false};
  await ops.record(record);ops.check();rows.push(record);
 }
 return {format:'nano-fresh-mac-owner-recovery-v1',passed:true,owners:['mac-native','mac-docker'],restoresExecuted:2,records:rows,finality:stable,runtimeRestorationInferredFromOfflineAudit:false,productionQualified:false};
}
