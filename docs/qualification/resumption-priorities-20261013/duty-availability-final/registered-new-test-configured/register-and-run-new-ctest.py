import hashlib,json,pathlib,subprocess,time
ROOT=pathlib.Path(__file__).resolve().parent
REPO=pathlib.Path('/Users/boris/Dev/blockchain-nano')
OUT=ROOT/'registered-new-test-configured';OUT.mkdir(mode=0o700,exist_ok=False)
BUILD=ROOT/'build'
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
DOCKER='/Users/boris/.docker/bin/docker'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=REPO/'tests/test_capella_duty_availability.cpp'
assert sha(source)=='8690cc722e6642d14be1bc4b021eef9924be47e2b7e17d070a04908aa256ded2'
core_before={'libnano_core.a':sha(BUILD/'libnano_core.a'),'nano':sha(BUILD/'nano')}
assert core_before=={'libnano_core.a':'161a3450c163aa324ca7c365beb028abb448fbd6912dfb34fcc26d8d230aae26','nano':'64ca2a4bf25e0d0bd82e6479f735070ca2cf2b43b756aa8b528d38169ebd0e19'}
def write(n,x):(OUT/n).write_text(json.dumps(x,indent=2)+'\n')
def run(name,argv,timeout,readonly=False):
    command=[DOCKER,'run','--rm','--pull=never','--platform','linux/arm64','--name','nano-duty-ctest-'+name+'-arm64-20261013',
        '--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20',
        '--cpus','2','--memory','2g','--env','TMPDIR=/receipts/tmp',
        '--mount',f'type=bind,source={REPO},target=/work,readonly',
        '--mount',f'type=bind,source={BUILD},target=/outputs'+(',readonly' if readonly else ''),
        '--mount',f'type=bind,source={OUT},target=/receipts','--entrypoint',argv[0],IMAGE]+argv[1:]
    write(name+'.command.json',{'args':command,'timeoutSeconds':timeout,'buildReadOnly':readonly})
    start=time.monotonic();log=OUT/(name+'.log')
    with log.open('wb') as f:
        try:p=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT,timeout=timeout);code=p.returncode
        except subprocess.TimeoutExpired:
            code=124;c=subprocess.run([DOCKER,'kill','nano-duty-ctest-'+name+'-arm64-20261013'],capture_output=True,text=True,timeout=15)
            write(name+'.timeout-cleanup.json',{'exitCode':c.returncode,'stdout':c.stdout,'stderr':c.stderr})
    write(name+'.exit.json',{'exitCode':code,'elapsedSeconds':time.monotonic()-start,'logSha256':sha(log)})
    print(json.dumps({'phase':name,'exitCode':code}),flush=True)
    if code:raise RuntimeError(name+' failed')
(OUT/'tmp').mkdir(mode=0o700)
write('source-before.json',{'testSha256':sha(source),'cmakeSha256':sha(REPO/'CMakeLists.txt'),'parents':core_before})
failure=None
try:
    run('configure-new-test',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles',
        '-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang',
        '-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],120)
    run('build-new-test',['/usr/bin/cmake','--build','/outputs','--parallel','2','--target','nano-capella-duty-availability-tests'],240)
    run('registration',['/usr/bin/ctest','--test-dir','/outputs','--show-only=json-v1','-R','^consensus_capella_duty_projection_availability$'],20)
    registry=json.loads((OUT/'registration.log').read_text());assert len(registry['tests'])==1
    t=registry['tests'][0];properties={p['name']:p['value'] for p in t['properties']}
    assert t['name']=='consensus_capella_duty_projection_availability'
    assert t['command']==['/outputs/nano-capella-duty-availability-tests','--self-test']
    assert properties['TIMEOUT']==70 and properties['LABELS']==['qualification_consensus_duty_availability']
    assert not any(p in properties for p in ('DISABLED','SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION','WILL_FAIL','PASS_REGULAR_EXPRESSION'))
    program=BUILD/'nano-capella-duty-availability-tests';write('program-before.json',{'sha256':sha(program),'bytes':program.stat().st_size})
    run('ctest',['/usr/bin/ctest','--test-dir','/outputs','-R','^consensus_capella_duty_projection_availability$',
        '--verbose','--output-on-failure','--no-tests=error','--output-junit','/receipts/new-test.xml'],90)
    write('program-after.json',{'sha256':sha(program),'bytes':program.stat().st_size})
except Exception as e:failure=str(e)
after={name:sha(BUILD/name) for name in core_before}
same_test=sha(source)=='8690cc722e6642d14be1bc4b021eef9924be47e2b7e17d070a04908aa256ded2'
write('terminal-receipt.json',{'format':'nano-duty-availability-registered-ctest-v1','passedWithinScope':failure is None and core_before==after and same_test,
    'failure':failure,'parentHashesBefore':core_before,'parentHashesAfter':after,'parentsUnchanged':core_before==after,
    'newTestSourceUnchanged':same_test,'manualAggregateTestPreviouslyRun':False,'onlyNewRegisteredCTestRun':True,
    'fullDailySelectionExecuted':False,'vpsAccessed':False,'productionQualified':False})
if failure or core_before!=after or not same_test:raise SystemExit(failure or 'source/parent changed')
