#!/usr/bin/env python3
"""Root-only Native04 continuation source; no automatic work on import.

The author reads public source only. Root supplies all private descriptors and
executes after review. Previously admitted identities are never refreshed.
"""
import argparse
import hashlib
import json
import os
import pathlib
import re
import signal
import stat
import time
import types

HERE = pathlib.Path(__file__).resolve().parent
E = HERE.parent
CANDIDATE = 'c6cdf780c68eab8b06306d76257bda9028928ea3'
PARENT = E / 'native-rpc-state-proof-controller-source-v3-20261017/controller.py'
PARENT_PIN = {'sha256': '9699b071cc0ddd5fa53f39e51395d4780affd0cf99a86cd9ed888bf540b3dff9', 'bytes': 28303}
ORACLE = PARENT.parent / 'oracles.py'
ORACLE_PIN = {'sha256': '8959fc32b92fe418b6f1692fe29ddb01c796c3b3c1718008bda1bc661f3c47ec', 'bytes': 7684}
PARTIAL_PIN = {'sha256': 'fa1c9230b8f32afa6e3b7d3043db1e94305ab3f53af783aa2040f1803970a80a', 'bytes': 2137996}
BASE_PIN = {'sha256': '7fac899b65881e15c0d026e902b5cee1859ff999d855c4b93585ce4dd5f35064', 'bytes': 13795}
CORE_PROOF_PIN = {'sha256': '663c52a86bc21f7b895f14f6a20c7ead9528d2c1c209b83e5718f739f326c782', 'bytes': 34764}
ORIGINAL_CORE_PIN = {'sha256': '3401bf777c09f98b2ce4c035c336176dcdbe3097926c232f524670cff697fc37', 'bytes': 29252992}
CANDIDATE_CORE_PIN = {'sha256': '68303178c54e24bffcd66bf7cb16ddd42f790f6748ed42b3669bc7190c3f4b0e', 'bytes': 29359272}
NANO_PIN = {'sha256': 'ac7f9b611a89b7c74523c655c499434dd3debb3bb754bed7fa8167d4fd39ce29', 'bytes': 15160760}
CHECKPOINT_OBJECT_PIN = {'sha256': 'a38690d8e061916cc41d215855fb6c9526dab6c3b6c48a2ae76dffaaeb875613', 'bytes': 460920}
F = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid', 'st_gid', 'st_size', 'st_mtime_ns', 'st_ctime_ns')
p = m = v = fragments = oracle = None
O = None
created = False
module_rows = {}


def need(ok, message):
    if not ok:
        raise ValueError(message)


def pin(body):
    return {'sha256': hashlib.sha256(body).hexdigest(), 'bytes': len(body)}


def material(path, maximum=16777216):
    path = pathlib.Path(path)
    need(path.is_absolute() and path.resolve(strict=True) == path, 'physical input')
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        first = os.fstat(fd)
        row = {key: getattr(first, key) for key in F}
        need(stat.S_ISREG(first.st_mode) and first.st_nlink == 1 and 0 < first.st_size <= maximum, 'bounded regular input')
        body = bytearray()
        while chunk := os.read(fd, 1048576):
            body.extend(chunk)
            need(len(body) <= first.st_size, 'input grew')
        need(len(body) == first.st_size and row == {key: getattr(os.fstat(fd), key) for key in F} == {key: getattr(path.lstat(), key) for key in F}, 'FD/path nine changed')
        return dict(row, sha256=hashlib.sha256(body).hexdigest()), bytes(body)
    finally:
        os.close(fd)


def module(path, expected, name):
    row, body = material(path, 131072)
    need(pin(body) == expected, 'frozen public module pin')
    module_rows[str(path)] = row
    value = types.ModuleType(name)
    value.__file__ = str(path)
    exec(compile(body, str(path), 'exec'), value.__dict__)
    return value


def descriptor(spec, expected=None):
    need(type(spec) is dict and set(spec) == {'path', 'sha256', 'bytes'}, 'exact Root descriptor')
    value = {key: spec[key] for key in ('sha256', 'bytes')}
    need(type(value['bytes']) is int and 0 < value['bytes'] <= 536870912 and type(value['sha256']) is str and re.fullmatch('[0-9a-f]{64}', value['sha256']), 'Root descriptor byte pin')
    need(expected is None or value == expected, 'fixed retained descriptor pin')
    path = pathlib.Path(spec['path'])
    row, body = m.material(path)
    need(pin(body) == value, 'retained descriptor bytes changed')
    need(str(path) not in m.HELD or m.HELD[str(path)] == row, 'retained descriptor identity refresh refused')
    m.HELD[str(path)] = row
    return p.json_body(body)


def retained_product(spec, expected=None):
    need(type(spec) is dict and set(spec) == {'path', 'sha256', 'bytes'}, 'retained product descriptor')
    path = pathlib.Path(spec['path'])
    need(str(path) in m.PRODUCTS and str(path) in m.AUTH, 'previously admitted product original row required')
    body = m.hold(path, m.PRODUCTS[str(path)])
    wanted = {key: spec[key] for key in ('sha256', 'bytes')}
    need(pin(body) == wanted and (expected is None or wanted == expected), 'retained product original identity and explicit bytes')
    return path, body


def main():
    global p, m, v, fragments, oracle, O, created
    parser = argparse.ArgumentParser()
    parser.add_argument('control')
    parser.add_argument('sha256')
    parser.add_argument('bytes', type=int)
    args = parser.parse_args()
    cp = pathlib.Path(args.control)
    control_row, raw = material(cp)
    need(pin(raw) == {'sha256': args.sha256, 'bytes': args.bytes}, 'Root control exact pin')
    p = module(PARENT, PARENT_PIN, 'native04_parent_public_definitions')
    control = p.json_body(raw)
    fields = {'format', 'ready', 'repoRoot', 'expectedHead', 'candidateRevision', 'sourceRoot', 'outputRoot', 'timeoutSeconds', 'baseControl', 'partialDescriptor', 'coreProofDescriptor', 'originalCore', 'candidateCore', 'retainedUnits', 'linkTemplate'}
    need(type(control) is dict and set(control) == fields and control['format'] == 'nano-native-rpc-state-proof-continuation-control-v1' and control['ready'] is True, 'exact ready Root continuation control')
    need(control['candidateRevision'] == CANDIDATE and re.fullmatch('[0-9a-f]{40}', control['expectedHead']), 'exact candidate/worktree revisions')
    need(os.getuid() == 501 and os.getgid() == 20 and os.uname().sysname == 'Darwin' and os.uname().machine == 'arm64', 'Native owner/platform')
    need(type(control['timeoutSeconds']) is int and 600 <= control['timeoutSeconds'] <= 14400, 'Root chosen bounded deadline')
    R = pathlib.Path(control['repoRoot'])
    source = pathlib.Path(control['sourceRoot'])
    for root in (R, source):
        need(root.is_absolute() and root.resolve(strict=True) == root and root.is_dir(), 'physical repository/source root')
    O = pathlib.Path(control['outputRoot'])
    need(O.is_absolute() and O.parent == E and re.fullmatch('native-rpc-state-proof-continuation-root-20261018-[a-z0-9-]{1,40}', O.name) and not O.exists() and not O.is_symlink(), 'fresh continuation output')
    loaded = {}
    for name in ('v', 'm', 'admission', 'fragments'):
        rel, sha, size = p.MODULES[name]
        loaded[name] = module(E / rel, {'sha256': sha, 'bytes': size}, 'native04_continuation_' + name)
    v, m, fragments = loaded['v'], loaded['m'], loaded['fragments']
    oracle = module(ORACLE, ORACLE_PIN, 'native04_continuation_oracles')
    O.mkdir(mode=0o700)
    created = True
    (O / 'tmp').mkdir(mode=0o700)
    (O / 'probe-home').mkdir(mode=0o700)
    v.m = m
    v.R = R
    v.O = p.O = m.O = O
    p.v = v
    p.m = m
    m.START = time.monotonic()
    m.DEADLINE = m.START + control['timeoutSeconds']
    m.ENV = {'PATH': '/usr/bin:/bin:/usr/sbin:/sbin', 'HOME': str(O / 'probe-home'), 'TMPDIR': str(O / 'tmp'), 'LANG': 'C', 'LC_ALL': 'C', 'DEVELOPER_DIR': '/Library/Developer/CommandLineTools'}
    m.run = p.run
    loaded['admission'].install(m)
    signal.signal(signal.SIGHUP, m.stop)
    for path, original in module_rows.items():
        need(m.material(path)[0] == original, 'public module unchanged after import')
        m.HELD[path] = original
    m.HELD[str(cp)] = control_row
    self_path = pathlib.Path(__file__).resolve()
    m.HELD[str(self_path)] = m.material(self_path)[0]
    base = descriptor(control['baseControl'], BASE_PIN)
    prior = descriptor(control['partialDescriptor'], PARTIAL_PIN)
    need(prior['format'] == 'nano-native-rpc-state-proof-partial-software-v1' and prior['sourceRevision'] == CANDIDATE and prior['firstFailurePreserved'] is True and prior['oldAuthoritiesRefreshed'] is False, 'exact terminal partial episode')
    for key in ('files', 'aliases', 'products', 'newBytePinsNotNecessarilyAdmitted'):
        need(type(prior[key]) is dict, 'terminal partial authority maps')
    need(len(prior['products']) == 94, 'all94 previously admitted products required')
    # These are original rows from the pinned failed episode. No current stat
    # becomes an old authority; the unadmitted byte-pin map is never imported.
    v.merge(m.AUTH, prior['files'])
    v.merge(m.AUTH, prior['products'])
    v.merge(m.HELD, prior['files'])
    v.merge(m.PRODUCTS, prior['products'])
    v.merge(m.AA, prior['aliases'])
    v.merge(m.ALIASES, prior['aliases'])
    m.fence()
    old_output = pathlib.Path(control['partialDescriptor']['path']).parent
    need(base['ready'] is True and base['format'] == 'nano-native-rpc-state-proof-root-control-v1' and base['candidateRevision'] == CANDIDATE and base['sourceRoot'] == str(source), 'exact source/control continuity, no new projection')
    old_steps = prior['steps']
    need(type(old_steps) is list and len(old_steps) == 194 and all(type(step) is dict and type(step.get('exitCode')) is int and step['exitCode'] == 0 for step in old_steps), '194 successful commands of the failed episode retained')
    need(len({step['step'] for step in old_steps}) == len(old_steps), 'unique prior command labels')
    need(sum(step['step'].startswith('M-') for step in old_steps) == 145 and sum(step['step'].endswith('-compile') for step in old_steps) == 46 and sum(step['step'].startswith('LINK-') for step in old_steps) == 1 and not any(step['step'].startswith(('TEST-', 'CLI-')) for step in old_steps), 'prior143CoreM+two unitM,44CoreCpp+two unitCpp,one Nano link,zero cases')
    inventory_spec = base['newSourceInventory']
    inventory = descriptor(inventory_spec['descriptor'])
    need(p.select(inventory, inventory_spec['revisionAt']) == CANDIDATE, 'candidate inventory revision retained')
    new_pins = p.pinned_map(p.select(inventory, inventory_spec['pinsAt']))
    for rel, expected in new_pins.items():
        m.EXPECTED[str(source / rel)] = expected
    source_entries = list(source.rglob('*'))
    need(not any(path.is_symlink() for path in source_entries) and {str(path.relative_to(source)) for path in source_entries if path.is_file()} == set(new_pins), 'exact original candidate source inventory')
    fixture = 'tests/fixtures/python-rpc.json'
    need(fixture in new_pins, 'legacy runtime fixture remains in candidate inventory')
    need(pin(m.hold(source / fixture)) == new_pins[fixture], 'physical legacy runtime fixture identity and bytes retained')
    need(v.git('rev-parse', 'HEAD').decode().strip() == control['expectedHead'] and not v.git('status', '--porcelain'), 'clean expected main worktree')
    cmake = m.hold(source / 'CMakeLists.txt').decode()
    database_path = v.B / 'compile_commands.json'
    need(str(database_path) in m.AUTH, 'original canonical Native compile DB row')
    database = p.json_body(m.hold(database_path, m.AUTH[str(database_path)]))
    # Preflight all12 compile recipes before any new M/CXX. The two retained
    # objects are not recompiled; their old configuration still gets checked.
    specs = base['units']
    need(type(specs) is dict and set(specs) == set(p.UNITS), 'twelve original unit configurations')
    flags_by_name = {}
    for name, (_, target, rel) in p.UNITS.items():
        spec = specs[name]
        need(set(spec) == {'compileRecipeSource', 'linkRecipeTarget', 'linkRecipeSource', 'sourceMacro', 'addBlstInclude'}, 'original unit recipe fields')
        need(cmake.count('add_executable(' + target + ' ' + rel + ')') == 1, 'candidate exact single-source target')
        declaration = 'target_link_libraries(' + target + ' PRIVATE nano_core' + (' nano_ckzg' if name == 'checkpointExporter' else '') + ')'
        need(cmake.count(declaration) == 1, 'candidate Core or checkpoint Core/ckzg link declaration')
        if name != 'nano':
            for command in ('target_link_options', 'target_link_directories', 'set_target_properties'):
                need(re.search(r'\b' + command + r'\s*\(\s*' + re.escape(target) + r'(?:\s|\))', cmake) is None, 'no target-specific link override on the reused Nano template')
            need(re.search(r'\bset_property\s*\(\s*TARGET\s+' + re.escape(target) + r'(?:\s|\))', cmake) is None, 'no target-specific property override')
        flags_by_name[name] = p.unit_flags((name, spec), database, source, cmake, base['sourceRemaps'])
    need(control['linkTemplate'] == {'target': 'nano', 'recipeSource': 'src/main.cpp'}, 'one explicitly reviewed original Nano link template')
    original_spec = control['originalCore']
    need(original_spec == base['retainedCore'] and {key: original_spec[key] for key in ('sha256', 'bytes')} == ORIGINAL_CORE_PIN, 'qualified original Core3401')
    original_core = pathlib.Path(original_spec['path'])
    need(str(original_core) in m.AUTH, 'original Core3401 full row retained')
    original_bytes = m.hold(original_core, m.AUTH[str(original_core)])
    need(pin(original_bytes) == ORIGINAL_CORE_PIN, 'original Core3401 original bytes')
    candidate_core, candidate_bytes = retained_product(control['candidateCore'], CANDIDATE_CORE_PIN)
    need(candidate_core == old_output / 'libnano_core.a', 'Core belongs to the retained failed episode')
    _, original_main_object = v.recipe(database, 'src/main.cpp')
    prefix, tail = v.link_recipe('nano', original_main_object, candidate_core)
    m.stable([*{values[0] for values in flags_by_name.values()}, prefix[0]], 'preflight-selected-tools')
    # Reconstruct the archive set with the prior admitted objects. No ar/ranlib,
    # dependency scanner or compiler is invoked for this retained Core.
    proof = descriptor(control['coreProofDescriptor'], CORE_PROOF_PIN)
    need(proof['passed'] is True and proof['members'] == 143 and proof['selectedReplacementCount'] == 44 and proof['unchangedOutsideReplacementSet'] == 99, 'pinned143/44/99 completed Core proof')
    original_members = m.archive(original_bytes)
    new_members = m.archive(candidate_bytes)
    need([row[0] for row in original_members] == [row[0] for row in new_members], '143 retained Core member order')
    rows = proof['rows']
    need(type(rows) is list and len(rows) == 143 and [row['member'] for row in rows] == [row[0] for row in original_members], 'proof rows align to all143 original members')
    selected = {row['member'] for row in rows if row['compileSelected'] is True}
    need(len(selected) == 44 and {row['source'] for row in rows if row['compileSelected'] is True} == set(proof['mmd']) and all(value['normalizedMEqualsMd'] is True and value['literalMEqualsMd'] is True for value in proof['mmd'].values()), '44 inherited M/MD selections')
    need([row for row in original_members if row[0] not in selected] == [row for row in new_members if row[0] not in selected], 'all99 unselected member bodies unchanged')
    current_by_name = {row[0]: row for row in new_members}
    for name in sorted(selected):
        obj = old_output / name
        md = obj.with_suffix(obj.suffix + '.d')
        need(str(obj) in m.PRODUCTS and str(md) in m.PRODUCTS, 'selected object and MD had original first product admission')
        body = m.hold(obj, m.PRODUCTS[str(obj)])
        m.hold(md, m.PRODUCTS[str(md)])
        need(current_by_name[name] == (name, len(body), pin(body)['sha256']), 'retained selected member equals its admitted compiled object')
    m.save('retained-core-set-proof-public.json', {'passed': True, 'members': 143, 'selectedReplacementCount': 44, 'unchangedOutsideReplacementSet': 99, 'actualChangedObjectBodies': sum(left != right for left, right in zip(original_members, new_members)), 'archiveUpdates': 0, 'CoreCompilationsRepeated': 0, 'CoreMRepeated': 0, 'sourceProofPin': CORE_PROOF_PIN, 'candidateCorePin': CANDIDATE_CORE_PIN})
    retained = control['retainedUnits']
    need(type(retained) is dict and set(retained) == {'nano', 'checkpointExporter'} and set(retained['nano']) == {'object', 'dependencyFile', 'executable'} and set(retained['checkpointExporter']) == {'object', 'dependencyFile'}, 'two completed object/MD sets, one completed executable')
    retained_objects = {}
    for name, value in retained.items():
        obj, _ = retained_product(value['object'], CHECKPOINT_OBJECT_PIN if name == 'checkpointExporter' else None)
        md, md_bytes = retained_product(value['dependencyFile'])
        need(obj == old_output / ('unit-' + name + '.cpp.o') and md == obj.with_suffix(obj.suffix + '.d'), 'original completed unit object/MD names')
        dependencies = m.deps(md_bytes)
        need(str(source / p.UNITS[name][2]) in dependencies, 'retained MD declares its exact source')
        for logical in dependencies:
            actual = str(pathlib.Path(logical).resolve(strict=True))
            need(actual in m.AUTH, 'retained MD dependency original row required')
            m.hold(logical, m.AUTH[actual])
        retained_objects[name] = obj
    nano, _ = retained_product(retained['nano']['executable'], NANO_PIN)
    need(nano == old_output / 'nano', 'original completed Nano executable')
    programs = {'nano': dict(path=str(nano), **NANO_PIN)}
    stats = {'nano': {'objectReused': True, 'originalMDClosureVerified': True, 'independentMRepeated': False, 'compileRepeated': False, 'linkRepeated': False}, 'checkpointExporter': {'objectReused': True, 'originalMDClosureVerified': True, 'independentMRepeated': False, 'compileRepeated': False}}
    # A current read-only loader inspection of the unchanged executable is not a
    # relink, compiler command or CLI replay. Its providers remain old strict rows.
    loaders = {'nano': v.loader_bindings('nano', nano)}
    for name, (_, target, rel) in p.UNITS.items():
        if name == 'nano':
            continue
        if name == 'checkpointExporter':
            obj = retained_objects[name]
        else:
            obj, stats[name] = fragments.consumer_or_test_block(m, lambda values: sorted(set(values)), 'unit-' + name, flags_by_name[name], source / rel)
        exe = O / target
        m.run('LINK-' + name, prefix + [str(obj), '-o', str(exe)] + tail, seconds=180)
        m.admit_products([exe], name + '-executable', True)
        loaders[name] = v.loader_bindings(name, exe)
        programs[name] = dict(path=str(exe), **pin(m.material(exe)[1]))
    batch = descriptor({'path': str(E / 'rpc-state-proof-test-oracles-source-20261017/test-batch.json'), 'sha256': '17231520964bd3b3e039bff96a5b46f6024c64af853767f9d909779545e4c802', 'bytes': 5545})
    cases = [case for case in batch['testCases'] if case['name'].startswith('candidate-')]
    need(len(cases) == 4 and {case['unit'] for case in cases} == {name for name, value in p.UNITS.items() if value[0] == 'candidate'}, 'four first Native candidate cases')
    results = []
    for case in cases:
        name = case['name']
        raw, _ = m.run('TEST-' + name, [programs[case['unit']]['path'], *case['args']], case['expectedExit'], case['timeoutSeconds'], cwd=source)
        results.append({'name': name, 'passed': True, 'stdout': oracle.stream_oracle(O / ('TEST-' + name + '-stdout.private.log'), case['stdout']), 'stderr': oracle.stream_oracle(O / ('TEST-' + name + '-stderr.private.log'), case['stderr']), 'stdoutPin': pin(raw)})
    runtime_cases = descriptor({'path': str(PARENT.parent / 'runtime-cases.json'), 'sha256': 'aa613ce2e1980a8f73aaf886be81ac5c3c0a1d9edf72cc5749f97ac61b68e068', 'bytes': 7038})
    need(base['runtimeCases'] == runtime_cases and len(runtime_cases) == 9, 'nine frozen source-derived CLI cases not previously invoked')
    for case in runtime_cases:
        need(case['args'] in ([], ['--help']) and (case['unit'] == 'nano' or case['args'] == []), 'pre-IO arity/help only')
        phase = 'CLI-' + case['name']
        m.run(phase, [programs[case['unit']]['path'], *case['args']], case['expectedExit'], 15)
        oracle.stream_oracle(O / (phase + '-stdout.private.log'), case['stdout'])
        oracle.stream_oracle(O / (phase + '-stderr.private.log'), case['stderr'])
    m.fence()
    need(v.git('rev-parse', 'HEAD').decode().strip() == control['expectedHead'] and not v.git('status', '--porcelain'), 'main stayed clean')
    need(not list((O / 'tmp').iterdir()) and not list((O / 'probe-home').iterdir()), 'test/CLI HOME and temp cleaned')
    counts = {'independentM': sum(step['step'].startswith('M-') for step in m.STEPS), 'compiledTranslationUnits': sum(step['step'].endswith('-compile') for step in m.STEPS), 'links': sum(step['step'].startswith('LINK-') for step in m.STEPS), 'testInvocations': sum(step['step'].startswith('TEST-') for step in m.STEPS), 'runtimeCliInvocations': sum(step['step'].startswith('CLI-') for step in m.STEPS)}
    need(counts == {'independentM': 10, 'compiledTranslationUnits': 10, 'links': 11, 'testInvocations': 4, 'runtimeCliInvocations': 9}, 'exact continuation commands; no accomplished M/CXX/link replay')
    m.save('software-admission.private.json', {'format': 'nano-native-rpc-state-proof-continuation-software-v1', 'sourceRevision': CANDIDATE, 'files': m.HELD, 'aliases': m.ALIASES, 'products': m.PRODUCTS, 'programs': programs, 'stats': stats, 'directLoaderBindings': loaders, 'newCore': dict(path=str(candidate_core), **CANDIDATE_CORE_PIN), 'retainedPartialDescriptor': control['partialDescriptor'], 'oldAuthoritiesRefreshed': False})
    m.save('result-public.json', {'format': 'nano-native-rpc-state-proof-continuation-qualification-v1', 'passed': True, 'sourceRevision': CANDIDATE, 'counts': counts, 'cohortCounts': {'independentM': 155, 'coreCompiledTranslationUnits': 44, 'unitCompiledTranslationUnits': 12, 'links': 12, 'archiveUpdates': 1}, 'retainedOriginalProducts': 94, 'retainedNanoObjectCompileOrLinkRepeated': False, 'retainedCheckpointObjectCompileRepeated': False, 'coreMRepeated': 0, 'coreCompilationsRepeated': 0, 'archiveUpdatesRepeated': 0, 'retainedNanoReadOnlyLoaderChecks': 1, 'newExecutableLoaderChecks': 11, 'testResults': results, 'programs': programs, 'candidateCore': CANDIDATE_CORE_PIN, 'previousFailedQualificationReclassified': False, 'rootIndependentProofVectorsVerified': False, 'runtimeServeOrRpcQualified': False, 'runtimeDutiesResumed': False, 'recursiveDylibClosureRequalified': False, 'wholeSdkQualified': False, 'fullCmakeConfigureExecuted': False, 'productionQualified': False, 'oldAuthoritiesRefreshed': False, 'steps': m.STEPS, 'elapsedMs': round((time.monotonic() - m.START) * 1000)})
    print(json.dumps({'passed': True, 'counts': counts, 'productionQualified': False}), flush=True)


def entry():
    first = None
    try:
        main()
    except BaseException as error:
        first = error
    finally:
        if created and m is not None:
            for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
                signal.signal(sig, signal.SIG_IGN)
            try:
                v.final_fence()
                m.save('final-fence-public.json', {'passed': True, 'firstFailurePreserved': first is not None, 'oldAuthoritiesRefreshed': False})
            except BaseException as error:
                if first is None:
                    first = error
                try:
                    m.save('final-fence-failure-public.json', {'passed': False, 'firstFailurePreserved': True, 'oldAuthoritiesRefreshed': False})
                except BaseException:
                    pass
            if first is not None:
                try:
                    m.save('partial-software-admission.private.json', {'format': 'nano-native-rpc-state-proof-continuation-partial-software-v1', 'sourceRevision': CANDIDATE, 'files': m.HELD, 'aliases': m.ALIASES, 'products': m.PRODUCTS, 'newBytePinsNotNecessarilyAdmitted': m.NEW, 'steps': m.STEPS, 'firstFailurePreserved': True, 'oldAuthoritiesRefreshed': False})
                    m.save('failure-public.json', {'passed': False, 'errorType': type(first).__name__, 'message': str(first), 'productionQualified': False})
                except BaseException:
                    pass
    if first is not None:
        raise first


if __name__ == '__main__':
    entry()
