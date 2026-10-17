#!/usr/bin/env python3
"""Host Python orchestrates one Root-owned SDK container; never creates/removes it.

CLI: worker.py CONTROL_JSON EXPECTED_SHA256. Host and guest source/output paths
are identical shared mounts. SDK/source/controls are mounted read-only by Root.
Guest observations are realpath + GNU stat/SHA/stat, NOT FD identities.
"""
import hashlib,json,os,pathlib,platform,posixpath,re,shlex,stat,subprocess,sys,time,traceback

F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
MAX_FILE=536870912

def material(path,limit=MAX_FILE):
    p=pathlib.Path(path);assert p.is_absolute() and p.resolve(strict=True)==p,str(p)
    fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        a=os.fstat(fd);assert stat.S_ISREG(a.st_mode) and 0<=a.st_size<=limit
        h=hashlib.sha256();n=0;parts=[]
        while b:=os.read(fd,1048576):
            n+=len(b);assert n<=a.st_size;h.update(b)
            if limit<=4194304:parts.append(b)
        row={k:getattr(a,k) for k in F}
        assert n==a.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(p.lstat(),k) for k in F},str(p)
        return dict(row,sha256=h.hexdigest()),b''.join(parts) if limit<=4194304 else None
    finally:os.close(fd)

def save(path,value):
    with pathlib.Path(path).open('x') as f:
        json.dump(value,f,sort_keys=True,indent=2);f.write('\n');f.flush();os.fsync(f.fileno())

# Fixed, positional-argument-only program. No input is interpolated as shell code.
# No path contents are returned; names, exact GNU strings and SHA only.
GUEST_OBSERVE=r'''set -eu
export LC_ALL=C
for p do
 r=$(/usr/bin/realpath -e -- "$p")
 [ -f "$r" ] && [ ! -L "$r" ]
 printf '%s\0' "$r"
 /usr/bin/stat --printf='%d\0%i\0%f\0%h\0%u\0%g\0%s\0%y\0%z\0' -- "$r"
 h=$(/usr/bin/sha256sum < "$r")
 printf '%s\0' "$h"
 /usr/bin/stat --printf='%d\0%i\0%f\0%h\0%u\0%g\0%s\0%y\0%z\0' -- "$r"
 r2=$(/usr/bin/realpath -e -- "$p")
 [ "$r" = "$r2" ]
done
'''

def main():
    assert len(sys.argv)==3
    control=pathlib.Path(sys.argv[1]);cr,raw=material(control,4194304)
    assert re.fullmatch('[0-9a-f]{64}',sys.argv[2]) and cr['sha256']==sys.argv[2]
    c=json.loads(raw);out=pathlib.Path(c['output']);source=pathlib.Path(c['source'])
    assert source.is_absolute() and out.is_absolute() and source.resolve(strict=True)==source and not out.exists()
    assert not out.is_relative_to(source) and not source.is_relative_to(out)
    prefix=c['dockerPrefix'];assert isinstance(prefix,list) and 3<=len(prefix)<=32 and all(isinstance(x,str) and x and '\0' not in x for x in prefix)
    assert 'exec' in prefix[1:-1] and re.fullmatch('[A-Za-z0-9][A-Za-z0-9_.-]{0,127}',prefix[-1]) and pathlib.Path(prefix[0]).is_absolute()
    mode=c['mode'];assert mode in ('baseline','candidate')
    os.umask(0o077);out.mkdir(mode=0o700)
    host={str(control):cr};guest={};aliases={};products={};logs=[];serial=0;first=None
    for p in [pathlib.Path(__file__).absolute(),pathlib.Path(prefix[0])]:host[str(p)]=material(p)[0]
    started=time.monotonic();deadline=started+1100
    env={'PATH':c.get('path','/usr/local/bin:/usr/bin:/bin'),'LANG':'C','LC_ALL':'C','CCACHE_DISABLE':'1','HOME':str(out),'TMPDIR':str(out),'CMAKE_POLICY_VERSION_MINIMUM':'3.5'}
    env.update(c.get('environment',{}));assert all(re.fullmatch('[A-Za-z_][A-Za-z0-9_]*',k) and isinstance(v,str) and '\0' not in v for k,v in env.items())

    def remember(table,path,row):
        assert path not in table or table[path]==row,('repeated input changed',path)
        table.setdefault(path,row)

    def run(label,args,expect=0,timeout=300,cwd=None):
        assert re.fullmatch('[A-Za-z0-9_.-]+',label)
        budget=min(timeout,deadline-time.monotonic());assert budget>0,'worker deadline'
        command=prefix[:-1]+['--workdir',str(cwd or out)]
        for k,v in sorted(env.items()):command+=['--env',k+'='+v]
        command += [prefix[-1],*map(str,args)]
        save(out/(label+'.argv.private.json'),command);begin=time.monotonic();event={'phase':label};error=None
        try:
            with (out/(label+'.stdout.log')).open('xb') as o,(out/(label+'.stderr.log')).open('xb') as e:
                q=subprocess.run(command,env={'PATH':'/usr/bin:/bin','LANG':'C','LC_ALL':'C'},stdout=o,stderr=e,timeout=budget)
            event['exitCode']=q.returncode
        except BaseException as ex:
            error=ex;event.update(errorType=type(ex).__name__,exitCode=None)
        event['elapsedMs']=int((time.monotonic()-begin)*1000);logs.append(event)
        try:save(out/(label+'.exit.json'),event);print(json.dumps(event),flush=True)
        except BaseException:
            if error is None:raise
        if error is not None:raise error
        assert event['exitCode']==expect,(label,event['exitCode'],expect)

    def observe(paths,retain=True):
        nonlocal serial
        names=list(paths);assert len(names)<=20000
        result={}
        for at in range(0,len(names),128):
            batch=names[at:at+128]
            assert all(isinstance(p,str) and p.startswith('/') and not any(x in p for x in ('\0','\n','\r')) for p in batch)
            serial+=1;label='observe-'+str(serial)
            run(label,['/bin/sh','-c',GUEST_OBSERVE,'root-selected-files',*batch],timeout=90)
            data=(out/(label+'.stdout.log')).read_bytes();assert len(data)<=1048576 and data.endswith(b'\0')
            fields=data[:-1].decode('utf-8',errors='strict').split('\0');assert len(fields)==20*len(batch)
            for index,literal in enumerate(batch):
                row=fields[index*20:(index+1)*20];resolved,before,hashline,after=row[0],row[1:10],row[10],row[11:20]
                assert resolved.startswith('/') and before==after and re.fullmatch('[0-9a-f]{64}  -',hashline)
                assert all(re.fullmatch('[0-9]+',before[i]) for i in (0,1,3,4,5,6)) and re.fullmatch('[0-9a-f]+',before[2])
                assert stat.S_ISREG(int(before[2],16)) and 0<=int(before[6])<=MAX_FILE
                observed={'materialGNU9':before,'sha256':hashline[:64],'bytes':int(before[6])}
                remember(result,resolved,observed)
                if retain:
                    remember(aliases,literal,resolved);remember(guest,resolved,observed)
        return result

    def deps(path,cwd):
        body=path.read_text();assert len(body)<=4194304
        _,sep,body=body.replace('\\\n',' ').partition(':');assert sep
        literal=sorted({posixpath.normpath(posixpath.join(cwd,x)) for x in shlex.split(body)})
        return sorted(observe(literal))

    try:
        assert isinstance(c['sourceFiles'],dict) and 0<len(c['sourceFiles'])<=20000
        for rel,pin in c['sourceFiles'].items():
            relpath=pathlib.PurePosixPath(rel);assert not relpath.is_absolute() and '..' not in relpath.parts
            p=source/rel;r,_=material(p);assert (r['sha256'],r['st_size'])==(pin['sha256'],pin['bytes']),str(p);remember(host,str(p),r)
        # Join exact host bytes to their separate read-only guest observations.
        selected=observe([str(source/rel) for rel in c['sourceFiles']])
        for rel,pin in c['sourceFiles'].items():
            r=selected[aliases[str(source/rel)]];assert (r['sha256'],r['bytes'])==(pin['sha256'],pin['bytes'])
        build=out/'build';cmake=c['cmake']
        run('configure',[cmake,'-S',str(source/'network'),'-B',str(build),'-DNANO_DEPS_DIR='+c['deps'],'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',*c.get('cmakeArgs',[])])
        dbfile=build/'compile_commands.json';assert dbfile.stat().st_size<=4194304
        db=json.loads(dbfile.read_bytes());targets=['nano-peer-status-tests','nano-json-stream-tests']
        if mode=='candidate':targets.insert(0,'nano-p2p')
        recipes={};scans={}
        for target in targets:
            rows=[r for r in db if 'CMakeFiles/'+target+'.dir/' in r['command']];assert len(rows)==1,target
            recipe=rows[0];argv=shlex.split(recipe['command']);obj=argv[argv.index('-o')+1];cwd=recipe['directory'];assert posixpath.isabs(cwd)
            scan=out/(target+'.M.d');remove={argv.index('-o'),argv.index('-o')+1}
            command=[a for i,a in enumerate(argv) if i not in remove and a!='-c']+['-M','-MF',str(scan),'-MT',obj]
            run('scan-'+target,command,cwd=cwd)
            scans[target]=deps(scan,cwd);recipes[target]={'compile':recipe,'dependencies':scans[target]}
            link=build/'CMakeFiles'/(target+'.dir')/'link.txt';linkrow,_=material(link,4194304);remember(host,str(link),linkrow)
            words=shlex.split(link.read_text());inputs=[argv[0],words[0]]
            own=posixpath.normpath(posixpath.join(cwd,obj))
            for word in words[1:]:
                if not word.startswith('-') and re.search(r'[.](a|o|obj|dylib|so(?:[.][0-9]+)*)$',word):
                    path=posixpath.normpath(posixpath.join(cwd,word))
                    if path!=own:inputs.append(path)
            observe(inputs)
        save(out/'inputs-before.private.json',{'hostFD9':host,'guestStatSha':guest,'guestRealpathAliases':aliases,'recipes':recipes,'sdkAuthority':c['sdkAuthority'],'sourceRevision':c['sourceRevision'],'guestFdIdentityVerified':False})
        run('build',[cmake,'--build',str(build),'--target',*targets,'--parallel','2','--verbose'],timeout=600)
        for target in targets:
            recipe=recipes[target]['compile'];argv=shlex.split(recipe['command']);obj=argv[argv.index('-o')+1]
            md=pathlib.Path(posixpath.normpath(posixpath.join(recipe['directory'],obj+'.d')))
            assert deps(md,recipe['directory'])==scans[target],('M/MD differs',target)
            products[target]={'hostFD9':material(build/target)[0],'guestStatSha':observe([str(build/target)],retain=False)[str(build/target)]}
            product=products[target];assert product['hostFD9']['st_size']>0 and (product['hostFD9']['sha256'],product['hostFD9']['st_size'])==(product['guestStatSha']['sha256'],product['guestStatSha']['bytes'])
        if mode=='baseline':
            for target,expected in [('nano-peer-status-tests','malformed best_hash accepted'),('nano-json-stream-tests','oversized status accepted or stream not reset')]:
                label='causal-'+target;run(label,[str(build/target),'--status-bounds-self-test'],expect=1,timeout=15)
                assert expected in (out/(label+'.stderr.log')).read_text(),target
        else:
            for target in targets:
                if target=='nano-p2p':
                    run('usage-nano-p2p',[str(build/target),'--help'],expect=1,timeout=15)
                    assert 'Usage: nano-p2p ' in (out/'usage-nano-p2p.stderr.log').read_text()
                else:run('test-'+target,[str(build/target)],timeout=15)
    except BaseException as ex:first=ex
    finally:
        for path,want in host.items():
            try:assert material(path)[0]==want,('host input changed',path)
            except BaseException as ex:
                if first is None:first=ex
        try:
            after=observe(list(aliases));assert after==guest,'guest stat/SHA input changed'
        except BaseException as ex:
            if first is None:first=ex
        for target,want in products.items():
            try:
                assert material(out/'build'/target)[0]==want['hostFD9']
                assert observe([str(out/'build'/target)],retain=False)[str(out/'build'/target)]==want['guestStatSha']
            except BaseException as ex:
                if first is None:first=ex
        try:save(out/'observations-final.private.json',{'hostFD9':host,'guestStatSha':guest,'guestRealpathAliases':aliases,'products':products,'firstFailurePreserved':first is not None})
        except BaseException as ex:
            if first is None:first=ex
    public={'format':'nano-p2p-status-targeted-docker-exec-build-v1','passed':first is None,'mode':mode,'sourceRevision':c['sourceRevision'],'hostSystem':platform.system(),'hostArchitecture':platform.machine(),'phases':logs,'sdkAuthority':c['sdkAuthority'],'hostSourcesFD9AndShaUnchanged':first is None,'guestSelectedStatShaUnchanged':first is None,'guestFdIdentityVerified':False,'actualMmdMatchesM':first is None,'selectedGuestPaths':len(guest),'products':{k:{'sha256':v['hostFD9']['sha256'],'bytes':v['hostFD9']['st_size']} for k,v in products.items()},'imagesBuilt':0,'nodeServicesStarted':0,'containerLifecycleOwnedByRoot':True,'dutiesResumed':False,'networkQualified':False,'productionQualified':False}
    if first is not None:public['firstFailureType']=type(first).__name__
    try:save(out/'result-public.json',public);print(json.dumps(public,sort_keys=True),flush=True)
    except BaseException:
        if first is None:raise
    if first is not None:raise first

if __name__=='__main__':
    try:main()
    except BaseException:traceback.print_exc();sys.exit(1)
