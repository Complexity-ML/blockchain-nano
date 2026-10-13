#!/usr/bin/env python3
"""Audit only PUBLIC CTest logs/JUnit. Never open a DB, config, key or ledger; never reuse ARM results."""
import argparse, hashlib, json, pathlib, re, xml.etree.ElementTree as ET
NEW = ['storage_capella_blob_gc_next_publication', 'storage_capella_blob_gc_adversarial_boundaries', 'storage_capella_blob_gc_natural_pruning', 'storage_capella_blob_gc_cli_closed_fixture']
AVAILABILITY = 'consensus_capella_duty_projection_availability'
REGRESSION = ['storage_checkpoint_immutable_blobs', 'consensus_capella_fragment_checkpoint_migration']
BOUNDARIES = {'primary-missing-live-ref','primary-corrupt-live-ref','witness-missing-live-ref','witness-corrupt-live-ref','pending-sequence-divergence','primary-neither-previous-nor-next','wrong-independent-witness-pin','wrong-independent-pending-next-pin','request-hash-before-parse','signing-store-role-forbidden','future-schema-refused','readers-not-stopped','complete-scan-budget','total-delete-budget','single-batch-budget','large-finite-duration-no-timepoint-overflow','inventory-cancellation','existing-mode-never-creates-missing-db','existing-mode-never-initializes-missing-schema','hardlinked-original-copy','symlinked-copy-file','stale-plan-directory-replaced','unknown-future-blob-namespace','successful-gc-preserves-nonprefix-values','pending-missing-next-only-witness-ref','pending-corrupt-existing-next-primary-ref','pending-union-preserved-no-gc-repair-then-normal-recovery','sigkill-after-primary-before-witness-new-plan-reopen','primary-sync-failure-close-new-plan','witness-sync-failure-close-new-plan'}
def require(condition, why):
    if not condition: raise ValueError(why)
def pinned(path, expected, maximum):
    require(re.fullmatch('[0-9a-f]{64}', expected) is not None, 'canonical external SHA required')
    with open(path, 'rb') as f: data = f.read(maximum + 1)
    require(len(data) <= maximum, 'public evidence exceeds bound')
    require(hashlib.sha256(data).hexdigest() == expected, 'public evidence pin mismatch')
    return data
def one(rows, name):
    found = [r for r in rows if r.get('scenario') == name]
    require(len(found) == 1, 'missing/duplicate scenario ' + name)
    return found[0]
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--junit', required=True); p.add_argument('--junit-sha', required=True)
    p.add_argument('--log', required=True); p.add_argument('--log-sha', required=True)
    p.add_argument('--terminal', required=True); p.add_argument('--terminal-sha', required=True)
    for name in ['before','build-terminal']:
        p.add_argument('--'+name,required=True); p.add_argument('--'+name+'-sha',required=True)
    a = p.parse_args()
    terminal = json.loads(pinned(a.terminal, a.terminal_sha, 65536))
    require(terminal.get('format')=='nano-gc-amd64-seven-selected-execution-terminal-v1' and type(terminal.get('exitCode')) is int and terminal['exitCode']==0 and terminal.get('processCompleted') is True and terminal.get('containerAbsentAfterCleanup') is True and terminal.get('launched') is True and terminal.get('timedOut') is False and terminal.get('bindingInvariantsHeld') is True and terminal.get('failure') is None and terminal.get('productionQualified') is False,'actual AMD terminal/cleanup/invariants not complete0')
    before=json.loads(pinned(a.before,a.before_sha,16*1024*1024)); build=json.loads(pinned(a.build_terminal,a.build_terminal_sha,8*1024*1024))
    require(before.get('buildTerminalSha256')==a.build_terminal_sha and re.fullmatch('[0-9a-f]{64}',before.get('publicInputsSha256','')) is not None and before.get('publicInputsSha256')==build.get('publicInputsSha256') and before.get('originalPublicInputsSha256')=='28cd003fdc76f4af8a6c831ae0c82d2f7e0f01a39a3e1c09af787f921f93e323' and before.get('gitCommit')=='f9d078d0d45d6081c8c8745a80575d898335ec3e','actual build/source provenance missing')
    require(build.get('format')=='nano-gc-amd64-incremental-build-terminal-v1' and build.get('status')=='BUILD_PASS_TESTS_NOT_EXECUTED' and build.get('coreFresh')==41 and build.get('coreReused')==99 and build.get('archiveMembers')==140 and build.get('freshConsumers')==8 and build.get('archiveRawBytesComparedTo140Objects') is True and build.get('testsExecuted')==0,'specific new AMD build not qualified')
    require(before['artifacts']['libnano_core.a']['sha256']==build['archive']['sha256'] and before.get('buildControllerSha256')==build.get('controllerSha256'),'actual archive/controller provenance differs')
    programs={r['target']:r for r in build['programs']}; require(len(build['programs'])==8 and set(programs)=={'nano','nano-capella-transition-tests','nano-storage-maintenance','nano-checkpoint-blob-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit','nano-capella-duty-availability-tests'},'exact eight actual AMD programs required')
    for name,row in programs.items(): require(before['artifacts'][name]['sha256']==row['binary']['sha256'] and row['coreArchiveSha256']==build['archive']['sha256'],'actual program/core pin mismatch '+name)
    require(before['artifacts']['libnano-test-sync-fault.so']['sha256']=='1c5fbe9a94c9e4137f0ae41999390a9a9690e63324998bfeff6368609345d86e','actual AMD fault shim absent')
    tree = ET.fromstring(pinned(a.junit, a.junit_sha, 8*1024*1024))
    cases = list(tree.iter('testcase')); expected = NEW + REGRESSION + [AVAILABILITY]
    require(len(cases) == len(expected) and sorted(c.get('name') for c in cases) == sorted(expected), 'exact seven AMD registered tests required')
    for c in cases:
        require(c.get('status') == 'run' and not any(list(c.iter(k)) for k in ['failure','error','skipped']), 'test not run or failed: ' + str(c.get('name')))
    rows = []
    for line in pinned(a.log, a.log_sha, 32*1024*1024).decode('utf-8', 'strict').splitlines():
        line = re.sub(r'^\s*\d+:\s*', '', line)
        if line.startswith('{'):
            try: value = json.loads(line)
            except json.JSONDecodeError: continue
            if isinstance(value, dict): rows.append(value)
    require(not any(row.get('result')=='FAIL' for row in rows),'public log contains a failed semantic scenario')
    adverse = one(rows, 'checkpoint-gc-adversarial-boundaries')
    ids = adverse.get('passedBoundaries', [])
    require(isinstance(ids, list) and all(isinstance(v,str) for v in ids) and len(ids) == len(set(ids)) and set(ids) == BOUNDARIES, 'exact thirty named adversarial boundaries required')
    for ident in ['primary-sync-failure-close-new-plan','witness-sync-failure-close-new-plan','sigkill-after-primary-before-witness-new-plan-reopen','large-finite-duration-no-timepoint-overflow','inventory-cancellation']:
        require(ident in ids, 'required branch not executed: ' + ident)
    require(adverse.get('allPreDeletionRefusalsLogicalContentsUnchanged') is True and adverse.get('physicalPowerLossQualified') is False, 'refusal/powerloss scope invalid')
    natural = one(rows, 'gc-natural-cold-pruning-reopen-next-publication')
    require(natural.get('injectedBlobCount') == 0 and natural.get('naturalPublicationPruningOrphansRequired') is True and natural.get('followingPublicationColdReopened') is True, 'natural path incomplete or staged')
    require(natural['collection']['noWork'] is False and len(natural['collection']['stores'])==2 and all(type(s.get('deletedRecords')) is int and s['deletedRecords']>0 for s in natural['collection']['stores']) and all(natural['collection'].get(k) is True for k in ['allProtectedReferencesReopened','nonGcNamespacesUnchanged','originalSourceInventoriesUnchanged']), 'natural GC deletion/protected closures incomplete')
    cli = one(rows, 'checkpoint-gc-cli-closed-staged')
    require(type(cli.get('cliExitCode')) is int and cli.get('cliExitCode') == 0 and cli.get('actualCliCompleted') is True and cli.get('noWork') is False and cli.get('deletedPrimaryRecords') == 1 and cli.get('deletedWitnessRecords') == 1, 'actual CLI deletion not demonstrated')
    require(cli.get('privateFixtureExported') is False and cli.get('naturalPruningReplay') is False and cli.get('receiptMode') == 0o600 and cli.get('receiptLinks') == 1, 'CLI receipt/privacy scope invalid')
    receipt = cli['receiptBytes'].encode('utf-8'); require(hashlib.sha256(receipt).hexdigest() == cli['receiptSha256'], 'exact CLI receipt bytes mismatch')
    report = json.loads(receipt)
    require(report.get('format') == 'nano-owned-checkpoint-gc-maintenance-receipt-v1' and report.get('requiresSuccessfulTerminalExit') is True, 'wrong CLI receipt format/scope')
    require(report['result']['noWork'] is False and [s['deletedRecords'] for s in report['result']['stores']]==[1,1] and all(report['result'].get(k) is True for k in ['allProtectedReferencesReopened','nonGcNamespacesUnchanged','originalSourceInventoriesUnchanged']), 'receipt deletion/protected closure mismatch')
    refusal_rows = cli.get('refusals',[])
    require(isinstance(refusal_rows,list) and len(refusal_rows) == 5, 'exact five CLI refusal rows required')
    refusals = {r['id']:r for r in refusal_rows}
    require(len(refusals) == len(refusal_rows), 'duplicate CLI refusal ids')
    require(set(refusals) == {'missing-receipt-argument','request-pin-mismatch','profile-pin-mismatch','preexisting-receipt-no-overwrite','physical-statistics-entry-budget'}, 'CLI refusal cases missing/duplicate')
    for ident, r in refusals.items():
        require(type(r.get('exitCode')) is int and r.get('exitCode') == 1 and r.get('newReceiptPublished') is False and r.get('logicalCopyContentsUnchanged') is True, 'CLI refusal incomplete: ' + ident)
    require(refusals['preexisting-receipt-no-overwrite'].get('existingReceiptBytesPreserved') is True, 'existing receipt overwritten')
    follow = [r for r in rows if r.get('scenario') == 'gc-cold-reopen-and-next-admission']
    require(sorted(r.get('collectorInterface') for r in follow) == ['actual-cli','cpp-api'], 'both collector interfaces/reopens required')
    require(all(r.get('nextChildNormallyAdmitted') is True and r.get('nextPublicationColdReopened') is True and r.get('sameCheckpointColdReopened') is True and r.get('originalAndSignerWitnessBytesUnchanged') is True and r.get('deletedPrimaryRecords')==1 and r.get('deletedWitnessRecords')==1 and r.get('localSigningProviderCalls')==0 for r in follow), 'post-GC deletion/cold-reopen/next-publication missing')
    for r in [adverse, natural, cli] + follow:
        require(r.get('productionQualified') is False, 'test falsely promotes production')
    require(adverse.get('syncFailureMayLeaveDurablePartialDeletion') is True and adverse.get('sigkillPartialProgressExplicit') is True,'crash/fsync scope missing')
    kill=[r for r in rows if r.get('scenario')=='gc-crash-child-phase' and r.get('phase')=='wal-change-logical-check' and r.get('primaryOrphanAbsent') is True and r.get('witnessOrphanPresent') is True]
    require(len(kill)==1,'actual primary-deleted/witness-retained crash boundary not observed exactly once')
    intermediate=one(rows,'gc-natural-collection-intermediate')
    require(type(intermediate.get('gcSlot')) is int and intermediate['gcSlot']>0 and intermediate.get('anchorMovesBeforeGc',0)>=2 and intermediate.get('postGcOrdinaryBlocksRequired')==16 and intermediate.get('followingPublicationColdReopened') is False and intermediate.get('terminalQualification') is False and intermediate.get('productionQualified') is False,'natural intermediate scope invalid')
    require(intermediate.get('collection')==natural['collection'] and intermediate.get('injectedBlobCount')==0,'natural final collection differs from actual intermediate')
    progress=[r for r in rows if type(r.get('progressSlot')) is int]
    require(progress and max(r['progressSlot'] for r in progress)==56 and 56-intermediate['gcSlot']>=16,'sixteen real post-GC ordinary admitted slots required')
    require(natural.get('sameCheckpointColdReopened') is True and natural.get('nextBlocksNormallyProducedAndAdmitted') is True and natural.get('protectedSignerWitnessBytesUnchangedDuringGc') is True,'natural cold-reopen/source/key assertions incomplete')
    availability=[one(rows,n) for n in ['capella-local-sync-postready63','capella-real-reactor-local-sync-postready63','capella-real-reactor-duty-projection-head-recovery']]
    for r in availability:
        require(r.get('result')=='PASS' and r.get('syntheticFixture') is True and r.get('runtimeStarted') is False and r.get('networkQualified') is False and r.get('productionQualified') is False,'availability scenario/scope invalid')
        require(type(r.get('elapsedMs')) is int and 0<=r['elapsedMs']<60000 and type(r.get('maxRssKiB')) is int and 0<=r['maxRssKiB']<=131072 and type(r.get('syntheticFixtureSignaturesCreated')) is int and 0<=r['syntheticFixtureSignaturesCreated']<=160,'availability sixty-second/128MiB/160-signature budgets')
    pure,real,peer=availability
    require(all(pure.get(k) is True for k in ['strictSyncBoundaryStillRejects','optionalLocalSyncDeferred','proposalAttestationContextsRemainReady','callbackAndResourceErrorsVisible']),'pure63 frontier incomplete')
    require(all(real.get(k) is True for k in ['realCoreAndLoopHealthy','proposalAttestationAssignmentsRemainAvailable','syncMembersDeferred','syncCurrentDeferredBeforeProvider','callbackResourceErrorsRemainVisible','checkpointAndJournalUnchanged','localKeyPendingRegistration']) and real.get('providerCalls')==0,'real63 deferral/provider boundary incomplete')
    require(all(peer.get(k) is True for k in ['oldHeadProjectionReallyUnavailable','oldHeadDeferredLoopHealthy','oldHeadCheckpointPreserved','newParentPreparationAvailable','loopHealthyAfterPeerAdmission','pendingJournalCommitmentUnchanged','localKeyPendingRegistration','clockMonotoneUnchangedAfterInitialTick','storageOnlyColdMigrationCanonicalFieldsUnchanged','independentWitnessMigrationBoundToPrimaryTokens','legacyFlatRssRefusalStillOpen']) and peer.get('normallyAdmittedPeerBlocks')==48 and peer.get('localProviderCalls')==0 and peer.get('coldEnvelopeCacheRecords')==2 and peer.get('coldEnvelopeCacheEncodedBytes')==1048576 and peer.get('coldDecodedCacheUsageDirectlyObserved') is False,'real peer-parent preparation/cold configuration scope incomplete')
    require(peer.get('targetSlot')==384 and type(peer.get('newHeadSlot')) is int and 0<peer['newHeadSlot']<=384 and re.fullmatch('0x[0-9a-f]{64}',peer.get('genesisValidatorsRoot','')),'bounded synthetic peer parent/domain missing')
    print(json.dumps({'format':'nano-gc-amd64-seven-public-result-audit-v1','passedWithinTestScope':True,'selectedRegisteredTests':expected,'actualSevenRunNoSkipRequired':True,'provenance':{name:{'path':getattr(a,name.replace('-','_')),'sha256':getattr(a,name.replace('-','_')+'_sha')} for name in ['junit','log','terminal','before','build-terminal']},'actualPublicInputsSha256':before['publicInputsSha256'],'actualNewAmdArchiveSha256':build['archive']['sha256'],'actualNewAvailabilityElfSha256':programs['nano-capella-duty-availability-tests']['binary']['sha256'],'adversarialBranchIdsObserved':ids,'actualFsyncBranchesObserved':2,'actualBetweenStoreLogicalBoundaryObserved':True,'actualNaturalGcSlot':intermediate['gcSlot'],'actualPostGcOrdinaryBlocks':56-intermediate['gcSlot'],'actualCliReceiptSha256':cli['receiptSha256'],'actualCliReceiptBytes':cli['receiptBytes'],'availabilityInternalScenarioCount':3,'availabilityNotAddedToRegisteredTestCount':True,'availabilityProviderCalls':0,'availabilityPreparationQualifiedNotLocalSignedDuties':True,'priorArmResultsReused':False,'skippedTestsCounted':0,'physicalPowerLossQualified':False,'privateInputsRead':False,'runtimeDutiesResumed':False,'productionQualified':False},indent=2))
if __name__ == '__main__': main()
