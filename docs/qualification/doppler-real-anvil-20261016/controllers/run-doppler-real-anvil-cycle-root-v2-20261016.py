#!/usr/bin/env python3
import base64,hashlib,json,os,pathlib,re,signal,stat,subprocess,time,uuid
os.umask(0o077)
E=pathlib.Path(__file__).resolve().parent;R=pathlib.Path('/Users/boris/Dev/blockchain-nano');O=E/'doppler-lp-real-anvil-cycle-v2-root-20261016';N=E/'stopped-backup-node-linux-arm64-20261016';T=E/'anvil-linux-arm64-1.5.1-official-20261016';IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';REV='f851c3bd52b9ef59a690748a37a1cd4668c45b5e';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');held={}
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
 b=p.read_bytes();assert pin(b)=={'sha256':r['sha256'],'bytes':r['st_size']}and material(p)==r;return b
def save(leaf,v):
 with(O/leaf).open('x')as f:json.dump(v,f,indent=2);f.write('\n');f.flush();os.fsync(f.fileno())
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=R).decode().strip()==REV and subprocess.check_output(['git','status','--porcelain'],cwd=R)==b''
nodeRows=json.loads((N/'prospective-admission.private.json').read_bytes())['observations'][-1]['table']
for leaf,want in nodeRows.items():assert material(N/leaf)==want;held[str(N/leaf)]=want
admission=json.loads(hold(T/'admission-public.json'));want=dict(admission['binary']['material'],sha256=admission['binary']['sha256']);assert material(T/'anvil')==want;held[str(T/'anvil')]=want
moduleRoot=R/'tools/doppler-launch/node_modules';assert moduleRoot.resolve(strict=True)==moduleRoot;lock=json.loads(hold(R/'tools/doppler-launch/package-lock.json'));versions={}
for pkg in ['viem','@whetstone-research/doppler-sdk','doppler-router']:
 v=json.loads(hold(moduleRoot/pkg/'package.json'))['version'];assert v==lock['packages']['node_modules/'+pkg]['version'];versions[pkg]=v
assert versions=={'viem':'2.33.3','@whetstone-research/doppler-sdk':'1.0.43','doppler-router':'1.0.15'}
queue=['tools/doppler-launch/fork-lifecycle.mjs','tools/doppler-launch/example.synthetic.json','tools/doppler-launch/package.json'];sources={}
while queue:
 rel=queue.pop()
 if rel in sources:continue
 b=hold(R/rel);assert subprocess.check_output(['git','show',REV+':'+rel],cwd=R)==b;sources[rel]=b
 if rel.endswith('.mjs'):
  for _,name in re.findall(r'(?:from\s*|import\s*\()([\'\"])([^\'\"]+)\1',b.decode()):
   if name.startswith('.'):
    p=((R/rel).parent/name).resolve();assert p.is_relative_to(R);queue.append(str(p.relative_to(R)))
wp=E/'doppler-anvil-real-cycle-linux-root-20261016.mjs';began=time.monotonic();prior=None;count=0;since=began
while True:
 row=material(wp);now=time.monotonic();assert now-began<30
 if row==prior:count+=1
 else:prior=row;count=1;since=now
 if count>=3 and now-since>=2:break
 time.sleep(1.05)
program=hold(wp);payload=(json.dumps({'format':'nano-doppler-real-anvil-input-v1','files':[{'path':p,**pin(b),'contentBase64':base64.b64encode(b).decode()}for p,b in sorted(sources.items())]},separators=(',',':'))+'\n').encode();assert len(payload)<2097152;O.mkdir(mode=0o700);proof=O/'proof';proof.mkdir(mode=0o700);(O/'input-public.json').write_bytes(payload);save('host-inputs-before.private.json',held)
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

name='nano-doppler-real-cycle-'+uuid.uuid4().hex;cidPath=O/'owned.cid';q=subprocess.run(D+['container','inspect',name],capture_output=True,timeout=10,env=env);assert q.returncode==1 and(b'No such object'in q.stderr or b'No such container'in q.stderr)
args=D+['run','--interactive','--cidfile',str(cidPath),'--rm','--pull=never','--name',name,'--label','nano.doppler.unit='+name,'--network','bridge','--read-only','--user','501:20','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','1024m','--cpus','1','--pids-limit','64','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount','type=bind,src='+str(N/'node')+',dst=/runtime/node,readonly','--mount','type=bind,src='+str(T/'anvil')+',dst=/runtime/anvil,readonly','--mount','type=bind,src='+str(moduleRoot)+',dst=/deps/node_modules,readonly','--mount','type=bind,src='+str(proof)+',dst=/proof','--entrypoint','/runtime/node',IMAGE,'--input-type=module','-e',program.decode()]
proc=None;firstError=None;cleanupError=None;cleanup=None;out=b'';err=b'';began=time.monotonic()
save('launch-public.json',{'sourceRevision':REV,'name':name,'image':IMAGE,'network':'bridge for read-only Base fork upstream; local writes remain loopback inside container','portsPublished':0,'personalKeysLoaded':False,'newConsensusIdentities':0,'sourceFiles':len(sources),'inputPin':pin(payload),'wrapperPin':pin(program),'maximumOuterSeconds':360,'resources':{'cpu':1,'memoryMiB':1024,'pids':64,'tmpfsMiB':256},'newImagesBuilt':0})
try:
 proc=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env);assert proc.pid!=28722;out,err=proc.communicate(input=payload,timeout=360)
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
(O/'stdout.public.json').write_bytes(out);(O/'stderr.private.log').write_bytes(err);save('command-public.json',{'exitCode':None if proc is None else proc.returncode,'elapsedMs':round((time.monotonic()-began)*1000),'firstError':None if firstError is None else type(firstError).__name__+': '+str(firstError),'cleanup':cleanup,'cleanupError':cleanupError});after={p:material(p)for p in held};save('host-inputs-after.private.json',after);changes={p:[k for k in r if r[k]!=after[p].get(k)]for p,r in held.items()if r!=after[p]};save('host-input-fence-public.json',{'passed':not changes,'inputCount':len(held),'changedFields':changes,'oldAuthoritiesRefreshed':False})
if firstError:raise firstError
assert cleanupError is None and cleanup['passed']and not changes and proc.returncode==0
r=json.loads((proof/'result-public.json').read_bytes());assert r['passed']and r['finalSourceFence']['passed']and r['cycle']['independentPinnedStateCalls']==6 and all(x['closed']for x in r['children'])
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=R).decode().strip()==REV and subprocess.check_output(['git','status','--porcelain'],cwd=R)==b''
save('qualification-public.json',{'format':'nano-real-anvil-lp-cycle-root-qualification-v1','passed':True,'sourceRevision':REV,'childResultPin':pin((proof/'result-public.json').read_bytes()),'cycleResultPin':pin((proof/'cycle/result.json').read_bytes()),'independentProofPin':pin((proof/'independent-real-proof.public.json').read_bytes()),'sourceAndOldToolAuthoritiesUnchanged':True,'dependencyVersionsMatchLock':versions,'dependencyTreeReproducibilityQualified':False,'publicPortPublished':False,'publicTransactionsSubmitted':0,'newConsensusIdentities':0,'originalPrivateCampaignRequalified':False,'nativeNanoQualified':False,'productionQualified':False});print(json.dumps({'passed':True,'realAnvilCycleComplete':True,'independentPinnedStateCalls':6,'threeActualAdversarialRejections':True,'originalPrivateCampaignRequalified':False,'productionQualified':False}))
