#!/usr/bin/env python3
"""Copy the final public duty-availability evidence; execute no test/build/source script."""
from pathlib import Path, PurePosixPath
import os, stat, json, hashlib, datetime
import xml.etree.ElementTree as ET

BASE = Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
REPO = Path('/Users/boris/Dev/blockchain-nano')
BUILD = BASE / 'duty-availability-coherent-arm64-20261013'
AUTO = BASE / 'new-duty-projection-availability-counterexample-20261013/candidate-preparation/postready-sync-extension/autonomous-integration-public-evidence'
OUT = BASE / 'duty-availability-final-public-evidence-20261013'
AUTO_SHA = '5822208e8b4049bfaaede583ec15fd8c867f20a66fd0042f94d43dc347a87b55'
REVIEW = 'independent-review/registered-final-lot-20261013'
REVIEW_HASHES = {
    'terminal-review.json': 'efe5308370d219bdee4230d842dce44d4a78eba551cbde6898ea5a6e6abe01b1',
    'README.md': 'c62c291c7e35b4d9594ddf1ced6a8cc8fc365e9d22fea847f4d2c07fb6e72031',
    'targeted-duty-binding-addendum.json': '0c4da1ff6c338a78d5afdc1a17c94f1e26ef81b5f19e6513aae90a9222375c5f',
}
ROWS, WATCH, PRODUCED = [], {}, []
sha = lambda b: hashlib.sha256(b).hexdigest()

def rel(text):
    p = PurePosixPath(text)
    assert not p.is_absolute() and '..' not in p.parts and str(p) == text
    return p

def ident(s):
    return s.st_dev,s.st_ino,s.st_mode,s.st_size,s.st_mtime_ns,s.st_ctime_ns

def read(root, name, expected=None, size=None):
    p = root / rel(name)
    assert p.resolve().is_relative_to(root.resolve())
    assert all(not q.is_symlink() for q in [p,*p.parents])
    fd = os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
    try:
        a = os.fstat(fd);assert stat.S_ISREG(a.st_mode)
        parts=[]
        while True:
            b = os.read(fd,1048576)
            if not b:break
            parts.append(b)
        data=b''.join(parts);z=os.fstat(fd)
    finally:os.close(fd)
    assert ident(a)==ident(z)==ident(p.lstat())
    h=sha(data);assert expected is None or h==expected,(name,h,expected)
    assert size is None or len(data)==size
    value=(ident(z),h,len(data))
    assert p not in WATCH or WATCH[p]==value
    WATCH[p]=value
    return data

def json_guard(j):
    if isinstance(j,dict):
        for k,v in j.items():
            if k in {'snapshot','witnessSnapshot','canonicalStore','rawState','privateKey','secretKey','seedBytes','keyMaterial'}:
                assert v in (None,False,[]),k
            json_guard(v)
    elif isinstance(j,list):
        for v in j:json_guard(v)

def add(root,name,destination=None,expected=None,size=None):
    dest=destination or name;rel(dest)
    p=PurePosixPath(name)
    assert p.suffix in {'.json','.log','.py','.cpp','.hpp','.md','.xml','.make','.txt','.sh','.cmake','.mjs','.patch'}
    assert not any(x in {'tmp','captures','capsules','private','ledger','data','witness'} for x in p.parts)
    assert not p.name.endswith(('.private.json','.env','.key','.o','.a','.so','.dylib'))
    data=read(root,name,expected,size);text=data.decode('utf-8');assert '\0' not in text
    assert all(x not in text for x in ['-----BEGIN '+'PRIVATE KEY-----','-----BEGIN '+'OPENSSH PRIVATE KEY-----'])
    if p.suffix=='.json':json_guard(json.loads(text))
    assert dest not in {x['destination'] for x in ROWS}
    ROWS.append({'sourceRoot':'repository-public-source' if root==REPO else 'local-workspace-public-evidence',
                 'source':str((root/name).relative_to(REPO if root==REPO else BASE)),
                 'destination':dest,'sha256':sha(data),'bytes':len(data),'_data':data})

def create(name,data):
    p=OUT/rel(name);p.parent.mkdir(parents=True,exist_ok=True)
    fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o644)
    try:
        off=0
        while off<len(data):off+=os.write(fd,data[off:])
        os.fsync(fd)
    finally:os.close(fd)
    assert p.read_bytes()==data and not p.is_symlink()
    PRODUCED.append({'path':name,'bytes':len(data),'sha256':sha(data)})

def save_json(name,j):
    create(name,(json.dumps(j,indent=2,ensure_ascii=False)+'\n').encode())

def stage_files(folder,names,prefix=None):
    for name in names:add(BUILD,folder+'/'+name,(prefix or folder)+'/'+name)

def main():
    assert not OUT.exists() and not OUT.is_symlink(),'fresh package required'
    old_manifest=BASE/'resumption-evidence-public-20261013/public-evidence-manifest.json'
    old_sha=sha(old_manifest.read_bytes());assert old_sha=='5c6e893594015c75bc9dc9e00bf2c798a156063638bc5e1b16e0298bf543bbb5'
    original=json.loads(read(AUTO,'manifest.json',AUTO_SHA));assert len(original['files'])==50
    for row in original['files']:add(AUTO,row['path'],'autonomous-integration-public-evidence/'+row['path'],row['sha256'],row['bytes'])
    add(AUTO,'manifest.json','autonomous-integration-public-evidence/manifest.json',AUTO_SHA)
    stage_files('receipts',['build.command.json','build.exit.json','build.log','configure.command.json','configure.exit.json','configure.log','executed-driver-terminal-binding.json','hashes.command.json','hashes.exit.json','hashes.log','scope.json','source-after.json','source-before.json','terminal-receipt.json'],'coherent-arm64-build/receipts')
    for name in ['driver-static-review.json','full-core-build-evidence-audit.json','main-link-audit.json']:
        add(BUILD,'independent-review/'+name,'coherent-arm64-build/independent-review/'+name)
    audit=json.loads(read(BUILD,'independent-review/full-core-build-evidence-audit.json'))
    assert audit['coreObjectCount']==139 and audit['allExplicitCompileCommandsObservedInBuildLog'] is True
    for source,expected in audit['references'].items():
        if source=='build-candidate.py':read(BUILD,'build-candidate.executed.py',expected)
        elif source=='build/compile_commands.json':continue
        else:read(BUILD,source,expected)
    for target in ['nano_core','nano']:
        for name in ['flags.make','link.txt']:
            source='build/CMakeFiles/'+target+'.dir/'+name
            add(BUILD,source,'coherent-arm64-build/generated-production-commands/'+target+'/'+name,audit['references'][source])
    add(BUILD,'build-candidate.executed.py','coherent-arm64-build/build-candidate.executed.py','8f31ee96ec1fecea5ec269cda8eb4d31bfb6d7fae6372c38054ee86aa3a25c5b')
    add(BUILD,'build-candidate.py','coherent-arm64-build/corrected-for-future-runs/build-candidate.py','676c594d158d09cd3431c7d3df6f3d57269a779b95869b58a08322cb7751457e')
    stage_files('focused-regression',['build-transition-tests.command.json','build-transition-tests.exit.json','build-transition-tests.log','ctest-local-sync.command.json','ctest-local-sync.exit.json','ctest-local-sync.log','parent-binary-after.json','parent-binary-before.json','prepared-block.command.json','prepared-block.exit.json','prepared-block.log','source-before.json','terminal-receipt.json','test-source-before.json'])
    add(BUILD,'run-focused-regression.py','focused-regression/run-focused-regression.py','579eed03aa12a75c7c23474790fb48ffe4cbda19ad9c8adae7383d8dd254adfd')
    add(BUILD,'build/duty-local-sync.xml','focused-regression/duty-local-sync.xml','5b7b6578445b84aabbd3e04930fd0507d55e1af04491a601d46f2bae13cfd777')
    stage_files('registered-new-test',['build-new-test.command.json','build-new-test.exit.json','build-new-test.log','driver-terminal-pin.json','register-and-run.executed-fail.py','source-before.json','terminal-receipt.json'])
    stage_files('registered-new-test-configured',['build-new-test.command.json','build-new-test.exit.json','build-new-test.log','configure-new-test.command.json','configure-new-test.exit.json','configure-new-test.log','ctest.command.json','ctest.exit.json','ctest.log','new-test.xml','program-after.json','program-before.json','registration.command.json','registration.exit.json','registration.log','source-before.json','terminal-receipt.json'])
    add(BUILD,'register-and-run-new-ctest.py','registered-new-test-configured/register-and-run-new-ctest.py','22decc717a057f187eb7b7c6e1aeb883b02ed2abb6b6498848cc9f4d15d9f62e')
    add(BUILD,'build/Testing/Temporary/LastTest.log','registered-new-test-configured/LastTest.log','b5e778ce495a1be76b54468547a4f35d0eb36d6f72d5bbb9f7b75d35942b9097')
    add(BUILD,'ci-fourteen-selection-tests.log','ci-controls/ci-fourteen-selection-tests.log','65f1a4a0ddad89cef5ce55977e8e4f3964a1b6723c69ed0527beeb98937fede9')
    add(BUILD,'ci-duty-binding-targeted-test.json','ci-controls/ci-duty-binding-targeted-test.json','a4aff5ecc3898f51aa2f5e4db428456058131775155c2a6d9143fe7aaa4e0f1c')
    for name,expected in REVIEW_HASHES.items():add(BUILD,REVIEW+'/'+name,'final-independent-review/'+name,expected)
    review=json.loads(read(BUILD,REVIEW+'/terminal-review.json',REVIEW_HASHES['terminal-review.json']))
    for source,expected in review['sourceBindings'].items():add(REPO,source,'final-public-source/'+source,expected)
    addendum=json.loads(read(BUILD,REVIEW+'/targeted-duty-binding-addendum.json',REVIEW_HASHES['targeted-duty-binding-addendum.json']))
    assert addendum['previousReviewSha256']==REVIEW_HASHES['terminal-review.json'] and addendum['fullNineHarnessSuiteRun'] is False
    ctest=json.loads(read(BUILD,'registered-new-test-configured/registration.log'))
    assert len(ctest['tests'])==1
    registration=ctest['tests'][0];assert registration['name']=='consensus_capella_duty_projection_availability'
    assert registration['command']==['/outputs/nano-capella-duty-availability-tests','--self-test']
    for source,name in [('build/duty-local-sync.xml','consensus_capella_local_sync'),('registered-new-test-configured/new-test.xml','consensus_capella_duty_projection_availability')]:
        junit=ET.fromstring(read(BUILD,source));assert junit.attrib['tests']=='1'
        assert all(junit.attrib[x]=='0' for x in ['failures','disabled','skipped'])
        cases=junit.findall('testcase');assert len(cases)==1 and cases[0].attrib['name']==name and cases[0].attrib['status']=='run'
    all_cases=[]
    for line in read(BUILD,'registered-new-test-configured/ctest.log').decode().splitlines():
        if line.startswith('6: {'):all_cases.append(json.loads(line[3:]))
    assert len(all_cases)==3 and all(x['result']=='PASS' for x in all_cases)
    assert max(x['maxRssKiB'] for x in all_cases)==30552
    assert all_cases[-1]['normallyAdmittedPeerBlocks']==48 and all_cases[-1]['syntheticFixtureSignaturesCreated']==144 and all_cases[-1]['localProviderCalls']==0
    initial_failure=json.loads(read(BUILD,'registered-new-test/terminal-receipt.json'))
    assert initial_failure['passedWithinScope'] is False
    assert 'No rule to make target' in read(BUILD,'registered-new-test/build-new-test.log').decode()
    # Observe mutated current graph hash only; never substitute it for the missing old snapshot.
    current_graph_sha=sha(read(BUILD,'build/compile_commands.json'))
    assert current_graph_sha!=audit['references']['build/compile_commands.json']
    add(BASE,Path(__file__).name,'package-builder.py')
    OUT.mkdir(mode=0o755)
    for row in ROWS:create(row['destination'],row['_data'])
    save_json('provenance-and-limitations.json',{
        'format':'nano-duty-availability-final-public-package-provenance-v1','createdAtUtc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'sourcesCopied':[{k:v for k,v in x.items() if k!='_data'} for x in ROWS],
        'originalAutonomousManifestSha256':AUTO_SHA,'originalAutonomousFileCount':50,'originalAutonomousManifestUnchanged':True,
        'prior213FilePackageManifestSha256':old_sha,'priorPackageNotModified':True,
        'initialCoherentBuild':{'coreObjectsIndependentlyAudited':139,'mainObjectIndependentlyAudited':1,'preLaunchEmptyDirectoryInventoryRecorded':False,'executedDriverSourceHashObservedTerminally':True},
        'missingOriginalCompileCommands':{'expectedOriginalSha256':audit['references']['build/compile_commands.json'],'currentPostReconfigureSha256':current_graph_sha,'originalSnapshotFound':False,'mutableCurrentFileIncludedAsOriginal':False,'historicalAuditRewritten':False},
        'focusedRegression':{'executedCases':['consensus_capella_local_sync','direct --prepared-block'],'localSyncJUnitIncluded':True,'preparedBlockJUnitExists':False,'faultLibrarySha256ObservedOnlyTerminally':'cb04d92fae79aa16c26158ddecdddfc70d45e4a53247762129fcf540bfdaebda','faultLibraryPreDispatchPinProved':False},
        'retroactiveCollectorPinRefusal':{'toolChunk':'9e6066','exitCode':1,'originalDurableReceiptLocated':False,'source':'parent report, not a reproduced command or literal error transcript','classification':'collector refused adding a retroactive fault-library pin after dispatch','automaticPermissionRejection':False,'cppTestFailure':False},
        'newRegisteredCTest':{'registrationsExecuted':1,'internalScenarios':3,'junitSystemOutTruncated':True,'fullThreeResultsSource':'registered-new-test-configured/ctest.log','containerMemory':'2g','internalRssGuardKiB':131072,'observedProcessHighWaterRssKiB':30552,'coreCompilationActuallyTriggered':['src/kzg.cpp.o'],'coreArchiveAndNanoBeforeAfterHashesEqual':True,'syntheticSignatures':144,'ordinaryPeerBlocks':48,'localSignatureProviderCalls':0},
        'ciHarness':{'oldHarnessTests':8,'newTargetedHarnessTests':1,'newNegativeVariants':6,'fullNineSuiteRun':False,'fullFourteenCppSelectionRun':False,'oldStandaloneEightHarnessPreexecutionSourcePinFound':False,'ciSourceObservedByReviewerTerminally':True},
        'excludedPayloads':['executables','object/static/shared libraries','database files','synthetic fixture database directories','private keys/seed/env','campaign/private capsules','private/stat capture inventories'],
        'legacyFlatRssGateOpen':True,'nativeOrAmd64CandidateQualified':False,'fullCiQualified':False,'actualCampaignLocalSigningDutiesQualified':False,'commonFinalityQualified':False,'runtimeRecoveryQualified':False,'productionQualified':False,
        'repoEditedByPackager':False,'compilerOrTestsRunByPackager':False,'networkOrVpsUsedByPackager':False})
    readme='''# Final duty-availability evidence — bounded public package, 2026-10-13

This is a new package. It leaves the historical 213-file delivery and every existing manifest unchanged. Original sources, receipts, reviews, failures and limits are copied as historical evidence, not rewritten to agree with later stages. No build, test, runtime or network operation is executed by this packaging step.

## Qualified scopes and their sources

- `autonomous-integration-public-evidence/`: the exact original 50 files and unchanged manifest5822208e. The three individual synthetic results remain bound to their own test source revisions. The old manifest deliberately omits two unavailable earlier controller snapshots; that historical omission is retained. Its README predates the registered aggregate CTest and is not a claim that the latter remained pending after completion.
- `coherent-arm64-build/`: initial compiler/configure receipts, before/after public-source hash inventories, executed driver8f31 and independent audits of139 explicitly compiled core objects plus the main object. Old/new-layout archives were not mixed. An empty output directory at launch was not independently recorded; no such evidence is invented. The subsequently corrected driver676c is labeled for future runs and was not the executed build driver. The initial compile_commands snapshot4d828 is no longer available after reconfiguration; its mutated current file is excluded, while the original audit/reference hash and this omission remain explicit. Matching core/main flags and link commands are included.
- `focused-regression/`: the existing local-sync CTest plus its genuine JUnit, and direct --prepared-block log/exit. The direct mode has no JUnit; none is fabricated. The fault-library hash was observed terminally only. There is no proof of a pin before test dispatch. The parent-reported collector refusal9e6066/1 is documented without inventing an unavailable exact transcript; it was not an automatic permission rejection or C++ failure.
- `registered-new-test/`: first build failed with “No rule to make target”; no test ran. Its executed failed driver and terminal pins are preserved.
- `registered-new-test-configured/`: explicit reconfigure, build, exact registration, actual CTest command/log/exit, genuine JUnit, before/after executable and parent hashes, executed controller and LastTest.log. One registered --self-test executed three internal PASS scenarios. Its JUnit system-out is truncated at the configured1024 threshold; the three complete JSON results are in ctest.log.
- `ci-controls/`: eight earlier Node harness tests, then one newly added targeted test with six negative duty-binding variants. These are separate executions on different source states, not a full nine-test suite or fourteen actual C++ CI runs. Final gate/controller source snapshots are hash-bound to the final independent review and addendum. No preexecution source pin is invented for the earlier standalone eight-case log.
- `final-independent-review/`: frozen terminal reviewefe530, its original README, and addendum0c4da1. The original statement that a dedicated mutation test was absent is historical; the separate addendum closes only that exact targeted coverage gap. The source-binding observations remain terminal-only.

## Important actual-run limits

The registered build recompiled src/kzg.cpp.o and relinked core. The archive161a and nano64ca remained byte-identical before/after; this is not a claim that no core compilation was triggered. The actual aggregate CTest ran inside a2GiB container, with a cooperative internal128MiB RSS guard and observed process high-water30552KiB. The elapsed values in individual JSON results are cumulative from program start and are not additive. The144 signatures and48 peer blocks belong to explicit synthetic fixtures. Local signature provider calls remain zero because its key is pending registration: these cases establish resumed preparations, not actual signed campaign duties. Incoming admission stays strict.

The cold-storage test uses two encoded cache records and1MiB; decoded private LRU usage is not directly observed. Its success does not close the separate legacy flat path RSS refusal (137524KiB after39 blocks/117 synthetic signatures). Native/AMD64 coherence, full CI, real startup/restoration, signed local duties, common-finality progression, V37 recovery and production readiness remain unqualified.

## Integrity and safety

`manifest.json` is the destination inventory. `provenance-and-limitations.json` maps copied bytes to exact public sources and states omissions and limits. Every selected source is regular, nonsymlink, contained in its approved root, and checked before/after copying. Only sources, public commands, logs, hash metadata, XML and reviews are delivered. Source hash inventories and build-object audit metadata are public build evidence; no private/stat capture inventory is included. Executables, objects, libraries, databases, fixture data directories, keys, seeds, environment files, private capsules and VPS evidence are excluded. All former refusal files remain unchanged in their original locations.
'''
    create('README.md',readme.encode())
    for p,(_,expected,size) in list(WATCH.items()):read(p.parent,p.name,expected,size)
    assert sha(old_manifest.read_bytes())==old_sha
    save_json('copy-validation.json',{'format':'nano-final-duty-public-copy-validation-v1','copiedSourceFiles':len(ROWS),'allSelectedSourcesBytesAndIdentitiesUnchanged':True,'allCopiedBytesRechecked':True,'symlinks':0,'privatePayloadIncluded':False,'priorPublicPackageManifestUnchanged':True,'compilerTestsNetworkVpsInvoked':False,'repoEdited':False,'productionQualified':False})
    inventory={'format':'nano-duty-availability-final-public-evidence-manifest-v1','files':sorted(PRODUCED,key=lambda x:x['path']),'originalManifestsUnchanged':True,'productionQualified':False}
    raw=(json.dumps(inventory,indent=2)+'\n').encode();create('manifest.json',raw)
    print(json.dumps({'destination':str(OUT),'sourceCopies':len(ROWS),'filesIncludingManifest':len(PRODUCED),'bytesIncludingManifest':sum(x['bytes'] for x in PRODUCED),'manifestSha256':sha(raw),'priorPackageUnchanged':True,'compilerOrTestsExecuted':False,'vpsUsed':False,'productionQualified':False},indent=2))

if __name__=='__main__':main()
