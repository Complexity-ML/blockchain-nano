import assert from 'node:assert/strict';
// A startup refusal is distinct from a healthy validator stop or an audited kill.
// This narrowly admits V29's single publication before the catalog guard failed.
export function verifyRestoredSourceStop(state,{expectedExit=0,crashReadonlyAudited=false,startupRefusalAudit,runtimeRefusalAudit}={}){
 assert.ok([0,1,137].includes(expectedExit),'Unsupported stopped-source exit');
 assert.equal(state.Status,'exited');assert.equal(state.Running,false);assert.equal(state.ExitCode,expectedExit);assert.equal(state.OOMKilled,false);
 if(expectedExit===137)assert.equal(crashReadonlyAudited,true,'Crash source must be independently audited');
 if(expectedExit===1&&runtimeRefusalAudit){
  assert.equal(startupRefusalAudit,undefined,'Startup and runtime refusal audits are distinct');
  const a=runtimeRefusalAudit;assert.equal(a.format,'nano-runtime-refusal-copy-audit-v1');
  for(const field of ['sourceObservedExited1NoOom','sourcePassedRuntimeReady','sourceFilesHashesUnchanged','independentCopyVerified','sourcePrimaryWitnessMismatchObserved','witnessedIntentRecoveryVerified','recoveryPerformedOnCopyOnly','canonicalReplayVerified','signerAndPeerFilesUnchanged','independentSignerWitnessPairsVerified','preCampaignExternalMinimumsVerified'])assert.equal(a[field],true,'Missing runtime refusal audit: '+field);
  for(const field of ['terminalExternalStopReceiptAvailable','exactTerminalExternalPinsVerified','recoveryRuntimeDutiesResumed','networkQualified','productionQualified'])assert.equal(a[field],false,'Runtime refusal scope mismatch: '+field);
  for(const field of ['sourceManifestSha256','externalCurrentCaptureSha256'])assert.match(a[field],/^[0-9a-f]{64}$/);
  for(const t of [a.sourcePrimary,a.witnessIntentNext,a.recoveredPrimary]){assert.ok(t&&typeof t==='object');assert.match(t.sequence,/^0x[1-9a-f][0-9a-f]*$/);assert.ok(BigInt(t.sequence)<=0xffffffffffffffffn);assert.match(t.digest,/^0x[0-9a-f]{64}$/);}
  assert.equal(BigInt(a.witnessIntentNext.sequence),BigInt(a.sourcePrimary.sequence)+1n,'Recovery must complete exactly the witnessed pending intent');assert.deepEqual(a.recoveredPrimary,a.witnessIntentNext);
 }else if(expectedExit===1){
  const a=startupRefusalAudit;assert.ok(a&&typeof a==='object');
  for(const field of ['sourceObservedExited1NoOom','auditHashesUnchanged','signerPinsUnchanged','independentWitnessVerified','canonicalReplayUnchangedExceptCheckpointToken','readOnly'])assert.equal(a[field],true,'Missing startup refusal audit: '+field);
  assert.equal(a.dutiesResumed,false);assert.equal(a.networkQualified,false);assert.equal(a.migrationCheckpointSequenceIncrement,1);
  for(const token of [a.oldCheckpoint,a.currentCheckpoint]){assert.ok(token&&typeof token==='object');assert.match(token.sequence,/^0x[1-9a-f][0-9a-f]*$/);assert.ok(BigInt(token.sequence)<=0xffffffffffffffffn);assert.match(token.digest,/^0x[0-9a-f]{64}$/);}
  assert.equal(BigInt(a.currentCheckpoint.sequence),BigInt(a.oldCheckpoint.sequence)+1n);
 }
 return true;
}
