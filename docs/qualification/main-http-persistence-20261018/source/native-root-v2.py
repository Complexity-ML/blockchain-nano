"""Root: bounded real-main HTTP/persistence qualification on retained Native C6."""
from pathlib import Path
import hashlib,json,os,signal,stat,subprocess,sys,time
E=Path(__file__).parent;R=Path('/Users/boris/Dev/blockchain-nano')
G='/Library/Developer/CommandLineTools/usr/bin/git';HEAD='bc99b0ec2cf8bc6bcb21a26ec980ef2c4e8573a8'
D=E/'main-http-persistence-fixture-proposal-20261018'
O=E/'main-http-native-qualification-root-20261018-02'
DRIVER=E/'verify-main-http-persistence-root-20261018.mjs'
NODE=Path('/opt/homebrew/Cellar/node/25.9.0_2/bin/node')
MODULE=R/'tests/qualification/rpc-proof.mjs'
FIELDS=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
held={}
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def read(p,want=None):
 need(p.is_absolute() and p.resolve(strict=True)==p,'physical selected input')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);need(stat.S_ISREG(s.st_mode) and s.st_nlink==1 and 0<s.st_size<=268435456,'bounded regular input')
  row={k:getattr(s,k) for k in FIELDS};parts=[];total=0
  while b:=os.read(fd,1048576):total+=len(b);need(total<=s.st_size,'read size bound');parts.append(b)
  b=b''.join(parts);need(len(b)==s.st_size and row=={k:getattr(os.fstat(fd),k) for k in FIELDS}=={k:getattr(p.stat(),k) for k in FIELDS},'stable FD/path read')
  row['sha256']=hashlib.sha256(b).hexdigest();need(want is None or row==want,'original/held authority mismatch')
  return b,row
 finally:os.close(fd)
def known(p,sha,size):
 b,row=read(p);need(pin(b)=={'sha256':sha,'bytes':size},'known input bytes');held[str(p)]=row;return b
def git(*a):return subprocess.check_output([G,'-C',str(R),*a])
def clean():need(git('rev-parse','HEAD').decode().strip()==HEAD and not git('status','--porcelain'),'unchanged clean source main')
need(len(sys.argv)==3 and len(sys.argv[2])==64,'worker path and final external SHA256')
WORKER=Path(sys.argv[1]);wb,wr=read(WORKER);need(wr['sha256']==sys.argv[2] and 0<len(wb)<=131072 and stat.S_IMODE(WORKER.stat().st_mode)==0o400,'final frozen worker')
held[str(WORKER)]=wr
for own in (Path(__file__).resolve(strict=True),Path(sys.executable).resolve(strict=True)):
 _,ownrow=read(own);held[str(own)]=ownrow
clean();need(not O.exists(),'fresh Native run namespace')
preflight=json.loads(known(E/'main-http-native-retained-preflight-root-20261018.json','30b8bc41fa0519355e00a1f9064eea8c7e23cb15d6ad24bb2fefb48c54a0f46f',4864))
need(preflight['passed'] and preflight['selectedOriginalRows']==7 and preflight['sourceRevision']=='c6cdf780c68eab8b06306d76257bda9028928ea3','retained C6 provenance')
for path,row in preflight['rows'].items():read(Path(path),row);held[path]=row
for alias,target in preflight['directBindings']['resolvedDirectNonSystemLoads'].items():need(str(Path(alias).resolve(strict=True))==target,'original direct dylib binding')
exe=E/'native-rpc-state-proof-root-20261017-04/nano';need(str(exe) in held,'original retained Nano executable')
old=json.loads(known(E/'rpc-proof-verifier-js-root-20261017-01/new-input-admission.json','1f20ae6cde3a0de220376b260395f86e1eef025306d5154b2889fbbe705440a6',8680))['samples'][-1]
for path in (NODE,R/'tests/qualification/account-proof.mjs',R/'tests/hardhat/package.json',R/'tests/hardhat/node_modules/ethers/package.json'):
 need(str(path) in old,'original Node/dependency row exists');read(path,old[str(path)]);held[str(path)]=old[str(path)]
known(MODULE,'b59224b4a64231d493ccf21b98d4c5ddb5f5ce3ec9889b792190d0e0e0b090ce',5340)
known(DRIVER,'d9900fc7e431e5e89ff90ed0a88f22cbd116b2eec52248595de63aaafb3e0c0f',4434)
mf=json.loads(known(D/'manifest.json','07f9258acabb8084f06a5cb986057e80667b65bfe79b1f06140855dfcbd0ce3e',4031))
need(mf['payloadCount']==11 and len(mf['payloads'])==11,'reviewed fixture package')
for rel,w in mf['payloads'].items():
 need(not Path(rel).is_absolute() and '..' not in Path(rel).parts,'relative fixture payload')
 known(D/rel,w['sha256'],w['bytes'])
e=json.loads((D/'expectations.json').read_bytes());need(e['ready'] is False and e['nano'] is None,'unmodified inactive fixture model')
e['ready']=True;e['nano']={'sha256':held[str(exe)]['sha256'],'bytes':held[str(exe)]['st_size']}
O.mkdir(mode=0o700)
def save(name,value):
 with (O/name).open('x') as f:json.dump(value,f,indent=2,allow_nan=False);f.write('\n');f.flush();os.fsync(f.fileno())
control=O/'expectations.json';control.write_text(json.dumps(e,indent=2)+'\n');control.chmod(0o400)
control_bytes=control.read_bytes();previous=None;since=None;streak=0;start=time.monotonic();observation=0
while True:
 data,row=read(control);observation+=1;now=time.monotonic()
 save('control-observation-'+str(observation)+'.json',{'elapsedMs':round((now-start)*1000),'row':row,'knownPin':pin(control_bytes)})
 need(data==control_bytes and now-start<30,'known fresh control / finite admission')
 if row==previous:streak+=1
 else:streak=1;since=now
 if streak>=3 and now-since>=2:break
 previous=row;time.sleep(1.05)
held[str(control)]=row
save('selected-input-admission.json',{'files':held,'oldAuthoritiesRefreshed':False,'scope':'Nano + six direct non-system dylibs; original Node subset, new fixed sources/fixtures/control','recursiveNativeLoaderOrNodeClosureQualified':False})
env={'HOME':str(Path.home()),'PATH':'/usr/bin:/bin','LANG':'C','LC_ALL':'C','TZ':'UTC','PYTHONDONTWRITEBYTECODE':'1'}
first=None;worker=None;independent=None;processes=[];changes=[];secondary=[]
def run(label,args,timeout):
 p=None;error=None;cleanup_errors=[];timed=False;residual=False;group_closed=False
 started=time.monotonic()
 def group_alive():
  try:os.killpg(p.pid,0);return True
  except ProcessLookupError:return False
 def signal_group(signum):
  try:os.killpg(p.pid,signum)
  except ProcessLookupError:pass
 with (O/(label+'.stdout.log')).open('xb') as out,(O/(label+'.stderr.log')).open('xb') as err:
  try:
   p=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=out,stderr=err,env=env,start_new_session=True)
   need(p.pid>1 and p.pid!=28722,'own process; prohibited PID28722')
   save(label+'.process.json',{'pid':p.pid,'argv':args,'newOwnedSession':True})
   try:p.wait(timeout=timeout)
   except subprocess.TimeoutExpired:timed=True;raise
  except BaseException as exc:error=exc
  finally:
   if p is not None and p.pid>1 and p.pid!=28722:
    # No diagnostic write may bypass this own-session stop/join closure.
    try:
     if p.poll() is None:signal_group(signal.SIGTERM)
     try:p.wait(timeout=5)
     except subprocess.TimeoutExpired:
      signal_group(signal.SIGKILL);p.wait(timeout=5)
    except BaseException as exc:cleanup_errors.append(type(exc).__name__+': '+str(exc))
    try:
     residual=group_alive()
     if residual:
      signal_group(signal.SIGTERM);cutoff=time.monotonic()+3
      while group_alive() and time.monotonic()<cutoff:time.sleep(0.05)
      if group_alive():
       signal_group(signal.SIGKILL);cutoff=time.monotonic()+3
       while group_alive() and time.monotonic()<cutoff:time.sleep(0.05)
     group_closed=not group_alive()
    except BaseException as exc:cleanup_errors.append(type(exc).__name__+': '+str(exc))
   event={'label':label,'pid':None if p is None else p.pid,'exitCode':None if p is None else p.returncode,'actualProcessJoined':p is not None and p.returncode is not None,'outerTimeout':timed,'residualOwnedGroupAfterJoin':residual,'ownedGroupClosed':group_closed,'cleanupErrors':cleanup_errors,'elapsedMs':round((time.monotonic()-started)*1000)}
   processes.append(event)
   try:save(label+'.exit-public.json',event)
   except BaseException as exc:
    cleanup_errors.append('exit-publication: '+type(exc).__name__+': '+str(exc))
    if error is None:error=exc
 if error is not None:raise error
 need(not timed and p is not None and p.returncode==0 and not residual and group_closed and not cleanup_errors,label+' actual process failed or left owned children')
 return (O/(label+'.stdout.log')).read_bytes()

try:
 for path,row in held.items():read(Path(path),row)
 clean()
 run('worker',[sys.executable,str(WORKER),'--nano',str(exe),'--genesis',str(D/'genesis.json'),'--transactions',str(D/'signed-raw-transactions.json'),'--expectations',str(control),'--output',str(O/'worker')],330)
 worker=json.loads((O/'worker/result-public.json').read_bytes())
 need(worker['passed'] is True and worker['httpCalls']==57 and worker['processStarts']==5 and worker['serverStarts']==3 and worker['mineCommands']==2 and worker['cleanStops']==3 and worker['proofVectors']==5 and worker['allOwnedProcessesJoined'] is True and worker['databaseReopened'] is True and worker['inputFencePassed'] is True,'exact whole HTTP persistence batch')
 vectors=O/'worker/proof-vectors.jsonl';vb=vectors.read_bytes();raw=D/'signed-raw-transactions.json';rb=raw.read_bytes()
 output=run('independent-proof',[str(NODE),str(DRIVER),str(vectors),pin(vb)['sha256'],str(raw),pin(rb)['sha256'],str(MODULE)],30)
 independent=json.loads(output);need(independent['passed'] is True and independent['verifiedVectors']==5 and independent['executionHeaderHashesVerified']==3 and independent['newSignaturesCreated']==0,'five actual HTTP proofs plus three independent header hashes')
 clean()
except BaseException as exc:first={'type':type(exc).__name__,'message':str(exc)}
finally:
 for path,row in held.items():
  try:read(Path(path),row)
  except BaseException as exc:changes.append({'path':path,'error':type(exc).__name__+': '+str(exc)})
 for alias,target in preflight['directBindings']['resolvedDirectNonSystemLoads'].items():
  try:need(str(Path(alias).resolve(strict=True))==target,'direct target changed')
  except BaseException as exc:changes.append({'alias':alias,'error':type(exc).__name__+': '+str(exc)})
 try:save('final-input-fence-public.json',{'passed':not changes,'files':len(held),'changes':changes,'oldAuthoritiesRefreshed':False})
 except BaseException as exc:
  failure={'stage':'final-fence-publication','type':type(exc).__name__,'message':str(exc)}
  if first is None:first=failure
  else:secondary.append(failure)
 result={'format':'nano-main-http-native-root-v1','passed':first is None and not secondary and not changes and worker is not None and independent is not None,'currentMain':HEAD,'compiledSource':'c6cdf780c68eab8b06306d76257bda9028928ea3','profile':'Shanghai','consensus':'pow','worker':None if worker is None else {k:worker[k] for k in ('passed','httpCalls','processStarts','cleanStops','proofVectors','databaseReopened','allOwnedProcessesJoined')},'independentProof':independent,'processes':processes,'firstFailure':first,'secondaryFailures':secondary,'inputFencePassed':not changes,'recursiveNativeLoaderOrNodeClosureQualified':False,'newCppCompilations':0,'newImagesBuilt':0,'runtimeDutiesResumed':False,'consensusRootAuthenticated':False,'attempt04Authorized':False,'productionQualified':False}
 try:save('result-public.json',result)
 except BaseException as exc:
  failure={'stage':'result-publication','type':type(exc).__name__,'message':str(exc)}
  if first is None:first=failure
  else:secondary.append(failure)
  result['passed']=False;result['firstFailure']=first;result['secondaryFailures']=secondary
 print(json.dumps(result))
sys.exit(0 if result['passed'] else 1)
