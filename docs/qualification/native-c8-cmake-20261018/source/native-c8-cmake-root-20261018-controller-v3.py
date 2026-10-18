#!/usr/bin/env python3
"""One fresh native CMake build; only eight scoped synthetic test families."""
import hashlib, json, os, signal, stat, subprocess, sys, tarfile, time
from pathlib import Path, PurePosixPath

E = Path(__file__).resolve().parent
O = E / 'native-c8-cmake-root-20261018-03'
R = Path('/Users/boris/Dev/blockchain-nano')
REV = 'a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7'
GIT = '/Library/Developer/CommandLineTools/usr/bin/git'
CMAKE = '/opt/homebrew/Cellar/cmake/4.4.3/bin/cmake'
CTEST = '/opt/homebrew/Cellar/cmake/4.4.3/bin/ctest'
CLANG = '/Library/Developer/CommandLineTools/usr/bin/clang'
CLANGXX = '/Library/Developer/CommandLineTools/usr/bin/clang++'
NAMES = ['cli_readonly_core', 'chain', 'block_auth',
         'consensus_capella_bootstrap_startup', 'beacon_execution_gate',
         'recovery_audit_paris', 'recovery_audit_shanghai']
TARGETS = ['nano', 'nano-cli-readonly-tests', 'nano-cli-readonly-probe',
           'nano-chain-tests', 'nano-block-auth-tests',
           'nano-capella-bootstrap-startup-tests', 'nano-beacon-execution-gate-tests',
           'nano-recovery-audit-tests', 'nano-storage-maintenance',
           'nano-bootstrap-archive-maintenance', 'nano-capella-duty-readiness']

def sha(b): return hashlib.sha256(b).hexdigest()
def dump(p, obj):
    with p.open('x') as f:
        json.dump(obj, f, indent=2, sort_keys=True); f.write('\n'); f.flush(); os.fsync(f.fileno())
def git(*args): return subprocess.check_output([GIT, '-C', str(R), *args])
def pin(p):
    p = Path(p)
    a = p.lstat()
    assert stat.S_ISREG(a.st_mode) and a.st_nlink == 1, str(p)
    b = p.read_bytes(); z = p.lstat()
    def row(s): return [s.st_dev,s.st_ino,s.st_mode,s.st_nlink,s.st_uid,s.st_gid,s.st_size,s.st_mtime_ns,s.st_ctime_ns]
    assert row(a) == row(z) and len(b) == a.st_size, str(p)
    return {'file':str(p), 'sha256':sha(b), 'bytes':len(b), 'nine':row(a)}
def check(rows):
    for row in rows: assert pin(row['file']) == row, row['file']

O.mkdir(mode=0o700)
S=Path('/private/tmp/nano-native-c8-cmake-source-20261018-03'); B=O/'build'; T=O/'tmp'; I=O/'install'
S.mkdir(mode=0o700); T.mkdir(mode=0o700)
env={'PATH':'/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin','LANG':'C','LC_ALL':'C','TMPDIR':str(T)}
report={'format':'nano-native-c8-cmake-result-v1','sourceRevision':REV,'productionQualified':False,
        'status':'RUNNING','phases':[], 'imageBuilds':0,'oldDataOpened':False,
        'runtimeDutiesStarted':False,'networkStarted':False,
        'sourceProjectionExcludes':'Nested docs evidence only; top-level docs retained for installation.'}
guards=[]

def run(name, args, timeout):
    check(guards)
    started=time.monotonic(); path=O/(name+'.log')
    print(json.dumps({'phase':name,'status':'START','log':str(path)}),flush=True)
    with path.open('xb') as log:
        p=subprocess.Popen(args,cwd=S,env=env,stdin=subprocess.DEVNULL,
                           stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        try: code=p.wait(timeout=timeout)
        except BaseException:
            assert p.pid>1 and p.pid!=28722
            os.killpg(p.pid,signal.SIGTERM)
            try:p.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=5)
            raise
        log.flush();os.fsync(log.fileno())
    phase={'name':name,'argv':args,'exitCode':code,'elapsedSeconds':round(time.monotonic()-started,3),
           'log':pin(path)}
    report['phases'].append(phase)
    check(guards)
    print(json.dumps({'phase':name,'exitCode':code,'elapsedSeconds':phase['elapsedSeconds']}),flush=True)
    assert code==0, name+' failed; see '+str(path)
    return path.read_text()

try:
    assert git('rev-parse','HEAD').decode().strip()==REV
    assert not git('status','--porcelain'), 'source checkout not clean'
    raw=git('ls-tree','-r','-z','-l',REV); selected=[]; omitted=[]
    for entry in raw.split(b'\0'):
        if not entry:continue
        head,name=entry.split(b'\t'); mode,kind,oid,size=head.decode().split(); name=name.decode()
        assert mode in ('100644','100755') and kind=='blob', name
        row={'path':name,'gitBlob':oid,'bytes':int(size),'gitMode':mode}
        (omitted if name.startswith('docs/') and len(PurePosixPath(name).parts)>2 else selected).append(row)
    roots=sorted({r['path'].split('/')[0] for r in selected if not r['path'].startswith('docs/')})
    roots += [r['path'] for r in selected if r['path'].startswith('docs/')]
    wanted={r['path']:r for r in selected}
    p=subprocess.Popen([GIT,'-C',str(R),'archive','--format=tar',REV,*roots],stdout=subprocess.PIPE)
    with tarfile.open(fileobj=p.stdout,mode='r|') as archive:
        for m in archive:
            name=m.name.rstrip('/'); dest=S/name
            assert not PurePosixPath(name).is_absolute() and '..' not in PurePosixPath(name).parts
            if m.isdir():dest.mkdir(parents=True,exist_ok=True);continue
            assert m.isfile() and name in wanted, name
            row=wanted[name]; body=archive.extractfile(m).read()
            assert len(body)==row['bytes']
            assert hashlib.sha1(('blob '+str(len(body))+'\0').encode()+body).hexdigest()==row['gitBlob']
            dest.parent.mkdir(parents=True,exist_ok=True)
            with dest.open('xb') as f:f.write(body)
            dest.chmod(0o500 if row['gitMode']=='100755' else 0o400)
            row['sha256']=sha(body)
    p.stdout.close(); assert p.wait()==0
    assert all('sha256' in r for r in selected)
    header=S/'tests/support/rocksdb.hpp'
    baseline=pin(header)
    candidate=(E/'rocksdb-test-helper-source-proposal-20261018-01/source/tests/support/rocksdb.hpp').read_bytes()
    assert sha(candidate)=='a128ab10f79c3207412dc0a8a224ee699d865139b1b0cc9fdd0da8de5aa96376'
    header.chmod(0o600);header.write_bytes(candidate);header.chmod(0o400)
    row=next(r for r in selected if r['path']=='tests/support/rocksdb.hpp')
    row['baseGitBlob']=row['gitBlob'];row['baseSha256']=row['sha256']
    row['bytes']=len(candidate);row['sha256']=sha(candidate)
    row['gitBlob']=hashlib.sha1(('blob '+str(len(candidate))+'\0').encode()+candidate).hexdigest()
    report['testOnlySourceDelta']={'path':'tests/support/rocksdb.hpp','base':baseline,'sha256':sha(candidate),'bytes':len(candidate)}
    report['productionCppChanged']=False
    for d in sorted((x for x in S.rglob('*') if x.is_dir()),key=lambda x:len(x.parts),reverse=True):d.chmod(0o500)
    S.chmod(0o500)
    dump(O/'source-projection-public.json',{'revision':REV,'files':selected,'omittedFiles':len(omitted),
          'omittedBytes':sum(r['bytes'] for r in omitted)})
    guards=[pin(S/r['path']) for r in selected]
    # Only this new source projection is admitted, before any CMake process.
    for _ in range(3):
        time.sleep(1);check(guards)
    providers={
      'rocksdb':Path('/opt/homebrew/lib/librocksdb.dylib').resolve(strict=True),
      'gmp':Path('/opt/homebrew/lib/libgmp.dylib').resolve(strict=True),
      'gmpxx':Path('/opt/homebrew/lib/libgmpxx.dylib').resolve(strict=True),
      'ssl':Path('/opt/homebrew/opt/openssl@3/lib/libssl.dylib').resolve(strict=True),
      'crypto':Path('/opt/homebrew/opt/openssl@3/lib/libcrypto.dylib').resolve(strict=True),
      'icu':Path('/opt/homebrew/opt/icu4c/lib/libicuuc.dylib').resolve(strict=True)}
    software=[pin(Path(x).resolve(strict=True)) for x in (GIT,CMAKE,CTEST,CLANG,CLANGXX)]
    software += [pin(p) for p in providers.values()]
    guards+=software
    dump(O/'inputs-private.json',guards)
    report['sourceFileCount']=len(selected);report['sourceBytes']=sum(r['bytes'] for r in selected)
    report['software']=software
    openssl=str(providers['ssl'].parent.parent)
    icu=str(providers['icu'].parent.parent)
    rocks=str(providers['rocksdb'].parent.parent)
    gmp=str(providers['gmp'].parent.parent)
    boost=str(Path('/opt/homebrew/include/boost/beast.hpp').resolve(strict=True).parent.parent)
    run('configure',[CMAKE,'-S',str(S),'-B',str(B),'-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_OSX_ARCHITECTURES=arm64','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
        '-DCMAKE_C_COMPILER='+CLANG,'-DCMAKE_CXX_COMPILER='+CLANGXX,'-DMCL_PORTABLE=ON',
        '-DNANO_NETWORK_EXECUTABLE=','-DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON',
        '-DOPENSSL_ROOT_DIR='+openssl,'-DICU_ROOT='+icu,'-DBOOST_INCLUDE_DIR='+boost,
        '-DGMP_INCLUDE_DIR='+gmp+'/include','-DGMP_LIBRARY='+str(providers['gmp']),
        '-DGMPXX_LIBRARY='+str(providers['gmpxx']),'-DROCKSDB_INCLUDE_DIR='+rocks+'/include',
        '-DROCKSDB_LIBRARY='+str(providers['rocksdb'])],120)
    run('build',[CMAKE,'--build',str(B),'--target',*TARGETS,'--parallel','2'],1800)
    pattern='^('+'|'.join(NAMES)+')$'
    registration=json.loads(run('ctest-registration',[CTEST,'--test-dir',str(B),'--show-only=json-v1','-R',pattern],30))
    assert sorted(t['name'] for t in registration['tests'])==sorted(NAMES)
    run('ctest',[CTEST,'--test-dir',str(B),'--output-on-failure','--verbose','--parallel','1','-R',pattern],360)
    nano=pin(B/'nano'); report['nano']=nano
    body=run('cli-probe',[str(B/'nano-cli-readonly-probe'),str(B/'nano'),nano['sha256'],str(nano['bytes']),
                         str(O/'synthetic-cli-readonly-native-c8')],210)
    probe=json.loads(body)
    assert probe['result']=='PASS' and probe['actualCliCommands']==18
    report['probe']=probe
    run('install',[CMAKE,'--install',str(B),'--prefix',str(I)],120)
    notices=I/'share/nano/licenses'; rows=[]
    for line in (notices/'notices.sha256').read_text().splitlines():
        digest,name=line.split('  '); actual=pin(notices/name);assert digest==actual['sha256'],name
        rows.append(actual)
    assert len(rows)==11
    report['installedNotices']=rows
    binaries=[I/'bin'/x for x in ['nano','nano-storage-maintenance','nano-bootstrap-archive-maintenance','nano-capella-duty-readiness']]
    report['installedBinaries']=[pin(x) for x in binaries]
    run('installed-load-commands',['/usr/bin/otool','-L',*[str(x) for x in binaries]],30)
    run('installed-help',[str(I/'bin/nano'),'--help'],30)
    check(guards)
    report['status']='PASS'
except BaseException as error:
    report['status']='FAILED';report['error']=str(error)
finally:
    dump(O/'result-public.json',report)
    print(json.dumps({'status':report['status'],'report':str(O/'result-public.json'),'phases':len(report['phases']),
                      'error':report.get('error')}),flush=True)
sys.exit(0 if report['status']=='PASS' else 1)
