import {PartitionRecoveryDeadline} from './partition.mjs';

// Continue bounded read-only observation only after a restored partition misses
// its liveness deadline. Never turn the original failure into a successful run.
export async function observeRecovery({partition,observe,record}) {
 let late=null;
 try{await partition();}
 catch(error){
  if(!(error instanceof PartitionRecoveryDeadline))throw error;
  late=error;await record({event:'partition-recovery-deadline-missed',message:error.message,...(error.recoveryObservation?{recoveryObservation:error.recoveryObservation}:{})});
 }
 try{
  if(late){
   if(typeof late.waitForRecovery!=='function')throw Error('Late recovery observer unavailable');
   await late.waitForRecovery();
  }
  await observe();
 }
 catch(error){
  if(late)await record({event:'late-finality-failed',message:error.message});
  throw error;
 }
 if(late){await record({event:'late-finality-passed',deadlineStillFailed:true});throw late;}
}
