#!/usr/bin/env python3
"""Root descriptor join after the read-only P2P collector; no repeated SDK sweep."""
import hashlib, json, os, pathlib, stat, time

E = pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
D = E / 'fresh-p2p-producer-collection-root-20261016/attempt02'
B = E.parent / 'work/blockchain-nano/.deps/libp2p-build'
F = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid', 'st_size', 'st_mtime_ns', 'st_ctime_ns')

def need(v, s):
    if not v:
        raise ValueError(s)

def known(p, sha, size):
    need(p.resolve(strict=True) == p, 'physical descriptor')
    fd = os.open(p, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        a = os.fstat(fd)
        need(stat.S_ISREG(a.st_mode) and a.st_size == size <= 67108864, 'bounded known descriptor')
        chunks, n = [], 0
        while True:
            block = os.read(fd, 1048576)
            if not block:
                break
            chunks.append(block)
            n += len(block)
            need(n <= size, 'descriptor size')
        b = b''.join(chunks)
        row = {k: getattr(a, k) for k in F}
        need(n == size and row == {k: getattr(os.fstat(fd), k) for k in F} == {k: getattr(p.lstat(), k) for k in F} and hashlib.sha256(b).hexdigest() == sha, 'known bytes and unchanged FD/path')
        return json.loads(b), row
    finally:
        os.close(fd)

def main():
    pub, pr = known(D / 'producer-inventory-public.json', '2aa83d80ef0ff704a6489a9c7505aee2efd9ecebfebac2dab703448f66f3eb45', 3855)
    data, dr = known(D / 'producer-inventory.private.json', '678c1f12dafaf161a9b3b87f1a6c96ebc1bf88fab6c6dc369ce96ab9cb181191', 28008348)
    need(pub['passed'] is True and pub['privateInventoryPin'] == {'sha256': '678c1f12dafaf161a9b3b87f1a6c96ebc1bf88fab6c6dc369ce96ab9cb181191', 'bytes': 28008348}, 'public to private descriptor')
    files, aliases, archives, objects = (data[k] for k in ('files', 'aliases', 'archives', 'selectedObjects'))
    need(len(files) == 4434 and len(archives) == 101 and len(objects) == 127, 'actual inventory counts')
    def row(path):
        return files[aliases[os.path.abspath(path)]]
    direct = [x for x in archives if x['producerPresent']]
    missing = [x for x in archives if not x['producerPresent']]
    need(len(direct) == 79 and len(missing) == 22 and sum(len(x['members']) for x in missing) == 683, 'direct and unmatched archive split')
    for ar in direct:
        need(not ar['installedBuildArchiveBytesEqual'] and ar['installedBuildOrderedNonIndexMembersEqual'] is True and row(ar['installed'])['sha256'] == ar['sha256'] and row(ar['installed'])['sha256'] != row(ar['buildArchive'])['sha256'], 'distinct archive bytes correctly retained')
        need(ar['objectCount'] == len(ar['members']), 'archive member count')
    need(sum(x['objectCount'] for x in direct) == 127, 'archive members and selected objects')
    for op, entry in objects.items():
        orow = row(op)
        need(orow['st_size'] > 0 and len(orow['sha256']) == 64 and row(entry['md'])['st_size'] > 0, 'retained object and actual MD rows')
        need(all(os.path.abspath(path) in aliases for path in entry['dependencies']), 'retained dependency references')
    other = [x['compile']['file'] for x in objects.values() if not x['sourceMatchesDirectGit']]
    need(sum(x['sourceMatchesDirectGit'] for x in objects.values()) == 123 and len(other) == 4 and all(pathlib.Path(x).is_relative_to(B / 'pb') and x.endswith('.pb.cc') for x in other), '123 Git TUs and four generated protobuf TUs')
    need(len(pub['sourcePacketsAvailable']) == 10 and all(x['legacySha1AndDefinitionMatch'] is True and x['linkedArchiveProducerAuthenticated'] is False for x in pub['sourcePacketsAvailable']), 'cached sources are not linked producers')
    need(all(pub[k] is False for k in ('historicalProducerSourceBindingRecovered', 'sourceReproductionQualification', 'linuxLibraryProducerQualification', 'all24RuntimeAdmissions', 'dutiesResumed', 'commonFinalityQualified', 'oldAuthoritiesRefreshed', 'productionQualified')), 'qualification boundary unchanged')
    start = time.monotonic()
    for i in range(4):
        need({k: getattr((D / 'producer-inventory-public.json').lstat(), k) for k in F} == pr and {k: getattr((D / 'producer-inventory.private.json').lstat(), k) for k in F} == dr, 'new descriptor quiescence')
        if i < 3:
            time.sleep(.8)
    need(time.monotonic() - start >= 2, 'new descriptor admission span')
    result = {'format': 'nano-p2p-current-producer-inventory-postqualification-v1', 'passed': True, 'descriptorJoinOnly': True, 'retainedFileRows': 4434, 'directArchiveMemberSequences': 79, 'exactBuildArchiveByteMatches': 0, 'selectedObjectMdCompileRows': 127, 'selectedTuSourcesMatchDirectGitBlobs': 123, 'generatedProtobufSourceRows': 4, 'transitiveArchivesWithoutRetainedBuildProducer': 22, 'transitiveNonIndexMembersNotProvenTuCount': 683, 'cachedSourcePacketsNotProducerProof': 10, 'descriptorAdmissionSamples': 4, 'descriptorAdmissionSpanMs': round((time.monotonic() - start) * 1000), 'sdkFileBytesReread': 0, 'sdkToolsExecuted': 0, 'projectExecutions': 0, 'compilerInvocations': 0, 'dockerCommands': 0, 'networkCommands': 0, 'oldAuthoritiesRefreshed': False, 'historicalProducerSourceBindingRecovered': False, 'sourceReproductionQualification': False, 'all24RuntimeAdmissions': False, 'dutiesResumed': False, 'commonFinalityQualified': False, 'productionQualified': False}
    b = (json.dumps(result, sort_keys=True, separators=(',', ':')) + '\n').encode()
    fd = os.open(D / 'postqualification-public.json', os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o400)
    try:
        need(os.write(fd, b) == len(b), 'full public result write')
        os.fsync(fd)
    finally:
        os.close(fd)
    need((D / 'postqualification-public.json').read_bytes() == b, 'public result readback')
    print(json.dumps({'result': result, 'sha256': hashlib.sha256(b).hexdigest(), 'bytes': len(b)}, sort_keys=True), flush=True)

if __name__ == '__main__':
    main()
