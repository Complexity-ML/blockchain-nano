"""SOURCE_ONLY: one fresh rollback TU on a retained C7 Core. Root executes."""
import hashlib,json,os,pathlib,posixpath,re,selectors,shlex,signal,stat,subprocess,sys,time
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def same(a,b):return json.dumps(a,sort_keys=True,allow_nan=False)==json.dumps(b,sort_keys=True,allow_nan=False)
def body(raw):
 def unique(items):
  d={}
  for k,v in items:need(k not in d,'duplicate JSON key');d[k]=v
  return d
 return json.loads(raw,object_pairs_hook=unique,parse_constant=lambda _:(_ for _ in()).throw(ValueError('nonfinite JSON')))
def material(path):
 p=pathlib.Path(path);need(p.is_absolute()and p.resolve(strict=True)==p,'physical file');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and a.st_nlink>=1 and 0<=a.st_size<=134217728,'bounded regular file');chunks=[];n=0
  while b:=os.read(fd,65536):n+=len(b);need(n<=a.st_size,'input grew');chunks.append(b)
  raw=b''.join(chunks);need(n==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'full9/path changed');return dict(row,sha256=pin(raw)['sha256']),raw
 finally:os.close(fd)
def main(config):
 c=body(material(pathlib.Path(config))[1]);need(set(c)=={'compilerFlags','linkPrefix','linkTail','projectPins','knownPins','workerPin'},'exact worker config');need(pin(material(pathlib.Path(__file__).resolve())[1])==c['workerPin'],'known worker source');need(os.geteuid()==os.getegid()==1000,'guest owner');out=pathlib.Path('/out');deadline=time.monotonic()+480;held={};aliases={};products={};first=None;secondary=[];events=[];record=None;mmd=None;counts={x:0 for x in('independentM','cppCompilations','links','loaderChecks','runs')}
 def save(name,value):
  with(out/name).open('x')as f:json.dump(value,f,sort_keys=True,indent=2,allow_nan=False);f.write('\n');f.flush();os.fsync(f.fileno())
 def error(exc,stage):
  nonlocal first
  item={'stage':stage,'type':type(exc).__name__,'message':str(exc)[:2000]}
  if first is None:first=item
  else:secondary.append(item)
 def observe(paths):
  need(len(set(paths))<=20000,'bounded selected dependency set');table={};links={}
  for lexical in sorted(set(paths)):
   need(time.monotonic()<deadline,'guest observation deadline');need(lexical.startswith('/')and not any(x in lexical for x in('\0','\n','\r')),'absolute dependency');physical=str(pathlib.Path(lexical).resolve(strict=True));r,b=material(pathlib.Path(physical));need(str(pathlib.Path(lexical).resolve(strict=True))==physical,'alias changed while reading');links[lexical]=physical;need(physical not in table or same(table[physical],r),'same canonical input changed across aliases');table[physical]=r
   known=c['projectPins'].get(lexical)or c['knownPins'].get(lexical)
   if lexical.startswith(('/work/','/case/','/old/','/retained/','/python-runtime/')):need(known is not None and physical==lexical,'known exact project/provider path')
   if physical.startswith(('/work/','/case/','/old/','/retained/','/python-runtime/')):need(physical in c['projectPins']or physical in c['knownPins'],'resolved project/provider known')
   need(known is None or pin(b)==known,'known dependency bytes');need(physical not in held or same(held[physical],r),'retained guest FD9/SHA changed');need(lexical not in aliases or aliases[lexical]==physical,'retained alias changed')
  return table,links
 def hold(paths):
  rows,links=observe(paths);held.update(rows);aliases.update(links)
 def command(phase,args,limit=180):
  remaining=min(limit,deadline-time.monotonic());need(remaining>0,'worker deadline');p=None;err=None;joined=False;streams=[];sel=selectors.DefaultSelector();start=time.monotonic();totals={};save(phase+'.argv-public.json',args)
  try:
   for name in('stdout','stderr'):streams.append((out/(phase+'.'+name+'.log')).open('xb'));totals[name]=0
   p=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True,env={'PATH':'/usr/bin:/bin','HOME':'/tmp','TMPDIR':'/tmp','LC_ALL':'C','LANG':'C'});need(p.pid>1 and p.pid!=28722,'own process');sel.register(p.stdout,selectors.EVENT_READ,('stdout',streams[0]));sel.register(p.stderr,selectors.EVENT_READ,('stderr',streams[1]))
   while sel.get_map():
    need(time.monotonic()-start<remaining,'owned child timeout')
    for key,_ in sel.select(.1):
     data=os.read(key.fd,65536)
     if not data:sel.unregister(key.fileobj);continue
     name,dst=key.data;totals[name]+=len(data);need(totals[name]<=16777216,'bounded logs');dst.write(data)
   p.wait(timeout=max(.1,remaining-(time.monotonic()-start)));need(p.returncode==0,'command nonzero: '+phase)
  except BaseException as exc:err=exc
  finally:
   if p is not None:
    try:
     try:os.killpg(p.pid,signal.SIGTERM)
     except ProcessLookupError:pass
     try:p.wait(timeout=3)
     except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=3)
     try:os.killpg(p.pid,0)
     except ProcessLookupError:joined=True
     need(joined,'owned group remains')
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-join','message':str(exc)})
   actions=[sel.close]
   for f in streams:actions.extend([f.flush,lambda f=f:os.fsync(f.fileno()),f.close])
   if p is not None:actions.extend([p.stdout.close,p.stderr.close])
   for action in actions:
    try:action()
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-close','message':str(exc)})
   event={'phase':phase,'exitCode':None if p is None else p.returncode,'groupClosed':joined,'elapsedMs':round((time.monotonic()-start)*1000)};events.append(event)
   try:save(phase+'.exit-public.json',event)
   except BaseException as exc:
    if err is None:err=exc
    else:secondary.append({'stage':phase+'-event','message':str(exc)})
  if err is not None:raise err
  return(out/(phase+'.stdout.log')).read_bytes(),(out/(phase+'.stderr.log')).read_bytes()
 def product(name):
  p=out/name;r,b=material(p);products[str(p)]=r;return b
 def deps(raw):
  need(len(raw)<=4194304,'dependency file bound');target,sep,text=raw.decode().replace('\\\n',' ').partition(':');need(sep and shlex.split(target)==['/out/journal.o'],'one exact MT target');values=sorted(set(posixpath.normpath(posixpath.join('/out',x))for x in shlex.split(text)));need(values and len(values)<=20000 and all(x.startswith('/')for x in values),'normalized dependencies');return values
 def parse_record(raw):
  need(len(raw)<=65536 and len(raw.splitlines())==1,'exact one C++ record');r=body(raw);fixed={'scenario':'beacon-journal-real-physical-rollback','result':'PASS','physicalPairs':2,'databases':4,'independentReadonlyPasses':2,'readonlyDatabaseOpens':8,'CoreAuthentications':7,'positiveAuthentications':4,'oldPairRecentPrimaryFloorRefused':True,'oldPairRecentWitnessFloorRefused':True,'oldPairBothRecentFloorsRefused':True,'expectedCoreRefusals':3,'newTestOnlySignaturesCreated':1,'signingProviderCalls':1,'signerConstructions':2,'newSignaturesInCheck':0,'signersConstructedInCheck':0,'fourClosedStoresFD9ShaUnchanged':True,'completeIndependentPhysicalCopy':True,'externalCommitmentOutsideBothTrees':True,'externalCommitmentFD9ShaUnchanged':True,'maximumStoreBytes':33554432,'maximumEntries':4096,'maximumRssKiB':131072,'maximumWallSeconds':180,'independentExternalRollbackProtectionQualified':False,'historicalDigestAncestryVerified':False,'deliveredSignatureHistoryVerified':False,'automaticRecoveryUsed':False,'operatorRecoveryQualified':False,'privateInputsRead':False,'runtimeDutiesAuthorized':False,'networkTransport':False,'commonFinalityQualified':False,'productionQualified':False};extra={'externalCommitmentSha256','externalCommitmentBytes','storeBytes','storeEntries','actualPeakRssKiB','elapsedMs','productionAdmissions'};need(type(r)is dict and set(r)==set(fixed)|extra and same({k:r[k]for k in fixed},fixed),'typed exact C++ counters/scope')
  need(type(r['externalCommitmentSha256'])is str and re.fullmatch('[0-9a-f]{64}',r['externalCommitmentSha256']),'commitment SHA')
  for k,lo,hi in[('externalCommitmentBytes',1,1048576),('storeBytes',1,33554432),('storeEntries',1,4096),('actualPeakRssKiB',1,131072),('elapsedMs',0,180000)]:need(type(r[k])is int and lo<=r[k]<=hi,'bounded actual C++ '+k)
  admissions=r['productionAdmissions'];need(type(admissions)is list and len(admissions)==2,'exact two production admissions')
  fields={'label','entries','payloadBytes','samples','stableSamples','elapsedMs','ctimeOnlyChanges','durabilityBeforeSamples','creatorEightAndShaUnchanged','exactLastCaptureAdopted','postAdoptionStrictFence'}
  for index,label in enumerate(('initial','post-sign')):
   item=admissions[index];need(type(item)is dict and set(item)==fields and type(item['label'])is str and item['label']==label,'closed ordered production admission')
   for k,lo,hi in[('entries',1,4096),('payloadBytes',0,33554432),('samples',3,30),('stableSamples',3,30),('elapsedMs',2000,29999),('ctimeOnlyChanges',0,122880)]:need(type(item[k])is int and lo<=item[k]<=hi,'bounded production admission '+k)
   need(item['stableSamples']<=item['samples']and all(item[k]is True for k in('durabilityBeforeSamples','creatorEightAndShaUnchanged','exactLastCaptureAdopted','postAdoptionStrictFence')),'production admission strict booleans/streak')
  need(sum(item['entries']for item in admissions)==r['storeEntries']and sum(item['payloadBytes']for item in admissions)==r['storeBytes'],'production admissions cover exact old/current store inventory')
  return r
 try:
  flags=c['compilerFlags'];need(type(flags)is list and flags[0]=='/usr/bin/clang++'and not any(x in flags for x in('-M','-MM','-MD','-MMD','-MF','-MT','-o','-c')),'exact explicit flags');source='/case/tests/qualification/beacon_journal_physical_rollback.cpp';obj='/out/journal.o';need(c['linkTail'].count('/retained/libnano_core.a')==1,'one retained Core');hold([flags[0],c['linkPrefix'][0],'/usr/bin/ldd','/python-runtime/bin/python3',source,'/case/tools/storage-maintenance/bootstrap-readonly-replica.hpp',*c['knownPins']])
  counts['independentM']+=1;command('M',flags+['-M','-MF',obj+'.M','-MT',obj,source]);names=deps(product('journal.o.M'));need(source in names,'declared TU reached');hold(names);save('pre-CXX-observations.json',{'rows':held,'aliases':aliases})
  counts['cppCompilations']+=1;command('CXX',flags+['-MD','-MF',obj+'.d','-MT',obj,'-o',obj,'-c',source],240);product('journal.o');actual=deps(product('journal.o.d'));need(actual==names,'literal M equals MD before accepting new inputs');hold(actual);need(sorted({aliases[p]for p in names})==sorted({aliases[p]for p in actual}),'normalized M equals MD');mmd={'normalizedMEqualsMd':True,'literalMEqualsMd':True,'dependencyCount':len({aliases[p]for p in names})}
  hold([p for p in c['linkPrefix']+c['linkTail']if p.startswith('/')]);counts['links']+=1;command('LINK',c['linkPrefix']+[obj,'-o','/out/rollback']+c['linkTail']);product('rollback');counts['loaderChecks']+=1;raw,_=command('LDD',['/usr/bin/ldd','/out/rollback'],30);need(b'not found'not in raw,'resolved loader');closure=[]
  for line in raw.decode().splitlines():
   value=line.strip()
   if re.fullmatch(r'linux-vdso\.so\.[0-9]+ \(0x[0-9a-f]+\)',value):continue
   match=re.fullmatch(r'(?:[^ \t]+ => )?(/[^ \t]+) \(0x[0-9a-f]+\)',value);need(match is not None,'recognized absolute ldd provider');closure.append(match[1])
  need(closure,'nonempty actual loader closure');hold(closure);save('loader-closure-public.json',{'paths':closure,'canonicalPaths':{p:aliases[p]for p in closure}});counts['runs']+=1;raw,err=command('RUN',['/out/rollback','/out/run-root'],180);need(err==b'','empty C++ stderr');record=parse_record(raw)
 except BaseException as exc:error(exc,'single-TU-qualification')
 finally:
  deadline=time.monotonic()+45
  if aliases:
   try:rows,links=observe(list(aliases));need(same(rows,held)and same(links,aliases),'all selected guest FD9/SHA/aliases unchanged')
   except BaseException as exc:error(exc,'guest-final-fence')
  for path,want in products.items():
   try:need(same(material(pathlib.Path(path))[0],want),'fresh product unchanged')
   except BaseException as exc:error(exc,'product-final-fence')
  try:save('final-observations.json',{'rows':held,'aliases':aliases,'products':products,'historicalImageFD9Claimed':False})
  except BaseException as exc:error(exc,'observations-publication')
  report={'format':'nano-real-journal-rollback-amd-worker-v1','passed':first is None and record is not None,'counts':counts,'mmd':mmd,'record':record,'firstFailure':first,'secondaryFailures':secondary,'events':events,'CoreRebuilt':False,'productionQualified':False}
  try:save('result-public.json',report)
  except BaseException as exc:error(exc,'result-publication');report['passed']=False
 return 0 if report['passed']else 1
if __name__=='__main__':
 try:need(len(sys.argv)==2,'usage amd_worker_v3.py CONFIG');raise SystemExit(main(sys.argv[1]))
 except Exception as exc:print(json.dumps({'passed':False,'prestartFailure':str(exc)}),file=sys.stderr);raise SystemExit(1)
