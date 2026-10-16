#!/usr/bin/env python3
"""Root VPS existing-binary disk probe; two selected artifacts, no SDK audit."""
import pathlib,json,hashlib,base64,subprocess,shlex,time
E=pathlib.Path(__file__).resolve().parent;L=E/'pruning-sst-disk-amd-root-qualification-20261016-01';L.mkdir(mode=0o700)
P=E/'pruning-sst-disk-runner-source-20261016/runner.py';raw=P.read_bytes();assert len(raw)==14526 and hashlib.sha256(raw).hexdigest()=='4a7880eed18315ac931adfe1f02ea344642488f2705c23817e776973c340392d' and P.stat().st_mode&511==0o400
Q=E/'pruning-operation-amd-root-qualification-20261016-01';ad=json.loads((Q/'selected-public-readback-root.json').read_bytes());rootrow=ad['files']['result-public.json'];rootraw=(Q/'remote/result-public.json').read_bytes();assert hashlib.sha256(rootraw).hexdigest()==rootrow['sha256']
def save(leaf,j):
 with(L/leaf).open('x')as f:json.dump(j,f,sort_keys=True);f.write('\n')
def ssh(code,j,timeout):
 return subprocess.run(['/usr/bin/ssh','-o','BatchMode=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','/usr/bin/python3 -c '+shlex.quote(code)],input=json.dumps(j).encode(),capture_output=True,timeout=timeout)
helpers=r"""import pathlib,os,json,hashlib,base64,stat,sys,subprocess,signal,time
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def read(p,expected=None):
 p=pathlib.Path(p);assert p.resolve()==p;fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};assert stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<=a.st_size<=(536870912 if str(p)in ['/usr/bin/docker','/usr/bin/sudo']else 16777216);b=b''
  while True:
   z=os.read(fd,65536)
   if not z:break
   b+=z;assert len(b)<=a.st_size
  row['sha256']=hashlib.sha256(b).hexdigest();assert len(b)==a.st_size and {k:row[k]for k in F}=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  if expected is not None:assert row==expected
  return row,b
 finally:os.close(fd)
def put(p,b):
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb')as f:f.write(b);f.flush();os.fsync(f.fileno())
 os.chmod(p,0o400)
os.umask(0o077)
"""
stage=helpers+r"""j=json.load(sys.stdin);A=pathlib.Path('/home/ubuntu/current-amd-pruning-operation-20261016-01');C=pathlib.Path('/home/ubuntu/pruning-sst-disk-root-control-20261016-01');assert os.getuid()==os.getgid()==1000
r,b=read(A/'result-public.json',dict(j['rootRow']['material9'],sha256=j['rootRow']['sha256']));z=json.loads(b);assert z['passed']and z['owner']=='vps-docker'and not z['productionQualified']
_,b=read(A/'outputs/software-admission.private.json');assert len(b)==z['softwareAdmissionBytes']and hashlib.sha256(b).hexdigest()==z['softwareAdmissionSha256'];binding=json.loads(b)
programs={}
for leaf in ['nano-owned-copy-physical-admission-tests','nano-storage-maintenance']:
 row=binding['guestMaterials']['/outputs/'+leaf];programs[leaf]={'materialGNU9':row['material'],'sha256':row['sha256'],'bytes':row['bytes']}
_,b=read(A/'original-selected-inputs-before-worker.private.json');oldrows=json.loads(b);tools={p:oldrows[p]for p in ['/usr/bin/docker','/usr/bin/sudo']}
for p,row in tools.items():read(p,row)
C.mkdir(mode=0o700);runner=base64.b64decode(j['runner'],validate=True);assert len(runner)==14526 and hashlib.sha256(runner).hexdigest()=='4a7880eed18315ac931adfe1f02ea344642488f2705c23817e776973c340392d';put(C/'runner.py',runner)
c={'format':'nano-pruning-sst-disk-runner-input-v1','owner':'vps-docker','imageId':z['imageId'],'dockerArgv':['/usr/bin/sudo','-n','/usr/bin/docker'],'dockerEndpoint':'unix:///var/run/docker.sock','oldOutputs':{'type':'bind','path':str(A/'outputs')},'outputRoot':'/home/ubuntu/pruning-sst-disk-amd-root-qualification-20261016-01','programs':programs};cb=(json.dumps(c,sort_keys=True)+'\n').encode();put(C/'control.json',cb)
observations=[]
for i in range(3):
 observations.append({leaf:read(C/leaf)[0]for leaf in ['runner.py','control.json']})
 if i<2:time.sleep(1.05)
assert observations[0]==observations[1]==observations[2]
print(json.dumps({'files':observations[-1],'toolRows':tools,'controlSha256':hashlib.sha256(cb).hexdigest(),'controlBytes':len(cb),'privateArtifactTransferred':False}))
"""
q=ssh(stage,{'rootRow':rootrow,'runner':base64.b64encode(raw).decode()},40);save('stage-exit-public.json',{'exitCode':q.returncode});assert q.returncode==0,q.stderr.decode();control=json.loads(q.stdout);save('new-control-admission.private.json',control)
execute=helpers+r"""j=json.load(sys.stdin);C=pathlib.Path('/home/ubuntu/pruning-sst-disk-root-control-20261016-01')
for leaf,row in j['files'].items():assert leaf in ['runner.py','control.json'];read(C/leaf,row)
for p,row in j['toolRows'].items():assert p in ['/usr/bin/docker','/usr/bin/sudo'];read(p,row)
with(C/'runner-stdout.private.log').open('xb')as out,(C/'runner-stderr.private.log').open('xb')as err:
 q=subprocess.Popen(['/usr/bin/python3',str(C/'runner.py'),'--control',str(C/'control.json'),'--sha256',j['controlSha256'],'--bytes',str(j['controlBytes'])],stdout=out,stderr=err,start_new_session=True)
 try:q.wait(timeout=260)
 except subprocess.TimeoutExpired:
  os.killpg(q.pid,signal.SIGTERM)
  try:q.wait(timeout=30)
  except subprocess.TimeoutExpired:os.killpg(q.pid,signal.SIGKILL);q.wait(timeout=5)
for p,row in j['toolRows'].items():read(p,row)
for leaf,row in j['files'].items():read(C/leaf,row)
print(json.dumps({'exitCode':q.returncode,'twoToolRowsFullNineAndShaUnchanged':True,'publicRunnerAndControlUnchanged':True,'noSdkAudit':True}))
"""
t=time.monotonic();q=ssh(execute,control,310);save('execute-transport-public.json',{'exitCode':q.returncode,'elapsedMs':int((time.monotonic()-t)*1000)});assert q.returncode==0,q.stderr.decode();execution=json.loads(q.stdout);save('root-execution-public.json',execution)
collect=helpers+r"""json.load(sys.stdin);A=pathlib.Path('/home/ubuntu/pruning-sst-disk-amd-root-qualification-20261016-01');files={}
for leaf in ['result-public.json','cleanup-public.json','failure-public.json','cleanup-failure-public.json']:
 p=A/leaf
 if p.exists():r,b=read(p);files[leaf]={'sha256':r['sha256'],'bytes':len(b),'data':base64.b64encode(b).decode()}
if 'result-public.json'in files:
 z=json.loads(base64.b64decode(files['result-public.json']['data']));r,b=read(A/'disk-probe-admission.private.json');assert len(b)==z['admission']['bytes']and r['sha256']==z['admission']['sha256'];want=json.loads(b)['stdout'];found=[]
 for p in A.glob('*-stdout.private.log'):
  if p.stat().st_size!=want['bytes']:continue
  r,b=read(p)
  if r['sha256']==want['sha256']:found.append(b)
 assert len(found)==1;b=found[0];files['sst-stdout.jsonl']={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),'data':base64.b64encode(b).decode()}
print(json.dumps({'files':files,'privateArtifactsOrDatabasesExported':False}))
"""
q=ssh(collect,{},30);assert q.returncode==0,q.stderr.decode();j=json.loads(q.stdout);assert not j['privateArtifactsOrDatabasesExported'];pins={}
for leaf,z in j['files'].items():
 assert leaf in ['result-public.json','cleanup-public.json','failure-public.json','cleanup-failure-public.json','sst-stdout.jsonl'];b=base64.b64decode(z.pop('data'),validate=True);assert len(b)==z['bytes']and hashlib.sha256(b).hexdigest()==z['sha256'];(L/leaf).write_bytes(b);pins[leaf]=z
save('selected-public-readback-root.json',{'files':pins,'privateArtifactsOrDatabasesExported':False});assert execution['exitCode']==0;z=json.loads((L/'result-public.json').read_bytes());assert z['passed']and z['owner']=='vps-docker';print(json.dumps({'passed':True,'filesystemType':z['filesystemType'],'sstSummary':z['sstSummary'],'ownedContainersAbsent':z['ownedContainersAbsent']}),flush=True)
