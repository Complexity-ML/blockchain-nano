// Pure boundary checks only: no files, processes, keys, RPC or network.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {validateLifecycleTerminalProjection} from './lifecycle-runtime.mjs';
import {requireExactFullWithdrawal} from './lifecycle.mjs';
const q=n=>'0x'+BigInt(n).toString(16),root=n=>'0x'+String(n).padStart(64,'0'),pub=n=>'0x'+String(n).padStart(96,'0');
const maximumU64=(1n<<64n)-1n,maximumU256=(1n<<256n)-1n;
function terminal(){return {format:'nano-fresh-lifecycle-audit-result-v1',status:'PASS_SCOPED_FRESH_LIFECYCLE',passed:true,stage:'terminal',owner:'vps-docker',campaignId:'a'.repeat(64),headRoot:root(1),headSlot:'0xa80',requiredFinalized:{epoch:'0x14f',root:root(2)},capturedLocalDutyIdentityVerified:true,canonicalWithdrawalIndicesStrictlyIncreasing:true,exactWithdrawalAccountingVerified:true,completePrimaryWitnessClosuresVerified:true,externalCoreTokensMatched:true,requiredFinalizedAncestorVerified:true,canonicalSignedStateReplayVerified:true,physicalCopiesUnchanged:true,canonicalDepositTransactions:1,canonicalDepositOperations:1,registrationTransitions:1,canonicalActionInclusions:2,acceptedJobArtifactsBound:2,restoredCapturePairsBound:2,finalizedRestartProposals:4,fullPayouts:2,partialPayouts:0,postSecondRestartDuplicateFullPayouts:0,replayedSignedBlocks:2600,liveRecordPeak:33,liveStateSszPeakBytes:1000000,prunes:300,canonicalExecutionIndexChecks:2600,payoutExecutionReplays:2,prunedExecutionStatesReplayed:2000,reportBytes:2000,fullPayoutGwei:'0x123',reportSha256:'f'.repeat(64),historicalForkChoiceVerified:false,restorationProcessExecutionInferred:false,newSignaturesCreated:0,envRead:false,runtimeStarted:false,productionQualified:false,rawCapsuleExported:false};}
test('full payout total uses exact aggregate bounds and a coherent safe counter',()=>{
 const context={campaignId:'a'.repeat(64),requiredFinalized:terminal().requiredFinalized};
 for(const [count,total] of [[2,2n],[2,2n*maximumU64],[3,3n*maximumU64]]){
  const value={...terminal(),fullPayouts:count,fullPayoutGwei:q(total)},before=structuredClone(value),result=validateLifecycleTerminalProjection(value,context);
  assert.equal(result.fullPayoutGwei,q(total));assert.notStrictEqual(result,value);assert.deepEqual(value,before);
 }
 const invalid=[
  x=>x.fullPayoutGwei='0x0',
  x=>x.fullPayoutGwei='0x1',
  x=>x.fullPayoutGwei=q(2n*maximumU64+1n),
  x=>{x.fullPayouts=3;x.fullPayoutGwei='0x2';},
  x=>x.fullPayouts=Number.MAX_SAFE_INTEGER+1,
  x=>x.fullPayoutGwei='0x01',
  x=>x.fullPayoutGwei=0,
  x=>x.headSlot=q(1n<<64n)
 ];
 for(const mutate of invalid){const value={...terminal(),fullPayoutGwei:q(2n*maximumU64)};mutate(value);const before=structuredClone(value);assert.throws(()=>validateLifecycleTerminalProjection(value,context));assert.deepEqual(value,before);}
 assert.equal(invalid.length,8);
});
test('individual withdrawals remain uint64 while exact recipient credits remain uint256',()=>{
 const recipient='0x'+'1'.repeat(40),binding={publicKey:pub(64),resolvedIndex:'0x40',withdrawalRecipient:recipient};
 const withdrawal=amount=>({publicKey:binding.publicKey,resolvedIndex:binding.resolvedIndex,withdrawalIndex:'0x1',epoch:'0xa',withdrawableEpoch:'0xa',validatorBeforeGwei:q(amount),validatorAfterGwei:'0x0',selectedWithdrawalGwei:q(amount),recipient,totalRecipientWithdrawalWei:q(amount*1000000000n),recipientBeforeWithdrawalWei:'0x0',recipientAfterWithdrawalWei:q(amount*1000000000n),canonicalInclusionRoot:root(7),partial:false});
 const value=withdrawal(maximumU64),before=structuredClone(value);assert.equal(requireExactFullWithdrawal(value,binding).selectedWithdrawalGwei,q(maximumU64));assert.deepEqual(value,before);
 const atWeiLimit={...value,recipientBeforeWithdrawalWei:q(maximumU256-maximumU64*1000000000n),recipientAfterWithdrawalWei:q(maximumU256)};assert.equal(requireExactFullWithdrawal(atWeiLimit,binding).recipientAfterWithdrawalWei,q(maximumU256));
 assert.throws(()=>requireExactFullWithdrawal(withdrawal(maximumU64+1n),binding));
 const overflow={...atWeiLimit,recipientBeforeWithdrawalWei:q(maximumU256+1n-maximumU64*1000000000n),recipientAfterWithdrawalWei:q(maximumU256+1n)};assert.throws(()=>requireExactFullWithdrawal(overflow,binding));
});
