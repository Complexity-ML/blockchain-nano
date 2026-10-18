#!/usr/bin/env python3
"""Source-only proposal: Root supplies authenticated public fixtures and owns execution.

Three isolated real-main HTTP lifetimes, separated by two explicit PoW CLI mines.
The proof stream needs a separate independent verifier; it authenticates no finality.
Only the Python standard library is used. No wallet, key or signing command exists.
"""
import argparse
import hashlib
import http.client
import json
import os
from pathlib import Path
import re
import signal
import stat
import subprocess
import sys
import time

FIELDS = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid',
          'st_size', 'st_mtime_ns', 'st_ctime_ns')
EXPECTED_KEYS = {'format', 'ready', 'nano', 'inputs', 'chainId', 'evmFork',
                 'consensus', 'timestamps', 'coinbase', 'sender', 'target',
                 'txHash', 'receipt', 'gasLimit', 'baseFeePerGas', 'storageKeys',
                 'genesisValues', 'postValues', 'senderNonceBefore',
                 'senderNonceAfter', 'senderBalanceBefore', 'senderBalanceAfter',
                 'targetNonce', 'targetBalance', 'targetCode'}
HASH = re.compile(r'0x[0-9a-f]{64}\Z')
ADDRESS = re.compile(r'0x[0-9a-f]{40}\Z')
QUANTITY = re.compile(r'0x(?:0|[1-9a-f][0-9a-f]*)\Z')
HEX_BYTES = re.compile(r'0x(?:[0-9a-f]{2})*\Z')
MAX_RPC = 79  # A successful run has exactly 57, including admission of one raw.
MAX_BODY = 1024 * 1024
MAX_STDOUT = 2 * 1024 * 1024
MAX_STDERR = 128 * 1024


def need(ok, message):
    if not ok:
        raise ValueError(message)


def pin(data):
    return {'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)}


def duplicate_free(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'duplicate JSON key')
        result[key] = value
    return result


def decode(data):
    return json.loads(data.decode('utf-8'), object_pairs_hook=duplicate_free,
                      parse_constant=lambda _: need(False, 'nonfinite JSON value'))


def row(st):
    return {name: getattr(st, name) for name in FIELDS}


def physical(path):
    need(path.is_absolute() and path.resolve(strict=True) == path,
         'input must be an absolute physical path: ' + str(path))


def read(path, limit, expected=None, capture=True, observer=None):
    physical(path)
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    observation = {'path': str(path), 'before': None, 'after': None, 'pathAfter': None,
                   'bytesRead': 0, 'sha256': None, 'readFailure': None}
    published = False
    try:
        before = os.fstat(fd)
        observation['before'] = row(before)
        need(stat.S_ISREG(before.st_mode) and before.st_nlink == 1
             and 0 < before.st_size <= limit, 'bounded regular single-link input')
        first = row(before)
        digest = hashlib.sha256()
        parts = []
        size = 0
        while True:
            chunk = os.read(fd, 1024 * 1024)
            if not chunk:
                break
            size += len(chunk)
            need(size <= before.st_size, 'input length changed')
            digest.update(chunk)
            if capture:
                parts.append(chunk)
        observation.update({'after': row(os.fstat(fd)), 'pathAfter': row(path.lstat()),
                            'bytesRead': size, 'sha256': digest.hexdigest()})
        if observer is not None:
            published = True
            observer(observation)  # Durable BEFORE all admission/fence comparisons.
        need(size == before.st_size and first == observation['after'] == observation['pathAfter'],
             'input FD/path nine fields changed: ' + str(path))
        first['sha256'] = digest.hexdigest()
        if expected is not None:
            need(first == expected, 'held input changed: ' + str(path))
        return b''.join(parts) if capture else None, first
    except BaseException as exc:
        if observer is not None and not published:
            observation['readFailure'] = type(exc).__name__ + ': ' + str(exc)
            try:
                observer(observation)
            except BaseException:
                pass  # Preserve the original failed read; no admission can follow.
        raise
    finally:
        os.close(fd)


def valid_pin(value, maximum):
    need(type(value) is dict and set(value) == {'sha256', 'bytes'}, 'exact input pin')
    need(type(value['sha256']) is str
         and re.fullmatch(r'[0-9a-f]{64}', value['sha256']) is not None,
         'SHA-256 pin')
    need(type(value['bytes']) is int and 0 < value['bytes'] <= maximum,
         'positive bounded byte pin')


def amount(value):
    need(type(value) is str and QUANTITY.fullmatch(value) is not None,
         'canonical uint256 quantity')
    number = int(value, 16)
    need(number < 1 << 256, 'uint256 quantity range')
    return number


def genesis_amount(value):
    need(type(value) is int or type(value) is str, 'genesis integer')
    number = value if type(value) is int else int(value, 16)
    need(0 <= number < 1 << 256, 'genesis uint256 range')
    return number


def validate(args):
    paths = {name: Path(getattr(args, name)) for name in
             ('nano', 'genesis', 'transactions', 'expectations')}
    need(len(set(paths.values())) == 4, 'four distinct input paths')
    expectations_bytes, expectations_row = read(paths['expectations'], 16384)
    e = decode(expectations_bytes)
    need(type(e) is dict and set(e) == EXPECTED_KEYS, 'exact expectations-v1 fields')
    need(e['format'] == 'nano-main-http-persistence-expectations-v1'
         and e['ready'] is True, 'Root must complete a new ready expectations file')
    need(e['evmFork'] == 'Shanghai' and e['consensus'] == 'pow'
         and e['timestamps'] == [12, 24]
         and all(type(x) is int for x in e['timestamps']), 'fixed PoW Shanghai timeline')
    need(type(e['inputs']) is dict and set(e['inputs']) == {'genesis', 'transactions'},
         'two fixture pins')
    limits = {'nano': 256 * 1024 * 1024, 'genesis': 65536, 'transactions': 1024 * 1024}
    held = {str(paths['expectations']): expectations_row}
    content = {}
    for name in ('nano', 'genesis', 'transactions'):
        wanted = e['nano'] if name == 'nano' else e['inputs'][name]
        valid_pin(wanted, limits[name])
        data, observed = read(paths[name], limits[name], capture=name != 'nano')
        need({'sha256': observed['sha256'], 'bytes': observed['st_size']} == wanted,
             'known input bytes: ' + name)
        if name == 'nano':
            need(observed['st_mode'] & 0o111, 'Nano executable bit')
        else:
            content[name] = decode(data)
        held[str(paths[name])] = observed
    for name in ('coinbase', 'sender', 'target'):
        need(type(e[name]) is str and ADDRESS.fullmatch(e[name]) is not None,
             'canonical fixture address: ' + name)
    need(len({e[x] for x in ('coinbase', 'sender', 'target')}) == 3,
         'distinct fixture roles')
    need(type(e['txHash']) is str and HASH.fullmatch(e['txHash']) is not None,
         'known raw transaction hash')
    for name in ('chainId', 'gasLimit', 'baseFeePerGas', 'senderNonceBefore',
                 'senderNonceAfter', 'senderBalanceBefore', 'senderBalanceAfter',
                 'targetNonce', 'targetBalance'):
        amount(e[name])
    need(amount(e['chainId']) > 0 and amount(e['senderNonceAfter']) ==
         amount(e['senderNonceBefore']) + 1, 'positive chain and single transaction nonce')
    need(type(e['receipt']) is dict and set(e['receipt']) == {'status', 'gasUsed', 'type'},
         'exact receipt expectations')
    for value in e['receipt'].values():
        amount(value)
    need(e['receipt']['status'] == '0x1' and e['receipt']['type'] == '0x0',
         'existing successful legacy transaction fixture')
    need(e['storageKeys'] == ['0x0', '0x1'], 'two fixed storage keys')
    for name in ('genesisValues', 'postValues'):
        need(type(e[name]) is list and len(e[name]) == 2, 'two fixture storage values')
        for value in e[name]:
            amount(value)
    need(type(e['targetCode']) is str and HEX_BYTES.fullmatch(e['targetCode']) is not None
         and 2 < len(e['targetCode']) <= 8194, 'small existing target bytecode')
    g = content['genesis']
    need(type(g) is dict and type(g.get('config')) is dict, 'genesis object')
    config = g['config']
    need(set(config) == {'chainId', 'consensus', 'evmFork', 'genesisHeaderVersion'}
         and config['consensus'] == 'pow' and config['evmFork'] == e['evmFork']
         and type(config['genesisHeaderVersion']) is int and config['genesisHeaderVersion'] == 2
         and genesis_amount(config['chainId']) == amount(e['chainId']),
         'no signed consensus configuration or later EVM profile')
    need(genesis_amount(g.get('difficulty')) == 1 and genesis_amount(g.get('timestamp')) == 0
         and genesis_amount(g.get('gasLimit')) == amount(e['gasLimit'])
         and genesis_amount(g.get('baseFeePerGas')) == amount(e['baseFeePerGas']),
         'known small PoW fixture header parameters')
    alloc = g.get('alloc')
    need(type(alloc) is dict and 2 <= len(alloc) <= 8
         and e['sender'] in alloc and e['target'] in alloc, 'small public fixture allocation')
    for address, account in alloc.items():
        need(ADDRESS.fullmatch(address) is not None and type(account) is dict
             and set(account) <= {'nonce', 'balance', 'code', 'storage'}, 'allocation account')
        need(type(account.get('storage', {})) is dict and len(account.get('storage', {})) <= 16,
             'bounded genesis storage')
    sender, target = alloc[e['sender']], alloc[e['target']]
    need(genesis_amount(sender.get('balance', 0)) == amount(e['senderBalanceBefore'])
         and genesis_amount(sender.get('nonce', 0)) == amount(e['senderNonceBefore']),
         'sender fixture allocation')
    need(genesis_amount(target.get('balance', 0)) == amount(e['targetBalance'])
         and genesis_amount(target.get('nonce', 0)) == amount(e['targetNonce'])
         and target.get('code') == e['targetCode'], 'target fixture allocation')
    need(all(genesis_amount(target.get('storage', {}).get(k, 0)) == amount(v)
             for k, v in zip(e['storageKeys'], e['genesisValues'])), 'genesis storage expectations')
    transactions = content['transactions']
    need(type(transactions) is list and len(transactions) == 1
         and type(transactions[0]) is str and HEX_BYTES.fullmatch(transactions[0]) is not None
         and 2 < len(transactions[0]) <= 2 * 1024 * 1024,
         'one existing signed raw; no signing in worker')
    output = Path(args.output)
    need(output.is_absolute() and output.parent.resolve(strict=True) == output.parent
         and re.fullmatch(r'[A-Za-z0-9._-]+', output.name) is not None
         and not output.exists() and not output.is_symlink(), 'new physical isolated output')
    need(all(output != p and output not in p.parents and p not in output.parents
             for p in paths.values()), 'output and inputs are disjoint')
    worker_path = Path(__file__).resolve(strict=True)
    _, worker_row = read(worker_path, 128 * 1024, capture=False)
    held[str(worker_path)] = worker_row
    return paths, e, transactions, output, held


class Worker:
    def __init__(self, validated, started):
        self.paths, self.e, self.transactions, self.output, self.held = validated
        self.started = started
        self.deadline = self.started + 300
        self.action_deadline = self.deadline - 18
        self.children = []
        self.rpc_count = 0
        self.vectors = 0
        self.stops = []
        self.mine_results = []
        self.phases = []
        self.first_failure = None
        self.secondary = []
        self.output.mkdir(mode=0o700)
        self.db = self.output / 'database'
        self.db.mkdir(mode=0o700)
        self.config = self.output / 'serve-config.json'
        self.write_bytes(self.config.name, b'{}\n')
        # This is an own-created file, not an old authority. Only ctime may settle
        # before its first hold, under a known byte pin and eight fixed fields.
        wanted = pin(b'{}\n')
        previous = None
        stable_since = None
        first_eight = None
        streak = 0
        samples = []
        cutoff = min(self.action_deadline, time.monotonic() + 30)
        while True:
            def publish(observation):
                self.save('config-first-observation-' + str(len(samples) + 1) + '-public.json',
                          {'knownPin': wanted, 'elapsedMs': round((time.monotonic() - self.started) * 1000),
                           'observation': observation, 'oldRowsRefreshed': False})
            data, observed = read(self.config, 16, observer=publish)
            samples.append({'elapsedMs': round((time.monotonic() - self.started) * 1000),
                            'row': observed})
            eight = {k: v for k, v in observed.items() if k != 'st_ctime_ns'}
            need(pin(data) == wanted and (first_eight is None or eight == first_eight),
                 'own empty serve config identity/bytes')
            first_eight = eight
            now = time.monotonic()
            need(now < cutoff, 'own config first-admission deadline')
            if observed == previous:
                streak += 1
            else:
                streak, stable_since = 1, now
            if streak >= 3 and now - stable_since >= 2:
                break
            previous = observed
            time.sleep(min(1.05, max(0, cutoff - time.monotonic())))
        self.held[str(self.config)] = observed
        self.save('config-first-admission-public.json', {'knownPin': wanted, 'samples': samples,
                                                       'oldRowsRefreshed': False})
        self.ledger = self.output / 'rpc-cases.jsonl'
        self.proofs = self.output / 'proof-vectors.jsonl'
        self.write_bytes(self.ledger.name, b'')
        self.write_bytes(self.proofs.name, b'')
        self.environment = {'PATH': '/usr/bin:/bin', 'LANG': 'C', 'LC_ALL': 'C', 'TZ': 'UTC'}

    def write_bytes(self, name, data):
        path = self.output / name
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
        try:
            with os.fdopen(fd, 'wb') as stream:
                stream.write(data)
                stream.flush()
                os.fsync(stream.fileno())
        except BaseException:
            raise

    def save(self, name, value):
        self.write_bytes(name, (json.dumps(value, sort_keys=True, indent=2,
                                           allow_nan=False) + '\n').encode())

    def append(self, path, value):
        with path.open('ab') as stream:
            stream.write((json.dumps(value, sort_keys=True, separators=(',', ':'),
                                     allow_nan=False) + '\n').encode())
            stream.flush()
            os.fsync(stream.fileno())

    def remaining(self, cleanup=False):
        left = (self.deadline if cleanup else self.action_deadline) - time.monotonic()
        need(left > 0, 'bounded worker deadline')
        return left

    def own(self, child):
        need(child in self.children and child['proc'].pid > 1
             and child['proc'].pid != 28722, 'only owned child; PID28722 prohibited')
        return child['proc']

    def logs(self, child):
        need(child['stdout'].stat().st_size <= MAX_STDOUT
             and child['stderr'].stat().st_size <= MAX_STDERR, 'bounded child logs')

    def launch(self, label, argv):
        self.remaining()
        need(not any(self.own(x).poll() is None for x in self.children),
             'previous child must be joined before launch; no concurrent DB opens')
        for path, expected in self.held.items():
            read(Path(path), max(expected['st_size'], 1), expected, capture=False)
        out = self.output / (label + '.stdout.log')
        err = self.output / (label + '.stderr.log')
        with out.open('xb') as stdout, err.open('xb') as stderr:
            proc = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=stdout,
                                    stderr=stderr, env=self.environment, close_fds=True)
        child = {'label': label, 'argv': argv, 'proc': proc, 'stdout': out, 'stderr': err,
                 'started': time.monotonic(), 'exitRecorded': False}
        self.children.append(child)
        self.save(label + '.process-public.json', {'pid': proc.pid, 'argv': argv,
                  'ownedByThisWorker': True, 'loopbackOnly': True})
        self.own(child)
        return child

    def record_exit(self, child, forced=False):
        proc = self.own(child)
        need(proc.returncode is not None, 'record only joined child')
        if not child['exitRecorded']:
            self.logs(child)
            self.save(child['label'] + '.exit-public.json', {
                'pid': proc.pid, 'argv': child['argv'], 'exitCode': proc.returncode,
                'actualProcessJoined': True, 'forcedKill': forced,
                'elapsedMs': round((time.monotonic() - child['started']) * 1000)})
            child['exitRecorded'] = True

    def terminate(self, child, normal=False, cleanup=False):
        proc = self.own(child)
        was_running = proc.poll() is None
        forced = False
        if was_running:
            proc.terminate()
        try:
            proc.wait(timeout=min(8, self.remaining(cleanup)))
        except subprocess.TimeoutExpired:
            self.own(child).kill()
            forced = True
            proc.wait(timeout=min(3, self.remaining(True)))
        self.record_exit(child, forced)
        child['forcedKill'] = forced
        if forced:
            self.secondary.append({'stage': 'owned-child-cleanup', 'label': child['label'],
                                   'error': 'forced kill after bounded TERM wait'})
        if normal:
            need(was_running and not forced and proc.returncode == 0,
                 'server TERM must join with exit0 before mine/reopen')
            need(child['stderr'].read_bytes() == b'', 'server stderr must stay empty')
            self.stops.append({'pid': proc.pid, 'exitCode': 0, 'actualProcessJoined': True,
                               'forcedKill': False})

    def start_server(self, cycle):
        child = self.launch('serve-' + str(cycle), [str(self.paths['nano']), 'serve',
            str(self.paths['genesis']), str(self.db), '0', '127.0.0.1', str(self.config)])
        cutoff = min(self.action_deadline, time.monotonic() + 12)
        observed = []
        try:
            while True:
                need(self.own(child).poll() is None, 'server exited before readiness')
                self.logs(child)
                raw = child['stdout'].read_bytes()
                error = child['stderr'].read_bytes()
                observed.append({'elapsedMs': round((time.monotonic() - child['started']) * 1000),
                                 'stdoutBytes': len(raw), 'stderrBytes': len(error)})
                need(not error, 'server readiness stderr')
                match = re.fullmatch(rb'Nano JSON-RPC listening on 127\.0\.0\.1:([0-9]+)\n', raw)
                if match:
                    port = int(match.group(1))
                    need(1 <= port <= 65535, 'real ephemeral loopback port')
                    child['port'] = port
                    self.save(child['label'] + '.readiness-public.json', {
                        'passed': True, 'pid': child['proc'].pid, 'host': '127.0.0.1',
                        'port': port, 'observations': observed})
                    return child
                need(time.monotonic() < cutoff, 'server readiness deadline')
                time.sleep(0.05)
        except BaseException as exc:
            try:
                self.save(child['label'] + '.readiness-failure-public.json', {
                    'passed': False, 'pid': child['proc'].pid, 'observations': observed,
                    'failure': type(exc).__name__ + ': ' + str(exc)})
            except BaseException as secondary:
                self.secondary.append({'stage': 'readiness-diagnostic',
                    'error': type(secondary).__name__ + ': ' + str(secondary)})
            raise

    def rpc(self, child, method, params, expected_error=None):
        need(self.own(child).poll() is None, 'HTTP only while owned server is alive')
        need(self.rpc_count < MAX_RPC, 'fewer than80 HTTP requests')
        self.rpc_count += 1
        ident = self.rpc_count
        request = {'jsonrpc': '2.0', 'id': ident, 'method': method, 'params': params}
        record = {'cycle': child['label'], 'request': request, 'response': None, 'failure': None}
        conn = None
        original_error = None
        try:
            conn = http.client.HTTPConnection('127.0.0.1', child['port'],
                                              timeout=min(3, self.remaining()))
            conn.request('POST', '/', json.dumps(request, separators=(',', ':')),
                         {'Content-Type': 'application/json', 'Connection': 'close'})
            response = conn.getresponse()
            record['httpStatus'] = response.status
            length = response.getheader('Content-Length')
            need(response.status == 200 and (length is None or
                 (length.isdigit() and int(length) <= MAX_BODY)), 'bounded HTTP200; no redirect')
            parts = []
            size = 0
            while True:
                self.remaining()
                part = response.read1(min(65536, MAX_BODY + 1 - size))
                if not part:
                    break
                parts.append(part)
                size += len(part)
                need(size <= MAX_BODY, 'bounded HTTP body')
            data = b''.join(parts)
            self.write_bytes('rpc-%03d.response.json' % ident, data)
            value = decode(data)
            record['response'] = value
            need(type(value) is dict and set(value) in
                 ({'jsonrpc', 'id', 'result'}, {'jsonrpc', 'id', 'error'})
                 and value['jsonrpc'] == '2.0' and type(value['id']) is int
                 and value['id'] == ident, 'exact paired JSON-RPC envelope')
            if expected_error is not None:
                need('error' in value and value['error'] == expected_error,
                     'exact expected JSON-RPC refusal: ' + method)
                record['expectedError'] = expected_error
                return value['error']
            need('error' not in value, 'unexpected JSON-RPC error: ' + method)
            return value['result']
        except BaseException as exc:
            original_error = exc
            record['failure'] = type(exc).__name__ + ': ' + str(exc)
            raise
        finally:
            for stage, operation in (('http-close', lambda: conn.close() if conn else None),
                                     ('rpc-ledger', lambda: self.append(self.ledger, record)),
                                     ('child-log-bound', lambda: self.logs(child))):
                try:
                    operation()
                except BaseException as secondary:
                    if original_error is None:
                        raise
                    self.secondary.append({'stage': stage,
                        'error': type(secondary).__name__ + ': ' + str(secondary)})

    def header(self, child, number, previous=None):
        value = self.rpc(child, 'eth_getBlockByNumber', [hex(number), False])
        need(type(value) is dict and value.get('number') == hex(number)
             and HASH.fullmatch(value.get('hash', '')) is not None
             and HASH.fullmatch(value.get('stateRoot', '')) is not None
             and value.get('gasLimit') == self.e['gasLimit'], 'actual canonical header')
        need(value.get('timestamp') == hex(12 * number), 'fixed synthetic timestamp')
        need(value.get('transactions') == ([self.e['txHash']] if number == 1 else []),
             'explicit inclusion/empty-block transaction list')
        if previous is not None:
            need(value.get('parentHash') == previous['hash'], 'contiguous canonical headers')
        if number == 0:
            need(value.get('baseFeePerGas') == self.e['baseFeePerGas'], 'known genesis base fee')
        else:
            need(value.get('miner') == self.e['coinbase'], 'explicit CLI miner address')
        return value

    @staticmethod
    def tag(header):
        return {'blockHash': header['hash'], 'requireCanonical': True}

    def storage(self, child, tag, values):
        for key, value in zip(self.e['storageKeys'], values):
            actual = self.rpc(child, 'eth_getStorageAt', [self.e['target'], key, tag])
            need(actual == '0x' + format(amount(value), '064x'), 'exact 32-byte storage value')

    def state(self, child, header, post):
        tag = self.tag(header)
        suffix = 'After' if post else 'Before'
        need(self.rpc(child, 'eth_getBalance', [self.e['sender'], tag]) ==
             self.e['senderBalance' + suffix], 'sender prepaid/settled balance')
        need(self.rpc(child, 'eth_getTransactionCount', [self.e['sender'], tag]) ==
             self.e['senderNonce' + suffix], 'sender persisted nonce')
        need(self.rpc(child, 'eth_getBalance', [self.e['target'], tag]) ==
             self.e['targetBalance'], 'target balance')
        need(self.rpc(child, 'eth_getTransactionCount', [self.e['target'], tag]) ==
             self.e['targetNonce'], 'target nonce')
        need(self.rpc(child, 'eth_getCode', [self.e['target'], tag]) ==
             self.e['targetCode'], 'target code remains exact')
        self.storage(child, tag, self.e['postValues' if post else 'genesisValues'])

    def historical(self, child, header, post):
        tag = self.tag(header)
        need(self.rpc(child, 'eth_getTransactionCount', [self.e['sender'], tag]) ==
             self.e['senderNonceAfter' if post else 'senderNonceBefore'], 'historical nonce')
        self.storage(child, tag, self.e['postValues' if post else 'genesisValues'])

    def proof(self, child, header, post):
        response = self.rpc(child, 'eth_getProof',
            [self.e['target'], self.e['storageKeys'], self.tag(header)])
        values = self.e['postValues' if post else 'genesisValues']
        need(type(response) is dict and response.get('address') == self.e['target']
             and response.get('nonce') == self.e['targetNonce']
             and response.get('balance') == self.e['targetBalance'], 'proof account fields')
        entries = response.get('storageProof')
        need(type(entries) is list and len(entries) == 2
             and [x.get('key') for x in entries] == self.e['storageKeys']
             and [x.get('value') for x in entries] == values, 'proof ordered storage values')
        self.append(self.proofs, {'scenario': 'proof-vector', 'root': header['stateRoot'],
            'header': header, 'address': self.e['target'], 'keys': self.e['storageKeys'],
            'expectedPresent': True, 'expectedValues': values, 'response': response})
        self.vectors += 1

    def transaction(self, child, header=None):
        tx = self.rpc(child, 'eth_getTransactionByHash', [self.e['txHash']])
        need(type(tx) is dict and tx.get('hash') == self.e['txHash']
             and tx.get('from') == self.e['sender'] and tx.get('to') == self.e['target']
             and tx.get('nonce') == self.e['senderNonceBefore'], 'actual transaction fields')
        need(tx.get('blockHash') == (None if header is None else header['hash']),
             'pending admission is distinct from mined inclusion')
        if header is not None:
            need(tx.get('blockNumber') == header['number'] and tx.get('transactionIndex') == '0x0',
                 'transaction canonical position')
        return tx

    def receipt(self, child, header):
        value = self.rpc(child, 'eth_getTransactionReceipt', [self.e['txHash']])
        need(type(value) is dict and all(value.get(k) == v for k, v in self.e['receipt'].items())
             and value.get('transactionHash') == self.e['txHash']
             and value.get('blockHash') == header['hash']
             and value.get('blockNumber') == header['number']
             and value.get('transactionIndex') == '0x0', 'persisted successful receipt')
        return value

    def mine(self, timestamp, include_raw):
        need(len(self.stops) == (1 if timestamp == 12 else 2), 'joined prior server before mine')
        argv = [str(self.paths['nano']), 'mine', str(self.paths['genesis']), str(self.db),
                self.e['coinbase'], str(timestamp)]
        if include_raw:
            argv.append(str(self.paths['transactions']))
        child = self.launch('mine-' + str(timestamp), argv)
        cutoff = min(self.action_deadline, time.monotonic() + 40)
        while self.own(child).poll() is None:
            self.logs(child)
            need(time.monotonic() < cutoff, 'CLI mine deadline')
            time.sleep(0.05)
        self.own(child).wait(timeout=min(1, self.remaining()))
        self.record_exit(child)
        need(child['proc'].returncode == 0 and child['stderr'].read_bytes() == b'',
             'CLI mine must complete with exit0')
        block = decode(child['stdout'].read_bytes())
        need(type(block) is dict and block.get('number') == hex(timestamp // 12)
             and block.get('timestamp') == hex(timestamp)
             and HASH.fullmatch(block.get('hash', '')) is not None,
             'actual CLI mined header')
        transactions = block.get('transactions')
        need(type(transactions) is list and len(transactions) == (1 if include_raw else 0),
             'CLI explicitly supplies raw only for first mine')
        if include_raw:
            need(transactions[0].get('hash') == self.e['txHash'], 'CLI included same existing raw')
        self.mine_results.append(block)
        return block

    def run(self):
        headers = {}
        child = self.start_server(0)
        need(self.rpc(child, 'eth_chainId', []) == self.e['chainId'], 'chain ID cycle0')
        need(self.rpc(child, 'eth_blockNumber', []) == '0x0', 'genesis before admission')
        headers[0] = self.header(child, 0)
        self.state(child, headers[0], False)
        self.proof(child, headers[0], False)
        need(self.rpc(child, 'eth_accounts', []) == [], 'no wallet accounts loaded')
        self.rpc(child, 'evm_mine', [], expected_error={
            'code': -32601, 'message': 'Development mode is disabled'})
        need(self.rpc(child, 'eth_getTransactionReceipt', [self.e['txHash']]) is None,
             'receipt absent before admission')
        need(self.rpc(child, 'eth_sendRawTransaction', [self.transactions[0]]) == self.e['txHash'],
             'HTTP accepts existing signed raw without signing')
        self.transaction(child)
        need(self.rpc(child, 'eth_getTransactionReceipt', [self.e['txHash']]) is None,
             'HTTP mempool admission is not inclusion')
        need(self.rpc(child, 'eth_getTransactionCount', [self.e['sender'], 'latest']) ==
             self.e['senderNonceBefore'], 'admission does not change canonical nonce')
        need(self.rpc(child, 'eth_getStorageAt', [self.e['target'], '0x1', 'latest']) ==
             '0x' + format(amount(self.e['genesisValues'][1]), '064x'),
             'admission does not execute canonical storage')
        need(self.rpc(child, 'eth_blockNumber', []) == '0x0', 'admission does not mine')
        self.terminate(child, normal=True)
        self.completed_phase('genesis-http-admission', 0)
        mined1 = self.mine(12, True)

        child = self.start_server(1)
        need(self.rpc(child, 'eth_chainId', []) == self.e['chainId'], 'chain ID cycle1')
        need(self.rpc(child, 'eth_blockNumber', []) == '0x1', 'first persisted block')
        headers[1] = self.header(child, 1, headers[0])
        need(headers[1]['hash'] == mined1['hash'], 'HTTP head equals completed CLI block')
        need(self.header(child, 0) == headers[0], 'genesis header survives first reopen')
        transaction = self.transaction(child, headers[1])
        receipt = self.receipt(child, headers[1])
        self.state(child, headers[1], True)
        self.historical(child, headers[0], False)
        self.proof(child, headers[1], True)
        self.proof(child, headers[0], False)
        self.terminate(child, normal=True)
        self.completed_phase('first-reopen-inclusion-history', 1)
        mined2 = self.mine(24, False)

        child = self.start_server(2)
        need(self.rpc(child, 'eth_chainId', []) == self.e['chainId'], 'chain ID cycle2')
        need(self.rpc(child, 'eth_blockNumber', []) == '0x2', 'second persisted block')
        headers[2] = self.header(child, 2, headers[1])
        need(headers[2]['hash'] == mined2['hash'], 'HTTP empty head equals CLI block')
        need(self.header(child, 1, headers[0]) == headers[1], 'included header survives second reopen')
        need(self.header(child, 0) == headers[0], 'genesis survives second reopen')
        need(self.transaction(child, headers[1]) == transaction, 'transaction persists byte-semantically')
        need(self.receipt(child, headers[1]) == receipt, 'receipt persists byte-semantically')
        self.state(child, headers[2], True)
        self.historical(child, headers[1], True)
        self.proof(child, headers[2], True)
        self.proof(child, headers[1], True)
        self.terminate(child, normal=True)
        self.completed_phase('second-reopen-empty-block-persistence', 2)
        need(self.rpc_count == 57 and self.vectors == 5 and len(self.stops) == 3
             and len(self.mine_results) == 2, 'exact bounded scenario completed')
        self.save('headers-public.json', headers)

    def completed_phase(self, name, height):
        value = {'phase': name, 'passed': True, 'height': height,
                 'httpCallsCumulative': self.rpc_count, 'proofVectorsCumulative': self.vectors,
                 'serverStopsJoinedExit0': len(self.stops)}
        self.phases.append(value)
        self.save('phase-' + str(height) + '-public.json', value)

    def finish(self):
        for child in self.children:
            try:
                proc = self.own(child)
                if proc.poll() is None:
                    self.terminate(child, cleanup=True)
                elif not child['exitRecorded']:
                    proc.wait(timeout=min(1, self.remaining(True)))
                    self.record_exit(child)
            except BaseException as exc:
                self.secondary.append({'stage': 'owned-child-cleanup', 'label': child['label'],
                                       'error': type(exc).__name__ + ': ' + str(exc)})
        if self.first_failure is not None:
            try:
                self.save('first-failure-public.json', self.first_failure)
            except BaseException as exc:
                self.secondary.append({'stage': 'first-failure-publication',
                                       'error': type(exc).__name__ + ': ' + str(exc)})
        changes = []
        for path, expected in self.held.items():
            try:
                read(Path(path), max(expected['st_size'], 1), expected, capture=False)
            except BaseException as exc:
                changes.append({'path': path, 'error': type(exc).__name__ + ': ' + str(exc)})
        self.save('input-final-fence-public.json', {'passed': not changes,
                  'selectedFiles': len(self.held), 'failures': changes, 'oldRowsRefreshed': False})
        report = {'format': 'nano-main-http-persistence-worker-result-v1',
            'passed': self.first_failure is None and not self.secondary and not changes
                      and self.rpc_count == 57 and self.vectors == 5 and len(self.stops) == 3
                      and all(x['proc'].returncode is not None for x in self.children),
            'profile': 'Shanghai', 'consensus': 'pow', 'httpRequests': self.rpc_count,
            'httpCalls': self.rpc_count, 'processStarts': len(self.children),
            'mineCommands': sum(x['label'].startswith('mine-') for x in self.children),
            'cleanStops': len(self.stops),
            'allOwnedProcessesJoined': all(x['proc'].returncode is not None for x in self.children),
            'databaseReopened': len(self.phases) == 3, 'phases': self.phases,
            'serverStarts': sum(x['label'].startswith('serve-') for x in self.children),
            'serverStopsJoinedExit0': len(self.stops),
            'cliMineStarts': sum(x['label'].startswith('mine-') for x in self.children),
            'cliMinesCompletedExit0': len(self.mine_results),
            'timestamps': [12, 24], 'databaseReopensCompleted': max(0, len(self.stops) - 1),
            'proofVectors': self.vectors, 'independentProofVerificationPending': True,
            'headerHashRecomputationPending': True, 'rawHashRecoveryIndependentPending': True,
            'mempoolAdmissionPromotedToInclusion': False, 'cliExplicitRawInclusion': True,
            'elapsedMs': round((time.monotonic() - self.started) * 1000),
            'firstFailure': self.first_failure, 'secondaryFailures': self.secondary,
            'inputFencePassed': not changes, 'inputPins': {
                name: {'sha256': self.held[str(path)]['sha256'],
                       'bytes': self.held[str(path)]['st_size']} for name, path in self.paths.items()},
            'emptyWalletAccountsHttpChecked': self.rpc_count == 57 and len(self.phases) == 3,
            'developmentMineHttpRefused': self.rpc_count == 57 and len(self.phases) == 3,
            'newTransactionSignaturesCreated': 0, 'walletCommands': 0, 'newKeysCreated': 0,
            'capellaOrValidatorRuntimeStarted': False, 'devModeEnabled': False,
            'externalRpcRequests': 0, 'beaconFinalityAuthenticated': False,
            'runtimeDutiesResumed': False, 'productionQualified': False}
        self.save('result-public.json', report)
        print(json.dumps(report, sort_keys=True), flush=True)
        return report['passed']


def main():
    started = time.monotonic()
    def interrupted(signum, frame):
        raise RuntimeError('worker interrupted by signal ' + str(signum))
    for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP, signal.SIGALRM):
        signal.signal(signum, interrupted)
    signal.setitimer(signal.ITIMER_REAL, 282)
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('nano', 'genesis', 'transactions', 'expectations', 'output'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    worker = None
    try:
        validated = validate(args)  # No subprocess or socket exists before this returns.
        worker = Worker(validated, started)
        passed = False
        try:
            worker.run()
        except BaseException as exc:
            worker.first_failure = {'type': type(exc).__name__, 'message': str(exc)}
        finally:
            try:
                signal.setitimer(signal.ITIMER_REAL, max(0.001, worker.deadline - time.monotonic()))
            except BaseException as exc:
                worker.secondary.append({'stage': 'cleanup-timer',
                                         'error': type(exc).__name__ + ': ' + str(exc)})
            finally:
                passed = worker.finish()  # Diagnostic/timer failures cannot skip cleanup.
        return 0 if passed else 1
    except BaseException as exc:
        # Preflight/own-output failures are not qualification results. Root retains
        # the actual process exit and stdout/stderr even if no namespace was made.
        print(json.dumps({'format': 'nano-main-http-persistence-controller-refusal-v1',
              'passed': False, 'workerConstructed': worker is not None,
              'firstFailure': None if worker is None else worker.first_failure,
              'error': type(exc).__name__ + ': ' + str(exc),
              'productionQualified': False}), file=sys.stderr, flush=True)
        return 1
    finally:
        signal.setitimer(signal.ITIMER_REAL, 0)


if __name__ == '__main__':
    sys.exit(main())
