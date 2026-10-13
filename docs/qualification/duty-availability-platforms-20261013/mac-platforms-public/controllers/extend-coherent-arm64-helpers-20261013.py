import pathlib,subprocess,json,hashlib,time,xml.etree.ElementTree as ET
E=pathlib.Path(__file__).parent
ROOT=E/'duty-availability-coherent-arm64-helper-extension-20261013'
REPO=pathlib.Path('/Users/boris/Dev/blockchain-nano')
BUILD=E/'duty-availability-coherent-arm64-20261013/build'
DOCKER='/Users/boris/.docker/bin/docker'
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
TEST='tests/test_capella_duty_availability.cpp'
TEST_SHA='44febdb90047ea8a7de5e78002d73ed96b82cc67259dd356e98c7ba096f6647d'
HELPERS=['nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def save(name,value):(ROOT/name).write_text(json.dumps(value,indent=2)+'\n')
def inventory():
 paths=subprocess.check_output(['git','ls-files','src','include','cmake','third_party','CMakeLists.txt',TEST,'tests/capella/paused_capture.cpp','tests/capella/private_capture_budget.hpp','tests/capella/isolated_stopped_capture.cpp','tests/capella/isolated_bootstrap_audit.cpp'],cwd=REPO,text=True).splitlines()
 return {p:sha(REPO/p) for p in paths}
def parents():return {n:sha(BUILD/n) for n in ['libnano_core.a','nano','nano-capella-transition-tests','libnano-test-sync-fault.so']}
def run(name,args,timeout,memory='4g'):
 container='nano-duty-helpers-'+name+'-arm64-20261013'
 cmd=[DOCKER,'run','--rm','--pull=never','--platform','linux/arm64','--name',container,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--cpus','2','--memory',memory,'--pids-limit','256','--env','TMPDIR=/receipts/tmp','--mount',f'type=bind,source={REPO},target=/work,readonly','--mount',f'type=bind,source={BUILD},target=/outputs','--mount',f'type=bind,source={ROOT},target=/receipts','--entrypoint',args[0],IMAGE]+args[1:]
 save(name+'.command.json',{'args':cmd,'timeoutSeconds':timeout,'onlyExistingCoherentBuildMounted':True,'campaignVolumesMounted':False})
 print(json.dumps({'phase':name,'status':'started'}),flush=True);t=time.monotonic()
 try:
  with (ROOT/(name+'.log')).open('wb') as log:code=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,timeout=timeout).returncode
 except subprocess.TimeoutExpired:
  killed=subprocess.run([DOCKER,'kill',container],capture_output=True,text=True,timeout=20)
  save(name+'.timeout-stop.json',{'exitCode':killed.returncode,'scope':'only this extension builder/test container'});code=124
 save(name+'.exit.json',{'exitCode':code,'elapsedSeconds':time.monotonic()-t,'logSha256':sha(ROOT/(name+'.log'))})
 print(json.dumps({'phase':name,'exitCode':code}),flush=True)
 if code:raise RuntimeError(name+' failed; no automatic retry')
assert sha(REPO/TEST)==TEST_SHA
assert sha(REPO/'CMakeLists.txt')=='28fe657bbf1a54ad5663f64024d95f90eeb8a5e5fdd25943f0a684042e66f0ec'
assert subprocess.check_output(['git','diff','--name-only','HEAD'],cwd=REPO,text=True).splitlines()==[TEST]
assert not ROOT.exists();ROOT.mkdir(mode=0o700);(ROOT/'tmp').mkdir(mode=0o700)
before=inventory();p_before=parents()
assert p_before['libnano_core.a']=='161a3450c163aa324ca7c365beb028abb448fbd6912dfb34fcc26d8d230aae26'
assert p_before['nano']=='64ca2a4bf25e0d0bd82e6479f735070ca2cf2b43b756aa8b528d38169ebd0e19'
old_test=sha(BUILD/'nano-capella-duty-availability-tests')
old_object=sha(BUILD/'CMakeFiles/nano-capella-duty-availability-tests.dir/tests/test_capella_duty_availability.cpp.o')
assert all(not (BUILD/n).exists() for n in HELPERS)
save('source-before.json',before);save('parents-before.json',p_before)
save('scope.json',{'existingImageReused':True,'dockerImageBuilt':False,'existingCoherent139CoreBuildReused':True,'testTypingOnlyPatchSha256':TEST_SHA,'testBeforeSha256':old_test,'testObjectBeforeSha256':old_object,'helperBinariesAbsentBeforeBuild':True,'runtimeOrCampaignStarted':False,'productionQualified':False})
failure=None
try:
 run('build',['/usr/bin/cmake','--build','/outputs','--parallel','2','--target',*HELPERS,'nano-capella-duty-availability-tests'],240)
 new_test=sha(BUILD/'nano-capella-duty-availability-tests');new_object=sha(BUILD/'CMakeFiles/nano-capella-duty-availability-tests.dir/tests/test_capella_duty_availability.cpp.o')
 save('portable-test-binary-comparison.json',{'beforeExecutable':old_test,'afterExecutable':new_test,'beforeObject':old_object,'afterObject':new_object,'binaryByteIdentical':new_test==old_test,'objectByteIdentical':new_object==old_object,'oldCTestResultReusedOnlyIfBinaryIdentical':new_test==old_test})
 if new_test!=old_test:
  run('portable-availability',['/usr/bin/ctest','--test-dir','/outputs','-R','^consensus_capella_duty_projection_availability$','--verbose','--no-tests=error','--output-on-failure','--output-junit','/receipts/portable-availability.xml'],90,'2g')
  xml=ET.parse(ROOT/'portable-availability.xml').getroot();cases=xml.findall('testcase');assert len(cases)==1 and cases[0].get('status')=='run' and not xml.findall('.//skipped') and not xml.findall('.//failure')
 run('capture-guards',['/usr/bin/ctest','--test-dir','/outputs','-R','^consensus_isolated_stopped_capture_guards$','--verbose','--no-tests=error','--output-on-failure','--output-junit','/receipts/capture-guards.xml'],30,'2g')
 xml=ET.parse(ROOT/'capture-guards.xml').getroot();cases=xml.findall('testcase');assert len(cases)==1 and cases[0].get('status')=='run' and not xml.findall('.//skipped') and not xml.findall('.//failure')
 save('helper-binary-manifest.json',{n:sha(BUILD/n) for n in HELPERS})
 assert p_before==parents() and before==inventory()
except Exception as exc:failure=str(exc)
save('source-after.json',inventory());save('parents-after.json',parents())
save('terminal-receipt.json',{'passedWithinRequestedScope':failure is None,'failure':failure,'parentsUnchanged':p_before==parents(),'sourceUnchanged':before==inventory(),'oldAvailabilityTestReusedOnlyOnActualExecutableByteIdentity':failure is None and new_test==old_test,'captureGuardsPassed':failure is None,'runtimeOrPrivateCampaignStarted':False,'newImageBuilt':False,'productionQualified':False})
if failure:raise SystemExit(failure)
