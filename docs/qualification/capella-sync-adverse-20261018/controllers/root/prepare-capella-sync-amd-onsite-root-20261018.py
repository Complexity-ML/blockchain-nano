"""Root-only public bundle admission against genuine retained AMD authorities."""
from pathlib import Path,PurePosixPath
import hashlib,io,json,os,stat,subprocess,tarfile,time,sys
HOME=Path('/home/ubuntu');R=HOME/'blockchain-nano-git';HEAD='4ae774f16220d8a9362004b573d83f3c8ed78a97'
BUNDLE=HOME/'capella-sync-adverse-public-transfer-root-20261018-01.tar'
BASE=HOME/'capella-sync-adverse-amd-input-root-20261018-01'
C=HOME/'capella-sync-adverse-amd-control-root-20261018-01'
D=HOME/'capella-sync-adverse-amd-docker-client-root-20261018-01'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def read(p,want=None):
 assert p.is_absolute()and p.resolve(strict=True)==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);v={k:getattr(a,k)for k in F};assert stat.S_ISREG(a.st_mode)and a.st_size<=268435456
  b=b''
  while chunk:=os.read(fd,1048576):b+=chunk
  assert len(b)==a.st_size and v=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  v['sha256']=pin(b)['sha256'];assert want is None or v==want,(str(p),'original row changed');return b,v
 finally:os.close(fd)
def admitted(p,want):
 b,r=read(p);assert pin(b)==want;return json.loads(b),r
oldp=HOME/'rpc-candidate-amd-continuation-control-20261018-01/control.private.json'
old,oldrow=admitted(oldp,{'sha256':'e6a37b5069a6786dea733bc3c0136e59fae63d2be214a34027b13d55ecb3b9b1','bytes':1281491})
initialp=HOME/'rpc-candidate-amd-control-root-20261018-01/control.private.json'
initial,initialrow=admitted(initialp,{'sha256':'ac2eeedf8406ae807043bb91ec56d135f8b9829b89e54665cd2d3c0e84e2d3d1','bytes':1178681})
producerp=HOME/'rpc-candidate-amd-root-20261018-01/outputs/final-observations.private.json'
producer,producerrow=admitted(producerp,{'sha256':'866e3f3d0042ec8cf90b487498bfa70601e9f9af826379d6dfe5eb03239e43c9','bytes':1512552})
assert len(sys.argv)==3
bundle,bundlerow=read(BUNDLE);assert len(bundle)<=33554432 and pin(bundle)=={'sha256':sys.argv[1],'bytes':int(sys.argv[2])}
files={}
with tarfile.open(fileobj=io.BytesIO(bundle),mode='r:')as tf:
 for member in tf:
  p=PurePosixPath(member.name);assert member.isfile()and p.as_posix()==member.name and not p.is_absolute()and '..'not in p.parts and member.name not in files
  assert len(files)<1100 and 0<=member.size<=33554432;files[member.name]=tf.extractfile(member).read();assert len(files[member.name])==member.size
spec=json.loads(files['spec.json']);assert spec['format']=='nano-capella-sync-public-transfer-v1'and spec['head']==HEAD
assert set(files)==set(spec['files'])|{'spec.json'}
for name,want in spec['files'].items():assert pin(files[name])==want
assert len(spec['coreSourcePins'])==989 and len(spec['projectPins'])==1043
assert all(k.startswith('/work/')and PurePosixPath(k).as_posix()==k and '..'not in PurePosixPath(k).parts for k in spec['projectPins'])
assert set(files)=={'source/'+k.removeprefix('/work/')for k in spec['projectPins']}|{'host.py','worker.py','spec.json'}
assert spec['workerConfig']['projectPins']==spec['projectPins']
for guest,want in spec['coreSourcePins'].items():
 rel=guest.removeprefix('/work/');assert guest.startswith(('/work/src/','/work/include/','/work/third_party/'))and pin(read(R/rel)[0])==want and spec['projectPins'][guest]==want
G=Path('/usr/bin/git');DOCKER=Path('/usr/bin/docker');SUDO=Path('/usr/bin/sudo')
def git(*args):return subprocess.check_output([str(G),'-c','core.fsmonitor=false','--no-optional-locks','-C',str(R),*args])
assert git('rev-parse','HEAD').decode().strip()==HEAD and not git('status','--porcelain')
assert not any(p.exists()for p in(BASE,C,D));BASE.mkdir(mode=0o700);C.mkdir(mode=0o700);D.mkdir(mode=0o700)
for name,data in files.items():
 p=BASE/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);p.chmod(0o400)
originals={}
core=old['retainedCore'];corepath=core['file'];assert old['originalFiles'][corepath]==producer['products'][corepath]['hostFD9']
selected=[{'file':corepath,'guest':'/baseline/libnano_core.a','pin':core['pin']},*old['libraries'],*old['generatedProviders']]
for item in selected:
 p=Path(item['file']);data,row=read(p,old['originalFiles'][str(p)]);assert pin(data)==item['pin'];originals[str(p)]=row
for p in(G,DOCKER,SUDO):data,row=read(p,old['originalFiles'][str(p)]);originals[str(p)]=row
prospective=[BASE/'source'/guest.removeprefix('/work/')for guest in spec['projectPins']]+[BASE/'host.py',BASE/'worker.py']
expected_new={str(BASE/name):want for name,want in spec['files'].items()}
assert set(map(str,prospective))==set(expected_new)
def prospective_rows():
 rows={}
 for p in prospective:
  data,r=read(p);assert pin(data)==expected_new[str(p)];rows[str(p)]=r
 return rows
initial_new=prospective_rows();last=None;stable=0;began=time.monotonic()
while stable<3:
 current=prospective_rows()
 for path,row in current.items():assert {k:v for k,v in row.items()if k!='st_ctime_ns'}=={k:v for k,v in initial_new[path].items()if k!='st_ctime_ns'}
 stable=stable+1 if current==last else 1;last=current;assert time.monotonic()-began<30
 if stable<3:time.sleep(1.05)
originals.update(current)
unit=dict(initial['units'][1]);unit.update(name='capella-sync-adverse',scope='candidate',source='/work/tests/test_capella_sync_adverse.cpp',executable='nano-capella-sync-adverse-tests',object={'kind':'compile'})
unit['compilerFlags']=[x for x in unit['compilerFlags']if not x.startswith('-DNANO_TEST_SOURCE')]+['-DNANO_TEST_SOURCE="/work"']
wc=dict(spec['workerConfig']);wc['units']=[unit]
control={'format':'nano-capella-sync-adverse-retained-control-v1','ready':True,'repoRoot':str(R),'gitExecutable':str(G),'actualHead':HEAD,'sourceRoot':str(BASE/'source'),'outputRoot':str(HOME/'capella-sync-adverse-amd-root-20261018-01'),'docker':{'prefix':[str(SUDO),'-n',str(DOCKER),'--host','unix:///var/run/docker.sock','--config',str(D)],'image':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','architecture':'amd64','uid':os.getuid(),'gid':os.getgid(),'tmpfsMiB':512},'worker':{'file':str(BASE/'worker.py'),'pin':spec['files']['worker.py']},'retainedCore':dict(selected[0],members=143),'libraries':old['libraries'],'generatedProviders':old['generatedProviders'],'originalFiles':originals,'workerConfig':wc,'coreSourcePins':spec['coreSourcePins']}
for path,want in originals.items():read(Path(path),want)
for path,want in((oldp,oldrow),(initialp,initialrow),(producerp,producerrow),(BUNDLE,bundlerow)):read(path,want)
assert git('rev-parse','HEAD').decode().strip()==HEAD and not git('status','--porcelain')
p=C/'control.private.json';p.write_text(json.dumps(control,indent=2,sort_keys=True)+'\n');p.chmod(0o400);cp=pin(read(p)[0])
result={'format':'nano-capella-sync-amd-prospective-control-v1','passed':True,'sourceBase':HEAD,'sourceFiles':len(spec['projectPins']),'unchangedCoreSources':989,'oldRowsRefreshed':False,'originalRetainedProducts':7,'originalSoftware':3,'control':cp,'controlFile':str(p),'hostFile':str(BASE/'host.py'),'newContainers':0,'newCppCompilations':0,'productionQualified':False}
(C/'admission-public.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
