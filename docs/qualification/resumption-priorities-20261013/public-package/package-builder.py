#!/usr/bin/env python3
"""One-shot local allowlist packaging; no runtime, network, Git or secret payload export."""
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import stat
from datetime import datetime, timezone

BASE = Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
DEST = BASE / 'resumption-evidence-public-20261013'
PROJ = BASE / 'isolated-stopped-store-projection-20261012'
GIT = BASE / 'git-recovery-audit-20261013'
COUNTER = BASE / 'new-duty-projection-availability-counterexample-20261013'
FOUR = COUNTER / 'candidate-preparation'
SIX = FOUR / 'postready-sync-extension'
EXPECTED_MANIFESTS = {
    'projection': '252f9b1c3c3bd129ec13659b3f9d0ebac2268002341b2679ccffc9b1f8b970a6',
    'counterexample': '10a8186fce6b7822258b1fd88e82970154f84c62a2de3b33c69ad427205171fe',
    'four': '9980438f3409dc904485b9ac3d5ebc5584c14b44ed6a206bc1550e9a6e623d12',
}
EXCLUDE = {
    'actual-run-20261012T221417Z/input-files-after.json': 'private input stat/inode inventory',
    'actual-run-20261012T221417Z/request.json': 'private input request',
    'actual-run-20261012T221716Z/input-files-after.json': 'private input stat/inode inventory',
    'actual-run-20261012T221716Z/request.json': 'private input request',
    'actual-run-20261012T222431Z/request.json': 'private input request',
    'actual-run-internal-20261012T223029Z/input-files-after.json': 'private input stat/inode inventory',
    'compile-type-repair/original/receipts/input-files-before.json': 'private input stat/inode inventory',
    'receipts/input-files-before.json': 'private input stat/inode inventory',
    'request-template.json': 'private stabilized input request template',
    'actual-run-20261012T222431Z/projection.log': 'raw request stat/ctime diagnostic',
    'actual-run-20261012T222431Z/collector-failure-receipt.json': 'embedded raw request stat/ctime diagnostic',
}
SSZ = ('derived-positive-balance-exit8-state.ssz', 19789,
       'a35690f28785126a5fc14df4408a0a1520845950165db94516718eb76f2dbf92')
GIT_HASHES = {
    'terminal-summary.json': '39a45d4efc7d4d7ec52cd5a34fcd378f53ba54bd3f640207760b7f5e5f9604d4',
    'archive-member-inventory.json': 'de71e8c982815672197405834bb45fc56cfe00c61a0d709c884398fec5379d77',
    'README.md': 'd9979630758f18faff1db2c5a62fc59b40a4e1c19a8428f12f11dfdc25d01f72',
    'abi-consumers-and-cmake.json': 'c396613c4f0b94060c3e87cc38e3542fff0765839f22597fdbfa562d9f4d7100',
}
COUNTER_EXTRAS = {
    'terminal-package.json': '51e2935c9da8589992fbe79dc69c85a63a0cba23cd42c2242bedcb25f525f1d5',
    'independent-review/terminal-review.json': '6cd5044ae20ef147bf98f34751c035a38b3c5ee4cc23eb568533d86aace7efee',
}
FOUR_EXTRAS = {
    'terminal-helper-source-binding.json': 'fe3404cbb77b5d816c44b56f0246103af6439f008f2ed0327df36d15873af9c8',
    'independent-review/terminal-review.json': 'a3674f8c29abd5276bebdd8ee7ae91600da0fc27adc4a4cb0c4438ae86114822',
    'independent-review/sync-post-ready-design.md': '1f6b34b86f34ebdf131ec065b74da69433f215a963c636d2eecf144040d2dcd4',
}
SIX_EXTRAS = {
    'source-binding.json': 'ddc6ca9d9dfaf214fecfae81e5e89dcae5188629830936c906718d03be708903',
    'final-six-source-binding.json': 'd18ca0b821f22065c6998c4f56ad7943db8c9803fa358b67fe4c3cbf41e307a6',
    'postready.patch': '3f56c608eb1622d3ff71cdf6f763f65e0bcc1799f07b16c4b312c067e46ff475',
    'independent-review/terminal-review.json': 'cf6573bf1d85055c8b21414fa5798f0c04cc06f5c5c7ead2dde218a11b246ec3',
}
SOURCE_OBSERVATIONS = {}
COPIES = []
COPIED = []
GROUPS = []

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def relpath(text):
    p = PurePosixPath(text)
    if p.is_absolute() or '..' in p.parts or not p.parts or str(p) != text:
        raise RuntimeError('noncanonical relative path')
    return p

def identity(s):
    return (s.st_dev, s.st_ino, s.st_mode, s.st_size, s.st_mtime_ns, s.st_ctime_ns)

def read_stable(root, rel, expected=None, expected_bytes=None):
    relpath(rel)
    p = root / rel
    for item in [p, *p.parents]:
        if item.is_symlink():
            raise RuntimeError('symlink component refused: ' + str(item))
    if not p.resolve().is_relative_to(root.resolve()):
        raise RuntimeError('source containment refused')
    fd = os.open(p, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        before = os.fstat(fd)
        if not stat.S_ISREG(before.st_mode):
            raise RuntimeError('nonregular source')
        chunks = []
        while True:
            chunk = os.read(fd, 1024 * 1024)
            if not chunk:
                break
            chunks.append(chunk)
        raw = b''.join(chunks)
        after = os.fstat(fd)
    finally:
        os.close(fd)
    if identity(before) != identity(after) or identity(after) != identity(p.lstat()):
        raise RuntimeError('source changed during read')
    digest = sha(raw)
    if expected is not None and digest != expected:
        raise RuntimeError('literal source SHA mismatch: ' + str(p))
    if expected_bytes is not None and len(raw) != expected_bytes:
        raise RuntimeError('literal source byte-count mismatch: ' + str(p))
    previous = SOURCE_OBSERVATIONS.get(p)
    observation = (identity(after), digest, len(raw))
    if previous is not None and previous != observation:
        raise RuntimeError('source changed between observations')
    SOURCE_OBSERVATIONS[p] = observation
    return raw

def payload_field_guard(value):
    if isinstance(value, dict):
        for key, item in value.items():
            if key in {'snapshot', 'witnessSnapshot', 'canonicalStore', 'rawState',
                       'privateKey', 'secretKey', 'seedBytes', 'keyMaterial'} and item not in (None, False, []):
                raise RuntimeError('private/raw structured payload field refused: ' + key)
            payload_field_guard(item)
    elif isinstance(value, list):
        for item in value:
            payload_field_guard(item)

def public_file_guard(root, rel, raw):
    p = PurePosixPath(rel)
    if p.name.endswith(('.private.json', '.env', '.key', '.o', '.a', '.so', '.dylib')) or any(
        x in {'private', 'capsules', 'captures', 'data', 'witness', 'ledger', '.git'} for x in p.parts
    ):
        raise RuntimeError('prohibited payload path refused: ' + rel)
    if p.suffix == '.ssz':
        if root != COUNTER or (rel, len(raw), sha(raw)) != SSZ:
            raise RuntimeError('unapproved binary/SSZ refused')
        return
    if p.suffix not in {'.json', '.log', '.py', '.cpp', '.md', '.hpp', '.patch', '.map'}:
        raise RuntimeError('filetype outside public allowlist')
    text = raw.decode('utf-8')
    forbidden_key_markers = ['-----BEGIN ' + 'PRIVATE KEY-----', '-----BEGIN ' + 'OPENSSH PRIVATE KEY-----']
    if '\0' in text or any(marker in text for marker in forbidden_key_markers):
        raise RuntimeError('binary/private-key content refused')
    if p.suffix == '.json':
        payload_field_guard(json.loads(text))

def queue(root, rel, dest_rel, expected=None, expected_bytes=None):
    raw = read_stable(root, rel, expected, expected_bytes)
    public_file_guard(root, rel, raw)
    relpath(dest_rel)
    if any(row['destination'] == dest_rel for row in COPIES):
        raise RuntimeError('duplicate destination')
    COPIES.append({'source': str((root / rel).relative_to(BASE)), 'destination': dest_rel,
                   'bytes': len(raw), 'sha256': sha(raw), '_raw': raw})

def manifest_group(root, manifest, key, prefix, excludes=None):
    excludes = excludes or {}
    raw = read_stable(root, manifest, EXPECTED_MANIFESTS[key])
    rows = json.loads(raw)['files']
    paths = [x['path'] for x in rows]
    if len(paths) != len(set(paths)) or not set(excludes).issubset(paths):
        raise RuntimeError('manifest duplicates or unknown exclusions')
    for row in rows:
        # Even excluded entries are verified unchanged, but not copied or parsed.
        read_stable(root, row['path'], row['sha256'], row['bytes'])
        if row['path'] not in excludes:
            queue(root, row['path'], prefix + '/' + row['path'], row['sha256'], row['bytes'])
    queue(root, manifest, prefix + '/' + manifest, EXPECTED_MANIFESTS[key])
    GROUPS.append({'group': key, 'destination': prefix, 'sourceManifest': str((root/manifest).relative_to(BASE)),
                   'sourceManifestSha256': sha(raw), 'originalListedFileCount': len(rows),
                   'copiedListedFileCount': len(rows)-len(excludes), 'excludedListedFileCount': len(excludes),
                   'originalManifestCopiedByteForByte': True, 'allOriginalEntriesVerified': True})
    return rows

def generated(rel, raw):
    relpath(rel)
    p = DEST / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(p, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o644)
    try:
        offset = 0
        while offset < len(raw):
            offset += os.write(fd, raw[offset:])
        os.fsync(fd)
    finally:
        os.close(fd)
    if p.is_symlink() or p.read_bytes() != raw:
        raise RuntimeError('destination verification failed')
    COPIED.append({'path': rel, 'bytes': len(raw), 'sha256': sha(raw)})

def generated_json(rel, value):
    generated(rel, (json.dumps(value, indent=2, ensure_ascii=False) + '\n').encode())

def main():
    if DEST.exists() or DEST.is_symlink():
        raise RuntimeError('fresh destination required; no overwrite/rewrite')
    proj_rows = manifest_group(PROJ, 'evidence-file-manifest.json', 'projection', 'projection-mac-stoppee', EXCLUDE)
    manifest_group(COUNTER, 'public-evidence-manifest.json', 'counterexample', 'contre-exemple-original')
    manifest_group(FOUR, 'public-evidence-manifest.json', 'four', 'candidat-non-integre/quatre-sources-tests-purs')
    for rel, digest in GIT_HASHES.items():
        queue(GIT, rel, 'audit-git-13-archives/' + rel, digest)
    git_summary = json.loads(read_stable(GIT, 'terminal-summary.json', GIT_HASHES['terminal-summary.json']))
    if (git_summary['trackedArchiveCount'], git_summary['archiveMemberCount']) != (13, 483):
        raise RuntimeError('archive audit scope mismatch')
    if git_summary['archiveInventorySha256'] != GIT_HASHES['archive-member-inventory.json'] or git_summary['abiReportSha256'] != GIT_HASHES['abi-consumers-and-cmake.json']:
        raise RuntimeError('Git audit cross-binding mismatch')
    for rel, digest in COUNTER_EXTRAS.items():
        queue(COUNTER, rel, 'contre-exemple-original/' + rel, digest)
    terminal = json.loads(read_stable(COUNTER, 'terminal-package.json', COUNTER_EXTRAS['terminal-package.json']))
    if terminal['publicEvidenceManifestSha256'] != EXPECTED_MANIFESTS['counterexample'] or terminal['staticReviewSha256'] != COUNTER_EXTRAS['independent-review/terminal-review.json']:
        raise RuntimeError('counterexample terminal cross-binding mismatch')
    for rel, digest in FOUR_EXTRAS.items():
        queue(FOUR, rel, 'candidat-non-integre/quatre-sources-tests-purs/' + rel, digest)
    for row in json.loads(read_stable(FOUR, 'terminal-helper-source-binding.json', FOUR_EXTRAS['terminal-helper-source-binding.json']))['files']:
        read_stable(FOUR, row['path'], row['sha256'], row['bytes'])
    for rel, digest in SIX_EXTRAS.items():
        queue(SIX, rel, 'candidat-non-integre/six-sources-extension/' + rel, digest)
    before = json.loads(read_stable(SIX, 'source-binding.json', SIX_EXTRAS['source-binding.json']))
    six = json.loads(read_stable(SIX, 'final-six-source-binding.json', SIX_EXTRAS['final-six-source-binding.json']))
    if len(before['files']) != 4 or len(six['files']) != 6:
        raise RuntimeError('extension source-count mismatch')
    for row in before['files']:
        for folder, field in [('before-source', 'beforeSha256'), ('candidate-source', 'candidateSha256')]:
            rel = folder + '/' + row['path']
            queue(SIX, rel, 'candidat-non-integre/six-sources-extension/' + rel, row[field])
    for row in six['files']:
        rel = 'final-six-source/' + row['path']
        queue(SIX, rel, 'candidat-non-integre/six-sources-extension/' + rel, row['sha256'])
    review = json.loads(read_stable(SIX, 'independent-review/terminal-review.json', SIX_EXTRAS['independent-review/terminal-review.json']))
    if review['sixSourceBindingSha256'] != SIX_EXTRAS['final-six-source-binding.json'] or review['previousFourSourceReviewSha256'] != FOUR_EXTRAS['independent-review/terminal-review.json'] or review['old11PureCasesPromotedToThisExtension'] is not False:
        raise RuntimeError('extension review cross-binding/scope mismatch')
    builder_name = Path(__file__).name
    queue(BASE, builder_name, 'package-builder.py')
    history_root = BASE / 'resumption-package-assembly-history-20261013'
    queue(history_root, 'first-refusal.json', 'package-assembly-history/first-refusal.json')
    # All source manifests, entries and bindings are validated before destination creation.
    DEST.mkdir(mode=0o755)
    for row in COPIES:
        generated(row['destination'], row['_raw'])
    excluded_rows = []
    for row in proj_rows:
        if row['path'] in EXCLUDE:
            excluded_rows.append({**row, 'reason': EXCLUDE[row['path']], 'sourceRetainedUnchanged': True, 'payloadCopied': False})
    generated_json('projection-mac-stoppee/excluded-files.json', {
        'format': 'nano-public-evidence-explicit-subset-exclusions-v1',
        'sourceManifestSha256': EXPECTED_MANIFESTS['projection'],
        'all127EntriesVerified': True, 'copiedListedFiles': 116, 'excludedListedFiles': 11,
        'historicalManifestRewritten': False, 'excludedFiles': excluded_rows,
        'note': 'The original manifest is historical, not a destination-completeness manifest. Excluded payloads stay local in the unchanged source folder.'})
    generated_json('projection-mac-stoppee/refusal-history-public-summary.json', {
        'format': 'nano-stopped-projection-public-refusal-history-v1',
        'sourceManifestSha256': EXPECTED_MANIFESTS['projection'],
        'events': [
            {'stage': 'original compile', 'result': 'missing JSON get<string> conversion; first compiler failure retained'},
            {'stage': 'compile-type-repair', 'result': 'compile/link succeeded; Python collector path failure retained separately'},
            {'stage': 'actual-run-20261012T221417Z', 'exitCode': 1, 'result': 'strict profile runtime path shape refused before projection'},
            {'stage': 'actual-run-20261012T221716Z', 'exitCode': 1, 'result': 'projection completed, then final input identity check refused; not PASS'},
            {'stage': 'identity-diagnostic', 'result': 'approval review timeout retained; authorized retry receipt retained'},
            {'stage': 'actual-run-20261012T222431Z', 'exitCode': 1, 'result': 'request input 0 ctime changed; eleven source input bytes/identities remained unchanged; raw stat diagnostics deliberately excluded',
             'originalExcludedLogSha256': '6741987edc52fbd2744506a789236b9fcd0bf33c621a73cd211e9cbb4677b0ac',
             'xattrCauseEstablished': False},
            {'stage': 'actual-run-internal-20261012T223029Z', 'exitCode': 0, 'result': 'static request null timestamp and actual C++ wall clock; all strict hashes and identities unchanged; pure projection scope only'}],
        'sourceFailuresOverwritten': False, 'rawStatOrCapsulePayloadPublished': False,
        'liveRecoveryQualified': False, 'productionQualified': False})
    # Replace no historical source: derived history records only existing metadata and outcome.
    history_path = DEST / 'projection-mac-stoppee/refusal-history-public-summary.json'
    history = json.loads(history_path.read_bytes())
    actual_log = next(row for row in proj_rows if row['path'] == 'actual-run-20261012T222431Z/projection.log')
    if history['events'][5]['originalExcludedLogSha256'] != actual_log['sha256']:
        raise RuntimeError('derived refusal history literal digest mismatch')
    generated_json('package-provenance.json', {
        'format': 'nano-local-public-resumption-package-provenance-v1',
        'createdAtUtc': datetime.now(timezone.utc).isoformat(),
        'groups': GROUPS,
        'copiedSourceFiles': [{k: v for k, v in row.items() if k != '_raw'} for row in COPIES],
        'gitAuditOnlyMembersNotPayloads': True, 'gitAuditArchives': 13, 'gitAuditArchiveMembers': 483,
        'approvedSyntheticPublicSSZ': {'path': 'contre-exemple-original/' + SSZ[0], 'bytes': SSZ[1], 'sha256': SSZ[2],
            'scope': 'public synthetic test state derived from public initial fixture and independently authenticated as an explicitly trusted synthetic anchor; not admitted peer state/private stopped Store'},
        'linkMapIncluded': {'path': 'candidat-non-integre/quatre-sources-tests-purs/candidate-link.map', 'bytes': 820013,
            'sha256': '7f1ebcc3202fcccd2dbd5516dbb2b89fc0d7aff422e6142124f64b0a9624c430', 'reason': 'public symbols prove old Reactor ABI objects were not extracted for the four-source pure test link'},
        'binariesOrObjectsIncluded': False, 'envSeedKeysCapsulesDatabasePayloadsIncluded': False,
        'vpsAccessedOrEvidenceExported': False, 'repoEdited': False, 'gitMutation': False,
        'compilerOrTestsInvokedDuringPackaging': False,
        'fourSource11TestsPromotedToSixSourceExtension': False,
        'realReactorIntegrationQualifiedByPackage': False, 'productionQualifiedByPackage': False})
    readme = '''# Public resumption evidence — local package, 2026-10-13

This package copies only the approved public evidence bytes. It creates no new test, recovery or production qualification and does not modify the repository, Git history or any historical source evidence.

## Included scopes

- `projection-mac-stoppee/`: 116 of the original 127 listed files plus the unchanged original manifest, explicit 11-file exclusion list and a separate safe refusal summary. All original 127 SHA/byte counts were verified, including excluded files. The original manifest remains a historical source manifest; excluded entries are intentionally absent from this delivery. Historical compiler, collector, profile and identity refusals remain separate from the final scoped success. Raw request/stat inventories, raw ctime diagnostic output, private capsules and snapshots remain local outside this package.
- `audit-git-13-archives/`: four existing audit files, limited to 13 archive member tables (483 members) and source/ABI metadata. No archive payload is copied or extracted. This local-ref audit at b7719685 did not verify live GitHub or restore a validator from Git.
- `contre-exemple-original/`: the original 37-file manifest set, unchanged manifest, independent review and terminal package. Its one 19,789-byte SSZ is explicitly approved PUBLIC synthetic state derived from the public initial fixture. It is an authenticated, explicitly trusted synthetic test anchor, not peer-admitted state or a private stopped Store/capsule.
- `candidat-non-integre/quatre-sources-tests-purs/`: the original 22-file pure candidate set, unchanged manifest, terminal helper-source binding, independent review and design note. The 11 cases qualify only this four-source pure test scope; helper hashes were observed at terminal, not before compilation. The public 820,013-byte link map is retained because it establishes exclusion of old Reactor ABI objects from that limited link.
- `candidat-non-integre/six-sources-extension/`: four-source before/after extension snapshots, six final source snapshots, exact bindings, patch and static review. It has no integration qualification in this package. The old 11 cases are not promoted to these six files. No current integration-test result is copied or inferred.

## Limits of the stopped-Mac projection

The final probe uses the previous qualified core85 archive and directly reads the real C++ system clock at 2026-10-12T22:30:29Z. One pure empty-slot continuation represents the two exactly equal stopped Mac Stores. It does not run a startup clock, duty preparation, signer, database, journal, network, VPS or finality. Current/future sync member availability and balances are scalar observations at that wall time, not future-time guarantees or aggregate-signature qualification. The input journal capture is reused by literal hashes, not re-audited. Linux stat/RSS APIs restrict this helper run to Linux ARM64 Docker.

The head gap and durable-clock gap are distinct: old head slot 1 projected to 1891, while durable clock downtime was 6,472,330 ms (54 ordinary steps within unchanged finite 2h/60-step policy). This historical success does not extend that allowance as wall time advances. The measured request ctime refusal is preserved; no xattr cause was established.

## Integrity and exclusions

`package-provenance.json` maps source paths, SHA-256 and byte counts to every copied file. `public-evidence-manifest.json` covers all package files except itself; use that destination manifest to verify completeness. The original manifests are copied byte-for-byte. All reads reject symlink components and nonregular source files; source bytes and identity are rechecked after copying. Selected JSON payloads reject raw snapshot/state/private-key fields; text files reject binary/NUL/private-key blocks. The only binary-format exception is the exact approved public synthetic SSZ above. No executable, object, library, seed, key, environment, private request, raw capsule, database or VPS payload is delivered. Public source and receipt paths/hash references may name private inputs without including their contents.

This package proves controlled local delivery only. Runtime restoration, real duty resumption, common-finality progression, coherent integration of six-source changes and production readiness remain separate open gates. No compiler, test, writer, network request, Git mutation or VPS operation was performed to assemble this package.
'''
    generated('README.md', readme.encode())
    # Recheck original source bytes and file identity after all copying.
    for p, expected in list(SOURCE_OBSERVATIONS.items()):
        read_stable(p.parent, p.name, expected[1], expected[2])
    generated_json('copy-validation-receipt.json', {
        'format': 'nano-public-package-copy-validation-v1',
        'sourceFilesVerifiedUnchangedAfterCopy': len(SOURCE_OBSERVATIONS),
        'sourceIdentityAndShaStrict': True, 'symlinksAccepted': False,
        'allCopiedDestinationBytesEqualSources': True,
        'original127ManifestEntriesChecked': 127, 'original37ManifestEntriesChecked': 37,
        'original22ManifestEntriesChecked': 22, 'original127EntriesExcluded': 11,
        'rawRequestDiagnosticCopied': False, 'sourceHistoricalFilesWritten': False,
        'compilerInvoked': False, 'testsRun': False, 'networkUsed': False, 'vpsAccessed': False,
        'repoEdited': False, 'gitMutation': False,
        'candidateFourSourceTestsAppliedToSixSources': False,
        'runtimeQualified': False, 'productionQualified': False})
    # No binary or historical manifest is rewritten to fabricate a pass.
    manifest = {'format': 'nano-public-resumption-delivery-manifest-v1',
                'files': sorted(COPIED, key=lambda row: row['path']),
                'originalManifestsRetainedUnchanged': True,
                'candidateSourcesNotRuntimeIntegratedByPackage': True,
                'productionQualified': False}
    manifest_raw = (json.dumps(manifest, indent=2, ensure_ascii=False) + '\n').encode()
    generated('public-evidence-manifest.json', manifest_raw)
    total = sum(row['bytes'] for row in COPIED)
    print(json.dumps({'destination': str(DEST), 'filesIncludingTopManifest': len(COPIED),
                      'bytesIncludingTopManifest': total, 'manifestSha256': sha(manifest_raw),
                      'sourceCopies': len(COPIES), 'strictSourceChecks': len(SOURCE_OBSERVATIONS),
                      'excludedOriginalProjectionFiles': len(EXCLUDE),
                      'repoEdited': False, 'vpsAccessed': False, 'productionQualified': False}, indent=2))

if __name__ == '__main__':
    main()
