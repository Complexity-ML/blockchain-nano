#!/usr/bin/env python3
"""Dispatch one reviewed public controller. No private input transfer."""
import hashlib,json,os,pathlib,shlex,signal,stat,subprocess,sys,time,uuid
HERE=pathlib.Path(__file__).resolve().parent
script=pathlib.Path(sys.argv[1]).resolve();expected=sys.argv[2];size=int(sys.argv[3])
assert script.parent==HERE.parent/'fresh-amd-canonical-build-source-20261016' and len(expected)==64
fd=os.open(script,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
try:
 fields=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
 before=os.fstat(fd);assert stat.S_ISREG(before.st_mode) and before.st_nlink==1 and before.st_size==size and 0<size<65536
 raw=os.read(fd,size+1);assert len(raw)==size and hashlib.sha256(raw).hexdigest()==expected
 original=tuple(getattr(before,k) for k in fields)
 assert original==tuple(getattr(os.fstat(fd),k) for k in fields)==tuple(getattr(script.lstat(),k) for k in fields)
finally:os.close(fd)
nonce=str(uuid.uuid4());out=HERE/'canonical-invocation-06';out.mkdir(mode=0o700,exist_ok=False)
control={'format':'nano-new-public-canonical-dispatch-v1','nonce':nonce,'controllerSha256':expected,'controllerBytes':size,'revision':'712b8357d05f09cc28df6e04920c9955e59b7b69','image':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','privateInputsTransferred':False,'productionQualified':False}
def write(leaf,data,mode=0o600):
 f=os.open(out/leaf,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,mode)
 with os.fdopen(f,'wb')as s:s.write(data);s.flush();os.fsync(s.fileno())
write('dispatch-public.json',(json.dumps(control,sort_keys=True)+'\n').encode(),0o400)
loader=r'''
import hashlib,os,pathlib,sys,time
expected=__EXPECTED__;size=__SIZE__;nonce=__NONCE__;revision=__REVISION__
raw=sys.stdin.buffer.read(size+1)
assert len(raw)==size and hashlib.sha256(raw).hexdigest()==expected
p=pathlib.Path('/home/ubuntu')/('nano-canonical-controller-'+nonce+'.py')
fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
with os.fdopen(fd,'wb')as f:f.write(raw);f.flush();os.fsync(f.fileno())
d=os.open(p.parent,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
try:os.fsync(d)
finally:os.close(d)
fields=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
previous=None;count=0;since=0;deadline=time.monotonic()+10
while True:
 assert time.monotonic()<deadline
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);b=os.read(fd,size+1);row=tuple(getattr(a,k)for k in fields)
  assert row==tuple(getattr(os.fstat(fd),k)for k in fields)==tuple(getattr(p.lstat(),k)for k in fields)
  assert a.st_nlink==1 and a.st_uid==1000 and a.st_gid==1000 and b==raw
 finally:os.close(fd)
 now=time.monotonic()
 if row==previous:count+=1
 else:previous=row;count=1;since=now
 if count>=3 and now-since>=2:break
 time.sleep(1)
sys.argv=[str(p),'--uuid',nonce,'--revision',revision]
exec(compile(raw,str(p),'exec'),{'__name__':'__main__','__file__':str(p)})
'''
for key,value in [('__EXPECTED__',repr(expected)),('__SIZE__',repr(size)),('__NONCE__',repr(nonce)),('__REVISION__',repr(control['revision']))]:loader=loader.replace(key,value)
argv=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=8','-o','StrictHostKeyChecking=yes','-o','ForwardAgent=no','ubuntu@145.239.199.166','python3 -c '+shlex.quote(loader)]
print(json.dumps({'state':'dispatching','nonce':nonce,'revision':control['revision'],'privateInputsTransferred':False}),flush=True)
fds=[];child=None;started=time.monotonic();code=None;transportError=None
try:
 for leaf in ('stdout.private','stderr.private'):fds.append(os.open(out/leaf,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600))
 child=subprocess.Popen(argv,stdin=subprocess.PIPE,stdout=fds[0],stderr=fds[1],start_new_session=True)
 try:child.communicate(raw,timeout=1960);code=child.returncode
 except (subprocess.TimeoutExpired,OSError)as e:transportError=type(e).__name__
finally:
 if child is not None and child.poll()is None:
  try:os.killpg(child.pid,signal.SIGTERM);child.wait(timeout=5)
  except (ProcessLookupError,subprocess.TimeoutExpired):
   try:os.killpg(child.pid,signal.SIGKILL)
   except ProcessLookupError:pass
   child.wait(timeout=5)
 for f in fds:os.fsync(f);os.close(f)
fd=os.open(script,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
try:
 a=os.fstat(fd);again=os.read(fd,size+1)
 assert stat.S_ISREG(a.st_mode) and original==tuple(getattr(a,k)for k in fields)==tuple(getattr(os.fstat(fd),k)for k in fields)==tuple(getattr(script.lstat(),k)for k in fields)
 assert again==raw and hashlib.sha256(again).hexdigest()==expected
finally:os.close(fd)
b=(out/'stdout.private').read_bytes();assert len(b)<=65536
result=None
if transportError is None and code is not None and code!=255:
 try:
  result=json.loads(b)
  assert isinstance(result,dict) and result['format']=='nano-new-public-canonical-amd-build-v1'
  assert result['hostPrivateBuildInputsRead']is False and result['oldMetadataReadOrRefreshed']is False and result['artifactOrSdkHashesExported']is False and result['runtimeStarted']is False and result['productionQualified']is False
 except (ValueError,KeyError,AssertionError):result=None
public={'format':'nano-new-public-canonical-amd-root-result-v1','sourceRevision':control['revision'],'nonce':nonce,'remoteExitCode':code,'transportComplete':transportError is None and code is not None and code!=255,'transportError':transportError,'remoteTerminalObserved':result is not None,'remoteCleanupUnconfirmed':result is None or result.get('ownedContainerAbsent')is not True,'elapsedMs':int((time.monotonic()-started)*1000),'result':result,'privateInputsTransferred':False,'productionQualified':False}
data=(json.dumps(public,sort_keys=True)+'\n').encode();write('public-result.json',data,0o400)
print(json.dumps({'result':public,'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data)}),flush=True)
raise SystemExit(0 if code==0 and result is not None and result['passed']is True else 1)
