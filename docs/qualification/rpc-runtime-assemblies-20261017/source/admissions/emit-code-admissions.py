"""Join completed software qualifications; emit code-only admissions, never a campaign."""
from pathlib import Path
import os,stat,json,hashlib,subprocess,time

E=Path(__file__).resolve().parent
R=Path('/Users/boris/Dev/blockchain-nano')
O=E/'rpc-runtime-code-admissions-root-20261017-01'
BUILD='d26e4744416d033dad5a2519cc2ab8899b838faf'
REV='7bd24deac1493d7fe4762513f2c32dcc7d97b9bb'
PUB='docs/qualification/rpc-runtime-assemblies-20261017'
PKG=E/'rpc-runtime-public-delivery-proposal-20261017/package'/PUB
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
held={}
os.umask(0o077)

def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def material(path):
    path=Path(path);assert path.is_absolute() and path.resolve()==path
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        s=os.fstat(fd);assert stat.S_ISREG(s.st_mode) and s.st_nlink==1 and 0<=s.st_size<=134217728
        row={k:getattr(s,k) for k in F};parts=[];total=0
        while True:
            b=os.read(fd,1048576)
            if not b:break
            total+=len(b);assert total<=s.st_size;parts.append(b)
        body=b''.join(parts)
        assert len(body)==s.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(path.lstat(),k) for k in F}
        return dict(row,sha256=hashlib.sha256(body).hexdigest()),body
    finally:os.close(fd)
def load(path,sha,size):
    row,body=material(path);assert pin(body)=={'sha256':sha,'bytes':size}
    assert str(path) not in held or held[str(path)]==row
    held[str(path)]=row;return json.loads(body)
def select(path,old,known):
    row,body=material(path);assert row==old and pin(body)==known and row['st_uid']==501
    assert str(path) not in held or held[str(path)]==row
    held[str(path)]=row;return dict(path=str(path),**known)
def git(*args):return subprocess.check_output(['/usr/bin/git','--no-optional-locks','-C',str(R),*args],timeout=30).decode().strip()
def write(leaf,value):
    p=O/leaf;b=(json.dumps(value,sort_keys=True,indent=2)+'\n').encode()
    with p.open('xb') as f:f.write(b);f.flush();os.fsync(f.fileno())
    p.chmod(0o400);return dict(path=str(p),**pin(b))
def fence():
    for p,row in held.items():assert material(Path(p))[0]==row

assert os.getuid()==501 and os.getgid()==20 and not O.exists()
assert git('rev-parse','HEAD')==REV and git('branch','--show-current')=='main' and git('status','--porcelain')==''
assert git('diff','--name-only',BUILD,REV)=='tests/test_rpc_simulation_context.cpp'
trees=['src','include','network','third_party','tests/support','tests/capella','tests/multihost']
for name in trees+['CMakeLists.txt']:assert git('rev-parse',REV+':'+name)==git('rev-parse',BUILD+':'+name)
assert pin((R/'tests/test_rpc_simulation_context.cpp').read_bytes())=={'sha256':'3aafab4910258d635ebc41d5290f5cda430a20d588ccf22e33b59f0f1de69073','bytes':18517}
nr=load(E/'rpc-runtime-native-root-qualification-20261017-03/result-public.json','13a65a8301affe923a4d1eef3dce3ea4b61121042becf69a9a94dfc542bca00a',5346)
ar=load(E/'current-arm-rpc-runtime-relink-root-20261017-01/result-public.json','87af971d33fa84cfa36f735a8eb5dcdd8306c1a105d41fec933465e0e0f5e911',9024)
dr=load(E/'rpc-runtime-amd-root-qualification-20261017-01/result-public.json','ab42235b015fd77c58d41e1d38160a667fbf52bb4ffd57d2addb4645fcd76681',7667)
remote=load(E/'rpc-runtime-amd-product-fence-root-20261017.stdout.json','15915e93b566a1e351043ed3a167b4b88df2e090a3165ce89f98d97c91af4a28',1836)
assert all(r['passed'] and r['productionQualified'] is False for r in (nr,ar,dr,remote))
assert nr['runtimeConsumerLinks']==7 and nr['newContextChecks']==476 and nr['newContextExpectedRefusals']==126 and nr['newContextCases']==258
assert ar['linkCommands']==6 and ar['rootImageIdentityVerified'] and ar['rootOldVolumeGuestBeforeAfterEqual']
assert dr['linkCommands']==5 and dr['rootOwnedContainerAbsent'] and dr['rootSelectedOriginalRowsFullNineAndShaUnchanged']
assert remote['originalNineAndShaMatched'] and remote['selectedSoftwareFiles']==7 and remote['programsExecuted']==0
index=load(PKG/'software-candidate-index.json','3ae2a03763bfd162c49c0b6f6c24a942d33bd92bddc7e26b22cc88317dd81a41',9598)
assert index['assetCount']==24 and len(index['assets'])==24
np=nr['softwareAdmission'];nb=load(E/'rpc-runtime-native-root-qualification-20261017-03/software-admission.private.json',np['sha256'],np['bytes'])
native={role:select(Path(a['path']),nb['products'][a['path']],{k:a[k] for k in ('sha256','bytes')}) for role,a in nr['programs'].items()}
ncore=Path(nb['newCore']['path']);nc=select(ncore,nb['products'][str(ncore)],nr['newCore'])
nlegacy=load(E/'current-native-runtime-bindings-root-attempt03-20261016/binding-inputs.private.json','7f9083e8bcab2c72237c3012bae9d858948f46b1947bc2dd2d88ac28afe10a89',2027120)
npa=nlegacy['programs']['p2p'];native['p2p']=select(Path(npa['path']),nlegacy['files'][npa['path']],{k:npa[k] for k in ('sha256','bytes')})
ap=ar['rootHeldAfterWorkerSnapshot'];ab=load(E/'current-arm-rpc-runtime-relink-root-20261017-01/selected-host-inputs-after-worker.private.json',ap['sha256'],ap['bytes'])
abroot=E/'current-arm-rpc-runtime-relink-root-20261017-01/runtime-programs'
arm={p['role']:select(abroot/p['target'],ab[str(abroot/p['target'])],{'sha256':p['softwareSha256'],'bytes':p['softwareBytes']}) for p in ar['programs']}
ac=select(abroot/'libnano_core.a',ab[str(abroot/'libnano_core.a')],{'sha256':ar['coreSha256'],'bytes':ar['coreBytes']})
alegacy=load(E/'current-arm-runtime-network-root-20261016/first-copy-admission.private.json','dccedc2df5f1ff21fef6f75501dfe2f48fe05cbb8c1818cd9f2436ba946a474d',1856)
apa=alegacy['program'];arm['p2p']=select(Path(apa['path']),alegacy['hostObservations'][-1]['material'],{k:apa[k] for k in ('sha256','bytes')})
actual={'mac-native':dict(native,core=nc),'mac-docker':dict(arm,core=ac),'vps-docker':dict(remote['programs'],core=remote['coreArchive'])}
assert len(actual['mac-native'])==9 and len(actual['mac-docker'])==8 and len(actual['vps-docker'])==7
for asset in index['assets']:
    a=actual[asset['owner']][asset['role']];assert {k:a[k] for k in ('sha256','bytes')}=={k:asset[k] for k in ('sha256','bytes')}
fence();O.mkdir(mode=0o700)
join={'format':'nano-current-rpc-runtime-source-join-v1','sourceBaseRevision':REV,'qualifiedRuntimeBuildRevision':BUILD,'effectiveProductionCppRevision':'2095012bb32234fecadeb024c5541ebaaa84470b','sourceTrees':{x:git('rev-parse',REV+':'+x) for x in trees},'cmake':pin((R/'CMakeLists.txt').read_bytes()),'runtimeTreesAndCmakeExactToCompletedBuilds':True,'deltaAfterCompletedBuilds':['tests/test_rpc_simulation_context.cpp'],'testIncludeOnly':True,'testQualifiedNativeSHA256':'3aafab4910258d635ebc41d5290f5cda430a20d588ccf22e33b59f0f1de69073','linuxTestTranslationUnitsRetainOriginalQualification':True,'globalSdkOrCmakeQualified':False,'productionQualified':False}
join_art=write('source-join.json',join)
manifest={'format':'nano-root-current-rpc-runtime-source-manifest-v3','sourceBaseRevision':REV,'effectiveCppSourceRevision':'2095012bb32234fecadeb024c5541ebaaa84470b','qualifiedRuntimeBuildRevision':BUILD,'gitTrees':join['sourceTrees'],'runtimeTreesAndCmakeExactToQualifiedBuilds':True,'sourceJoin':dict(path=str(R/PUB/'source-join.json'),**{k:join_art[k] for k in ('sha256','bytes')}),'softwareIndex':dict(path=str(R/PUB/'software-candidate-index.json'),**{k:pin((PKG/'software-candidate-index.json').read_bytes())[k] for k in ('sha256','bytes')}),'overlays':[],'softwareAssets':24,'historical24AdmissionsRetargeted':False,'priorManifestD30eRetargeted':False,'globalSdkOrCmakeQualified':False,'campaignAuthorizedByThisManifest':False,'productionQualified':False}
mf=write('source-manifest.public.json',manifest)
owners={}
for name in actual:
    files=actual[name];native_mode=name=='mac-native';arm_mode=name=='mac-docker'
    core_root=str(E/'rpc-runtime-native-root-qualification-20261017-03') if native_mode else str(abroot) if arm_mode else '/home/ubuntu/current-amd-rpc-runtime-relink-20261017-01/outputs'
    network_root=str(Path(files['p2p']['path']).parent)
    def artifact(role):
        a=dict(files[role])
        if not native_mode:
            if role=='p2p':a['path']='/network-build/nano-p2p'
            elif arm_mode:a['path']='/outputs/'+Path(a['path']).name
        return a
    owner={'coreBuildRoot':core_root,'networkBuildRoot':network_root,'coreArchive':artifact('core'),'programs':{role:artifact(role) for role in files if role!='core'}}
    admission={'format':'nano-root-fresh-capella-runtime-assets-admission-v1','owner':name,'sourceBaseRevision':REV,'overlayManifestSha256':mf['sha256'],'coreObjects':143,'coreArchive':owner['coreArchive'],'programs':owner['programs'],'canonicalCmake':True,'currentCompilerMmdArchiveClosureVerified':True,'sdkAuthority':'resolved-file-pins' if native_mode else 'immutable-image','actualLinksExact':True,'actualBuildExitCode':0,'productionQualified':False}
    owner['admission']=write(name+'.json',admission);owners[name]=owner
locators=write('software-locators.private.json',{'sourceBaseRevision':REV,'overlayManifest':mf,'owners':owners})
js="""import {readFileSync} from 'node:fs';import {validateFreshOwnerAdmission} from 'file:///Users/boris/Dev/blockchain-nano/tests/multihost/prepare.mjs';import {freshSoftwareLocators} from 'file:///Users/boris/Dev/blockchain-nano/tests/multihost/program-locators.mjs';const s=JSON.parse(readFileSync(process.argv[1]));let assets=0;for(const [name,o] of Object.entries(s.owners)){validateFreshOwnerAdmission(JSON.parse(readFileSync(o.admission.path)),s,name);assets+=freshSoftwareLocators(name,o).length;}if(assets!==24)throw Error('Expected24');console.log(JSON.stringify({passed:true,owners:3,assets,campaignSpecValidated:false,runtimeStarted:false,productionQualified:false}));"""
q=subprocess.run(['/opt/homebrew/bin/node','--input-type=module','-e',js,locators['path']],capture_output=True,timeout=30)
with (O/'schema-validator.stdout.json').open('xb') as f:f.write(q.stdout)
with (O/'schema-validator.stderr.private.log').open('xb') as f:f.write(q.stderr)
assert q.returncode==0 and not q.stderr and json.loads(q.stdout)['assets']==24
# Only newly written metadata files may settle before their first identity hold.
artifacts=[join_art,mf,locators,*[o['admission'] for o in owners.values()]]
new_paths=[Path(a['path']) for a in artifacts]
known={a['path']:{k:a[k] for k in ('sha256','bytes')} for a in artifacts};observations=[];previous=None;count=0;start=time.monotonic();since=start
while True:
    rows={str(p):material(p)[0] for p in new_paths};now=time.monotonic();assert now-start<30
    for p,row in rows.items():assert {'sha256':row['sha256'],'bytes':row['st_size']}==known[p]
    if rows==previous:count+=1
    else:count=1;since=now
    observations.append({'elapsedMs':int((now-start)*1000),'files':rows})
    if count>=3 and now-since>=2:break
    previous=rows;time.sleep(1)
for p,row in rows.items():assert p not in held;held[p]=row
fence();assert git('rev-parse','HEAD')==REV and git('status','--porcelain')==''
write('new-metadata-first-admission.private.json',observations)
write('selected-local-software-original-rows.private.json',held)
receipt={'format':'nano-rpc-runtime-code-admissions-root-v1','passed':True,'sourceBaseRevision':REV,'qualifiedRuntimeBuildRevision':BUILD,'sourceManifest':mf,'sourceJoin':join_art,'newAdmissions':{name:o['admission'] for name,o in owners.items()},'assets':24,'owners':3,'selectedLocalSoftwareFiles':17,'selectedRemoteSoftwareFiles':7,'originalSoftwareNineAndShaPreserved':True,'oldAuthoritiesRefreshed':False,'oldAdmissionsRetargeted':False,'programCopiesByThisEmitter':0,'pureSchemaValidatorCommands':1,'fullCampaignSpecValidated':False,'newCampaignAuthorized':False,'newKeys':0,'newSignatures':0,'privateDatabaseOpened':False,'runtimeStarted':False,'dutiesResumed':False,'commonFinalityQualified':False,'productionQualified':False}
write('result-public.json',receipt)
print(json.dumps({'passed':True,'owners':3,'assets':24,'sourceBaseRevision':REV,'sourceManifestSha256':mf['sha256'],'runtimeStarted':False,'productionQualified':False},sort_keys=True))
