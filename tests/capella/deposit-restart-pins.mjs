import assert from 'node:assert/strict';
const pin=t=>{assert.ok(t&&typeof t==='object'&&Object.keys(t).length===2);assert.match(t.sequence,/^0x[1-9a-f][0-9a-f]*$/);assert.match(t.digest,/^0x[0-9a-f]{64}$/);return structuredClone(t);};
function applyPins(configuration,receipt,pendingPublicKey){
 assert.match(pendingPublicKey,/^0x[0-9a-f]{96}$/);
 const c=structuredClone(configuration),keys=c.capella.validator.keys,pins=receipt.validatorKeys;
 assert.equal(pins.length,keys.length);
 const identity=x=>x.index===null?(assert.equal(x.publicKey,pendingPublicKey),'key:'+x.publicKey):'index:'+BigInt(x.index).toString();
 const inventory=new Map();for(const p of pins){const id=identity(p);assert.ok(!inventory.has(id),'Duplicate signer receipt');inventory.set(id,pin(p.restart));}
 for(const k of keys){const id=k.index===null?'key:'+pendingPublicKey:'index:'+BigInt(k.index).toString();assert.ok(inventory.has(id),'Missing signer commitment');k.restart=inventory.get(id);inventory.delete(id);}
 assert.equal(inventory.size,0);c.capella.restart=null;c.capella.initial=null;c.capella.recoveryCommitment=pin(receipt.restart);c.capella.automaticRecovery=false;
 return c;
}

export function pinnedDepositRestart(configuration,receipt,pendingPublicKey){
 assert.equal(receipt.clockHealthy,true);assert.equal(receipt.validator?.healthy,true);assert.equal(receipt.validator.failure,null);
 assert.equal(receipt.validator.localKeys,configuration.capella.validator.keys.length);
 return applyPins(configuration,receipt,pendingPublicKey);
}
export function pinnedQuiescentRestart(configuration,capsule,pendingPublicKey,binding){
 assert.equal(binding.writersSuspended,true,'Writer suspension must be observed by the controller');
 assert.match(binding.genesisHash,/^0x[0-9a-f]{64}$/);assert.match(binding.finalized.root,/^0x[0-9a-f]{64}$/);assert.ok(BigInt(binding.finalized.epoch)>0n);assert.match(configuration.capella.profile.sha256,/^0x[0-9a-f]{64}$/);
 assert.equal(capsule.format,'nano-private-quiescent-recovery-pins-v1');assert.equal(capsule.chainId,'0x539');
 assert.equal(capsule.readOnlyStores,true);assert.equal(capsule.primaryWitnessAndSignersVerified,true);assert.equal(capsule.minimumSequenceBoundsVerified,true);
 for(const key of capsule.validatorKeys)assert.equal(key.retired,false,'Retired keys cannot resume signing');
 assert.equal(capsule.sourceQuiescenceExternallyRequired,true);assert.equal(capsule.runtimeHealthInferred,false);assert.equal(capsule.productionQualified,false);
 assert.equal(capsule.genesisHash,binding.genesisHash);assert.equal(capsule.profileSha256,configuration.capella.profile.sha256);
 const a=capsule.canonicalReplay;assert.equal(a.canonicalHistoryVerified,true);assert.equal(a.externalFinalizedAncestorVerified,true);
 assert.equal(BigInt(a.requiredFinalized.epoch),BigInt(binding.finalized.epoch));assert.equal(a.requiredFinalized.root,binding.finalized.root);
 return applyPins(configuration,capsule,pendingPublicKey);
}
