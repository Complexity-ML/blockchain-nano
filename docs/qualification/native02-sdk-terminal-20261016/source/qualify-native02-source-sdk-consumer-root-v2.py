"""ROOT-only: one network consumer build after the existing source SDK terminates.

No image/Core build, historical SDK access, identity argument or node listener.
Preserve all old authorities. Only prospective new outputs get a first admission.
"""
from pathlib import Path, PurePosixPath
import hashlib, io, json, os, shlex, signal, stat, subprocess, tarfile, time, uuid

R=Path('/Users/boris/Dev/blockchain-nano')
B=R/'.deps/p2p-source-20261016-native-02'
OUT=B/'source-sdk-consumer-qualification-v1'
SOURCE=OUT/'source'
REV='5fc36d9a1e2bd2bd1c61201064c1f85a0498996f'
NINE=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
held={}
steps=[]
out_created=False
held_aliases=[]

def need(value,message):
    if not value: raise RuntimeError(message)

def row(path):
    path=Path(path)
    need(path.resolve(strict=True)==path,'physical input path')
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW)
    try:
        s=os.fstat(fd); before={key:getattr(s,key) for key in NINE}
        need(stat.S_ISREG(s.st_mode) and s.st_nlink>=1,'ordinary input')
        h=hashlib.sha256()
        while chunk:=os.read(fd,1048576): h.update(chunk)
        need(before=={k:getattr(os.fstat(fd),k) for k in NINE}=={k:getattr(path.lstat(),k) for k in NINE},'FD/path metadata changed')
        return {**before,'sha256':h.hexdigest()}
    finally: os.close(fd)

def new(path,raw,mode=0o400):
    path=Path(path); fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,mode)
    try:
        view=memoryview(raw)
        while view: view=view[os.write(fd,view):]
        os.fsync(fd)
    finally: os.close(fd)
    need(path.read_bytes()==raw,'new output readback')

def enc(value): return (json.dumps(value,sort_keys=True,separators=(',',':'))+'\n').encode()

def pin(path,expected=None):
    path=Path(path); value=row(path)
    if expected is not None: need(value==expected,'original authority mismatch')
    if str(path) in held: need(value==held[str(path)],'held input mismatch; never refresh')
    else: held[str(path)]=value
    return value

def stable(paths):
    start=time.monotonic()
    for i in range(3):
        if i: time.sleep(1.05)
        for path in paths: need(row(path)==held[str(path)],'first input admission changed')
    elapsed=round((time.monotonic()-start)*1000)
    need(elapsed>=2000,'first admission observation span')
    return {'samples':3,'spanMs':elapsed}

def git(*args):
    return subprocess.check_output(['git','-C',str(R),*args],timeout=60)

def check_aliases():
    for value in held_aliases:
        path=Path(value['logical'])
        need(path.is_symlink() and str(path.resolve(strict=True))==value['resolved'] and {k:getattr(path.lstat(),k) for k in NINE}==value['material'],'original Python alias changed')

def group_exists(pid):
    try: os.killpg(pid,0); return True
    except ProcessLookupError: return False

def cleanup_owned_group(child):
    # The group was created by this invocation; never target a shared parent PG.
    if group_exists(child.pid):
        try: os.killpg(child.pid,signal.SIGTERM)
        except ProcessLookupError: pass
        deadline=time.monotonic()+5
        while group_exists(child.pid) and time.monotonic()<deadline: time.sleep(.05)
        if group_exists(child.pid):
            try: os.killpg(child.pid,signal.SIGKILL)
            except ProcessLookupError: pass
    try: child.wait(timeout=3)
    except subprocess.TimeoutExpired: raise RuntimeError('owned leader did not join after cleanup')
    deadline=time.monotonic()+2
    while group_exists(child.pid) and time.monotonic()<deadline: time.sleep(.05)
    need(not group_exists(child.pid),'owned process group remains after cleanup')

def run(label,args,env,timeout=900):
    check_aliases()
    start=time.monotonic()
    with (OUT/'logs'/f'{label}.log').open('xb') as log:
        child=subprocess.Popen(args,cwd=OUT,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        try: child.wait(timeout=timeout)
        except BaseException:
            cleanup_owned_group(child)
            raise
        if group_exists(child.pid):
            cleanup_owned_group(child)
            raise RuntimeError('owned descendants remained after leader completion')
        log.flush(); os.fsync(log.fileno())
    result={'step':label,'exitCode':child.returncode,'elapsedMs':round((time.monotonic()-start)*1000)}
    steps.append(result); print(json.dumps(result),flush=True)
    return child.returncode

def main():
    global out_created
    # Never replace or restart the source build. Its existing terminal is mandatory.
    terminal=B/'source-build-terminal-public.json'
    terminal_row=pin(terminal)
    terminal_value=json.loads(terminal.read_bytes())
    need(terminal_value['format']=='nano-p2p-source-build-terminal-v1' and terminal_value['exitCode']==0 and terminal_value['passed'] is True and terminal_value['sourceOnlyRequested'] is True,'source SDK has no successful terminal')
    unresolved=[p for p in (B/'calls').iterdir() if not (p/'terminal.json').is_file() and not (p/'refusal.json').is_file()]
    need(not unresolved,'source SDK compiler call still incomplete')
    repository_head=git('rev-parse','HEAD').decode().strip()
    need(not git('status','--porcelain','--untracked-files=all'),'clean source repository')
    need(subprocess.call(['git','-C',str(R),'merge-base','--is-ancestor',REV,repository_head])==0,'network source revision must remain an ancestor')
    candidate_entries=git('ls-tree','-z',REV,'--','network','third_party','scripts/build-libp2p.sh')
    current_entries=git('ls-tree','-z',repository_head,'--','network','third_party','scripts/build-libp2p.sh')
    need(candidate_entries.count(b'\0')==3 and candidate_entries==current_entries,'network/SDK source tree entries changed since reviewed candidate')
    descriptor_admission=Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012/native02-consumer-controller-input-admission.private.json')
    admission_row=pin(descriptor_admission)
    need(admission_row['sha256']=='ab803de588e74f7854435f02f9ea668d92bf305661d05e86aee23717e557a6c1' and admission_row['st_size']==961,'prospective descriptor-file authority')
    descriptor_authorities=json.loads(descriptor_admission.read_bytes())['files']
    for path,expected in descriptor_authorities.items(): pin(path,expected)
    prep_path=B/'prepared-inputs.private.json'
    prep=json.loads(prep_path.read_bytes())
    # Installed BoringSSL files already have an authority from the real join.
    # Reuse it exactly; this consumer must never refresh those input identities.
    bssl_descriptor=Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012/native02-boringssl-installed-member-join-root-20261016-attempt02/member-join.private.json')
    descriptor_row=pin(bssl_descriptor)
    need(descriptor_row['sha256']=='052f03a4d06fdd7c56df27810342cb9929e2b56bb6f6515af569c3abb04ec65c' and descriptor_row['st_size']==10660646,'original BoringSSL join descriptor')
    bssl=json.loads(bssl_descriptor.read_bytes())
    bssl_prefix=B/'deps/hunter/_Base/15ca502/b63ef16/00f4bd3/Install/lib'
    for name,value in bssl['archives'].items(): pin(bssl_prefix/name,value['material'])
    for path,expected in prep['hostToolMaterials']['files'].items(): pin(path,expected)
    admission=json.loads((B/'producer-control-admission.private.json').read_bytes())
    for path,expected in admission['files'].items(): pin(path,expected)
    python_alias=admission['pythonAlias']
    held_aliases.append(python_alias); check_aliases()
    need(not OUT.exists(),'new consumer namespace only')
    OUT.mkdir(mode=0o700); out_created=True; SOURCE.mkdir(mode=0o700)
    for name in ('calls','logs'): (OUT/name).mkdir(mode=0o700)
    source_archive=git('archive','--format=tar',REV,'network','third_party')
    source_rows={}
    with tarfile.open(fileobj=io.BytesIO(source_archive),mode='r:') as archive:
        for member in archive:
            path=PurePosixPath(member.name)
            need(not path.is_absolute() and '..' not in path.parts and path.parts[0] in ('network','third_party'),'bounded committed source path')
            dst=SOURCE/str(path)
            if member.isdir(): dst.mkdir(parents=True,exist_ok=True); continue
            need(member.isfile(),'ordinary committed source only')
            dst.parent.mkdir(parents=True,exist_ok=True)
            body=archive.extractfile(member).read(); new(dst,body)
            source_rows[str(dst)]=pin(dst)
    cmake_source=SOURCE/'network/CMakeLists.txt'
    need(held[str(cmake_source)]['sha256']=='95fc4952f691506f4d5874318c1a7018c70a6ae65243202c9864b5a11ea46130','reviewed network provider fix')
    control=json.loads((B/'compiler-control.private.json').read_bytes())
    control['runId']=uuid.uuid4().hex; control['callRoot']=str(OUT/'calls')
    raw=enc(control); control_path=OUT/'compiler-control.private.json'; new(control_path,raw); pin(control_path)
    env=json.loads((B/'env-outer.private.json').read_bytes())['env']
    env.update(NANO_P2P_COMPILER_CONTROL=str(control_path),NANO_P2P_COMPILER_CONTROL_SHA256=hashlib.sha256(raw).hexdigest(),NANO_P2P_COMPILER_CONTROL_BYTES=str(len(raw)))
    need('NANO_SUPERVISOR_PID' not in env and 'NANO_NETWORK_TOKEN' not in env and not any(key.startswith('DYLD_') for key in env),'sanitized consumer environment')
    source_observations=stable(list(held))
    cmake=env['CMAKE_BIN']; build=OUT/'arm64'
    configure=[cmake,'-S',str(SOURCE/'network'),'-B',str(build),'-DNANO_DEPS_DIR='+str(B/'deps'),'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_OSX_ARCHITECTURES=arm64','-DCMAKE_C_COMPILER='+env['CC'],'-DCMAKE_CXX_COMPILER='+env['CXX'],'-DCMAKE_MAKE_PROGRAM=/opt/homebrew/Cellar/make/4.4.1/bin/gmake','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON']
    need(run('configure-arm64-new-sdk',configure,env)==0,'new SDK configure failed')
    recipe=build/'CMakeFiles/nano-p2p.dir/link.txt'; pin(recipe)
    arguments=shlex.split(recipe.read_text())
    ordered_archives=[Path(x) for x in arguments if x.endswith('.a')]
    need(ordered_archives and all(p.is_absolute() and p.resolve(strict=True).is_relative_to(B/'deps') for p in ordered_archives),'archive link leaves new source SDK')
    archives={str(p.resolve(strict=True)):pin(p.resolve(strict=True)) for p in ordered_archives}
    archive_observations=stable(list(archives)+[str(recipe)])
    new(OUT/'inputs-before-build.private.json',enc({'files':held,'sourceArchiveSha256':hashlib.sha256(source_archive).hexdigest(),'sourceArchiveBytes':len(source_archive),'sourceFiles':len(source_rows),'sourceMetadata':source_observations,'archiveMetadata':archive_observations,'orderedArchivePaths':[str(p) for p in ordered_archives],'sourceRevision':REV,'oldAuthoritiesRefreshed':False}))
    before_descriptor=pin(OUT/'inputs-before-build.private.json')
    stable([str(OUT/'inputs-before-build.private.json')])
    need(run('build-nano-p2p-new-sdk',[cmake,'--build',str(build),'--target','nano-p2p','--parallel','1'],env)==0,'new SDK network consumer build failed')
    compiler_terminals=[]
    for directory in (OUT/'calls').iterdir():
        path=directory/'terminal.json'
        if not path.is_file(): continue
        value=json.loads(path.read_bytes()); output=value.get('producedOutput')
        if value.get('kind')=='compile' and value['exitCode']==0 and output and output['resolvedPath']==str(build/'CMakeFiles/nano-p2p.dir/main.cpp.o'):
            compiler_terminals.append((directory,value))
    need(len(compiler_terminals)==1 and compiler_terminals[0][1]['independentMEqualsActualMd'] is True,'one genuine new consumer main M/MD')
    directory,compile_terminal=compiler_terminals[0]
    helper=Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012/fresh-p2p-compiler-receipt-index-source-20261016/compiler_receipt_index.py')
    helper_row=pin(helper,bssl['receiptInputs'][str(helper)])
    need(helper_row['sha256']=='2f47b78018a16a69bb9cd4cf392a661fc21394d99e816e6b1f5ac8160ea2e8a7' and helper_row['st_size']==12709,'reviewed pure receipt index')
    namespace={'__name__':'root_owned_pure_receipt_index'}
    exec(compile(helper.read_bytes(),str(helper),'exec'),namespace)
    bundle={'id':directory.name}
    for field,name in {'command':'command.json','executed':'executed-command.json','dependenciesBefore':'dependencies-before.json','terminal':'terminal.json','refusal':'refusal.json'}.items():
        receipt=directory/name
        if receipt.is_file(): pin(receipt); bundle[field]=json.loads(receipt.read_bytes())
        else: bundle[field]=None
    indexed=namespace['index_compiler_receipts']([bundle])
    compile_terminal=bundle['terminal']
    need(len(indexed['categories']['compileMEqualsMd'])==1 and compile_terminal['controlSha256']==hashlib.sha256(raw).hexdigest(),'complete actual main receipt and control join')
    for field in ('producedOutput','actualDependencyFile'):
        value=compile_terminal[field]
        pin(value['resolvedPath'],{**value['material'],'sha256':value['sha256']})
    stable([str(directory/name) for name in ('command.json','executed-command.json','dependencies-before.json','terminal.json')])
    deps=json.loads((directory/'dependencies-before.json').read_bytes())['files']
    boost_deps={p:r for p,r in deps.items() if '/include/boost/' in p or '/include/boost/' in r['resolvedPath']}
    boost_archives=[p for p in ordered_archives if p.name.startswith('libboost_')]
    need(len(boost_archives)>=2,'actual linked Boost archives')
    prefixes={p.parent.parent.resolve(strict=True) for p in boost_archives}
    need(len(prefixes)==1,'one actual Hunter Boost prefix')
    prefix=next(iter(prefixes))
    need(prefix.is_relative_to(B/'deps/hunter') and all('-a64' in p.name for p in boost_archives),'Hunter Boost arm64 archives')
    need(boost_deps and all(Path(p).is_relative_to(prefix) and Path(r['resolvedPath']).is_relative_to(prefix) for p,r in boost_deps.items()),'actual Boost headers and libraries share prefix')
    version=prefix/'include/boost/version.hpp'
    need(str(version) in deps and row(version)['sha256']==deps[str(version)]['sha256'] and '#define BOOST_VERSION 108700' in version.read_text(),'actual selected Boost1.87 header')
    executable=build/'nano-p2p'
    executable_row=pin(executable); executable_observations=stable([str(executable)])
    # Source main(argc<4) exits before identity(), injector, listeners and Network.
    need(run('argument-refusal-no-identity',[str(executable)],env,timeout=10)==1,'expected argument refusal did not occur')
    output=(OUT/'logs/argument-refusal-no-identity.log').read_text()
    need(output=='Usage: nano-p2p <core-rpc-port> <identity-file> <listen-multiaddress> [peer-multiaddress ...]\n','exact usage refusal')
    for path,expected in held.items(): need(row(path)==expected,'input post-fence mismatch')
    check_aliases()
    need(git('rev-parse','HEAD').decode().strip()==repository_head and not git('status','--porcelain','--untracked-files=all'),'source repository changed during qualification')
    new(OUT/'inputs-post-fence.private.json',enc({'files':held,'allHeldFullNineAndShaUnchanged':True,'oldAuthoritiesRefreshed':False}))
    after_descriptor=pin(OUT/'inputs-post-fence.private.json')
    stable([str(OUT/'inputs-post-fence.private.json')])
    for path,expected in held.items():need(row(path)==expected,'final input post-fence mismatch')
    check_aliases()
    result={'format':'nano-native02-source-sdk-consumer-functional-public-v1','passed':True,'sourceRevision':REV,'repositoryHeadDuringQualification':repository_head,'privateInputsBeforePin':{'sha256':before_descriptor['sha256'],'bytes':before_descriptor['st_size']},'privateInputsAfterPin':{'sha256':after_descriptor['sha256'],'bytes':after_descriptor['st_size']},'sourceSdkTerminalPin':{'sha256':terminal_row['sha256'],'bytes':terminal_row['st_size']},'steps':steps,'orderedArchiveOccurrences':len(ordered_archives),'distinctArchiveFiles':len(archives),'allArchivesInNewSourceSdk':True,'actualBoostArchives':[{'name':p.name,'sha256':archives[str(p.resolve())]['sha256']} for p in boost_archives],'actualBoostHeaderDependencies':len(boost_deps),'genuineMainMEqualsMd':True,'executablePin':{'sha256':executable_row['sha256'],'bytes':executable_row['st_size']},'argumentRefusalExecuted':True,'identityArgumentProvided':False,'nanoNodeStarted':False,'dutiesResumed':False,'campaignAdmission':False,'oldAuthoritiesRefreshed':False,'wholeProducerClosureQualified':False,'networkQualified':False,'productionQualified':False}
    result_bytes=enc(result)
    new(OUT/'result-public.json',result_bytes)
    print(json.dumps({'result':result,'publicResultPin':{'sha256':hashlib.sha256(result_bytes).hexdigest(),'bytes':len(result_bytes)}}),flush=True)

if __name__=='__main__':
    def interrupted(signum,frame): raise InterruptedError('controller signal '+str(signum))
    signal.signal(signal.SIGTERM,interrupted)
    signal.signal(signal.SIGINT,interrupted)
    try: main()
    except BaseException as error:
        if out_created:
            mismatches=[]
            for path,expected in held.items():
                try:
                    if row(path)!=expected: mismatches.append(path)
                except BaseException: mismatches.append(path)
            alias_mismatch=False
            try: check_aliases()
            except BaseException: alias_mismatch=True
            new(OUT/'failure.private.json',enc({'errorType':type(error).__name__,'reason':str(error),'steps':steps,'heldInputs':held,'postFenceMismatchPaths':mismatches,'oldAuthoritiesRefreshed':False}))
            new(OUT/'failure-public.json',enc({'passed':False,'errorType':type(error).__name__,'reason':str(error),'steps':steps,'heldInputPostFenceMismatches':len(mismatches),'aliasPostFenceMismatch':alias_mismatch,'oldAuthoritiesRefreshed':False,'productionQualified':False}))
        raise
