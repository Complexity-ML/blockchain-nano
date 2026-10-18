
from pathlib import Path
import hashlib,json,os,stat,time,shutil
BASE=Path('/home/ubuntu/main-http-python-provider-root-20261018-01');ROOT=BASE/'runtime';LIB=Path('/usr/lib/python3.12');BIN=Path('/usr/bin/python3.12');F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');os.umask(0o077)
assert os.getuid()==os.getgid()==1000 and not BASE.exists() and BASE.parent.resolve(strict=True)==BASE.parent and shutil.disk_usage(BASE.parent).free>268435456
aliases={'_sysconfigdata__linux_x86_64-linux-gnu.py':'/usr/lib/python3.12/_sysconfigdata__x86_64-linux-gnu.py','sitecustomize.py':'/etc/python3.12/sitecustomize.py','config-3.12-x86_64-linux-gnu/libpython3.12.so':'/usr/lib/x86_64-linux-gnu/libpython3.12.so.1.0'}
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def material(p):
 assert p.resolve(strict=True)==p
 with p.open('rb') as f:
  s=os.fstat(f.fileno());assert stat.S_ISREG(s.st_mode) and 0<s.st_size<=33554432;b=f.read();a={k:getattr(s,k) for k in F};assert len(b)==s.st_size and a=={k:getattr(os.fstat(f.fileno()),k) for k in F}=={k:getattr(p.stat(),k) for k in F}
 return b,dict(a,sha256=pin(b)['sha256'])
BASE.mkdir(mode=0o700);ROOT.mkdir(mode=0o700);known={};origins={};seen_aliases={}
def copy(rel,p,source_alias=None):
 b,row=material(p);q=ROOT/rel;q.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
 with q.open('xb') as f:f.write(b);f.flush();os.fsync(f.fileno())
 q.chmod(0o500 if rel=='bin/python3' else 0o400);assert q.read_bytes()==b
 known[rel]=pin(b);origins[rel]={'source':str(p),'sourceRow':row,'materializedAlias':source_alias}
copy('bin/python3',BIN);assert known['bin/python3']=={'sha256':'e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f','bytes':8020928}
for p in sorted(LIB.rglob('*')):
 if '__pycache__' in p.parts or p.suffix in ('.pyc','.pyo'):continue
 rel=str(p.relative_to(LIB));dst='lib/python3.12/'+rel
 if p.is_symlink():
  target=str(p.resolve(strict=True));assert aliases.get(rel)==target;seen_aliases[rel]=target;copy(dst,Path(target),{'path':str(p),'link':os.readlink(p)})
 elif p.is_file():copy(dst,p)
 elif p.is_dir():(ROOT/dst).mkdir(parents=True,exist_ok=True,mode=0o700)
 else:raise ValueError('unsupported stdlib object')
assert seen_aliases==aliases and len(known)==628
for p in sorted((x for x in ROOT.rglob('*') if x.is_dir()),key=lambda p:len(p.parts),reverse=True):p.chmod(0o500)
ROOT.chmod(0o500);dirs=sorted(['.']+[str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_dir()]);previous=None;count=0;since=None;start=time.monotonic();samples=0
while True:
 rows={};mismatches=[]
 for rel,w in known.items():
  b,row=material(ROOT/rel);rows[rel]=row
  if pin(b)!=w:mismatches.append(rel)
 now=time.monotonic();samples+=1
 with (BASE/('admission-observation-'+str(samples)+'.json')).open('x') as f:json.dump({'elapsedMs':round((now-start)*1000),'rows':rows,'mismatches':mismatches},f);f.write('\n');f.flush();os.fsync(f.fileno())
 assert not mismatches and now-start<60
 if rows==previous:count+=1
 else:count=1;since=now
 if count>=3 and now-since>=2:break
 previous=rows;time.sleep(1.05)
env={'PYTHONHOME':'/python-runtime','PYTHONDONTWRITEBYTECODE':'1','PYTHONNOUSERSITE':'1','PYTHONSAFEPATH':'1','PYTHONPYCACHEPREFIX':'/tmp/nano-python-cache'}
c={'format':'nano-test-python-runtime-control-v1','ready':True,'runtimeRoot':str(ROOT),'guestRoot':'/python-runtime','interpreter':'bin/python3','version':'3.12.3','files':rows,'directories':dirs,'environment':env,'flags':['-S'],'provenance':{'sourceBinary':str(BIN),'sourceStdlib':str(LIB),'sourceAliasesMaterialized':aliases,'cachedBytecodeExcluded':True,'sourceAndCopyBytesMatched':True,'prospectiveAdmissionSamples':samples,'originalAuthoritiesRefreshed':False,'newDependenciesInstalled':0,'newCompilations':0,'testRuntimeOnly':True}}
p=BASE/'control.json';p.write_text(json.dumps(c,indent=2,sort_keys=True)+'\n');p.chmod(0o400);b=p.read_bytes();(BASE/'source-provenance.json').write_text(json.dumps(origins,indent=2)+'\n')
print(json.dumps({'format':'nano-test-python-runtime-producer-v1','passed':True,'control':str(p),'controlPin':pin(b),'runtimeRoot':str(ROOT),'providerFiles':len(rows),'providerBytes':sum(r['st_size'] for r in rows.values()),'directories':len(dirs),'sourceAliasesMaterialized':3,'cachedBytecodeExcluded':True,'admissionSamples':samples,'knownBytePinsHeld':True,'newDependenciesInstalled':0,'newCompilations':0,'pythonRuntimeExecuted':False,'nanoProcessesStarted':0,'productionQualified':False}))
