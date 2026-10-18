"""Public source, worker and prospective test oracle only; no private authorities exported."""
from pathlib import Path
import hashlib,io,json,os,stat,sys,tarfile
E=Path(__file__).resolve().parent
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def read(p,want=None):
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);r={k:getattr(a,k)for k in F};assert stat.S_ISREG(a.st_mode)and a.st_size<=33554432
  b=b''
  while chunk:=os.read(fd,1048576):b+=chunk
  assert len(b)==a.st_size and r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  r['sha256']=pin(b)['sha256'];assert want is None or r==want
  return b,r
 finally:os.close(fd)
assert len(sys.argv)==4
controlp=Path(sys.argv[1]);raw,controlrow=read(controlp);assert pin(raw)=={'sha256':sys.argv[2],'bytes':int(sys.argv[3])}
c=json.loads(raw);assert c['ready']and c['actualHead']=='4ae774f16220d8a9362004b573d83f3c8ed78a97'
assert len(c['workerConfig']['projectPins'])==1043
assert all(k.startswith('/work/')and Path(k).as_posix()==k and '..'not in Path(k).parts for k in c['workerConfig']['projectPins'])
source=Path(c['sourceRoot']);data={};held={}
for guest,want in c['workerConfig']['projectPins'].items():
 p=source/guest.removeprefix('/work/');b,r=read(p,c['originalFiles'][str(p)]);assert pin(b)==want;held[str(p)]=r;data['source/'+guest.removeprefix('/work/')]=b
for name,p in(('host.py',E/'capella-sync-adverse-host-proposal-20261018/host.py'),('worker.py',Path(c['worker']['file']))):
 b,r=read(p,c['originalFiles'][str(p)]);held[str(p)]=r;data[name]=b
spec={'format':'nano-capella-sync-public-transfer-v1','head':c['actualHead'],'projectPins':c['workerConfig']['projectPins'],'coreSourcePins':c['coreSourcePins'],'workerConfig':c['workerConfig'],'files':{name:pin(b)for name,b in data.items()},'privateAuthorityRowsExported':False,'privateCampaignInputsExported':False}
data['spec.json']=(json.dumps(spec,sort_keys=True,indent=2)+'\n').encode()
p=E/'capella-sync-adverse-public-transfer-root-20261018-01.tar';assert not p.exists()
with tarfile.open(p,'x:',format=tarfile.USTAR_FORMAT)as tf:
 for name,b in sorted(data.items()):
  info=tarfile.TarInfo(name);info.size=len(b);info.mode=0o400;info.uid=0;info.gid=0;info.mtime=0;tf.addfile(info,io.BytesIO(b))
p.chmod(0o400)
for path,want in held.items():read(Path(path),want)
read(controlp,controlrow)
result={'passed':True,'bundle':str(p),'bundlePin':pin(read(p)[0]),'files':len(data),'sourceFiles':len(c['workerConfig']['projectPins']),'privateAuthorityRowsExported':False,'privateCampaignInputsExported':False,'newCompilations':0,'productionQualified':False}
(E/'capella-sync-adverse-public-transfer-root-20261018-01.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
