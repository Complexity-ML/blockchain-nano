#!/usr/bin/env python3
"""Root-only AMD real-main HTTP/persistence host; author has not executed it.

No build/pull/prune, keys, wallet, consensus runtime or remote RPC. Original
software rows come from authenticated prior documents, never current restats.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import stat
import subprocess
import sys
import time
import uuid

REPO = Path('/home/ubuntu/blockchain-nano-git')
TOOLS = Path('/home/ubuntu/main-http-persistence-tools-20261018-02')
OUT = Path('/home/ubuntu/main-http-amd-qualification-root-20261018-02')
OLD = Path('/home/ubuntu/rpc-candidate-amd-root-20261018-01')
NANO = OLD / 'outputs/nano'
OLD_CONTROL = Path('/home/ubuntu/rpc-candidate-amd-control-root-20261018-01/control.private.json')
BRIDGE_CONTROL = Path('/home/ubuntu/rpc-candidate-amd-continuation-control-20261018-01/control.private.json')
AUTHORITY = OLD / 'outputs/final-observations.private.json'
IMAGE = 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
MAIN = 'bc99b0ec2cf8bc6bcb21a26ec980ef2c4e8573a8'
C6 = 'c6cdf780c68eab8b06306d76257bda9028928ea3'
BRIDGE_PIN = {'sha256': 'e6a37b5069a6786dea733bc3c0136e59fae63d2be214a34027b13d55ecb3b9b1', 'bytes': 1281491}
CONTROL_PIN = {'sha256': 'ac2eeedf8406ae807043bb91ec56d135f8b9829b89e54665cd2d3c0e84e2d3d1', 'bytes': 1178681}
AUTHORITY_PIN = {'sha256': '866e3f3d0042ec8cf90b487498bfa70601e9f9af826379d6dfe5eb03239e43c9', 'bytes': 1512552}
NANO_PIN = {'sha256': 'cc0eed49d62d9decccf128b773241191d9ef39abc4cb2c90f876a4a8f9ece94b', 'bytes': 13997336}
PUBLIC_PINS = {
    'worker.py': {'sha256': '2959cc8a72b77ef4ce48318bd7b250be786d71280c7d6a4396a9c5e1a8a2e844', 'bytes': 40458},
    'genesis.json': {'sha256': '875ed228e1a31cf774f23f2329763175ed01651ee4dcf3db8a1eb59baf5ce6d8', 'bytes': 910},
    'transactions.json': {'sha256': '8220db3001ba2c25dc99646046bc205c7cbebc3d72bd4847b874f9e8cbe15db3', 'bytes': 209},
    'expectations-template.json': {'sha256': '48e24ec04c7a7aef4a4011a2d92c0e756fcf1d7fe689bf74396db1368b038c70', 'bytes': 1399}}
F = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid',
     'st_size', 'st_mtime_ns', 'st_ctime_ns')


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(raw):
    return {'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}


def unique(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'duplicate JSON key')
        result[key] = value
    return result


def parse(raw):
    def nonfinite(_):
        raise ValueError('nonfinite JSON value')
    return json.loads(raw.decode('utf-8'), object_pairs_hook=unique, parse_constant=nonfinite)


def fdrow(st):
    return {key: getattr(st, key) for key in F}


def valid_row(value):
    return (type(value) is dict and set(value) == set(F) | {'sha256'}
            and all(type(value[k]) is int and value[k] >= 0 for k in F)
            and type(value['sha256']) is str
            and re.fullmatch(r'[0-9a-f]{64}', value['sha256']) is not None)


def material(path, observer=None, allow_empty=False):
    need(path.is_absolute() and path.resolve(strict=True) == path, 'physical absolute input')
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    event = {'path': str(path), 'before': None, 'after': None, 'pathAfter': None,
             'sha256': None, 'bytesRead': 0, 'failure': None}
    published = False
    try:
        before = os.fstat(fd)
        event['before'] = fdrow(before)
        need(stat.S_ISREG(before.st_mode) and before.st_nlink == 1
             and (0 <= before.st_size if allow_empty else 0 < before.st_size)
             and before.st_size <= (64 if path == Path('/usr/bin/docker') else 32) * 1024 * 1024,
             'bounded single-link regular input')
        data = bytearray()
        while True:
            chunk = os.read(fd, 1024 * 1024)
            if not chunk:
                break
            data.extend(chunk)
            need(len(data) <= before.st_size, 'fixed FD input length')
        raw = bytes(data)
        event.update(after=fdrow(os.fstat(fd)), pathAfter=fdrow(path.lstat()),
                     sha256=hashlib.sha256(raw).hexdigest(), bytesRead=len(raw))
        if observer is not None:
            published = True
            observer(event)
        need(len(raw) == before.st_size and event['before'] == event['after'] == event['pathAfter'],
             'FD/path nine-field input change')
        return raw, dict(event['before'], sha256=event['sha256'])
    except BaseException as exc:
        if observer is not None and not published:
            event['failure'] = type(exc).__name__ + ': ' + str(exc)
            try:
                observer(event)
            except BaseException:
                pass  # No admission follows a failed read; keep its first cause.
        raise
    finally:
        os.close(fd)


class Host:
    def __init__(self, revision, python_control):
        need(sys.platform.startswith('linux') and os.getuid() == os.getgid() == 1000,
             'Root AMD Linux execution as uid/gid1000')
        need(revision == MAIN, 'explicit current main revision')
        need(OUT.parent.resolve(strict=True) == OUT.parent and not OUT.exists()
             and not OUT.is_symlink(), 'fresh isolated output namespace')
        os.umask(0o077)
        OUT.mkdir(mode=0o700)
        for leaf in ('outputs', 'controls', 'docker-client', 'logs'):
            (OUT / leaf).mkdir(mode=0o700)
        self.started = time.monotonic()
        self.deadline = self.started + 450
        self.cleanup_deadline = None
        self.revision = revision
        self.python_control = python_control
        self.python_root = None
        self.python_files = set()
        self.python_directories = set()
        self.empty_python_files = set()
        self.python_ready = False
        self.python_summary = None
        self.python_environment = {}
        self.held = {}
        self.first = None
        self.secondary = []
        self.steps = []
        self.serial = 0
        self.cid = None
        self.attempted = False
        self.image_checked = False
        self.docker_ready = False
        self.bridge_admitted = False
        self.image_final = None
        self.cleanup = {'passed': False, 'ownedContainerAbsent': False}
        self.worker = None
        self.worker_exit = None
        self.authority_prior_row = False
        self.original_software_rows = 0
        self.new_results = 0
        self.name = 'nano-main-http-amd-' + uuid.uuid4().hex
        self.label = 'nano.main.http.persistence'
        self.cidfile = OUT / 'container.cid'
        self.prefix = ['/usr/bin/sudo', '-n', '/usr/bin/docker', '--host',
                       'unix:///var/run/docker.sock', '--config', str(OUT / 'docker-client')]
        self.environment = {'PATH': '/usr/bin:/bin', 'HOME': '/home/ubuntu',
                            'LANG': 'C', 'LC_ALL': 'C', 'GIT_OPTIONAL_LOCKS': '0'}

    def write(self, path, raw):
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
        with os.fdopen(fd, 'wb') as output:
            output.write(raw)
            output.flush()
            os.fsync(output.fileno())
            return dict(fdrow(os.fstat(output.fileno())), sha256=pin(raw)['sha256'])

    def save(self, name, value):
        return self.write(OUT / name, (json.dumps(value, indent=2, sort_keys=True,
                                                allow_nan=False) + '\n').encode())

    def hold(self, path, expected):
        need(valid_row(expected), 'genuine full-nine SHA authority row')
        raw, row = material(path, allow_empty=str(path) in self.empty_python_files)
        need(row == expected, 'original input changed: ' + str(path))
        need(str(path) not in self.held or self.held[str(path)] == expected,
             'never refresh an already held input')
        self.held[str(path)] = dict(expected)
        return raw

    def new_input(self, path, wanted, label, creation=None):
        need(str(path) not in self.held, 'prospective admission only before first hold')
        start = time.monotonic()
        cutoff = min(self.deadline, start + 30)
        previous = None
        count = 0
        since = None
        first_eight = None if creation is None else {k: v for k, v in creation.items() if k != 'st_ctime_ns'}
        samples = 0
        while True:
            samples += 1
            def publish(event):
                self.save('controls/' + label + '-sample-' + str(samples) + '.private.json', {
                    'knownPin': wanted, 'elapsedMs': round((time.monotonic() - start) * 1000),
                    'observation': event, 'oldRowsRefreshed': False})
            raw, row = material(path, publish)
            now = time.monotonic()
            eight = {k: v for k, v in row.items() if k != 'st_ctime_ns'}
            need(pin(raw) == wanted and (first_eight is None or eight == first_eight),
                 'prospective bytes/eight-field identity')
            first_eight = eight
            need(now < cutoff, 'prospective admission deadline')
            if row == previous:
                count += 1
            else:
                count, since = 1, now
            if count >= 3 and now - since >= 2:
                need(str(path) not in self.held, 'no retention race before first hold')
                self.held[str(path)] = row
                return raw
            previous = row
            time.sleep(min(1.05, max(0, cutoff - time.monotonic())))

    def fence(self):
        failures = []
        for path, expected in list(self.held.items()):
            try:
                _, actual = material(Path(path), allow_empty=path in self.empty_python_files)
                if actual != expected:
                    failures.append({'path': path, 'changedFields': [k for k in expected if actual[k] != expected[k]]})
            except BaseException as exc:
                failures.append({'path': path, 'error': type(exc).__name__ + ': ' + str(exc)})
        return failures

    def run(self, label, argv, timeout=20, cleanup=False, expected=0):
        cutoff = self.cleanup_deadline if cleanup else self.deadline
        need(cutoff is not None and time.monotonic() < cutoff, 'bounded host command deadline')
        need(all(type(x) is str and x and '\0' not in x for x in argv), 'explicit argv only')
        for path in ('/usr/bin/sudo', '/usr/bin/docker', '/usr/bin/git'):
            if path in self.held:
                self.hold(Path(path), self.held[path])
        self.serial += 1
        stem = 'logs/%03d-%s' % (self.serial, label)
        self.save(stem + '.argv.json', argv)
        started = time.monotonic()
        with (OUT / (stem + '.stdout.log')).open('xb') as stdout, (OUT / (stem + '.stderr.log')).open('xb') as stderr:
            proc = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr,
                                    env=self.environment, close_fds=True)
            need(proc.pid > 1 and proc.pid != 28722, 'only owned CLI; never PID28722')
            timed_out = False
            first = None
            try:
                proc.wait(timeout=min(timeout, max(0.001, cutoff - time.monotonic())))
            except BaseException as exc:
                first = exc
                timed_out = isinstance(exc, subprocess.TimeoutExpired)
            finally:
                if proc.pid != 28722 and proc.poll() is None:
                    try:
                        proc.terminate()
                        proc.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait(timeout=3)
                    except BaseException as exc:
                        if first is None:
                            first = exc
                event = {'phase': label, 'pid': proc.pid, 'exitCode': proc.returncode,
                         'actualProcessJoined': proc.returncode is not None, 'timedOut': timed_out,
                         'elapsedMs': round((time.monotonic() - started) * 1000)}
                self.steps.append(event)
                try:
                    self.save(stem + '.exit.json', event)
                except BaseException as exc:
                    if first is None:
                        first = exc
                    else:
                        self.secondary.append({'stage': 'CLI-exit-publication', 'error': str(exc)})
        if first is not None:
            raise first
        raw = (OUT / (stem + '.stdout.log')).read_bytes()
        err = (OUT / (stem + '.stderr.log')).read_bytes()
        need(len(raw) <= 4 * 1024 * 1024 and len(err) <= 256 * 1024, 'bounded CLI logs')
        need(expected is None or proc.returncode == expected, 'command exit: ' + label)
        return raw, err, proc.returncode

    def docker(self, label, args, timeout=20, cleanup=False, expected=0):
        return self.run(label, self.prefix + args, timeout, cleanup, expected)

    def image(self, label, cleanup=False):
        raw, err, _ = self.docker(label, ['image', 'inspect', IMAGE, '--format', '{{json .}}'], cleanup=cleanup)
        value = parse(raw)
        need(not err and value['Id'] == IMAGE and value['Architecture'] == 'amd64'
             and value['Os'] == 'linux', 'same immutable AMD image')
        return {'passed': True, 'image': IMAGE, 'architecture': 'amd64'}

    def owned(self, cleanup=False):
        need(type(self.cid) is str and re.fullmatch(r'[0-9a-f]{64}', self.cid) is not None, 'full own CID')
        raw, err, _ = self.docker('inspect-owned', ['container', 'inspect', self.cid], cleanup=cleanup)
        values = parse(raw)
        need(not err and type(values) is list and len(values) == 1, 'one owned container')
        value = values[0]
        need(value['Id'] == self.cid and value['Name'] == '/' + self.name and value['Image'] == IMAGE
             and (value['Config'].get('Labels') or {}).get(self.label) == self.name,
             'CID/name/image/label ownership')
        return value

    def discover(self):
        if self.cid is not None:
            return
        try:
            if self.cidfile.exists():
                raw, _ = material(self.cidfile)
                text = raw.decode().strip()
                need(re.fullmatch(r'[0-9a-f]{64}', text) is not None, 'complete own cidfile')
                self.cid = text
                return
        except BaseException as exc:
            # sudo Docker may create a root-owned/partial cidfile. Its failure
            # cannot skip the bounded name lookup and full owned() validation.
            self.secondary.append({'stage': 'cidfile-discovery',
                                   'error': type(exc).__name__ + ': ' + str(exc)})
        raw, err, _ = self.docker('discover-own', ['ps', '-a', '--no-trunc', '--filter',
                    'name=^/' + self.name + '$', '--format', '{{.ID}}'], cleanup=True)
        ids = raw.decode().splitlines()
        need(not err and len(ids) <= 1 and all(re.fullmatch(r'[0-9a-f]{64}', x) for x in ids), 'single own namespace')
        self.cid = ids[0] if ids else None

    def python_inventory(self):
        need(self.python_root is not None, 'declared test Python root')
        files, directories = set(), {str(self.python_root)}
        pending, entries = [self.python_root], 0
        while pending:
            directory = pending.pop()
            row = directory.lstat()
            need(stat.S_ISDIR(row.st_mode) and directory.resolve(strict=True) == directory
                 and stat.S_IMODE(row.st_mode) == 0o500, 'physical test Python directory mode500')
            with os.scandir(directory) as children:
                for child in children:
                    entries += 1
                    need(entries <= 20000 and not child.is_symlink(), 'bounded alias-free test Python inventory')
                    if child.is_dir(follow_symlinks=False):
                        directories.add(child.path)
                        pending.append(Path(child.path))
                    else:
                        need(child.is_file(follow_symlinks=False), 'regular test Python provider')
                        files.add(child.path)
        need(files == self.python_files and directories == self.python_directories,
             'exact admitted test Python files and directories')

    def prepare_python(self):
        descriptor = self.python_control
        raw = self.new_input(descriptor['file'], descriptor['pin'], 'python-runtime-control')
        control = parse(raw)
        fields = {'format', 'ready', 'runtimeRoot', 'guestRoot', 'interpreter', 'version',
                  'files', 'directories', 'environment', 'flags', 'provenance'}
        need(type(control) is dict and set(control) == fields
             and control['format'] == 'nano-test-python-runtime-control-v1'
             and control['ready'] is True, 'exact ready Root test Python control')
        need(control['guestRoot'] == '/python-runtime' and control['interpreter'] == 'bin/python3'
             and control['version'] == '3.12.3' and control['flags'] == ['-S'],
             'explicit Python3.12 test interpreter and no site initialization')
        environment = {'PYTHONHOME': '/python-runtime', 'PYTHONDONTWRITEBYTECODE': '1',
                       'PYTHONNOUSERSITE': '1', 'PYTHONSAFEPATH': '1',
                       'PYTHONPYCACHEPREFIX': '/tmp/nano-python-cache'}
        need(control['environment'] == environment and type(control['provenance']) is dict
             and len(json.dumps(control['provenance'], allow_nan=False).encode()) <= 65536,
             'explicit test-only environment and bounded descriptive provenance')
        root = Path(control['runtimeRoot'])
        need(root.is_absolute() and root.resolve(strict=True) == root and root.is_dir()
             and all(not (root == p or root in p.parents or p in root.parents)
                     for p in (OUT, TOOLS, REPO, OLD)), 'isolated physical test Python root')
        files, directories = control['files'], control['directories']
        need(type(files) is dict and 2 <= len(files) <= 4096
             and type(directories) is list and len(directories) <= 8192
             and all(type(x) is str for x in directories)
             and len(set(directories)) == len(directories), 'bounded provider tables')
        self.python_root = root
        self.python_directories = {str(root)}
        for rel in directories:
            part = Path(rel)
            need(type(rel) is str and not part.is_absolute() and '..' not in part.parts
                 and part.as_posix() == rel and len(rel) <= 512, 'canonical provider directory')
            self.python_directories.add(str(root / part))
        total = 0
        for rel, expected in files.items():
            part = Path(rel)
            need(type(rel) is str and not part.is_absolute() and rel not in ('', '.')
                 and '..' not in part.parts and part.as_posix() == rel and len(rel) <= 512
                 and valid_row(expected), 'canonical provider file and original Root FD9SHA')
            path = root / part
            if expected['st_size'] == 0:
                need(expected['sha256'] == hashlib.sha256(b'').hexdigest(), 'known empty provider SHA')
                self.empty_python_files.add(str(path))
            mode = 0o500 if rel == 'bin/python3' else 0o400
            need(stat.S_IMODE(expected['st_mode']) == mode
                 and expected['st_uid'] == expected['st_gid'] == 1000,
                 'Root copied test provider owner/mode')
            self.hold(path, expected)
            self.python_files.add(str(path))
            total += expected['st_size']
        need('bin/python3' in files and 'lib/python3.12/encodings/__init__.py' in files
             and total <= 128 * 1024 * 1024, 'complete bounded interpreter and standard-library cohort')
        self.python_inventory()
        self.python_ready = True
        self.python_environment = environment
        self.python_summary = {'controlPin': descriptor['pin'], 'declaredVersion': control['version'],
            'interpreterGuest': '/python-runtime/bin/python3', 'flags': ['-S'],
            'files': len(files), 'directories': len(self.python_directories), 'bytes': total,
            'knownEmptyProviderFiles': len(self.empty_python_files), 'readOnlyMount': True,
            'rootCopiedProvidersAdmittedRowsRetained': True, 'wholeSdkQualified': False,
            'pythonBundledInImageClaimed': False, 'productionRuntimeProviderQualified': False}

    def preflight(self):
        need(TOOLS.resolve(strict=True) == TOOLS and TOOLS.is_dir(), 'physical staged public tools')
        own_source = Path(__file__).resolve(strict=True)
        allowed = set(PUBLIC_PINS)
        if own_source.parent == TOOLS:
            need(own_source.name == 'host.py', 'public host staged under exact name')
            allowed.add('host.py')
        need({x.name for x in TOOLS.iterdir()} == allowed
             and all(x.is_file() and not x.is_symlink() for x in TOOLS.iterdir()), 'public-only tools inventory')
        for name, wanted in PUBLIC_PINS.items():
            self.new_input(TOOLS / name, wanted, 'tool-' + name.replace('.', '-'))
            need(stat.S_IMODE(self.held[str(TOOLS / name)]['st_mode']) == 0o400, 'staged public input mode0400')
        _, own_row = material(own_source)
        self.hold(own_source, own_row)  # Root authenticates this source before execution.
        need(stat.S_IMODE(own_row['st_mode']) == 0o400, 'Root freezes host source before execution')
        bridge_raw = self.new_input(BRIDGE_CONTROL, BRIDGE_PIN, 'bridge-control-e6')
        self.bridge_admitted = True
        bridge = parse(bridge_raw)
        originals = bridge['originalFiles']
        need(type(originals) is dict and str(OLD_CONTROL) in originals, 'original ac2 authority row')
        old_raw = self.hold(OLD_CONTROL, originals[str(OLD_CONTROL)])
        need(pin(old_raw) == CONTROL_PIN, 'original ac2 bytes')
        old = parse(old_raw)
        need(old['format'] == 'nano-rpc-candidate-amd-control-v1' and old['ready'] is True
             and old['sourceRoots']['candidate']['revision'] == C6, 'original C6 build control')
        for path in ('/usr/bin/git', '/usr/bin/docker', '/usr/bin/sudo'):
            need(path in old['originalFiles'], 'original host tool authority: ' + path)
            self.hold(Path(path), old['originalFiles'][path])
        if str(AUTHORITY) in originals:
            authority_raw = self.hold(AUTHORITY, originals[str(AUTHORITY)])
            need(pin(authority_raw) == AUTHORITY_PIN, '866 known content, prior staging row strict')
            self.authority_prior_row = True
        else:
            authority_raw = self.new_input(AUTHORITY, AUTHORITY_PIN, 'authority866-new-own-row')
        authority = parse(authority_raw)
        need(type(authority['products']) is dict and len(authority['products']) == 265,
             '265 original products in known authority content')
        nano = authority['products'][str(NANO)]['hostFD9']
        need(valid_row(nano) and {'sha256': nano['sha256'], 'bytes': nano['st_size']} == NANO_PIN,
             'original retained C6 Nano row and known byte pin')
        self.hold(NANO, nano)
        need(nano['st_mode'] & 0o111, 'original Nano executable mode')
        self.original_software_rows = 4  # Nano, Docker, sudo, Git; not all 265 products.
        self.docker_ready = True
        self.prepare_python()
        raw, err, _ = self.run('git-head-before', ['/usr/bin/git', '-C', str(REPO), 'rev-parse', 'HEAD'])
        need(not err and raw.decode().strip() == self.revision, 'current clean main revision')
        raw, err, _ = self.run('git-clean-before', ['/usr/bin/git', '-C', str(REPO), 'status', '--porcelain'])
        need(not raw and not err, 'clean main before isolated runtime')
        template = parse(self.hold(TOOLS / 'expectations-template.json', self.held[str(TOOLS / 'expectations-template.json')]))
        need(template['ready'] is False and template['nano'] is None, 'immutable incomplete public fixture template')
        template['ready'], template['nano'] = True, dict(NANO_PIN)
        data = (json.dumps(template, indent=2, sort_keys=True, allow_nan=False) + '\n').encode()
        expected_path = OUT / 'controls/expectations.json'
        creation = self.write(expected_path, data)
        self.new_input(expected_path, pin(data), 'new-ready-expectations', creation)
        self.save('prepared-public.json', {'passed': True, 'actualHead': self.revision,
            'compiledSourceRevision': C6, 'nanoPin': NANO_PIN, 'workerPin': PUBLIC_PINS['worker.py'],
            'image': IMAGE, 'originalSoftwareRows': 4, 'testPythonRuntime': self.python_summary,
            'authority866InitialBuildHistoricalFd9Claimed': False,
            'authority866PriorStagingRowRetained': self.authority_prior_row,
            'bridgeControlOwnRowProspectivelyAdmitted': self.bridge_admitted, 'newExpectationsPin': pin(data),
            'newCompilations': 0, 'newImages': 0, 'productionQualified': False})
        self.image('image-before')
        self.image_checked = True

    def execute(self):
        self.preflight()
        raw, err, _ = self.docker('own-name-absent', ['ps', '-a', '--no-trunc', '--filter',
                                  'name=^/' + self.name + '$', '--format', '{{.ID}}'])
        need(not raw.strip() and not err, 'fresh own container namespace')
        python_env = ['--env', 'PYTHONPATH=']
        for key, value in sorted(self.python_environment.items()):
            python_env += ['--env', key + '=' + value]
        argv = ['run', '--detach', '--pull=never', '--cidfile', str(self.cidfile), '--name', self.name,
            '--label', self.label + '=' + self.name, '--network', 'none', '--read-only', '--user', '1000:1000',
            '--cap-drop', 'ALL', '--security-opt', 'no-new-privileges', '--cpus', '1', '--memory', '1g',
            '--pids-limit', '64', '--tmpfs', '/tmp:rw,nosuid,nodev,size=128m',
            '--mount', 'type=bind,src=' + str(NANO) + ',dst=/candidate/nano,readonly',
            '--mount', 'type=bind,src=' + str(TOOLS) + ',dst=/inputs,readonly',
            '--mount', 'type=bind,src=' + str(OUT / 'controls/expectations.json') + ',dst=/controls/expectations.json,readonly',
            '--mount', 'type=bind,src=' + str(OUT / 'outputs') + ',dst=/outputs',
            '--mount', 'type=bind,src=' + str(self.python_root) + ',dst=/python-runtime,readonly',
            *python_env, '--entrypoint', '/python-runtime/bin/python3', IMAGE, '-S', '/inputs/worker.py',
            '--nano', '/candidate/nano', '--genesis', '/inputs/genesis.json',
            '--transactions', '/inputs/transactions.json', '--expectations', '/controls/expectations.json',
            '--output', '/outputs/worker']
        self.attempted = True
        raw, err, _ = self.docker('start-own', argv, timeout=30)
        need(not err and re.fullmatch(rb'[0-9a-f]{64}\n', raw) is not None, 'full detached CID')
        self.cid = raw.decode().strip()
        value = self.owned()
        config, limits = value['Config'], value['HostConfig']
        need(config['User'] == '1000:1000' and config['Entrypoint'] == ['/python-runtime/bin/python3']
             and config['Cmd'][:2] == ['-S', '/inputs/worker.py']
             and limits['NetworkMode'] == 'none' and limits['ReadonlyRootfs'] is True
             and not limits.get('PortBindings') and not limits.get('Privileged')
             and limits['NanoCpus'] == 1000000000 and limits['Memory'] == 1073741824
             and limits['PidsLimit'] == 64 and 'ALL' in limits['CapDrop']
             and any(x.startswith('no-new-privileges') for x in limits['SecurityOpt']), 'fixed private runtime guards')
        actual_environment = dict(x.split('=', 1) for x in config['Env'])
        need(all(actual_environment.get(k) == v for k, v in self.python_environment.items())
             and actual_environment.get('PYTHONPATH') == '', 'only explicit isolated Python import paths')
        mounts = {x['Destination']: x for x in value['Mounts']}
        expected_mounts = {'/candidate/nano': str(NANO), '/inputs': str(TOOLS),
                           '/controls/expectations.json': str(OUT / 'controls/expectations.json'),
                           '/outputs': str(OUT / 'outputs'), '/python-runtime': str(self.python_root)}
        need({k for k, x in mounts.items() if x['Type'] == 'bind'} == set(expected_mounts)
             and all(mounts[g]['Source'] == p and mounts[g]['RW'] == (g == '/outputs')
                     for g, p in expected_mounts.items()), 'only original Nano/public inputs RO and new output RW')
        raw, err, _ = self.docker('wait-own-terminal', ['wait', self.cid], timeout=330)
        need(not err and re.fullmatch(rb'[0-9]+\n', raw) is not None, 'actual container wait status')
        self.worker_exit = int(raw.strip())
        value = self.owned()
        need(not value['State']['Running'] and not value['State']['OOMKilled']
             and value['State']['ExitCode'] == self.worker_exit, 'actual owned terminal/OOM status')
        stdout, stderr, _ = self.docker('worker-logs', ['logs', self.cid])
        self.write(OUT / 'worker.stdout.log', stdout)
        self.write(OUT / 'worker.stderr.log', stderr)
        result_path = OUT / 'outputs/worker/result-public.json'
        if result_path.exists():
            raw_result, result_row = material(result_path)
            self.worker = parse(raw_result)
            self.hold(result_path, result_row)
        need(self.worker_exit == 0 and not stderr and type(self.worker) is dict, 'worker actual exit0/result')
        need(parse(stdout) == self.worker and self.worker['format'] == 'nano-main-http-persistence-worker-result-v1'
             and self.worker['passed'] is True and self.worker['httpCalls'] == 57
             and self.worker['processStarts'] == 5 and self.worker['serverStarts'] == 3
             and self.worker['mineCommands'] == 2 and self.worker['cleanStops'] == 3
             and self.worker['proofVectors'] == 5 and self.worker['allOwnedProcessesJoined'] is True
             and self.worker['databaseReopened'] is True and self.worker['inputFencePassed'] is True
             and self.worker['firstFailure'] is None and self.worker['secondaryFailures'] == []
             and self.worker['productionQualified'] is False, 'exact three-cycle 57/5/3/2 worker oracles')
        need(self.worker['newTransactionSignaturesCreated'] == 0 and self.worker['walletCommands'] == 0
             and self.worker['newKeysCreated'] == 0 and self.worker['capellaOrValidatorRuntimeStarted'] is False
             and self.worker['devModeEnabled'] is False and self.worker['externalRpcRequests'] == 0
             and self.worker['beaconFinalityAuthenticated'] is False, 'no signing/wallet/consensus/external RPC')
        phases = self.worker['phases']
        need(type(phases) is list and len(phases) == 3 and [p['height'] for p in phases] == [0, 1, 2]
             and [p['httpCallsCumulative'] for p in phases] == [20, 38, 57]
             and [p['proofVectorsCumulative'] for p in phases] == [1, 3, 5]
             and all(p['passed'] is True for p in phases), 'previous phases preserved and complete')
        proof_path = OUT / 'outputs/worker/proof-vectors.jsonl'
        proof_raw, proof_row = material(proof_path)
        need(len(proof_raw) <= 2 * 1024 * 1024, 'five bounded public proof vectors')
        vectors = [parse(line) for line in proof_raw.splitlines()]
        need(len(vectors) == 5 and [v['header']['number'] for v in vectors] == ['0x0', '0x1', '0x0', '0x2', '0x1']
             and all(v['scenario'] == 'proof-vector' and v['root'] == v['header']['stateRoot']
                     and v['keys'] == ['0x0', '0x1'] and v['expectedPresent'] is True
                     and v['address'] == '0x5cacb0cd1bea69d7453cab8dfe870a90f5fff30b'
                     and v['expectedValues'] == (['0x0', '0x0'] if v['header']['number'] == '0x0' else ['0x0', '0x67'])
                     for v in vectors),
             'five actual header-bearing vectors; independent crypto remains pending')
        self.hold(proof_path, proof_row)
        self.new_results = 2
        raw, err, _ = self.run('git-head-after', ['/usr/bin/git', '-C', str(REPO), 'rev-parse', 'HEAD'])
        need(not err and raw.decode().strip() == self.revision, 'main unchanged after runtime')
        raw, err, _ = self.run('git-clean-after', ['/usr/bin/git', '-C', str(REPO), 'status', '--porcelain'])
        need(not raw and not err, 'main remains clean')

    def finish(self):
        self.cleanup_deadline = time.monotonic() + 60
        try:
            if self.attempted:
                self.discover()
                if self.cid is not None:
                    value = self.owned(cleanup=True)
                    if value['State']['Running']:
                        self.docker('stop-own', ['stop', '--time', '12', self.cid], timeout=20, cleanup=True)
                    need(not self.owned(cleanup=True)['State']['Running'], 'own container stopped before remove')
                    self.docker('remove-own', ['rm', self.cid], cleanup=True)
                raw, err, _ = self.docker('own-absence', ['ps', '-a', '--no-trunc', '--filter',
                        'name=^/' + self.name + '$', '--format', '{{.ID}}'], cleanup=True)
                need(not raw.strip() and not err, 'owned container absent after cleanup')
            self.cleanup = {'passed': True, 'ownedContainerAbsent': True, 'imagesOrVolumesPruned': False}
        except BaseException as exc:
            self.cleanup = {'passed': False, 'ownedContainerAbsent': False, 'error': type(exc).__name__ + ': ' + str(exc)}
            if self.first is None:
                self.first = exc
            else:
                self.secondary.append({'stage': 'owned-container-cleanup', 'error': str(exc)})
        if self.docker_ready:
            try:
                self.image_final = self.image('image-final', cleanup=True)
            except BaseException as exc:
                self.image_final = {'passed': False, 'error': type(exc).__name__ + ': ' + str(exc)}
                if self.first is None:
                    self.first = exc
                else:
                    self.secondary.append({'stage': 'image-final', 'error': str(exc)})
        # Independent of cleanup/image success and always after both attempts.
        failures = self.fence()
        if self.python_ready:
            try:
                self.python_inventory()
            except BaseException as exc:
                failures.append({'scope': 'testPythonRuntimeInventory',
                                 'error': type(exc).__name__ + ': ' + str(exc)})
        if failures and self.first is None:
            self.first = ValueError('original/new selected inputs changed after cleanup/image-final')
        fence_published = False
        try:
            self.save('host-final-fence-public.json', {'passed': not failures, 'selectedFiles': len(self.held),
                'originalSoftwareRows': self.original_software_rows, 'freshResultRows': self.new_results,
                'failures': failures, 'oldRowsRefreshed': False, 'afterCleanupAndImageFinal': True})
            fence_published = True
        except BaseException as exc:
            if self.first is None:
                self.first = exc
            else:
                self.secondary.append({'stage': 'fence-publication', 'error': str(exc)})
        result = {'format': 'nano-main-http-amd-host-result-v1', 'passed': self.first is None,
            'actualHead': self.revision, 'compiledSourceRevision': C6, 'nanoPin': NANO_PIN,
            'image': IMAGE, 'workerExitCode': self.worker_exit, 'worker': self.worker,
            'testPythonRuntime': self.python_summary, 'pythonBundledInImageClaimed': False,
            'cleanup': self.cleanup, 'imageFinalFence': self.image_final,
            'hostFinalFenceAfterCleanup': not failures, 'hostFinalFencePublicationPassed': fence_published,
            'hostInputRows': len(self.held),
            'originalSoftwareRowsStrictlyVerified': self.original_software_rows,
            'authority866InitialBuildHistoricalFd9Claimed': False,
            'authority866PriorStagingRowRetained': self.authority_prior_row,
            'bridgeControlOwnRowProspectivelyAdmitted': self.bridge_admitted,
            'oldRowsRefreshed': False, 'retained265ProductsAllReaudited': False,
            'newCompilations': 0, 'newImagesBuilt': 0, 'newCppTestsReplayed': 0,
            'independentNodeVerificationPending': True, 'proofVectorsExpected': 5,
            'workerPublicResult': 'outputs/worker/result-public.json',
            'proofVectorsPublicFile': 'outputs/worker/proof-vectors.jsonl',
            'beaconFinalityAuthenticated': False, 'runtimeDutiesResumed': False,
            'newCampaignStarted': False, 'newSignaturesCreatedByHost': 0, 'productionQualified': False,
            'firstFailure': None if self.first is None else {'type': type(self.first).__name__, 'message': str(self.first)},
            'secondaryFailures': self.secondary, 'steps': self.steps,
            'elapsedMs': round((time.monotonic() - self.started) * 1000)}
        try:
            self.save('result-public.json', result)
        except BaseException as exc:
            if self.first is None:
                self.first = exc
            else:
                self.secondary.append({'stage': 'result-publication', 'error': str(exc)})
            raise
        print(json.dumps(result, sort_keys=True), flush=True)
        return result['passed']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--revision', required=True)
    parser.add_argument('--python-runtime-control', nargs=3, required=True, metavar=('FILE', 'SHA256', 'BYTES'))
    args = parser.parse_args()
    host = None
    def interrupted(signum, frame):
        raise RuntimeError('Root host interrupted: ' + str(signum))
    for signum in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
        signal.signal(signum, interrupted)
    try:
        file, sha, size = args.python_runtime_control
        need(re.fullmatch(r'[0-9a-f]{64}', sha) is not None
             and re.fullmatch(r'[1-9][0-9]*', size) is not None
             and int(size) <= 8 * 1024 * 1024, 'external Root test Python control pin')
        python_control = {'file': Path(file), 'pin': {'sha256': sha, 'bytes': int(size)}}
        host = Host(args.revision, python_control)
        try:
            host.execute()
        except BaseException as exc:
            host.first = exc
        finally:
            passed = host.finish()
        return 0 if passed else 1
    except BaseException as exc:
        print(json.dumps({'format': 'nano-main-http-amd-host-controller-refusal-v1', 'passed': False,
            'firstFailure': None if host is None or host.first is None else str(host.first),
            'publicationOrPrestartError': type(exc).__name__ + ': ' + str(exc),
            'productionQualified': False}), file=sys.stderr, flush=True)
        return 1


if __name__ == '__main__':
    sys.exit(main())
