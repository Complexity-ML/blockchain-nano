// Independent stop receipts accompany a backup; never derive pins from it.
function token(value) {
 if(!value||Object.keys(value).length!==2||!/^0x[1-9a-f][0-9a-f]*$/.test(value.sequence)||!/^0x[0-9a-f]{64}$/.test(value.digest))throw Error('Invalid independent restart commitment');
 return structuredClone(value);
}
export function stoppedReceipt(text) {
 const rows=text.split('\n').flatMap(line=>{try{const row=JSON.parse(line);return row.event==='capella-runtime-stopped'?[row]:[];}catch{return [];}});
 if(rows.length!==1)throw Error('Expected one stopped Capella receipt');
 const r=rows[0];token(r.restart);
 if(r.clockHealthy!==true||r.validator?.healthy!==true||r.validator.failure!==null||!Number.isSafeInteger(r.validator.attestationsAdmitted)||r.validator.attestationsAdmitted<=0)throw Error('Stopped signer/clock unhealthy or inactive');
 if(!/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(r.finalized?.epoch)||!/^0x[0-9a-f]{64}$/.test(r.finalized?.root))throw Error('Missing stopped finality');
 return r;
}
// Automatic indices remain null even after admission. Their independently
// prepared keyEnv/publicKey inventory, never the backup, supplies the identity.
export function pinnedRestoreConfiguration(node, receipt, signerIdentities) {
 const result=structuredClone(node),c=result.capella;
 if(!c||node.bellatrix||!Array.isArray(c.validator?.keys)||!c.validator.keys.length||!Array.isArray(receipt.validatorKeys)||receipt.validatorKeys.length!==c.validator.keys.length||receipt.validator.localKeys!==c.validator.keys.length)throw Error('Restore signer inventory mismatch');
 const publicKeys=new Map(),seenPublicKeys=new Set();
 if(signerIdentities!==undefined){
  if(!Array.isArray(signerIdentities)||signerIdentities.length!==c.validator.keys.length)throw Error('Independent signer identities required');
  for(const item of signerIdentities){
   if(!item||Object.keys(item).length!==2||typeof item.keyEnv!=='string'||!item.keyEnv||!/^0x[0-9a-f]{96}$/.test(item.publicKey)||publicKeys.has(item.keyEnv)||seenPublicKeys.has(item.publicKey))throw Error('Duplicate or malformed independent signer identity');
   publicKeys.set(item.keyEnv,item.publicKey);seenPublicKeys.add(item.publicKey);
  }
 }
 const identity=(item,configuration=false)=>{
  if(item.index===null){
   const publicKey=configuration?publicKeys.get(item.keyEnv):item.publicKey;
   if(!/^0x[0-9a-f]{96}$/.test(publicKey))throw Error('Automatic signer requires independent public key');
   return 'publicKey:'+publicKey;
  }
  if(!/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(item.index)||(!configuration&&item.publicKey!==undefined))throw Error('Duplicate or unsupported signer commitment');
  return 'index:'+item.index;
 };
 const pins=new Map();
 for(const item of receipt.validatorKeys){
  const id=identity(item);
  if(pins.has(id))throw Error('Duplicate or unsupported signer commitment');
  pins.set(id,token(item.restart));
 }
 const identities=new Set(),keyEnvs=new Set();
 for(const key of c.validator.keys){
  const id=identity(key,true);
  if(!pins.has(id)||identities.has(id)||!key.recoveryWitness||!key.journal||(signerIdentities!==undefined&&(!publicKeys.has(key.keyEnv)||keyEnvs.has(key.keyEnv))))throw Error('Restore signer identity/witness mismatch');
  identities.add(id);keyEnvs.add(key.keyEnv);key.restart=pins.get(id);
 }
 if(!c.recoveryWitness)throw Error('Restore chain witness missing');
 c.recoveryCommitment=token(receipt.restart);c.initial=null;c.restart=null;c.automaticRecovery=false;
 return result;
}
export function restoreAuditRequest(original,receipt) {
 const finalized=receipt?.finalized;
 if(!/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(finalized?.epoch)||!/^0x[0-9a-f]{64}$/.test(finalized?.root))throw Error('Independent stopped finalized checkpoint required');
 const c=original.capella;if(!c?.initial?.file||!c.initial.sha256||!c.profile?.sha256)throw Error('Independent profile/anchor required');
 return {profile:{file:c.profile.file,sha256:c.profile.sha256},anchor:structuredClone(c.initial),limits:structuredClone(c.limits),expectedCheckpoint:token(receipt.restart),requiredFinalized:structuredClone(finalized),maximumBlocks:'0x400',maximumArchiveBytes:'0x20000000'};
}
export function validateRestoredFinality(receipt,result,terminal) {
 const decimal=value=>typeof value==='string'&&/^(?:0|[1-9][0-9]*)$/.test(value);
 const checkpoint=value=>value&&typeof value.epoch==='string'&&/^(?:0|[1-9][0-9]*|0x(?:0|[1-9a-f][0-9a-f]*))$/.test(value.epoch)&&/^0x[0-9a-f]{64}$/.test(value.root);
 if(!decimal(result?.firstEpoch)||!decimal(result?.lastEpoch)||!checkpoint(result.lastCheckpoint)||BigInt(result.lastCheckpoint.epoch)!==BigInt(result.lastEpoch)||!checkpoint(receipt?.finalized)||!checkpoint(terminal?.finalized))throw Error('Missing or malformed restored finality evidence');
 if(result.epochs!==3||BigInt(result.firstEpoch)<BigInt(receipt.finalized.epoch)||BigInt(result.lastEpoch)<BigInt(receipt.finalized.epoch)+3n)throw Error('Restored common finality did not advance three epochs');
 const validator=terminal.validator;
 if(validator?.healthy!==true||validator.failure!==null||terminal.clockHealthy!==true||!Number.isSafeInteger(validator.attestationsAdmitted)||validator.attestationsAdmitted<=0||!Number.isSafeInteger(validator.proposalsAdmitted)||validator.proposalsAdmitted<=0)throw Error('Restored signer did not resume healthy attestations and proposals');
 if(BigInt(terminal.finalized.epoch)<BigInt(result.lastEpoch))throw Error('Restored terminal finality behind observed window');
 // This check alone does not prove ancestry at a later terminal epoch.
 // Same-epoch disagreement must never pass on the epoch number alone.
 if(BigInt(terminal.finalized.epoch)===BigInt(result.lastEpoch)&&terminal.finalized.root!==result.lastCheckpoint.root)throw Error('Restored terminal finality conflicts with observed checkpoint');
}
