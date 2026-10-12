import hashlib
import json
import pathlib
import subprocess
import time

ROOT = pathlib.Path(__file__).resolve().parent
REPO = pathlib.Path('/Users/boris/Dev/blockchain-nano')
IMAGE = 'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
BINDING = ROOT.parent / 'new-duty-projection-availability-counterexample-20261013/candidate-preparation/postready-sync-extension/final-six-source-binding.json'
DOCKER = '/Users/boris/.docker/bin/docker'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def inventory():
    out = {}
    for prefix in ('src', 'include', 'cmake', 'third_party'):
        for path in (REPO / prefix).rglob('*'):
            if path.is_file() and not path.is_symlink():
                out[str(path.relative_to(REPO))] = sha(path)
    out['CMakeLists.txt'] = sha(REPO / 'CMakeLists.txt')
    return out

def write(name, value):
    (ROOT / 'receipts' / name).write_text(json.dumps(value, indent=2) + '\n')

def run(name, args, timeout):
    command = [DOCKER, 'run', '--rm', '--pull=never', '--platform', 'linux/arm64',
        '--name', 'nano-duty-availability-' + name + '-arm64-20261013',
        '--network', 'none', '--read-only', '--cap-drop', 'ALL',
        '--security-opt', 'no-new-privileges', '--user', '501:20',
        '--cpus', '2', '--memory', '4g', '--env', 'TMPDIR=/outputs/tmp',
        '--mount', f'type=bind,source={REPO},target=/work,readonly',
        '--mount', f'type=bind,source={ROOT / "build"},target=/outputs',
        '--entrypoint', args[0], IMAGE] + args[1:]
    write(name + '.command.json', {'args': command, 'timeoutSeconds': timeout})
    start = time.monotonic()
    log = ROOT / 'receipts' / (name + '.log')
    disposition = None
    with log.open('wb') as stream:
        try:
            proc = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT, timeout=timeout)
            code = proc.returncode
        except subprocess.TimeoutExpired:
            code = 124
            disposition = 'collector-timeout-no-success-claimed'
            cleanup = subprocess.run([DOCKER, 'kill', 'nano-duty-availability-' + name + '-arm64-20261013'],
                capture_output=True, text=True, timeout=15)
            write(name + '.timeout-cleanup.json', {'exitCode': cleanup.returncode,
                'stdout': cleanup.stdout, 'stderr': cleanup.stderr,
                'scope': 'only this fresh build container; no validator container'})
    write(name + '.exit.json', {'exitCode': code, 'elapsedSeconds': time.monotonic() - start,
        'logSha256': sha(log), 'disposition': disposition})
    print(json.dumps({'phase': name, 'exitCode': code}), flush=True)
    if code:
        raise RuntimeError('failed phase: ' + name)

# Refuse stale objects/cache even if an old or incomplete run left no receipt.
# Existing evidence is preserved; this driver never cleans or silently reuses it.
for name in ('build', 'receipts'):
    if (ROOT / name).exists() or (ROOT / name).is_symlink():
        raise SystemExit('fresh candidate output required: ' + name)
for name in ('build', 'build/tmp', 'receipts'):
    (ROOT / name).mkdir(mode=0o700, exist_ok=False)
binding = json.loads(BINDING.read_text())
for item in binding['files']:
    if sha(REPO / item['path']) != item['sha256']:
        raise SystemExit('candidate source differs: ' + item['path'])
if sha(REPO / 'CMakeLists.txt') != '20bff51087f0772434eac798fec4bf1eda4ca6b57c4b2b036d4ae2a24310f8f5':
    raise SystemExit('unreviewed CMake change before build')
before = inventory()
write('source-before.json', before)
write('scope.json', {'format': 'nano-duty-availability-coherent-build-scope-v1',
    'image': IMAGE, 'sourceBindingSha256': sha(BINDING), 'sources': len(before),
    'sourceMountedReadOnly': True, 'network': 'none', 'targets': ['nano_core', 'nano'],
    'fullFreshCoreBuildRequiredByChangedClassLayout': True,
    'existingImageReused': True, 'dockerImageBuildRequested': False,
    'runtimeOrValidatorStarted': False, 'vpsAccessed': False})
failure = None
try:
    run('configure', ['/usr/bin/cmake', '-S', '/work', '-B', '/outputs', '-G', 'Unix Makefiles',
        '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_C_COMPILER=/usr/bin/clang',
        '-DCMAKE_CXX_COMPILER=/usr/bin/clang++', '-DMCL_PORTABLE=ON',
        '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'], 180)
    run('build', ['/usr/bin/cmake', '--build', '/outputs', '--parallel', '2', '--target', 'nano_core', 'nano'], 1200)
    run('hashes', ['/usr/bin/sha256sum', '/outputs/libnano_core.a', '/outputs/nano'], 30)
except Exception as error:
    failure = str(error)
after = inventory()
write('source-after.json', after)
same = before == after
write('terminal-receipt.json', {'format': 'nano-duty-availability-coherent-arm64-build-v1',
    'passedWithinBuildScope': failure is None and same, 'failure': failure,
    'allProductionSourcesUnchangedDuringBuild': same, 'sources': len(before),
    'image': IMAGE, 'fullFreshCoreAndNanoBuild': failure is None,
    'oldCoreArchiveMixedWithNewLayout': False, 'testsExecuted': 0,
    'runtimeStarted': False, 'vpsAccessed': False, 'productionQualified': False})
if failure or not same:
    raise SystemExit(failure or 'production source changed during build')
