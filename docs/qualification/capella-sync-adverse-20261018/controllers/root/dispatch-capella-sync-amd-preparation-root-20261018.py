"""Send only admitted public test sources and prepare a fresh VPS control. No SDK execution."""
from pathlib import Path
import hashlib,json,os,shlex,subprocess,time
E=Path(__file__).resolve().parent;O=E/'capella-sync-adverse-amd-dispatch-preparation-root-20261018-01'
B=E/'capella-sync-adverse-public-transfer-root-20261018-01.tar'
P=E/'prepare-capella-sync-amd-onsite-root-20261018.py'
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
bundle=B.read_bytes();source=P.read_bytes()
bp={'sha256':'eeb2b3aaa04fbeac43c0168bde4fd5b35969131e5faa0d564eaf2741d5f43d82','bytes':19496960}
assert pin(bundle)==bp and pin(source)=={'sha256':'65d664666718cbd8a3992e3dad8a05c3cc82bc211ee68f1bb4e3f934cc3ae8fa','bytes':7216}
assert not O.exists();O.mkdir(mode=0o700)
prefix=['/usr/bin/ssh','-T','-o','BatchMode=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166']
def command(name,args,data,timeout):
 (O/(name+'.argv.json')).write_text(json.dumps(args)+'\n');p=None;error=None;began=time.monotonic();stdout=b'';stderr=b''
 try:
  p=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True);assert p.pid!=28722
  stdout,stderr=p.communicate(data,timeout=timeout)
 except BaseException as exc:
  error={'type':type(exc).__name__,'message':str(exc)}
  if p is not None:
   assert p.pid!=28722;os.killpg(p.pid,15)
   try:stdout,stderr=p.communicate(timeout=3)
   except subprocess.TimeoutExpired:os.killpg(p.pid,9);stdout,stderr=p.communicate(timeout=3)
 finally:
  (O/(name+'.stdout.log')).write_bytes(stdout);(O/(name+'.stderr.log')).write_bytes(stderr)
  event={'exitCode':None if p is None else p.returncode,'elapsedMs':round((time.monotonic()-began)*1000),'error':error};(O/(name+'.exit-public.json')).write_text(json.dumps(event,indent=2)+'\n')
 assert error is None and p.returncode==0,(name,event,stderr.decode()[-2000:]);return stdout
upload="from pathlib import Path;import sys,hashlib,os,json;b=sys.stdin.buffer.read(33554433);assert len(b)==19496960 and hashlib.sha256(b).hexdigest()=='eeb2b3aaa04fbeac43c0168bde4fd5b35969131e5faa0d564eaf2741d5f43d82';p=Path('/home/ubuntu/capella-sync-adverse-public-transfer-root-20261018-01.tar');f=p.open('xb');f.write(b);f.flush();os.fsync(f.fileno());f.close();p.chmod(0o400);print(json.dumps({'passed':True,'bytes':len(b),'privateAuthoritiesTransferred':False}))"
command('upload-public-only',prefix+['/usr/bin/python3 -c '+shlex.quote(upload)],bundle,120)
out=command('prepare-onsite',prefix+['/usr/bin/python3 - '+bp['sha256']+' '+str(bp['bytes'])],source,120)
v=json.loads(out);assert v['passed']and v['newContainers']==0 and v['newCppCompilations']==0
assert pin(B.read_bytes())==bp and pin(P.read_bytes())=={'sha256':'65d664666718cbd8a3992e3dad8a05c3cc82bc211ee68f1bb4e3f934cc3ae8fa','bytes':7216}
result={'passed':True,'publicBundle':bp,'onsitePreparation':v,'runtimeStarted':False,'productionQualified':False}
(O/'result-public.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
