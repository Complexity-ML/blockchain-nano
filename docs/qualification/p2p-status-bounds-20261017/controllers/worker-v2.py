#!/usr/bin/env python3
"""Root worker: targeted P2P builds, no node service or dependency build."""
import hashlib,json,os,pathlib,platform,shlex,stat,subprocess,sys,time,traceback

CONTROL=pathlib.Path(sys.argv[1])
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def material(path):
    p=pathlib.Path(path); fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        a=os.fstat(fd); assert stat.S_ISREG(a.st_mode) and a.st_size<=536870912
        h=hashlib.sha256(); n=0
        while b:=os.read(fd,1048576): h.update(b); n+=len(b)
        row={k:getattr(a,k) for k in F}
        assert n==a.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(p.lstat(),k) for k in F},str(p)
        return dict(row,sha256=h.hexdigest())
    finally: os.close(fd)
def save(p,value):
    with pathlib.Path(p).open('x') as f: json.dump(value,f,sort_keys=True,indent=2);f.write('\n')
def main():
    raw=CONTROL.read_bytes();assert hashlib.sha256(raw).hexdigest()==sys.argv[2]
    c=json.loads(raw); out=pathlib.Path(c['output']);out.mkdir(mode=0o700)
    source=pathlib.Path(c['source']); held={}
    for rel,pin in c['sourceFiles'].items():
        p=source/rel;r=material(p);assert (r['sha256'],r['st_size'])==(pin['sha256'],pin['bytes']),str(p);held[str(p)]=r
    env={'PATH':c.get('path','/usr/local/bin:/usr/bin:/bin'),'LANG':'C','LC_ALL':'C','CCACHE_DISABLE':'1','HOME':str(out),'TMPDIR':str(out),'CMAKE_POLICY_VERSION_MINIMUM':'3.5'}
    env.update(c.get('environment',{}))
    logs=[]
    def run(label,args,expect=0,timeout=300):
        save(out/(label+'.argv.json'),args);start=time.monotonic()
        with (out/(label+'.stdout.log')).open('xb') as o,(out/(label+'.stderr.log')).open('xb') as e:
            p=subprocess.run(args,env=env,stdout=o,stderr=e,timeout=timeout)
        row={'phase':label,'exitCode':p.returncode,'elapsedMs':int((time.monotonic()-start)*1000)};logs.append(row);save(out/(label+'.exit.json'),row);print(json.dumps(row),flush=True)
        assert p.returncode==expect,(label,p.returncode,expect)
    mode=c['mode'];build=out/'build';cmake=c['cmake']
    args=[cmake,'-S',str(source/'network'),'-B',str(build),'-DNANO_DEPS_DIR='+c['deps'],'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON']+c.get('cmakeArgs',[])
    run('configure',args)
    targets=['nano-peer-status-tests','nano-json-stream-tests']
    if mode=='candidate':targets.insert(0,'nano-p2p')
    db=json.loads((build/'compile_commands.json').read_bytes()); recipes={}
    scans={}
    for target in targets:
        rows=[r for r in db if 'CMakeFiles/'+target+'.dir/' in r['command']];assert len(rows)==1,target
        r=rows[0];argv=shlex.split(r['command']);obj=argv[argv.index('-o')+1];scan=out/(target+'.M.d')
        dependency=[a for i,a in enumerate(argv) if i not in (argv.index('-o'),argv.index('-o')+1) and a!='-c']+['-M','-MF',str(scan),'-MT',obj]
        # Relative object paths are not used by the dependency scan.
        run('scan-'+target,dependency)
        def deps(p):
            _,sep,body=p.read_text().replace('\\\n',' ').partition(':');assert sep
            return sorted({str(pathlib.Path(x).resolve(strict=True)) for x in shlex.split(body)})
        scans[target]=deps(scan)
        for dep in scans[target]:
            r0=material(dep);assert dep not in held or held[dep]==r0,dep;held[dep]=r0
        recipes[target]={'compile':r,'dependencies':scans[target]}
        link=build/'CMakeFiles'/(target+'.dir')/'link.txt'
        for word in shlex.split(link.read_text()):
            if word.endswith(('.a','.dylib','.so')) and pathlib.Path(word).is_absolute():
                p=pathlib.Path(word).resolve(strict=True);observed=material(p)
                assert str(p) not in held or held[str(p)]==observed,('repeated link input changed',str(p))
                held[str(p)]=observed
        held[str(link)]=material(link)
    save(out/'inputs-before.private.json',{'files':held,'recipes':recipes,'sdkAuthority':c['sdkAuthority'],'sourceRevision':c['sourceRevision']})
    run('build',[cmake,'--build',str(build),'--target',*targets,'--parallel','2','--verbose'],timeout=600)
    products={}
    for target in targets:
        recipe=recipes[target]['compile'];argv=shlex.split(recipe['command']);obj=argv[argv.index('-o')+1]
        md=pathlib.Path(obj+'.d');md=md if md.is_absolute() else build/md
        assert deps(md)==scans[target],('M/MD differs',target)
        products[target]=material(build/target)
    if mode=='baseline':
        for target,expected in [('nano-peer-status-tests','malformed best_hash accepted'),('nano-json-stream-tests','oversized status accepted or stream not reset')]:
            run('causal-'+target,[str(build/target),'--status-bounds-self-test'],expect=1,timeout=15)
            assert expected in (out/('causal-'+target+'.stderr.log')).read_text(),target
    else:
        for target in targets:
            if target=='nano-p2p':
                run('usage-nano-p2p',[str(build/target),'--help'],expect=1,timeout=15)
                assert 'Usage: nano-p2p ' in (out/'usage-nano-p2p.stderr.log').read_text()
            else:run('test-'+target,[str(build/target)],timeout=15)
    for path,row in held.items():assert material(path)==row,('input changed',path)
    for target,row in products.items():assert material(build/target)==row,('output changed',target)
    save(out/'products.private.json',products)
    public={'format':'nano-p2p-status-targeted-build-v1','passed':True,'mode':mode,'sourceRevision':c['sourceRevision'],'system':platform.system(),'architecture':platform.machine(),'phases':logs,'sdkAuthority':c['sdkAuthority'],'sourceAndSelectedDependencyInputsUnchanged':True,'actualMmdMatchesM':True,'dependencyFiles':len(held),'products':{k:{'sha256':v['sha256'],'bytes':v['st_size']}for k,v in products.items()},'imagesBuilt':0,'dependencyBuilds':0,'nodeServicesStarted':0,'dutiesResumed':False,'networkQualified':False,'productionQualified':False}
    save(out/'result-public.json',public);print(json.dumps(public,sort_keys=True),flush=True)
if __name__=='__main__':
    try:main()
    except BaseException:
        traceback.print_exc();sys.exit(1)
