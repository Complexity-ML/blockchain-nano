#!/usr/bin/env python3
"""Read-only Root audit of retained native authorities; never refresh a pin."""
import hashlib
import json
import os
from pathlib import Path
import stat
import time

E = Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
OUT = Path(__file__).resolve().parent
FIELDS = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid', 'st_size', 'st_mtime_ns', 'st_ctime_ns')
REFS = {
    'original': ('historical-beacon-rpc-20261015/controls/native-historical-test-protected-inputs.json', '25c5441c78a55f88f4d4c4255d498f7ceb174ff075833164f8be7e1e2679a633', 6869424),
    'sdk': ('historical-beacon-rpc-20261015/controls/native-sdk-authority-v2.json', '963765ee106da27929144fdf095f8523a840610ce16cc726498e27f33eff7a10', 1514893),
    'projection': ('cross-arch-validator-read-root-20261015/native-old-authority-projection.private.json', 'f24ef99e86b15cd11f921d348471e3f77c0eeca8541b2719cb71a7f53a657851', 7528022),
    'oldConfig': ('fresh-rotation-thin-build-root-20261015/native-02/inputs.private.json', '3ec9e61e1820d9b838554bb85b95573467de00cd1bbc5712f2cf4a8f852a5e40', 1934711),
}

def nine(s):
    return {k: getattr(s, k) for k in FIELDS}

def read_stable(path, prior=None):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        before = os.fstat(fd)
        assert stat.S_ISREG(before.st_mode), 'not-regular'
        assert before.st_size <= 268435456, 'file-bound'
        if prior is not None:
            assert nine(before) == {k: prior[k] for k in FIELDS}, 'original-nine-fields-mismatch'
        digest = hashlib.sha256()
        chunks = []
        total = 0
        while True:
            chunk = os.read(fd, 1048576)
            if not chunk:
                break
            total += len(chunk)
            assert total <= before.st_size, 'file-grew-during-read'
            digest.update(chunk)
            if prior is None:
                chunks.append(chunk)
        assert total == before.st_size, 'file-short-read'
        assert nine(before) == nine(os.fstat(fd)) == nine(os.lstat(path)), 'changed-during-read'
        if prior is not None:
            assert digest.hexdigest() == prior['sha256'], 'original-bytes-mismatch'
        return digest.hexdigest(), before.st_size, b''.join(chunks), nine(before)
    finally:
        os.close(fd)

def write_new(name, value, mode):
    raw = (json.dumps(value, sort_keys=True, separators=(',', ':')) + '\n').encode()
    dest = OUT / name
    fd = os.open(dest, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, mode)
    try:
        pending = memoryview(raw)
        while pending:
            written = os.write(fd, pending)
            assert written > 0, 'zero-write'
            pending = pending[written:]
        os.fsync(fd)
    finally:
        os.close(fd)
    assert dest.read_bytes() == raw
    return {'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}

def main():
    start = time.monotonic_ns()
    docs, descriptor_pins = {}, {}
    for label, (rel, sha, size) in REFS.items():
        observed_sha, observed_size, raw, pin = read_stable(E / rel)
        assert (observed_sha, observed_size) == (sha, size), 'descriptor-original-byte-authority'
        descriptor_pins[label] = {**pin, 'sha256': sha}
        docs[label] = json.loads(raw)
    original, sdk, projection, config = (docs[k] for k in ('original', 'sdk', 'projection', 'oldConfig'))
    assert projection['files'] == original, 'projection-original-files-differ'
    assert projection['aliases'] == sdk['sdkAliases'], 'projection-original-aliases-differ'
    assert sdk['existingPinsNineFieldsAndBytesVerified'] is True
    assert sdk['oldSdkPinsRefreshed'] is False and sdk['newSdkInputsAutomaticallyRefreshed'] is False
    assert config['nativeSdkAuthority']['sha256'] == REFS['sdk'][1]
    assert all(original.get(path) == pin for path, pin in sdk['sdkFiles'].items()), 'sdk-regular-subset'
    expected_fields = set(FIELDS) | {'sha256'}
    failures, checked, byte_count = [], 0, 0
    for path, pin in original.items():
        assert set(pin) == expected_fields, 'original-row-shape'
        try:
            _, size, _, _ = read_stable(path, pin)
            byte_count += size
            checked += 1
        except (AssertionError, OSError) as exc:
            failures.append({'path': path, 'stage': 'original-file', 'exception': type(exc).__name__, 'reason': str(exc)})
    aliases_checked = 0
    for path, target in sdk['sdkAliases'].items():
        assert isinstance(target, str), 'original-alias-row-is-resolution-only'
        try:
            assert str(Path(path).resolve(strict=True)) == target, 'original-resolved-target-mismatch'
            aliases_checked += 1
        except (AssertionError, OSError) as exc:
            failures.append({'path': path, 'stage': 'alias-resolution', 'exception': type(exc).__name__, 'reason': str(exc)})
    # Fence original authorities without minting or replacing any old metadata row.
    for path, pin in original.items():
        try:
            assert nine(os.lstat(path)) == {k: pin[k] for k in FIELDS}, 'original-final-nine-mismatch'
        except (AssertionError, OSError) as exc:
            failures.append({'path': path, 'stage': 'final-file-fence', 'exception': type(exc).__name__, 'reason': str(exc)})
    for path, target in sdk['sdkAliases'].items():
        try:
            assert str(Path(path).resolve(strict=True)) == target, 'original-final-resolved-target-mismatch'
        except (AssertionError, OSError) as exc:
            failures.append({'path': path, 'stage': 'final-alias-fence', 'exception': type(exc).__name__, 'reason': str(exc)})
    for label, (rel, _, _) in REFS.items():
        read_stable(E / rel, descriptor_pins[label])
    private_ref = write_new('native-original-input-audit.private.json', {
        'format': 'nano-native-original-input-audit-private-v1',
        'failures': failures,
        'oldAuthoritiesRefreshed': False,
    }, 0o600)
    public = {
        'format': 'nano-native-original-input-audit-public-v1',
        'passed': not failures,
        'descriptorOriginalByteAuthoritiesVerified': len(REFS),
        'projectionExactlyMatchesOriginalAuthority': True,
        'sdkRegularFilesSubsetExactlyMatchesOriginalAuthority': True,
        'originalRegularFiles': len(original),
        'regularFilesFullNineAndSha256Verified': checked,
        'sdkRegularFiles': len(sdk['sdkFiles']),
        'bytesHashed': byte_count,
        'aliasesOriginalResolvedTargets': len(sdk['sdkAliases']),
        'aliasesOriginalResolvedTargetsVerified': aliases_checked,
        'aliasHistoricalNineFieldsClaimed': False,
        'failureCount': len(failures),
        'privateFailureReceipt': private_ref,
        'elapsedMs': (time.monotonic_ns() - start) // 1000000,
        'oldAuthoritiesRefreshed': False,
        'projectProgramsExecuted': 0,
        'compilations': 0,
        'testsRun': 0,
        'runtimeDutiesResumed': False,
        'effectiveSourceClosureQualified': False,
    }
    ref = write_new('native-original-input-audit.public.json', public, 0o400)
    print(json.dumps({'result': public, 'receipt': ref}), flush=True)
    raise SystemExit(0 if public['passed'] else 1)

if __name__ == '__main__':
    main()
