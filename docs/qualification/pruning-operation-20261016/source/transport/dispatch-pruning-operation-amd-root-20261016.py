#!/usr/bin/env python3
"""Root transport of two public qualification programs; no campaign material."""
import pathlib,hashlib,json,base64,subprocess,shlex,os,stat,time
E=pathlib.Path(__file__).resolve().parent
C='/home/ubuntu/pruning-operation-root-control-20261016-01'
O='/home/ubuntu/current-amd-pruning-operation-20261016-01'
LOCAL=E/'pruning-operation-amd-root-qualification-20261016-01'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
PINS={
 'host.py':(E/'pruning-operation-amd-host-source-proposal-20261016/qualify-pruning-operation-amd-root.py','3643c1936e36b171b57fba5df9fd17cd2b931cfaf89a6f4b942b45d19804d43a',24705),
 'worker.pl':(E/'pruning-operation-worker-source-20261016/worker.pl','fc0f3bddc134321f0eabc0b830e0a5ffdb484741321fa9cc3fd6a093d1b04082',36785)}
os.umask(0o077)
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def save(name,j):
 with(LOCAL/name).open('x')as f:json.dump(j,f,sort_keys=True);f.write('\n')
def ssh(code,payload,timeout):
 return subprocess.run(['/usr/bin/ssh','-o','BatchMode=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','/usr/bin/python3 -c '+shlex.quote(code)],input=json.dumps(payload).encode(),capture_output=True,timeout=timeout)
LOCAL.mkdir(mode=0o700)
files={};held={}
for leaf,(p,sha,size)in PINS.items():
 assert p.resolve()==p and stat.S_IMODE(p.stat().st_mode)==0o400
 st=p.lstat();held[str(p)]={k:getattr(st,k)for k in F};b=p.read_bytes();assert pin(b)=={'sha256':sha,'bytes':size}and held[str(p)]=={k:getattr(p.lstat(),k)for k in F};files[leaf]={'data':base64.b64encode(b).decode(),**pin(b)}
stage=r"""import pathlib,os,json,sys,base64,hashlib,stat,time
j=json.load(sys.stdin);r=pathlib.Path(j['root']);assert str(r)=='/home/ubuntu/pruning-operation-root-control-20261016-01'and r.parent.resolve()==r.parent and os.getuid()==os.getgid()==1000
assert set(j['files'])=={'host.py','worker.pl'};os.umask(0o077);r.mkdir(mode=0o700)
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
for leaf,z in j['files'].items():
 b=base64.b64decode(z['data'],validate=True);assert len(b)==z['bytes']and hashlib.sha256(b).hexdigest()==z['sha256'];p=r/leaf
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb')as f:f.write(b);f.flush();os.fsync(f.fileno())
 os.chmod(p,0o400)
rows=[]
for sample in range(3):
 table={}
 for leaf,z in j['files'].items():
  p=r/leaf;s=p.lstat();assert stat.S_ISREG(s.st_mode)and s.st_nlink==1 and s.st_uid==s.st_gid==1000 and stat.S_IMODE(s.st_mode)==0o400
  b=p.read_bytes();assert len(b)==z['bytes']and hashlib.sha256(b).hexdigest()==z['sha256'];table[leaf]={**{k:getattr(s,k)for k in F},'sha256':z['sha256']};assert table[leaf]=={**{k:getattr(p.lstat(),k)for k in F},'sha256':z['sha256']}
 rows.append(table)
 if sample<2:time.sleep(1.05)
assert rows[0]==rows[1]==rows[2]
print(json.dumps({'publicProgramsTransferred':2,'files':rows[-1],'privateCampaignInputsTransferred':False,'newRoot':str(r)}))
"""
a=ssh(stage,{'root':C,'files':files},40);save('transport-exit-public.json',{'exitCode':a.returncode});assert a.returncode==0,a.stderr.decode();ad=json.loads(a.stdout);save('public-control-first-admission.private.json',ad)
execute=r"""import pathlib,os,sys,json,hashlib,stat
j=json.load(sys.stdin);r=pathlib.Path(j['root']);assert str(r)=='/home/ubuntu/pruning-operation-root-control-20261016-01';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
for leaf,row in j['files'].items():
 assert leaf in ['host.py','worker.pl'];p=r/leaf;s=p.lstat();b=p.read_bytes();assert row=={**{k:getattr(s,k)for k in F},'sha256':hashlib.sha256(b).hexdigest()}=={**{k:getattr(p.lstat(),k)for k in F},'sha256':hashlib.sha256(b).hexdigest()}
os.execv('/usr/bin/python3',['/usr/bin/python3',str(r/'host.py'),'--output-root','/home/ubuntu/current-amd-pruning-operation-20261016-01','--worker',str(r/'worker.pl')])
"""
t=time.monotonic();q=ssh(execute,{'root':C,'files':ad['files']},1730)
(LOCAL/'host-stdout.private.log').write_bytes(q.stdout);(LOCAL/'host-stderr.private.log').write_bytes(q.stderr)
save('host-execution-exit-public.json',{'exitCode':q.returncode,'elapsedMs':int((time.monotonic()-t)*1000),'publicProgramsTransferred':2,'privateCampaignInputsTransferred':False,'remoteOutputRoot':O})
for p,row in held.items():assert row=={k:getattr(pathlib.Path(p).lstat(),k)for k in F}
print(json.dumps({'hostExitCode':q.returncode,'privateCampaignInputsTransferred':False,'localLogs':str(LOCAL),'publicControlFilesUnchanged':True}),flush=True)
