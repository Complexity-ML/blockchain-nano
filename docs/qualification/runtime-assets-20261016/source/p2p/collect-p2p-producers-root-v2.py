#!/usr/bin/env python3
"""Root-only prospective, read-only producer inventory. Never executes SDK tools."""
import collections, hashlib, json, os, pathlib, shlex, stat, subprocess, time

E = pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
R = pathlib.Path('/Users/boris/Dev/blockchain-nano')
P = E.parent / 'work/blockchain-nano/.deps'
B = P / 'libp2p-build'
S = P / 'cpp-libp2p'
OUT = E / 'fresh-p2p-producer-collection-root-20261016/attempt02'
REV = '944aa77b6d92e3ac609938f6d5a024acd88cc118'
LIBREV = 'c03bf7c44c4fd30a390f3ef93bd1eab482e686a6'
F = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid', 'st_size', 'st_mtime_ns', 'st_ctime_ns')
HELD, ALIASES = {}, {}

def need(v, message):
    if not v:
        raise ValueError(message)

def read(p):
    logical = pathlib.Path(os.path.abspath(p))
    real = logical.resolve(strict=True)
    alias = str(real)
    if str(logical) in ALIASES:
        need(ALIASES[str(logical)] == alias, 'alias changed')
    ALIASES[str(logical)] = alias
    fd = os.open(real, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        a = os.fstat(fd)
        need(stat.S_ISREG(a.st_mode) and 0 <= a.st_size <= 268435456, 'bounded regular input')
        chunks, n = [], 0
        while True:
            block = os.read(fd, 1048576)
            if not block:
                break
            chunks.append(block)
            n += len(block)
            need(n <= a.st_size, 'input grew')
        b = b''.join(chunks)
        row = {k: getattr(a, k) for k in F}
        need(n == a.st_size and row == {k: getattr(os.fstat(fd), k) for k in F} == {k: getattr(real.lstat(), k) for k in F}, 'FD/path fields')
        row['sha256'] = hashlib.sha256(b).hexdigest()
        if str(real) in HELD:
            need(HELD[str(real)] == row, 'observed input changed')
        else:
            HELD[str(real)] = row
        need(logical.resolve(strict=True) == real, 'alias resolution changed')
        return b
    finally:
        os.close(fd)

def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args])

def write_new(name, obj):
    b = (json.dumps(obj, sort_keys=True, separators=(',', ':')) + '\n').encode()
    fd = os.open(OUT / name, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o400)
    try:
        need(os.write(fd, b) == len(b), 'full result write')
        os.fsync(fd)
    finally:
        os.close(fd)
    need((OUT / name).read_bytes() == b, 'result readback')
    return {'sha256': hashlib.sha256(b).hexdigest(), 'bytes': len(b)}

def members(b):
    need(b[:8] == b'!<arch>\n', 'ordinary ar format required')
    off, out = 8, []
    while off < len(b):
        need(off + 60 <= len(b), 'ar header bounds')
        header = b[off:off + 60]
        need(header[58:60] == b'`\n', 'ar member header')
        size = int(header[48:58].strip())
        need(size >= 0, 'nonnegative ar member size')
        name = header[:16].decode('ascii').strip()
        start = off + 60
        need(start + size <= len(b), 'ar member data bounds')
        data = b[start:start + size]
        if name.startswith('#1/'):
            count = int(name[3:])
            need(0 <= count <= size, 'BSD ar name bounds')
            name = data[:count].rstrip(b'\0').decode('utf-8')
            data = data[count:]
        else:
            name = name.rstrip('/')
        need(name and not name.startswith('/') and name != '//', 'no unparsed GNU name table')
        if name not in ('__.SYMDEF', '__.SYMDEF SORTED', '__.SYMDEF_64', '__.SYMDEF_64 SORTED'):
            out.append({'name': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
        next_off = start + size + (size % 2)
        need(next_off > off, 'ar member progress')
        off = next_off
    need(off == len(b), 'ar final boundary')
    return out

def resolve_arg(cwd, arg):
    return pathlib.Path(os.path.normpath(str(cwd / arg))) if not arg.startswith('/') else pathlib.Path(arg)

def main():
    need(not OUT.exists(), 'new collection output only')
    OUT.mkdir(mode=0o700)
    need(git(R, 'rev-parse', 'HEAD').decode().strip() == REV and not git(R, 'status', '--porcelain', '--untracked-files=all'), 'current repository clean revision')
    need(git(S, 'rev-parse', 'HEAD').decode().strip() == LIBREV and not git(S, 'status', '--porcelain', '--untracked-files=no'), 'direct source tracked revision')
    tree = {}
    for row in git(S, 'ls-tree', '-rz', '--full-tree', 'HEAD').split(b'\0'):
        if row:
            head, name = row.split(b'\t', 1)
            mode, kind, sha = head.split()
            if kind == b'blob':
                tree[name.decode()] = sha.decode()
    recipe_path = R / 'docs/qualification/core-trace-bound-http-20261007/native-integration/nano-p2p-link-command.json'
    raw = read(recipe_path)
    need(len(raw) == 14977 and hashlib.sha256(raw).hexdigest() == '3ebe85f585bb4bd2d6412dc51bfc19d1330b5fb8ea3412c2965c90b726d817d1', 'actual public link recipe')
    recipe = json.loads(raw)
    installed = [pathlib.Path(x) for x in recipe['args'] if x.endswith('.a')]
    need(len(installed) == len(set(installed)) == 101, '101 unique linked archives')
    cache = read(B / 'CMakeCache.txt').decode()
    need('CMAKE_GENERATOR:INTERNAL=Unix Makefiles\n' in cache, 'actual Makefiles generator')
    db = json.loads(read(B / 'compile_commands.json'))
    need(len(db) == 177, 'actual retained compile database count')
    compiles = {}
    for row in db:
        args = shlex.split(row['command'])
        need(args.count('-o') == 1 and args.count('-c') == 1, 'ordinary compile recipe')
        obj = resolve_arg(pathlib.Path(row['directory']), args[args.index('-o') + 1])
        need(str(obj) not in compiles, 'unique compile output')
        compiles[str(obj)] = {'recipe': row, 'args': args}
    build = collections.defaultdict(list)
    for lp in sorted(B.glob('**/link.txt')):
        lines = read(lp).decode().splitlines()
        args = shlex.split(lines[0])
        need(len(lines) == 2 and pathlib.Path(args[0]).name == 'ar' and args[1] == 'qc', 'actual archive producer command')
        need(lp.parent.parent.name == 'CMakeFiles', 'ordinary CMake target recipe path')
        cwd = lp.parent.parent.parent
        archive = resolve_arg(cwd, args[2])
        objs = [resolve_arg(cwd, x) for x in args[3:]]
        build[archive.name].append({'archive': archive, 'link': lp, 'objects': objs, 'lines': lines})
    archives, selected_objects, all_dependencies = [], {}, set()
    for ip in installed:
        data = read(ip)
        ims = members(data)
        record = {'installed': str(ip), 'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data), 'members': ims, 'producerPresent': False}
        candidates = build.get(ip.name, [])
        if candidates:
            need(len(candidates) == 1 and ip.name.startswith('libp2p'), 'unambiguous libp2p producer')
            item = candidates[0]
            bd = read(item['archive'])
            need(ims == members(bd), 'installed/build ordered non-index members differ')
            record.update({'installedBuildArchiveBytesEqual': data == bd, 'installedBuildOrderedNonIndexMembersEqual': True})
            for make_input in ('build.make', 'flags.make', 'DependInfo.cmake'):
                read(item['link'].parent / make_input)
            expected = []
            for obj in item['objects']:
                od = read(obj)
                expected.append({'name': obj.name, 'bytes': len(od), 'sha256': hashlib.sha256(od).hexdigest()})
                need(str(obj) in compiles, 'archive member compile record absent')
                cr = compiles[str(obj)]
                need(pathlib.Path(cr['recipe']['directory']) == item['link'].parent.parent.parent, 'archive and compile recipe working directories agree')
                need(resolve_arg(B, cr['recipe']['output']) == obj, 'compile database output bound to object')
                md = obj.with_name(obj.name + '.d')
                md_data = read(md).decode().replace('\\\n', ' ')
                target, sep, deptext = md_data.partition(':')
                targets = shlex.split(target)
                need(sep and len(targets) == 1 and resolve_arg(B, targets[0]) == obj, 'MD target bound to compile database output at binary root')
                deps = [resolve_arg(pathlib.Path(cr['recipe']['directory']), x) for x in shlex.split(deptext)]
                source = pathlib.Path(cr['recipe']['file'])
                need(resolve_arg(pathlib.Path(cr['recipe']['directory']), cr['args'][cr['args'].index('-c') + 1]) == source, 'compile source argument bound to source record')
                need(source in deps and source.is_file(), 'source present in actual MD')
                source_data = read(source)
                source_rel = str(source.relative_to(S)) if source.is_relative_to(S) else None
                source_matches_git = source_rel in tree and hashlib.sha1(b'blob ' + str(len(source_data)).encode() + b'\0' + source_data).hexdigest() == tree[source_rel]
                for dep in deps:
                    read(dep)
                    all_dependencies.add(str(dep))
                selected_objects[str(obj)] = {'compile': cr['recipe'], 'md': str(md), 'dependencies': [str(x) for x in deps], 'sourceMatchesDirectGit': source_matches_git}
            need(ims == expected, 'archive member order/bytes versus physical objects differ')
            record.update({'producerPresent': True, 'buildArchive': str(item['archive']), 'linkRecipe': str(item['link']), 'objectCount': len(item['objects'])})
        archives.append(record)
    need(sum(x['producerPresent'] for x in archives) == 79, 'all79 direct archive producers')
    hunter = P / 'hunter/_Base/Download'
    definitions = hunter / 'Hunter/0.25.3-qdrvm36/15ca502/Unpacked/cmake/projects'
    source_packets = []
    for family in ('BoringSSL', 'c-ares', 'lsquic', 'libsecp256k1', 'Protobuf', 'soralog', 'fmt', 'yaml-cpp', 'ZLIB', 'Boost'):
        definition = read(definitions / family / 'hunter.cmake').decode()
        markers = list((hunter / family).glob('*/*/SHA1'))
        need(len(markers) == 1, 'one cached source version per selected family')
        marker = markers[0]
        expected_sha1 = read(marker).decode().strip()
        packets = [x for x in marker.parent.iterdir() if x.name.endswith(('.tar.gz', '.tar.bz2', '.tar.xz', '.zip'))]
        need(len(packets) == 1, 'one source packet in cached version')
        packet = read(packets[0])
        need(hashlib.sha1(packet).hexdigest() == expected_sha1 and expected_sha1 in definition, 'cached source packet and Hunter definition legacy digest')
        source_packets.append({'family': family, 'cachedVersion': marker.parent.parent.name, 'sourceArchiveSha256': hashlib.sha256(packet).hexdigest(), 'bytes': len(packet), 'legacySha1AndDefinitionMatch': True, 'linkedArchiveProducerAuthenticated': False})
    own_objects = [x for x in recipe['args'] if x.endswith('.o')]
    need(len(own_objects) == 1, 'one native consumer object in recipe')
    need(not pathlib.Path(own_objects[0]).exists(), 'missing original consumer object observation changed')
    start = time.monotonic()
    for sample in range(4):
        for path, row in HELD.items():
            need({k: getattr(pathlib.Path(path).lstat(), k) for k in F} == {k: row[k] for k in F}, 'new input not quiescent')
        for alias, real in ALIASES.items():
            need(str(pathlib.Path(alias).resolve(strict=True)) == real, 'alias changed during quiescence')
        if sample < 3:
            time.sleep(.8)
    need(time.monotonic() - start >= 2, 'minimum admission span')
    for path in list(HELD):
        read(path)
    need(git(R, 'rev-parse', 'HEAD').decode().strip() == REV and not git(R, 'status', '--porcelain', '--untracked-files=all') and git(S, 'rev-parse', 'HEAD').decode().strip() == LIBREV and not git(S, 'status', '--porcelain', '--untracked-files=no'), 'source authority unchanged after collection')
    private = {'format': 'nano-p2p-current-producer-inventory-private-v1', 'files': HELD, 'aliases': ALIASES, 'archives': archives, 'selectedObjects': selected_objects, 'sourcePackets': source_packets, 'sourceRevision': REV, 'directSourceRevision': LIBREV, 'historicalProducerSourceBindingRecovered': False, 'oldAuthoritiesRefreshed': False}
    private_pin = write_new('producer-inventory.private.json', private)
    unmatched = [x for x in archives if not x['producerPresent']]
    result = {'format': 'nano-p2p-current-producer-inventory-public-v2', 'passed': True, 'sourceRevision': REV, 'directSourceRevision': LIBREV, 'trackedDirectSourceClean': True, 'generator': 'Unix Makefiles', 'installedLinkedArchives': 101, 'directArchivesEqualBuildArchiveBytes': sum(x.get('installedBuildArchiveBytesEqual', False) for x in archives), 'directArchivesOrderedNonIndexMembersEqualBuild': 79, 'directArchiveBytesDifferOutsideNonIndexPayload': sum(not x['installedBuildArchiveBytesEqual'] for x in archives if x['producerPresent']), 'directArchiveMembersEqualRetainedObjectsInOrder': sum(x['objectCount'] for x in archives if x['producerPresent']), 'selectedDistinctCompileObjects': len(selected_objects), 'selectedObjectMdAndCompileRowsPresent': len(selected_objects), 'selectedSourcesEqualDirectGitBlobs': sum(x['sourceMatchesDirectGit'] for x in selected_objects.values()), 'selectedOtherSourceRows': sum(not x['sourceMatchesDirectGit'] for x in selected_objects.values()), 'uniqueRecordedDependencies': len(all_dependencies), 'fullNineShaRetainedFileRows': len(HELD), 'newAdmissionSamples': 4, 'newAdmissionSpanMs': round((time.monotonic() - start) * 1000), 'transitiveArchivesWithoutRetainedBuildProducer': len(unmatched), 'transitiveArchiveNonIndexMembers': sum(len(x['members']) for x in unmatched), 'transitiveCompileTuCountKnown': False, 'sourcePacketsAvailable': source_packets, 'originalNativeConsumerObjectMissing': True, 'originalNativeConsumerCompilationsNeededIfNoAlternateFound': 1, 'newCompilerInvocations': 0, 'dependencyOnlyM': 0, 'archiveOrProjectExecutions': 0, 'dockerCommands': 0, 'networkCommands': 0, 'privateInventoryPin': private_pin, 'historicalProducerSourceBindingRecovered': False, 'sourceReproductionQualification': False, 'linuxLibraryProducerQualification': False, 'all24RuntimeAdmissions': False, 'dutiesResumed': False, 'commonFinalityQualified': False, 'oldAuthoritiesRefreshed': False, 'productionQualified': False}
    pin = write_new('producer-inventory-public.json', result)
    print(json.dumps({'publicResult': result, 'publicPin': pin}, sort_keys=True), flush=True)

if __name__ == '__main__':
    try:
        main()
    except BaseException:
        print(json.dumps({'format': 'nano-p2p-producer-collection-refused-v1', 'passed': False, 'detailsSuppressed': True, 'compilerInvocations': 0, 'projectExecutions': 0, 'dockerCommands': 0, 'networkCommands': 0, 'oldAuthoritiesRefreshed': False}), flush=True)
        raise
