from pathlib import Path
import subprocess,os,sys,json,hashlib,time,signal,shlex,shutil
repo=Path('/Users/boris/Dev/blockchain-nano')
evidence=Path(__file__).resolve().parent
root=evidence/'duty-availability-coherent-native-20261013'
old=evidence/'native-candidate/build'
revision='c672a30dbdc7b43ab04186db0e54be78510e6789'
cmake_pin='28fe657bbf1a54ad5663f64024d95f90eeb8a5e5fdd25943f0a684042e66f0ec'
test_pin='8690cc722e6642d14be1bc4b021eef9924be47e2b7e17d070a04908aa256ded2'
core_pins={
 'include/nano/consensus_altair_epoch.hpp':'668bedeb2334109d9cf19ba6fa3a77f5f6396dc484fed525e41e5abe7dc315ac',
 'src/consensus_altair_sync.cpp':'86e3ea6d1ee0c4b979ca076194d86da57f2dd54821a14c86391b3b91e3fedeeb',
 'include/nano/consensus_capella_duty.hpp':'b5000e20680212b38bf56457d74a557336928637349c650011671698f9570236',
 'src/consensus_capella_duty.cpp':'a08ed22c61ca6fa389b5d7e4304fee6772a2f9ee04e8e16ee2b1936bf18a0fff',
 'include/nano/consensus_capella_reactor.hpp':'b7b6a3f716945ffca5420d8cf60e9fd73df76974d45938eefaa6341ffe6d4cb4',
 'src/consensus_capella_sync_signing.cpp':'975b5072cbcf054fee2f88033763044825630616631f55e2fdfbe3287c3f5fad'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
def query(args,cwd=repo):return subprocess.check_output(args,cwd=cwd,text=True,timeout=30).strip()
def snap(paths):return {p:sha(repo/p) for p in paths}
def main():
 assert sys.platform=='darwin' and query(['/usr/bin/uname','-m'])=='arm64'
 assert query(['git','rev-parse','HEAD'])==revision
 assert query(['git','diff','--name-only','HEAD','--','src','include','third_party','cmake','network','CMakeLists.txt','tests/test_capella_transition.cpp','tests/test_capella_duty_availability.cpp','tests/support','tests/capella/paused_capture.cpp','tests/capella/isolated_stopped_capture.cpp','tests/capella/isolated_bootstrap_audit.cpp'])==''
 assert sha(repo/'CMakeLists.txt')==cmake_pin and sha(repo/'tests/test_capella_duty_availability.cpp')==test_pin
 assert all(sha(repo/p)==h for p,h in core_pins.items())
 # Names only: no campaign values or process environment is archived.
 unsafe=[n for n in os.environ if n.startswith(('NANO_','DYLD_')) or n=='LD_PRELOAD']
 assert not unsafe,'Unexpected runtime-influencing environment names: '+','.join(unsafe)
 root.mkdir(mode=0o700);build=root/'build';build.mkdir(mode=0o700)
 st=build.stat();assert list(build.iterdir())==[]
 save(root/'fresh-build-empty-proof.json',{'rootCreatedAtomically':True,'buildCreatedAtomically':True,'entriesBeforeConfigure':0,'device':st.st_dev,'inode':st.st_ino,'uid':st.st_uid,'mode':oct(st.st_mode&0o777),'ctimeNs':st.st_ctime_ns,'mtimeNs':st.st_mtime_ns})
 logs=root/'logs';logs.mkdir(mode=0o700);temp=root/'test-tmp';temp.mkdir(mode=0o700)
 tracked=query(['git','ls-files','src','include','third_party','CMakeLists.txt','cmake','network','tests/test_capella_transition.cpp','tests/test_capella_duty_availability.cpp','tests/support','tests/capella/paused_capture.cpp','tests/capella/isolated_stopped_capture.cpp','tests/capella/isolated_bootstrap_audit.cpp']).splitlines()
 source_paths=sorted(p for p in tracked if (repo/p).is_file());before=snap(source_paths);driver_before=sha(__file__)
 old_names=['libnano_core.a','nano','nano-capella-transition-tests'];old_pins={n:sha(old/n) for n in old_names}
 save(root/'source-before.json',{'revision':revision,'fileCount':len(before),'scope':'974-Git-tracked-production+network+two-tests+three-capture-helpers+public-test-support','files':before,'sixReviewedSources':core_pins,'testSha256':test_pin,'CMakeSha256':cmake_pin,'driverSha256':sha(__file__)})
 save(root/'old-native-artifacts-before.json',old_pins)
 canonical=query(['git','ls-files','src','include','third_party','CMakeLists.txt','cmake']).splitlines();assert len(canonical)==974
 arm_scope=json.loads((evidence/'duty-availability-coherent-arm64-20261013/receipts/source-before.json').read_text());ignored=sorted(set(arm_scope)-set(canonical));assert len(ignored)==11 and not(set(canonical)-set(arm_scope))
 save(root/'source-scope-reconciliation.json',{'canonicalTrackedProductionFiles':len(canonical),'extendedSourceScopeFiles':len(before),'priorArmUnconsumedIgnoredExtras':ignored,'trackedProductionNotInArmScope':[]})
 fixtures=repo/'tests/fixtures';names=['consensus-capella-slots-official/manifest.json','consensus-slashing-official/manifest.json','consensus-altair-state-roots-official/manifest.json','consensus-altair-state-roots-official/minimal-altair.yaml','consensus-bellatrix-containers-official/manifest.json','consensus-bellatrix-containers-official/minimal-bellatrix.yaml','consensus-capella-withdrawals-official/minimal-capella.yaml','consensus-altair-fork-head-official/manifest.json','consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml']
 manifest=json.loads((fixtures/'consensus-capella-slots-official/manifest.json').read_text());row=next(x for x in manifest['cases'] if x['name']=='slots_1');names+=['consensus-capella-slots-official/slots_1/'+n for n in row['files']]
 for group in ['consensus-slashing-official','consensus-bellatrix-containers-official']:
  m=json.loads((fixtures/group/'manifest.json').read_text());names += [group+'/'+n for n in m.get('referenceFiles',{})];names += [group+'/'+x['name'] for x in m.get('presets',[])]
 fixture_pins={n:sha(fixtures/n) for n in sorted(set(names))};save(root/'public-fixture-before.json',fixture_pins)
 cache={}
 for line in (old/'CMakeCache.txt').read_text().splitlines():
  if ':' in line and '=' in line and not line.startswith(('//','#')):
   name,value=line.split('=',1);cache[name.split(':',1)[0]]=value
 dep_names=['BOOST_INCLUDE_DIR','ROCKSDB_INCLUDE_DIR','ROCKSDB_LIBRARY','GMP_INCLUDE_DIR','GMP_LIBRARY','GMPXX_LIBRARY','ICU_INCLUDE_DIR','ICU_UC_LIBRARY_RELEASE','OPENSSL_INCLUDE_DIR','OPENSSL_SSL_LIBRARY','OPENSSL_CRYPTO_LIBRARY','CMAKE_C_COMPILER','CMAKE_CXX_COMPILER']
 deps={n:cache[n] for n in dep_names};assert deps['CMAKE_CXX_COMPILER']=='/usr/bin/c++' and deps['CMAKE_C_COMPILER']=='/usr/bin/cc'
 libs={v:{'resolved':str(Path(v).resolve()),'sha256':sha(Path(v).resolve())} for n,v in deps.items() if n.endswith('LIBRARY') or n=='ICU_UC_LIBRARY_RELEASE'}
 save(root/'native-dependencies-before.json',{'cacheSource':str(old/'CMakeCache.txt'),'cacheSha256':sha(old/'CMakeCache.txt'),'paths':deps,'resolvedLibraryBindings':libs,'compilerVersion':query(['/usr/bin/c++','--version']),'cmakeVersion':query(['/opt/homebrew/bin/cmake','--version']),'uname':query(['/usr/bin/uname','-a']),'sdk':query(['/usr/bin/xcrun','--show-sdk-path']),'unsafeEnvironmentNamesObserved':unsafe,'noInstallOrUpgrade':True})
 env=dict(os.environ);env['TMPDIR']=str(temp)+'/'
 def run(name,args,timeout,cwd=repo):
  save(logs/(name+'-command.json'),{'args':args,'cwd':str(cwd),'timeoutSeconds':timeout,'temporaryDirectory':str(temp)})
  start=time.monotonic()
  with (logs/(name+'.log')).open('wb') as log:
   child=subprocess.Popen(args,cwd=cwd,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
   save(logs/(name+'-status.json'),{'state':'running','pid':child.pid,'processGroup':child.pid,'startUtc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'timeoutSeconds':timeout})
   print(json.dumps({'phase':name,'state':'running','pid':child.pid}),flush=True)
   expired=False
   try:code=child.wait(timeout=timeout)
   except subprocess.TimeoutExpired:
    expired=True
    os.killpg(child.pid,signal.SIGTERM)
    try:code=child.wait(timeout=5)
    except subprocess.TimeoutExpired:
     os.killpg(child.pid,signal.SIGKILL);code=child.wait(timeout=5)
   group_gone=True
   if expired:
    try:os.killpg(child.pid,0);group_gone=False
    except ProcessLookupError:pass
   terminal={'state':'failed-timeout' if expired else 'terminal','pid':child.pid,'exitCode':code,'timedOut':expired,'allManagedDescendantsStopped':group_gone if expired else None,'elapsedSeconds':time.monotonic()-start}
   save(logs/(name+'-exit.json'),terminal);save(logs/(name+'-status.json'),terminal);print(json.dumps({'phase':name,**terminal}),flush=True)
   if expired or code:raise RuntimeError('phase '+name+' failed; no automatic retry')
 try:
  run('configure',['/opt/homebrew/bin/cmake','-S',str(repo),'-B',str(build),'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','-DMCL_PORTABLE=ON',*['-D'+n+'='+v for n,v in deps.items()]],120)
  shutil.copyfile(build/'compile_commands.json',root/'compile_commands.original.json')
  commands=json.loads((build/'compile_commands.json').read_text());core=[x for x in commands if 'CMakeFiles/nano_core.dir/' in x.get('command','')]
  expected=sorted(str(p) for p in (repo/'src').glob('*.cpp') if p.name!='main.cpp');assert len(core)==139 and sorted(x['file'] for x in core)==expected
  save(root/'core-139-compile-source-audit-before.json',{'units':len(core),'allExpectedCoreUnitsPresentExactlyOnce':True,'compileCommandsSha256':sha(root/'compile_commands.original.json'),'files':[{'source':x['file'],'sha256':sha(x['file']),'command':x['command'],'directory':x['directory']} for x in core]})
  run('build',['/opt/homebrew/bin/cmake','--build',str(build),'--parallel','2','--target','nano_core','nano','nano-capella-duty-availability-tests','nano-capella-transition-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit'],1200)
  assert snap(source_paths)==before and {n:sha(old/n) for n in old_names}==old_pins
  generated=root/'generated-compile-link';generated.mkdir()
  for target in ['nano_core','nano','nano-capella-duty-availability-tests','nano-capella-transition-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit','nano-test-sync-fault']:
   targetdir=generated/target;targetdir.mkdir()
   for n in ['flags.make','link.txt','DependInfo.cmake']:shutil.copyfile(build/'CMakeFiles'/(target+'.dir')/n,targetdir/n)
  objects=[]
  for x in core:
   args=shlex.split(x['command']);obj=Path(args[args.index('-o')+1]);obj=obj if obj.is_absolute() else Path(x['directory'])/obj
   assert obj.is_file() and obj.stat().st_mtime_ns>=st.st_ctime_ns
   objects.append({'source':x['file'],'object':str(obj.relative_to(build)),'objectSha256':sha(obj),'mtimeNs':obj.stat().st_mtime_ns})
  save(root/'core-139-fresh-object-audit.json',{'freshEmptyBuildObserved':True,'units':len(objects),'all139ObjectsWrittenAfterFreshBuildCreation':True,'objects':objects})
  archive_entries=query(['/usr/bin/ar','-t',str(build/'libnano_core.a')]).splitlines();archive_units=[n for n in archive_entries if n not in ['__.SYMDEF','__.SYMDEF SORTED']];assert len(archive_units)==139 and sorted(archive_units)==sorted(Path(x['file']).name+'.o' for x in core)
  selected_targets=['nano_core','nano','nano-capella-duty-availability-tests','nano-capella-transition-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit','nano-test-sync-fault']
  consumed=[]
  for target in selected_targets:
   consumed += list((build/'CMakeFiles'/(target+'.dir')).rglob('*.o.d'))
   consumed += [build/'CMakeFiles'/(target+'.dir')/'link.txt']
  texts=[f.read_text() for f in consumed];assert not any(extra in text or str(repo/extra) in text for extra in ignored for text in texts)
  save(root/'prior-arm-ignored-inputs-nonconsumption-audit.json',{'ignoredPaths':ignored,'allAbsentFromSelectedTargetCompilerDependenciesAndLinks':True,'selectedTargetDependencyFilesChecked':len(consumed),'archiveUnits':archive_units,'archiveUnitCount':139,'bsdArchiveSymbolTables':[n for n in archive_entries if n not in archive_units],'globalUnbuiltTargetsNotClaimedAudited':True})
  binary_names=['libnano_core.a','nano','nano-capella-duty-availability-tests','nano-capella-transition-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit','libnano-test-sync-fault.dylib'];binaries={n:sha(build/n) for n in binary_names}
  save(root/'binary-manifest.json',{'revision':revision,'CMakeSha256':cmake_pin,'sixSourcePins':core_pins,'testSha256':test_pin,'sourcesUnchanged':True,'files':binaries,'oldArtifactsUnchanged':True,'runtimeQualified':False,'productionQualified':False})
  run('native-artifact-metadata',['/usr/bin/file',*[str(build/n) for n in binary_names]],30)
  run('native-dynamic-dependencies',['/usr/bin/otool','-L',*[str(build/n) for n in binary_names if n!='libnano_core.a']],30)
  run('ctest-registration',['/opt/homebrew/bin/ctest','--test-dir',str(build),'--show-only=json-v1','-R','^(consensus_isolated_stopped_capture_guards|consensus_capella_duty_projection_availability|consensus_capella_local_sync)$'],30)
  discovery=json.loads((logs/'ctest-registration.log').read_text());assert [t['name'] for t in discovery['tests']]==['consensus_isolated_stopped_capture_guards','consensus_capella_duty_projection_availability','consensus_capella_local_sync']
  run('ctest-targeted',['/opt/homebrew/bin/ctest','--test-dir',str(build),'--output-on-failure','-V','--output-junit',str(root/'targeted-junit.xml'),'-R','^(consensus_isolated_stopped_capture_guards|consensus_capella_duty_projection_availability|consensus_capella_local_sync)$'],360)
  run('prepared-block',[str(build/'nano-capella-transition-tests'),'--prepared-block'],120)
  assert snap(source_paths)==before and {n:sha(old/n) for n in old_names}==old_pins and {n:sha(build/n) for n in binary_names}==binaries and sha(__file__)==driver_before
  assert {n:sha(fixtures/n) for n in fixture_pins}==fixture_pins
  assert all(sha(Path(v).resolve())==libs[v]['sha256'] for v in libs)
  save(root/'terminal-qualification.json',{'format':'nano-duty-availability-coherent-darwin-arm64-qualification-v1','result':'PASS','revision':revision,'CMakeSha256':cmake_pin,'sixSourcePins':core_pins,'testSha256':test_pin,'all139CoreUnitsFreshCompiled':True,'sourceFileCount':len(before),'sourceBeforeAfterIdentical':True,'fixtureFilesBeforeAfterIdentical':True,'dependenciesBeforeAfterIdentical':True,'faultDylibBeforeAfterIdentical':True,'driverBeforeAfterIdentical':True,'oldNativeArtifactsBeforeAfterIdentical':True,'newBinaryFiles':binaries,'threeSyntheticScenariosInRegisteredCtest':True,'stoppedCapturePureGuardsRegisteredCtestPassed':True,'localSyncRegisteredCtestPassed':True,'preparedBlockDirectPassed':True,'localSignatureDutiesResumedQualified':False,'networkQualified':False,'runtimeStarted':False,'productionQualified':False})
 except BaseException as ex:
  save(root/'terminal-failure.json',{'result':'FAIL','diagnostic':type(ex).__name__+': '+str(ex),'sourceUnchanged':snap(source_paths)==before,'oldNativeArtifactsUnchanged':{n:sha(old/n) for n in old_names}==old_pins,'runtimeStarted':False,'productionQualified':False});raise
 finally:
  save(root/'source-after.json',{'revisionObserved':query(['git','rev-parse','HEAD']),'files':snap(source_paths),'sameAsBefore':snap(source_paths)==before})
  save(root/'old-native-artifacts-after.json',{n:sha(old/n) for n in old_names})
  save(root/'public-fixture-after.json',{n:sha(fixtures/n) for n in fixture_pins})
if __name__=='__main__':main()
