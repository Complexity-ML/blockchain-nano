import hashlib, json, os, pathlib, stat, subprocess, time, uuid

E = pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
P = E / 'checkpoint-blob-gc-runtime-packaging-proposal-20261013'
Q = E / 'checkpoint-blob-gc-arm-qualification-20261013'
def sha(b): return hashlib.sha256(b).hexdigest()
def pinned(p, expected):
    p = pathlib.Path(p)
    assert p.resolve() == p and not p.is_symlink()
    a = p.stat(); b = p.read_bytes(); z = p.stat()
    assert a == z and stat.S_ISREG(a.st_mode) and sha(b) == expected
    return b
def save(name, value):
    with (P / name).open('x') as f:
        json.dump(value, f, indent=2); f.write('\n'); f.flush(); os.fsync(f.fileno())

assert __debug__ and os.geteuid() == 501
os.umask(0o077)
plan = json.loads(pinned(P / 'configure-only-plan.json', '295e6dbd57d889ce42471ab7a7b2c77a80ccd002c9d9ca5dea92a03189d346e0'))
manifest = json.loads(pinned(P / 'manifest.json', 'a6272df29886b1a0d322766a5401e7f5aa0fff28f73e71be0a25190950d8df16'))
index = json.loads(pinned(Q / 'source-after-testfix2.json', 'e89d6eab8eca7faf8492b26c8971e4a566ce159659fb3835c31ef5beb7e054f8'))
source = pathlib.Path(plan['sourceFullTreeToBePreparedByRoot'])
build = pathlib.Path(plan['freshBuildOutput'])
assert source.parent == P and build.parent == P and not source.exists() and not build.exists()
source.mkdir(mode=0o700); build.mkdir(mode=0o700)
overlays = {r['path']: r for r in manifest['sourceFiles']}
assert set(overlays) == {'CMakeLists.txt', 'Dockerfile'}
before = {}
for r in index['files']:
    rel = pathlib.PurePosixPath(r['path'])
    assert not rel.is_absolute() and '..' not in rel.parts
    b = pinned(Q / 'source-testfix2' / rel, r['sha256'])
    if r['path'] in overlays:
        overlay = overlays[r['path']]
        assert r['sha256'] == overlay['beforeSha256']
        b = pinned(P / 'source' / rel, overlay['sha256'])
    target = source / rel
    target.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    with target.open('xb') as f: f.write(b)
    target.chmod(r.get('mode', 0o600) & 0o777)
    before[r['path']] = sha(b)
assert len(before) == 9411 and not list(build.iterdir())
save('configure-input-receipt.json', {'sourceFiles':before,'emptyOutputObserved':True,'privateFilesCopied':False})
command = list(plan['command'])
name = 'nano-gc-packaging-' + uuid.uuid4().hex[:16]
command[command.index('--name')+1] = name
assert command[-1] == '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'
save('configure-command.json', {'argv':command,'timeoutSeconds':180,'imageBuilt':False})
start = time.monotonic()
try:
    with (P / 'configure.log').open('xb') as f:
        result = subprocess.run(command, stdout=f, stderr=subprocess.STDOUT, timeout=180)
except subprocess.TimeoutExpired:
    subprocess.run([command[0], 'rm', '-f', name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    save('configure-terminal.json', {'exitCode':None,'timeout':True,'productionQualified':False})
    raise
save('configure-exit.json', {'exitCode':result.returncode,'elapsedSeconds':time.monotonic()-start})
assert result.returncode == 0
current = json.loads((build / 'compile_commands.json').read_bytes())
baseline = json.loads(pinned(plan['compileDatabaseBaseline']['path'], plan['compileDatabaseBaseline']['sha256']))
selected = lambda rows: [r for r in rows if '/nano_core.dir/' in r['output'] or any('/'+n+'.dir/' in r['output'] for n in ['nano','nano-capella-transition-tests','nano-storage-maintenance','nano-checkpoint-blob-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit'])]
assert len(selected(current)) == len(selected(baseline)) == 147
metadata = []
for r in plan['generatedMetadataExpectedUnchanged']:
    old = pinned(r['baselinePath'], r['sha256'])
    new = (build / r['relativePath']).read_bytes()
    metadata.append({'path':r['relativePath'],'beforeSha256':sha(old),'afterSha256':sha(new),'unchanged':old==new})
install = (build / 'cmake_install.cmake').read_text()
assert all(s in install for s in plan['installScriptRequired'])
for rel, expected in before.items(): assert sha((source / rel).read_bytes()) == expected
core_targets = [build / 'CMakeFiles/nano_core.dir'] + [build / 'CMakeFiles' / (n+'.dir') for n in ['nano','nano-capella-transition-tests','nano-storage-maintenance','nano-checkpoint-blob-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit']]
assert not any(list(d.rglob('*.o')) for d in core_targets)
unchanged = selected(current) == selected(baseline) and all(r['unchanged'] for r in metadata)
save('configure-terminal.json', {'exitCode':0,'selectedCompilerRows':147,'selectedCompilerCommandsUnchanged':selected(current)==selected(baseline),'fullCompileDatabaseUnchanged':current==baseline,'generatedMetadata':metadata,'installTargetsPresent':['nano','nano-storage-maintenance'],'nanoObjectsBuilt':0,'testsExecuted':0,'runtimeImageBuilt':False,'binariesInstalled':False,'sourceInputsUnchangedDuringConfigure':True,'packagingMetadataQualified':unchanged,'productionQualified':False})
assert unchanged, 'Configure metadata differs; preserve and inspect, do not mask'
print(json.dumps({'configureExitCode':0,'packagingMetadataQualified':True,'compilerRowsUnchanged':147,'nanoObjectsBuilt':0,'testsExecuted':0,'runtimeImageBuilt':False}))
