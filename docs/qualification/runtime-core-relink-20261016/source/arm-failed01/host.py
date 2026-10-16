#!/usr/bin/env python3
"""Root-only ARM runtime relinks; author has not executed this source."""
import argparse, hashlib, json, os, pathlib, re, resource, shlex, signal, stat, subprocess, sys, tarfile, time, types, uuid

E = pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
R = pathlib.Path('/Users/boris/Dev/blockchain-nano')
REV = '68d64a5330d16c971dd603b56e0f14c88c1459c8'
PREVIOUS = 'bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d'
IMAGE = 'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
CORE = {'sha256': '35779b74ff3894d8857d06732caa801cb340381f4cf6dc5801074c7e372dbdf6', 'bytes': 41053984}
A = E / 'current-arm-helpers-root-attempt02-20261016'
N = E / 'complete-nano-arm-root-qualification-attempt02-20261016'
C = E / 'pruning-operation-arm-root-qualification-20261016-01'
ORIGIN = E / 'fresh-arm-bootstrap-import-current-core-root-20261016'
WORKER = E / 'current-linux-runtime-relink-worker-source-20261016/worker.pl'
WORKER_PIN = {'sha256': 'db0e026bffce0407e89c89951a14e208b0a7c714d49113a8874d0bac3f177526', 'bytes': 27007}
ROLES = {'nano': ('nano', 'src/main.cpp'), 'bootstrapImport': ('nano-capella-isolated-bootstrap-import', 'tests/capella/isolated_bootstrap_import.cpp'), 'bootstrapAudit': ('nano-capella-isolated-bootstrap-audit', 'tests/capella/isolated_bootstrap_audit.cpp'), 'stoppedCapture': ('nano-capella-isolated-stopped-capture', 'tests/capella/isolated_stopped_capture.cpp'), 'lifecycleTool': ('nano-capella-deposit-audit', 'tests/capella/deposit_audit.cpp'), 'lifecycleSubmit': ('nano-capella-lifecycle-submit', 'tests/multihost/lifecycle_submit.cpp')}
P = E / 'qualify-complete-nano-arm-root-attempt02-20261016.py'
raw = P.read_bytes()
assert hashlib.sha256(raw).hexdigest() == 'a73f0267359a4ac155b5a63a005385930b558a3408343622e8909931ffe92442' and len(raw) == 14095
m = types.ModuleType('retained_arm_runtime_functions')
m.__file__ = str(P)
exec(compile(raw, str(P), 'exec'), m.__dict__)  # __name__ is not __main__; no old main is called.
O = None
CREATED = False
END = time.monotonic() + 1200
ACTIVE = None
ENV = {'PATH': '/usr/bin:/bin', 'HOME': '/Users/boris', 'LANG': 'C', 'LC_ALL': 'C', 'GIT_OPTIONAL_LOCKS': '0', 'GIT_NO_REPLACE_OBJECTS': '1', 'GIT_NO_LAZY_FETCH': '1', 'GIT_TERMINAL_PROMPT': '0'}
os.umask(0o077)


def interrupted(*_):
    raise RuntimeError('ARM runtime relink interrupted')


for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
    signal.signal(sig, interrupted)


def pin(body):
    return {'sha256': hashlib.sha256(body).hexdigest(), 'bytes': len(body)}


def material(path):
    path = pathlib.Path(path)
    assert path.is_absolute() and path.resolve() == path
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        s = os.fstat(fd)
        assert stat.S_ISREG(s.st_mode) and s.st_nlink == 1 and 0 <= s.st_size <= 536870912
        row = {k: getattr(s, k) for k in m.F}
        chunks, total = [], 0
        while True:
            b = os.read(fd, min(1048576, 536870913 - total))
            if not b:
                break
            total += len(b)
            assert total <= s.st_size
            chunks.append(b)
        assert total == s.st_size and row == {k: getattr(os.fstat(fd), k) for k in m.F} == {k: getattr(path.lstat(), k) for k in m.F}
        body = b''.join(chunks)
        return dict(row, sha256=hashlib.sha256(body).hexdigest()), body
    finally:
        os.close(fd)


m.material = material


def js(path, known=None):
    assert pathlib.Path(path).lstat().st_size <= 16777216
    return json.loads(m.hold(path, known))


def git(*args):
    return subprocess.check_output(['/usr/bin/git', '-C', str(R), *args], env=ENV, timeout=30)


def save(leaf, value):
    assert CREATED and re.fullmatch(r'[A-Za-z0-9_.-]+', leaf) and leaf not in ('.', '..')
    m.save(O / leaf, value)


def stop_child(p):
    if p.poll() is None:
        try:
            os.killpg(p.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(p.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            p.wait(timeout=5)


def run(argv, leaf, seconds=30, wanted=0, cleanup=False):
    global ACTIVE
    assert CREATED and re.fullmatch(r'[A-Za-z0-9_.-]+', leaf)
    left = END - time.monotonic()
    assert cleanup or left > 65
    seconds = min(seconds, max(1, left - (0 if cleanup else 60)))
    save(leaf + '-argv.private.json', argv)
    start = time.monotonic()
    def limits():
        resource.setrlimit(resource.RLIMIT_FSIZE, (268435456, 268435456))
    with (O / (leaf + '-stdout.private.log')).open('xb') as out, (O / (leaf + '-stderr.private.log')).open('xb') as err:
        p = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=out, stderr=err, start_new_session=True, preexec_fn=limits, env=ENV)
        ACTIVE = p
        try:
            code = p.wait(timeout=seconds)
        finally:
            stop_child(p)
            ACTIVE = None
    event = {'step': leaf, 'exitCode': code, 'elapsedMs': int((time.monotonic() - start) * 1000)}
    save(leaf + '-exit.json', event)
    assert wanted is None or code == wanted
    return event


def stdout(leaf):
    return (O / (leaf + '-stdout.private.log')).read_bytes()


def stable(paths, known, leaf):
    start, since, previous, count, observations = time.monotonic(), time.monotonic(), None, 0, []
    while True:
        table = {str(p): material(p)[0] for p in paths}
        for path, row in table.items():
            assert {'sha256': row['sha256'], 'bytes': row['st_size']} == known[path]
        now = time.monotonic()
        assert now - start < 30
        observations.append({'elapsedMs': int((now - start) * 1000), 'table': table})
        if table == previous:
            count += 1
        else:
            count, since = 1, now
        if count >= 3 and now - since >= 2:
            break
        previous = table
        time.sleep(1)
    for path, row in table.items():
        m.hold(pathlib.Path(path), row)
    save(leaf + '.private.json', {'observations': observations, 'knownBytesMatched': True, 'oldAuthoritiesRefreshed': False})
    return table


def volume_locator(root):
    # This is a current locator, not recovered historical authority for this descriptor file.
    z = js(root / 'owned-output-volume.private.json')
    assert set(z) == {'volume', 'label', 'image'} and z['image'] == IMAGE
    assert re.fullmatch(r'nano-[A-Za-z0-9_.-]{1,150}', z['volume'])
    assert re.fullmatch(r'[A-Za-z0-9_.-]+=[A-Za-z0-9_.-]+', z['label'])
    return z


def binding(root, result_pin, binding_pin, binding_format):
    result = js(root / 'result-public.json', result_pin)
    assert result['passed'] is True and result['productionQualified'] is False
    assert {'sha256': result['softwareAdmissionSha256'], 'bytes': result['softwareAdmissionBytes']} == binding_pin
    value = js(root / 'outputs/software-admission.private.json', binding_pin)
    assert value['format'] == binding_format and value['oldAuthoritiesRefreshed'] is False
    return result, value


def main():
    global O, CREATED
    ap = argparse.ArgumentParser()
    ap.add_argument('--output-root', required=True)
    args = ap.parse_args()
    assert os.getuid() == 501 and os.getgid() == 20
    candidate = pathlib.Path(args.output_root)
    assert candidate.is_absolute() and candidate.parent.resolve() == candidate.parent and candidate.parent == E
    assert re.fullmatch(r'current-arm-runtime-relink-root-[A-Za-z0-9-]+', candidate.name) and not candidate.exists() and not candidate.is_symlink()
    current_head = git('rev-parse', 'HEAD').decode().strip()
    assert re.fullmatch(r'[0-9a-f]{40}', current_head) and git('status', '--porcelain') == b''
    assert git('merge-base', '--is-ancestor', REV, current_head) == b''
    assert git('diff', '--name-only', PREVIOUS, REV, '--', 'include', 'src').decode().splitlines() == ['src/consensus_capella_runtime.cpp']
    for rel in ('include', 'network', 'third_party', 'tests/support', 'tests/capella', 'tests/multihost'):
        assert git('rev-parse', REV + ':' + rel) == git('rev-parse', PREVIOUS + ':' + rel)
    worker = m.hold(WORKER, WORKER_PIN)
    assert worker.count(b'\nsub main {') == 1
    prefix = worker.split(b'\nsub main {', 1)[0]
    _, nb = binding(N, {'sha256': '9a701afead298f73014ab890f48eeaa9062168d2cbafd248d9282e21f2389f28', 'bytes': 1729}, {'sha256': 'c066e47cd802e4d77d02ac021b0790cf7745b2fc6c06c171227b10a333674920', 'bytes': 133230}, 'nano-current-core-linux-software-binding-v1')
    _, hb = binding(A, {'sha256': '5bf63c1289a9b83bc3a9988d171c1071614111754324a5296a9599336178bfcf', 'bytes': 7375}, {'sha256': '731e403d33f4984560ac39a881b4d41b62d1ec4cd5acc0a4d854483c273dd357', 'bytes': 235689}, 'nano-current-linux-selected-helper-software-binding-v1')
    _, cb = binding(C, {'sha256': '036e86a1a2b7087390a222249054cd7a8bbf2de984ae4d4dd0f5d079e69d7b58', 'bytes': 7710}, {'sha256': 'abf76536d3159dc04445528718df173aed751a4f1900a93065cacd0c5e36a6cc', 'bytes': 250014}, 'nano-linux-pruning-operation-software-binding-v1')
    assert {'sha256': cb['newCore']['sha256'], 'bytes': cb['newCore']['bytes']} == CORE
    def recipes(root, oldbinding):
        row = oldbinding['guestMaterials']['/controls/recipes.json']
        return js(root / 'controls/recipes.json', {'sha256': row['sha256'], 'bytes': row['bytes']})
    nr, hr = recipes(N, nb), recipes(A, hb)
    assert set(nr) == {'mainFlags', 'linkArgvTail', 'uid', 'gid', 'reuseMain'} and nr['uid'] == 501 and nr['gid'] == 20 and nr['reuseMain'] is False
    assert hr['format'] == 'nano-current-linux-selected-helpers-recipes-v1' and hr['imageId'] == IMAGE and hr['owner'] == 'mac-docker'
    original = js(ORIGIN / 'selected-descriptor-admission.private.json', {'sha256': '9a2caf6a466b7dd103c4100d29ed7bbe9df50f475bfd340d340ccb961ef1248a', 'bytes': 1920})
    closure = js(ORIGIN / 'selected-original-closure.private.json', original['materials']['originalClosure'])
    tool = js(E / 'fresh-linux-lifecycle-tool-u64-root-20261016/arm-02/new-software-admission.private.json', {'sha256': 'a4772c9bc694d8f43c08b52f92b3e7982b089563e89c5c4507fa01acc7032827', 'bytes': 298848})
    source = C / 'source'
    assert source.is_dir() and source.resolve() == source and stat.S_IMODE(source.lstat().st_mode) == 0o555
    selected = ['include', 'src', 'network', 'third_party', 'CMakeLists.txt', 'tests/support', 'tests/capella', 'tests/multihost', 'tests/test_capella_startup_deadline.cpp', 'tests/test_owned_copy_physical_admission.cpp', 'tools/storage-maintenance']
    # The retained projection contains only lifecycle_submit.cpp from this directory.
    selected.remove('tests/multihost')
    selected.append('tests/multihost/lifecycle_submit.cpp')
    fixture = 'tests/fixtures/capella-bootstrap-anchor-cb532/'
    fj = json.loads(git('show', REV + ':' + fixture + 'manifest.json'))
    selected += [fixture + 'manifest.json', *[fixture + fj['files'][k]['path'] for k in ('profile', 'initial', 'genesis', 'genesisState')]]
    wanted = {}
    source_listing = git('ls-tree', '-r', '-z', REV, '--', *selected)
    assert source_listing == git('ls-tree', '-r', '-z', current_head, '--', *selected)
    for entry in source_listing.split(b'\0'):
        if not entry:
            continue
        meta, rel = entry.split(b'\t')
        mode, kind, oid = meta.decode().split()
        assert mode in ('100644', '100755') and kind == 'blob'
        wanted[rel.decode()] = oid
    expected, empty = {}, set()
    for rel, oid in wanted.items():
        path = source / rel
        body = m.hold(path)
        assert hashlib.sha1(b'blob ' + str(len(body)).encode() + b'\0' + body).hexdigest() == oid and stat.S_IMODE(path.lstat().st_mode) == 0o444
        if body:
            expected['/work/' + rel] = pin(body)
        else:
            empty.add('/work/' + rel)
    assert empty == {f'/work/third_party/mcl/{x}/.emptydir' for x in ('bin', 'lib', 'obj')}
    # Compiler-reached original project bytes must still equal this immutable Git projection.
    project_joins = 0
    for oldbinding in (nb, hb):
        for guest, row in oldbinding['guestMaterials'].items():
            if guest.startswith('/work/') and guest != '/work/CMakeLists.txt':
                assert expected[os.path.normpath(guest)] == {'sha256': row['sha256'], 'bytes': row['bytes']}
                project_joins += 1
    mounts, guest_guards = [], {}
    def guard(guest, row):
        assert set(row) == {'material', 'sha256', 'bytes'} and len(row['material']) == 9
        assert guest not in guest_guards or guest_guards[guest] == row
        guest_guards[guest] = row
    # Only retained volume files have cross-container GNU9 authority here.
    # SDK overlay files and macOS bind mounts receive current guest rows in the
    # worker; their old cross-container device/inode rows are not reused.
    oldlibs = {p: z for p, z in hb['guestMaterials'].items() if p.startswith('/old/')}
    assert len(oldlibs) == 6
    for guest, row in oldlibs.items():
        candidates = [(p, z) for p, z in closure['files'].items() if z.get('sha256') == row['sha256'] and z.get('st_size') == row['bytes']]
        assert len(candidates) == 1
        path, oldrow = candidates[0]
        body = m.hold(pathlib.Path(path), oldrow)
        assert pin(body) == {'sha256': row['sha256'], 'bytes': row['bytes']}
        expected[guest] = pin(body)
        mounts += ['--mount', 'type=bind,src=' + path + ',dst=' + guest + ',readonly']
    guard('/baseline/libnano_core.a', cb['newCore'])
    main_obj, main_md = nb['mainObject'], nb['mainDependencyFile']
    guard('/inputs/nano/main.cpp.o', main_obj)
    guard('/inputs/nano/main.cpp.o.d', main_md)
    programs = [{'role': 'nano', 'source': '/work/src/main.cpp', 'compileFlags': nr['mainFlags'], 'linkPrefix': ['/usr/bin/clang++', '-O3', '-DNDEBUG'], 'linkTail': nr['linkArgvTail'], 'reuse': {'object': dict(path='/inputs/nano/main.cpp.o', **{k: main_obj[k] for k in ('sha256', 'bytes')}), 'md': dict(path='/inputs/nano/main.cpp.o.d', **{k: main_md[k] for k in ('sha256', 'bytes')}), 'dependencyPrefixMap': []}}]
    previous_programs = {x['role']: x for x in hb['programs']}
    assert set(previous_programs) == set(ROLES) - {'nano'}
    for oldrecipe in hr['programs']:
        role = oldrecipe['role']
        assert role in previous_programs and oldrecipe['source'] == '/work/' + ROLES[role][1]
        oldprogram = previous_programs[role]
        if role in ('bootstrapImport', 'lifecycleTool'):
            reuse = oldrecipe['reuse']
            assert reuse is not None
            if role == 'bootstrapImport':
                op = pathlib.Path(closure['buildRoot']) / pathlib.Path(closure['objectGuestPath']).relative_to('/outputs')
                dp = pathlib.Path(closure['originalMdPath'])
                m.hold(op, closure['files'][str(op)])
                m.hold(dp, closure['files'][str(dp)])
            else:
                op = pathlib.Path(tool['programs']['deposit_audit.cpp.o']['path'])
                dp = pathlib.Path(tool['programs']['deposit_audit.cpp.o.d']['path'])
                m.hold(op, tool['programs']['deposit_audit.cpp.o']['material'])
                m.hold(dp, tool['programs']['deposit_audit.cpp.o.d']['material'])
            for path, descriptor, oldrow in ((op, reuse['object'], oldprogram['object']), (dp, reuse['md'], oldprogram['dependencyFile'])):
                body = m.hold(path)
                assert pin(body) == {k: descriptor[k] for k in ('sha256', 'bytes')} == {k: oldrow[k] for k in ('sha256', 'bytes')}
                mounts += ['--mount', 'type=bind,src=' + str(path) + ',dst=' + descriptor['path'] + ',readonly']
        else:
            assert oldrecipe['reuse'] is None and oldprogram['objectReused'] is False
            objpath = '/inputs/helpers/' + role + '.cpp.o'
            mdpath = objpath + '.d'
            guard(objpath, oldprogram['object'])
            guard(mdpath, oldprogram['dependencyFile'])
            reuse = {'object': dict(path=objpath, **{k: oldprogram['object'][k] for k in ('sha256', 'bytes')}), 'md': dict(path=mdpath, **{k: oldprogram['dependencyFile'][k] for k in ('sha256', 'bytes')}), 'dependencyPrefixMap': []}
        p = dict(oldrecipe, reuse=reuse)
        assert p['linkTail'] == programs[0]['linkTail'] and p['linkPrefix'] == programs[0]['linkPrefix']
        programs.append(p)
    assert len(programs) == 6 and {x['role'] for x in programs} == set(ROLES)
    assert len(guest_guards) == 9  # Core, Main pair, and three retained helper pairs.
    recipe = {'format': 'nano-current-linux-runtime-relink-recipes-v1', 'owner': 'mac-docker', 'uid': 501, 'gid': 20, 'imageId': IMAGE, 'elfMachine': 183, 'sourceRevision': REV, 'programs': programs}
    volumes = {'nano': volume_locator(N), 'helpers': volume_locator(A), 'core': volume_locator(C)}
    cp = js(E / 'campaign-resource-guard-root-control-20261016/probe-only-config.private.json', {'sha256': 'fed2abc6d9fd4107be44b4a40e767c05745365515bbc3df35b81cfaa8d4d9209', 'bytes': 2490})['tools']['docker']
    toolrow = {k: int(v) for k, v in zip(m.F, cp['material'])}
    assert len(cp['material']) == 9
    toolrow['sha256'] = cp['sha256']
    m.hold(pathlib.Path(cp['path']), toolrow)
    docker = [cp['path'], '--host', 'unix:///Users/boris/.docker/run/docker.sock']
    candidate.mkdir(mode=0o700)
    O, CREATED = candidate, True
    m.OUT = O
    for leaf in ('controls', 'outputs', 'runtime-programs'):
        (O / leaf).mkdir(mode=0o700)
    bodies = {'worker.pl': worker, 'recipes.json': (json.dumps(recipe, sort_keys=True) + '\n').encode(), 'source-expected.private.json': (json.dumps(expected, sort_keys=True) + '\n').encode(), 'core-pin.json': (json.dumps(CORE, sort_keys=True) + '\n').encode(), 'retained-guest-materials.private.json': (json.dumps(guest_guards, sort_keys=True) + '\n').encode()}
    for leaf, body in bodies.items():
        with (O / 'controls' / leaf).open('xb') as f:
            f.write(body)
            f.flush()
            os.fsync(f.fileno())
        os.chmod(O / 'controls' / leaf, 0o444)
    os.chmod(O / 'controls', 0o555)
    name = 'nano-current-arm-runtime-' + uuid.uuid4().hex
    labelkey, label = 'nano.current.arm.runtime', 'nano.current.arm.runtime=' + name
    volume = name + '-outputs'
    def dk(leaf, args, seconds=30, wanted=0, cleanup=False):
        return run(docker + args, leaf, seconds, wanted, cleanup)
    first = None
    checker, command, guest_before, guest_after = None, None, None, False
    volume_created = False
    try:
        dk('image-inspect', ['image', 'inspect', IMAGE, '--format', '{{json .}}'])
        image = json.loads(stdout('image-inspect'))
        assert image['Id'] == IMAGE and image['Architecture'] == 'arm64' and image['Os'] == 'linux'
        for key, z in volumes.items():
            dk('retained-volume-' + key, ['volume', 'inspect', z['volume']])
            v = json.loads(stdout('retained-volume-' + key))
            k, value = z['label'].split('=', 1)
            assert len(v) == 1 and v[0]['Name'] == z['volume'] and (v[0].get('Labels') or {}).get(k) == value
        dk('initial-containers', ['ps', '-a', '--filter', 'name=^/' + name + '(-init)?$', '--format', '{{.ID}}'])
        assert not stdout('initial-containers').strip()
        dk('initial-volumes', ['volume', 'ls', '--format', '{{.Name}}'])
        assert volume not in stdout('initial-volumes').decode().splitlines()
        dk('create-volume', ['volume', 'create', '--label', label, volume])
        assert stdout('create-volume').decode().strip() == volume
        volume_created = True
        save('owned-output-volume.private.json', {'volume': volume, 'label': label, 'image': IMAGE})
        dk('inspect-new-volume', ['volume', 'inspect', volume])
        v = json.loads(stdout('inspect-new-volume'))
        assert len(v) == 1 and v[0]['Name'] == volume and (v[0].get('Labels') or {}).get(labelkey) == name
        common = ['--pull=never', '--network', 'none', '--read-only', '--cap-drop', 'ALL', '--security-opt', 'no-new-privileges']
        dk('initialize-volume', ['run', '--rm', '--name', name + '-init', '--label', label, *common, '--user', '0:0', '--cap-add', 'CHOWN', '--mount', 'type=volume,src=' + volume + ',dst=/outputs,volume-nocopy', '--entrypoint', '/bin/sh', IMAGE, '-ec', 'chmod 700 /outputs && chown 501:20 /outputs'])
        dk('start-container', ['run', '-d', '--rm', '--name', name, '--label', label, *common, '--user', '501:20', '--cpus', '2', '--memory', '3g', '--pids-limit', '128', '--tmpfs', '/tmp:rw,nosuid,nodev,size=128m', '--mount', 'type=bind,src=' + str(source) + ',dst=/work,readonly', '--mount', 'type=bind,src=' + str(O / 'controls') + ',dst=/controls,readonly', '--mount', 'type=volume,src=' + volumes['core']['volume'] + ',dst=/baseline,readonly,volume-nocopy', '--mount', 'type=volume,src=' + volumes['nano']['volume'] + ',dst=/inputs/nano,readonly,volume-nocopy', '--mount', 'type=volume,src=' + volumes['helpers']['volume'] + ',dst=/inputs/helpers,readonly,volume-nocopy', '--mount', 'type=volume,src=' + volume + ',dst=/outputs,volume-nocopy', *mounts, '--workdir', '/outputs', '--entrypoint', '/bin/sh', IMAGE, '-ec', 'exec sleep 1500'])
        cid = stdout('start-container').decode().strip()
        assert re.fullmatch(r'[0-9a-f]{64}', cid)
        dk('inspect-container', ['inspect', cid])
        a = json.loads(stdout('inspect-container'))
        assert len(a) == 1 and a[0]['Id'] == cid and a[0]['Name'] == '/' + name and a[0]['Image'] == IMAGE and a[0]['Config']['Image'] == IMAGE and a[0]['Config']['User'] == '501:20' and a[0]['Config']['Labels'].get(labelkey) == name
        actual = {x['Destination']: x for x in a[0]['Mounts']}
        for key, dest in (('core', '/baseline'), ('nano', '/inputs/nano'), ('helpers', '/inputs/helpers')):
            assert actual[dest]['Type'] == 'volume' and actual[dest]['Name'] == volumes[key]['volume'] and actual[dest]['RW'] is False
        assert actual['/outputs']['Type'] == 'volume' and actual['/outputs']['Name'] == volume and actual['/outputs']['RW'] is True
        command = ['exec', '--user', '501:20', name, '/usr/bin/env', '-i', 'PATH=/usr/bin:/bin', 'HOME=/tmp', 'TMPDIR=/tmp', 'LANG=C', 'LC_ALL=C', '/usr/bin/perl']
        prime = r"use Digest::SHA ();use JSON::PP ();my @r;for my $p(@ARGV){open(my $f,'<',$p)or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);push @r,{sha256=>$d->hexdigest,bytes=>-s $p};}print JSON::PP->new->canonical->encode(\@r);"
        names = sorted(bodies)
        dk('prime-controls', command + ['-e', prime, *['/controls/' + leaf for leaf in names]])
        assert json.loads(stdout('prime-controls')) == [pin(bodies[leaf]) for leaf in names]
        stable([O / 'controls' / leaf for leaf in names], {str(O / 'controls' / leaf): pin(bodies[leaf]) for leaf in names}, 'new-controls-first-admission')
        checker = prefix + b"\n%ENV=(PATH=>'/usr/bin:/bin',HOME=>'/outputs',TMPDIR=>'/tmp',LANG=>'C',LC_ALL=>'C');my $old=$J->decode(read_bytes('/controls/retained-guest-materials.private.json'));need(ref($old) eq 'HASH'&&keys(%$old)>0);for my $path(sort keys %$old){need(same(material($path),$old->{$path}));}print $J->encode({passed=>$yes,retainedRows=>scalar(keys %$old)}).qq(\\n);\n"
        m.fence()
        save('selected-host-inputs-before-worker.private.json', m.HELD)
        dk('old-guest-before', command + ['-e', checker.decode()], 180)
        before = json.loads(stdout('old-guest-before'))
        assert before == {'passed': True, 'retainedRows': len(guest_guards)}
        guest_before = before
        dk('worker-syntax', command + ['-c', '/controls/worker.pl'])
        ev = dk('runtime-relink-worker', command + ['/controls/worker.pl', '--current-core-runtime-relink'], 950, None)
        # Preserve logs/refusal and the original exit before interpreting worker success.
        reader = r"use strict;use warnings;opendir(my $d,'/outputs')or die;my @f=sort grep {/\A[A-Za-z0-9_.-]+\z/&&-f('/outputs/'.$_)&&!-l('/outputs/'.$_)}readdir($d);die unless @f<=256;my $sum=0;for(@f){my $s=-s('/outputs/'.$_);die unless $s<=67108864;$sum+=$s;}die if $sum>251658240;if(@f){exec '/usr/bin/tar','-cf','-','-C','/outputs','--',@f;}else{exec '/usr/bin/tar','-cf','-','--files-from','/dev/null';}die;"
        dk('collect-output', command + ['-e', reader], 60)
        data = stdout('collect-output')
        assert len(data) <= 268435456
        (O / 'artifacts.private.tar').write_bytes(data)
        seen, total = set(), 0
        with tarfile.open(O / 'artifacts.private.tar', 'r:') as tf:
            for z in tf:
                assert z.isfile() and re.fullmatch(r'[A-Za-z0-9_.-]+', z.name) and z.name not in ('.', '..') and z.name not in seen and z.size <= 67108864
                seen.add(z.name)
                total += z.size
                assert len(seen) <= 256 and total <= 268435456
                body = tf.extractfile(z).read(z.size + 1)
                assert len(body) == z.size
                with (O / 'outputs' / z.name).open('xb') as f:
                    f.write(body)
        dk('old-guest-after', command + ['-e', checker.decode()], 180)
        assert json.loads(stdout('old-guest-after')) == before
        guest_after = True
        assert ev['exitCode'] == 0, 'Worker refused; original exit and artifacts retained'
        result = js(O / 'outputs/result-public.json')
        assert result == json.loads(stdout('runtime-relink-worker')) and result['passed'] is True and result['productionQualified'] is False
        assert result['owner'] == 'mac-docker' and result['imageId'] == IMAGE and result['sourceRevisionDeclared'] == REV
        assert all(result[k] == 6 for k in ('selectedPrograms', 'independentMCommands', 'linkCommands', 'loaderCommands', 'argumentRefusals'))
        assert result['smokeCommands'] == 7 and result['helpCommands'] == 1 and result['compiledTranslationUnits'] == 0 and result['archiveUpdates'] == 0
        assert all(result[k] == 0 for k in ('p2pMCommands', 'p2pLinks', 'p2pProgramRuns', 'coreCompilations'))
        software_binding = js(O / 'outputs/software-admission.private.json', {'sha256': result['softwareAdmissionSha256'], 'bytes': result['softwareAdmissionBytes']})
        assert software_binding['coreArchive'] == cb['newCore']
        assert {x['role'] for x in result['programs']} == set(ROLES)
        for public, private in zip(result['programs'], software_binding['programs']):
            assert public['role'] == private['role'] and public['target'] == ROLES[public['role']][0]
            assert public['softwareSha256'] == private['software']['sha256'] and public['softwareBytes'] == private['software']['bytes']
            r = next(p['reuse'] for p in programs if p['role'] == public['role'])
            for field, descriptor in (('object', r['object']), ('dependencyFile', r['md'])):
                assert {k: private[field][k] for k in ('sha256', 'bytes')} == {k: descriptor[k] for k in ('sha256', 'bytes')}
                if descriptor['path'] in guest_guards:
                    assert private[field] == guest_guards[descriptor['path']]
            assert public['objectReused'] is True and public['normalizedMEqualsRemappedMd'] is True
        assets = {x['target']: {'sha256': x['softwareSha256'], 'bytes': x['softwareBytes']} for x in result['programs']}
        for leaf, known in assets.items():
            body = (O / 'outputs' / leaf).read_bytes()
            assert pin(body) == known
            with (O / 'runtime-programs' / leaf).open('xb') as f:
                f.write(body)
                f.flush()
                os.fsync(f.fileno())
            os.chmod(O / 'runtime-programs' / leaf, 0o555)
        dk('collect-known-core', command + ['-e', r"exec '/usr/bin/tar','-cf','-','-C','/baseline','libnano_core.a';die;"], 60)
        (O / 'core-copy.private.tar').write_bytes(stdout('collect-known-core'))
        with tarfile.open(O / 'core-copy.private.tar', 'r:') as tf:
            members = tf.getmembers()
            assert len(members) == 1 and members[0].isfile() and members[0].name == 'libnano_core.a' and members[0].size == CORE['bytes']
            body = tf.extractfile(members[0]).read(CORE['bytes'] + 1)
            assert pin(body) == CORE
        with (O / 'runtime-programs/libnano_core.a').open('xb') as f:
            f.write(body)
            f.flush()
            os.fsync(f.fileno())
        os.chmod(O / 'runtime-programs/libnano_core.a', 0o444)
        assets['libnano_core.a'] = CORE
        stable([O / 'runtime-programs' / leaf for leaf in assets], {str(O / 'runtime-programs' / leaf): row for leaf, row in assets.items()}, 'runtime-bundle-first-admission')
        os.chmod(O / 'runtime-programs', 0o555)
        result.update(rootGitHead=current_head, rootGitSourceRevision=REV, rootGitProjectionFilesVerified=len(wanted), rootProjectProducerSourceJoins=project_joins, rootOldVolumeGuestRowsRetained=len(guest_guards), rootOldVolumeGuestBeforeAfterEqual=True, rootOriginalHostRowsUnchanged=True, rootOldSdkGuestNineReusedAcrossContainers=False, rootMacBindGuestNineRecovered=False, rootCurrentImageSdkProspectivelyAdmitted=True, rootImageIdentityVerified=True, rootProgramObjectsAndMdBoundToRetainedProducer=True, runtimeBundlePrograms=6, runtimeBundleCoreFiles=1, runtimeBundleFirstSamplesAtLeast=3, runtimeBundleSampleSpanMsAtLeast=2000, volumeLocatorHistoricalFileIdentityRecovered=False, p2pQualificationReusedByThisOperator=False, wholeCmakeConfigureQualified=False, wholeSdkQualified=False, campaignAdmission=False, productionQualified=False)
    except BaseException as ex:
        first = ex
        raise
    finally:
        if guest_before is not None and not guest_after:
            try:
                dk('old-guest-after-failure', command + ['-e', checker.decode()], 180, cleanup=True)
                assert json.loads(stdout('old-guest-after-failure')) == guest_before
                save('old-guest-failure-fence-public.json', {'passed': True, 'retainedRows': len(guest_guards), 'firstFailurePreserved': first is not None})
            except BaseException as ex:
                try:
                    save('old-guest-failure-fence-public.json', {'passed': False, 'errorType': type(ex).__name__, 'firstFailurePreserved': first is not None})
                except BaseException:
                    pass
                if first is None:
                    first = ex
        try:
            for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
                signal.signal(sig, signal.SIG_IGN)
            # No old volume is removed. Only containers bearing this exact nonce are eligible.
            dk('cleanup-discovery', ['ps', '-a', '--no-trunc', '--filter', 'label=' + label, '--format', '{{.ID}}'], 15, cleanup=True)
            ids = stdout('cleanup-discovery').decode().splitlines()
            assert len(ids) <= 2 and all(re.fullmatch(r'[0-9a-f]{64}', x) for x in ids)
            for i, cid in enumerate(ids):
                dk('cleanup-inspect-' + str(i), ['inspect', cid], 10, cleanup=True)
                a = json.loads(stdout('cleanup-inspect-' + str(i)))
                assert len(a) == 1 and a[0]['Id'] == cid and a[0]['Name'] in ('/' + name, '/' + name + '-init') and a[0]['Image'] == IMAGE and a[0]['Config']['Image'] == IMAGE and a[0]['Config']['Labels'].get(labelkey) == name
                dk('cleanup-remove-' + str(i), ['rm', '-f', cid], 10, cleanup=True)
            dk('cleanup-final', ['ps', '-a', '--no-trunc', '--filter', 'label=' + label, '--format', '{{.ID}}'], 10, cleanup=True)
            assert not stdout('cleanup-final').strip()
            save('cleanup-public.json', {'passed': True, 'ownedContainersAbsent': True, 'newOutputVolumeRetained': volume_created, 'oldVolumesRemoved': 0, 'firstFailurePreserved': first is not None, 'imagesBuilt': 0})
        except BaseException as ex:
            try:
                save('cleanup-failure-public.json', {'passed': False, 'errorType': type(ex).__name__, 'firstFailurePreserved': first is not None})
            except BaseException:
                pass
            if first is None:
                raise
    m.fence()
    assert git('rev-parse', 'HEAD').decode().strip() == current_head and git('status', '--porcelain') == b''
    if first is not None:
        raise first
    save('result-public.json', result)
    print(json.dumps(result, sort_keys=True), flush=True)


if __name__ == '__main__':
    failed = False
    try:
        main()
    except BaseException:
        failed = True
        raise
    finally:
        try:
            m.fence()
            if CREATED:
                save('selected-host-final-fence-public.json', {'passed': True, 'selectedFiles': len(m.HELD), 'firstFailurePreserved': failed, 'oldAuthoritiesRefreshed': False})
        except BaseException:
            if CREATED:
                try:
                    save('selected-host-final-fence-failure-public.json', {'passed': False, 'firstFailurePreserved': failed})
                except BaseException:
                    pass
            if not failed:
                raise
