#!/usr/bin/env python3
"""One explicit storage-object reproduction; no historical alias refresh."""
import hashlib
import json
import os
from pathlib import Path
import selectors
import shlex
import signal
import stat
import subprocess
import time
import traceback

E=Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
O=Path(__file__).resolve().parent/'native-storage-reproduction-01'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
ALIAS='/opt/homebrew/include/rocksdb/file_system.h'

def save(name,value,mode=0o400):
    raw=(json.dumps(value,sort_keys=True,separators=(',',':'))+'\n').encode();p=O/name
    fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,mode)
    try:
        v=memoryview(raw)
        while v:
            n=os.write(fd,v);assert n>0;v=v[n:]
        os.fsync(fd)
    finally:os.close(fd)
    assert p.read_bytes()==raw
    return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}

def run(command,cwd,env,label):
    start=time.monotonic_ns();p=subprocess.Popen(command,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    streams={p.stdout:bytearray(),p.stderr:bytearray()};sel=selectors.DefaultSelector()
    for pipe in streams:os.set_blocking(pipe.fileno(),False);sel.register(pipe,selectors.EVENT_READ)
    try:
        deadline=time.monotonic()+180
        while sel.get_map():
            assert time.monotonic()<deadline,'bounded-command-deadline'
            for key,_ in sel.select(min(1,max(0,deadline-time.monotonic()))):
                pipe=key.fileobj;chunk=os.read(pipe.fileno(),65536)
                if not chunk:sel.unregister(pipe);continue
                streams[pipe].extend(chunk);assert len(streams[pipe])<=2097152,'stream-byte-limit'
        code=p.wait(timeout=max(0.01,deadline-time.monotonic()))
    except BaseException:
        try:os.killpg(p.pid,signal.SIGKILL)
        except ProcessLookupError:pass
        p.wait(timeout=10);raise
    finally:sel.close();p.stdout.close();p.stderr.close()
    stdout,stderr=(bytes(streams[x]) for x in (p.stdout,p.stderr))
    save(label+'.private.json',{'argv':command,'cwd':str(cwd),'exitCode':code,'stdout':stdout.decode(),'stderr':stderr.decode()},0o600)
    assert code==0 and not stderr,'compiler-command-refused'
    return stdout,{'kind':label,'exitCode':code,'elapsedMs':(time.monotonic_ns()-start)//1000000}

def main():
    def interrupted(_signal,_frame):
        raise InterruptedError('root-reproduction-interrupted')
    signal.signal(signal.SIGTERM,interrupted)
    O.mkdir(mode=0o700,exist_ok=False);start=time.monotonic_ns()
    helper=E/'cross-arch-validator-read-native-controller-v2-20261015/build-native-validator-read-root.py'
    raw=helper.read_bytes();assert hashlib.sha256(raw).hexdigest()=='c586c596fe2b9cf44600f2559d24b5fa7dfa007c4e480750902aba9e4607d28d' and len(raw)==31909
    h={'__name__':'nano_root_reproduction_helpers','__file__':str(helper)};exec(compile(raw,str(helper),'exec'),h)
    hold=h['hold']
    def doc(path,sha,size):return json.loads(hold(path,{'sha256':sha,'bytes':size}))
    old=doc(E/'historical-beacon-rpc-20261015/controls/native-historical-test-protected-inputs.json','25c5441c78a55f88f4d4c4255d498f7ceb174ff075833164f8be7e1e2679a633',6869424)
    c=doc(E/'cross-arch-validator-read-root-20261015/native-01/inputs.private.json','e5b454d02b291e259bfd57486e2269f8eb2e1e3a18e5dfaa401667ab23af64e8',5672)
    first=doc(E/'cross-arch-validator-read-root-20261015/native-new-software-first-root-admission.private.json','4d1d0f4ef6d16fff6574d07555a4dd2a6b4b6965b843394b3fd5b2d9d656f206',1903)
    sdk=json.loads(hold(Path(c['nativeSdkAuthority']['path']),c['nativeSdkAuthority']))
    b,s,native=(Path(c[k]) for k in ('buildRoot','sourceRoot','controlRoot'))
    def prior(path):
        p=Path(os.path.normpath(str(path)));assert str(p) in old,'old-authority-missing';return hold(p,old[str(p)])
    for path,pin in sdk['sdkFiles'].items():
        assert old.get(path)==pin,'sdk-original-authority-conflict';prior(Path(path))
    for path,target in sdk['sdkAliases'].items():h['alias_admit'](path,target)
    if 'sdkRootLocator' in sdk:h['alias_admit'](sdk['sdkRootLocator'],sdk['sdkRootResolved'])
    # Select the target from the OLD canonical namespace, not from current resolve().
    choices=[p for p in sdk['sdkFiles'] if p.endswith('/include/rocksdb/file_system.h')]
    assert len(choices)==1 and ALIAS not in sdk['sdkAliases'],'exact-one-new-alias-boundary'
    target=choices[0];assert str(Path(ALIAS).resolve(strict=True))==target,'new-alias-target-not-old-canonical-sdk-file'
    new_alias_guards={}
    for p in [*reversed(Path(ALIAS).parents),Path(ALIAS)]:
        row=p.lstat()
        if stat.S_ISLNK(row.st_mode):
            new_alias_guards[str(p)]={'material':{k:getattr(row,k) for k in F},'target':os.readlink(p),'resolvedTarget':str(p.resolve(strict=True))}
    assert new_alias_guards,'new-alias-has-no-parent-symlink'
    h['alias_admit'](ALIAS,target)
    def fence():
        h['fence']()
        for name,row in new_alias_guards.items():
            p=Path(name);assert {k:getattr(p.lstat(),k) for k in F}==row['material'] and os.readlink(p)==row['target'] and str(p.resolve(strict=True))==row['resolvedTarget'],'new-alias-component-moved'
        assert str(Path(ALIAS).resolve(strict=True))==target
    def admit_dep(value):
        p=Path(value)
        if p.is_relative_to(s) or p.is_relative_to(b):prior(p);return
        resolved=str(p.resolve(strict=True));assert resolved in sdk['sdkFiles'],'unadmitted-sdk-regular'
        if str(p)!=resolved:
            assert sdk['sdkAliases'].get(str(p))==resolved or (str(p)==ALIAS and resolved==target),'another-new-alias-refused'
            h['alias_admit'](str(p),resolved)
        prior(Path(resolved))
    recipes=json.loads(prior(b/'compile_commands.json'));found=[r for r in recipes if r['file']==str(s/'src/storage.cpp')]
    assert len(found)==1 and found[0]['directory']==str(b),'unique-original-storage-recipe'
    argv=found[0].get('arguments') or shlex.split(found[0]['command'])
    assert argv[0]==c['tools']['compiler'] and argv.count('-o')==argv.count('-c')==1 and argv.count(str(s/'src/storage.cpp'))==1
    assert not any(x in argv for x in ('-M','-MM','-MD','-MMD','-MF','-MT','-MQ','-MP'))
    admit_dep(argv[0]);prior(s/'src/storage.cpp')
    original_object=b/argv[argv.index('-o')+1];expected_raw=prior(original_object)
    expected={'sha256':hashlib.sha256(expected_raw).hexdigest(),'bytes':len(expected_raw)}
    archive=hold(native/'libnano_core.a',first['newSoftwareMaterials']['libnano_core.a'])
    assert h['archive_members'](archive)['storage.cpp.o']==expected,'retained-core-storage-member-differs'
    original_md=prior(Path(str(original_object)+'.d'));historical=h['dependency_set'](original_md,b)
    assert ALIAS in historical,'alias-not-reached-by-this-unit'
    env={k:v for k,v in os.environ.items() if not k.startswith(('NANO_','DYLD_')) and k not in ('LD_PRELOAD','CPATH','CPLUS_INCLUDE_PATH','C_INCLUDE_PATH','LIBRARY_PATH','CXXFLAGS','LDFLAGS')}
    m=list(argv);at=m.index('-o');del m[at:at+2];m.remove('-c');m+=['-M','-MT','storage-reproduction']
    fence();stdout,m_event=run(m,b,env,'M');deps=h['dependency_set'](stdout,b)
    assert deps==historical,'new-M-differs-from-pinned-historical-MD'
    for dep in sorted(deps):admit_dep(dep)
    fence();obj=O/'storage.cpp.o';md=O/'storage.cpp.o.d';command=list(argv);command[command.index('-o')+1]=str(obj);command+=['-MD','-MT','storage-reproduction','-MF',str(md)]
    _,cxx_event=run(command,b,env,'CXX');fence()
    md_material,md_raw=h['material'](md);assert h['dependency_set'](md_raw,b)==deps,'actual-CXX-MD-differs-from-independent-M'
    first_observed=None;samples=[];sample_times=[]
    for i in range(3):
        if i:time.sleep(1.02)
        observed,raw=h['material'](obj);assert {'sha256':observed['sha256'],'bytes':len(raw)}==expected,'reproduction-object-not-byte-identical'
        if first_observed is None:first_observed=observed
        assert observed==first_observed,'new-object-first-admission-not-stable';samples.append(observed);sample_times.append(time.monotonic_ns());fence()
    assert sample_times[-1]-sample_times[0]>=2000000000
    private=save('admission.private.json',{'newObject':first_observed,'newObjectSamples':samples,'sampleTimesNs':sample_times,'actualMdMaterial':md_material,'independentDependencies':sorted(deps),'heldOriginalMaterials':h['HELD'],'newAliasLiteral':ALIAS,'newAliasTargetFromOldSdkNamespace':target,'newAliasAndExistingAliasGuards':h['ALIASES'],'newParentAliasGuards':new_alias_guards,'historicalAliasBindingRecovered':False,'oldAuthoritiesRefreshed':False},0o600)
    result={'format':'nano-native-storage-single-object-reproduction-v1','passed':True,'coreArchiveChanged':False,'oldObjectChanged':False,'storageObjectByteIdenticalToRetainedCoreMember':True,'reproducedObject':expected,'canonicalOriginalRecipeReused':True,'newProspectiveAliasLocatorCount':1,'newParentAliasGuardCount':len(new_alias_guards),'historicalAliasBindingRecovered':False,'sdkRegularAuthoritiesRefreshed':False,'otherNewAliasesAccepted':0,'newMEqualsPinnedHistoricalMdDependencySet':True,'newMEqualsActualCxxMd':True,'dependencyCount':len(deps),'actualCompilations':1,'archiveCommands':0,'programLinks':0,'projectTestsExecuted':0,'stableAdmissionSamples':3,'stableAdmissionSpanMs':(sample_times[-1]-sample_times[0])//1000000,'originalSdkRegularFilesFenced':len(sdk['sdkFiles']),'privateReceipt':private,'events':[m_event,cxx_event],'elapsedMs':(time.monotonic_ns()-start)//1000000,'runtimeStarted':False,'productionQualified':False}
    receipt=save('public-result.json',result);print(json.dumps({'result':result,'receipt':receipt}),flush=True)

if __name__=='__main__':
    try:main()
    except BaseException as exc:
        if O.is_dir():
            ref=save('refusal.private.json',{'errorType':type(exc).__name__,'reason':str(exc),'frames':[{'file':Path(t.filename).name,'line':t.lineno,'function':t.name} for t in traceback.extract_tb(exc.__traceback__)],'productionQualified':False},0o600)
            print(json.dumps({'passed':False,'errorType':type(exc).__name__,'privateReceipt':ref,'productionQualified':False,'detailsExported':False}),flush=True)
        raise SystemExit(1)
