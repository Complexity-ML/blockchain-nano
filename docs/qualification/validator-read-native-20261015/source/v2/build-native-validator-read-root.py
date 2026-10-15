#!/usr/bin/env python3
"""ROOT-only five-TU native API qualification from previously admitted recipes.

No operation on import. No configure, full build, Docker, Git, SSH, private
campaign, old test replay, node runtime or anti-double-signature database access.
All original inputs remain strict; only the three public API source byte rows
supersede their historical source equivalents in a separate current source root.
"""
import argparse, hashlib, json, os, pathlib, re, shlex, signal, stat, subprocess, sys, time

F = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid', 'st_size', 'st_mtime_ns', 'st_ctime_ns')
HELD, ALIASES, NEW, EVENTS = {}, {}, {}, []
START, DEADLINE, PHASE, CONTROL = time.monotonic(), None, 'admission', None
SOURCE_MANIFEST_SHA = '54ecd15d7b3c95473f19013ea3cab41ee0a2f072cc9621c83775e84494bd52c9'
SOURCE_MANIFEST_BYTES = 3659
OLD_CORE_SHA = '25192162e6f82adcd8921470ce28ab1565aafab6edc361eb11c93d559bdd4908'
OLD_CORE_BYTES = 29236208
OLD_PROOF_SHA = 'cb218cd8f1290b195164990e259ea2edf5f03f9de383dc4cf78f7b43677b9131'
OLD_PROOF_BYTES = 1481
OLD_MATERIAL_SHA = '25c5441c78a55f88f4d4c4255d498f7ceb174ff075833164f8be7e1e2679a633'
OLD_MATERIAL_BYTES = 6869424
API_FILES = {
    'src/capella_beacon_api.cpp': {'sha256': 'c4dd87d9b678b5f28fc3bcf1875469205dca05ddc072893f5f7e39dac5095b1d', 'bytes': 15278},
    'src/beacon_validator_json.hpp': {'sha256': '6ec4ec77490bc9447e6066c2193e02c303bcc39a7b388fbd7dee689cee5bc4c9', 'bytes': 1410},
    'tests/test_capella_validator_read.cpp': {'sha256': '8ac5c59067d85fa8cf8d127968bae94762b4a95841421dcbcdf01ab5c9911347', 'bytes': 10367},
}
ROLES = {
    'nano-capella-validator-read-tests': ('tests/test_capella_validator_read.cpp', 'tests/test_capella_duty_availability.cpp', 'nano-capella-duty-availability-tests'),
    'nano': ('src/main.cpp', 'src/main.cpp', 'nano'),
    'nano-capella-isolated-bootstrap-audit': ('tests/capella/isolated_bootstrap_audit.cpp', 'tests/capella/isolated_bootstrap_audit.cpp', 'nano-capella-isolated-bootstrap-audit'),
    'nano-capella-isolated-stopped-capture': ('tests/capella/isolated_stopped_capture.cpp', 'tests/capella/isolated_stopped_capture.cpp', 'nano-capella-isolated-stopped-capture'),
}
FIXTURES = {
    'tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json',
    'tests/fixtures/capella-bootstrap-anchor-cb532/profile.json',
    'tests/fixtures/capella-bootstrap-anchor-cb532/genesis-state.ssz',
    'tests/fixtures/capella-bootstrap-anchor-cb532/genesis.json',
    'tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz',
}


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def exact(value, fields, reason):
    need(isinstance(value, dict) and set(value) == set(fields), reason)


def physical(value, exists=True):
    need(isinstance(value, str) and 0 < len(value) < 4096 and not re.search(r'[\x00\r\n\\]', value), 'physical locator shape')
    p = pathlib.Path(value)
    need(p.is_absolute() and str(p) == os.path.normpath(value) and p != pathlib.Path('/'), 'absolute normalized locator')
    need(p.resolve(strict=exists) == p, 'canonical physical locator')
    return p


def row(s):
    return {k: getattr(s, k) for k in F}


def material(path):
    p = pathlib.Path(path)
    fd = os.open(p, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        a = os.fstat(fd)
        need(stat.S_ISREG(a.st_mode) and 0 <= a.st_size <= 268435456, 'bounded regular input')
        digest, chunks, total = hashlib.sha256(), [], 0
        while True:
            b = os.read(fd, 1048576)
            if not b:
                break
            total += len(b)
            need(total <= a.st_size, 'input read bound')
            digest.update(b); chunks.append(b)
        need(total == a.st_size and row(a) == row(os.fstat(fd)) == row(p.lstat()), 'FD and path nine fields moved')
        return dict(row(a), sha256=digest.hexdigest()), b''.join(chunks)
    finally:
        os.close(fd)


def byte_pin(pin):
    if 'st_dev' in pin:
        exact(pin, (*F, 'sha256'), 'old nine-field pin shape')
        need(all(type(pin[k]) is int for k in F), 'old nine-field value types')
        return {'sha256': pin['sha256'], 'bytes': pin['st_size']}
    need(isinstance(pin, dict) and set(pin).issuperset({'sha256', 'bytes'}), 'external known-byte pin')
    need(re.fullmatch('[0-9a-f]{64}', pin['sha256']) and type(pin['bytes']) is int and 0 <= pin['bytes'] <= 268435456, 'known-byte values')
    return {k: pin[k] for k in ('sha256', 'bytes')}


def hold(path, pin):
    p = physical(str(path))
    need(isinstance(pin, dict), 'external authority is mandatory before admission')
    observed, raw = material(p)
    if 'st_dev' in pin:
        byte_pin(pin); need(observed == pin, 'previously recorded nine-field authority differs')
    else:
        known = byte_pin(pin)
        need(observed['sha256'] == known['sha256'] and observed['st_size'] == known['bytes'], 'previously frozen byte authority differs')
    need(str(p) not in HELD or HELD[str(p)] == observed, 'held nine-field authority changed')
    HELD[str(p)] = observed
    return raw


def artifact(a):
    exact(a, ('path', 'sha256', 'bytes'), 'Artifact fields')
    byte_pin(a)
    return physical(a['path'])


def document(a):
    return json.loads(hold(artifact(a), a))


def alias_admit(path, target):
    p = pathlib.Path(path)
    need(p.is_absolute() and str(p) == os.path.normpath(str(p)) and str(p.resolve(strict=True)) == target, 'old alias target')
    a = p.lstat()
    observed = {'resolved': target, 'material': row(a), 'link': os.readlink(p) if stat.S_ISLNK(a.st_mode) else None}
    need(str(p) not in ALIASES or ALIASES[str(p)] == observed, 'previous alias authority changed')
    ALIASES[str(p)] = observed


def output(path):
    p = physical(str(path))
    observed, raw = material(p)
    need(observed['st_nlink'] == 1 and observed['st_uid'] == 501, 'owned new output regular file')
    pin = {'sha256': observed['sha256'], 'bytes': observed['st_size']}
    need(str(p) not in NEW or NEW[str(p)] == pin, 'new output byte commitment changed')
    NEW[str(p)] = pin
    return raw


def write_new(path, raw, track=True):
    need(isinstance(raw, bytes), 'new bytes required')
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    try:
        os.fchmod(fd, 0o600)
        view = memoryview(raw)
        while view:
            n = os.write(fd, view); need(n > 0, 'new file write'); view = view[n:]
        os.fsync(fd)
    finally:
        os.close(fd)
    parent = os.open(pathlib.Path(path).parent, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        os.fsync(parent)
    finally:
        os.close(parent)
    observed, actual = material(path)
    need(actual == raw and observed['st_nlink'] == 1 and observed['st_uid'] == 501 and stat.S_IMODE(observed['st_mode']) == 0o600, 'new file bytes and ownership')
    if track:
        output(path)


def publish(path, value):
    write_new(path, (json.dumps(value, sort_keys=True, separators=(',', ':')) + '\n').encode())


def fence():
    for path, pin in list(HELD.items()):
        hold(path, pin)
    for path, pin in ALIASES.items():
        p = pathlib.Path(path); a = p.lstat()
        need(str(p.resolve(strict=True)) == pin['resolved'] and row(a) == pin['material'] and (os.readlink(p) if stat.S_ISLNK(a.st_mode) else None) == pin['link'], 'original alias nine fields changed')
    for path in list(NEW):
        output(path)


def run(name, argv, cwd, env, timeout=180):
    global PHASE
    PHASE = name
    remaining = DEADLINE - time.monotonic()
    need(remaining > 0 and 0 < timeout <= 300, 'finite remaining controller deadline')
    duration = min(timeout, remaining)
    publish(CONTROL / (name + '-argv.private.json'), {'argv': argv, 'cwd': str(cwd), 'maximumSeconds': duration})
    paths = [CONTROL / (name + '-stdout.private.log'), CONTROL / (name + '-stderr.private.log')]
    descriptors = [os.open(p, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600) for p in paths]
    started, child, expired, overflow, code = time.monotonic(), None, False, False, None
    try:
        child = subprocess.Popen(argv, cwd=cwd, env=env, stdout=descriptors[0], stderr=descriptors[1], start_new_session=True)
        while True:
            overflow = any(os.fstat(fd).st_size > 33554432 for fd in descriptors)
            expired = time.monotonic() - started >= duration
            if overflow or expired:
                try:
                    os.killpg(child.pid, signal.SIGTERM); child.wait(timeout=5)
                except (ProcessLookupError, subprocess.TimeoutExpired):
                    try: os.killpg(child.pid, signal.SIGKILL)
                    except ProcessLookupError: pass
                    child.wait(timeout=5)
                code = child.returncode; break
            try:
                code = child.wait(timeout=min(0.1, max(0.001, duration - (time.monotonic() - started)))); break
            except subprocess.TimeoutExpired:
                pass
    except BaseException:
        if child is not None and child.poll() is None:
            try: os.killpg(child.pid, signal.SIGTERM); child.wait(timeout=5)
            except (ProcessLookupError, subprocess.TimeoutExpired):
                try: os.killpg(child.pid, signal.SIGKILL)
                except ProcessLookupError: pass
                child.wait(timeout=5)
        raise
    finally:
        overflow = overflow or any(os.fstat(fd).st_size > 33554432 for fd in descriptors)
        for fd in descriptors:
            os.fsync(fd); os.close(fd)
    event = {'phase': name, 'exitCode': code, 'timedOut': expired, 'logBoundExceeded': overflow, 'elapsedMs': int((time.monotonic() - started) * 1000)}
    EVENTS.append(event); publish(CONTROL / (name + '-exit.json'), event)
    for p in paths: output(p)
    print(json.dumps(event), flush=True)
    need(code == 0 and not expired and not overflow and time.monotonic() < DEADLINE, 'bounded build command refused')


def dependency_set(raw, cwd):
    text = raw.decode().replace('\\\n', ' ')
    need(':' in text, 'real dependency rule')
    found = {os.path.normpath(str(pathlib.Path(x) if pathlib.Path(x).is_absolute() else cwd / x)) for x in shlex.split(text.split(':', 1)[1])}
    need(20 < len(found) < 5000 and all(pathlib.Path(p).is_absolute() for p in found), 'bounded actual dependency closure')
    return found


def archive_members(raw):
    need(raw.startswith(b'!<arch>\n') and len(raw) <= 268435456, 'bounded BSD archive')
    at, found = 8, {}
    while at < len(raw):
        header = raw[at:at + 60]
        need(len(header) == 60 and header[58:60] == b'`\n', 'archive member header')
        size = int(header[48:58].strip())
        need(0 <= size <= len(raw) - at - 60, 'archive member byte bound')
        name, body = header[:16].decode('ascii').strip(), raw[at + 60:at + 60 + size]
        if name.startswith('#1/'):
            n = int(name[3:]); need(0 < n <= len(body), 'BSD extended name')
            name, body = body[:n].rstrip(b'\0').decode(), body[n:]
        else:
            name = name.rstrip('/')
        if name not in ('__.SYMDEF', '__.SYMDEF SORTED', '__.SYMDEF_64', '__.SYMDEF_64 SORTED'):
            need(name.endswith('.cpp.o') and '/' not in name and name not in found, 'unique Core object member')
            found[name] = {'sha256': hashlib.sha256(body).hexdigest(), 'bytes': len(body)}
        at += 60 + size + size % 2
    need(at == len(raw) and len(found) == 143, 'exact143 Core members')
    return found


def main(args):
    global DEADLINE, CONTROL
    DEADLINE = START + 1100
    def interrupted(_signal, _frame):
        raise InterruptedError('native build interrupted')
    signal.signal(signal.SIGTERM, interrupted)
    need(sys.platform == 'darwin' and os.uname().machine == 'arm64' and os.getuid() == 501 and os.getgid() == 20, 'actual native Mac arm64 owner501:20')
    c = json.loads(hold(physical(args.inputs), {'sha256': args.sha256, 'bytes': args.bytes}))
    exact(c, ('format', 'controller', 'sourceBaseRevision', 'buildRoot', 'sourceRoot', 'currentSourceRoot', 'controlRoot', 'oldConfig', 'oldAdmission', 'nativeSdkAuthority', 'oldCoreQualification', 'sourceManifest', 'currentConsumerSources', 'runtimePublicFixtures', 'linkRecipes', 'archiveRecipe', 'tools'), 'exact native input17 fields')
    need(c['format'] == 'nano-cross-arch-validator-native-build-input-v1' and re.fullmatch('[0-9a-f]{40}', c['sourceBaseRevision']), 'native input format and source revision')
    need(artifact(c['controller']) == pathlib.Path(__file__).absolute(), 'current controller locator')
    hold(pathlib.Path(__file__).absolute(), c['controller'])
    b, s, current, out = (physical(c[k]) for k in ('buildRoot', 'sourceRoot', 'currentSourceRoot', 'controlRoot'))
    roots = (b, s, current, out)
    need(all(x != y and not x.is_relative_to(y) and not y.is_relative_to(x) for i, x in enumerate(roots) for y in roots[i + 1:]), 'independent old/new roots')
    need(out.is_dir() and out.stat().st_uid == 501 and stat.S_IMODE(out.stat().st_mode) == 0o700 and set(p.name for p in out.iterdir()) == {'inputs.private.json'} and physical(args.inputs) == out / 'inputs.private.json', 'fresh owned0700 control root')
    CONTROL = out
    cfg, old, sdk = document(c['oldConfig']), document(c['oldAdmission']), document(c['nativeSdkAuthority'])
    exact(old, ('format', 'files', 'aliases', 'provenance'), 'explicit original-record native authority projection')
    need(old['format'] == 'nano-root-historical-native-build-authority-projection-v1', 'known native authority projection format')
    exact(old['provenance'], ('originalInputs', 'nativeSdkAuthority', 'oldConfig'), 'projection provenance descriptors')
    need(old['provenance']['nativeSdkAuthority'] == c['nativeSdkAuthority'] and old['provenance']['oldConfig'] == c['oldConfig'], 'projection old SDK and config origin')
    original_ref = old['provenance']['originalInputs']
    need(original_ref['sha256'] == OLD_MATERIAL_SHA and original_ref['bytes'] == OLD_MATERIAL_BYTES, 'exact original material-map authority')
    original_material = document(original_ref)
    need(isinstance(original_material, dict) and old['files'] == original_material and old['aliases'] == sdk.get('sdkAliases'), 'projection must preserve original records and old SDK aliases exactly')
    need(0 < len(old['files']) <= 65536 and isinstance(cfg.get('projectFiles'), dict) and 0 < len(cfg['projectFiles']) <= 20000, 'old input/source namespaces')
    old_pins, project_pins = old['files'], cfg['projectFiles']

    def prior(path, fallback=None):
        p = physical(str(path)); pin = old_pins.get(str(p))
        if pin is None: pin = fallback
        need(isinstance(pin, dict), 'old authority missing; no fresh hash fallback')
        return hold(p, pin)

    need(sdk.get('existingPinsNineFieldsAndBytesVerified') is True and sdk.get('newSdkInputsAutomaticallyRefreshed') is False and isinstance(sdk.get('sdkFiles'), dict) and isinstance(sdk.get('sdkAliases'), dict), 'existing native SDK authority contract')
    for path, pin in sdk['sdkFiles'].items():
        need('st_dev' in pin, 'SDK requires recorded full nine fields')
        hold(physical(path), pin)
    for path, target in sdk['sdkAliases'].items(): alias_admit(path, target)
    if 'sdkRootLocator' in sdk:
        alias_admit(sdk['sdkRootLocator'], sdk['sdkRootResolved'])
    exact(c['tools'], ('compiler', 'archiver', 'ranlib', 'linker'), 'four native tool locators')

    def sdk_path(value):
        need(isinstance(value, str) and pathlib.Path(value).is_absolute(), 'SDK absolute input')
        p = pathlib.Path(value); resolved = str(p.resolve(strict=True))
        need(resolved in sdk['sdkFiles'], 'tool/header/library absent from existing SDK authority')
        if str(p) != resolved:
            need(sdk['sdkAliases'].get(str(p)) == resolved or str(p) == sdk.get('sdkRootLocator'), 'unrecorded SDK alias refused')
            alias_admit(str(p), resolved)
        hold(physical(resolved), sdk['sdkFiles'][resolved])
        return value

    for value in c['tools'].values(): sdk_path(value)
    proof = document(c['oldCoreQualification'])
    need(c['oldCoreQualification']['sha256'] == OLD_PROOF_SHA and c['oldCoreQualification']['bytes'] == OLD_PROOF_BYTES and proof['format'] == 'nano-historical-beacon-native-build-public-counts-v1' and proof['status'] == 'PASS_SCOPED_CURRENT_NATIVE_BUILD' and proof['archiveMembers'] == 143 and proof['oldInputsNineFieldsAndSha256Unchanged'] is True and proof['sdkPinsAutomaticallyRefreshed'] is False and proof['softwareArtifacts']['libnano_core.a'] == {'sha256': OLD_CORE_SHA, 'bytes': OLD_CORE_BYTES}, 'external old native Core qualification')
    old_core_path = b / 'libnano_core.a'
    old_core = prior(old_core_path, cfg['coreArchive'])
    need(hashlib.sha256(old_core).hexdigest() == OLD_CORE_SHA and len(old_core) == OLD_CORE_BYTES, 'exact externally qualified old Core')
    before = archive_members(old_core)
    need('capella_beacon_api.cpp.o' in before, 'original API member')
    need(c['sourceManifest']['sha256'] == SOURCE_MANIFEST_SHA and c['sourceManifest']['bytes'] == SOURCE_MANIFEST_BYTES, 'exact public API source manifest')
    source_manifest = document(c['sourceManifest'])
    need(len(source_manifest['files']) == 3 and {r['path'] for r in source_manifest['files']} == set(API_FILES), 'three exact API source rows')
    for item in source_manifest['files']:
        need(byte_pin(item['source']) == API_FILES[item['path']], 'public API row byte binding')
        hold(artifact(item['source']), item['source']); hold(current / item['path'], API_FILES[item['path']])
    exact(c['currentConsumerSources'], tuple(k for k in ROLES if k != 'nano-capella-validator-read-tests'), 'three current runtime consumer pins')
    for role, ref in c['currentConsumerSources'].items():
        relative = ROLES[role][0]
        need(artifact(ref) == current / relative and relative in project_pins and byte_pin(ref) == byte_pin(project_pins[relative]), 'current consumer must equal qualified historical source bytes')
        hold(current / relative, ref)
    need(isinstance(c['runtimePublicFixtures'], list) and len(c['runtimePublicFixtures']) == 5, 'exact five public runtime fixtures')
    fixture_rows = {}
    for ref in c['runtimePublicFixtures']:
        exact(ref, ('relative', 'sha256', 'bytes'), 'public fixture descriptor')
        need(ref['relative'] in FIXTURES and ref['relative'] not in fixture_rows, 'fixed public fixture leaf')
        p = s / ref['relative']; prior(p, project_pins.get(ref['relative'])); hold(p, byte_pin(ref)); fixture_rows[ref['relative']] = ref
    need(set(fixture_rows) == FIXTURES and fixture_rows['tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json']['sha256'] == 'ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6' and fixture_rows['tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz']['sha256'] == 'f5a464175565a8ed6a74790a60b84477c16a5b1bf71ece55736f192142cbca3d', 'literal focused fixture identities')
    fixture_manifest = json.loads(prior(s / 'tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json', project_pins.get('tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json')))
    for kind, leaf in (('profile', 'profile.json'), ('genesisState', 'genesis-state.ssz'), ('genesis', 'genesis.json')):
        ref = fixture_rows['tests/fixtures/capella-bootstrap-anchor-cb532/' + leaf]
        need(fixture_manifest['files'][kind] == {'path': leaf, 'sha256': ref['sha256'], 'bytes': ref['bytes']}, 'runtime manifest selected leaves')
    recipes = json.loads(prior(b / 'compile_commands.json', cfg['canonicalCompileCommands']))
    need(isinstance(recipes, list), 'canonical compiler database')

    def recipe(relative):
        source = str(s / relative); found = [r for r in recipes if r['file'] == source]
        need(len(found) == 1 and found[0]['directory'] == str(b), 'unique original native compile recipe')
        argv = found[0].get('arguments') or shlex.split(found[0]['command'])
        need(isinstance(argv, list) and all(isinstance(x, str) for x in argv) and argv.count('-o') == argv.count('-c') == argv.count(source) == 1 and argv[0] == c['tools']['compiler'], 'native compile template shape')
        need(not any(x in argv for x in ('-M', '-MM', '-MD', '-MMD', '-MF', '-MT', '-MQ', '-MP')), 'no inherited dependency output')
        sdk_path(argv[0])
        return list(argv)

    def current_compile(actual, template):
        argv = recipe(template)
        argv[argv.index(str(s / template))] = str(current / actual)
        argv.insert(1, '-iquote' + str((s / template).parent))
        return argv

    compiles = {'core': current_compile('src/capella_beacon_api.cpp', 'src/capella_beacon_api.cpp')}
    for role, (actual, template, _) in ROLES.items(): compiles[role] = current_compile(actual, template)
    test_macros = [x for x in compiles['nano-capella-validator-read-tests'] if x.startswith('-DNANO_TEST_SOURCE=')]
    need(len(test_macros) == 1 and test_macros[0].split('=', 1)[1].strip('"') == str(s), 'original native runtime fixture macro retained')
    exact(c['linkRecipes'], ROLES, 'four canonical consumer links')
    links, objects, link_orders = {}, {}, {}
    for role, (_, _, original_target) in ROLES.items():
        ref = c['linkRecipes'][role]
        need(artifact(ref) == b / 'CMakeFiles' / (original_target + '.dir') / 'link.txt', 'original generated link locator')
        link = shlex.split(prior(artifact(ref), ref).decode())
        obj = compiles[role][compiles[role].index('-o') + 1]
        need(link[0] == c['tools']['compiler'] and link.count('-o') == link.count(obj) == link.count('libnano_core.a') == 1 and link[link.index('-o') + 1] == original_target, 'only object Core and output substitutions')
        archives, assembly, order = 0, 0, []
        for word in link[1:]:
            if word == obj:
                order.append({'kind': 'NEW_CONSUMER', 'argument': word}); continue
            if word == 'libnano_core.a':
                archives += 1; order.append({'kind': 'NEW_CORE', 'argument': word}); continue
            if word.endswith(('.a', '.o', '.dylib', '.tbd')):
                p = pathlib.Path(word); p = p if p.is_absolute() else b / p
                p = pathlib.Path(os.path.normpath(str(p)))
                if p.is_relative_to(b): prior(p, cfg.get('linkInputPins', {}).get(str(p)))
                else: sdk_path(str(p))
                archives += word.endswith('.a'); assembly += word.endswith('.o')
                order.append({'kind': 'OLD_ARCHIVE' if word.endswith('.a') else 'OLD_ASSEMBLY' if word.endswith('.o') else 'OLD_SDK_LIBRARY', 'argument': word})
        need(archives == 5 and assembly == 2, 'canonical five archives and two assembly objects per link')
        links[role], objects[role], link_orders[role] = link, obj, order
    archive_ref = c['archiveRecipe']
    need(artifact(archive_ref) == b / 'CMakeFiles/nano_core.dir/link.txt', 'canonical Core archive recipe locator')
    archive_lines = [shlex.split(x) for x in prior(artifact(archive_ref), archive_ref).decode().splitlines() if x.strip()]
    need(len(archive_lines) == 2 and archive_lines[0][:3] == [c['tools']['archiver'], 'qc', 'libnano_core.a'] and archive_lines[1] == [c['tools']['ranlib'], 'libnano_core.a'] and len(archive_lines[0][3:]) == 143 and [pathlib.Path(x).name for x in archive_lines[0][3:]] == list(before), 'original ar143 and ranlib tool/order authority')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('NANO_', 'DYLD_')) and k not in ('LD_PRELOAD', 'CPATH', 'CPLUS_INCLUDE_PATH', 'C_INCLUDE_PATH', 'LIBRARY_PATH', 'CXXFLAGS', 'LDFLAGS')}
    dependencies = {}
    for index, (role, argv) in enumerate(compiles.items()):
        base = list(argv); i = base.index('-o'); del base[i:i + 2]; base.remove('-c')
        run('M-' + str(index), base + ['-M', '-MT', 'native-validator-' + str(index)], b, env, 120)
        found = dependency_set(output(out / ('M-' + str(index) + '-stdout.private.log')), b)
        actual = 'src/capella_beacon_api.cpp' if role == 'core' else ROLES[role][0]
        need(str(current / actual) in found, 'actual current source belongs to genuine M')
        for value in sorted(found):
            p = pathlib.Path(value)
            if p.is_relative_to(current):
                relative = str(p.relative_to(current))
                known = API_FILES.get(relative)
                if known is None:
                    need(relative in project_pins, 'current dependency requires existing relative source authority')
                    known = byte_pin(project_pins[relative])
                hold(physical(str(p)), known)
            elif p.is_relative_to(s): prior(p, project_pins.get(str(p.relative_to(s))))
            elif p.is_relative_to(b): prior(p, cfg.get('linkInputPins', {}).get(str(p)))
            else: sdk_path(value)
        dependencies[role] = found
        fence()
    publish(out / 'admitted.private.json', {'hostFiles': HELD, 'aliases': ALIASES, 'dependencies': {k: sorted(v) for k, v in dependencies.items()}, 'linkOrders': link_orders, 'runtimeFixtures': c['runtimePublicFixtures'], 'sourceBaseRevision': c['sourceBaseRevision'], 'oldSdkPinsRefreshed': False})
    new_objects = {}
    for index, (role, argv) in enumerate(compiles.items()):
        filename = 'capella_beacon_api.cpp.o' if role == 'core' else 'validator_read.cpp.o' if role == 'nano-capella-validator-read-tests' else pathlib.Path(ROLES[role][0]).name + '.o'
        obj, md = out / filename, out / (filename + '.d')
        command = list(argv); command[command.index('-o') + 1] = str(obj)
        command += ['-MD', '-MT', 'native-validator-' + str(index), '-MF', str(md)]
        run('CXX-' + str(index), command, b, env)
        need(dependency_set(output(md), b) == dependencies[role], 'genuine M must equal actual compile MD')
        output(obj); new_objects[role] = obj; fence()
    candidate_core = out / 'libnano_core.a'
    write_new(candidate_core, old_core, track=False)
    need(str(candidate_core) not in NEW and material(candidate_core)[1] == old_core, 'copy old archive before first new archive admission')
    run('replace-one-member', [c['tools']['archiver'], 'r', str(candidate_core), str(new_objects['core'])], b, env, 60)
    run('index-new-core', [c['tools']['ranlib'], str(candidate_core)], b, env, 60)
    new_core_bytes = material(candidate_core)[1]; after = archive_members(new_core_bytes)
    need(list(after) == list(before) and [k for k in before if before[k] != after[k]] == ['capella_beacon_api.cpp.o'] and after['capella_beacon_api.cpp.o'] == NEW[str(new_objects['core'])], '143 members142 unchanged and exact compiled replacement')
    output(candidate_core); fence()
    programs = {}
    for index, role in enumerate(ROLES):
        command = [str(new_objects[role]) if x == objects[role] else str(candidate_core) if x == 'libnano_core.a' else x for x in links[role]]
        command[command.index('-o') + 1] = str(out / role)
        run('LINK-' + str(index), command, b, env)
        binary = output(out / role)
        need(len(binary) >= 32 and binary[:4] == b'\xcf\xfa\xed\xfe' and int.from_bytes(binary[4:8], 'little') == 0x0100000c, 'actual native arm64 Mach-O output')
        programs[role] = dict(NEW[str(out / role)], architecture='arm64', format='Mach-O64')
        fence()
    # One new focused test only. No old-Core baseline, legacy or rotation suite.
    run('focused-validator-read', [str(out / 'nano-capella-validator-read-tests')], b, env, 70)
    result_rows = [json.loads(x) for x in output(out / 'focused-validator-read-stdout.private.log').decode().splitlines()]
    groups = ('validator_status_projection', 'validator_authenticated_selection', 'validator_selector_refusals', 'validator_projection_integrity', 'validator_retained_historical_state')
    need(len(result_rows) == 6 and [x.get('scenario') for x in result_rows[:-1]] == list(groups) and all(x.get('passed') is True for x in result_rows), 'six exact successful focused result rows')
    summary = result_rows[-1]
    need(summary['scenario'] == 'validator_read_summary' and summary['groups'] == 5 and summary['statuses'] == 9 and summary['httpReads'] == 21 and summary['expectedRefusals'] == 16 and summary['fixtureBlsSignatures'] == 2 and summary['auditSignatures'] == 0 and type(summary['peakRssKiB']) is int and 0 < summary['peakRssKiB'] <= 131072 and all(summary[k] is False for k in ('databaseAccessed', 'runtimeStarted', 'networkStarted', 'productionQualified')), 'new focused API contract')
    need(all(x['apiSignaturesCreated'] == 0 and x['databaseAccessed'] is False and x['runtimeStarted'] is False and x['productionQualified'] is False for x in result_rows[:-1]) and result_rows[0]['fullUint64Strings'] is True and result_rows[1]['indexAndPublicKeyAgree'] is True and result_rows[1]['immutableStateAgrees'] is True and result_rows[2]['expectedHttpRefusals'] == 13 and result_rows[3]['corruptStateRejected'] is True and result_rows[3]['projectionCancellationObserved'] is True and result_rows[3]['originalStoreUnchanged'] is True and result_rows[4]['realSignedTransition'] is True and result_rows[4]['distinctHistoricalBalance'] is True and result_rows[4]['removedStateDoesNotFallback'] is True and result_rows[4]['sourceStoreUnchanged'] is True, 'all five group-specific typed assertions')
    need(not output(out / 'focused-validator-read-stderr.private.log'), 'focused test stderr must be empty')
    fence(); need(time.monotonic() < DEADLINE, 'whole controller1100s final bound')
    result = {'format': 'nano-cross-arch-validator-native-build-terminal-v1', 'passed': True, 'owner': 'mac-native', 'physicalArchitecture': 'Darwin-arm64', 'sourceBaseRevision': c['sourceBaseRevision'], 'sourceManifestSha256': SOURCE_MANIFEST_SHA, 'oldCoreArchive': {'sha256': OLD_CORE_SHA, 'bytes': OLD_CORE_BYTES}, 'newCoreArchive': NEW[str(candidate_core)], 'programs': programs, 'coreMembers': 143, 'unchangedCoreMembers': 142, 'coreCompilations': 1, 'testCompilations': 1, 'runtimeConsumerCompilations': 3, 'totalCompilations': 5, 'programLinks': 4, 'newFocusedTestInvocations': 1, 'genuineMPairsEqualActualMd': 5, 'canonicalCompileAndLinkOrderPreserved': True, 'originalFixtureMacroRetained': True, 'runtimeFixtureLeavesFenced': 5, 'oldArchiveAndInputsNineAndShaUnchanged': True, 'sdkOldNineAndShaBeforeAfterEqual': True, 'sdkPinsAutomaticallyRefreshed': False, 'appleSystemToolchainScopeRetained': True, 'test': summary, 'configureCommands': 0, 'fullBuildCommands': 0, 'oldSuiteReplays': 0, 'causalOldCoreReplays': 0, 'imageBuilds': 0, 'imagePulls': 0, 'dockerInvocations': 0, 'httpTransportTested': False, 'runtimeStarted': False, 'privateCampaignRead': False, 'privateVpsAccessed': False, 'multiownerFinalityQualified': False, 'productionQualified': False, 'events': EVENTS}
    publish(out / 'terminal.json', result); fence()
    print(json.dumps(result), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--inputs', required=True); parser.add_argument('--sha256', required=True); parser.add_argument('--bytes', type=int, required=True)
    arguments = parser.parse_args()
    try:
        main(arguments)
    except BaseException as error:
        refusal = {'format': 'nano-cross-arch-validator-native-build-refused-v1', 'passed': False, 'phase': PHASE, 'errorType': type(error).__name__, 'events': EVENTS, 'detailsExported': False, 'productionQualified': False}
        if CONTROL is not None:
            try: publish(CONTROL / 'terminal-refusal.private.json', dict(refusal, reason=str(error), heldOriginalAuthorities=HELD, heldOriginalAliases=ALIASES, newOutputCommitments=NEW))
            except Exception: pass
        print(json.dumps(refusal), flush=True)
        raise SystemExit(1)
