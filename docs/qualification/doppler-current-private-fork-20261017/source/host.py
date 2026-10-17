#!/usr/bin/env python3
# Derived from the published O2 controller; definition/import is not a dry-run API.
import base64,hashlib,json,os,pathlib,re,signal,stat,subprocess,sys,time,uuid
os.umask(0o077)
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');held={}
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def need(v,m):
 if not v:raise RuntimeError(m)
def material(p):
 p=pathlib.Path(p);need(p.is_absolute()and p.resolve(strict=True)==p,'physical input path');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);need(stat.S_ISREG(s.st_mode)and s.st_nlink==1 and 0<s.st_size<=268435456,'bounded regular input');r={k:getattr(s,k)for k in F};h=hashlib.sha256();n=0
  while b:=os.read(fd,1048576):n+=len(b);need(n<=s.st_size,'input grew');h.update(b)
  need(n==s.st_size and r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'FD/path input changed');return dict(r,sha256=h.hexdigest())
 finally:os.close(fd)
def hold(p,expected):
 p=pathlib.Path(p);need(material(p)==expected,'original authority mismatch: '+str(p));key=str(p)
 if key in held:need(held[key]==expected,'old authority cannot be refreshed')
 else:held[key]=dict(expected)
 b=p.read_bytes();need(pin(b)=={'sha256':expected['sha256'],'bytes':expected['st_size']}and material(p)==expected,'input changed during copy: '+key);return b
def stable_new(p,expected):
 began=time.monotonic();last=None;count=0;since=began;observations=[]
 while True:
  r=material(p);now=time.monotonic();need(now-began<30,'new control stability deadline');need(pinrow(r)==expected,'new control known bytes changed');observations.append({'elapsedMs':round((now-began)*1000),'row':r})
  if r==last:count+=1
  else:last=r;count=1;since=now
  if count>=3 and now-since>=2:return r,observations
  time.sleep(1.05)
def pinrow(r):return {'sha256':r['sha256'],'bytes':r['st_size']}
def save(leaf,v):
 with(O/leaf).open('x')as f:json.dump(v,f,indent=2);f.write('\n');f.flush();os.fsync(f.fileno())
def run_git(args):return subprocess.check_output(['git',*args],cwd=R,timeout=30,env=env)
def main():
 global O,R,D,IMAGE,env
 need(len(sys.argv)==4,'Usage: host.py CONTROL_JSON CONTROL_SHA256 CONTROL_BYTES');cp=pathlib.Path(sys.argv[1]);need(re.fullmatch('[0-9a-f]{64}',sys.argv[2])and re.fullmatch('[1-9][0-9]*',sys.argv[3]),'external control pin');controlPin={'sha256':sys.argv[2],'bytes':int(sys.argv[3])};need(controlPin['bytes']<=1048576,'control bound');cr,controlObservations=stable_new(cp,controlPin);c=json.loads(hold(cp,cr));need(type(c)is dict and set(c)=={'format','ready','repoRoot','sourceRevision','outputRoot','node','anvil','moduleRoot','worker','image','dockerPrefix','originalFiles'},'control fields');need(c['format']=='nano-doppler-current-fork-control-v1'and c['ready']is True,'control not ready');REV=c['sourceRevision'];need(type(REV)is str and re.fullmatch('[0-9a-f]{40}',REV)and REV.startswith('4158176c'),'source revision');R=pathlib.Path(c['repoRoot']);need(R.is_absolute()and R.resolve(strict=True)==R,'repository path');O=pathlib.Path(c['outputRoot']);need(O.is_absolute()and O.parent.resolve(strict=True)==O.parent and re.fullmatch('doppler-current-private-fork-root-20261017-[0-9]{2}',O.name)and not O.exists(),'new output namespace required')
 env={'HOME':str(pathlib.Path.home()),'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','LANG':'C','LC_ALL':'C'};need(run_git(['rev-parse','HEAD']).decode().strip()==REV and run_git(['status','--porcelain'])==b'','selected clean source revision');IMAGE=c['image'];need(IMAGE=='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','retained image only');D=c['dockerPrefix'];need(type(D)is list and len(D)==5 and all(type(x)is str for x in D)and D[1]=='--config'and D[3]=='--host'and D[4].startswith('unix:///'),'fixed Docker prefix');need(pathlib.Path(D[0]).is_absolute()and pathlib.Path(D[0]).resolve(strict=True)==pathlib.Path(D[0]),'physical Docker binary');dc=pathlib.Path(D[2]);need(dc.is_absolute()and dc.resolve(strict=True)==dc,'physical Docker config')
 originals=c['originalFiles'];need(type(originals)is dict and 6<=len(originals)<=64,'bounded original authority subset');
 for p,r in originals.items():
  need(type(r)is dict and set(r)==set(F)|{'sha256'}and all(type(r[k])is int and r[k]>=0 for k in F)and type(r['sha256'])is str and re.fullmatch('[0-9a-f]{64}',r['sha256']),'full original material row');hold(p,r)
 def original(p):
  need(type(p)is str and p in originals,'missing original authority: '+str(p));return hold(p,originals[p])
 node=pathlib.Path(c['node']);anvil=pathlib.Path(c['anvil']);wp=pathlib.Path(c['worker']);moduleRoot=pathlib.Path(c['moduleRoot']);need(moduleRoot.is_absolute()and moduleRoot.resolve(strict=True)==moduleRoot,'physical dependency root');original(str(node));original(str(anvil));original(str(wp));original(D[0]);lock=json.loads(original(str(R/'tools/doppler-launch/package-lock.json')));versions={}
 for pkg in ['viem','@whetstone-research/doppler-sdk','doppler-router']:
  v=json.loads(original(str(moduleRoot/pkg/'package.json')))['version'];need(v==lock['packages']['node_modules/'+pkg]['version'],'dependency version/lock');versions[pkg]=v
 need(versions=={'viem':'2.33.3','@whetstone-research/doppler-sdk':'1.0.43','doppler-router':'1.0.15'},'retained dependency versions')
 queue=['tools/doppler-launch/fork-lifecycle.mjs','tools/doppler-launch/example.synthetic.json','tools/doppler-launch/package.json'];sources={}
 while queue:
  rel=queue.pop()
  if rel in sources:continue
  b=original(str(R/rel));need(run_git(['show',REV+':'+rel])==b,'source blob changed');sources[rel]=b
  if rel.endswith('.mjs'):
   for _,name in re.findall(r'(?:from\s*|import\s*\()([\'\"])([^\'\"]+)\1',b.decode()):
    if name.startswith('.'):
     p=((R/rel).parent/name).resolve();need(p.is_relative_to(R),'relative import escapes source');queue.append(str(p.relative_to(R)))
 need(set(sources)=={'tools/doppler-launch/fork-lifecycle.mjs','tools/doppler-launch/fork-invariants.mjs','tools/doppler-launch/preview.mjs','tools/doppler-launch/verify-migration.mjs','tools/doppler-launch/example.synthetic.json','tools/doppler-launch/package.json','tools/bridge-source/rpc.mjs'},'exact current runtime source closure');need(pin(sources['tools/doppler-launch/fork-lifecycle.mjs'])=={'sha256':'3c72b22f4423688e9076ca68c11a72688c0da1e120fd88230312f9c1a04f4203','bytes':11391},'current driver pin');program=original(str(wp));need(pin(program)=={'sha256':'53603a0f4b56d4f1c33349afdc81226b10727a6bf7d554d0286f18605e3ba995','bytes':15120},'reviewed worker pin');original(str(pathlib.Path(__file__).resolve()));payload=(json.dumps({'format':'nano-doppler-current-real-anvil-input-v1','sourceRevision':REV,'files':[{'path':p,**pin(b),'contentBase64':base64.b64encode(b).decode()}for p,b in sorted(sources.items())]},separators=(',',':'))+'\n').encode();need(len(payload)<2097152,'source packet bound');O.mkdir(mode=0o700);proof=O/'proof';proof.mkdir(mode=0o700);(O/'input-public.json').write_bytes(payload);save('control-first-admission.private.json',{'pin':controlPin,'observations':controlObservations,'noOldAuthoritiesRefreshed':True});save('host-inputs-before.private.json',held)
 return execute(REV,node,anvil,moduleRoot,program,payload,proof,versions)
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

def execute(REV,node,anvil,moduleRoot,program,payload,proof,versions):
 name='nano-doppler-current-cycle-'+uuid.uuid4().hex;cidPath=O/'owned.cid';q=subprocess.run(D+['container','inspect',name],capture_output=True,timeout=10,env=env);need(q.returncode==1 and (b'No such object'in q.stderr or b'No such container'in q.stderr),'owned name must be absent')
 args=D+['run','--interactive','--cidfile',str(cidPath),'--rm','--pull=never','--name',name,'--label','nano.doppler.unit='+name,'--network','bridge','--read-only','--user','501:20','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','1024m','--cpus','1','--pids-limit','64','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount','type=bind,src='+str(node)+',dst=/runtime/node,readonly','--mount','type=bind,src='+str(anvil)+',dst=/runtime/anvil,readonly','--mount','type=bind,src='+str(moduleRoot)+',dst=/deps/node_modules,readonly','--mount','type=bind,src='+str(proof)+',dst=/proof','--entrypoint','/runtime/node',IMAGE,'--input-type=module','-e',program.decode()]
 proc=None;firstError=None;cleanupError=None;cleanup=None;out=b'';err=b'';began=time.monotonic();save('launch-public.json',{'sourceRevision':REV,'name':name,'image':IMAGE,'network':'bridge for read-only Base fork upstream; local writes remain loopback inside container','portsPublished':0,'personalKeysLoaded':False,'newConsensusIdentities':0,'sourceFiles':len(json.loads(payload)['files']),'inputPin':pin(payload),'wrapperPin':pin(program),'maximumOuterSeconds':360,'resources':{'cpu':1,'memoryMiB':1024,'pids':64,'tmpfsMiB':256},'newImagesBuilt':0})
 try:
  proc=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env);need(proc.pid>1 and proc.pid!=os.getpid()and proc.pid!=28722,'owned command PID;28722 forbidden');out,err=proc.communicate(input=payload,timeout=360)
 except BaseException as exc:firstError=exc
 finally:
  try:cleanup=cleanup_owned(name,cidPath)
  except BaseException as exc:cleanupError=type(exc).__name__+': '+str(exc)
  if cleanup is not None and not cleanup['passed']:cleanupError='CID refusal: '+str(cleanup.get('cidFileIssue'))
  if proc is not None and proc.pid!=28722 and proc.poll()is None:
   try:out,err=proc.communicate(timeout=5)
   except subprocess.TimeoutExpired:
    if proc.pid!=28722:proc.terminate()
    try:out,err=proc.communicate(timeout=5)
    except subprocess.TimeoutExpired:
     if proc.pid!=28722:proc.kill()
     if proc.pid!=28722:out,err=proc.communicate(timeout=5)
  (O/'stdout.public.json').write_bytes(out);(O/'stderr.private.log').write_bytes(err);save('command-public.json',{'exitCode':None if proc is None else proc.returncode,'elapsedMs':round((time.monotonic()-began)*1000),'firstError':None if firstError is None else type(firstError).__name__+': '+str(firstError),'cleanup':cleanup,'cleanupError':cleanupError})
  after={};changes={}
  for p,r in held.items():
   try:
    after[p]=material(p)
    if after[p]!=r:changes[p]={'differentFields':[k for k in r if r[k]!=after[p].get(k)]}
   except BaseException as exc:changes[p]={'readError':type(exc).__name__+': '+str(exc)}
  save('host-inputs-after.private.json',after);save('host-input-fence-public.json',{'passed':not changes,'inputCount':len(held),'changes':changes,'oldAuthoritiesRefreshed':False})
 if firstError:raise firstError
 need(cleanupError is None and cleanup is not None and cleanup['passed']and not changes and proc is not None and proc.returncode==0,'command/cleanup/final fence refused')
 r=json.loads((proof/'result-public.json').read_bytes());need(r['passed']is True and r['sourceRevision']==REV and r['finalSourceFence']['passed']is True and r['cycle']['independentPinnedStateCalls']==6 and r['cycle']['receivedMinimumBothPassed']is True and len(r['cycle']['liveSwapOracles'])==2 and all(x['closed']for x in r['children']),'current child qualification refused');need(run_git(['rev-parse','HEAD']).decode().strip()==REV and run_git(['status','--porcelain'])==b'','source revision changed')
 save('qualification-public.json',{'format':'nano-current-real-anvil-cycle-root-qualification-v1','passed':True,'sourceRevision':REV,'childResultPin':pin((proof/'result-public.json').read_bytes()),'cycleResultPin':pin((proof/'cycle/result.json').read_bytes()),'independentProofPin':pin((proof/'independent-real-proof.public.json').read_bytes()),'sourceAndOldToolAuthoritiesUnchanged':True,'dependencyVersionsMatchLock':versions,'dependencyTreeReproducibilityQualified':False,'publicPortPublished':False,'publicTransactionsSubmitted':0,'newConsensusIdentities':0,'twoRawReceivedMinimumOraclesPassed':True,'originalPrivateCampaignRequalified':False,'nativeNanoQualified':False,'productionQualified':False});print(json.dumps({'passed':True,'realAnvilCycleComplete':True,'twoRawReceivedMinimumOraclesPassed':True,'independentPinnedStateCalls':6,'threeActualAdversarialRejections':True,'originalPrivateCampaignRequalified':False,'productionQualified':False}))
if __name__=='__main__':main()
