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
    for name in ['before','build-terminal','prior-manifest','fixture-manifest']:
        p.add_argument('--'+name,required=True); p.add_argument('--'+name+'-sha',required=True)
    p.add_argument('--prior-run',required=True)
    a = p.parse_args()
    terminal = json.loads(pinned(a.terminal, a.terminal_sha, 65536))
    require(terminal.get('format')=='nano-gc-amd64-five-retry-execution-terminal-v1' and type(terminal.get('exitCode')) is int and terminal['exitCode']==0 and terminal.get('processCompleted') is True and terminal.get('containerAbsentAfterCleanup') is True and terminal.get('launched') is True and terminal.get('timedOut') is False and terminal.get('bindingInvariantsHeld') is True and terminal.get('failure') is None and terminal.get('productionQualified') is False,'actual AMD terminal/cleanup/invariants not complete0')
    before=json.loads(pinned(a.before,a.before_sha,16*1024*1024)); build=json.loads(pinned(a.build_terminal,a.build_terminal_sha,8*1024*1024))
    require(before.get('buildTerminalSha256')==a.build_terminal_sha and re.fullmatch('[0-9a-f]{64}',before.get('publicInputsSha256','')) is not None and before.get('publicInputsSha256')==build.get('publicInputsSha256') and before.get('originalPublicInputsSha256')=='28cd003fdc76f4af8a6c831ae0c82d2f7e0f01a39a3e1c09af787f921f93e323' and before.get('gitCommit')=='f9d078d0d45d6081c8c8745a80575d898335ec3e','actual build/source provenance missing')
    require(build.get('format')=='nano-gc-amd64-incremental-build-terminal-v1' and build.get('status')=='BUILD_PASS_TESTS_NOT_EXECUTED' and build.get('coreFresh')==41 and build.get('coreReused')==99 and build.get('archiveMembers')==140 and build.get('freshConsumers')==8 and build.get('archiveRawBytesComparedTo140Objects') is True and build.get('testsExecuted')==0,'specific new AMD build not qualified')
    require(before['artifacts']['libnano_core.a']['sha256']==build['archive']['sha256'] and before.get('buildControllerSha256')==build.get('controllerSha256'),'actual archive/controller provenance differs')
    programs={r['target']:r for r in build['programs']}; require(len(build['programs'])==8 and set(programs)=={'nano','nano-capella-transition-tests','nano-storage-maintenance','nano-checkpoint-blob-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit','nano-capella-duty-availability-tests'},'exact eight actual AMD programs required')
    for name,row in programs.items(): require(before['artifacts'][name]['sha256']==row['binary']['sha256'] and row['coreArchiveSha256']==build['archive']['sha256'],'actual program/core pin mismatch '+name)
    require(before['artifacts']['libnano-test-sync-fault.so']['sha256']=='1c5fbe9a94c9e4137f0ae41999390a9a9690e63324998bfeff6368609345d86e','actual AMD fault shim absent')
    prior_manifest=json.loads(pinned(a.prior_manifest,a.prior_manifest_sha,65536)); fixture_manifest=json.loads(pinned(a.fixture_manifest,a.fixture_manifest_sha,65536))
    require(a.prior_manifest_sha=='3221c362c901e32a569727ea5febb15ee4d1a5f6124b6e40e5c5a63c1c132776' and a.fixture_manifest_sha=='ae9bb195e095cd5224d39782b2b88bae936c07f36ec51cb11baf5925c2d3ead2','specific reviewed earlier/fixture manifest required')
    relative={'junit':'testing/gc-selected.junit.xml','log':'ctest.log','terminal':'execution-terminal.json','before':'before.json','after':'after.json','result':'result.json'}
    require(prior_manifest.get('format')=='nano-gc-amd64-first-seven-run-public-bindings-v1' and set(prior_manifest.get('files',{}))==set(relative),'exact prior public manifest required')
    old={}
    for key,rel in relative.items():
        row=prior_manifest['files'][key];require(row['path']==rel,'prior relative evidence path');raw=pinned(pathlib.Path(a.prior_run)/rel,row['sha256'],32*1024*1024);require(len(raw)==row['bytes'],'prior evidence bytes');old[key]=raw
    prior_terminal=json.loads(old['terminal']);prior_result=json.loads(old['result']);prior_before=json.loads(old['before']);prior_after=json.loads(old['after'])
    require(prior_terminal.get('format')=='nano-gc-amd64-seven-selected-execution-terminal-v1' and prior_terminal.get('exitCode')==8 and prior_terminal.get('processCompleted') is True and prior_terminal.get('containerAbsentAfterCleanup') is True and prior_terminal.get('bindingInvariantsHeld') is True and prior_terminal.get('timedOut') is False and prior_terminal.get('launched') is True and isinstance(prior_terminal.get('failure'),str) and prior_terminal['failure'],'historical exit8/failure/stopped/invariant result must remain visible')
    require(prior_result.get('passedWithinTestScope') is False and prior_result.get('executionTerminalSha256')==prior_manifest['files']['terminal']['sha256'] and prior_result.get('noRelaunchAttempted') is True,'historical failed result cannot be rewritten')
    require(prior_after.get('allBindingsUnchanged') is True and all(prior_before.get(k)==prior_after.get(k) for k in ['source','roots','artifacts']),'original source/build changed during earlier run')
    require(all(before.get(k)==prior_before.get(k) for k in ['source','roots','artifacts','metadata','ctestGraph','buildTerminalSha256','buildControllerSha256','publicInputsSha256','gitCommit']),'source/core/program/graph bytes must be identical across runs')
    require(before.get('priorManifestSha256')==a.prior_manifest_sha and before.get('fixtureManifestSha256')==a.fixture_manifest_sha,'retry evidence/fixture provenance pins missing')
    require(fixture_manifest.get('format')=='nano-gc-amd64-public-runtime-fixtures-v1' and fixture_manifest.get('gitCommit')==before['gitCommit'] and fixture_manifest.get('privateDataCopied') is False and fixture_manifest.get('cppCompilerSourcePinsModified') is False and fixture_manifest.get('overlayPathCount')==23 and fixture_manifest.get('overlayPayloadBytes')<=16*1024*1024,'23 public current-Git runtime-only fixtures required')
    fixture_files=before.get('fixtures',{}).get('files',{}); require(len(fixture_manifest['files'])==23 and set(fixture_files)=={r['path'] for r in fixture_manifest['files']} and all(fixture_files[r['path']]['sha256']==r['sha256'] and fixture_files[r['path']]['bytes']==r['bytes'] for r in fixture_manifest['files']),'actual before-overlay bytes not bound')
    historical={k[len('tests/fixtures/'):]:v['sha256'] for k,v in before['source'].items() if k.startswith('tests/fixtures/')}
    require(len(historical)==20 and fixture_manifest.get('historicalFixturePinsPreserved')==historical and all(fixture_files[k]['sha256']==v for k,v in historical.items()),'overlay must preserve all old fixture pins')
    oldcases=list(ET.fromstring(old['junit']).iter('testcase')); all_names=NEW+REGRESSION+[AVAILABILITY];prior_names=[REGRESSION[0],AVAILABILITY];retry_names=NEW+[REGRESSION[1]]
    require(len(oldcases)==7 and {c.get('name') for c in oldcases}==set(all_names),'exact old seven JUnit names required')
    for c in oldcases:
        require(not list(c.iter('skipped')),'historical skips cannot count')
        if c.get('name') in prior_names:require(c.get('status')=='run' and not list(c.iter('failure')) and not list(c.iter('error')),'both previous actual passes required')
        else:require(c.get('status')=='fail' and list(c.iter('failure')) and '/work/tests/fixtures/consensus-bellatrix-containers-official/minimal-phase0.yaml' in ''.join(c.itertext()),'five prior fixture failures must stay visible')
    tree = ET.fromstring(pinned(a.junit, a.junit_sha, 8*1024*1024))
    cases = list(tree.iter('testcase')); expected = retry_names
    require(len(cases) == len(expected) and sorted(c.get('name') for c in cases) == sorted(expected), 'exact five newly retried AMD registered tests required')
    for c in cases:
        require(c.get('status') == 'run' and not any(list(c.iter(k)) for k in ['failure','error','skipped']), 'test not run or failed: ' + str(c.get('name')))
    rows = []
    for line in pinned(a.log, a.log_sha, 32*1024*1024).decode('utf-8', 'strict').splitlines():
        line = re.sub(r'^\s*\d+:\s*', '', line)
        if line.startswith('{'):
            try: value = json.loads(line)
            except json.JSONDecodeError: continue
            if isinstance(value, dict): rows.append(value)
    prior_rows=[]
    for line in old['log'].decode('utf-8','strict').splitlines():
        line=re.sub(r'^\s*\d+:\s*','',line)
        if line.startswith('{'):
            try: value=json.loads(line)
            except json.JSONDecodeError: continue
            if isinstance(value,dict): prior_rows.append(value)
    availability_names={'capella-local-sync-postready63','capella-real-reactor-local-sync-postready63','capella-real-reactor-duty-projection-head-recovery'}
    preserved_availability=[r for r in prior_rows if r.get('scenario') in availability_names];require(len(preserved_availability)==3,'three old actual availability rows required'); rows+=preserved_availability
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
    print(json.dumps({'format':'nano-gc-amd64-five-new-two-prior-public-result-audit-v1','passedWithinTestScope':True,'coverageComposedFromDistinctActualRuns':True,'newActualRegisteredTests':expected,'historicalActualPassNames':prior_names,'historicalFailureNamesRetained':retry_names,'historicalRunExitCodeRetained':8,'singleSevenTestExecution':False,'newFiveActualRunNoSkipRequired':True,'priorPublicProvenance':prior_manifest,'runtimeFixtureManifestSha256':a.fixture_manifest_sha,'runtimeFixtureGitCommit':fixture_manifest['gitCommit'],'compilerSourcePinsRewritten':False,'provenance':{name:{'path':getattr(a,name.replace('-','_')),'sha256':getattr(a,name.replace('-','_')+'_sha')} for name in ['junit','log','terminal','before','build-terminal','prior-manifest','fixture-manifest']},'actualPublicInputsSha256':before['publicInputsSha256'],'actualNewAmdArchiveSha256':build['archive']['sha256'],'actualNewAvailabilityElfSha256':programs['nano-capella-duty-availability-tests']['binary']['sha256'],'adversarialBranchIdsObserved':ids,'actualFsyncBranchesObserved':2,'actualBetweenStoreLogicalBoundaryObserved':True,'actualNaturalGcSlot':intermediate['gcSlot'],'actualPostGcOrdinaryBlocks':56-intermediate['gcSlot'],'actualCliReceiptSha256':cli['receiptSha256'],'actualCliReceiptBytes':cli['receiptBytes'],'availabilityActualProvenance':'earlier run01 passing availability CTest','availabilityInternalScenarioCount':3,'availabilityNotAddedToRegisteredTestCount':True,'availabilityProviderCalls':0,'availabilityPreparationQualifiedNotLocalSignedDuties':True,'priorArmResultsReused':False,'skippedTestsCounted':0,'physicalPowerLossQualified':False,'privateInputsRead':False,'runtimeDutiesResumed':False,'productionQualified':False},indent=2))
if __name__ == '__main__': main()
