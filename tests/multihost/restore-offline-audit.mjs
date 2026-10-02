import {restoreAuditRequest,validateRestoredFinality,pinnedRestoreConfiguration} from './restore-config.mjs';
// Observation and stop commitments arrive separately; never derive either
// checkpoint from the restored database or from the audit's response.
export function restoredFinalityAuditRequest(original,prior,stable,terminal){
 validateRestoredFinality(prior,stable,terminal);
 pinnedRestoreConfiguration(original,terminal);
 return {...restoreAuditRequest(original,terminal),requiredFinalized:{epoch:'0x'+BigInt(stable.lastCheckpoint.epoch).toString(16),root:stable.lastCheckpoint.root}};
}
export function verifyRestoredFinalityAudit(request,terminal,audit){
 if(audit?.canonicalHistoryVerified!==true||audit.externalFinalizedAncestorVerified!==true)throw Error('Native restored finalized ancestry not verified');
 if(!Number.isSafeInteger(audit.checkpointSequence)||BigInt(audit.checkpointSequence)!==BigInt(request.expectedCheckpoint.sequence)||audit.checkpointDigest!==request.expectedCheckpoint.digest)throw Error('Native audit stopped commitment mismatch');
 if(audit.requiredFinalized?.epoch!==request.requiredFinalized.epoch||audit.requiredFinalized.root!==request.requiredFinalized.root)throw Error('Native audit observed checkpoint mismatch');
 if(audit.finalized?.epoch!==terminal.finalized.epoch||audit.finalized.root!==terminal.finalized.root)throw Error('Native audit terminal checkpoint mismatch');
 return audit;
}
