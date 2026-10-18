#!/usr/bin/env python3
"""Run only the eight new independent proof-verifier unit tests, offline."""
from pathlib import Path
import hashlib
import json
import os
import stat
import subprocess
import time

E = Path(__file__).parent
R = Path('/Users/boris/Dev/blockchain-nano')
S = E / 'rpc-eip1186-account-storage-proof-source-proposal-20261017/source'
OUT = E / 'rpc-proof-verifier-js-root-20261017-01'
NODE = Path('/opt/homebrew/Cellar/node/25.9.0_2/bin/node')
sources = {
    'tests/qualification/rpc-proof.mjs': S / 'tests/qualification/rpc-proof.mjs',
    'tests/qualification/rpc-proof.test.mjs': S / 'tests/qualification/rpc-proof.test.mjs',
    'tests/qualification/account-proof.mjs': R / 'tests/qualification/account-proof.mjs',
    'tests/hardhat/package.json': R / 'tests/hardhat/package.json',
}
materials = [*sources.values(), NODE, R / 'tests/hardhat/node_modules/ethers/package.json']

def read(path):
    assert path.resolve() == path
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        st = os.fstat(fd)
        assert stat.S_ISREG(st.st_mode) and st.st_nlink == 1
        blocks = []
        while True:
            chunk = os.read(fd, 1024 * 1024)
            if not chunk:
                break
            blocks.append(chunk)
        data = b''.join(blocks)
        after = os.fstat(fd)
        assert st == after and len(data) == st.st_size
        row = {key: getattr(st, key) for key in ['st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns']}
        row['sha256'] = hashlib.sha256(data).hexdigest()
        return row, data
    finally:
        os.close(fd)

OUT.mkdir(exist_ok=False)
samples = []
for i in range(3):
    samples.append({str(path): read(path)[0] for path in materials})
    if i != 2:
        time.sleep(1.05)
assert samples[0] == samples[1] == samples[2]
(OUT / 'new-input-admission.json').write_text(json.dumps({'samples': samples}, indent=2) + '\n')
projection = OUT / 'source'
for relative, original in sources.items():
    target = projection / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    row, data = read(original)
    assert row == samples[-1][str(original)]
    with target.open('xb') as handle:
        handle.write(data)
    target.chmod(0o400)

versions = subprocess.run([str(NODE), '-p', 'JSON.stringify(process.versions)'], text=True, capture_output=True, timeout=10)
(OUT / 'node-versions.stdout.json').write_text(versions.stdout)
(OUT / 'node-versions.stderr.log').write_text(versions.stderr)
assert versions.returncode == 0
argv = [str(NODE), '--test', str(projection / 'tests/qualification/rpc-proof.test.mjs')]
env = {'PATH': '/usr/bin:/bin', 'NODE_PATH': str(R / 'tests/hardhat/node_modules')}
(OUT / 'invocation.json').write_text(json.dumps({'argv':argv,'env':env,'cwd':str(projection),'scope':'eight new independent JS verifier unit tests; no C++ or live RPC qualification'}, indent=2) + '\n')
started = time.monotonic_ns()
result = subprocess.run(argv, cwd=projection, env=env, text=True, capture_output=True, timeout=60)
elapsed = (time.monotonic_ns() - started) // 1000000
(OUT / 'tests.stdout.log').write_text(result.stdout)
(OUT / 'tests.stderr.log').write_text(result.stderr)
unchanged = all(read(path)[0] == samples[-1][str(path)] for path in materials)
ethers = json.loads((R / 'tests/hardhat/node_modules/ethers/package.json').read_text())
public = {
    'format':'nano-independent-rpc-proof-verifier-unit-qualification-v1',
    'exitCode':result.returncode,'elapsedMs':elapsed,'inputsUnchanged':unchanged,
    'nodeVersion':json.loads(versions.stdout)['node'],'ethersVersion':ethers['version'],
    'testsExpected':8,'passedEight': result.returncode==0 and '# pass 8' in result.stdout and '# fail 0' in result.stdout,
    'newSourcePins':{relative:samples[-1][str(original)] for relative,original in sources.items()},
    'liveRpcCalled':False,'cppCompiled':False,'dockerBuilt':False,
    'fullNativeLoaderClosureQualified':False,
    'nanoProofOutputVerified':False,'eip1186Qualified':False,'productionQualified':False,
}
(OUT / 'result-public.json').write_text(json.dumps(public, ensure_ascii=False, indent=2) + '\n')
print(json.dumps({k:v for k,v in public.items() if k!='newSourcePins'}, ensure_ascii=False))
assert unchanged
assert public['passedEight']
