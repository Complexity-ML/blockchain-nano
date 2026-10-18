"""Read back explicit public terminal evidence only. Never reads/copies DBs or keys."""
from pathlib import Path,PurePosixPath
import hashlib,io,json,os,stat,subprocess,tarfile,time
E=Path(__file__).resolve().parent;O=E/'capella-sync-adverse-amd-public-readback-root-20261018-01'
assert not O.exists();O.mkdir(mode=0o700)
remote=r'''
from pathlib import Path
import hashlib,io,json,os,stat,sys,tarfile
O=Path('/home/ubuntu/capella-sync-adverse-amd-root-20261018-01')
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def read(p,want=None):
 assert p.is_absolute()and p.resolve(strict=True)==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);r={k:getattr(a,k)for k in F};assert stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<=a.st_size<=1048576;b=b''
  while c:=os.read(fd,65536):
   b+=c;assert len(b)<=a.st_size
  assert len(b)==a.st_size and r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  r['sha256']=hashlib.sha256(b).hexdigest();assert want is None or r==want;return b,r
 finally:os.close(fd)
names=['result-public.json','host-final-fence-public.json']
if (O/'outputs/result-public.json').exists():names.append('outputs/result-public.json')
for stem in('M-capella-sync-adverse','CXX-capella-sync-adverse','LINK-capella-sync-adverse','LDD-capella-sync-adverse','test-capella-sync-adverse-self-test'):
 for suffix in('exit.json','stdout.log','stderr.log'):
  name='outputs/'+stem+'.'+suffix
  if (O/name).exists():names.append(name)
for stem in('git-head-before','git-clean-before','image-before','name-before','create-own','created-own','start-own','running-own','git-head-after','git-clean-after','cleanup-by-cid','stop-own','cleanup-stopped','remove-own','cleanup-absence','image-final'):
 p=O/(stem+'.exit-public.json')
 if p.exists():names.append(p.name)
data={};held={}
for name in names:
 data[name],held[name]=read(O/name);assert sum(map(len,data.values()))<=4000000
host=json.loads(data['result-public.json']);fence=json.loads(data['host-final-fence-public.json'])
assert host['actualHead']=='4ae774f16220d8a9362004b573d83f3c8ed78a97'
for name,want in held.items():read(O/name,want)
index={'format':'nano-capella-sync-public-readback-v1','hostPassed':host['passed'],'fencePassed':fence['passed'],'files':{name:{'sha256':r['sha256'],'bytes':r['st_size']}for name,r in held.items()},'prospectivePublicOutputsOnly':True,'privateAuthorityRowsReadOrExported':False,'databaseOrKeyFilesRead':False}
data['readback-index-public.json']=(json.dumps(index,sort_keys=True,indent=2)+'\n').encode()
buffer=io.BytesIO()
with tarfile.open(fileobj=buffer,mode='w:',format=tarfile.USTAR_FORMAT)as tf:
 for name,b in sorted(data.items()):
  info=tarfile.TarInfo(name);info.size=len(b);info.mode=0o400;tf.addfile(info,io.BytesIO(b))
for name,want in held.items():read(O/name,want)
assert buffer.tell()<=4194304;sys.stdout.buffer.write(buffer.getvalue())
'''
args=['/usr/bin/ssh','-T','-o','BatchMode=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','/usr/bin/python3 -']
p=None;first=None;began=time.monotonic();out=b'';err=b''
try:
 p=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True);assert p.pid!=28722
 out,err=p.communicate(remote.encode(),timeout=60)
except BaseException as exc:
 first={'type':type(exc).__name__,'message':str(exc)}
 if p is not None:
  try:
   assert p.pid!=28722
   try:os.killpg(p.pid,15)
   except ProcessLookupError:pass
   try:out,err=p.communicate(timeout=3)
   except subprocess.TimeoutExpired:
    try:os.killpg(p.pid,9)
    except ProcessLookupError:pass
    out,err=p.communicate(timeout=3)
  except BaseException as closing:first['closingError']={'type':type(closing).__name__,'message':str(closing)}
try:(O/'ssh.stderr.log').write_bytes(err)
finally:(O/'ssh.exit-public.json').write_text(json.dumps({'exitCode':None if p is None else p.returncode,'error':first,'elapsedMs':round((time.monotonic()-began)*1000)},indent=2)+'\n')
assert first is None and p.returncode==0 and not err and len(out)<=4194304
(O/'public-evidence.tar').write_bytes(out);files={}
with tarfile.open(fileobj=io.BytesIO(out),mode='r:')as tf:
 for member in tf:
  q=PurePosixPath(member.name);assert member.isfile()and not q.is_absolute()and '..'not in q.parts and q.as_posix()==member.name and member.name not in files and member.size<=1048576
  assert len(files)<64;files[member.name]=tf.extractfile(member).read();assert len(files[member.name])==member.size
allowed={'result-public.json','host-final-fence-public.json','outputs/result-public.json','readback-index-public.json'}
for stem in('M-capella-sync-adverse','CXX-capella-sync-adverse','LINK-capella-sync-adverse','LDD-capella-sync-adverse','test-capella-sync-adverse-self-test'):
 for suffix in('exit.json','stdout.log','stderr.log'):allowed.add('outputs/'+stem+'.'+suffix)
for stem in('git-head-before','git-clean-before','image-before','name-before','create-own','created-own','start-own','running-own','git-head-after','git-clean-after','cleanup-by-cid','stop-own','cleanup-stopped','remove-own','cleanup-absence','image-final'):allowed.add(stem+'.exit-public.json')
assert set(files)<=allowed and {'result-public.json','host-final-fence-public.json','readback-index-public.json'}<=set(files)
index=json.loads(files['readback-index-public.json']);assert set(files)==set(index['files'])|{'readback-index-public.json'}
for name,want in index['files'].items():assert {'sha256':hashlib.sha256(files[name]).hexdigest(),'bytes':len(files[name])}==want
for name,b in files.items():
 q=O/'evidence'/name;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(b);q.chmod(0o400)
for name,b in files.items():
 q=O/'evidence'/name;assert q.resolve(strict=True)==q and q.read_bytes()==b and stat.S_IMODE(q.stat().st_mode)==0o400
host=json.loads(files['result-public.json']);worker=json.loads(files['outputs/result-public.json'])if 'outputs/result-public.json'in files else {'passed':False}
result={'passed':True,'qualificationPassed':host['passed']and worker['passed']and index['fencePassed']and(host.get('cleanup')or{}).get('ownedContainerAbsent')is True and(host.get('imageFinalFence')or{}).get('passed')is True,'files':len(files),'evidenceBytes':sum(map(len,files.values())),'archivePin':{'sha256':hashlib.sha256(out).hexdigest(),'bytes':len(out)},'sshExitCode':p.returncode,'privateAuthorityRowsReadOrExported':False,'databaseOrKeyFilesRead':False,'newTestsRun':0,'productionQualified':False}
(O/'result-public.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
