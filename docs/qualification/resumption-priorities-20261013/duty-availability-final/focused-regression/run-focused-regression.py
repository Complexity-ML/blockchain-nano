import hashlib
import json
import pathlib
import subprocess
import time

ROOT = pathlib.Path(__file__).resolve().parent
REPO = pathlib.Path('/Users/boris/Dev/blockchain-nano')
OUT = ROOT / 'focused-regression'
OUT.mkdir(mode=0o700, exist_ok=False)
IMAGE = 'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
DOCKER = '/Users/boris/.docker/bin/docker'

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

before = {str(p.relative_to(ROOT)): sha(p) for p in (ROOT/'build/libnano_core.a', ROOT/'build/nano')}
sources = json.loads((ROOT/'receipts/source-after.json').read_text())
for name, expected in sources.items():
    assert sha(REPO/name) == expected, 'production source changed: ' + name
test_inputs = {str(p.relative_to(REPO)):sha(p) for p in (REPO/'tests').rglob('*')
    if p.is_file() and p.suffix in ('.cpp','.hpp','.h')}

def write(name, value):
    (OUT/name).write_text(json.dumps(value, indent=2)+'\n')

def run(name, argv, timeout):
    command = [DOCKER,'run','--rm','--pull=never','--platform','linux/arm64',
        '--name','nano-duty-focused-'+name+'-arm64-20261013',
        '--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges',
        '--user','501:20','--cpus','2','--memory','4g','--env','TMPDIR=/outputs/tmp',
        '--mount',f'type=bind,source={REPO},target=/work,readonly',
        '--mount',f'type=bind,source={ROOT/"build"},target=/outputs',
        '--entrypoint',argv[0],IMAGE]+argv[1:]
    write(name+'.command.json',{'args':command,'timeoutSeconds':timeout})
    start=time.monotonic()
    log=OUT/(name+'.log')
    with log.open('wb') as f:
        try:
            p=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
            code=p.returncode
        except subprocess.TimeoutExpired:
            code=124
            c=subprocess.run([DOCKER,'kill','nano-duty-focused-'+name+'-arm64-20261013'],capture_output=True,text=True,timeout=15)
            write(name+'.timeout-cleanup.json',{'exitCode':c.returncode,'stdout':c.stdout,'stderr':c.stderr})
    write(name+'.exit.json',{'exitCode':code,'elapsedSeconds':time.monotonic()-start,'logSha256':sha(log)})
    print(json.dumps({'phase':name,'exitCode':code}),flush=True)
    if code:raise RuntimeError('failed phase: '+name)

failure=None
write('source-before.json',sources)
write('test-source-before.json',test_inputs)
write('parent-binary-before.json',before)
try:
    run('build-transition-tests',['/usr/bin/cmake','--build','/outputs','--parallel','2','--target','nano-capella-transition-tests'],240)
    run('ctest-local-sync',['/usr/bin/ctest','--test-dir','/outputs','-R','^consensus_capella_local_sync$',
        '--output-on-failure','--no-tests=error','--output-junit','/outputs/duty-local-sync.xml'],260)
    run('prepared-block',['/outputs/nano-capella-transition-tests','--prepared-block'],90)
except Exception as e:
    failure=str(e)
same_sources=all(sha(REPO/name)==value for name,value in sources.items())
same_test_sources=all(sha(REPO/name)==value for name,value in test_inputs.items())
after={name:sha(ROOT/name) for name in before}
write('parent-binary-after.json',after)
program=ROOT/'build/nano-capella-transition-tests'
fault=ROOT/'build/libnano-test-sync-fault.so'
write('terminal-receipt.json',{'format':'nano-duty-availability-focused-regression-v1',
    'passedWithinScope':failure is None and same_sources and same_test_sources and before==after,
    'failure':failure,'productionSourcesUnchanged':same_sources,'existingTestSourcesUnchanged':same_test_sources,
    'coherentCoreAndNanoUnchanged':before==after,'testBinarySha256':sha(program) if program.exists() else None,
    'faultLibrarySha256':sha(fault) if fault.exists() else None,
    'selection':['consensus_capella_local_sync','direct --prepared-block'],
    'newAvailabilityCounterexampleQualified':False,'vpsAccessed':False,'liveRuntimeStarted':False,
    'productionQualified':False})
if failure or not same_sources or not same_test_sources or before!=after:
    raise SystemExit(failure or 'source or parent changed during focused regression')
