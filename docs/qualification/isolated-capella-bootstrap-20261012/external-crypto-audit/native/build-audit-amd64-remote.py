from pathlib import Path
import hashlib, json, os, shlex, subprocess, time

ROOT = Path(__file__).parent
OUTPUT = ROOT / 'build'
SOURCE = ROOT / 'sources/private-binding-audit.cpp'
SOURCE_SHA = '25223c9adc0ca0afffa01a40ef5e0dcc9eee5cb4fc81d23daee65d77ef99a312'
IMAGE = 'fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
VOLUME = 'nano-clean-rpc-build-20261002'
REPO = '/home/ubuntu/blockchain-nano-git'
PARENT = '/build/coherent-e33ad89e-amd64'
CORE = '/build/publication-cancel-boundary-amd64-20261007/libnano_core.a'
CORE_SHA = '783ccae2127905741238dd58618124f9582aae380625ef3fe592523a58a056dd'
OUTPUT.mkdir(mode=0o700, exist_ok=False)
(OUTPUT / 'outputs').mkdir(mode=0o700)
assert OUTPUT.stat().st_uid == 1000 and OUTPUT.stat().st_gid == 1000
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
assert sha(SOURCE) == SOURCE_SHA

def save(name, value):
    (OUTPUT / name).write_text(json.dumps(value, indent=2) + '\n')

def prefix(write_output=False):
    args = ['sudo', '-n', 'docker', 'run', '--rm', '--pull=never', '--platform', 'linux/amd64',
            '--network', 'none', '--read-only', '--cap-drop', 'ALL', '--security-opt', 'no-new-privileges',
            '--user', '1000:1000', '--cpus', '1', '--memory', '2g', '--memory-swap', '2g', '--pids-limit', '128',
            '--mount', 'type=volume,source=' + VOLUME + ',target=/build,readonly',
            '--mount', 'type=bind,source=' + REPO + ',target=/work,readonly', '--workdir', PARENT]
    if write_output:
        args += ['--mount', 'type=bind,source=' + str(ROOT / 'sources') + ',target=/inputs,readonly',
                 '--mount', 'type=bind,source=' + str(OUTPUT / 'outputs') + ',target=/outputs']
    return [*args, IMAGE]

def capture(name, args):
    save(name + '-command.json', {'args': args})
    start = time.monotonic()
    result = subprocess.run(args, cwd=ROOT, text=True, capture_output=True, timeout=180,
                            env={k: v for k, v in os.environ.items() if not k.startswith('NANO_')})
    (OUTPUT / (name + '.log')).write_text(result.stdout + result.stderr)
    save(name + '-exit.json', {'exitCode': result.returncode, 'elapsedSeconds': time.monotonic() - start})
    assert result.returncode == 0, name
    return result.stdout

image = json.loads(capture('image-binding', ['sudo', '-n', 'docker', 'image', 'inspect', IMAGE]))[0]
assert image['Architecture'] == 'amd64' and image['Id'] == 'sha256:' + IMAGE
flags_text = capture('parent-flags', [*prefix(), 'cat', 'CMakeFiles/nano-capella-transition-tests.dir/flags.make'])
link_text = capture('parent-link', [*prefix(), 'cat', 'CMakeFiles/nano-capella-transition-tests.dir/link.txt'])
flags = {}
for line in flags_text.splitlines():
    if line.startswith(('CXX_DEFINES =', 'CXX_INCLUDES =', 'CXX_FLAGS =')):
        k, value = line.split('=', 1); flags[k.strip()] = shlex.split(value)
link_args = shlex.split(link_text)
assert link_args[0] == '/usr/bin/clang++' and link_args.count('libnano_core.a') == 1
old_object = 'CMakeFiles/nano-capella-transition-tests.dir/tests/test_capella_transition.cpp.o'
assert link_args.count(old_object) == 1
parent_files = {CORE, PARENT + '/nano', PARENT + '/nano-capella-transition-tests',
                PARENT + '/' + old_object, PARENT + '/CMakeFiles/nano-capella-transition-tests.dir/flags.make',
                PARENT + '/CMakeFiles/nano-capella-transition-tests.dir/link.txt'}
for a in link_args[1:]:
    if a.endswith(('.a', '.o', '.so')): parent_files.add(a if a.startswith('/') else PARENT + '/' + a)
parent_files = sorted(parent_files)

def parents(name):
    values = {}
    for line in capture(name, [*prefix(), 'sha256sum', *parent_files]).splitlines():
        digest, path = line.split('  ', 1); assert path not in values and len(digest) == 64; values[path] = digest
    assert sorted(values) == parent_files and values[CORE] == CORE_SHA
    save(name + '.json', values); return values

before = parents('parent-hashes-before')
compile_args = [link_args[0], *flags['CXX_DEFINES'], *flags['CXX_INCLUDES'], *flags['CXX_FLAGS'],
                '-c', '/inputs/private-binding-audit.cpp', '-o', '/outputs/private-binding-audit.o']
link_args = ['/outputs/private-binding-audit.o' if a == old_object else CORE if a == 'libnano_core.a' else a for a in link_args]
link_args[link_args.index('-o') + 1] = '/outputs/private-binding-audit'
save('source-binding.json', {'sourceSha256': SOURCE_SHA, 'imageId': image['Id'], 'qualifiedCoreArchive': CORE,
                            'qualifiedCoreSha256': CORE_SHA, 'parentCwd': PARENT, 'parentsReadOnly': True,
                            'productionCoreRebuilt': False, 'imageRebuilt': False, 'privateInputsRead': False,
                            'runtimeStarted': False, 'networkUsed': False})
for name, args in [('compile', compile_args), ('link', link_args)]:
    capture(name, [*prefix(True), *args]); print(name, '0', flush=True)
after = parents('parent-hashes-after')
assert before == after and sha(SOURCE) == SOURCE_SHA
save('binary-manifest.json', {'format': 'nano-amd64-private-binding-audit-build-v1', 'sourceSha256': SOURCE_SHA,
                              'helperSha256': sha(OUTPUT / 'outputs/private-binding-audit'),
                              'objectSha256': sha(OUTPUT / 'outputs/private-binding-audit.o'),
                              'qualifiedCoreSha256': CORE_SHA, 'parentFilesUnchanged': True,
                              'parentFilesCount': len(before), 'privateInputsRead': False,
                              'newSignaturesCreated': 0, 'journalCreated': False,
                              'runtimeStarted': False, 'networkQualified': False, 'productionQualified': False})
print('Built same helper AMD64 only; all parents unchanged; no private inputs or node', flush=True)
