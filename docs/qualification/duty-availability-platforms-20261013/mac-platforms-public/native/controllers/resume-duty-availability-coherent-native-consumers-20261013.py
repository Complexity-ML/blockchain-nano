from pathlib import Path
import importlib.util,subprocess,json,hashlib,time,os,signal,shlex,shutil,xml.etree.ElementTree as ET,re
initial=Path(__file__).parent/'build-duty-availability-coherent-native-20261013.py';assert hashlib.sha256(initial.read_bytes()).hexdigest()=='50e2b6e609029f03fc293b666dff212bc2fead3a31d0604d255175949f69e7e9'
spec=importlib.util.spec_from_file_location('native_initial',initial);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
repo=m.repo;root=m.root;build=root/'build';sha=m.sha;save=m.save;query=m.query
resume=root/'consumer-typing-repair';resume.mkdir(mode=0o700);logs=resume/'logs';logs.mkdir(mode=0o700)
test_pin='44febdb90047ea8a7de5e78002d73ed96b82cc67259dd356e98c7ba096f6647d'
assert query(['git','rev-parse','HEAD'])==m.revision and sha(repo/'CMakeLists.txt')==m.cmake_pin and sha(repo/'tests/test_capella_duty_availability.cpp')==test_pin
assert all(sha(repo/p)==v for p,v in m.core_pins.items())
assert not [n for n in os.environ if n.startswith(('NANO_','DYLD_')) or n=='LD_PRELOAD']
before=json.loads((root/'source-before.json').read_text())['files'];assert before['tests/test_capella_duty_availability.cpp']==m.test_pin;before['tests/test_capella_duty_availability.cpp']=test_pin
extras=json.loads((root/'supplemental-before-consumer-compile.json').read_text())['additionalPublicHeaders'];before.update(extras);assert m.snap(before)==before
save(resume/'source-before.json',{'gitRevision':m.revision,'authorizedTestOnlyWorkingTreeDelta':True,'testOldSha256':m.test_pin,'testNewSha256':test_pin,'scope':'initial tracked source set plus two supplemental headers; supplementals are before-consumer, not beforewholebuild','files':before,'driverSha256':sha(__file__)})
commands=json.loads((root/'compile_commands.original.json').read_text());core=[x for x in commands if 'CMakeFiles/nano_core.dir/' in x.get('command','')];assert len(core)==139
objects=[]
for x in core+[next(x for x in commands if 'CMakeFiles/nano.dir/' in x.get('command',''))]:
 args=shlex.split(x['command']);p=Path(args[args.index('-o')+1]);p=p if p.is_absolute() else Path(x['directory'])/p;assert p.is_file();objects.append(p)
object_pins={str(p.relative_to(build)):sha(p) for p in objects};parent_pins={n:sha(build/n) for n in ['libnano_core.a','nano']};assert parent_pins=={'libnano_core.a':'90ee449b6c8087248d5b867347cfe408e44eb7bb680343eca5a2b74990febc80','nano':'2b4856b15d11c926e3b723199d50eb1f7ada0de89a7e7f4df6e6404861b87901'}
object_times={str(p.relative_to(build)):p.stat().st_mtime_ns for p in objects};commands_pin=sha(build/'compile_commands.json');assert commands_pin==sha(root/'compile_commands.original.json')
old_pins=json.loads((root/'old-native-artifacts-before.json').read_text());dep_pins=json.loads((root/'native-dependencies-before.json').read_text())['resolvedLibraryBindings'];fixture_pins=json.loads((root/'public-fixture-before.json').read_text());driver_pin=sha(__file__)
save(resume/'parent-core-main-before.json',{'files':parent_pins,'140CoreMainObjectSha256':object_pins,'140CoreMainObjectMtimeNs':object_times,'compileCommandsOriginalSha256':commands_pin})
env=dict(os.environ);env['TMPDIR']=str(root/'test-tmp')+'/'
def run(name,args,timeout):
 save(logs/(name+'-command.json'),{'args':args,'cwd':str(repo),'timeoutSeconds':timeout,'temporaryDirectory':str(root/'test-tmp')})
 start=time.monotonic()
 with (logs/(name+'.log')).open('wb') as log:
  child=subprocess.Popen(args,cwd=repo,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True);save(logs/(name+'-status.json'),{'state':'running','pid':child.pid,'processGroup':child.pid,'timeoutSeconds':timeout});print(json.dumps({'phase':name,'state':'running','pid':child.pid}),flush=True);expired=False
  try:code=child.wait(timeout=timeout)
  except subprocess.TimeoutExpired:
   expired=True;os.killpg(child.pid,signal.SIGTERM)
   try:code=child.wait(timeout=5)
   except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);code=child.wait(timeout=5)
  group_gone=None
  if expired:
   try:os.killpg(child.pid,0);group_gone=False
   except ProcessLookupError:group_gone=True
  terminal={'state':'failed-timeout' if expired else 'terminal','pid':child.pid,'exitCode':code,'timedOut':expired,'allManagedDescendantsStopped':group_gone,'elapsedSeconds':time.monotonic()-start};save(logs/(name+'-exit.json'),terminal);save(logs/(name+'-status.json'),terminal);print(json.dumps({'phase':name,**terminal}),flush=True)
  if expired or code:raise RuntimeError('phase '+name+' failed; no automatic retry')
def parent_audit():
 assert {str(p.relative_to(build)):sha(p) for p in objects}==object_pins and {str(p.relative_to(build)):p.stat().st_mtime_ns for p in objects}==object_times
 assert {n:sha(build/n) for n in parent_pins}==parent_pins and sha(build/'compile_commands.json')==commands_pin
 assert {n:sha(m.old/n) for n in old_pins}==old_pins and m.snap(before)==before
 save(resume/'parent-core-main-after.json',{'files':parent_pins,'140CoreMainObjectSha256':object_pins,'140CoreMainObjectMtimeNs':object_times,'all140ObjectBytesAndMtimeUnchanged':True,'coreArchiveAndNanoUnchanged':True,'compileCommandsOriginalUnchanged':True,'oldNativeArtifactsUnchanged':True})
try:
 # No configure call: only consumers and their still-unbuilt dependencies.
 targets=['nano-capella-duty-availability-tests','nano-capella-transition-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit']
 run('build-consumers-only',['/opt/homebrew/bin/cmake','--build',str(build),'--parallel','2','--target',*targets],600);parent_audit()
 text=(logs/'build-consumers-only.log').read_text();assert 'Building CXX object CMakeFiles/nano_core.dir/' not in text and 'Building CXX object CMakeFiles/nano.dir/' not in text
 generated=resume/'generated-compile-link';generated.mkdir()
 for target in ['nano_core','nano',*targets,'nano-test-sync-fault']:
  d=generated/target;d.mkdir()
  for n in ['flags.make','link.txt','DependInfo.cmake']:shutil.copyfile(build/'CMakeFiles'/(target+'.dir')/n,d/n)
 archive_entries=query(['/usr/bin/ar','-t',str(build/'libnano_core.a')]).splitlines();archive_units=[n for n in archive_entries if n not in ['__.SYMDEF','__.SYMDEF SORTED']];assert len(archive_units)==139 and sorted(archive_units)==sorted(Path(x['file']).name+'.o' for x in core)
 scope=json.loads((root/'source-scope-reconciliation.json').read_text());ignored=scope['priorArmUnconsumedIgnoredExtras'];depfiles=list(build.rglob('*.o.d'));consumed=set();aliases={}
 for d in depfiles:
  for token in d.read_text().replace('\\\n',' ').split():
   token=token.removesuffix(':')
   if token.startswith(str(repo)+'/'):
    resolved=Path(token).resolve(strict=True);assert resolved.is_relative_to(repo)
    path=str(resolved.relative_to(repo));consumed.add(path)
    if token!=str(resolved):aliases[token]=str(resolved)
 missing=sorted(consumed-set(before));assert not missing,'Consumed repo inputs absent from source bindings: '+str(missing)
 assert all(sha(repo/n)==before[n] for n in consumed) and not(set(ignored)&consumed)
 save(resume/'full-repo-input-dependency-closure.json',{'compiledDependencyFiles':len(depfiles),'actualConsumedRepoInputs':len(consumed),'allConsumedRepoInputsBound':True,'missingConsumedRepoInputs':missing,'files':{n:before[n] for n in sorted(consumed)},'normalizedDotDotAliases':aliases,'11PriorArmIgnoredFilesNotConsumed':ignored,'archiveObjectUnits':139,'bsdArchiveSymbolTables':[n for n in archive_entries if n not in archive_units],'supplementalHeadersPinnedBeforeConsumerCompile':extras,'supplementalPinsNotPrewholebuild':True})
 binary_names=['libnano_core.a','nano',*targets,'libnano-test-sync-fault.dylib'];binary_pins={n:sha(build/n) for n in binary_names};save(resume/'binary-manifest-before-tests.json',{'revision':m.revision,'authorizedTestOnlyWorkingTreeDeltaSha256':test_pin,'CMakeSha256':m.cmake_pin,'core6':m.core_pins,'files':binary_pins,'faultDylibIncluded':True,'runtimeQualified':False,'productionQualified':False})
 run('native-file-metadata',['/usr/bin/file',*[str(build/n) for n in binary_names]],30)
 run('native-dynamic-dependencies',['/usr/bin/otool','-L',*[str(build/n) for n in binary_names if n!='libnano_core.a']],30)
 names=['consensus_isolated_stopped_capture_guards','consensus_capella_duty_projection_availability','consensus_capella_local_sync'];regex='^('+ '|'.join(names)+')$'
 run('ctest-registration',['/opt/homebrew/bin/ctest','--test-dir',str(build),'--show-only=json-v1','-R',regex],30);discovery=json.loads((logs/'ctest-registration.log').read_text());assert [t['name'] for t in discovery['tests']]==names
 run('ctest-targeted',['/opt/homebrew/bin/ctest','--test-dir',str(build),'--output-on-failure','-V','--output-junit',str(resume/'targeted-junit.xml'),'-R',regex],360)
 junit=ET.parse(resume/'targeted-junit.xml');cases=junit.findall('.//testcase');assert sorted(c.get('name') for c in cases)==sorted(names) and len(cases)==3 and all(c.find('skipped') is None and c.find('failure') is None and c.find('error') is None for c in cases)
 rows=[]
 for line in (logs/'ctest-targeted.log').read_text().splitlines():
  try:row=json.loads(re.sub(r'^\d+:\s*','',line))
  except (ValueError,TypeError):continue
  if row.get('scenario') in ['capella-local-sync-postready63','capella-real-reactor-local-sync-postready63','capella-real-reactor-duty-projection-head-recovery']:rows.append(row)
 assert len(rows)==3 and all(x['result']=='PASS' and x['productionQualified'] is False and x['maxRssKiB']<=131072 and x['elapsedMs']<60000 and x['syntheticFixtureSignaturesCreated']<=160 for x in rows)
 save(resume/'new-synthetic-three-scenario-results.json',{'testSourceSha256':test_pin,'rows':rows,'scopesNotSummed':True,'localSignatureDutiesResumedQualified':False,'networkQualified':False,'productionQualified':False})
 run('prepared-block',[str(build/'nano-capella-transition-tests'),'--prepared-block'],120);parent_audit()
 assert {n:sha(build/n) for n in binary_pins}==binary_pins and {n:sha(repo/'tests/fixtures'/n) for n in fixture_pins}==fixture_pins and sha(__file__)==driver_pin
 assert all(sha(Path(v).resolve())==dep_pins[v]['sha256'] for v in dep_pins)
 save(resume/'binary-manifest-after-tests.json',binary_pins)
 save(resume/'terminal-qualification.json',{'format':'nano-duty-availability-coherent-darwin-arm64-qualification-v1','result':'PASS','productionSourceRevision':m.revision,'authorizedTestOnlyWorkingTreeDeltaSha256':test_pin,'CMakeSha256':m.cmake_pin,'core6':m.core_pins,'all139CoreUnitsFreshCompiledInInitialBuild':True,'initialBuildTest8690RefusalPreserved':True,'resumeBuiltOnlyConsumers':True,'all140CoreMainObjectBytesAndMtimeUnchanged':True,'archiveCoreAndNanoUnchanged':True,'compileCommandsOriginalSnapshotUnchanged':True,'sourceInventoryFiles':len(before),'sourceAndSupplementalInputsUnchangedBeforeAfter':True,'allConsumedRepoInputsBoundBeforeConsumption':True,'supplementalHeadersNotClaimedPinnedBeforeWholeBuild':True,'allEightArtifactsBeforeAfterTestsIdentical':True,'faultDylibPinned':True,'fixturesAndNativeLibrariesUnchanged':True,'oldNativeArtifactsUnchanged':True,'registeredCtestsPassed':names,'junitRealTestsPassed':3,'junitSkipped':0,'preparedBlockDirectPassed':True,'newThreeSyntheticScenariosCaptured':True,'localSignatureDutiesResumedQualified':False,'runtimeStarted':False,'networkQualified':False,'productionQualified':False})
except BaseException as e:
 save(resume/'terminal-failure.json',{'result':'FAIL','diagnostic':type(e).__name__+': '+str(e),'runtimeStarted':False,'productionQualified':False});raise
finally:
 save(resume/'source-after.json',{'revisionObserved':query(['git','rev-parse','HEAD']),'files':m.snap(before),'sameAsBefore':m.snap(before)==before,'driverSha256Observed':sha(__file__)})
