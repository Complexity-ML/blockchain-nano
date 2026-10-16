#!/usr/bin/env python3
"""One isolated Docker container; fresh Linux copies, causal and candidate unit runs."""
import base64,hashlib,json,os,pathlib,re,shutil,signal,stat,subprocess,sys,time,uuid
os.umask(0o077)
E=pathlib.Path(__file__).resolve().parent;R=pathlib.Path('/Users/boris/Dev/blockchain-nano');P=E/'doppler-v2-lp-attribution-source-proposal-20261016';O=E/'doppler-v2-lp-attribution-linux-source-root-qualification-20261016';N=E/'stopped-backup-node-linux-arm64-20261016';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');REV='e05ea5b9ff5bc8282a9fc120c079bdbddc29f7e6';IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';held={}
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

assert len(sys.argv)==3 and all(re.fullmatch('[0-9a-f]{64}',x)for x in sys.argv[1:]);clean_git();mb=hold(P/'manifest.json');assert pin(mb)['sha256']==sys.argv[1];mf=json.loads(mb);assert 'FROZEN'in mf['status']
for a in mf['payloads']:assert pin(hold(P/a['path']))=={k:a[k]for k in ['sha256','bytes']}
for a in mf['baselines']:assert pin(hold(R/a['path']))=={k:a[k]for k in ['sha256','bytes']}
W=E/'doppler-v2-lp-linux-unit-wrapper-source-20261016';wb=hold(W/'manifest.json');assert pin(wb)['sha256']==sys.argv[2];wm=json.loads(wb);assert 'FROZEN'in wm['status']
for a in wm['payloads']:assert pin(hold(W/a['path']))=={k:a[k]for k in ['sha256','bytes']}
program=hold(W/'wrapper.mjs');assert len(program)<=32768 and not program.startswith(b'#!')
nodeRows=json.loads((N/'prospective-admission.private.json').read_bytes())['observations'][-1]['table']
for leaf,want in nodeRows.items():assert material(N/leaf)==want;held[str(N/leaf)]=want
moduleRoot=R/'tools/doppler-launch/node_modules';assert moduleRoot.resolve(strict=True)==moduleRoot
lock=json.loads(hold(R/'tools/doppler-launch/package-lock.json'));versions={}
for name in ['viem','@whetstone-research/doppler-sdk']:
 v=json.loads(hold(moduleRoot/name/'package.json'))['version'];assert v==lock['packages']['node_modules/'+name]['version'];versions[name]=v
assert versions=={'viem':'2.33.3','@whetstone-research/doppler-sdk':'1.0.43'}
fixture='docs/qualification/doppler-adversarial-lifecycle-20261004/attempt-b.json';relative=[fixture,'tools/doppler-launch/package.json','tools/doppler-launch/verify-migration.mjs','tools/doppler-launch/migration.test.mjs'];phases={}
for mode in ['baseline','candidate']:
 files=[]
 for rel in relative:
  src=R/rel
  if rel.endswith('migration.test.mjs')or(mode=='candidate'and rel.endswith('verify-migration.mjs')):src=P/'source'/rel
  b=hold(src);files.append({'path':rel,**pin(b),'contentBase64':base64.b64encode(b).decode()})
 phases[mode]={'files':files}
payload=(json.dumps({'format':'nano-doppler-lp-linux-unit-input-v1','phases':phases},sort_keys=True,separators=(',',':'))+'\n').encode();assert len(payload)<=1048576
O.mkdir(mode=0o700)
with(O/'input-public.json').open('xb')as f:f.write(payload);f.flush();os.fsync(f.fileno())
os.chmod(O/'input-public.json',0o400);save('selected-host-inputs-before.private.json',held)
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

name='nano-doppler-lp-linux-'+uuid.uuid4().hex;cidPath=O/'unit.cid';pre=subprocess.run(D+['container','inspect',name],capture_output=True,timeout=10,env=env);assert pre.returncode==1 and(b'No such object'in pre.stderr or b'No such container'in pre.stderr)
args=D+['run','--interactive','--cidfile',str(cidPath),'--rm','--pull=never','--name',name,'--label','nano.doppler.unit='+name,'--network','none','--read-only','--user','501:20','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','768m','--cpus','1','--pids-limit','64','--tmpfs','/tmp:rw,nosuid,nodev,size=64m','--env','HOME=/tmp','--env','NODE_ENV=test','--mount','type=bind,src='+str(N/'node')+',dst=/runtime/node,readonly','--mount','type=bind,src='+str(moduleRoot)+',dst=/deps/node_modules,readonly','--entrypoint','/runtime/node',IMAGE,'--input-type=module','-e',program.decode()]
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
(O/'stdout.public.json').write_bytes(out);(O/'stderr.private.log').write_bytes(err);command={'exitCode':None if proc is None else proc.returncode,'elapsedMs':round((time.monotonic()-began)*1000),'firstError':None if firstError is None else type(firstError).__name__+': '+str(firstError),'cleanup':cleanup,'cleanupError':cleanupError,'image':IMAGE,'network':'none','cpuLimit':1,'memoryLimitMiB':768,'stdinPin':pin(payload),'wrapperProgramPin':pin(program)};save('command-public.json',command)
if firstError is not None:raise firstError
assert cleanupError is None and cleanup['passed'];assert proc.returncode==0 and len(out)<=786432
r=json.loads(out);assert r['format']=='nano-doppler-lp-linux-unit-result-v1'and r['passed']and len(r['phases'])==2 and r['finalFence']['passed'];assert r['sourceAdmission']['observations']>=3 and 2000<=r['sourceAdmission']['stableSpanMs']<=r['sourceAdmission']['elapsedMs']<30000
for phase in r['phases']:
 assert phase['oracleMatched']and phase['postFence']['passed'];(O/(phase['phase']+'-stdout.tap')).write_text(phase['stdout']);(O/(phase['phase']+'-stderr.log')).write_text(phase['stderr'])
assert r['phases'][0]['phase']=='baseline'and r['phases'][0]['expectedCausalFailureObserved']and r['phases'][0]['summary']['fail']==1
assert r['phases'][1]['phase']=='candidate'and r['phases'][1]['candidateTestsPassed']and r['phases'][1]['summary']['pass']==mf['expectedTargetedQualification']['testsIncludingParents']
after={p:material(p)for p in held};save('selected-host-inputs-after.private.json',after);differences={p:[k for k in v if v[k]!=after[p].get(k)]for p,v in held.items()if v!=after[p]}
if differences:save('host-source-fence-refusal.json',{'changedFieldsByKnownPath':differences,'qualificationPassed':False})
assert not differences,differences;clean_git()
initial=E/'doppler-v2-lp-attribution-root-qualification-20261016/first-refusal-public.json';second=E/'doppler-v2-lp-attribution-candidate-root-qualification-20261016/first-refusal-public.json'
report={'format':'nano-doppler-v2-lp-linux-fixture-qualification-v1','passed':True,'sourceBaseRevision':REV,'sourceProposalManifest':pin(mb),'linuxWrapperManifest':pin(wb),'linuxWrapperProgram':pin(program),'command':command,'linuxResult':pin(out),'candidateTestsPassed':29,'oldMissingRejectionReproduced':True,'newNeverAdmittedLinuxCopies':True,'linuxSourceNineAndShaUnchangedAfterBothPhases':True,'oldHostSourceAndNodeAuthoritiesUnchanged':True,'hostKnownInputs':len(held),'installedVersionsMatchLock':versions,'dependencyReproducibilityQualified':False,'oldRefusalsPreserved':[pin(initial.read_bytes()),pin(second.read_bytes()),pin((E/'doppler-v2-lp-attribution-linux-root-qualification-20261016/first-refusal-public.json').read_bytes())],'additionalHostReceiptRefusalPreserved':pin((E/'doppler-v2-lp-attribution-linux-stdin-root-qualification-20261016/first-refusal-public.json').read_bytes()),'inputReceiptIsNotExecutionSource':True,'oldRefusalsRequalified':False,'dockerContainers':1,'dockerImageBuilds':0,'cppCompilations':0,'network':'none','rpcTransport':'injected fixtures only','anvilStarted':False,'transactionsSubmitted':0,'liveBaseMigrationQualified':False,'nativeNanoQualified':False,'productionQualified':False};save('result-public.json',report);print(json.dumps({'passed':True,'candidateTestsPassed':29,'oldMissingRejectionReproduced':True,'hostSourceAndNodeNinePreserved':True,'newLinuxCopiesNinePreserved':True,'oldRefusalsRequalified':False,'sourceWorkingTreeClean':True,'productionQualified':False}))
