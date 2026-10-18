"""Root prospective source/authority projection, no compiler, Docker or signing."""
from pathlib import Path
import hashlib,json,os,stat,subprocess,time
E=Path(__file__).resolve().parent;R=Path('/Users/boris/Dev/blockchain-nano')
G=Path('/Library/Developer/CommandLineTools/usr/bin/git')
HEAD='4ae774f16220d8a9362004b573d83f3c8ed78a97'
P=E/'capella-sync-adverse-source-proposal-v2-20261018'
H=E/'capella-sync-adverse-host-proposal-20261018/host.py'
C=E/'capella-sync-adverse-arm-control-root-20261018-02'
S=E/'capella-sync-adverse-public-source-root-20261018-02'
D=E/'capella-sync-adverse-docker-client-root-20261018-02'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def read(p,want=None):
 assert p.is_absolute() and p.resolve(strict=True)==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};assert stat.S_ISREG(a.st_mode)and a.st_nlink>=1 and a.st_size<=268435456
  b=b''
  while chunk:=os.read(fd,1048576):b+=chunk
  assert len(b)==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  row['sha256']=pin(b)['sha256'];assert want is None or row==want,(str(p),'original row changed')
  return b,row
 finally:os.close(fd)
def git(*args):return subprocess.check_output([str(G),'-c','core.fsmonitor=false','--no-optional-locks','-C',str(R),*args])
assert git('rev-parse','HEAD').decode().strip()==HEAD and not git('status','--porcelain')
oldp=E/'rpc-runtime-arm-control-root-20261017-02/control.private.json';b,row=read(oldp)
assert pin(b)=={'sha256':'42bfd4ea085d757235ab6366de9c838b753afb99bdf2066d0dcdfc13243bdd43','bytes':777097}
old=json.loads(b);oldrow=row
mb,mr=read(P/'manifest.json');mf=json.loads(mb)
assert mf['sourceBase']==HEAD and mf['productionCoreChanged']is False and mf['sourceFiles']==5
assert mf['status']=='FROZEN_SOURCE_ONLY_REQUIRE_ROOT_ADMISSION'
payload={}
for item in mf['payloads']:
 p=P/item['path'];data,r=read(p);assert pin(data)=={k:item[k]for k in('sha256','bytes')}and stat.S_IMODE(r['st_mode'])==0o400
 if item['path'].startswith('source/'):payload[item['path'][7:]]=data
assert len(payload)==5
for item in mf['baselines']:
 data,_=read(R/item['repositoryPath']);assert pin(data)=={k:item[k]for k in('sha256','bytes')}and git('show',HEAD+':'+item['repositoryPath'])==data
core={k:v for k,v in old['workerConfig']['projectPins'].items()if k.startswith(('/work/src/','/work/include/','/work/third_party/'))}
assert len(core)==989
runtime=['tests/fixtures/capella-bootstrap-anchor-cb532/'+x for x in('manifest.json','profile.json','genesis-state.ssz','genesis.json')]+['tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz']
paths={k.removeprefix('/work/')for k in core}|set(payload)|set(runtime)|{'tests/support/durable_owned_fixture.hpp','scripts/ci/operations.sh','scripts/ci/verify-results.sh'}
paths.update(x for x in git('ls-files','network').decode().splitlines()if x.endswith(('.hpp','.h')))
assert not any(p.exists()for p in(C,S,D));C.mkdir(mode=0o700);S.mkdir(mode=0o700);D.mkdir(mode=0o700)
originals={};project={};total=0
for rel in sorted(paths):
 data=payload.get(rel)
 if data is None:
  data,_=read(R/rel);assert git('show',HEAD+':'+rel)==data
 guest='/work/'+rel
 if guest in core:assert pin(data)==core[guest]
 dest=S/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data);dest.chmod(0o400)
 data,r=read(dest);assert r['st_nlink']==1;originals[str(dest)]=r;project[guest]=pin(data);total+=len(data)
assert total<=268435456 and len(project)>=995
def provider(item):
 p=Path(item['file']);data,r=read(p,old['originalFiles'][str(p)]);assert pin(data)==item['pin'];originals[str(p)]=r;return item
baseline=provider(old['baselineCore']);libs=[provider(x)for x in old['libraries']];generated=[provider(x)for x in old['generatedProviders']]
for p in(G,Path(old['docker']['prefix'][0])):
 data,r=read(p,old['originalFiles'][str(p)]);originals[str(p)]=r
wb=Path(old['worker']['file']);data,r=read(wb,old['originalFiles'][str(wb)]);assert pin(data)=={'sha256':'e9bfc40c92f8659af4eda8a4ebcd306699531b058b56acb5d2a426e34ebb7301','bytes':37768};originals[str(wb)]=r
data,r=read(H);assert pin(data)=={'sha256':'e110810623f27acc871903ffa2ef62839d58c0cc0573f3bbcb8b649e7b9b3ec6','bytes':27890}and stat.S_IMODE(r['st_mode'])==0o400;originals[str(H)]=r
unit=dict(old['workerConfig']['units'][1]);unit.update(name='capella-sync-adverse',scope='candidate',source='/work/tests/test_capella_sync_adverse.cpp',executable='nano-capella-sync-adverse-tests',object={'kind':'compile'})
unit['compilerFlags']=[x for x in unit['compilerFlags']if not x.startswith('-DNANO_TEST_SOURCE')]+['-DNANO_TEST_SOURCE="/work"']
records=[]
test=mf['test'];assert len(test['expectedFields'])==3
for i,fields in enumerate(test['expectedFields']):
 fixed=dict(fields);ints={}
 if i==0:ints['workerQuanta']={'minimum':5,'maximum':64}
 if i==2:ints['actualPeakRssKiB']={'minimum':1,'maximum':131072}
 records.append({'expectedFields':fixed,'integerFields':ints})
wc={'coreMode':'retained','expectedObjectMembers':143,'timeoutSeconds':900,'runtimeInputs':['/work/'+x for x in runtime],'projectPins':project,'sourcePins':{k:v for k,v in project.items()if k in(['/work/'+x for x in runtime]+['/work/CMakeLists.txt','/work/tests/test_capella_sync_adverse.cpp','/work/tests/support/durable_owned_fixture.hpp'])},'units':[unit],'cases':[{'name':'capella-sync-adverse-self-test','unit':'capella-sync-adverse','args':['--self-test'],'timeoutSeconds':180,'expectedExit':0,'stderr':{'kind':'empty'},'stdout':{'kind':'json-field-records','records':records}}]}
control={'format':'nano-capella-sync-adverse-retained-control-v1','ready':True,'repoRoot':str(R),'gitExecutable':str(G),'actualHead':HEAD,'sourceRoot':str(S),'outputRoot':str(E/'capella-sync-adverse-arm-root-20261018-02'),'docker':{'prefix':[old['docker']['prefix'][0],'--config',str(D),'--host','unix:///Users/boris/.docker/run/docker.sock'],'image':old['docker']['image'],'architecture':'arm64','uid':os.getuid(),'gid':os.getgid(),'tmpfsMiB':512},'worker':{'file':str(wb),'pin':pin(read(wb)[0])},'retainedCore':dict(baseline,members=143),'libraries':libs,'generatedProviders':generated,'originalFiles':originals,'workerConfig':wc,'coreSourcePins':core}
# Only newly copied source rows are prospective; settle ctime before first hold.
copied={str(S/k.removeprefix('/work/')):v for k,v in project.items()}
first_rows={p:dict(r)for p,r in originals.items()if p in copied};last=None;stable=0;start=time.monotonic()
while stable<3:
 current={}
 for path,expected in copied.items():
  data,r=read(Path(path));assert pin(data)==expected
  assert {k:v for k,v in r.items()if k!='st_ctime_ns'}=={k:v for k,v in first_rows[path].items()if k!='st_ctime_ns'},'new source changed beyond ctime'
  current[path]=r
 stable=stable+1 if current==last else 1
 assert time.monotonic()-start<30,'new projection metadata did not settle'
 last=current
 if stable<3:time.sleep(1.05)
originals.update(current)
for path,want in originals.items():read(Path(path),want)
read(oldp,oldrow);read(P/'manifest.json',mr)
assert git('rev-parse','HEAD').decode().strip()==HEAD and not git('status','--porcelain')
dest=C/'control.private.json';dest.write_text(json.dumps(control,indent=2,sort_keys=True)+'\n');dest.chmod(0o400);cp=pin(read(dest)[0])
(C/'admission-public.json').write_text(json.dumps({'passed':True,'sourceBase':HEAD,'sourceFiles':len(project),'sourceBytes':total,'coreSourceFilesCompared':989,'oldRowsRefreshed':False,'selectedOldProviders':8,'newSourceProjection':True,'sourceManifest':pin(mb),'control':cp,'newCppCompilations':0,'newContainers':0,'productionQualified':False},indent=2)+'\n')
print(json.dumps({'control':str(dest),'pin':cp,'sourceFiles':len(project),'sourceBytes':total,'ready':True}))
