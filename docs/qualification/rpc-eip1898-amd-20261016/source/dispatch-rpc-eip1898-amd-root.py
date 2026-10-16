#!/usr/bin/env python3
import pathlib,hashlib,json,base64,subprocess,shlex,uuid,time
E=pathlib.Path(__file__).resolve().parent;O=E/'rpc-eip1898-amd-root-qualification-20261016';H=E/'qualify-rpc-eip1898-amd-root-20261016.py'
BOOT=r'''import os,pathlib,json,hashlib,base64,subprocess,time,stat,signal
os.umask(0o077)
p=json.loads(__import__('sys').stdin.buffer.read(100000));assert set(p)=={'nonce','controller','sha256','bytes'}
assert len(p['nonce'])==32 and all(c in '0123456789abcdef' for c in p['nonce']) and os.getuid()==os.getgid()==1000
body=base64.b64decode(p['controller'],validate=True);assert len(body)==p['bytes'] and hashlib.sha256(body).hexdigest()==p['sha256']
root=pathlib.Path('/home/ubuntu')/('nano-rpc-eip1898-amd-'+p['nonce']);assert not root.exists();root.mkdir(mode=0o700)
script=root/'controller.py';fd=os.open(script,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
try:
 at=0
 while at<len(body):at+=os.write(fd,body[at:])
 os.fsync(fd)
finally:os.close(fd)
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');rows=[];start=time.monotonic()
for i in range(3):
 fd=os.open(script,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  s=os.fstat(fd);b=b''
  while True:
   c=os.read(fd,65536)
   if not c:break
   b+=c
  row={k:getattr(s,k) for k in F};assert stat.S_ISREG(s.st_mode) and s.st_nlink==1 and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(script.lstat(),k) for k in F} and b==body;rows.append(row)
 finally:os.close(fd)
 if i<2:time.sleep(1)
assert rows[0]==rows[1]==rows[2] and time.monotonic()-start>=2
with (root/'controller-first-admission.private.json').open('x') as f:json.dump({'knownSha256':p['sha256'],'rows':rows,'stableSpanMs':int((time.monotonic()-start)*1000)},f)
env={'PATH':'/usr/bin:/bin','HOME':'/home/ubuntu','LANG':'C','LC_ALL':'C','GIT_TERMINAL_PROMPT':'0'}
start=time.monotonic()
interrupted=False
def stop_requested(*_):raise RuntimeError('owned host supervisor interrupted')
for sig in (signal.SIGTERM,signal.SIGINT,signal.SIGHUP):signal.signal(sig,stop_requested)
with (root/'host-stdout.private.log').open('xb') as out,(root/'host-stderr.private.log').open('xb') as err:
 q=subprocess.Popen(['/usr/bin/python3',str(script)],stdin=subprocess.DEVNULL,stdout=out,stderr=err,env=env,start_new_session=True)
 try:q.wait(timeout=720)
 except BaseException:
  interrupted=True
  for sig in (signal.SIGTERM,signal.SIGINT,signal.SIGHUP):signal.signal(sig,signal.SIG_IGN)
  if q.poll() is None:
   q.send_signal(signal.SIGTERM)
   try:q.wait(timeout=90)
   except subprocess.TimeoutExpired:
    try:os.killpg(q.pid,signal.SIGKILL)
    except ProcessLookupError:pass
    q.wait(timeout=10)
fd=os.open(script,os.O_RDONLY|os.O_NOFOLLOW)
try:
 s=os.fstat(fd);b=b''
 while True:
  c=os.read(fd,65536)
  if not c:break
  b+=c
 assert {k:getattr(s,k) for k in F}==rows[0]=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(script.lstat(),k) for k in F} and b==body
finally:os.close(fd)
packet={'format':'nano-rpc-eip1898-amd-dispatch-v1','controllerSha256':p['sha256'],'controllerBytes':len(body),'namespace':str(root),'hostExitCode':q.returncode,'hostElapsedMs':int((time.monotonic()-start)*1000),'publicControllerTransferOnly':True,'privateTablesOrKeysTransferred':False,'productionQualified':False}
packet['controllerFullNineAndShaUnchanged']=True
packet['ownedHostSupervisorInterrupted']=interrupted
for key,leaf in [('result','result-public.json'),('cleanup','cleanup-public.json'),('finalFence','originals-final-fence-public.json')]:
 f=root/'attempt01'/leaf
 if f.is_file():
  b=f.read_bytes();assert len(b)<16384;packet[key]=json.loads(b)
for stream in ('stdout','stderr'):
 b=(root/('host-'+stream+'.private.log')).read_bytes();packet[stream+'Pin']={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
packet['passed']=not interrupted and q.returncode==0 and packet.get('result',{}).get('passed') is True and packet.get('cleanup',{}).get('ownedContainerAbsent') is True and packet.get('finalFence',{}).get('originalInputsFullNineAndShaUnchanged') is True
with (root/'dispatch-result-public.json').open('x') as f:json.dump(packet,f,sort_keys=True);f.write('\n')
print(json.dumps(packet,sort_keys=True),flush=True)
'''
def main():
 raw=H.read_bytes();assert hashlib.sha256(raw).hexdigest()=='910f1cc16b2a5e4dd58d1dda3387692c3ff6e437bb8f4fd967f666f80ee4064e' and len(raw)==14044
 O.mkdir(mode=0o700);(O/'bootstrap-source.py').write_text(BOOT)
 data={'nonce':uuid.uuid4().hex,'controller':base64.b64encode(raw).decode(),'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
 start=time.monotonic();q=subprocess.run(['ssh','-o','BatchMode=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','python3 -c '+shlex.quote(BOOT)],input=json.dumps(data).encode(),capture_output=True,timeout=890)
 for name,b in [('stdout',q.stdout),('stderr',q.stderr)]:
  with (O/(name+'.private.log')).open('xb') as f:f.write(b)
 assert q.returncode==0
 result=json.loads(q.stdout)
 with (O/'dispatch-result-public.json').open('x') as f:json.dump(result,f,sort_keys=True);f.write('\n')
 print(json.dumps(result,sort_keys=True),flush=True)
 assert result['passed'] is True
if __name__=='__main__':main()
