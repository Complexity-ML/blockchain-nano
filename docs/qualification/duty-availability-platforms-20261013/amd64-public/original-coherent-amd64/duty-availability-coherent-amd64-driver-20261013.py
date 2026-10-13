#!/usr/bin/env python3
"""New-output-only coherent AMD64 compilation and three bounded synthetic tests."""
import hashlib, json, os, pathlib, stat, subprocess, sys, time, re
import xml.etree.ElementTree as ET

REPO = pathlib.Path('/home/ubuntu/blockchain-nano-git')
ROOT = pathlib.Path('/home/ubuntu/nanobees-qualification/duty-availability-coherent-amd64-20261013')
IMAGE = 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
REVISION = 'c672a30dbdc7b43ab04186db0e54be78510e6789'
REFERENCE_SHA = 'a1ed23b1d939a9ec02e7e0f3e357c7ed7f70eabf15d58be20e507a3b9e7f3479'
PINS = {
    'include/nano/consensus_altair_epoch.hpp':'668bedeb2334109d9cf19ba6fa3a77f5f6396dc484fed525e41e5abe7dc315ac',
    'src/consensus_altair_sync.cpp':'86e3ea6d1ee0c4b979ca076194d86da57f2dd54821a14c86391b3b91e3fedeeb',
    'include/nano/consensus_capella_duty.hpp':'b5000e20680212b38bf56457d74a557336928637349c650011671698f9570236',
    'src/consensus_capella_duty.cpp':'a08ed22c61ca6fa389b5d7e4304fee6772a2f9ee04e8e16ee2b1936bf18a0fff',
    'include/nano/consensus_capella_reactor.hpp':'b7b6a3f716945ffca5420d8cf60e9fd73df76974d45938eefaa6341ffe6d4cb4',
    'src/consensus_capella_sync_signing.cpp':'975b5072cbcf054fee2f88033763044825630616631f55e2fdfbe3287c3f5fad',
    'CMakeLists.txt':'28fe657bbf1a54ad5663f64024d95f90eeb8a5e5fdd25943f0a684042e66f0ec',
    'tests/test_capella_duty_availability.cpp':'8690cc722e6642d14be1bc4b021eef9924be47e2b7e17d070a04908aa256ded2',
    'tests/test_capella_transition.cpp':'e16d75e46b8efda8118f0752c06a0247a06b8cdd173526609bc521284a9f3355',
    'tests/support/isolated_capella_fixture.hpp':'afc9356b01085e9b96f6cb705abfe7ce9c5de8d0a70dcb3d138ec82688a538af',
}
HOST_ENV={'PATH':'/usr/bin:/bin','LANG':'C.UTF-8'}
STOPS=[]
sha = lambda data: hashlib.sha256(data).hexdigest()

def safe_read(p):
    assert not p.is_symlink() and p.is_file(),str(p)
    fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
    try:
        a=os.fstat(fd);data=b''
        while True:
            part=os.read(fd,1048576)
            if not part:break
            data+=part
        b=os.fstat(fd)
    finally:os.close(fd)
    assert (a.st_dev,a.st_ino,a.st_size,a.st_mtime_ns,a.st_ctime_ns)==(b.st_dev,b.st_ino,b.st_size,b.st_mtime_ns,b.st_ctime_ns)
    return data

def file_sha(p):return sha(safe_read(p))

def source_inventory():
    result={}
    tracked=host(['/usr/bin/git','-C',str(REPO),'ls-files','-z']).split('\0')
    fixture_configs={
        'consensus-slashing-official/manifest.json','consensus-slashing-official/minimal-phase0.yaml','consensus-slashing-official/minimal-config.yaml',
        'consensus-altair-state-roots-official/manifest.json','consensus-altair-state-roots-official/minimal-altair.yaml',
        'consensus-bellatrix-containers-official/minimal-bellatrix.yaml','consensus-bellatrix-containers-official/manifest.json',
        'consensus-capella-withdrawals-official/minimal-capella.yaml','consensus-altair-fork-head-official/manifest.json',
        'consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml','consensus-capella-slots-official/manifest.json'}
    # Legacy helpers authenticate every referenceFiles member in these public manifests.
    for folder in ['consensus-slashing-official','consensus-altair-state-roots-official','consensus-altair-fork-head-official']:
        manifest=json.loads(safe_read(REPO/'tests/fixtures'/folder/'manifest.json'))
        for name in manifest.get('referenceFiles',{}):
            assert not pathlib.PurePosixPath(name).is_absolute() and '..' not in pathlib.PurePosixPath(name).parts
            fixture_configs.add(folder+'/'+name)
    for name in sorted(x for x in tracked if x):
        p=REPO/name;parts=pathlib.PurePosixPath(name).parts
        selected=name=='CMakeLists.txt' or parts[0] in {'src','include','cmake','third_party'}
        selected=selected or (parts[0]=='tests' and p.suffix in {'.cpp','.hpp','.h'}) or (parts[0]=='network' and p.suffix in {'.hpp','.h'})
        selected=selected or (name.startswith('tests/fixtures/') and name[len('tests/fixtures/'):] in fixture_configs)
        selected=selected or name.startswith('tests/fixtures/consensus-capella-slots-official/slots_1/')
        if selected:
            assert not p.is_symlink(),name;result[name]=file_sha(p)
    return result

def host(command):
    return subprocess.check_output(command,env=HOST_ENV,text=True,timeout=30)

def write(name,value):
    p=ROOT/'receipts'/name
    assert not p.exists() and not p.is_symlink(),name
    data=(json.dumps(value,indent=2)+'\n').encode()
    fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
    try:
        offset=0
        while offset<len(data):offset+=os.write(fd,data[offset:])
        os.fsync(fd)
    finally:os.close(fd)

def directory_identity(p):
    s=p.lstat();assert stat.S_ISDIR(s.st_mode) and not p.is_symlink() and s.st_uid==os.getuid() and stat.S_IMODE(s.st_mode)==0o700
    return {'device':s.st_dev,'inode':s.st_ino,'owner':s.st_uid,'mode':stat.S_IMODE(s.st_mode)}

def same_directories():
    for name,value in OUTPUT_IDS.items():assert directory_identity(ROOT/name)==value

def run(name,args,timeout,memory='4g'):
    same_directories()
    container='nano-duty-'+name+'-amd64-20261013'
    command=['/usr/bin/sudo','-n','/usr/bin/docker','run','--rm','--pull=never','--platform','linux/amd64',
             '--name',container,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges',
             '--user','1000:1000','--cpus','2','--memory',memory,'--pids-limit','256','--env','TMPDIR=/receipts/tmp',
             '--mount',f'type=bind,source={REPO},target=/work,readonly',
             '--mount',f'type=bind,source={ROOT/"build"},target=/outputs',
             '--mount',f'type=bind,source={ROOT/"receipts"},target=/receipts',
             '--entrypoint',args[0],IMAGE]+args[1:]
    write(name+'.command.json',{'args':command,'timeoutSeconds':timeout,'privateCampaignVolumesMounted':False})
    start=time.monotonic();log=ROOT/'receipts'/(name+'.log');fd=os.open(log,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
    disposition=None
    print(json.dumps({'phase':name,'status':'started'}),flush=True)
    with os.fdopen(fd,'wb') as stream:
        try:code=subprocess.run(command,env=HOST_ENV,stdout=stream,stderr=subprocess.STDOUT,timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            code=124;disposition='original-phase-timeout-no-restart'
            kill=subprocess.run(['/usr/bin/sudo','-n','/usr/bin/docker','kill',container],env=HOST_ENV,capture_output=True,text=True,timeout=20)
            write(name+'.timeout-stop.json',{'container':container,'exitCode':kill.returncode,'scope':'only this fresh build/test container; never a validator','stdout':kill.stdout,'stderr':kill.stderr})
    write(name+'.exit.json',{'exitCode':code,'elapsedSeconds':time.monotonic()-start,'logSha256':file_sha(log),'disposition':disposition})
    same_directories()
    print(json.dumps({'phase':name,'exitCode':code}),flush=True)
    if code:raise RuntimeError(name+' failed: '+str(code))

def programs():
    return {name:file_sha(ROOT/'build'/name) for name in ['libnano_core.a','nano','nano-capella-duty-availability-tests','nano-capella-transition-tests','libnano-test-sync-fault.so','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit']}

def positive_junit(name,test_name):
    root=ET.fromstring(safe_read(ROOT/'receipts'/name))
    assert root.tag=='testsuite' and root.attrib['tests']=='1'
    assert all(root.attrib[x]=='0' for x in ['failures','disabled','skipped'])
    cases=root.findall('testcase');assert len(cases)==1 and cases[0].attrib['name']==test_name and cases[0].attrib['status']=='run'

def verbose_json(name):
    values=[]
    for line in safe_read(ROOT/'receipts'/(name+'.log')).decode().splitlines():
        match=re.fullmatch(r'\d+: (\{.*\})',line)
        if match:values.append(json.loads(match.group(1)))
    return values

def preserve_original_build_evidence():
    out=ROOT/'original-generated-build-evidence';out.mkdir(mode=0o700)
    names=['compile_commands.json']+[f'CMakeFiles/{target}.dir/{name}' for target in ['nano_core','nano','nano-capella-duty-availability-tests','nano-capella-transition-tests','nano-test-sync-fault','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit'] for name in ['flags.make','link.txt']]
    rows=[]
    for name in names:
        data=safe_read(ROOT/'build'/name);dest=out/name;dest.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
        fd=os.open(dest,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
        try:
            offset=0
            while offset<len(data):offset+=os.write(fd,data[offset:])
            os.fsync(fd)
        finally:os.close(fd)
        rows.append({'path':name,'bytes':len(data),'sha256':sha(data)})
    write('original-generated-build-evidence.json',{'files':rows,'capturedBeforeAnyTestOrReconfiguration':True})
    commands=json.loads(safe_read(out/'compile_commands.json'))
    core=[x for x in commands if 'CMakeFiles/nano_core.dir/' in x['command']]
    assert len(core)==139
    tracked=host(['/usr/bin/git','-C',str(REPO),'ls-files','-z']).split('\0')
    expected_core={'/work/'+x for x in tracked if x.startswith('src/') and pathlib.PurePosixPath(x).parent==pathlib.PurePosixPath('src') and x.endswith('.cpp') and x!='src/main.cpp'}
    assert len(expected_core)==139 and {x['file'] for x in core}==expected_core
    build_log=safe_read(ROOT/'receipts/build.log').decode()
    objects=[]
    for item in core:
        # CMake's compile database uses a relative -o output; no command is executed here.
        import shlex
        args=shlex.split(item['command']);object_name=args[args.index('-o')+1]
        assert object_name.startswith('CMakeFiles/nano_core.dir/') and '..' not in pathlib.PurePosixPath(object_name).parts
        p=ROOT/'build'/object_name;s=p.stat()
        assert 'Building CXX object '+object_name in build_log or 'Building ASM object '+object_name in build_log or 'Building C object '+object_name in build_log
        objects.append({'source':item['file'],'path':object_name,'bytes':s.st_size,'sha256':file_sha(p),'mtimeNs':s.st_mtime_ns,'explicitCompileInLog':True})
    write('core-object-audit.json',{'coreObjectCount':len(objects),'objects':objects,'initialOutputEmptyProved':True,'compileCommandsSnapshotSha256':file_sha(out/'compile_commands.json'),'noOldCoreArchiveMounted':True})
    dependency_rows=[];tokens=[]
    import shlex
    for path in sorted((ROOT/'build').rglob('*.o.d')):
        assert not path.is_symlink();data=safe_read(path)
        name=str(path.relative_to(ROOT/'build'))
        destination=out/'compiler-dependencies'/name;destination.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
        with destination.open('xb') as stream:stream.write(data);stream.flush();os.fsync(stream.fileno())
        dependency_rows.append({'path':name,'bytes':len(data),'sha256':sha(data)})
        text=data.decode().replace('\\\n',' ');tokens+=shlex.split(text.split(':',1)[1])
    for item in commands:tokens+=shlex.split(item['command'])+[item['file']]
    for path in out.rglob('link.txt'):tokens+=shlex.split(safe_read(path).decode())
    ignored=reference['ignoredNonconsumedMclMiscEntries']
    matches={name:[x for x in tokens if '/work/'+name in x] for name in ignored}
    assert all(not values for values in matches.values())
    assert all(any(x['path']==item['path']+'.d' for x in dependency_rows) for item in objects)
    write('compiler-dependency-audit.json',{'dependencyFiles':dependency_rows,'coreSourcesExactlyMatchCanonicalGitCppGlobExcludingMain':True,'coreDependencyFilesPresentFor139Objects':True,'coreConsumerAndFaultLinkFlagsRetained':True,'ignored11MclMiscNoCompilerDependencyCommandOrLinkReference':True,'ignoredNames':ignored,'scope':'actual generated .o.d and compiler/link commands; no global historical claim or dependency-image requalification'})
    run('archive-members',['/usr/bin/ar','t','/outputs/libnano_core.a'],30)
    members=safe_read(ROOT/'receipts/archive-members.log').decode().splitlines()
    assert sorted(members)==sorted(pathlib.PurePosixPath(x['path']).name for x in objects)
    write('archive-object-members-audit.json',{'archiveMemberCount':len(members),'exact139CoreObjectMembers':True,'archiveSha256':file_sha(ROOT/'build/libnano_core.a'),'objectAuditSha256':file_sha(ROOT/'receipts/core-object-audit.json')})

assert len(sys.argv)==2 and sys.argv[1].startswith('--expected-driver-sha=')
driver_sha=sys.argv[1].split('=',1)[1];assert file_sha(pathlib.Path(__file__))==driver_sha
assert os.getuid()==1000
assert host(['/usr/bin/git','-C',str(REPO),'rev-parse','HEAD']).strip()==REVISION
assert host(['/usr/bin/git','-C',str(REPO),'status','--porcelain'])==''
for name,value in PINS.items():assert file_sha(REPO/name)==value,name
before=source_inventory()
reference_path=pathlib.Path(__file__).parent/'canonical-arm-production-reference.json'
assert file_sha(reference_path)==REFERENCE_SHA
reference=json.loads(safe_read(reference_path))
production={k:v for k,v in before.items() if pathlib.PurePosixPath(k).parts[0] in {'src','include','cmake','third_party'} or k=='CMakeLists.txt'}
assert production==reference['canonicalProductionHashes'],'canonical ARM/AMD production-source mismatch'
image=json.loads(host(['/usr/bin/sudo','-n','/usr/bin/docker','image','inspect',IMAGE]))[0]
assert image['Id']==IMAGE and image['Architecture']=='amd64'
assert not any(x.startswith(('LD_','DYLD_','NANO_')) for x in image['Config'].get('Env',[]))
assert ROOT.parent.resolve()==ROOT.parent and not ROOT.parent.is_symlink()
assert not ROOT.exists() and not ROOT.is_symlink(),'new output required; existing data never removed'
os.mkdir(ROOT,0o700)
os.mkdir(ROOT/'build',0o700);os.mkdir(ROOT/'receipts',0o700)
OUTPUT_IDS={name:directory_identity(ROOT/name) for name in ['build','receipts']}
assert list((ROOT/'build').iterdir())==[]
write('initial-empty-output.json',{'format':'nano-amd64-fresh-empty-output-proof-v1','output':str(ROOT/'build'),'entries':[],'createdAtomically':True,'beforeCmakeAndTmpCreation':True,'identity':OUTPUT_IDS['build'],'rootIdentity':directory_identity(ROOT),'driverSha256':driver_sha})
(ROOT/'receipts/tmp').mkdir(mode=0o700)
write('source-before.json',before)
write('canonical-production-comparison.json',{'canonicalReferenceSha256':file_sha(reference_path),'canonicalProductionCount':len(production),'exactCanonicalReferenceMatch':True,'ignored11MclMiscExpectedNonconsumedPendingCompilerDependencyAudit':True,'fullInventoryAddsPublicTestSupportNetworkHeadersAndConsumedFixtures':True})
write('scope.json',{'revision':REVISION,'image':IMAGE,'sourcePins':PINS,'publicSourceFiles':len(before),'repoMountedReadOnly':True,'network':'none','oldBuildOrValidatorVolumesMounted':False,'imageBuiltOrPulled':False,'freshOutputProvedEmpty':True,'driverPinnedBeforeAnyBuild':True,'actualCampaignDutiesOrNetworkRequested':False,'productionQualified':False})
failure=None;passed=[];prior=None
try:
    run('configure',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],180)
    run('build',['/usr/bin/cmake','--build','/outputs','--parallel','2','--target','nano_core','nano','nano-capella-duty-availability-tests','nano-capella-transition-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit'],1800)
    preserve_original_build_evidence()
    prior=programs();write('programs-before-tests.json',prior)
    run('registration',['/usr/bin/ctest','--test-dir','/outputs','-R','^(consensus_capella_duty_projection_availability|consensus_isolated_stopped_capture_guards)$','--show-only=json-v1'],30)
    registration=json.loads(safe_read(ROOT/'receipts/registration.log'))['tests'];assert len(registration)==2
    expected_registrations={
        'consensus_capella_duty_projection_availability':('nano-capella-duty-availability-tests',70,'qualification_consensus_duty_availability'),
        'consensus_isolated_stopped_capture_guards':('nano-capella-isolated-stopped-capture',15,'qualification_consensus_recovery_tooling')}
    for row in registration:
        program,timeout,label=expected_registrations.pop(row['name'])
        assert row['command']==['/outputs/'+program,'--self-test']
        properties=row['properties'];props={x['name']:x['value'] for x in properties};assert len(props)==len(properties)
        assert props['TIMEOUT']==timeout and props['LABELS']==[label]
        assert not set(props).intersection({'DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION','WILL_FAIL','PASS_REGULAR_EXPRESSION'})
    assert not expected_registrations
    run('availability',['/usr/bin/ctest','--test-dir','/outputs','-R','^consensus_capella_duty_projection_availability$','--verbose','--output-on-failure','--no-tests=error','--output-junit','/receipts/availability.xml'],90,'2g')
    positive_junit('availability.xml','consensus_capella_duty_projection_availability')
    results=verbose_json('availability');assert len(results)==3
    assert {x['scenario'] for x in results}=={'capella-local-sync-postready63','capella-real-reactor-local-sync-postready63','capella-real-reactor-duty-projection-head-recovery'}
    assert all(x['result']=='PASS' and x['maxRssKiB']<=131072 and x['elapsedMs']<60000 for x in results)
    peer=next(x for x in results if x['scenario']=='capella-real-reactor-duty-projection-head-recovery')
    assert peer['normallyAdmittedPeerBlocks']==48 and peer['newHeadSlot']==384 and peer['localProviderCalls']==0 and peer['syntheticFixtureSignaturesCreated']==144
    write('availability-results.json',{'registeredTests':1,'results':results,'genuineJUnitRunWithNoSkippedCases':True,'signedCampaignDutiesQualified':False})
    passed.append('new-availability-three-scenarios')
    run('local-sync',['/usr/bin/ctest','--test-dir','/outputs','-R','^consensus_capella_local_sync$','--output-on-failure','--no-tests=error','--output-junit','/receipts/local-sync.xml'],260)
    positive_junit('local-sync.xml','consensus_capella_local_sync');passed.append('existing-local-sync')
    run('prepared-block',['/outputs/nano-capella-transition-tests','--prepared-block'],180)
    prepared=json.loads(safe_read(ROOT/'receipts/prepared-block.log'))
    assert prepared['scenario']=='capella-prepared-block-transition' and prepared['localProposalIntegration'] is True and prepared['fullGapOracleEqual'] is True
    passed.append('existing-prepared-block')
    run('stopped-capture-guards',['/usr/bin/ctest','--test-dir','/outputs','-R','^consensus_isolated_stopped_capture_guards$','--verbose','--output-on-failure','--no-tests=error','--output-junit','/receipts/stopped-capture-guards.xml'],30)
    positive_junit('stopped-capture-guards.xml','consensus_isolated_stopped_capture_guards')
    pure=verbose_json('stopped-capture-guards');assert len(pure)==1 and pure[0]['passed'] is True and pure[0]['cases']==25 and pure[0]['databaseAccessed'] is False and pure[0]['newSignaturesCreated']==0
    passed.append('pure-stopped-capture-guards')
except Exception as error:failure=str(error)
after=source_inventory();write('source-after.json',after)
final=programs() if prior is not None else None
if final is not None:write('programs-after-tests.json',final)
same=before==after and host(['/usr/bin/git','-C',str(REPO),'rev-parse','HEAD']).strip()==REVISION and host(['/usr/bin/git','-C',str(REPO),'status','--porcelain'])==''
write('terminal-receipt.json',{'format':'nano-coherent-amd64-duty-availability-qualification-v1','passedWithinRequestedScope':failure is None and same and prior==final,'failure':failure,'sourceUnchanged':same,'programAndFaultLibraryBeforeAfterHashesEqual':prior==final if prior is not None else None,'faultLibraryPinnedBeforeAnyTest':prior is not None,'passedTestSelection':passed,'freshOutputProvedEmptyBeforeCmake':True,'originalCompileCommandsSnapshotRetained':(ROOT/'original-generated-build-evidence/compile_commands.json').exists(),'original139CoreObjectAuditRetained':(ROOT/'receipts/core-object-audit.json').exists(),'runtimeOrValidatorCampaignStarted':False,'campaignDataOrKeyOrEnvRead':False,'privateVpsRecoveryEvidenceExported':False,'imageBuiltOrPulled':False,'productionQualified':False})
print(json.dumps({'terminal':failure or 'success','passedSelection':passed,'sourceUnchanged':same,'programsUnchanged':prior==final,'root':str(ROOT)}),flush=True)
if failure or not same or prior!=final:raise SystemExit(failure or 'source/program drift')
