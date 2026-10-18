"""Affected CI registration guard only. Its CTest/build programs are fixture stubs."""
from pathlib import Path
import hashlib,json,os,stat,subprocess,time
E=Path(__file__).resolve().parent
S=E/'capella-sync-adverse-public-source-root-20261018-02'
C=E/'capella-sync-adverse-arm-control-root-20261018-02/control.private.json'
O=E/'capella-sync-adverse-ci-guard-root-20261018-02'
N=Path('/opt/homebrew/Cellar/node/25.9.0_2/bin/node')
CM=Path('/opt/homebrew/Cellar/cmake/4.4.3/bin/cmake')
G=Path('/Library/Developer/CommandLineTools/usr/bin/git')
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def row(p):
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);v={k:getattr(a,k)for k in F};h=hashlib.sha256();n=0
  assert stat.S_ISREG(a.st_mode)and a.st_size<=268435456
  while b:=os.read(fd,1048576):h.update(b);n+=len(b)
  assert n==a.st_size and v=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  return dict(v,sha256=h.hexdigest())
 finally:os.close(fd)
assert not O.exists();O.mkdir(mode=0o700)
control=json.loads(C.read_bytes());held={}
for rel in('scripts/ci/check-core.test.mjs','scripts/ci/check-core.cmake','scripts/ci/core.sh','scripts/ci/verify-results.sh','scripts/ci/operations.sh'):
 p=S/rel;held[str(p)]=row(p);assert held[str(p)]==control['originalFiles'][str(p)]
for p in(N,CM,G):held[str(p)]=row(p)
(O/'inputs.private.json').write_text(json.dumps(held,indent=2)+'\n')
args=[str(N),'--test',str(S/'scripts/ci/check-core.test.mjs')]
env=dict(os.environ,PATH=str(CM.parent)+':'+str(N.parent)+':'+str(G.parent)+':/usr/bin:/bin:/usr/sbin:/sbin')
began=time.monotonic();first=None;rc=None
try:
 with(O/'stdout.log').open('xb')as out,(O/'stderr.log').open('xb')as err:
  p=subprocess.Popen(args,cwd=S,env=env,stdin=subprocess.DEVNULL,stdout=out,stderr=err,start_new_session=True)
  try:rc=p.wait(timeout=120)
  except BaseException:
   os.killpg(p.pid,15)
   try:p.wait(timeout=3)
   except subprocess.TimeoutExpired:os.killpg(p.pid,9);p.wait(timeout=3)
   raise
 assert rc==0,'affected Node/CMake CI registration guard failed'
except BaseException as exc:first={'type':type(exc).__name__,'message':str(exc)}
changes=[]
for path,want in held.items():
 try:assert row(Path(path))==want
 except BaseException as exc:changes.append({'path':path,'error':str(exc)})
result={'format':'nano-capella-sync-ci-guard-v1','passed':first is None and not changes,'exitCode':rc,'elapsedMs':round((time.monotonic()-began)*1000),'args':args,'inputRows':len(held),'finalFencePassed':not changes,'firstFailure':first,'changes':changes,'actualCppTestsRun':0,'actualProjectBuilds':0,'cmakeProjectConfigures':0,'productionQualified':False}
(O/'result-public.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));raise SystemExit(0 if result['passed']else 1)
