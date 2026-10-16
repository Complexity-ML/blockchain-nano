#!/usr/bin/env python3
import base64,hashlib,json,os,pathlib,re,signal,stat,subprocess,sys,time,uuid
os.umask(0o077)
E=pathlib.Path(__file__).resolve().parent;R=pathlib.Path('/Users/boris/Dev/blockchain-nano');P=E/'bridge-source-serial-observation-proposal-20261016';W=E/'bridge-source-serial-linux-wrapper-20261016';O=E/'bridge-source-serial-linux-root-qualification-20261016';N=E/'stopped-backup-node-linux-arm64-20261016';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');REV='d6b6fee31c5d07e26ec7f2278e1ac012322cceff';IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';held={}
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

def clean_git():assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=R).decode().strip()==REV and subprocess.check_output(['git','status','--porcelain'],cwd=R)==b''

def interrupted(sig,frame):raise KeyboardInterrupt('owned unit command interrupted '+str(sig))

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

assert len(sys.argv)==3 and all(re.fullmatch('[0-9a-f]{64}',x)for x in sys.argv[1:]);clean_git()
mb=hold(P/'manifest.json');wb=hold(W/'manifest.json');assert pin(mb)['sha256']==sys.argv[1] and pin(wb)['sha256']==sys.argv[2]
mf=json.loads(mb);wm=json.loads(wb);assert 'FROZEN'in mf['status'] and 'FROZEN'in wm['status']
for root,manifest in [(P,mf),(W,wm)]:
 for a in manifest['payloads']:assert pin(hold(root/a['path']))=={k:a[k]for k in ['sha256','bytes']}
for a in mf['baselines']:
 b=hold(R/a['path']);assert pin(b)=={k:a[k]for k in ['sha256','bytes']};assert subprocess.check_output(['git','show',REV+':'+a['path']],cwd=R)==b
program=hold(W/'wrapper.mjs');assert len(program)<=32768
nodeRows=json.loads((N/'prospective-admission.private.json').read_bytes())['observations'][-1]['table']
for leaf,want in nodeRows.items():assert material(N/leaf)==want;held[str(N/leaf)]=want
phases={}
for mode in ['baseline','candidate']:
 files=[]
 for rel in ['tools/bridge-source/finality.mjs','tools/bridge-source/finality.test.mjs']:
  src=(P/'source'/rel)if(mode=='candidate'or rel.endswith('.test.mjs'))else(R/rel)
  b=hold(src);files.append({'path':rel,**pin(b),'contentBase64':base64.b64encode(b).decode()})
 phases[mode]={'files':files}
expectations={'baselineCounts':{'tests':4,'pass':0,'fail':4,'cancelled':0,'skipped':0,'todo':0},'candidateCounts':{'tests':35,'pass':35,'fail':0,'cancelled':0,'skipped':0,'todo':0},'baselineMissingExpectedRejections':3}
payload=(json.dumps({'format':'nano-bridge-source-serial-linux-unit-input-v1','phases':phases,'newTestPattern':'^synchronous RPC cache collisions cannot erase already fulfilled divergences$','expectations':expectations},sort_keys=True,separators=(',',':'))+'\n').encode();assert len(payload)<=262144
O.mkdir(mode=0o700);(O/'input-public.json').write_bytes(payload);save('host-inputs-before.private.json',held)
D=['/Applications/Docker.app/Contents/Resources/bin/docker','--config',str(E/'fresh-three-owner-current-20261016-02/docker-client'),'--host','unix:///Users/boris/.docker/run/docker.sock'];env={'HOME':'/Users/boris','PATH':'/usr/bin:/bin:/usr/sbin:/sbin','LANG':'C','LC_ALL':'C'}
signal.signal(signal.SIGTERM,interrupted)
name='nano-bridge-source-serial-'+uuid.uuid4().hex;cidPath=O/'unit.cid';q=subprocess.run(D+['container','inspect',name],capture_output=True,timeout=10,env=env);assert q.returncode==1 and(b'No such object'in q.stderr or b'No such container'in q.stderr)
args=D+['run','--interactive','--cidfile',str(cidPath),'--rm','--pull=never','--name',name,'--label','nano.doppler.unit='+name,'--network','none','--read-only','--user','501:20','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','512m','--cpus','1','--pids-limit','32','--tmpfs','/tmp:rw,nosuid,nodev,size=64m','--env','HOME=/tmp','--mount','type=bind,src='+str(N/'node')+',dst=/runtime/node,readonly','--entrypoint','/runtime/node',IMAGE,'--input-type=module','-e',program.decode()]
proc=None;firstError=None;cleanupError=None;cleanup=None;out=b'';err=b'';began=time.monotonic()
try:
 proc=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env);assert proc.pid!=28722;out,err=proc.communicate(input=payload,timeout=70)
except BaseException as exc:firstError=exc
finally:
 try:cleanup=cleanup_owned(name,cidPath)
 except BaseException as exc:cleanupError=type(exc).__name__+': '+str(exc)
 if cleanup is not None and not cleanup['passed']:cleanupError='CID refusal: '+str(cleanup.get('cidFileIssue'))
 if proc is not None and proc.poll()is None:
  try:out,err=proc.communicate(timeout=5)
  except subprocess.TimeoutExpired:
   if proc.pid!=28722:proc.terminate()
   try:out,err=proc.communicate(timeout=5)
   except subprocess.TimeoutExpired:
    if proc.pid!=28722:proc.kill()
    out,err=proc.communicate(timeout=5)
(O/'stdout.public.json').write_bytes(out);(O/'stderr.private.log').write_bytes(err);command={'exitCode':None if proc is None else proc.returncode,'elapsedMs':round((time.monotonic()-began)*1000),'firstError':None if firstError is None else type(firstError).__name__+': '+str(firstError),'cleanup':cleanup,'cleanupError':cleanupError,'image':IMAGE,'network':'none','wrapperPin':pin(program),'inputPin':pin(payload)};save('command-public.json',command)
after={p:material(p)for p in held};changes={p:[k for k in old if old[k]!=after[p].get(k)]for p,old in held.items()if old!=after[p]};save('host-input-fence-public.json',{'passed':not changes,'inputCount':len(held),'changedFields':changes,'oldAuthoritiesRefreshed':False})
if firstError:raise firstError
assert cleanupError is None and cleanup['passed'] and proc.returncode==0 and not changes and len(out)<=1048576
r=json.loads(out);assert r['passed'] and r['finalFence']['passed'] and len(r['phases'])==2
for phase in r['phases']:
 assert phase['oracleMatched'] and phase['postFence']['passed'];(O/(phase['phase']+'-stdout.tap')).write_text(phase['stdout']);(O/(phase['phase']+'-stderr.log')).write_text(phase['stderr'])
clean_git()
report={'format':'nano-bridge-source-serial-linux-root-qualification-v1','passed':True,'sourceRevision':REV,'proposalManifest':pin(mb),'wrapperManifest':pin(wb),'wrapperSource':pin(program),'linuxResult':pin(out),'command':command,'expectations':expectations,'hostInputCount':len(held),'sourceAndOldNodeIdentitiesUnchanged':True,'candidateFullObserverFilePassed':True,'oldObserverCausalMissingRejections':3,'sourceRpcTransportRerun':False,'newImagesBuilt':0,'cppCompilations':0,'transactions':0,'network':'none','eligibleForMint':False,'cryptographicFinalityVerified':False,'productionQualified':False}
save('qualification-public.json',report);print(json.dumps({'passed':True,'candidateTests':35,'oldObserverCausalMissingRejections':3,'newImagesBuilt':0,'productionQualified':False}))
