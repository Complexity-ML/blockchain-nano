"""Root-only raw export of an existing public Native04 fixture; no DB API."""
import hashlib, json, os, pathlib, signal, stat, sys, tarfile, time, types

E = pathlib.Path(__file__).absolute().parents[1]
N = E / 'native-c7-cmake-root-20261018-01/native-successor-04'
TREE = N / 'cohort-stores'
OUT = E / 'physical-native-export-root-20261018-01'
COMMON = pathlib.Path(__file__).absolute().with_name('cohort_common.py')
COMMON_PIN = {'sha256':'09422e11287ce5f0357e3b3e9e5219d6c492ac2a3d3815aa6bbb6aa2ade7943c','bytes':10200}
MF_PIN = {'sha256':'b8dfb4a9b2144c037f9f85a2137d0c4047bd0f344a739290b22c9242369246e6','bytes':12445}
PROOFS = {
 'producerTerminal': (N/'cohort-produce.terminal.json', {'sha256':'dd6e7880efb4a8a4a3649f1c905e7e2c7ad5033db8b568fcefed76a05cd88ec8','bytes':3307}),
 'checkerTerminal': (N/'cohort-check.terminal.json', {'sha256':'c71f5d186272c3d1de8df97348bc017a07cd912c58c991ec8a041b3808b0774f','bytes':3300}),
 'nativeHostResult': (N/'result-public.json', {'sha256':'a6117e885646e809be061ba98fbcc08fea3ff1728b128866a4056f239d0b1cdc','bytes':30408})}
F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
MAX_BYTES, MAX_ENTRIES = 128*1024*1024, 4096
START = time.monotonic()
held, creators, adopted_outputs, diagnostics = {}, {}, {}, {}
source_admitted = None
common = None
owned = False

def need(ok, why):
    if not ok: raise ValueError(why)

def guard():
    need(time.monotonic()-START < 180, 'export180s deadline')

def pin(raw): return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}

def decoded(raw):
    def unique(pairs):
        result={}
        for key,value in pairs: need(key not in result,'duplicate JSON key'); result[key]=value
        return result
    return json.loads(raw,object_pairs_hook=unique,parse_constant=lambda _:(_ for _ in()).throw(ValueError('nonfinite JSON')))

def material(path, maximum=8*1024*1024):
    guard(); path = pathlib.Path(path)
    need(path.is_absolute() and path.resolve(strict=True)==path, 'canonical explicit input')
    fd = os.open(path, os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK|os.O_CLOEXEC)
    try:
        a=os.fstat(fd); row={k:getattr(a,k) for k in F}
        need(stat.S_ISREG(a.st_mode) and a.st_nlink==1 and 0<=a.st_size<=maximum, 'bounded single-link input')
        chunks=[]; size=0
        while part:=os.read(fd,65536):
            guard(); size+=len(part); need(size<=a.st_size, 'input grew'); chunks.append(part)
        raw=b''.join(chunks)
        need(size==a.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(path.lstat(),k) for k in F}, 'input FD9/path stable during read')
        return dict(row,sha256=hashlib.sha256(raw).hexdigest()),raw
    finally: os.close(fd)

def retain(path, wanted, original=None):
    row,raw=material(path)
    need(pin(raw)==wanted, 'independently known bytes: '+str(path))
    if original is not None:
        need(type(original)is dict and set(original)==set(F) and all(type(original[k])is int for k in F), 'closed original descriptor FD9')
        need({k:row[k] for k in F}==original, 'original proof descriptor FD9 unchanged')
    need(str(path) not in held or held[str(path)]==row, 'held input cannot be refreshed')
    held.setdefault(str(path),row)
    return raw

def original_proof(descriptor, expected_path, expected_pin):
    need(type(descriptor)is dict and set(descriptor)=={'path','pin','fd9'} and descriptor['path']==str(expected_path) and descriptor['pin']==expected_pin, 'exact fixed proof descriptor')
    need(type(descriptor['fd9'])is dict and set(descriptor['fd9'])==set(F), 'proof descriptor requires original FD9')
    return retain(expected_path,expected_pin,descriptor['fd9'])

def fence_inputs():
    for path,row in held.items(): need(material(path)[0]==row, 'retained external input changed')

def source_fence():
    need(source_admitted is not None and common.snapshot(TREE,(501,20))==source_admitted, 'current per-export69 source FD9/SHA changed')

def observation_log(label, number, value):
    raw=(json.dumps(value,sort_keys=True,indent=2,allow_nan=False)+'\n').encode()
    need(len(raw)<=4*1024*1024,'bounded admission observation')
    path=OUT/(label+'-sample-'+str(number).zfill(2)+'-public.json')
    fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o400)
    with os.fdopen(fd,'wb')as stream:
        need(stream.write(raw)==len(raw),'complete exclusive observation'); stream.flush(); os.fsync(stream.fileno())
    parent=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW|os.O_CLOEXEC)
    try: os.fsync(parent)
    finally: os.close(parent)

def freeze_observation(label, observe, anchor=None, validate=None, peer=None):
    began=time.monotonic(); previous=None; since=None; streak=0; changes=0; records=[]
    diagnostics[label]={'firstEightSha':anchor,'samples':records,'admitted':False}
    for sample_number in range(1,31):
        guard(); fence_inputs()
        if peer: peer()
        sample=observe()
        if peer: peer()
        first_error=None
        if anchor is None:
            anchor=sample
            if validate:
                try: validate(anchor)
                except BaseException as exc: first_error=exc
            diagnostics[label]['firstEightSha']=anchor
        names_changed={'missing':sorted(set(anchor)-set(sample)),'added':sorted(set(sample)-set(anchor))}
        bad=[p for p in set(anchor)&set(sample) if {k:v for k,v in sample[p].items() if k!='st_ctime_ns'}!={k:v for k,v in anchor[p].items() if k!='st_ctime_ns'}]
        equal=previous is not None and sample==previous
        if previous is not None: changes+=sum(sample[p]['st_ctime_ns']!=previous[p]['st_ctime_ns'] for p in set(sample)&set(previous))
        now=time.monotonic()
        if equal: streak+=1
        else: streak=1; since=now
        record={'rows':sample,'namesChanged':names_changed,'eightOrShaChanged':sorted(bad),'elapsedMs':round((now-began)*1000),'stableSamples':streak,'ctimeOnlyChanges':changes,'validationFailure':None if first_error is None else str(first_error)[:1500]}
        records.append(record)
        if first_error is None and any(names_changed.values()): first_error=ValueError('prospective names changed')
        if first_error is None and bad: first_error=ValueError('prospective eight fields/SHA changed')
        if first_error is None and now-began>30: first_error=ValueError('prospective admission30s deadline')
        try: observation_log(label,sample_number,record)
        except BaseException as publication_error:
            if first_error is None: raise
            diagnostics[label]['observationPublicationFailure']=str(publication_error)[:1500]
        if first_error is not None: raise first_error
        need(time.monotonic()-began<=30, 'prospective admission30s includes durable observations')
        if streak>=3 and now-since>=2:
            diagnostics[label]['admitted']=True
            return sample
        previous=sample; time.sleep(1.05)
    raise ValueError('at most30 prospective observations')

def portable_source(rows, manifest):
    need(len(rows)==69 and sum(r['kind']=='directory' for r in rows.values())==11, 'exact69-entry public fixture')
    children={p:[] for p,r in rows.items() if r['kind']=='directory'}
    for p in rows:
        if not p: continue
        parent=p.rpartition('/')[0]; need(parent in children, 'explicit directory parent'); children[parent].append(p)
    for values in children.values(): values.sort(key=lambda p:p.rsplit('/',1)[-1])
    need(set(children[''])=={'alpha','beta','profile.json','manifest.json'}, 'exact four fixture top-level names')
    need(pin(raw_manifest)==MF_PIN and rows['manifest.json']['sha256']==MF_PIN['sha256'] and rows['manifest.json']['st_size']==MF_PIN['bytes'], 'source manifest original bytes')
    need({'sha256':rows['profile.json']['sha256'],'bytes':rows['profile.json']['st_size']}==manifest['profile'], 'committed profile bytes')
    need(manifest['format']=='nano-physical-authenticated-public-cohort-v1' and manifest['syntheticPublicFixture']is True and manifest['newTestOnlySignaturesCreated']==14, 'public fixture manifest')
    cohorts=manifest['cohorts']; need(type(cohorts)is list and len(cohorts)==2 and [c['name'] for c in cohorts]==['alpha','beta'] and [c['children'] for c in cohorts]==[2,3], 'fixed two cohorts')
    result={}
    for cohort in cohorts:
        prefix=cohort['name']; roles={'primary','witness','signer','signer-witness'}
        need({p.rsplit('/',1)[-1] for p in children[prefix]}==roles and set(cohort['physical'])==roles, 'four committed roles')
        for role in sorted(roles):
            root=prefix+'/'+role; need(root in children, 'role directory'); portable=[['','directory']]; total=0
            def visit(parent,relative,depth):
                nonlocal total
                guard(); need(depth<=128, 'portable DFS depth')
                for child in children[parent]:
                    leaf=child.rsplit('/',1)[-1]; rel=relative+'/'+leaf if relative else leaf; row=rows[child]
                    if row['kind']=='directory': portable.append([rel,'directory']); visit(child,rel,depth+1)
                    else: total+=row['st_size']; portable.append([rel,'file',row['st_size'],row['sha256']])
            visit(root,'',0)
            raw=json.dumps(portable,ensure_ascii=False,separators=(',',':'),allow_nan=False).encode()
            description={'sha256':hashlib.sha256(raw).hexdigest(),'entries':len(portable),'bytes':total}
            need(description==cohort['physical'][role], 'C++ portable inventory: '+root); result[root]=description
    return result

def write_artifact(name, raw):
    path=OUT/name; fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o400)
    with os.fdopen(fd,'wb')as stream:
        need(stream.write(raw)==len(raw), 'complete new artifact'); stream.flush(); os.fsync(stream.fileno())
        created=os.fstat(stream.fileno()); row={k:getattr(created,k) for k in F}
    parent=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW|os.O_CLOEXEC)
    try: os.fsync(parent)
    finally: os.close(parent)
    creators[str(path)]=dict(row,sha256=pin(raw)['sha256'])
    return pin(raw)

def json_artifact(name, value):
    return write_artifact(name,(json.dumps(value,sort_keys=True,indent=2,allow_nan=False)+'\n').encode())

def archive():
    source_fence()
    path=OUT/'cohort.tar'; fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o400); h=hashlib.sha256(); size=0
    with os.fdopen(fd,'wb')as stream:
        def put(raw):
            nonlocal size
            guard(); size+=len(raw); need(size<=MAX_BYTES,'raw archive128MiB bound')
            need(stream.write(raw)==len(raw),'complete archive write'); h.update(raw)
        for relative,row in sorted(source_admitted.items()):
            guard(); directory=row['kind']=='directory'; name='public-cohort'+('/'+relative if relative else '')
            info=tarfile.TarInfo(name+('/' if directory else '')); info.type=tarfile.DIRTYPE if directory else tarfile.REGTYPE
            info.uid=info.gid=1000; info.mode=0o700 if directory else 0o600; info.mtime=row['st_mtime_ns']//1000000000; info.size=0 if directory else row['st_size']
            put(info.tobuf(format=tarfile.USTAR_FORMAT,encoding='ascii'))
            if not directory:
                actual,raw=common.material(TREE/relative); need(dict(actual,kind='file')==row,'admitted source file unchanged before copy')
                put(raw); put(bytes((-len(raw))%512))
        put(bytes(1024)); put(bytes((-size)%10240)); stream.flush(); os.fsync(stream.fileno()); created=os.fstat(stream.fileno()); row={k:getattr(created,k) for k in F}
    parent=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW|os.O_CLOEXEC)
    try: os.fsync(parent)
    finally: os.close(parent)
    creators[str(path)]=dict(row,sha256=h.hexdigest())
    return {'sha256':h.hexdigest(),'bytes':size}

def main():
    global common,owned,source_admitted,raw_manifest,adopted_outputs
    need(len(sys.argv)==6,'usage runner.py CONTROL SHA BYTES SELF_SHA SELF_BYTES')
    os.umask(0o077); need(sys.platform=='darwin' and os.geteuid()==501 and os.getegid()==20,'Native macOS501:20 exporter only')
    def expired(signum,frame): raise TimeoutError('export180s deadline/interruption')
    for sig in (signal.SIGALRM,signal.SIGINT,signal.SIGTERM,signal.SIGHUP): signal.signal(sig,expired)
    signal.alarm(180); sys.dont_write_bytecode=True
    retain(pathlib.Path(__file__).absolute(),{'sha256':sys.argv[4],'bytes':int(sys.argv[5])})
    need(stat.S_IMODE(held[str(pathlib.Path(__file__).absolute())]['st_mode'])==0o400,'exporter frozen before use')
    config_path=pathlib.Path(sys.argv[1]); config=decoded(retain(config_path,{'sha256':sys.argv[2],'bytes':int(sys.argv[3])}))
    need(type(config)is dict and set(config)=={'format','ready','sourceTree','outputRoot','common','proofs','manifestPin','historicalSourceFD9Qualified','preserveOriginalHostWholeFailure'},'closed control schema')
    need(config['format']=='nano-physical-native-export-control-v1' and config['ready']is True and config['sourceTree']==str(TREE) and config['outputRoot']==str(OUT) and config['manifestPin']==MF_PIN and config['historicalSourceFD9Qualified']is False and config['preserveOriginalHostWholeFailure']is True,'explicit fresh-export authority, no old refresh')
    source=original_proof(config['common'],COMMON,COMMON_PIN); common=types.ModuleType('native_export_common_verified_bytes'); common.__file__=str(COMMON); exec(compile(source,str(COMMON),'exec'),common.__dict__)
    need(set(config['proofs'])==set(PROOFS),'exact three public proof sources')
    proof_raw={k:original_proof(config['proofs'][k],p,w) for k,(p,w) in PROOFS.items()}
    proof={k:common.json_body(raw) for k,raw in proof_raw.items()}; records={}
    for key,mode in (('producerTerminal','produce'),('checkerTerminal','check')):
        terminal=proof[key]; need(terminal['exitCode']==0 and terminal['groupClosed']is True and terminal['forcedCleanup']is False and terminal['firstFailure']is None and not terminal['secondaryFailures'],'original CLI0 and joined group')
        suffix='cohort-'+mode; stdout_path=N/(suffix+'.stdout.jsonl'); stderr_path=N/(suffix+'.stderr.log')
        for path,field in ((stdout_path,'stdout'),(stderr_path,'stderr')):
            row=terminal[field]; need(type(row)is dict and set(row)==set(F)|{'sha256'},'original terminal material row')
            raw=retain(path,{'sha256':row['sha256'],'bytes':row['st_size']})
            if field=='stdout': parsed=[common.json_body(line) for line in raw.splitlines()]
            else: need(raw==b'','original CLI stderr empty')
        need(len(parsed)==3 and parsed==terminal['rows'],'exact three original CLI records')
        for row in parsed: need(row['mode']==mode and row['result']=='PASS','original successful typed fixture result')
        summary=parsed[-1]; need(summary['scenario']=='physical-authenticated-cohort-summary' and summary['manifestSha256']==MF_PIN['sha256'] and summary['cohorts']==2 and summary['databases']==8 and summary['archivesAuthenticated']==5 and summary['independentReadonlyPasses']==2 and summary['newTestOnlySignaturesCreated']==(14 if mode=='produce'else 0) and summary['newSignaturesInCheck']==0 and summary['productionQualified']is False and summary['runtimeDutiesAuthorized']is False,'original bounded public fixture facts')
        records[mode]=parsed
    need([{k:v for k,v in row.items() if k!='mode'} for row in records['produce'][:2]]==[{k:v for k,v in row.items() if k!='mode'} for row in records['check'][:2]],'original producer/checker same observations')
    host=proof['nativeHostResult']; need(host['passed']is False and host['cases']['cohort']['passed']is True and host['cases']['cohort']['producer']==records['produce'] and host['cases']['cohort']['checker']==records['check'],'Native04 whole FAILED preserved, CLI facts only')
    _,raw_manifest=common.material(TREE/'manifest.json'); need(pin(raw_manifest)==MF_PIN,'independently committed manifest bytes'); manifest=common.json_body(raw_manifest)
    need(not os.path.lexists(OUT),'fresh output namespace'); OUT.mkdir(mode=0o700); owned=True
    source_admitted=freeze_observation('source',lambda:common.snapshot(TREE,(501,20)),validate=lambda rows:portable_source(rows,manifest))
    source_fence(); roles=portable_source(source_admitted,manifest)
    source_pin=json_artifact('source-current-observation-public.json',{'sourceTree':str(TREE),'rows':source_admitted,'portableRoleInventories':roles,'admission':diagnostics['source'],'historicalSourceFD9Qualified':False,'currentObservationForThisExportOnly':True,'native04HostWholeFailedPreserved':True,'productionQualified':False})
    archive_pin=archive(); source_fence()
    inventory=common.validate_tar(OUT/'cohort.tar',raw_manifest,min(START+180,time.monotonic()+60),MF_PIN,(1000,1000)); need(inventory['entryCount']==69 and inventory['portableRoleInventories']==roles,'USTAR69/eight portable inventories')
    inventory_pin=json_artifact('export-inventory-public.json',inventory); write_artifact('cohort-manifest-public.json',raw_manifest)
    commitment_pin=json_artifact('export-external-commitment-public.json',{'format':'nano-native-cohort-export-commitment-v1','archivePin':archive_pin,'manifestPin':MF_PIN,'inventoryPin':inventory_pin,'sourceCurrentObservationPin':source_pin,'sourceProofPins':{k:w for k,(_,w) in PROOFS.items()},'outsideCohort':True,'historicalSourceFD9Qualified':False,'native04HostWholeFailedPreserved':True,'productionQualified':False})
    adopted_outputs=freeze_observation('outputs',lambda:{p:material(p,MAX_BYTES)[0] for p in creators},anchor=creators,peer=source_fence)
    source_fence(); fence_inputs()
    for path,row in adopted_outputs.items(): need(material(path,MAX_BYTES)[0]==row,'admitted new carrier final FD9/SHA unchanged')
    result={'format':'nano-physical-native-export-result-v1','passed':True,'sourceArchitecture':'macOS-ARM64','archivePin':archive_pin,'manifestPin':MF_PIN,'inventoryPin':inventory_pin,'commitmentPin':commitment_pin,'sourceEntries':69,'portableRoleInventories':8,'historicalSourceFD9Qualified':False,'historicalProofLogFD9Qualified':False,'sourceCurrentFD9ShaUnchanged':True,'native04HostWholeFailedPreserved':True,'oldHostResultsRequalified':False,'cppInvocations':0,'newSignatures':0,'databaseApiOpens':0,'imagesBuilt':0,'productionQualified':False,'outputAdmission':{k:v for k,v in diagnostics['outputs'].items() if k!='samples'},'newCarriers':adopted_outputs}
    raw=(json.dumps(result,sort_keys=True,indent=2,allow_nan=False)+'\n').encode()
    path=OUT/'result-public.json'; fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o400)
    with os.fdopen(fd,'wb')as stream: stream.write(raw); stream.flush(); os.fsync(stream.fileno())
    print(json.dumps({'passed':True,'resultPin':pin(raw),'archivePin':archive_pin,'historicalSourceFD9Qualified':False,'productionQualified':False}),flush=True)

if __name__=='__main__':
    try: main()
    except BaseException as exc:
        closing={}
        for label,action in (('inputs',fence_inputs),('source',source_fence if source_admitted is not None else None),('admittedOutputs',lambda: [need(material(p,MAX_BYTES)[0]==row,'admitted output changed on failure') for p,row in adopted_outputs.items()])):
            if action is None: closing[label]={'attempted':False}; continue
            try: action(); closing[label]={'attempted':True,'passed':True}
            except BaseException as secondary: closing[label]={'attempted':True,'passed':False,'reason':str(secondary)[:1500]}
        failure={'format':'nano-physical-native-export-refusal-v1','passed':False,'reason':str(exc)[:1500],'admissions':diagnostics,'closingFences':closing,'historicalSourceFD9Qualified':False,'native04HostWholeFailedPreserved':True,'productionQualified':False}
        if owned:
            path=OUT/'failure-public.json'
            try:
                fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o400)
                with os.fdopen(fd,'w')as stream: json.dump(failure,stream,sort_keys=True); stream.write('\n'); stream.flush(); os.fsync(stream.fileno())
            except BaseException: pass
        print(json.dumps({'passed':False,'reason':str(exc)[:1500],'productionQualified':False}),file=sys.stderr,flush=True)
        raise SystemExit(1)
