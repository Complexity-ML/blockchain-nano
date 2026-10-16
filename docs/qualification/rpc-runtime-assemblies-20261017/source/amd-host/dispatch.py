#!/usr/bin/env python3
"""Transfer public relink controls only; retain private artifacts on the VPS."""
from pathlib import Path
import os,json,hashlib,base64,subprocess,shlex,time,ast,stat
E=Path(__file__).resolve().parent;LOCAL=E/'rpc-runtime-amd-root-qualification-20261017-01';C='/home/ubuntu/rpc-runtime-root-control-20261017-01';O='/home/ubuntu/current-amd-rpc-runtime-relink-20261017-01'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
os.umask(0o077)
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def save(name,value):
 with(LOCAL/name).open('x')as f:json.dump(value,f,sort_keys=True);f.write('\n')
def main():
 LOCAL.mkdir(mode=0o700);roots=E/'current-amd-rpc-runtime-relink-source-20261017';host=roots/'host.py';worker=E/'current-linux-rpc-runtime-source-20261017/worker.pl';files={};held={}
 for leaf,p in [('host.py',host),('worker.pl',worker)]:
  assert p.resolve()==p and stat.S_IMODE(p.stat().st_mode)==0o400;b=p.read_bytes();s=p.lstat();held[str(p)]={k:getattr(s,k)for k in F};assert len(b)<131072
  assert pin(b)==({'sha256':'c97f1bc0dce5212d16464d9b73c699f94575d8ed4f908af413297369a279b568','bytes':26887} if leaf=='host.py' else {'sha256':'f60a63c70d5d2fab65647917458d9829f6966df840275c2d778291aaef8b3154','bytes':27010})
  if p.suffix=='.py':ast.parse(b)
  files[leaf]={'data':base64.b64encode(b).decode(),**pin(b)}
 save('source-controls-public.json',{leaf:{k:v[k]for k in ('sha256','bytes')}for leaf,v in files.items()})
 stage=r'''from pathlib import Path
import os,json,hashlib,sys,base64,stat,time
j=json.load(sys.stdin);root=Path('/home/ubuntu/rpc-runtime-root-control-20261017-01');assert j['root']==str(root)and os.getuid()==os.getgid()==1000 and root.parent.resolve()==root.parent;os.umask(0o077);root.mkdir(mode=0o700)
assert set(j['files'])=={'host.py','worker.pl'};F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
for leaf,z in j['files'].items():
 b=base64.b64decode(z['data'],validate=True);assert len(b)==z['bytes']and len(b)<131072 and hashlib.sha256(b).hexdigest()==z['sha256'];p=root/leaf
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
 with os.fdopen(fd,'wb')as f:f.write(b);f.flush();os.fsync(f.fileno())
rows=[];start=time.monotonic()
for sample in range(3):
 table={}
 for leaf,z in j['files'].items():
  p=root/leaf;s=p.lstat();assert stat.S_ISREG(s.st_mode)and s.st_nlink==1 and s.st_uid==s.st_gid==1000 and stat.S_IMODE(s.st_mode)==0o400;b=p.read_bytes();assert len(b)==z['bytes']and hashlib.sha256(b).hexdigest()==z['sha256'];row={k:getattr(s,k)for k in F};assert row=={k:getattr(p.lstat(),k)for k in F};table[leaf]=dict(row,sha256=z['sha256'])
 rows.append(table)
 if sample<2:time.sleep(1.05)
assert rows[0]==rows[1]==rows[2]and time.monotonic()-start>=2
print(json.dumps({'files':rows[-1],'publicControlFiles':2,'privateCampaignInputsTransferred':False},sort_keys=True))
'''
 def ssh(code,payload,timeout):return subprocess.run(['/usr/bin/ssh','-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','/usr/bin/python3 -c '+shlex.quote(code)],input=json.dumps(payload).encode(),capture_output=True,timeout=timeout)
 q=ssh(stage,{'root':C,'files':files},35);save('stage-exit-public.json',{'exitCode':q.returncode});(LOCAL/'stage-stderr.private.log').write_bytes(q.stderr);assert q.returncode==0 and not q.stderr;ad=json.loads(q.stdout);save('controls-admission.private.json',ad)
 execute=r'''from pathlib import Path
import os,json,sys,hashlib
j=json.load(sys.stdin);root=Path('/home/ubuntu/rpc-runtime-root-control-20261017-01');F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
for leaf,want in j['files'].items():
 p=root/leaf;s=p.lstat();b=p.read_bytes();assert want==dict({k:getattr(s,k)for k in F},sha256=hashlib.sha256(b).hexdigest())==dict({k:getattr(p.lstat(),k)for k in F},sha256=hashlib.sha256(b).hexdigest())
os.execv('/usr/bin/python3',['/usr/bin/python3',str(root/'host.py'),'--worker',str(root/'worker.pl'),'--output-root','/home/ubuntu/current-amd-rpc-runtime-relink-20261017-01'])
'''
 start=time.monotonic();q=ssh(execute,ad,1800);(LOCAL/'host-stdout.private.log').write_bytes(q.stdout);(LOCAL/'host-stderr.private.log').write_bytes(q.stderr);save('host-execution-exit-public.json',{'exitCode':q.returncode,'elapsedMs':int((time.monotonic()-start)*1000),'privateInputsTransferred':False,'remoteOutputRoot':O})
 readback=r'''from pathlib import Path
import json,hashlib
r=Path('/home/ubuntu/current-amd-rpc-runtime-relink-20261017-01');result={}
for name in ['result-public.json','cleanup-public.json','originals-final-fence-public.json','failure-public.json']:
 p=r/name
 if p.exists():
  b=p.read_bytes();assert len(b)<32768;result[name]={'contents':json.loads(b),'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
print(json.dumps(result,sort_keys=True))
'''
 z=ssh(readback,{},35);assert z.returncode==0 and not z.stderr;packet=json.loads(z.stdout)
 for leaf,v in packet.items():
  b=(json.dumps(v['contents'],sort_keys=True)+'\n').encode();save(leaf,v['contents'])
 save('remote-public-readback-pins.json',{k:{n:v[n]for n in ('sha256','bytes')}for k,v in packet.items()})
 for p,row in held.items():assert row=={k:getattr(Path(p).lstat(),k)for k in F}
 assert q.returncode==0 and packet.get('result-public.json',{}).get('contents',{}).get('passed')is True
 print(json.dumps({'passed':True,'remoteOutputRoot':O,'localPublicReadback':str(LOCAL),'privateInputsTransferred':False,'productionQualified':False}),flush=True)
if __name__=='__main__':main()
