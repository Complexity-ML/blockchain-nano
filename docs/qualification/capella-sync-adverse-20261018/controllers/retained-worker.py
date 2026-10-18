"""Incremental or retained Core plus explicit Linux RPC/runtime units. Root owns all operations."""
import hashlib, json, os, pathlib, posixpath, re, shlex, stat, subprocess, time

F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
OBSERVE = r'''set -eu
export LC_ALL=C
for p do
 r=$(/usr/bin/realpath -e -- "$p"); [ -f "$r" ] && [ ! -L "$r" ]
 printf '%s\0' "$r"
 /usr/bin/stat --printf='%d\0%i\0%f\0%h\0%u\0%g\0%s\0%y\0%z\0' -- "$r"
 h=$(/usr/bin/sha256sum < "$r"); printf '%s\0' "$h"
 /usr/bin/stat --printf='%d\0%i\0%f\0%h\0%u\0%g\0%s\0%y\0%z\0' -- "$r"
 [ "$r" = "$(/usr/bin/realpath -e -- "$p")" ]
done
'''


def run_qualification(config):
    """Execute the explicit candidate batch; persist partial evidence then raise on failure."""
    c = dict(config)
    def need(ok, why):
        if not ok: raise ValueError(why)
    mode=c.get('coreMode');need(mode in ('incremental','retained'),'explicit Core mode')
    common={'coreMode','hostOutput','sourceGuest','baselineSourceGuest','outputGuest','dockerPrefix','baselineCoreGuest','baselineCorePin','knownGuestPins','expectedObjectMembers','sourcePins','projectPins','units','cases','runtimeInputs','guestUid','guestGid','timeoutSeconds'}
    specific={'coreCompilerFlags','coreSources','qualifiedOldObjectRows','proveArchiveSet'}if mode=='incremental'else{'candidateCoreGuest','candidateCorePin'}
    optional={'ar'}if mode=='incremental'else set()
    need((common|specific)<=set(c)<=(common|specific|optional),'exact worker configuration; no silently ignored legacy batches')
    out = pathlib.Path(c['hostOutput']); og = c['outputGuest']; sg = c['sourceGuest']
    prefix = list(c['dockerPrefix']); baseline = c['baselineCoreGuest']; bg = c['baselineSourceGuest']
    need(out.is_absolute() and out.resolve(strict=True) == out and out.is_dir(), 'physical host output')
    need(all(isinstance(p,str) and p.startswith('/') and p!='/' and posixpath.normpath(p)==p for p in (sg,bg,og,baseline)) and not (og==sg or og.startswith(sg+'/') or sg.startswith(og+'/')) and not (bg==sg or bg.startswith(sg+'/') or sg.startswith(bg+'/') or bg==og or bg.startswith(og+'/') or og.startswith(bg+'/')) and baseline.endswith('/libnano_core.a') and not baseline.startswith(og+'/'), 'guest namespaces')
    need(all(isinstance(x,str) and x and '\0' not in x for x in prefix) and len(prefix) >= 3 and 'exec' in prefix[1:-1] and re.fullmatch('[0-9a-f]{64}',prefix[-1]), 'Root exec prefix')
    need(pathlib.Path(prefix[0]).is_absolute() and type(c['expectedObjectMembers'])is int and 1<=c['expectedObjectMembers']<=512, 'explicit tools/qualified object count')
    need((mode!='incremental'or callable(c['proveArchiveSet']))and type(c['guestUid'])is int and type(c['guestGid'])is int,'proof/guest owner')
    deadline = time.monotonic() + c.get('timeoutSeconds',1500)
    need(type(c['timeoutSeconds'])is int and 60<=c['timeoutSeconds']<=1800,'worker budget')
    source_pins = dict(c['sourcePins']); project = dict(c['projectPins']); known = dict(c['knownGuestPins'])
    def known_core(path,value):return type(value)is dict and set(value)=={'sha256','bytes'}and type(value['sha256'])is str and re.fullmatch('[0-9a-f]{64}',value['sha256'])is not None and type(value['bytes'])is int and 0<value['bytes']<=268435456 and path in known and known[path]==value
    if mode=='incremental':need(known_core(baseline,c['baselineCorePin']),'known baseline pin')
    else:need(type(c['candidateCoreGuest'])is str and posixpath.normpath(c['candidateCoreGuest'])==c['candidateCoreGuest'] and c['candidateCoreGuest'].startswith('/') and c['candidateCoreGuest'].endswith('/libnano_core.a') and not c['candidateCoreGuest'].startswith((og+'/',sg+'/',bg+'/')) and known_core(c['candidateCoreGuest'],c['candidateCorePin']),'known retained candidate Core pin')
    aliases, guest, products, steps, scans = {}, {}, {}, [], {}
    counts = dict(independentM=0,coreIndependentM=0,testIndependentM=0,compiledTranslationUnits=0,coreCompiledTranslationUnits=0,testCompiledTranslationUnits=0,archiveUpdates=0,links=0,loaderChecks=0,testInvocations=0,runtimeCliInvocations=0,unitInvocations=0,runtimeIndependentM=0,runtimeCompiledTranslationUnits=0,reusedSubmitObjects=0)
    serial = 0; first = None; core_proof = None; selection = {}; test_results = []; mmd = {}; unit_products = {}

    def save(name,value):
        with (out/name).open('x') as f:
            json.dump(value,f,sort_keys=True,indent=2);f.write('\n');f.flush();os.fsync(f.fileno())
    def host_file(path,body=False):
        need(path.resolve(strict=True) == path, 'physical host product')
        fd = os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
        try:
            a=os.fstat(fd);need(stat.S_ISREG(a.st_mode) and a.st_nlink==1 and 0<a.st_size<=268435456,'bounded host product')
            h=hashlib.sha256();parts=[];size=0
            while chunk:=os.read(fd,1048576):
                size+=len(chunk);need(size<=a.st_size,'host product growth');h.update(chunk)
                if body:parts.append(chunk)
            row={k:getattr(a,k) for k in F}
            need(size==a.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(path.lstat(),k) for k in F},'host FD/path changed')
            return dict(row,sha256=h.hexdigest()), b''.join(parts) if body else None
        finally:os.close(fd)
    def pin(row): return {'sha256':row['sha256'],'bytes':row.get('bytes',row.get('st_size'))}
    def mapped(path):
        need(path.startswith(og+'/') and '..' not in pathlib.PurePosixPath(path).parts,'own guest product')
        return out/path[len(og)+1:]
    def run(label,args,timeout=180,fence=False,expected_exit=0,workdir=None,test=False,runtime=False):
        limit=min(timeout,deadline+(60 if fence else 0)-time.monotonic());need(limit>0,'worker deadline')
        env={'HOME':'/tmp','TMPDIR':'/tmp','LANG':'C','LC_ALL':'C','CCACHE_DISABLE':'1'}
        command=prefix[:-1]+['--workdir',workdir or og]
        for k,v in sorted(env.items()):command+=['--env',k+'='+v]
        command += [prefix[-1],*map(str,args)];save(label+'.argv.private.json',command)
        event={'phase':label};error=None;at=time.monotonic()
        try:
            with (out/(label+'.stdout.log')).open('xb') as stdout,(out/(label+'.stderr.log')).open('xb') as stderr:
                result=subprocess.run(command,stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr,env={'PATH':'/usr/bin:/bin','LANG':'C','LC_ALL':'C'},timeout=limit)
            event['exitCode']=result.returncode
            if test: counts['testInvocations']+=1
            if runtime:counts['runtimeCliInvocations']+=1
            if test or runtime:counts['unitInvocations']+=1
        except BaseException as ex:error=ex;event.update(exitCode=None,errorType=type(ex).__name__)
        event['elapsedMs']=round((time.monotonic()-at)*1000);steps.append(event)
        try:save(label+'.exit.json',event)
        except BaseException:
            if error is None:raise
        if error is not None:raise error
        need(event['exitCode']==expected_exit,'command refused: '+label)
        need(all((out/(label+s)).stat().st_size<=16777216 for s in ('.stdout.log','.stderr.log')),'command capture bound')
    def observe(paths,fence=False):
        nonlocal serial
        paths=sorted(set(paths));need(len(paths)<=20000,'selected path count');rows={};resolved={}
        for at in range(0,len(paths),128):
            batch=paths[at:at+128];need(all(p.startswith('/') and not any(x in p for x in ('\0','\n','\r')) for p in batch),'guest paths')
            serial+=1;label='observe-'+str(serial);run(label,['/bin/sh','-c',OBSERVE,'selected-inputs',*batch],90,fence)
            raw=(out/(label+'.stdout.log')).read_bytes();need(len(raw)<=1048576 and raw.endswith(b'\0'),'observation bound')
            fields=raw[:-1].decode().split('\0');need(len(fields)==20*len(batch),'observation field count')
            for i,literal in enumerate(batch):
                values=fields[i*20:(i+1)*20];physical,a,h,b=values[0],values[1:10],values[10],values[11:20]
                need(physical.startswith('/') and a==b and re.fullmatch('[0-9a-f]{64}  -',h),'guest stat/hash coherence')
                need(all(re.fullmatch('[0-9]+',a[j]) for j in (0,1,3,4,5,6)) and re.fullmatch('[0-9a-f]+',a[2]),'GNU nine encoding')
                need(stat.S_ISREG(int(a[2],16)) and 0<int(a[6])<=268435456,'guest regular file bound')
                row={'materialGNU9':a,'sha256':h[:64],'bytes':int(a[6])}
                need(physical not in rows or rows[physical]==row,'shared guest file changed')
                rows[physical]=row;resolved[literal]=physical
        return rows,resolved
    def admit_inputs(paths,label):
        previous=None;stable=0;since=None;start=time.monotonic();pins={};samples=[]
        while True:
            need(time.monotonic()<min(deadline,start+60),'input first-admission deadline')
            rows,links=observe(paths)
            for literal,physical in links.items():
                row=rows[physical]
                need(literal not in aliases or aliases[literal]==physical,'retained alias changed')
                need(physical not in guest or guest[physical]==row,'retained guest input changed')
                expected=known.get(literal) or project.get(literal)
                if literal.startswith((sg+'/',bg+'/')):need(literal in project and physical==literal,'known project source required')
                if physical.startswith((sg+'/',bg+'/')):need(physical in project and pin(row)==project[physical],'resolved project source pin')
                if literal.startswith(('/old/','/baseline/')):need(literal in known,'known old bind pin required')
                if expected is not None:need(pin(row)==expected,'known input bytes changed: '+literal)
                need(physical not in pins or pins[physical]==pin(row),'prospective SDK bytes changed')
                pins.setdefault(physical,pin(row))
            now=time.monotonic();need(now<min(deadline,start+60),'input first-admission deadline after observation')
            if (rows,links)==previous:stable+=1
            else:stable=1;since=now
            samples.append({'elapsedMs':round((now-start)*1000),'files':rows,'aliases':links})
            if stable>=3 and now-since>=2:break
            previous=(rows,links);time.sleep(1.05)
        for path,row in rows.items():guest.setdefault(path,row)
        for path,physical in links.items():aliases.setdefault(path,physical)
        save(label+'-first-admission.private.json',{'knownBytePins':pins,'observations':samples,'oldAuthoritiesRefreshed':False})
    def admit_products(paths,label,executables=()):
        need(all(p not in aliases and p not in guest for p in paths),'new products only')
        for p in paths:need(not mapped(p).exists() or str(mapped(p)) not in products,'product already admitted')
        initial,_=observe(paths);pins={p:pin(initial[p]) for p in paths}
        need(set(executables)<=set(paths),'executable admission subset')
        ordinary=[p for p in paths if p not in executables]
        if ordinary:run(label+'-mode',['/bin/chmod','400',*ordinary])
        if executables:run(label+'-exec-mode',['/bin/chmod','500',*executables])
        previous=None;stable=0;since=None;start=time.monotonic();samples=[]
        while True:
            need(time.monotonic()<min(deadline,start+30),'new product admission deadline')
            rows,_=observe(paths);table={}
            for p in paths:
                row=rows[p];host,_=host_file(mapped(p));need(pin(row)==pins[p]==pin(host),'new product byte pin changed')
                need(int(row['materialGNU9'][4])==c['guestUid'] and int(row['materialGNU9'][5])==c['guestGid'] and row['materialGNU9'][3]=='1','new product owner')
                table[p]={'hostFD9':host,'guestStatSha':row}
            now=time.monotonic();need(now<min(deadline,start+30),'new product admission deadline after observation')
            if table==previous:stable+=1
            else:stable=1;since=now
            samples.append({'elapsedMs':round((now-start)*1000),'files':table})
            if stable>=3 and now-since>=2:break
            previous=table;time.sleep(1.05)
        for p,row in table.items():products[str(mapped(p))]=row
        save(label+'-products-first-admission.private.json',{'knownBytePins':pins,'observations':samples,'oldAuthoritiesRefreshed':False})
    def dependency_names(name,base=None):
        host,body=host_file(mapped(name),True);want=products.get(str(mapped(name)))
        need(want is not None and host==want['hostFD9'] and len(body)<=4194304,'retained dependency product')
        text=body.decode().replace('\\\n',' ');_,sep,text=text.partition(':');need(sep,'dependency rule')
        names=sorted(set(posixpath.normpath(posixpath.join(base or og,p)) for p in shlex.split(text)))
        need(names and all(p.startswith('/')for p in names),'absolute normalized dependencies');return names
    def admit_chunks(paths,label):
        values=sorted(set(paths))
        for at in range(0,len(values),512):admit_inputs(values[at:at+512],label+'-'+str(at//512))
    def product_chunks(paths,label):
        for at in range(0,len(paths),128):admit_products(paths[at:at+128],label+'-'+str(at//128))
    def scope_dependencies(names,scope):
        forbidden=sg if scope=='baseline' else bg
        reached=set(names)|{aliases[p]for p in names}
        need(not any(p==forbidden or p.startswith(forbidden+'/')for p in reached),'cross-ABI source-root dependency')
    def flags(values):
        need(type(values)is list,'compile flag array');values=list(values);need(values and all(isinstance(x,str) and x and '\0' not in x for x in values),'compile flags')
        need(not any(x in ('-o','-c','-M','-MM','-MD','-MMD','-MF','-MT') for x in values),'compile suffix already present');return values

    def loader(label,exe):
        run('LDD-'+label,['/usr/bin/ldd',exe],30);counts['loaderChecks']+=1
        need('not found' not in (out/('LDD-'+label+'.stdout.log')).read_text(),'unresolved runtime library')
    def link(label,obj,tail,prefix_args,basename):
        exe=og+'/'+basename;need(not mapped(exe).exists(),'fresh executable')
        run('LINK-'+label,list(prefix_args)+[obj,'-o',exe]+tail,180);counts['links']+=1
        admit_products([exe],label+'-executable',executables=[exe]);loader(label,exe);return exe
    def json_records(raw):
        def unique(pairs):
            keys=[k for k,_ in pairs];need(len(keys)==len(set(keys)),'duplicate JSON key');return dict(pairs)
        def nonfinite(value):raise ValueError('nonfinite JSON constant')
        return [json.loads(line,object_pairs_hook=unique,parse_constant=nonfinite)for line in raw.decode().splitlines()]
    def proof_stream(records):
        names=['get-proof-native-mpt-shapes-and-canonical-rlp','get-proof-account-storage-and-object-selectors','get-proof-errors-configurable-budgets-and-cancellation','get-proof-pinned-reorg-real-retention-and-reopen','get-proof-http-websocket-capacity-and-shutdown'];reports=[];vectors=0;checks=0;refusals=0
        def exact(value,keys):need(type(value)is dict and set(value)==set(keys),'proof record exact fields')
        def integer(value,minimum,maximum):need(type(value)is int and minimum<=value<=maximum,'proof bounded integer')
        def quantity(value):need(type(value)is str and re.fullmatch(r'0x(?:0|[1-9a-f][0-9a-f]{0,63})',value),'proof canonical uint256 quantity')
        def hexd(value,size):need(type(value)is str and re.fullmatch('0x[0-9a-f]{'+str(size*2)+'}',value),'proof hex width')
        def nodes(value):
            need(type(value)is list and len(value)<=130,'proof node count')
            for node in value:need(type(node)is str and len(node)<=65538 and re.fullmatch(r'0x(?:[0-9a-f]{2})+',node),'bounded proof node hex')
        need(type(records)is list and len(records)==30,'proof stream requires 24 vectors, five reports and summary')
        for record in records[:-1]:
            need(type(record)is dict,'proof record object')
            if record.get('scenario')=='proof-vector':
                exact(record,('scenario','root','address','keys','expectedPresent','expectedValues','response'));hexd(record['root'],32);hexd(record['address'],20);need(type(record['expectedPresent'])is bool and 1<=len(reports)<5,'proof presence/report phase')
                keys=record['keys'];values=record['expectedValues'];need(type(keys)is list and len(keys)<=64 and type(values)is list and len(values)==len(keys),'proof requested keys/expected values')
                for key in keys:need(type(key)is str and re.fullmatch(r'0x[0-9a-f]{1,64}',key),'proof request uint256 key')
                for value in values:quantity(value)
                response=record['response'];exact(response,('address','balance','nonce','codeHash','storageHash','accountProof','storageProof'));need(response['address']==record['address'],'proof response address');quantity(response['balance']);quantity(response['nonce']);hexd(response['codeHash'],32);hexd(response['storageHash'],32);nodes(response['accountProof']);need(type(response['storageProof'])is list and len(response['storageProof'])==len(keys),'proof storage response length')
                for key,value,entry in zip(keys,values,response['storageProof']):exact(entry,('key','value','proof'));quantity(entry['key']);quantity(entry['value']);need(int(entry['key'],16)==int(key,16) and entry['value']==value,'proof storage ordered key/value');nodes(entry['proof'])
                vectors+=1;need(vectors<=24,'proof vector count')
            else:
                exact(record,('scenario','passed','checks','expectedRefusals'));need(len(reports)<5 and record['scenario']==names[len(reports)]and record['passed']is True,'five ordered proof scenario reports');integer(record['checks'],checks+1,10000000);integer(record['expectedRefusals'],refusals,63);checks=record['checks'];refusals=record['expectedRefusals'];reports.append(record['scenario'])
        summary=records[-1];exact(summary,('scenario','passed','groups','checks','expectedRefusals','proofVectors','selectedGroup','externalNetwork','consensusModified','productionQualified'));need(summary['scenario']=='get-proof-summary'and summary['passed']is True and summary['selectedGroup']=='all'and all(summary[k]is False for k in('externalNetwork','consensusModified','productionQualified')),'proof final summary/scope');integer(summary['checks'],checks+1,10000000)
        for key,value in (('groups',5),('expectedRefusals',63),('proofVectors',24)):integer(summary[key],value,value)
        need(len(reports)==5 and vectors==24 and refusals==63,'proof final observed groups/vectors/cumulative refusals')
        return {'policy':'get-proof-record-stream-v1','records':30,'proofVectors':vectors,'scenarioReports':len(reports),'summary':summary,'mechanicalCppPassed':True,'rootIndependentProofVectorsVerified':False}
    def stream_oracle(path,policy):
        need(type(policy)is dict and type(policy.get('kind'))is str,'explicit stream oracle');kind=policy['kind'];raw=path.read_bytes();need(len(raw)<=16777216,'stream bound')
        if kind=='empty':need(set(policy)=={'kind'}and raw==b'','expected empty stream')
        elif kind=='exact':need(set(policy)=={'kind','text'}and type(policy['text'])is str and len(policy['text'].encode())<=1048576 and raw==policy['text'].encode(),'exact stream mismatch')
        elif kind=='json-lines':
            need(set(policy)=={'kind','records'}and type(policy['records'])is list and 1<=len(policy['records'])<=128 and all(type(x)is dict for x in policy['records']),'JSON record oracle');actual=[json.loads(x)for x in raw.decode().splitlines()];need(json.dumps(actual,sort_keys=True,separators=(',',':'),allow_nan=False)==json.dumps(policy['records'],sort_keys=True,separators=(',',':'),allow_nan=False),'typed exact JSON records mismatch')
        elif kind=='last-line':
            need(set(policy)=={'kind','text','contains','count'}and all(type(policy[k])is str and 0<len(policy[k])<=4096 for k in ('text','contains'))and type(policy['count'])is int and 1<=policy['count']<=128,'last line oracle');lines=raw.decode().splitlines();need(lines and lines[-1]==policy['text']and sum(policy['contains']in x for x in lines)==policy['count'],'last line oracle mismatch')
        elif kind=='json-field-records':
            need(set(policy)=={'kind','records'}and type(policy['records'])is list and 1<=len(policy['records'])<=128,'field record policies');actual=json_records(raw);need(len(actual)==len(policy['records']),'field record count')
            for record,rule in zip(actual,policy['records']):
                need(type(rule)is dict and set(rule)=={'expectedFields','integerFields'}and type(rule['expectedFields'])is dict and type(rule['integerFields'])is dict and rule['expectedFields']and not set(rule['expectedFields'])&set(rule['integerFields']),'field record rule');fixed=rule['expectedFields'];ints=rule['integerFields'];need(type(record)is dict and set(record)==set(fixed)|set(ints),'field record exact keys');need(json.dumps({k:record[k]for k in fixed},sort_keys=True,separators=(',',':'),allow_nan=False)==json.dumps(fixed,sort_keys=True,separators=(',',':'),allow_nan=False),'typed fixed record fields')
                for key,limits in ints.items():need(type(limits)is dict and set(limits)=={'minimum','maximum'}and type(limits['minimum'])is int and type(limits['maximum'])is int and 0<=limits['minimum']<=limits['maximum']<=10000000 and type(record[key])is int and limits['minimum']<=record[key]<=limits['maximum'],'typed bounded record integer')
        elif kind=='get-proof-record-stream-v1':need(set(policy)=={'kind'},'dedicated proof stream policy');return proof_stream(json_records(raw))
        else:raise ValueError('unknown stream oracle')
        return {'policy':kind,'semanticOraclePassed':True}
    try:
        core_flags=flags(c['coreCompilerFlags'])if mode=='incremental'else[];ar=c.get('ar','/usr/bin/ar')
        sources=list(c['coreSources'])if mode=='incremental'else[];units=list(c['units']);cases=list(c['cases']);runtime=list(c['runtimeInputs'])
        if mode=='incremental':
            need(len(sources)==c['expectedObjectMembers']and len(set(sources))==len(sources)and all(type(x)is str and re.fullmatch(r'src/[A-Za-z0-9_+.-]+[.]cpp',x)and pathlib.PurePosixPath(x).name!='main.cpp'for x in sources),'explicit complete Core source set')
            members={rel:pathlib.PurePosixPath(rel).name+'.o'for rel in sources};need(set(members.values())=={r['name']for r in c['qualifiedOldObjectRows']},'Core sources/qualified member names bijection')
        need(1<=len(units)<=32 and 1<=len(cases)<=64 and len(runtime)<=128,'bounded explicit batch')
        by_name={};unit_flags={};outputs=set();reuses={};tails={}
        for u in units:
            need(type(u)is dict and set(u)=={'name','scope','source','compilerFlags','linkPrefix','linkTail','executable','object'}and type(u['name'])is str and re.fullmatch('[A-Za-z][A-Za-z0-9_-]{0,63}',u['name'])and u['name']not in by_name and u['scope']in('baseline','candidate','runtime'),'unit descriptor');root=bg if u['scope']=='baseline'else sg
            need(type(u['source'])is str and u['source'].startswith(root+'/')and u['source'].endswith('.cpp')and posixpath.normpath(u['source'])==u['source'],'scoped unit source')
            need(type(u['executable'])is str and re.fullmatch('[A-Za-z][A-Za-z0-9_-]{0,95}',u['executable'])and u['executable']not in outputs and u['executable']!='libnano_core.a','distinct executable basename');outputs.add(u['executable'])
            need(type(u['linkPrefix'])is list and u['linkPrefix']and type(u['linkTail'])is list and u['linkTail'].count(baseline)==1 and all(type(x)is str and x and '\0'not in x for x in u['linkPrefix']+u['linkTail']),'one Core reference in explicit per-unit link recipe')
            need(pathlib.PurePosixPath(u['linkPrefix'][0]).is_absolute(),'absolute linker driver')
            obj=u['object'];need(type(obj)is dict and obj.get('kind')in('compile','reuse'),'explicit object mode')
            if obj['kind']=='compile':need(set(obj)=={'kind'},'compile object fields')
            else:
                need(u['scope']=='runtime'and u['source']==sg+'/tests/multihost/lifecycle_submit.cpp'and set(obj)=={'kind','path','dependencyFile','dependencyBase','dependencyPins','compilerFlags'}and obj['compilerFlags']==u['compilerFlags'],'Submit only, unchanged qualified compiler flags')
                for key in('path','dependencyFile','dependencyBase'):need(type(obj[key])is str and obj[key].startswith('/')and posixpath.normpath(obj[key])==obj[key]and not obj[key].startswith((og+'/',sg+'/',bg+'/')),'external retained object/MD/base')
                need(obj['path']!=obj['dependencyFile']and obj['path']in known and obj['dependencyFile']in known and type(obj['dependencyPins'])is dict and 1<=len(obj['dependencyPins'])<=20000,'known original Submit object/MD/dependencies');reuses[u['name']]=obj
            by_name[u['name']]=u;unit_flags[u['name']]=flags(u['compilerFlags']);tails[u['name']]=list(u['linkTail'])
        case_names=set()
        for t in cases:
            need(type(t)is dict and set(t)=={'name','unit','args','timeoutSeconds','expectedExit','stdout','stderr'}and type(t['name'])is str and re.fullmatch('[A-Za-z][A-Za-z0-9_-]{0,63}',t['name'])and t['name']not in case_names and t['unit']in by_name,'case descriptor');case_names.add(t['name']);scope=by_name[t['unit']]['scope']
            need(type(t['expectedExit'])is int and (t['expectedExit']in(0,1)if scope=='runtime'else t['expectedExit']==(1 if scope=='baseline'else 0))and type(t['timeoutSeconds'])is int and 1<=t['timeoutSeconds']<=300,'scoped exact exit/deadline')
            need(type(t['args'])is list and len(t['args'])<=16 and all(type(x)is str and '\0'not in x and len(x)<=4096 for x in t['args']),'case args')
            if scope=='runtime':need(t['args']in([],['--help'])and t['timeoutSeconds']<=30,'arity/help only; no serve, DB or consensus invocation')
            need(type(t['stdout'])is dict and type(t['stderr'])is dict and any(p.get('kind')!='empty'for p in(t['stdout'],t['stderr'])),'independent nonempty semantic output oracle required')
        need({t['unit']for t in cases}==set(by_name),'every unit exercised; no ignored runtime or baseline')
        need(all(type(p)is str and p.startswith((sg+'/',bg+'/'))and p in project and source_pins.get(p)==project[p]for p in runtime),'explicit runtime fixture pins')
        old={p[len(bg)+1:]:v for p,v in project.items()if p.startswith(bg+'/')};new={p[len(sg)+1:]:v for p,v in project.items()if p.startswith(sg+'/')}
        required={sg+'/CMakeLists.txt',*[u['source']for u in units],*runtime}
        if any(u['scope']=='baseline'for u in units):required.add(bg+'/CMakeLists.txt')
        if mode=='incremental':
            need(all(rel in old and rel in new for rel in sources),'both source projections contain all Core TUs')
            changed_project=sorted(rel for rel in set(old)|set(new)if old.get(rel)!=new.get(rel));changed_headers=[rel for rel in changed_project if rel.endswith(('.hpp','.h'))];changed_sources=sorted(rel for rel in sources if old[rel]!=new[rel]);changed_paths={sg+'/'+rel for rel in changed_project if rel in new}
            required|={bg+'/CMakeLists.txt',*[sg+'/'+r for r in changed_sources],*[sg+'/'+r for r in changed_headers if r in new]};need(changed_headers or changed_sources,'explicit source/header delta required')
        need(all(p in project and source_pins.get(p)==project[p]for p in required),'changed surfaces/unit sources/fixtures joined')
        tools=['/bin/cp','/bin/chmod','/bin/cat','/usr/bin/ld','/usr/bin/ldd',*[u['linkPrefix'][0]for u in units],*[a[0]for a in unit_flags.values()]]
        inputs=[*tools,*required]
        if mode=='incremental':inputs += [baseline,ar,core_flags[0],*[sg+'/'+r for r in sources]]
        else:inputs.append(c['candidateCoreGuest'])
        if any(u['scope']=='baseline'for u in units):need(known_core(baseline,c['baselineCorePin']),'known original baseline Core');inputs.append(baseline)
        for u in units:
            inputs += [posixpath.normpath(posixpath.join(og,w))for w in u['linkTail']if not w.startswith('-')and w!=baseline]
            if u['name']in reuses:inputs += [reuses[u['name']]['path'],reuses[u['name']]['dependencyFile']]
        admit_chunks(inputs,'tools-sources')
        if mode=='incremental':
            scans_by_source={};literals={};all_dependencies=[]
            for rel in sources:
                obj=og+'/'+members[rel];scan=obj+'.M';need(not any(mapped(p).exists()for p in(obj,obj+'.d',scan)),'fresh Core products');role='core-'+pathlib.PurePosixPath(rel).stem;run('M-'+role,core_flags+['-M','-MF',scan,'-MT',obj,sg+'/'+rel]);counts['independentM']+=1;counts['coreIndependentM']+=1;scans_by_source[rel]=scan
            product_chunks(list(scans_by_source.values()),'core-scans')
            for rel,scan in scans_by_source.items():literals[rel]=dependency_names(scan);all_dependencies+=literals[rel]
            admit_chunks(all_dependencies,'all-Core-M-inputs');selected=[];selection_rows=[]
            for rel in sources:
                names=literals[rel];scope_dependencies(names,'candidate');physical=sorted({aliases[p]for p in names});header_hits=sorted((set(names)|set(physical))&changed_paths);choose=rel in changed_sources or bool(header_hits);role='core-'+pathlib.PurePosixPath(rel).stem
                if choose:selected.append(rel)
                scans[role]=physical;selection_rows.append({'source':rel,'member':members[rel],'sourceBytesChanged':rel in changed_sources,'changedProjectDependenciesReached':header_hits,'dependencyCount':len(physical),'compileSelected':choose})
            need(selected,'nonempty Core replacement set');selection={'coreSourceCount':len(sources),'independentCoreScans':counts['coreIndependentM'],'changedHeaderPaths':changed_headers,'changedProjectPaths':changed_project,'modifiedCoreSourcePaths':changed_sources,'selectedSources':selected,'rows':selection_rows};save('core-recompile-selection-public.json',selection)
            objects={}
            for rel in selected:
                obj=og+'/'+members[rel];md=obj+'.d';role='core-'+pathlib.PurePosixPath(rel).stem;run('CXX-'+role,core_flags+['-MD','-MF',md,'-MT',obj,'-o',obj,'-c',sg+'/'+rel],300);counts['compiledTranslationUnits']+=1;counts['coreCompiledTranslationUnits']+=1;admit_products([obj,md],role);actual=dependency_names(md);admit_chunks(actual,role+'-MD-inputs');scope_dependencies(actual,'candidate');resolved=sorted({aliases[p]for p in actual});need(resolved==scans[role],'Core M differs from actual MD');mmd[role]={'normalizedMEqualsMd':True,'literalMEqualsMd':actual==literals[rel],'dependencyCount':len(resolved)};objects[members[rel]]=obj
            core=og+'/libnano_core.a';need(not mapped(core).exists(),'new Core absent');run('copy-core',['/bin/cp','--',baseline,core]);oldrow,oldbytes=host_file(mapped(core),True);need(pin(oldrow)==c['baselineCorePin'],'copied old Core pin');run('writable-new-core',['/bin/chmod','600',core]);run('replace-selected-Core-objects',[ar,'rcs',core,*objects.values()],180);counts['archiveUpdates']+=1;admit_products([core],'new-core');newrow,newbytes=host_file(mapped(core),True)
            replacements={name:pin(products[str(mapped(obj))]['hostFD9'])for name,obj in objects.items()};core_proof=c['proveArchiveSet'](oldbytes,newbytes,replacements=replacements,qualified_old_rows=c['qualifiedOldObjectRows'],old_archive_pin=c['baselineCorePin'],expected_members=c['expectedObjectMembers']);need(core_proof.get('passed')is True and core_proof.get('members')==c['expectedObjectMembers']and set(core_proof.get('replacementMembers',[]))==set(replacements)and core_proof.get('selectedReplacementCount')==len(replacements)and core_proof.get('unchangedOutsideReplacementSet')==c['expectedObjectMembers']-len(replacements),'exact selected-set archive proof');save('core-member-proof.json',core_proof)
        else:
            core=c['candidateCoreGuest'];selection={'mode':'retained','coreScansRepeated':0,'coreCompilationsRepeated':0,'archiveUpdatesRepeated':0,'originalCorePin':c['candidateCorePin']}
        executables={}
        for name,u in by_name.items():
            argv=unit_flags[name];obj=og+'/unit-'+name+'.o';scan=obj+'.M';md=obj+'.d';need(not any(mapped(p).exists()for p in(obj,scan,md)),'fresh scoped unit products')
            run('M-'+name,argv+['-M','-MF',scan,'-MT',obj,u['source']]);counts['independentM']+=1;counts['runtimeIndependentM'if u['scope']=='runtime'else'testIndependentM']+=1
            admit_products([scan],name+'-scan');literal=dependency_names(scan);admit_chunks(literal,name+'-inputs');scope_dependencies(literal,u['scope']);current=sorted({aliases[p]for p in literal});need(u['source']in literal,'declared unit source in M')
            if name in reuses:
                retained=reuses[name];captured=og+'/unit-'+name+'.retained.d';need(not mapped(captured).exists(),'fresh retained MD copy')
                run('COPY-MD-'+name,['/bin/cp','--',retained['dependencyFile'],captured]);admit_products([captured],name+'-retained-MD');need(pin(products[str(mapped(captured))]['hostFD9'])==known[retained['dependencyFile']],'copied original MD byte pin')
                actual=dependency_names(captured,retained['dependencyBase']);need(set(actual)==set(retained['dependencyPins']),'qualified original MD/pin names exact');admit_chunks(actual,name+'-qualified-MD-inputs');scope_dependencies(actual,u['scope'])
                need(sorted({aliases[p]for p in actual})==current,'current Submit M differs from original MD')
                for p in actual:need(pin(guest[aliases[p]])==retained['dependencyPins'][p],'Submit original dependency bytes changed: '+p)
                obj=retained['path'];need(pin(guest[aliases[obj]])==known[obj],'original Submit object pin');counts['reusedSubmitObjects']+=1
                mmd[name]={'scope':u['scope'],'objectReused':True,'qualifiedCompilerFlagsEqual':True,'currentMEqualsQualifiedMd':True,'literalMEqualsQualifiedMd':actual==literal,'dependencyCount':len(current)}
            else:
                run('CXX-'+name,argv+['-MD','-MF',md,'-MT',obj,'-o',obj,'-c',u['source']],300);counts['compiledTranslationUnits']+=1;counts['runtimeCompiledTranslationUnits'if u['scope']=='runtime'else'testCompiledTranslationUnits']+=1
                admit_products([obj,md],name+'-object');actual=dependency_names(md);admit_chunks(actual,name+'-MD-inputs');scope_dependencies(actual,u['scope']);need(sorted({aliases[p]for p in actual})==current,'scoped unit M differs from actual MD');mmd[name]={'scope':u['scope'],'objectReused':False,'normalizedMEqualsMd':True,'literalMEqualsMd':actual==literal,'dependencyCount':len(current)}
            link_tail=tails[name]if u['scope']=='baseline'else[core if p==baseline else p for p in tails[name]]
            executables[name]=link(name,obj,link_tail,u['linkPrefix'],u['executable']);unit_products[name]={'scope':u['scope'],'source':u['source'],'object':obj,'objectReused':name in reuses,'executable':executables[name]}
        for t in cases:
            u=by_name[t['unit']];label='test-'+t['name'];run(label,[executables[t['unit']],*t['args']],t['timeoutSeconds'],expected_exit=t['expectedExit'],workdir=bg if u['scope']=='baseline'else sg,test=u['scope']!='runtime',runtime=u['scope']=='runtime')
            stdout_oracle=stream_oracle(out/(label+'.stdout.log'),t['stdout']);stderr_oracle=stream_oracle(out/(label+'.stderr.log'),t['stderr'])
            test_results.append({'name':t['name'],'unit':t['unit'],'scope':u['scope'],'actualExitCode':t['expectedExit'],'semanticStreamOraclesPassed':True,'baselineExpectedFailure':u['scope']=='baseline','runtimeArityOnly':u['scope']=='runtime','stdoutOracle':stdout_oracle,'stderrOracle':stderr_oracle});save('case-'+t['name']+'-public.json',test_results[-1])
    except BaseException as error:first=error
    finally:
        try:
            rows,links=observe(list(aliases),True);need(rows==guest and links==aliases,'final selected guest input changed')
            for path,want in products.items():
                p=og+'/'+str(pathlib.Path(path).relative_to(out));rows,_=observe([p],True);need(host_file(pathlib.Path(path))[0]==want['hostFD9']and rows[p]==want['guestStatSha'],'final product changed: '+p)
            save('final-observations.private.json',{'guestStatSha':guest,'guestAliases':aliases,'products':products,'guestFdIdentityVerified':False})
        except BaseException as error:
            if first is None:first=error
        report={'format':'nano-linux-rpc-state-proof-runtime-units-qualification-v1','coreMode':mode,'unitProducts':unit_products,'passed':first is None,'counts':counts,'selection':selection,'coreProof':core_proof,'testResults':test_results,'allConfiguredCasesPassed':first is None and len(test_results)==len(c.get('cases',[])),'rootIndependentProofVectorsVerified':False,'mmd':mmd,'steps':steps,'selectedGuestInputFiles':len(guest),'guestFdIdentityVerified':False,'hostOriginalAuthoritiesVerifiedByThisWorker':False,'containerLifecycleOwnedByRoot':True,'coreFullBuilds':int(mode=='incremental'and counts['coreCompiledTranslationUnits']==c['expectedObjectMembers']),'retainedCandidateCoreVerifiedByParent':False,'runtimeServeOrRpcQualified':False,'cmakeConfigures':0,'p2pCommands':0,'externalNetworkRequired':False,'runtimeDutiesResumed':False,'productionQualified':False}
        if first is not None:report['firstFailureType']=type(first).__name__
        try:save('result-public.json',report)
        except BaseException:
            if first is None:raise
    if first is not None:raise first
    return report
