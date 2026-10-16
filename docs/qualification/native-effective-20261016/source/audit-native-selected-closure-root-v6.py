#!/usr/bin/env python3
"""ROOT: audit inherited MDs and five NEW preprocessor scans, never compile."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import signal
import stat
import subprocess
import time
import traceback

E = Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
R = Path('/Users/boris/Dev/blockchain-nano')
O = Path(__file__).resolve().parent / 'native-selected-closure-04'
HEAD = 'def5cda61919bbb40e91744a18661a2ad7a76eba'
F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')

def read_code(path, sha, size):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        a = os.fstat(fd)
        assert stat.S_ISREG(a.st_mode) and a.st_size == size
        raw = os.read(fd, size + 1)
        assert len(raw) == size and hashlib.sha256(raw).hexdigest() == sha
        assert tuple(getattr(a,k) for k in F) == tuple(getattr(os.fstat(fd),k) for k in F) == tuple(getattr(os.lstat(path),k) for k in F)
        return raw
    finally:
        os.close(fd)

def git(*args, input=None):
    p = subprocess.run(['/usr/bin/git', '-C', str(R), *args], input=input, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
    assert p.returncode == 0 and not p.stderr, 'git-read-failed'
    return p.stdout

def publish(name, value, mode=0o400):
    raw = (json.dumps(value, sort_keys=True, separators=(',', ':')) + '\n').encode()
    p = O / name
    fd = os.open(p, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, mode)
    try:
        v = memoryview(raw)
        while v:
            n = os.write(fd, v); assert n > 0; v = v[n:]
        os.fsync(fd)
    finally:
        os.close(fd)
    assert p.read_bytes() == raw
    return {'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}

def main():
    start = time.monotonic_ns()
    def interrupted(_signal, _frame):
        raise InterruptedError('selected-closure-interrupted')
    signal.signal(signal.SIGTERM, interrupted)
    O.mkdir(mode=0o700, exist_ok=False)
    # Execute definitions from exactly the reviewed public helper, never main().
    helper = E / 'cross-arch-validator-read-native-controller-v2-20261015/build-native-validator-read-root.py'
    h = {'__name__': 'nano_root_read_only_helpers', '__file__': str(helper)}
    exec(compile(read_code(helper, 'c586c596fe2b9cf44600f2559d24b5fa7dfa007c4e480750902aba9e4607d28d', 31909), str(helper), 'exec'), h)
    hold, members, dep_set = h['hold'], h['archive_members'], h['dependency_set']
    def document(path, sha, size):
        return json.loads(hold(path, {'sha256': sha, 'bytes': size}))
    old = document(E/'historical-beacon-rpc-20261015/controls/native-historical-test-protected-inputs.json', '25c5441c78a55f88f4d4c4255d498f7ceb174ff075833164f8be7e1e2679a633',6869424)
    c = document(E/'cross-arch-validator-read-root-20261015/native-01/inputs.private.json','e5b454d02b291e259bfd57486e2269f8eb2e1e3a18e5dfaa401667ab23af64e8',5672)
    cfg = json.loads(hold(Path(c['oldConfig']['path']), c['oldConfig']))
    sdk = json.loads(hold(Path(c['nativeSdkAuthority']['path']), c['nativeSdkAuthority']))
    first = document(E/'cross-arch-validator-read-root-20261015/native-new-software-first-root-admission.private.json','4d1d0f4ef6d16fff6574d07555a4dd2a6b4b6965b843394b3fd5b2d9d656f206',1903)
    hold(Path(c['sourceManifest']['path']), c['sourceManifest'])
    universal = document(Path(__file__).resolve().parent/'native-original-input-audit.public.json','b731674e4444699c12e8f4d8b7f5638d945365cdc574331a338996cc557dccad',782)
    failures = document(Path(__file__).resolve().parent/'native-original-input-audit.private.json','2bde4d09d09a196e78084d0431980ab059ebf2ded779137172bceee4b1c8c330',3916)
    assert universal['passed'] is False and universal['failureCount'] == len(failures['failures']) == 14
    b, s, current, native = (Path(c[k]) for k in ('buildRoot','sourceRoot','currentSourceRoot','controlRoot'))
    storage_root = Path(__file__).resolve().parent/'native-storage-reproduction-01'
    storage_public = document(storage_root/'public-result.json','6e71264a556f534c38dd95bff1e2a2ed7cbe01c700a16437635c5c9ff20715eb',1060)
    storage = document(storage_root/'admission.private.json','c347ea2c7f436e24af53e197d0a63cef97569892f9f14064d8da628bf90080d9',2426521)
    assert storage_public['passed'] is True and storage_public['storageObjectByteIdenticalToRetainedCoreMember'] is True and storage_public['newMEqualsActualCxxMd'] is True
    assert storage['historicalAliasBindingRecovered'] is False and storage['oldAuthoritiesRefreshed'] is False
    hold(storage_root/'storage.cpp.o',storage['newObject'])
    new_alias = storage['newAliasLiteral']; new_target = storage['newAliasTargetFromOldSdkNamespace']
    assert new_alias == '/opt/homebrew/include/rocksdb/file_system.h' and new_alias not in sdk['sdkAliases']
    choices = [p for p in sdk['sdkFiles'] if p.endswith('/include/rocksdb/file_system.h')]
    assert choices == [new_target] and storage['newAliasAndExistingAliasGuards'][new_alias]['resolved'] == new_target
    h['ALIASES'][new_alias] = storage['newAliasAndExistingAliasGuards'][new_alias]
    for name,row in storage['newParentAliasGuards'].items():
        assert name not in sdk['sdkAliases']
        h['ALIASES'][name] = {'resolved':row['resolvedTarget'],'material':row['material'],'link':row['target']}
    helper_storage = Path(__file__).resolve().parent/'reproduce-native-storage-root.py'
    q = {'__name__':'bounded_root_scan_helpers','__file__':str(helper_storage)}
    exec(compile(read_code(helper_storage,'468584d60e88a5b75db0249b930fb28d580168b70aa048b2c1f0dd915e457248',10406),str(helper_storage),'exec'),q)
    q['O'] = O
    bounded_run = q['run']
    assert current == R and c['sourceBaseRevision'] == 'ae039e1c6fa4dbc6b1ed705a6f27a96956ba8050'
    required, deps_by_role, project = set(), {}, {}
    def prior(path):
        path = Path(os.path.normpath(str(path)))
        assert str(path) in old, 'missing-original-authority'
        required.add(str(path))
        return hold(path, old[str(path)])
    def source(path):
        path = Path(path)
        if path.is_relative_to(current):
            rel = str(path.relative_to(current)); pin = h['API_FILES'].get(rel) or cfg['projectFiles'].get(rel)
            assert pin is not None, 'current-source-authority-missing'
            raw = hold(path, h['byte_pin'](pin))
        elif path.is_relative_to(s):
            rel = str(path.relative_to(s)); raw = prior(path)
        else:
            return False
        digest = {'sha256': hashlib.sha256(raw).hexdigest(), 'bytes':len(raw)}
        assert rel not in project or project[rel] == digest, 'source-relative-byte-conflict'
        project[rel] = digest
        return True
    def sdk_path(path):
        p = Path(path); target = str(p.resolve(strict=True))
        assert target in sdk['sdkFiles'] and old.get(target) == sdk['sdkFiles'][target], 'sdk-target-not-original'
        if str(p) != target:
            assert sdk['sdkAliases'].get(str(p)) == target or (str(p) == sdk.get('sdkRootLocator') and target == sdk.get('sdkRootResolved')) or (str(p) == new_alias and target == new_target)
            h['alias_admit'](str(p), target)
        prior(Path(target))
    def admit_dep(value):
        p = Path(value)
        if source(p): return
        if p.is_relative_to(b): prior(p)
        else: sdk_path(p)
    assert git('rev-parse', 'HEAD').decode().strip() == HEAD and not git('status','--porcelain'), 'source-head-or-worktree-changed'
    for path, pin in sdk['sdkFiles'].items():
        assert old.get(path) == pin
        prior(Path(path))
    for path, target in sdk['sdkAliases'].items():
        h['alias_admit'](path, target)
    if 'sdkRootLocator' in sdk:
        h['alias_admit'](sdk['sdkRootLocator'], sdk['sdkRootResolved'])
    for p in c['tools'].values(): sdk_path(p)
    old_proof = json.loads(hold(Path(c['oldCoreQualification']['path']), c['oldCoreQualification']))
    assert old_proof['status'] == 'PASS_SCOPED_CURRENT_NATIVE_BUILD' and old_proof['genuineMmdPairs'] == 148 and old_proof['oldInputsNineFieldsAndSha256Unchanged'] is True
    old_archive = prior(b/'libnano_core.a')
    old_members = members(old_archive)
    new_archive = hold(native/'libnano_core.a', first['newSoftwareMaterials']['libnano_core.a'])
    new_members = members(new_archive)
    changed = [k for k in old_members if old_members[k] != new_members[k]]
    assert changed == ['capella_beacon_api.cpp.o'], 'not-exactly-one-replaced-member'
    assert len(old_members) == len(new_members) == 143
    assert new_members['storage.cpp.o'] == storage_public['reproducedObject'], 'reproduced-storage-does-not-bind-selected-core'
    hold(native/'terminal.json', first['terminalMaterial'])
    for name, pin in first['newSoftwareMaterials'].items(): hold(native/name, pin)
    archive_lines = [shlex.split(x) for x in prior(Path(c['archiveRecipe']['path'])).decode().splitlines() if x.strip()]
    assert len(archive_lines) == 2 and archive_lines[0][:3] == [c['tools']['archiver'],'qc','libnano_core.a'] and archive_lines[1] == [c['tools']['ranlib'],'libnano_core.a']
    objects = archive_lines[0][3:]
    assert [Path(x).name for x in objects] == list(old_members)
    for item in objects:
        p = b/item; raw = prior(p)
        assert old_members[p.name] == {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
        if p.name == 'capella_beacon_api.cpp.o': continue
        md = prior(Path(str(p)+'.d')); found = dep_set(md,b)
        if p.name == 'storage.cpp.o':
            assert found == set(storage['independentDependencies']), 'storage-reproduction-closure-differs'
        else:
            assert new_alias not in found, 'new-sdk-alias-reaches-another-inherited-unit'
        for dep in sorted(found): admit_dep(dep)
        deps_by_role['inherited:'+p.name] = sorted(found)
    recipes = json.loads(prior(b/'compile_commands.json'))
    def recipe(relative):
        name = str(s/relative); found = [r for r in recipes if r['file'] == name]
        assert len(found) == 1 and found[0]['directory'] == str(b)
        argv = found[0].get('arguments') or shlex.split(found[0]['command'])
        assert argv[0] == c['tools']['compiler'] and argv.count('-o') == argv.count('-c') == argv.count(name) == 1
        assert not any(x in argv for x in ('-M','-MM','-MD','-MMD','-MF','-MT','-MQ','-MP'))
        return argv
    def current_compile(actual, template):
        argv = recipe(template); argv[argv.index(str(s/template))] = str(current/actual)
        argv.insert(1,'-iquote'+str((s/template).parent)); return argv
    compiles = {'core':current_compile('src/capella_beacon_api.cpp','src/capella_beacon_api.cpp')}
    for role,(actual,template,_) in h['ROLES'].items(): compiles[role] = current_compile(actual,template)
    for rel, pin in h['API_FILES'].items(): hold(current/rel,pin)
    env = {k:v for k,v in os.environ.items() if not k.startswith(('NANO_','DYLD_')) and k not in ('LD_PRELOAD','CPATH','CPLUS_INCLUDE_PATH','C_INCLUDE_PATH','LIBRARY_PATH','CXXFLAGS','LDFLAGS')}
    events = []
    for i,(role,argv) in enumerate(compiles.items()):
        command = list(argv); at=command.index('-o'); del command[at:at+2]; command.remove('-c'); command += ['-M','-MT','native-effective-'+str(i)]
        h['fence']()
        t=time.monotonic_ns(); stdout,scan_event=bounded_run(command,b,env,'M-'+str(i))
        found=dep_set(stdout,b)
        assert new_alias not in found, 'new-sdk-alias-reaches-an-api-consumer-unit'
        for dep in sorted(found): admit_dep(dep)
        deps_by_role['new-M:'+role]=sorted(found)
        h['fence'](); events.append({'role':role,'exitCode':scan_event['exitCode'],'dependencyCount':len(found),'elapsedMs':(time.monotonic_ns()-t)//1000000})
    for ref in c['runtimePublicFixtures']:
        p=s/ref['relative']; prior(p); hold(p,ref); source(p)
    for role,ref in c['linkRecipes'].items():
        argv=shlex.split(prior(Path(ref['path'])).decode())
        old_consumer = compiles[role][compiles[role].index('-o')+1]
        assert argv[0] == c['tools']['compiler'] and argv.count('libnano_core.a') == argv.count(old_consumer) == 1
        archives,assembly=1,0
        for word in argv[1:]:
            if word in (old_consumer,'libnano_core.a'): continue
            if word.endswith(('.a','.o','.dylib','.tbd')):
                archives+=word.endswith('.a');assembly+=word.endswith('.o')
                p=Path(word); p=p if p.is_absolute() else b/p
                if p.is_relative_to(b): prior(p)
                else: sdk_path(p)
        assert archives==5 and assembly==2,'canonical-five-archives-two-asm-order'
    # Exact source bytes at the effective Git revision, not just an empty diff.
    tree={}
    for row in git('ls-tree','-r','-z',HEAD).split(b'\0'):
        if not row: continue
        meta,path=row.split(b'\t',1);mode,kind,oid=meta.split(b' ')
        if kind==b'blob': tree[path.decode()]=oid.decode()
    assert set(project) <= set(tree), 'reached-project-source-not-in-effective-git-tree'
    names=sorted(project); request=''.join(tree[name]+'\n' for name in names).encode()
    batch=git('cat-file','--batch',input=request); at=0
    for name in names:
        end=batch.index(b'\n',at); fields=batch[at:end].split(b' '); n=int(fields[2]); assert fields[0].decode()==tree[name] and fields[1]==b'blob'
        raw=batch[end+1:end+1+n]; assert len(raw)==n and batch[end+1+n:end+2+n]==b'\n'
        assert project[name] == {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}, 'effective-git-source-bytes-differ'
        at=end+2+n
    assert at==len(batch)
    mismatch_paths={Path(row['path']) for row in failures['failures']}
    assert len(mismatch_paths) == 7 and all(p.is_relative_to(s) for p in mismatch_paths)
    mismatches=sorted(str(p.relative_to(s)) for p in mismatch_paths)
    intersection=[rel for rel in mismatches if str(s/rel) in required]
    assert not intersection, 'original-material-mismatch-is-reached'
    h['fence']()
    assert git('rev-parse','HEAD').decode().strip()==HEAD and not git('status','--porcelain')
    private=publish('closure.private.json',{'originalRequiredPaths':sorted(required),'dependencies':deps_by_role,'heldMaterials':h['HELD'],'newStorageReproductionPublicSha256':'6e71264a556f534c38dd95bff1e2a2ed7cbe01c700a16437635c5c9ff20715eb','prospectiveAliasGuards':h['ALIASES'],'projectSourceBytes':project,'oldAuthoritiesRefreshed':False},0o600)
    result={'format':'nano-native-selected-closure-read-only-public-v1','passed':True,'effectiveSourceRevision':HEAD,'originalApiProducerBase':c['sourceBaseRevision'],'inheritedCoreMembersVerified':142,'inheritedStorageMemberBackedByNewByteIdenticalCompilation':1,'otherInheritedMembers':141,'newStorageReproductionPublicSha256':'6e71264a556f534c38dd95bff1e2a2ed7cbe01c700a16437635c5c9ff20715eb','historicalUnknownAliasBindingRecovered':False,'newProspectiveAliasLocatorCount':1,'newApiMemberFromRetainedProducer':1,'coreMembers':143,'inheritedOriginalMdClosuresVerified':142,'newPreprocessorScans':5,'newScansAreHistoricalMdBytes':False,'reachedProjectSourcesEqualEffectiveGitBytes':len(project),'originalRequiredRegularFilesVerified':len(required),'originalSdkRegularFilesVerified':len(sdk['sdkFiles']),'originalAliasResolvedTargetsVerified':len(sdk['sdkAliases']),'aliasHistoricalNineFieldsClaimed':False,'prospectiveAliasGuardsRecorded':len(h['ALIASES']),'originalMaterialMismatchesOutsideSelectedClosure':mismatches,'originalMaterialMismatchesInsideSelectedClosure':intersection,'historicalUniversalAuditFailurePreserved':True,'canonicalCmakeAtOriginalProducer':True,'effectiveCmakeRecipeEquivalenceQualified':False,'fiveRuntimeFixtureLeavesVerified':True,'currentLoaderClosureVerified':False,'all24RolesAdmitted':False,'oldAuthoritiesRefreshed':False,'compilations':0,'testsRun':0,'runtimeStarted':False,'productionQualified':False,'privateReceipt':private,'events':events,'elapsedMs':(time.monotonic_ns()-start)//1000000}
    receipt=publish('public-result.json',result);print(json.dumps({'result':result,'receipt':receipt}),flush=True)

if __name__=='__main__':
    try: main()
    except BaseException as exc:
        if O.is_dir():
            publish('refusal.private.json',{'errorType':type(exc).__name__,'reason':str(exc),'frames':[{'file':Path(t.filename).name,'line':t.lineno,'function':t.name} for t in traceback.extract_tb(exc.__traceback__)],'productionQualified':False},0o600)
            print(json.dumps({'passed':False,'errorType':type(exc).__name__,'productionQualified':False,'detailsExported':False}),flush=True)
        raise SystemExit(1)
