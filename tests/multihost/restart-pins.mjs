// A replacement never restores old journals; advance independent commitments
// only after an independently captured healthy terminal receipt.
import {pinnedRestoreConfiguration,stoppedReceipt} from './restore-config.mjs';
export function advanceRestartConfiguration(original,priorText,nextText){
 const prior=stoppedReceipt(priorText),next=stoppedReceipt(nextText);
 // Also validate inventories/pins and preserve operational paths by identity.
 const before=pinnedRestoreConfiguration(original,prior);
 const after=pinnedRestoreConfiguration(original,next);
 const compare=(a,b,strict=false)=>{
  const old=BigInt(a.sequence),fresh=BigInt(b.sequence);
  if(fresh<old||(strict&&fresh===old)||(fresh===old&&a.digest!==b.digest))throw Error('Restart commitment regressed or changed at the same sequence');
 };
 compare(before.capella.recoveryCommitment,after.capella.recoveryCommitment,true);
 const old=new Map(before.capella.validator.keys.map(k=>[k.index,k.restart]));
 let advanced=0;
 for(const key of after.capella.validator.keys){const previous=old.get(key.index);if(!previous)throw Error('Restart signer identity changed');compare(previous,key.restart);if(BigInt(key.restart.sequence)>BigInt(previous.sequence))++advanced;}
 if(!advanced)throw Error('No independent signer commitment advanced');
 if(BigInt(next.finalized.epoch)<BigInt(prior.finalized.epoch)||(next.finalized.epoch===prior.finalized.epoch&&next.finalized.root!==prior.finalized.root))throw Error('Restart finalized checkpoint regressed or conflicted');
 return {configuration:after,advancedSignerPins:advanced,prior:prior.restart,current:next.restart};
}
