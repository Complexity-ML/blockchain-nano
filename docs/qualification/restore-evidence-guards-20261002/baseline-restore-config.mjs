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
export function pinnedRestoreConfiguration(node, receipt) {
 const result=structuredClone(node),c=result.capella;
 if(!c||node.bellatrix||!Array.isArray(c.validator?.keys)||!c.validator.keys.length||!Array.isArray(receipt.validatorKeys)||receipt.validatorKeys.length!==c.validator.keys.length||receipt.validator.localKeys!==c.validator.keys.length)throw Error('Restore signer inventory mismatch');
 const pins=new Map();
 for(const item of receipt.validatorKeys){
  if(!/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(item.index)||pins.has(item.index))throw Error('Duplicate or unsupported signer commitment');
  pins.set(item.index,token(item.restart));
 }
 const indices=new Set();
 for(const key of c.validator.keys){
  if(!pins.has(key.index)||indices.has(key.index)||!key.recoveryWitness||!key.journal)throw Error('Restore signer identity/witness mismatch');
  indices.add(key.index);key.restart=pins.get(key.index);
 }
 if(!c.recoveryWitness)throw Error('Restore chain witness missing');
 c.recoveryCommitment=token(receipt.restart);c.initial=null;c.restart=null;c.automaticRecovery=false;
 return result;
}
export function restoreAuditRequest(original,receipt) {
 const c=original.capella;if(!c?.initial?.file||!c.initial.sha256||!c.profile?.sha256)throw Error('Independent profile/anchor required');
 return {profile:{file:c.profile.file,sha256:c.profile.sha256},anchor:structuredClone(c.initial),limits:structuredClone(c.limits),expectedCheckpoint:token(receipt.restart),maximumBlocks:'0x400',maximumArchiveBytes:'0x20000000'};
}
export function validateRestoredFinality(receipt,result,terminal) {
 if(result.epochs!==3||BigInt(result.firstEpoch)<BigInt(receipt.finalized.epoch)||BigInt(result.lastEpoch)<BigInt(receipt.finalized.epoch)+3n)throw Error('Restored common finality did not advance three epochs');
 if(terminal.validator?.healthy!==true||terminal.validator.failure!==null||terminal.clockHealthy!==true||terminal.validator.attestationsAdmitted<=0||(!Number.isSafeInteger(terminal.validator.proposalsAdmitted)||terminal.validator.proposalsAdmitted<=0))throw Error('Restored signer did not resume healthy attestations and proposals');
 if(BigInt(terminal.finalized.epoch)<BigInt(result.lastEpoch))throw Error('Restored terminal finality behind observed window');
}
