"""Root-only allowlisted AMD readback and offline five-vector verification."""
from pathlib import Path
import argparse,base64,hashlib,json,os,re,signal,stat,subprocess,sys,time
E=Path(__file__).resolve().parent;R=Path('/Users/boris/Dev/blockchain-nano')
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
NODE=Path('/opt/homebrew/Cellar/node/25.9.0_2/bin/node');held={};procs=[]
MODULE=R/'tests/qualification/rpc-proof.mjs';DRIVER=E/'verify-main-http-persistence-root-20261018.mjs'
RAW=E/'main-http-persistence-fixture-proposal-20261018/signed-raw-transactions.json'
NATIVE=E/'main-http-native-qualification-root-20261018-02/worker/proof-vectors.jsonl'
SIMPLE={'rpc-cases.jsonl','proof-vectors.jsonl','headers-public.json','result-public.json','input-final-fence-public.json','serve-config.json','config-first-admission-public.json'}
PATTERN=r'(?:rpc-0(?:0[1-9]|[1-4][0-9]|5[0-7])\.response\.json|phase-[012]-public\.json|serve-[012]\.(?:stdout\.log|stderr\.log|exit-public\.json|process-public\.json|readiness-public\.json)|mine-(?:12|24)\.(?:stdout\.log|stderr\.log|exit-public\.json|process-public\.json)|config-first-observation-(?:[1-9]|10)-public\.json)'
def need(v,m):
 if not v:raise ValueError(m)
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def row(s):return {k:getattr(s,k) for k in F}
def read(p,want=None,limit=268435456):
 need(p.is_absolute() and p.resolve(strict=True)==p,'physical input');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);r=row(a);need(stat.S_ISREG(a.st_mode) and a.st_nlink==1 and 0<=a.st_size<=limit,'input bound');b=b''
  while c:=os.read(fd,1048576):b+=c;need(len(b)<=a.st_size,'fixed length')
  need(len(b)==a.st_size and r==row(os.fstat(fd))==row(p.lstat()),'strict FD9');r['sha256']=pin(b)['sha256'];need(want is None or r==want,'held/original changed');return b,r
 finally:os.close(fd)
def known(p,sha,size):
 b,r=read(p);need(pin(b)=={'sha256':sha,'bytes':size},'known bytes');held[str(p)]=r;return b
def write(p,b):
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fchmod(f.fileno(),0o400);os.fsync(f.fileno());return dict(row(os.fstat(f.fileno())),sha256=pin(b)['sha256'])
def save(n,v):return write(O/n,(json.dumps(v,sort_keys=True,indent=2,allow_nan=False)+'\n').encode())
def run(label,argv,timeout,source=None):
 p=None;error=None;closed=False
 def alive():
  try:os.killpg(p.pid,0);return True
  except ProcessLookupError:return False
 def stop(sig):
  try:os.killpg(p.pid,sig)
  except ProcessLookupError:pass
 with (O/(label+'.stdout.log')).open('xb') as out,(O/(label+'.stderr.log')).open('xb') as err:
  try:
   p=subprocess.Popen(argv,stdin=subprocess.PIPE if source else subprocess.DEVNULL,stdout=out,stderr=err,start_new_session=True,env={'PATH':'/usr/bin:/bin','HOME':str(Path.home()),'LANG':'C','LC_ALL':'C'})
   need(p.pid>1 and p.pid!=28722,'owned PID');p.communicate(source,timeout=timeout)
  except BaseException as exc:error=exc
  finally:
   if p is not None and p.pid>1 and p.pid!=28722:
    try:
     if alive():stop(signal.SIGTERM)
     try:p.wait(timeout=3)
     except subprocess.TimeoutExpired:stop(signal.SIGKILL);p.wait(timeout=3)
     if alive():stop(signal.SIGKILL)
     end=time.monotonic()+3
     while alive() and time.monotonic()<end:time.sleep(.05)
     closed=not alive()
    except BaseException as exc:
     if error is None:error=exc
   event={'pid':None if p is None else p.pid,'exitCode':None if p is None else p.returncode,'joined':p is not None and p.returncode is not None,'groupClosed':closed};procs.append(dict(label=label,**event))
   try:save(label+'.exit-public.json',event)
   except BaseException as exc:
    errors.append(str(exc))
    if error is None:error=exc
 if error is not None:raise error
 need(p.returncode==0 and closed,label+' failed');return (O/(label+'.stdout.log')).read_bytes()
REMOTE=r'''
from pathlib import Path
import os,stat,hashlib,json,base64,re
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def row(s):return {k:getattr(s,k) for k in F}
root=Path('/home/ubuntu/main-http-amd-qualification-root-20261018-'+ATTEMPT)
assert root.resolve(strict=True)==root
paths=[root/n for n in ('result-public.json','host-final-fence-public.json','worker.stdout.log','worker.stderr.log') if (root/n).exists()]
assert all((root/n) in paths for n in ('result-public.json','host-final-fence-public.json'))
w=root/'outputs/worker'
if w.exists():
 assert w.resolve(strict=True)==w and w.is_dir()
 with os.scandir(w) as entries:
  for i,x in enumerate(entries):
   assert i<256
   if x.name in SIMPLE or re.fullmatch(PATTERN,x.name):assert x.is_file(follow_symlinks=False) and not x.is_symlink();paths.append(Path(x.path))
assert len(paths)<=115
packet={};total=0
for p in sorted(paths):
 assert p.resolve(strict=True)==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);r=row(a);assert stat.S_ISREG(a.st_mode) and a.st_nlink==1 and 0<=a.st_size<=1048576
  total+=a.st_size;assert total<=4194304;b=b''
  while c:=os.read(fd,65536):b+=c;assert len(b)<=a.st_size
  assert len(b)==a.st_size and r==row(os.fstat(fd))==row(p.lstat())
  packet[str(p.relative_to(root))]={'row':dict(r,sha256=hashlib.sha256(b).hexdigest()),'base64':base64.b64encode(b).decode()}
 finally:os.close(fd)
print(json.dumps({'format':'nano-main-http-amd-public-packet-v1','attempt':ATTEMPT,'bytes':total,'files':packet}))
'''
parser=argparse.ArgumentParser();parser.add_argument('source_sha256');parser.add_argument('--attempt',choices=['01','02'],default='02');a=parser.parse_args()
O=E/('main-http-amd-public-readback-root-20261018-01' if a.attempt=='02' else 'main-http-amd-public-readback-root-20261018-failed01')
need(not O.exists(),'fresh readback namespace');O.mkdir(mode=0o700);first=None;errors=[];changed=[];proof=None;host=None;copied=False
try:
 own=Path(__file__).resolve();b,r=read(own);need(re.fullmatch('[0-9a-f]{64}',a.source_sha256) and r['sha256']==a.source_sha256 and stat.S_IMODE(r['st_mode'])==0o400,'externally pinned frozen collector');held[str(own)]=r
 old=json.loads(known(E/'rpc-proof-verifier-js-root-20261017-01/new-input-admission.json','1f20ae6cde3a0de220376b260395f86e1eef025306d5154b2889fbbe705440a6',8680))['samples'][-1]
 for p in (NODE,R/'tests/qualification/account-proof.mjs',R/'tests/hardhat/package.json',R/'tests/hardhat/node_modules/ethers/package.json'):
  need(str(p) in old,'original Node row');read(p,old[str(p)]);held[str(p)]=old[str(p)]
 known(MODULE,'b59224b4a64231d493ccf21b98d4c5ddb5f5ce3ec9889b792190d0e0e0b090ce',5340);known(DRIVER,'d9900fc7e431e5e89ff90ed0a88f22cbd116b2eec52248595de63aaafb3e0c0f',4434)
 known(RAW,'8220db3001ba2c25dc99646046bc205c7cbebc3d72bd4847b874f9e8cbe15db3',209);nv=known(NATIVE,'6b60c1c6ecd2a3e51c4a88ea6a5cd656a0dfaa7660785b9063c0ec5f7203f288',13075)
 dispatchPin={'01':('d26a9ed1afc85cb06472d447decf123cca43a3fd5077b23de377a5b4af3f0483',1538),'02':('ad71956ccee7cdbc64e4923ed28bf58585699fa3f914b028f50acf110852e1f5',6064)}[a.attempt]
 dispatch=json.loads(known(E/('main-http-amd-dispatch-root-20261018-'+a.attempt+'/stdout.log'),*dispatchPin))
 source=('ATTEMPT='+repr(a.attempt)+'\nSIMPLE='+repr(SIMPLE)+'\nPATTERN='+repr(PATTERN)+'\n'+REMOTE).encode();save('remote-source-public.json',{'sha256':pin(source),'readOnly':True});rawpacket=run('ssh',['/usr/bin/ssh','-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','/usr/bin/python3','-'],60,source)
 need(len(rawpacket)<=6*1024*1024,'packet bound');p=json.loads(rawpacket);need(p['format']=='nano-main-http-amd-public-packet-v1' and p['attempt']==a.attempt and type(p['files']) is dict and len(p['files'])<=115,'packet schema');new={};total=0
 for rel,x in p['files'].items():
  q=Path(rel);need(not q.is_absolute() and '..' not in q.parts and q.as_posix()==rel and '.private' not in rel,'public relative name');need(len(q.parts)==1 and rel in ('result-public.json','host-final-fence-public.json','worker.stdout.log','worker.stderr.log') or len(q.parts)==3 and q.parts[:2]==('outputs','worker') and (q.name in SIMPLE or re.fullmatch(PATTERN,q.name)),'direct public file')
  b=base64.b64decode(x['base64'],validate=True);need(len(b)<=1048576 and x['row']['st_size']==len(b) and x['row']['sha256']==pin(b)['sha256'],'packet pin');total+=len(b);need(total<=4194304,'total bound');dest=O/'files'/q;dest.parent.mkdir(parents=True,exist_ok=True);new[str(dest)]=write(dest,b)
 need(total==p['bytes'],'packet total');prev=None;streak=0;since=start=time.monotonic();sample=0
 while True:
  rows={k:read(Path(k),limit=1048576)[1] for k in new};sample+=1;save('copy-observation-'+str(sample)+'.json',rows);now=time.monotonic()
  need(all({k:v for k,v in rows[f].items() if k!='st_ctime_ns'}=={k:v for k,v in new[f].items() if k!='st_ctime_ns'} for f in new) and now-start<30,'known copies before first hold')
  if rows==prev:streak+=1
  else:streak=1;since=now
  if streak>=3 and now-since>=2:break
  prev=rows;time.sleep(1.05)
 held.update(rows);load=lambda n:json.loads(read(O/'files'/n,held[str(O/'files'/n)],1048576)[0]);host=load('result-public.json');fence=load('host-final-fence-public.json');copied=True
 need(host==dispatch,'readback differs from actual Root terminal');need(fence['passed'] is True,'host final fence');
 if host['passed'] is False:
  need(a.attempt=='01','unexpected failed current host')
 else:
  w=load('outputs/worker/result-public.json');need(host['worker']==w and host['workerExitCode']==0 and host['cleanup']['passed'] is True and host['hostFinalFenceAfterCleanup'] is True,'host terminal');need(w['passed'] is True and [w[k] for k in ('httpCalls','processStarts','serverStarts','mineCommands','cleanStops','proofVectors')]==[57,5,3,2,3,5] and w['allOwnedProcessesJoined'] is True and w['inputFencePassed'] is True,'whole worker')
  need(all(str(O/('files/outputs/worker/rpc-%03d.response.json'%i)) in held for i in range(1,58)),'57 response files');vf=O/'files/outputs/worker/proof-vectors.jsonl';vb=read(vf,held[str(vf)],1048576)[0];need([json.loads(x) for x in vb.splitlines()]==[json.loads(x) for x in nv.splitlines()],'five complete Native/AMD vectors differ');need(load('worker.stdout.log')==w and not read(O/'files/worker.stderr.log',held[str(O/'files/worker.stderr.log')])[0],'worker stdout/stderr')
  for path,want in held.items():read(Path(path),want)
  output=run('node',[str(NODE),str(DRIVER),str(vf),pin(vb)['sha256'],str(RAW),held[str(RAW)]['sha256'],str(MODULE)],30);proof=json.loads(output);need(not (O/'node.stderr.log').read_bytes() and proof['passed'] is True and proof['verifiedVectors']==5 and proof['executionHeaderHashesVerified']==3 and proof['newSignaturesCreated']==0,'independent actual five vectors')
except BaseException as exc:first={'type':type(exc).__name__,'message':str(exc)}
finally:
 for path,want in held.items():
  try:read(Path(path),want)
  except BaseException as exc:changed.append({'path':path,'error':str(exc)})
 try:save('final-input-fence-public.json',{'passed':not changed,'files':len(held),'changes':changed,'oldRowsRefreshed':False})
 except BaseException as exc:errors.append({'stage':'fence-publication','error':str(exc)})
 result={'format':'nano-main-http-amd-offline-audit-v1','passed':first is None and not errors and not changed and proof is not None,'collectionPassed':copied and first is None,'readbackFiles':len(new) if copied else 0,'readbackBytes':total if copied else 0,'nativeCompleteVectorsEqual':proof is not None,'attempt':a.attempt,'hostPassed':None if host is None else host['passed'],'independent':proof,'processes':procs,'firstFailure':first,'secondaryFailures':errors,'inputFencePassed':not changed,'liveRpcCalls':0,'newCppCompilations':0,'oldJsTestsReplayed':0,'consensusRootAuthenticated':False,'productionQualified':False}
 try:save('result-public.json',result)
 except BaseException as exc:result['passed']=False;errors.append({'stage':'result-publication','error':str(exc)})
 print(json.dumps(result),flush=True)
sys.exit(0 if result['passed'] or a.attempt=='01' and result['collectionPassed'] and not changed and not errors else 1)
