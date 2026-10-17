"""Root-operated selected software join; SOURCE ONLY until external control is ready.

No Docker, SSH, compiler, software smoke, database, seed or signing operation.
audit-amd runs on the VPS; emit runs on the Mac. Neither transfers anything.
"""
from pathlib import Path
import argparse, calendar, datetime, hashlib, json, os, re, stat, subprocess, sys, time

F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
RUNTIME = '1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352'
P2P_REV = '00154f57d4b6e1515d60edeae4e0ab38c42da28a'
LEAVES = {'core':'libnano_core.a','nano':'nano','p2p':'nano-p2p','exporter':'nano-capella-transition-tests',
          'bootstrapImport':'nano-capella-isolated-bootstrap-import','bootstrapAudit':'nano-capella-isolated-bootstrap-audit',
          'stoppedCapture':'nano-capella-isolated-stopped-capture','lifecycleTool':'nano-capella-deposit-audit',
          'lifecycleSubmit':'nano-capella-lifecycle-submit'}
ROLES = {'mac-native':('nano','exporter','bootstrapImport','bootstrapAudit','stoppedCapture','lifecycleTool','lifecycleSubmit'),
         'mac-docker':('nano','bootstrapImport','bootstrapAudit','stoppedCapture','lifecycleTool','lifecycleSubmit'),
         'vps-docker':('nano','bootstrapAudit','stoppedCapture','lifecycleTool','lifecycleSubmit')}
IMAGES = {'mac-docker':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb',
          'vps-docker':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'}
CORES = {'mac-native':{'sha256':'3401bf777c09f98b2ce4c035c336176dcdbe3097926c232f524670cff697fc37','bytes':29252992},
         'mac-docker':{'sha256':'36f77c8b5989d376932a2254c39349d293b8f111341163a3498b26286170993e','bytes':41045640},
         'vps-docker':{'sha256':'a3fdf87a71e05264058fb8bbb7894082330ef6042dd23f68007cf9d782ae2c48','bytes':40811064}}
P2PS = {'mac-native':{'sha256':'0db7d0ec5c83bb8239e84c3a54b13401f560d9919fc3ad8a5f10b7bc55d738fa','bytes':13406904},
        'mac-docker':{'sha256':'39efdc986056342b68226120eb48f3a2f93cc14324959929a257f2f1b9946ccf','bytes':19201936},
        'vps-docker':{'sha256':'99850b2c393d43d479505446bde9569013ea33b4693b86ad9695276a7380669a','bytes':18969392}}

def need(ok, label):
    if not ok: raise ValueError(label)

def pin(body): return {'sha256':hashlib.sha256(body).hexdigest(),'bytes':len(body)}

def known(value):
    need(isinstance(value,dict) and re.fullmatch('[0-9a-f]{64}',value.get('sha256','')) is not None
         and type(value.get('bytes')) is int and 0 < value['bytes'] <= 134217728, 'pin-shape')
    return {k:value[k] for k in ('sha256','bytes')}

def absolute(value):
    need(isinstance(value,str) and 1 < len(value) < 4096 and value.startswith('/')
         and not any(c in value for c in '\x00\r\n\\,') and os.path.normpath(value)==value, 'absolute-path')
    return Path(value)

def exact_row(row):
    need(isinstance(row,dict) and set(row)==set(F)|{'sha256'} and all(type(row[k]) is int for k in F)
         and re.fullmatch('[0-9a-f]{64}',row['sha256']) is not None, 'python-nine-row')
    return row

def material(value):
    path=absolute(str(value));need(path.resolve()==path,'canonical-source-path')
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        s=os.fstat(fd);need(stat.S_ISREG(s.st_mode) and s.st_nlink==1 and 0<=s.st_size<=134217728,'regular-bounded-file')
        row={k:getattr(s,k) for k in F};parts=[];n=0
        while True:
            b=os.read(fd,1048576)
            if not b:break
            n+=len(b);need(n<=s.st_size,'read-size-growth');parts.append(b)
        body=b''.join(parts)
        need(n==s.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}
             =={k:getattr(path.lstat(),k) for k in F},'fd-path-nine-changed')
        return dict(row,sha256=pin(body)['sha256']),body
    finally:os.close(fd)

def ns(text):
    m=re.fullmatch(r'(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})[.](\d{9}) ([+-])(\d{2})(\d{2})',text)
    need(m is not None,'gnu-timestamp');need(int(m[9])<=23 and int(m[10])<=59,'gnu-zone')
    zone=(int(m[9])*60+int(m[10]))*60
    if m[8]=='-':zone=-zone
    d=datetime.datetime(*map(int,m.groups()[:6]))
    return (calendar.timegm(d.timetuple())-zone)*1000000000+int(m[7])

def gnu_host_row(value):
    # This conversion is valid only for a Linux filesystem directly observed on
    # that same VPS. Mac-bind guestStatSha is never converted into hostFD9.
    need(isinstance(value,dict) and set(value)=={'material','bytes','sha256'},'gnu-row-shape')
    r=value['material'];need(isinstance(r,list) and len(r)==9 and all(isinstance(x,str) for x in r),'gnu-nine-strings')
    need(all(re.fullmatch('[0-9]+',r[i]) for i in (0,1,3,4,5,6)) and re.fullmatch('[0-9a-f]+',r[2]),'gnu-integers')
    values=[int(r[0]),int(r[1]),int(r[2],16),int(r[3]),int(r[4]),int(r[5]),int(r[6]),ns(r[7]),ns(r[8])]
    need(values[6]==known(value)['bytes'],'gnu-byte-join')
    return exact_row(dict(zip(F,values),sha256=value['sha256']))

class Session:
    def __init__(self):
        self.held={};self.out=None;self.new=[];self.observations=[];self.deadline=time.monotonic()+240

    def check_time(self):need(time.monotonic()<self.deadline,'operation-deadline')

    def hold(self,path,row):
        key=str(path);need(key not in self.held or self.held[key]==row,'old-authority-changed')
        self.held.setdefault(key,row)

    def load(self,a):
        self.check_time();need(isinstance(a,dict) and set(a)=={'path','sha256','bytes'},'artifact-shape')
        p=absolute(a['path']);row,body=material(p);need(pin(body)==known(a),'artifact-bytes')
        self.hold(p,row);return json.loads(body)

    def load_text(self,a):
        need(set(a)=={'path','sha256','bytes'},'source-artifact');row,body=material(a['path'])
        need(pin(body)==known(a),'pure-source-pin');self.hold(a['path'],row)

    def select(self,path,old,p,owner,executable):
        self.check_time();path=absolute(str(path));old=exact_row(old);row,body=material(path)
        uid,gid=(1000,1000) if owner=='vps-docker' else (501,20)
        need(row==old and pin(body)==known(p),'selected-original-nine-sha')
        need(row['st_uid']==uid and row['st_gid']==gid and not row['st_mode']&0o022
             and (not executable or bool(row['st_mode']&0o100)),'selected-owner-mode')
        self.hold(path,row);return dict(path=str(path),**known(p))

    def fence(self):
        self.check_time()
        for p,row in self.held.items():need(material(p)[0]==row,'selected-final-fence')

    def stable(self,artifacts,label):
        # Only never-held NEW metadata may reset before its first admission.
        need(all(a['path'] not in self.held for a in artifacts),'new-only-admission')
        expected={a['path']:known(a) for a in artifacts};start=time.monotonic();since=start;last=None;count=0
        observations=[]
        try:
            while True:
                self.check_time();rows={p:material(p)[0] for p in expected};now=time.monotonic()
                need(now-start<30,'new-metadata-stability-timeout')
                for p,row in rows.items():need({'sha256':row['sha256'],'bytes':row['st_size']}==expected[p],'new-known-bytes')
                if rows==last:count+=1
                else:since=now;count=1
                observations.append({'elapsedMs':int((now-start)*1000),'files':rows})
                if count>=3 and now-since>=2:break
                last=rows;time.sleep(1)
            for p,row in rows.items():self.hold(p,row)
        finally:self.observations.append({'label':label,'observations':observations})

    def create_output(self,value):
        p=absolute(value);need(p.resolve()==p and not p.exists(),'new-output-root')
        need(p.name.startswith('reactor-runtime-') and p.parent.is_dir(),'output-namespace')
        p.mkdir(mode=0o700);self.out=p;self.sync_dir(p.parent)

    @staticmethod
    def sync_dir(p):
        fd=os.open(p,os.O_RDONLY|os.O_DIRECTORY)
        try:os.fsync(fd)
        finally:os.close(fd)

    def write(self,leaf,value,admit=True):
        need(self.out is not None and re.fullmatch('[a-zA-Z0-9_.-]+',leaf),'output-leaf')
        body=(json.dumps(value,sort_keys=True,indent=2)+'\n').encode();need(len(body)<=134217728,'output-size')
        path=self.out/leaf
        with path.open('xb') as f:f.write(body);f.flush();os.fsync(f.fileno())
        path.chmod(0o400);self.sync_dir(self.out)
        artifact=dict(path=str(path),**pin(body))
        if admit:self.new.append(artifact)
        return artifact

def fields(value,expected):
    for k,v in expected.items():need(k in value and type(value[k]) is type(v) and value[k]==v,'result-'+k)

def completion(s,descriptor):
    need(set(descriptor)=={'artifact','checks'} and descriptor['checks'].get('passed') is True,'terminal-pass-control')
    obj=s.load(descriptor['artifact']);fields(obj,descriptor['checks'])
    if 'firstFailurePreserved' in obj:need(obj['firstFailurePreserved'] is False,'terminal-first-failure')
    if 'oldAuthoritiesRefreshed' in obj:need(obj['oldAuthoritiesRefreshed'] is False,'terminal-old-refresh')

def runtime_result(s,d,owner):
    r=s.load(d['result']);completion(s,d['completion'])
    fields(r,{'passed':True,'productionQualified':False,'oldAuthoritiesRefreshed':False,'compiledTranslationUnits':0})
    if owner=='mac-native':
        fields(r,{'format':'nano-native-reactor-runtime-relink-qualification-v1','sourceRevision':RUNTIME,
                  'coreMembers':143,'unchangedCoreMembers':142,'authenticatedObjectReuses':7,'independentMCommands':7,
                  'links':7,'cliSmokes':8,'coreArchiveUpdates':0,'p2pCommands':0,'runtimeStarted':False})
        need(known(r['core'])==CORES[owner],'native-core-pin')
    else:
        count=len(ROLES[owner]);fields(r,{'format':'nano-current-linux-runtime-relink-qualification-v1','owner':owner,
          'sourceRevisionDeclared':RUNTIME,'imageId':IMAGES[owner],'selectedPrograms':count,'independentMCommands':count,
          'linkCommands':count,'loaderCommands':count,'argumentRefusals':count,'smokeCommands':count+1,
          'helpCommands':1,'archiveUpdates':0,'coreCompilations':0,'p2pMCommands':0,'p2pLinks':0,'p2pProgramRuns':0})
        need({'sha256':r['coreSha256'],'bytes':r['coreBytes']}==CORES[owner],'linux-core-pin')
        if owner=='mac-docker':fields(r,{'rootOriginalHostRowsUnchanged':True,'rootOldVolumeGuestBeforeAfterEqual':True,
          'rootNewCoreHostOriginalNineAndShaPreserved':True,'rootImageIdentityVerified':True,'rootRelinkOnly':True})
        else:fields(r,{'rootSelectedOriginalRowsFullNineAndShaUnchanged':True,'rootOwnedContainerAbsent':True,
          'rootCurrentCoreProducerAndHostRowJoined':True})
    return r

def p2p(s,d,owner):
    r=s.load(d['result']);completion(s,d['completion'])
    fields(r,{'passed':True,'mode':'candidate','actualMmdMatchesM':True})
    native=owner=='mac-native'
    if native:fields(r,{'sourceAndSelectedDependencyInputsUnchanged':True})
    else:fields(r,{'hostSourcesFD9AndShaUnchanged':True,'guestSelectedStatShaUnchanged':True,'guestFdIdentityVerified':False})
    need(known(r['products']['nano-p2p'])==P2PS[owner],'p2p-known-product')
    rows=s.load(d['products']);path=absolute(d['softwarePath']);need(path.name==LEAVES['p2p'],'p2p-leaf')
    old=rows['nano-p2p'] if native else rows['products']['nano-p2p']['hostFD9']
    return s.select(path,old,P2PS[owner],owner,True)

def local_owner(s,d,owner):
    r=runtime_result(s,d,owner);binding=s.load(d['binding'])
    producerpin=known(r['softwareAdmission']) if owner=='mac-native' else {'sha256':r['softwareAdmissionSha256'],'bytes':r['softwareAdmissionBytes']}
    need(known(d['binding'])==producerpin,'runtime-binding-result-join')
    files={}
    if owner=='mac-native':
        fields(binding,{'format':'nano-native-reactor-runtime-relink-software-v1','sourceRevision':RUNTIME,'oldAuthoritiesRefreshed':False})
        need(binding['programs']==r['programs'] and set(r['programs'])==set(ROLES[owner]),'native-programs')
        for role,a in r['programs'].items():
            need(Path(a['path']).name==LEAVES[role],'native-leaf')
            files[role]=s.select(a['path'],binding['products'][a['path']],a,owner,True)
        a=binding['core'];need(known(a)==CORES[owner],'native-core-binding')
        files['core']=s.select(a['path'],binding['files'][a['path']],a,owner,False)
    else:
        fields(binding,{'format':'nano-current-linux-runtime-relink-software-binding-v1','owner':owner,'sourceRevisionDeclared':RUNTIME})
        snapshot=s.load(d['hostSnapshot']);need(known(d['hostSnapshot'])==known(r['rootHeldAfterWorkerSnapshot']),'post-worker-snapshot-pin')
        root=absolute(d['coreBuildRoot']);public={p['role']:p for p in r['programs']}
        private={p['role']:p for p in binding['programs']};need(set(public)==set(private)==set(ROLES[owner]),'arm-programs')
        for role,p in public.items():
            need(p['target']==LEAVES[role] and known(private[role]['software'])=={'sha256':p['softwareSha256'],'bytes':p['softwareBytes']},'arm-program-join')
            path=root/LEAVES[role];files[role]=s.select(path,snapshot[str(path)],private[role]['software'],owner,True)
        path=root/LEAVES['core'];need(known(binding['coreArchive'])==CORES[owner],'arm-core-binding')
        files['core']=s.select(path,snapshot[str(path)],CORES[owner],owner,False)
    files['p2p']=p2p(s,d['p2p'],owner);return files

def audit_amd(s,c):
    need(sys.platform=='linux' and os.getuid()==os.getgid()==1000,'audit-vps-identity')
    d=c['amd'];r=runtime_result(s,d,'vps-docker');b=s.load(d['binding'])
    need(known(d['binding'])=={'sha256':r['softwareAdmissionSha256'],'bytes':r['softwareAdmissionBytes']},'amd-binding-result-join')
    fields(b,{'format':'nano-current-linux-runtime-relink-software-binding-v1','owner':'vps-docker','sourceRevisionDeclared':RUNTIME,'oldAuthoritiesRefreshed':False})
    root=absolute(d['root']);public={p['role']:p for p in r['programs']};private={p['role']:p for p in b['programs']}
    need(set(public)==set(private)==set(ROLES['vps-docker']),'amd-programs')
    files={}
    for role in ROLES['vps-docker']:
        q=private[role];p=public[role];need(q['target']==p['target']==LEAVES[role] and q['softwarePath']=='/outputs/'+LEAVES[role],'amd-role-path')
        need(known(q['software'])=={'sha256':p['softwareSha256'],'bytes':p['softwareBytes']},'amd-public-binding-join')
        path=root/'outputs'/LEAVES[role];files[role]=s.select(path,gnu_host_row(q['software']),q['software'],'vps-docker',True)
    need(known(b['coreArchive'])==CORES['vps-docker'],'amd-core-binding')
    path=root/'baseline'/LEAVES['core'];files['core']=s.select(path,gnu_host_row(b['coreArchive']),CORES['vps-docker'],'vps-docker',False)
    files['p2p']=p2p(s,d['p2p'],'vps-docker');s.fence()
    result={'format':'nano-reactor-amd-selected-code-fence-v1','passed':True,'sourceRevision':RUNTIME,'p2pCommittedRevision':P2P_REV,
      'programs':{k:v for k,v in files.items() if k!='core'},'coreArchive':files['core'],
      'runtimeResult':known(d['result']),'runtimeBinding':known(d['binding']),'runtimeCompletion':known(d['completion']['artifact']),
      'p2pResult':known(d['p2p']['result']),'p2pProducts':known(d['p2p']['products']),'p2pCompletion':known(d['p2p']['completion']['artifact']),
      'originalNineAndShaMatched':True,'selectedSoftwareFiles':7,'privateProducerTablesRead':2,
      'guestStatShaComparedAsHostFD9':False,'oldAuthoritiesRefreshed':False,'programsExecuted':0,'sdkFilesRead':0,
      'newConsensusKeys':0,'consensusSigningRequested':False,'databaseOpened':False,'runtimeStarted':False,'productionQualified':False}
    s.create_output(c['output']);return result

def tool(s,a):
    need(set(a)=={'path','sha256','bytes','material'},'tool-control')
    row,body=material(a['path']);need(row==exact_row(a['material']) and pin(body)==known(a),'old-tool-original-nine')
    s.hold(a['path'],row);return str(absolute(a['path']))

def emit(s,c):
    need(sys.platform=='darwin' and os.getuid()==501 and os.getgid()==20,'emit-mac-identity')
    repo=absolute(c['repo']);gitbin=tool(s,c['tools']['git']);node=tool(s,c['tools']['node'])
    need(not absolute(c['output']).is_relative_to(repo),'output-outside-repository')
    def git(*args):
        s.check_time();q=subprocess.run([gitbin,'--no-optional-locks','-C',str(repo),*args],capture_output=True,timeout=20,
          env={'PATH':'/usr/bin:/bin','HOME':str(repo),'GIT_NO_LAZY_FETCH':'1','GIT_TERMINAL_PROMPT':'0'})
        need(q.returncode==0 and not q.stderr,'source-git');return q.stdout.decode().strip()
    rev=c['sourceBaseRevision'];need(re.fullmatch('[0-9a-f]{40}',rev) and git('rev-parse','HEAD')==rev
      and git('branch','--show-current')=='main' and git('status','--porcelain')=='','current-clean-source')
    trees=('src','include','network','third_party','tests/support','tests/capella')
    source_trees={x:git('rev-parse',rev+':'+x) for x in trees}
    need(all(source_trees[x]==git('rev-parse',RUNTIME+':'+x) for x in trees),'current-runtime-selected-source-trees')
    cmake=git('rev-parse',rev+':CMakeLists.txt');need(cmake==git('rev-parse',RUNTIME+':CMakeLists.txt'),'current-runtime-cmake')
    need(source_trees['network']==git('rev-parse',P2P_REV+':network')
      and git('rev-parse',rev+':third_party/json.hpp')==git('rev-parse',P2P_REV+':third_party/json.hpp'),'current-p2p-selected-source')
    # Pin the actual pure API source files, not the whole mutable harness tree.
    for leaf,a in c['pureModules'].items():
        need(leaf in ('tests/multihost/prepare.mjs','tests/multihost/program-locators.mjs') and a['path']==str(repo/leaf),'pure-module-path');s.load_text(a)
    need(set(c['pureModules'])=={'tests/multihost/prepare.mjs','tests/multihost/program-locators.mjs'},'pure-modules')
    actual={name:local_owner(s,c['owners'][name],name) for name in ('mac-native','mac-docker')}
    remote=s.load(c['amdAudit']);fields(remote,{'format':'nano-reactor-amd-selected-code-fence-v1','passed':True,
      'sourceRevision':RUNTIME,'p2pCommittedRevision':P2P_REV,'originalNineAndShaMatched':True,'selectedSoftwareFiles':7,
      'oldAuthoritiesRefreshed':False,'programsExecuted':0,'sdkFilesRead':0,'runtimeStarted':False,'productionQualified':False})
    for key in ('runtimeResult','runtimeBinding','runtimeCompletion','p2pResult','p2pProducts','p2pCompletion'):
        need(remote[key]==known(c['amdProducerPins'][key]),'remote-producer-pin-join')
    actual['vps-docker']=dict(remote['programs'],core=remote['coreArchive'])
    old=s.load(c['historicalIndex']);need(old['assetCount']==24 and len(old['assets'])==24,'historical-index')
    oldmap={(a['owner'],a['role']):known(a) for a in old['assets']};need(len(oldmap)==24,'historical-index-unique')
    assets=[];owners={}
    for name,files in actual.items():
        need(set(files)==set(ROLES[name])|{'p2p','core'},'owner-exact-assets')
        need(known(files['core'])==CORES[name] and known(files['p2p'])==P2PS[name],'current-core-p2p')
        for role,a in files.items():
            need(absolute(a['path']).name==LEAVES[role],'software-leaf');known(a)
            assets.append({'owner':name,'role':role,**known(a),'effectiveCppSourceRevision':P2P_REV if role=='p2p' else RUNTIME,
              'changedFromHistoricalIndex':known(a)!=oldmap[(name,role)]})
        cfg=c['owners'][name];core_root=str(absolute(cfg['coreBuildRoot']));network_root=str(Path(files['p2p']['path']).parent)
        def artifact(role):
            a=dict(files[role])
            if name!='mac-native' and role!='core':a['path']=('/network-build/' if role=='p2p' else '/outputs/')+LEAVES[role]
            if name=='mac-docker' and role=='core':a['path']='/outputs/libnano_core.a'
            if name!='mac-native' and role not in ('p2p','core'):need(files[role]['path']==core_root+'/'+LEAVES[role],'linux-host-runtime-mapping')
            return a
        owners[name]={'coreBuildRoot':core_root,'networkBuildRoot':network_root,'coreArchive':artifact('core'),
                      'programs':{role:artifact(role) for role in files if role!='core'}}
    need(len(assets)==24,'twenty-four-assets');s.fence();s.create_output(c['output'])
    index=s.write('software-candidate-index.json',{'format':'nano-software-candidate-index-v1','assetCount':24,
      'ownerAssetCounts':{'mac-native':9,'mac-docker':8,'vps-docker':7},'sourceJoin':'source-join.json',
      'effectiveCppSourceRevision':RUNTIME,'p2pCommittedRevision':P2P_REV,'assets':assets,
      'historicalIndex':known(c['historicalIndex']),'historicalIndexRetargeted':False,'productionQualified':False})
    join=s.write('source-join.json',{'format':'nano-reactor-runtime-and-status-p2p-source-join-v1','sourceBaseRevision':rev,
      'qualifiedRuntimeBuildRevision':RUNTIME,'qualifiedP2pCommittedRevision':P2P_REV,'gitTrees':source_trees,
      'cmakeGitObject':cmake,'runtimeSelectedTreesAndCmakeExactTo1925':True,'p2pNetworkAndJsonExactTo00154':True,
      'runtimeCoreLineage':['68d','RPC-2095012b','reactor-1925eee'],'coreObjects':143,'replacedReactorMembers':1,
      'unchangedRpcCoreMembers':142,'oldSixTreesEqual68dClaimed':False,'wholeMultihostTreeQualified':False,
      'historicalIndexRetargeted':False,'wholeSdkQualified':False,'productionQualified':False})
    mf=s.write('source-manifest.public.json',{'format':'nano-root-reactor-runtime-source-manifest-v1','sourceBaseRevision':rev,
      'qualifiedRuntimeBuildRevision':RUNTIME,'qualifiedP2pCommittedRevision':P2P_REV,'softwareIndex':index,'sourceJoin':join,
      'runtimeResultPins':{name:known(c['owners'][name]['result']) for name in ('mac-native','mac-docker')},
      'amdSelectedAudit':known(c['amdAudit']),'softwareAssets':24,'historicalAdmissionsRetargeted':False,
      'campaignAuthorizedByThisManifest':False,'wholeSdkQualified':False,'productionQualified':False})
    for name,owner in owners.items():
        admission={'format':'nano-root-fresh-capella-runtime-assets-admission-v1','owner':name,'sourceBaseRevision':rev,
          'overlayManifestSha256':mf['sha256'],'coreObjects':143,'coreArchive':owner['coreArchive'],'programs':owner['programs'],
          'canonicalCmake':True,'currentCompilerMmdArchiveClosureVerified':True,
          'sdkAuthority':'resolved-file-pins' if name=='mac-native' else 'immutable-image',
          'actualLinksExact':True,'actualBuildExitCode':0,'productionQualified':False}
        need(len(admission)==13,'thirteen-fields');owner['admission']=s.write(name+'.json',admission)
    locators=s.write('software-locators.private.json',{'sourceBaseRevision':rev,'overlayManifest':mf,'owners':owners})
    s.stable(s.new,'new-index-join-manifest-admissions-before-api')
    js="""import{readFileSync}from'node:fs';const repo=process.argv[1];const{validateFreshOwnerAdmission}=await import('file://'+repo+'/tests/multihost/prepare.mjs');const{freshSoftwareLocators}=await import('file://'+repo+'/tests/multihost/program-locators.mjs');const s=JSON.parse(readFileSync(process.argv[2]));let assets=0;for(const[name,o]of Object.entries(s.owners)){validateFreshOwnerAdmission(JSON.parse(readFileSync(o.admission.path)),s,name);assets+=freshSoftwareLocators(name,o).length;}if(assets!==24)throw Error('Expected24');console.log(JSON.stringify({passed:true,owners:3,assets,campaignSpecValidated:false,runtimeStarted:false,productionQualified:false}));"""
    q=subprocess.run([node,'--input-type=module','-e',js,str(repo),locators['path']],capture_output=True,timeout=30,
      env={'PATH':'/usr/bin:/bin','HOME':str(s.out),'NODE_OPTIONS':'','NODE_PATH':''})
    s.write('schema-validator-stderr.private.json',{'exitCode':q.returncode,'stderrHex':q.stderr.hex()},False)
    need(q.returncode==0 and not q.stderr and len(q.stdout)<=4096,'pure-schema-command')
    validation=json.loads(q.stdout);fields(validation,{'passed':True,'owners':3,'assets':24,'campaignSpecValidated':False})
    validator=s.write('pure-validation-public.json',validation);s.stable([validator],'new-pure-result');s.fence()
    need(git('rev-parse','HEAD')==rev and git('status','--porcelain')=='','final-clean-source')
    return {'format':'nano-reactor-runtime-code-admissions-root-v1','passed':True,'sourceBaseRevision':rev,
      'sourceManifest':mf,'softwareIndex':index,'sourceJoin':join,'newAdmissions':{name:o['admission'] for name,o in owners.items()},
      'pureValidation':validator,'assets':24,'owners':3,'selectedLocalSoftwareFiles':17,'selectedRemoteSoftwareFiles':7,
      'originalSoftwareNineAndShaPreserved':True,'oldAuthoritiesRefreshed':False,'historicalAdmissionsRetargeted':False,
      'programCopies':0,'programRuns':0,'compilerCommands':0,'pureSchemaValidatorCommands':1,'fullCampaignSpecValidated':False,
      'newCampaignAuthorized':False,'newConsensusKeys':0,'consensusSigningRequested':False,'databaseOpened':False,
      'runtimeStarted':False,'dutiesResumed':False,'productionQualified':False}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('mode',choices=('audit-amd','emit'));ap.add_argument('control');ap.add_argument('sha256');ap.add_argument('bytes',type=int);a=ap.parse_args()
    os.umask(0o077);s=Session();first=None;result=None
    try:
        control=dict(path=str(absolute(a.control)),sha256=a.sha256,bytes=a.bytes);known(control)
        row,body=material(control['path']);need(pin(body)==known(control),'control-known-bytes');c=json.loads(body)
        need(c['format']=='nano-reactor-runtime-code-admission-control-v1' and c['ready'] is True and c['mode']==a.mode,'ready-control-required')
        s.stable([control],'new-control-before-first-hold')
        worker=c['worker'];need(worker['path']==str(Path(__file__).resolve()),'worker-self-path');tool(s,worker)
        result=audit_amd(s,c) if a.mode=='audit-amd' else emit(s,c)
    except BaseException as error:first=error
    finally:
        try:s.fence()
        except BaseException as error:
            if first is None:first=error
        if s.out is not None:
            for leaf,value in (('new-metadata-observations.private.json',s.observations),('selected-original-rows.private.json',s.held)):
                try:s.write(leaf,value,False)
                except BaseException as error:
                    if first is None:first=error
            try:s.write('final-fence-public.json',{'passed':first is None,'firstFailurePreserved':first is not None,'oldAuthoritiesRefreshed':False},False)
            except BaseException as error:
                if first is None:first=error
    if first is not None:
        if s.out is not None:
            try:s.write('failure-public.json',{'passed':False,'errorType':type(first).__name__,'oldAuthoritiesRefreshed':False,'productionQualified':False},False)
            except BaseException:pass
        print(json.dumps({'passed':False,'errorType':type(first).__name__,'productionQualified':False}),flush=True);raise SystemExit(1)
    s.write('result-public.json',result,False)
    print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':main()
