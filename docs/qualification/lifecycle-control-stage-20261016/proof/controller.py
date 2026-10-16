#!/usr/bin/env python3
import ast,base64,hashlib,json,os,pathlib,re,signal,stat,subprocess,sys,time,uuid
os.umask(0o077)
E=pathlib.Path(__file__).resolve().parent;R=pathlib.Path('/Users/boris/Dev/blockchain-nano');P=E/'lifecycle-control-stage-stop-receipts-source-proposal-20261016';O=E/'lifecycle-control-stage-stop-linux-v2-root-qualification-20261016';N=E/'stopped-backup-node-linux-arm64-20261016';IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');held={}
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def material(p):
 p=pathlib.Path(p);assert p.resolve(strict=True)==p;fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);assert stat.S_ISREG(s.st_mode)and s.st_nlink==1 and 0<s.st_size<=268435456;r={k:getattr(s,k)for k in F};h=hashlib.sha256();n=0
  while b:=os.read(fd,1048576):n+=len(b);h.update(b)
  assert n==s.st_size and r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F};return dict(r,sha256=h.hexdigest())
 finally:os.close(fd)
def hold(p):
 p=pathlib.Path(p);r=material(p)
 if str(p)in held:assert r==held[str(p)]
 else:held[str(p)]=r
 b=p.read_bytes();assert len(b)==r['st_size']and hashlib.sha256(b).hexdigest()==r['sha256']and material(p)==r;return b
def save(leaf,value):
 p=O/leaf
 with p.open('x')as f:json.dump(value,f,sort_keys=True,indent=2);f.write('\n');f.flush();os.fsync(f.fileno())
assert len(sys.argv)==2 and re.fullmatch('[0-9a-f]{64}',sys.argv[1]);assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=R).decode().strip()=='e05ea5b9ff5bc8282a9fc120c079bdbddc29f7e6';subprocess.run(['git','diff','--exit-code','--quiet','HEAD','--','tests/multihost','tests/capella','scripts/operations'],cwd=R,check=True)
mb=hold(P/'manifest.json');assert pin(mb)['sha256']==sys.argv[1];mf=json.loads(mb);assert 'FROZEN'in mf['status'];replacements={}
for x in mf['baselines']:assert pin(hold(R/x['path']))=={k:x[k]for k in ['sha256','bytes']}
for x in mf['payloads']:
 b=hold(P/x['path']);assert pin(b)=={k:x[k]for k in ['sha256','bytes']}
 if 'target'in x:replacements[x['target']]=b
nodeRows=json.loads((N/'prospective-admission.private.json').read_bytes())['observations'][-1]['table']
for leaf,want in nodeRows.items():assert material(N/leaf)==want;held[str(N/leaf)]=want
wrapperPath=E/'lifecycle-stage-stop-linux-unit-root-v2-20261016.mjs';began=time.monotonic();prior=None;rounds=0;since=began
while True:
 current=material(wrapperPath);now=time.monotonic();assert now-began<30
 if current==prior:rounds+=1
 else:prior=current;rounds=1;since=now
 if rounds>=3 and now-since>=2:break
 time.sleep(1.05)
program=hold(wrapperPath);assert len(program)<32768
queue=['tests/multihost/vps-linux-bootstrap.test.mjs','tests/multihost/lifecycle-runtime.test.mjs','tests/multihost/lifecycle-driver-guard.test.mjs','tests/multihost/run.mjs'];sources={}
while queue:
 rel=queue.pop()
 if rel in sources:continue
 b=replacements.get(rel)
 if b is None:b=hold(R/rel)
 else:hold(R/rel)
 sources[rel]=b
 for _,name in re.findall(r'(?:from\s*|import\s*\()([\'\"])([^\'\"]+)\1',b.decode()):
  if name.startswith('.'):
   q=((R/rel).parent/name).resolve();assert q.is_relative_to(R);queue.append(str(q.relative_to(R)))
for rel in ['docs/qualification/local-supervisor-linux-20261015/source/known-files.pl','docs/qualification/local-supervisor-linux-20261015/source/copy-bootstrap-pairs.pl']:sources[rel]=hold(R/rel)
def rows(items):return [{'path':rel,**pin(b),'contentBase64':base64.b64encode(b).decode()}for rel,b in sorted(items.items())]
baseline={'tests/multihost/vps-linux-bootstrap.test.mjs':sources['tests/multihost/vps-linux-bootstrap.test.mjs'],'tests/multihost/vps-linux-bootstrap.mjs':hold(R/'tests/multihost/vps-linux-bootstrap.mjs')}
payload=(json.dumps({'format':'nano-lifecycle-stage-stop-linux-unit-input-v1','projects':[{'name':'baseline','files':rows(baseline)},{'name':'candidate','files':rows(sources)}]},separators=(',',':'))+'\n').encode();assert len(payload)<=4194304;O.mkdir(mode=0o700);(O/'input-public.json').write_bytes(payload);save('host-source-before.private.json',held)
D=['/Applications/Docker.app/Contents/Resources/bin/docker','--config',str(E/'fresh-three-owner-current-20261016-02/docker-client'),'--host','unix:///Users/boris/.docker/run/docker.sock'];env={'HOME':'/Users/boris','PATH':'/usr/bin:/bin:/usr/sbin:/sbin','LANG':'C','LC_ALL':'C'}
def interrupted(sig,frame):raise KeyboardInterrupt('owned unit command interrupted '+str(sig))
signal.signal(signal.SIGTERM,interrupted)
def cleanup_owned(name,cidPath):
 result={};old={sig:signal.signal(sig,signal.SIG_IGN)for sig in [signal.SIGINT,signal.SIGTERM]}
 try:
  cid=None;cidIssue=None
  if cidPath.exists():
   try:
    before=material(cidPath);assert before['st_size']<=65;b=cidPath.read_bytes();assert material(cidPath)==before and len(b)==before['st_size']and hashlib.sha256(b).hexdigest()==before['sha256'];cid=b.decode().strip();assert re.fullmatch('[0-9a-f]{64}',cid)
   except Exception as exc:cid=None;cidIssue=type(exc).__name__+': '+str(exc)
  q=subprocess.run(D+['container','inspect',cid or name],capture_output=True,timeout=10,env=env)
  if q.returncode!=0:
   assert q.returncode==1 and (b'No such object'in q.stderr or b'No such container'in q.stderr);return {'passed':cidIssue is None,'alreadyAbsent':True,'cidObserved':cid is not None,'cidFileIssue':cidIssue}
  data=json.loads(q.stdout);assert len(data)==1;x=data[0];actual=x['Id'];assert re.fullmatch('[0-9a-f]{64}',actual)and(cid is None or actual==cid)and x['Name']=='/'+name and x['Image']==IMAGE and x['Config']['Labels']['nano.doppler.unit']==name
  if x['State']['Running']:subprocess.run(D+['stop','--time','5',actual],capture_output=True,timeout=12,env=env)
  q=subprocess.run(D+['container','inspect',actual],capture_output=True,timeout=10,env=env)
  if q.returncode==0:
   x=json.loads(q.stdout)[0];assert x['Id']==actual and not x['State']['Running'];subprocess.run(D+['rm',actual],capture_output=True,timeout=10,env=env)
  q=subprocess.run(D+['container','inspect',actual],capture_output=True,timeout=10,env=env);assert q.returncode==1 and(b'No such object'in q.stderr or b'No such container'in q.stderr)
  return {'passed':cidIssue is None,'cidFileIssue':cidIssue,'identityVerifiedBeforeStop':True,'id':actual,'name':name,'image':IMAGE,'label':name,'absentAfterCleanup':True}
 finally:
  for sig,handler in old.items():signal.signal(sig,handler)

name='nano-lifecycle-stage-unit-'+uuid.uuid4().hex;cidPath=O/'unit.cid';pre=subprocess.run(D+['container','inspect',name],capture_output=True,timeout=10,env=env);assert pre.returncode==1 and(b'No such object'in pre.stderr or b'No such container'in pre.stderr)
args=D+['run','--interactive','--cidfile',str(cidPath),'--rm','--pull=never','--name',name,'--label','nano.doppler.unit='+name,'--network','none','--read-only','--user','501:20','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','768m','--cpus','1','--pids-limit','64','--tmpfs','/tmp:rw,nosuid,nodev,size=64m','--env','HOME=/tmp','--mount','type=bind,src='+str(N/'node')+',dst=/runtime/node,readonly','--entrypoint','/runtime/node',IMAGE,'--input-type=module','-e',program.decode()]
proc=None;firstError=None;cleanupError=None;out=b'';err=b'';cleanup=None;began=time.monotonic()
try:
 proc=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env);assert proc.pid!=28722;out,err=proc.communicate(input=payload,timeout=105)
except BaseException as exc:firstError=exc
finally:
 try:cleanup=cleanup_owned(name,cidPath)
 except BaseException as exc:cleanupError=type(exc).__name__+': '+str(exc)
 if cleanup is not None and not cleanup['passed']:cleanupError='New CID file refused: '+str(cleanup.get('cidFileIssue'))
 if proc is not None and proc.poll()is None:
  try:out,err=proc.communicate(timeout=5)
  except subprocess.TimeoutExpired:
   if proc.pid!=28722:proc.terminate()
   try:out,err=proc.communicate(timeout=5)
   except subprocess.TimeoutExpired:
    if proc.pid!=28722:proc.kill()
    out,err=proc.communicate(timeout=5)
(O/'stdout.public.json').write_bytes(out);(O/'stderr.private.log').write_bytes(err);save('command-public.json',{'exitCode':None if proc is None else proc.returncode,'elapsedMs':round((time.monotonic()-began)*1000),'firstError':None if firstError is None else type(firstError).__name__+': '+str(firstError),'cleanup':cleanup,'cleanupError':cleanupError,'image':IMAGE,'network':'none','stdinPin':pin(payload),'wrapperPin':pin(program)})
after={p:material(p)for p in held};save('host-source-after.private.json',after);changes={p:[k for k in r if r[k]!=after[p].get(k)]for p,r in held.items()if r!=after[p]};save('host-source-fence-public.json',{'passed':not changes,'knownInputCount':len(held),'changedFields':changes,'oldAuthorityRefreshed':False})
if firstError:raise firstError
assert not changes and cleanupError is None and cleanup['passed']and proc.returncode==0 and len(out)<=1048576
r=json.loads(out);assert r['format']=='nano-lifecycle-stage-stop-linux-unit-result-v1'and r['passed']and len(r['phases'])==2 and r['finalFence']['passed'];assert r['admission']['observations']>=3 and 2000<=r['admission']['stableSpanMs']<=r['admission']['elapsedMs']<30000
for phase in r['phases']:
 (O/(phase['name']+'-stdout.tap')).write_text(phase['stdout']);(O/(phase['name']+'-stderr.log')).write_text(phase['stderr']);assert phase['postFence']['passed']
assert r['phases'][0]['expectedBaselineRefusal']and r['phases'][1]['candidatePassed'];count=r['phases'][1]['summary']['pass'];save('result-public.json',{'format':'nano-lifecycle-stage-stop-root-qualification-v1','passed':True,'sourceBaseRevision':'e05ea5b9ff5bc8282a9fc120c079bdbddc29f7e6','sourceProposal':pin(mb),'wrapper':pin(program),'rawLinuxResult':pin(out),'candidateTestsPassed':count,'expectedBaselineRefusal':True,'sourceFilesProjected':len(sources)+len(baseline),'linuxSourceNineAndShaUnchanged':True,'hostOldSourceAndNodeAuthoritiesUnchanged':True,'freshDomainReserved':False,'signaturesCreated':0,'campaignRestarted':False,'previousCampaignRequalified':False,'cppCompilations':0,'imageBuilds':0,'network':'none','productionQualified':False});print(json.dumps({'passed':True,'candidateTestsPassed':count,'expectedBaselineRefusal':True,'linuxSourceNineAndShaUnchanged':True,'hostSourceAndNodeNinePreserved':True,'previousCampaignRequalified':False,'productionQualified':False}))
