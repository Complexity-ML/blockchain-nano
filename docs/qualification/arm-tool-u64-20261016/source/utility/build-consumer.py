#!/usr/bin/env python3
"""ROOT-only direct consumer build from existing canonical CMake recipes.

No configure, core compilation, image build, image pull, or database access.
Private per-host receipts are not publication artifacts.
"""
import argparse, hashlib, json, os, pathlib, shlex, signal, stat, subprocess, sys, time

F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
HELD, ALIASES, NEW, EVENTS = {}, {}, {}, []
START = time.monotonic()

def need(ok, message):
    if not ok: raise ValueError(message)

def material(path):
    p = pathlib.Path(path)
    fd = os.open(p, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        a = os.fstat(fd)
        need(stat.S_ISREG(a.st_mode) and a.st_size <= 268435456, 'bounded regular input required')
        h, chunks = hashlib.sha256(), []
        while True:
            raw = os.read(fd, 1048576)
            if not raw: break
            h.update(raw); chunks.append(raw)
        row = {k:getattr(a,k) for k in F}
        need(row == {k:getattr(os.fstat(fd),k) for k in F} == {k:getattr(p.lstat(),k) for k in F}, 'input changed during FD read')
        return dict(row, sha256=h.hexdigest()), b''.join(chunks)
    finally: os.close(fd)

def hold(path, pin=None):
    p = pathlib.Path(path)
    need(p.is_absolute(), 'absolute input required')
    resolved = p.resolve(strict=True)
    if resolved != p: ALIASES[str(p)] = str(resolved)
    row, raw = material(resolved)
    if pin:
        if 'st_dev' in pin: need(row == pin, 'externally admitted nine-field input differs')
        else: need(row['sha256'] == pin['sha256'] and row['st_size'] == pin['bytes'], 'externally pinned input bytes differ')
    prior = HELD.get(str(resolved))
    need(prior is None or prior == row, 'previously admitted input changed')
    HELD[str(resolved)] = row
    return raw

def fence():
    for path, pin in list(HELD.items()): hold(path, pin)
    for alias, resolved in ALIASES.items(): need(str(pathlib.Path(alias).resolve(strict=True)) == resolved, 'input alias changed')
    for path, pin in list(NEW.items()): output(path,pin)

def output(path,pin=None):
    p=pathlib.Path(path); need(p.is_absolute() and p.resolve()==p,'canonical new output required')
    row,raw=material(p); actual={'sha256':row['sha256'],'bytes':row['st_size']}
    expected=pin or NEW.get(str(p)); need(expected is None or actual==expected,'new output byte commitment changed')
    NEW[str(p)]=actual
    return raw

def publish(path, value):
    raw = (json.dumps(value, sort_keys=True, separators=(',',':'))+'\n').encode()
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    try:
        at=0
        while at<len(raw): at += os.write(fd,raw[at:])
        os.fsync(fd)
    finally: os.close(fd)

def run(name, argv, cwd, out, timeout=300):
    need(time.monotonic()-START < 1100, 'whole controller deadline')
    publish(out/(name+'-argv.private.json'), {'argv':argv,'cwd':str(cwd)})
    began=time.monotonic()
    env={k:v for k,v in os.environ.items() if not k.startswith(('NANO_','DYLD_')) and k not in ('LD_PRELOAD','CPATH','CPLUS_INCLUDE_PATH','C_INCLUDE_PATH','LIBRARY_PATH','CXXFLAGS','LDFLAGS')}
    with (out/(name+'-stdout.private.log')).open('xb') as stdout, (out/(name+'-stderr.private.log')).open('xb') as stderr:
        child=subprocess.Popen(argv,cwd=cwd,env=env,stdout=stdout,stderr=stderr,start_new_session=True)
        expired=False
        try: code=child.wait(timeout=timeout)
        except BaseException:
            expired=True
            try: os.killpg(child.pid,signal.SIGTERM); code=child.wait(timeout=5)
            except (ProcessLookupError,subprocess.TimeoutExpired):
                try: os.killpg(child.pid,signal.SIGKILL)
                except ProcessLookupError: pass
                code=child.wait(timeout=5)
            if not isinstance(sys.exc_info()[1],subprocess.TimeoutExpired): raise
    event={'phase':name,'exitCode':code,'timedOut':expired,'elapsedMs':int((time.monotonic()-began)*1000)}
    EVENTS.append(event); publish(out/(name+'-exit.json'),event)
    print(json.dumps(event),flush=True)
    need(code==0 and not expired,'consumer command failed: '+name)

def deps(raw,cwd):
    raw=raw.decode().replace('\\\n',' ')
    need(':' in raw,'dependency rule missing')
    return {str(pathlib.Path(os.path.normpath(str(pathlib.Path(x) if pathlib.Path(x).is_absolute() else cwd/x)))) for x in shlex.split(raw.split(':',1)[1])}

def main(args):
    config=json.loads(hold(pathlib.Path(args.inputs),{'sha256':args.inputs_sha,'bytes':args.inputs_bytes}))
    need(config['format']=='nano-fresh-rotation-thin-build-root-v1','thin input format')
    need(config['owner'] in ('mac-native','mac-docker','vps-docker'),'owner')
    b,s,out=(pathlib.Path(config[k]) for k in ('buildRoot','sourceRoot','controlRoot'))
    need(all(p.is_absolute() and p.resolve()==p for p in (b,s,out)),'canonical roots')
    need(out.is_dir() and set(x.name for x in out.iterdir())=={'inputs.private.json'},'new control root with only input authority required')
    hold(pathlib.Path(__file__).absolute(),config['controller'])
    source=s/'tests/multihost/rotation_audit.cpp'
    hold(source,config['consumerSource'])
    core=b/'libnano_core.a'; hold(core,config['coreArchive'])
    command_file=b/'compile_commands.json'; link_file=b/'CMakeFiles/nano-capella-rotation-audit.dir/link.txt'
    rows=json.loads(hold(command_file,config['canonicalCompileCommands']))
    candidates=[r for r in rows if r['file']==str(source)]
    need(len(candidates)==1,'unique canonical consumer recipe required')
    recipe=candidates[0]
    need(recipe['directory']==str(b),'canonical build directory mismatch')
    compile_argv=recipe.get('arguments') or shlex.split(recipe['command'])
    need(compile_argv.count('-o')==1 and compile_argv.count('-c')==1 and compile_argv[-1]==str(source),'canonical compile shape')
    compiler=pathlib.Path(compile_argv[0]); hold(compiler)
    original_object=compile_argv[compile_argv.index('-o')+1]
    base=list(compile_argv); at=base.index('-o'); del base[at:at+2]; base.remove('-c')
    link=shlex.split(hold(link_file,config['canonicalLink']).decode())
    need(link[0]==str(compiler) and link.count('-o')==1 and link.count(original_object)==1,'canonical link shape')
    target=b/'nano-capella-rotation-audit'
    need(link[link.index('-o')+1]=='nano-capella-rotation-audit' and not target.exists(),'new consumer output must not overwrite')
    libraries=[]
    for x in link[1:]:
        if x.endswith(('.a','.o','.so','.dylib')) and x!=original_object:
            path=pathlib.Path(x); path=path if path.is_absolute() else b/path
            need(str(path) in config['linkInputPins'],'external link-input authority required')
            hold(path,config['linkInputPins'][str(path)]); libraries.append(str(path))
    need(len([x for x in libraries if x.endswith('.a')])==5 and len([x for x in libraries if x.endswith('.o')])==2,'five archives and two existing asm objects required')
    sdk=None
    if config['owner']=='mac-native':
        a=config['nativeSdkAuthority']; sdk=json.loads(hold(pathlib.Path(a['path']),a))
        need(sdk['existingPinsNineFieldsAndBytesVerified'] and not sdk['newSdkInputsAutomaticallyRefreshed'],'old SDK admission required')
        for path,pin in sdk['sdkFiles'].items(): hold(pathlib.Path(path),pin)
        for alias,resolved in sdk['sdkAliases'].items():
            need(str(pathlib.Path(alias).resolve(strict=True))==resolved,'old SDK alias changed'); ALIASES[alias]=resolved
    # Only actual compiler-discovered dependencies are admitted. -M is not a compilation.
    run('dependency-scan',base+['-M','-MT','fresh-rotation-consumer'],b,out)
    scan=out/'dependency-scan-stdout.private.log'; dependency_set=deps(output(scan),b)
    need(source.as_posix() in dependency_set and 20<len(dependency_set)<5000,'real dependency closure required')
    project=0; external=0
    project_pins=config['projectFiles']
    for name in sorted(dependency_set):
        p=pathlib.Path(name)
        if p.is_relative_to(s):
            rel=str(p.relative_to(s)); need(rel in project_pins,'project dependency absent from admitted source authority')
            hold(p,project_pins[rel]); project+=1
        else:
            if sdk: need(str(p.resolve(strict=True)) in sdk['sdkFiles'],'new SDK input requires explicit ROOT authority before CXX')
            hold(p); external+=1
    fence(); publish(out/'admitted.private.json',{'files':HELD,'aliases':ALIASES,'dependencies':sorted(dependency_set)})
    obj=out/'rotation_audit.cpp.o'; md=out/'rotation_audit.cpp.o.d'
    compile_new=list(compile_argv); compile_new[compile_new.index('-o')+1]=str(obj)
    compile_new+=['-MD','-MT','fresh-rotation-consumer','-MF',str(md)]
    run('compile-consumer',compile_new,b,out)
    need(deps(output(md),b)==dependency_set,'actual compiler MD differs from preflight -M')
    output(obj); fence()
    link_new=[str(obj) if x==original_object else x for x in link]
    run('link-consumer',link_new,b,out)
    binary=output(target)
    run('fresh-self-test',[str(target),'--fresh-self-test'],b,out,30)
    # Dispatch changed; the six tiny legacy signature checks protect that shared entry point.
    run('legacy-self-test',[str(target),'--self-test'],b,out,30)
    self_result=json.loads(output(out/'fresh-self-test-stdout.private.log'))
    need(self_result['passed'] is True and self_result['positiveOwnershipChecks']==2 and self_result['positiveCryptographicChecks']==1 and self_result['positiveWeightChecks']==3 and self_result['expectedRefusals']==12 and self_result['fixtureBlsSignatures']==2 and self_result['auditSignatures']==0 and not self_result['databaseAccessed'] and not self_result['runtimeStarted'],'fresh self-test typed result differs')
    need(output(out/'legacy-self-test-stdout.private.log').decode().strip()=='6 empty-aggregate checks passed; no network qualification','legacy signature checks differ')
    fence()
    result={'format':'nano-fresh-rotation-thin-build-terminal-v1','passed':True,'owner':config['owner'],'sourceSha256':config['consumerSource']['sha256'],'binary':{'sha256':hashlib.sha256(binary).hexdigest(),'bytes':len(binary)},'coreArchive':config['coreArchive'],'coreArchiveUnchanged':True,'consumerCompilations':1,'consumerLinks':1,'coreCompilations':0,'configureCommands':0,'imageBuilds':0,'imagePulls':0,'canonicalCmakeRecipeProvenance':True,'genuineMEqualsActualMD':True,'projectDependencies':project,'externalDependencies':external,'existingAsmObjects':2,'archiveInputs':5,'selfTest':self_result,'legacySignatureChecks':6,'events':EVENTS,'productionQualified':False}
    publish(out/'terminal.json',result); print(json.dumps(result),flush=True)

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--inputs',required=True); p.add_argument('--inputs-sha',required=True); p.add_argument('--inputs-bytes',type=int,required=True); args=p.parse_args()
    try: main(args)
    except Exception as error:
        print(json.dumps({'passed':False,'errorType':type(error).__name__,'reason':str(error),'events':EVENTS}),flush=True); raise
