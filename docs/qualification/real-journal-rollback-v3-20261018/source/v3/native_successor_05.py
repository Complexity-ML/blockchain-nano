#!/usr/bin/env python3
"""Source-only retained Native Core successor; only fresh synthetic runtime trees."""
import hashlib,json,os,pathlib,shlex,signal,stat,subprocess,sys,time,types
E=pathlib.Path(__file__).resolve().parents[1];N=E/'native-c7-cmake-root-20261018-01';O=N/'native-successor-05'
PARENT=N/'audit-and-run-native-v6.py';AUTH=N/'native-runtime-04/input-admission.private.json'
PARENT_PIN=('2e64ffa88f68a3a84f8fdce173b18fe6673d31dc49eba1194fe0a58a37bffbe3',23906);AUTH_PIN=('a38802c155ce07f7b4c9c858407e4c1e4860bbf99008ede4d4f2d3e18c17a6a1',2588992)
CPP=pathlib.Path(__file__).resolve().with_name('beacon_journal_physical_rollback.cpp');CPP_PIN=('2e5ef451f13cca770cf02128bbb389eaae60379974252859829e197d14521acd',28801)
HELPER_PIN=('d3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125',20319)
RUNTIME_RESOURCES=('tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json','tests/fixtures/capella-bootstrap-anchor-cb532/profile.json','tests/fixtures/capella-bootstrap-anchor-cb532/genesis.json','tests/fixtures/capella-bootstrap-anchor-cb532/genesis-state.ssz','tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz')
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def need(ok,why):
 if not ok:raise RuntimeError(why)
def read(path,known):
 path=pathlib.Path(path);need(path.is_absolute()and path.resolve(strict=True)==path,'physical input path');fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_CLOEXEC)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and a.st_size==known[1],'known bounded single-link input');raw=b''
  while part:=os.read(fd,65536):raw+=part;need(len(raw)<=known[1],'bounded input read')
  need(len(raw)==known[1]and hashlib.sha256(raw).hexdigest()==known[0]and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(path.lstat(),k)for k in F},'known input bytes and FD9/path');return raw,dict(row,sha256=known[0])
 finally:os.close(fd)
def main(self_pin):
 os.umask(0o077);own_raw,own_row=read(__file__,self_pin);authority_raw,authority_row=read(AUTH,AUTH_PIN);parent_raw,parent_row=read(PARENT,PARENT_PIN);sys.dont_write_bytecode=True
 m=types.ModuleType('native_retained_source_parent');m.__file__=str(PARENT);exec(compile(parent_raw,str(PARENT),'exec'),m.__dict__);m.O=O
 old=m.decoded(authority_raw);need(old['format']=='nano-native-c7-runtime-admission-v1'and len(old['files'])==3334 and len(old['prospectiveDirectoryRows'])==1252,'original Native04 admission shape/counts');need(old['files'][str(PARENT)]==parent_row,'parent original row, not a replacement authority')
 first=None;secondary=[];cases={'journal':{'passed':False,'status':'not-started'}};handlers={};owned=False;fresh={};retained={};final={};build_events=[];cancelled=False;directory_observations=[];admissions={}
 def failure(exc,stage):
  nonlocal first
  if first is None:first=exc
  else:secondary.append({'stage':stage,'type':type(exc).__name__,'message':str(exc)[:1500]})
 legacy_journal_schema=m.journal_schema
 def journal_schema(j):
  need(type(j)is dict and len(j)==42 and 'productionAdmissions'in j,'closed42-key V3 journal record');base=dict(j);production=base.pop('productionAdmissions');legacy_journal_schema(base)
  need(type(production)is list and len(production)==2,'exactly two ordered production admission records')
  for row,label in zip(production,('initial','post-sign')):
   m.exact(row,{'label':label,'durabilityBeforeSamples':True,'creatorEightAndShaUnchanged':True,'exactLastCaptureAdopted':True,'postAdoptionStrictFence':True},('entries','payloadBytes','samples','stableSamples','elapsedMs','ctimeOnlyChanges'))
   for key,low,high in(('entries',1,4096),('payloadBytes',0,33554432),('samples',3,30),('stableSamples',3,30),('elapsedMs',2000,29999),('ctimeOnlyChanges',0,122880)):m.integer(row[key],low,high,'production admission '+key)
   need(row['stableSamples']<=row['samples'],'admission stable samples bounded by samples')
  need(sum(row['entries']for row in production)==base['storeEntries']and sum(row['payloadBytes']for row in production)==base['storeBytes'],'production admissions equal final old/current inventory totals')
 m.journal_schema=journal_schema
 def old_guard():
  changed=[];source_rows=None;source_drift=[];directory_record=None
  for p,row in old['files'].items():
   try:need(m.pin(p)==row,'original input changed: '+p)
   except BaseException as exc:changed.append(p);failure(exc,'original-file-fence')
  for p,resolved in old['dependencyAliases'].items():
   try:need(str(pathlib.Path(p).resolve(strict=True))==resolved,'original alias changed: '+p)
   except BaseException as exc:changed.append(p);failure(exc,'original-alias-fence')
  try:
   a=N/'source-admission.private.json';need(m.pin(a)==old['files'][str(a)]and m.pin(a)['sha256']==m.ADMISSION_SHA,'original source authority unchanged');source_rows=m.decoded(a.read_text())['sourceRows'];actual={}
   for parent,dirs,files in os.walk(m.S,followlinks=False):
    for name in dirs:need(stat.S_ISDIR(os.lstat(pathlib.Path(parent)/name).st_mode),'source directory type; no symlink traversal')
    for name in files:
     path=pathlib.Path(parent)/name;relative=str(path.relative_to(m.S));actual[relative]=m.pin(path)
     if str(path)in old['files']:need(actual[relative]==old['files'][str(path)],'consumed source original FD9 and SHA unchanged: '+relative)
   need(len(source_rows)==len(actual)==9541 and set(actual)==set(source_rows)and all(actual[p]['st_size']==source_rows[p]['st_size']and actual[p]['sha256']==source_rows[p]['sha256']for p in source_rows),'9541 exact original source names/size/SHA; historical FD9 not qualified')
   for relative in RUNTIME_RESOURCES:need(relative in source_rows and m.pin(m.S/relative)==source_rows[relative],'runtime resource ORIGINAL FD9 and SHA unchanged: '+relative)
   source_drift=[{'path':p,'changedFields':[k for k in F if actual[p][k]!=source_rows[p][k]],'original':{k:source_rows[p][k]for k in F},'observed':{k:actual[p][k]for k in F}}for p in sorted(source_rows)if actual[p]!=source_rows[p]]
  except BaseException as exc:changed.append('original-source');failure(exc,'original-source-fence')
  try:
   need(source_rows is not None,'original source authority available for consumed resource derivation');original=old['prospectiveDirectoryRows'];current=m.source_directory_rows();need(set(current)==set(original)and all(current[p]['kind']==original[p]['kind']=='directory'and stat.S_ISDIR(current[p]['st_mode'])and stat.S_ISDIR(original[p]['st_mode'])for p in original),'all original1252 directory names/types, no symlinks')
   consumed={pathlib.Path(p)for p in old['files']if pathlib.Path(p).is_relative_to(m.S)};compile_source_count=len(consumed)
   for relative in RUNTIME_RESOURCES:
    need(relative in source_rows,'runtime resource bound to original9541 file rows');consumed.add(m.S/relative)
   ancestors=set()
   for path in consumed:
    need(path.is_relative_to(m.S)and str(path.relative_to(m.S))in source_rows,'consumed source path has original file authority');parent=path.parent
    while parent.is_relative_to(m.S):
     ancestors.add(str(parent.relative_to(m.S)))
     if parent==m.S:break
     parent=parent.parent
   need(ancestors<=set(original),'all consumed ancestors have ORIGINAL source-directory rows');identity_fields=('st_dev','st_ino','st_nlink','st_uid','st_gid','st_size');structural_changed=[p for p in sorted(original)if any(current[p][k]!=original[p][k]for k in identity_fields)];observed_consumed_drift=[p for p in sorted(ancestors)if current[p]!=original[p]]
   drift=[{'path':p,'changedFields':[k for k in F if current[p][k]!=original[p][k]],'original':{k:original[p][k]for k in F},'observed':{k:current[p][k]for k in F},'consumedAncestor':p in ancestors}for p in sorted(original)if current[p]!=original[p]]
   directory_record={'qualifiedScope':'original-consumed-file-fd9-and-source-bytes-with-original-directory-identities','originalAuthoritySha256':AUTH_PIN[0],'originalDirectoryInventoryNamesTypesUnchanged':1252,'originalDirectoryIdentityComparisonFields':list(identity_fields),'changedOriginalDirectoryIdentities':structural_changed,'currentDirectoryAllowedPermissions':['0500','0700'],'historicalDirectoryExactModesQualified':False,'historicalDirectoryTimestampsQualified':False,'historicalSourceFileFD9Qualified':False,'originalSourceNamesSizeShaUnchanged':9541,'strictRuntimeOriginalFileRows':len(RUNTIME_RESOURCES),'observedOriginalSourceFD9DriftCount':len(source_drift),'observedOriginalSourceFD9Drifts':source_drift,'wholeTreeDirectoryFD9Qualified':False,'sdkOrBuildParentDirectoryFD9Qualified':False,'consumedOldAdmissionSourceFiles':compile_source_count,'runtimeResourceLiterals':list(RUNTIME_RESOURCES),'consumedAncestorCount':len(ancestors),'consumedAncestors':sorted(ancestors),'observedConsumedAncestorFD9Drifts':observed_consumed_drift,'observedOriginalFD9DriftCount':len(drift),'observedOriginalFD9Drifts':drift,'currentDirectoryRowsAdoptedAsAuthority':False,'currentSourceRowsAdoptedAsAuthority':False,'priorNativeSuccessor01FAILED':True,'priorNativeSuccessor02FAILED':True,'priorNativeSuccessor03FAILED':True,'productionQualified':False}
   if owned:
    name='directory-scope-'+str(len(directory_observations)+1).zfill(2)+'-public.json';m.put(O/name,directory_record);directory_observations.append({'file':name,'consumedAncestorCount':len(ancestors),'originalFD9Drifts':len(drift),'observedConsumedAncestorFD9Drifts':observed_consumed_drift,'changedOriginalDirectoryIdentities':structural_changed,'originalSourceFD9Drifts':len(source_drift)})
   need(not structural_changed,'ORIGINAL source-directory structural identity changed: '+str(structural_changed[:8]))
  except BaseException as exc:changed.append('original-source-directory-identities');failure(exc,'original-structural-directory-fence')
  need(not changed,'original authority software scope changed: '+str(changed[:8]));return {'files':3334,'strictRuntimeOriginalFileRows':len(RUNTIME_RESOURCES),'originalDirectoryInventoryNamesTypesUnchanged':1252,'originalDirectoryStructuralIdentitiesUnchanged':1252,'consumedAncestorCount':directory_record['consumedAncestorCount'],'historicalSourceFileFD9Qualified':False,'historicalDirectoryExactModesQualified':False,'historicalDirectoryTimestampsQualified':False,'wholeTreeDirectoryFD9Qualified':False,'sourceRows':9541,'sourceComparisonFields':['st_size','sha256'],'aliases':len(old['dependencyAliases']),'qualifiedScope':'original-consumed-file-fd9-and-source-bytes-with-original-directory-identities','passed':True}
 def strict_fresh():
  for p,row in fresh.items():need(m.pin(p)==row,'already admitted fresh input changed: '+p)
 def product_creator(path,label):
  fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_CLOEXEC)
  try:
   a=os.fstat(fd);row={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<a.st_size<=134217728,'bounded new producer file');h=hashlib.sha256();total=0
   while part:=os.read(fd,65536):total+=len(part);need(total<=a.st_size,'new producer file grew');h.update(part)
   row['sha256']=h.hexdigest();now=os.fstat(fd);linked=os.lstat(path);need(total==a.st_size and all(getattr(a,k)==getattr(now,k)==getattr(linked,k)for k in F if k!='st_ctime_ns'),'producer eight fields immutable during initial hash');m.put(O/(label+'-creator-public.json'),{'path':str(path),'row':row,'evidence':'first closed-producer FD; not an older authority','productionQualified':False});return row
  finally:os.close(fd)
 def admit(label,creators):
  start=time.monotonic();since=None;previous=None;streak=0;samples=0;ctime_changes=0;current_input=None;admitted=False;decision_error=None
  m.put(O/(label+'-creators-public.json'),{'creatorRows':creators,'policy':'new inodes only; creator eight fields and SHA never change','productionQualified':False})
  try:
   while True:
    strict_fresh();sample={};errors=[];samples+=1
    for p,creator in creators.items():
     current_input=p;sample[p]=m.pin(p);bad=[k for k in(*F,'sha256')if k!='st_ctime_ns'and sample[p][k]!=creator[k]]
     if bad:errors.append({'path':p,'changedFields':bad})
    strict_fresh();now=time.monotonic();equal=previous is not None and sample==previous
    if equal:streak+=1
    else:
     if previous is not None:ctime_changes+=sum(sample[p]['st_ctime_ns']!=previous[p]['st_ctime_ns']for p in sample)
     streak=1;since=now
    m.put(O/(label+'-sample-'+str(samples).zfill(2)+'-public.json'),{'sampleRows':sample,'creatorEightOrShaChanges':errors,'samples':samples,'stableSamples':streak,'ctimeOnlyChanges':ctime_changes,'elapsedMs':round((now-start)*1000),'productionQualified':False});need(not errors,'new candidate creator eight fields/SHA changed');need(time.monotonic()-start<=30,'prospective admission30s includes durable observations')
    if streak>=3 and now-since>=2:
     fresh.update(sample);strict_fresh();admitted=True;break
    previous=sample;time.sleep(1.05)
  except BaseException as exc:decision_error=str(exc)[:1500];raise
  finally:
   decision={'admitted':admitted,'samples':samples,'stableSamples':streak,'ctimeOnlyChanges':ctime_changes,'elapsedMs':round((time.monotonic()-start)*1000),'currentInput':current_input,'reason':decision_error,'creatorRowsRetained':True,'lastSampleAdoptedExactlyOnce':admitted,'productionQualified':False};admissions[label]=decision
   try:m.put(O/(label+'-decision-public.json'),decision)
   except BaseException as exc:
    if decision_error is None:raise
    secondary.append({'stage':label+'-decision-publication','type':type(exc).__name__,'message':str(exc)[:1500]})
 def build(name,argv):
  start=time.monotonic();p=None;error=None;others=[];joined=False;owned_group=False;streams=[];previous={}
  def err(exc,stage):
   nonlocal error
   if error is None:error=exc
   else:others.append({'stage':stage,'message':str(exc)[:1500]})
  def alive():
   need(owned_group and p.pid>1 and p.pid!=28722,'only newly owned nonreserved group')
   try:os.killpg(p.pid,0);return True
   except ProcessLookupError:return False
  def send(sig):
   need(owned_group and p.pid>1 and p.pid!=28722,'reserved PID never signaled')
   try:os.killpg(p.pid,sig)
   except ProcessLookupError:pass
  try:
   for ext in('stdout','stderr'):streams.append((O/(name+'.'+ext+'.log')).open('xb'))
   p=subprocess.Popen(argv,cwd=m.D,stdout=streams[0],stderr=streams[1],start_new_session=True);owned_group=p.pid>1 and p.pid!=28722;need(owned_group,'new compile group excludes28722')
   while True:
    need(time.monotonic()-start<200 and all(os.fstat(f.fileno()).st_size<=1048576 for f in streams),'bounded compiler200s/1MiB')
    try:p.wait(timeout=.1);break
    except subprocess.TimeoutExpired:pass
   need(all(os.fstat(f.fileno()).st_size<=1048576 for f in streams),'final compiler log1MiB bound');need(p.returncode==0,'compile/link exit '+str(p.returncode))
  except BaseException as exc:err(exc,'build')
  finally:
   for sig in(signal.SIGINT,signal.SIGTERM,signal.SIGHUP):
    try:previous[sig]=signal.signal(sig,signal.SIG_IGN)
    except BaseException as exc:err(exc,'closing-signal')
   if p is not None and owned_group:
    try:
     forced=p.poll()is None
     if forced:send(signal.SIGTERM)
     try:p.wait(timeout=5)
     except subprocess.TimeoutExpired:forced=True;send(signal.SIGKILL);p.wait(timeout=5)
     if alive():forced=True;send(signal.SIGKILL)
     end=time.monotonic()+5
     while alive()and time.monotonic()<end:time.sleep(.05)
     joined=not alive();need(joined and(not forced or error is not None),'compile group closed without unexplained survivor')
    except BaseException as exc:err(exc,'join')
   for f in streams:
    for action in(f.flush,lambda f=f:os.fsync(f.fileno()),f.close):
     try:action()
     except BaseException as exc:err(exc,'log-close')
   event={'phase':name,'argv':argv,'pid':None if p is None else p.pid,'exitCode':None if p is None else p.returncode,'groupClosed':joined,'elapsedMs':round(1000*(time.monotonic()-start)),'firstFailure':None if error is None else str(error)[:1500],'secondaryFailures':others};build_events.append(event)
   try:m.put(O/(name+'.terminal-public.json'),event)
   except BaseException as exc:err(exc,'event')
   for sig,h in previous.items():
    try:signal.signal(sig,h)
    except BaseException as exc:err(exc,'restore-signal')
  if error is not None:raise error
 def dependency(path,obj,known):
  raw=path.read_text().replace('\\\n',' ');target,body=raw.split(':',1);need(shlex.split(target)==[str(obj)],'exact shared dependency MT');paths=set()
  for word in shlex.split(body):
   spelling=os.path.abspath(os.path.join(str(m.D),word));resolved=str(pathlib.Path(spelling).resolve(strict=True));paths.add(resolved)
   if resolved in known:need(m.pin(resolved)==fresh[resolved],'new case dependency fixed pin')
   else:need(resolved in old['files']and m.pin(resolved)==old['files'][resolved]and(spelling==resolved or old['dependencyAliases'].get(spelling)==resolved),'dependency must match original row/alias')
  need(known<=paths,'both exact /case inputs in dependency closure');return paths
 def signal_handler(sig,frame):
  nonlocal cancelled
  cancelled=True;raise RuntimeError('successor interrupted by '+str(sig))
 try:
  O.mkdir(mode=0o700);owned=True;need(stat.S_IMODE(own_row['st_mode'])==0o400,'successor source frozen');retained[str(AUTH)]=authority_row;retained[str(PARENT)]=parent_row;retained[str(pathlib.Path(__file__).resolve())]=own_row
  for sig in(signal.SIGINT,signal.SIGTERM,signal.SIGHUP):handlers[sig]=signal.signal(sig,signal_handler)
  old_guard();helper=m.S/'tools/storage-maintenance/bootstrap-readonly-replica.hpp';cpp_raw,cpp_row=read(CPP,CPP_PIN);helper_raw,helper_row=read(helper,HELPER_PIN);need(helper_row==old['files'][str(helper)]and stat.S_IMODE(cpp_row['st_mode'])==0o400,'original helper row and frozen successor source');retained[str(CPP)]=cpp_row
  target=O/'case/tests/qualification/beacon_journal_physical_rollback.cpp';support=O/'case/tools/storage-maintenance/bootstrap-readonly-replica.hpp'
  case_creators={}
  for dst,raw in((target,cpp_raw),(support,helper_raw)):
   dst.parent.mkdir(mode=0o700,parents=True,exist_ok=True);fd=os.open(dst,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
   with os.fdopen(fd,'wb')as f:
    f.write(raw);f.flush();os.fsync(f.fileno());st=os.fstat(f.fileno());row={k:getattr(st,k)for k in F};row['sha256']=hashlib.sha256(raw).hexdigest();need(st.st_size==len(raw)and stat.S_IMODE(st.st_mode)==0o400 and st.st_nlink==1,'new case creator known bytes/size/mode');case_creators[str(dst)]=row;m.put(O/('case-creator-'+str(len(case_creators))+'-public.json'),{'path':str(dst),'creatorRow':row,'knownSha256':row['sha256'],'productionQualified':False})
  admit('case',case_creators);old_guard()
  try:
   commands=m.decoded((m.D/'compile_commands.json').read_text());entries=[c for c in commands if pathlib.Path(c['file']).resolve()==m.S/'tests/qualification/beacon_journal_physical_rollback.cpp'];need(len(entries)==1,'one original journal compile recipe');c=entries[0];need(pathlib.Path(c['directory']).resolve()==m.D.resolve(),'original build cwd');args=shlex.split(c['command']);old_obj=(m.D/c['output']).resolve();need(str(old_obj)in old['files'],'original object row');src=str(pathlib.Path(c['file']));base=[];seen_c=seen_o=0;i=0
   while i<len(args):
    if args[i]=='-c':need(i+1<len(args)and args[i+1]==src,'only original -c source removed');seen_c+=1;i+=2
    elif args[i]=='-o':need(i+1<len(args)and (m.D/args[i+1]).resolve()==old_obj,'only original -o object removed');seen_o+=1;i+=2
    else:need(args[i]not in('-M','-MM','-MD','-MMD','-MF','-MT','-MQ'),'original recipe has no hidden dependency-output replacement');base.append(args[i]);i+=1
   need(seen_c==seen_o==1,'one exact source/object pair');obj=O/'new.o';md=O/'new.d';pre=O/'new.M';exe=O/'nano-beacon-journal-physical-rollback';known={str(target),str(support)}
   product_creators={};build('preprocess',base+['-M','-MT',str(obj),'-MF',str(pre),str(target)]);product_creators[str(pre)]=product_creator(pre,'preprocess');predeps=dependency(pre,obj,known)
   build('compile',base+['-MD','-MT',str(obj),'-MF',str(md),'-c',str(target),'-o',str(obj)]);product_creators[str(obj)]=product_creator(obj,'object');product_creators[str(md)]=product_creator(md,'dependency');need(dependency(md,obj,known)==predeps,'normalized M/MD closure equal')
   recipe=m.D/'CMakeFiles'/f'{m.J}.dir/link.txt';link=shlex.split(recipe.read_text());need(not any(t in('&&',';','|','>','<')for t in link),'one argv link recipe');changed=0
   for i,t in enumerate(link):
    if not t.startswith('-')and(m.D/t).resolve()==old_obj:link[i]=str(obj);changed+=1
   need(changed==1 and link.count('-o')==1,'only old journal object replaced');at=link.index('-o');need((m.D/link[at+1]).resolve()==m.D/m.J,'old journal executable output');link[at+1]=str(exe);build('link',link);os.chmod(exe,0o500)
   product_creators[str(exe)]=product_creator(exe,'executable');admit('products',product_creators)
   old_guard();j=m.command('journal',[str(exe),str(O/'journal-stores')],1)[0];p=O/'journal-stores/commitments.json';r=m.pin(p);need(r['sha256']==j['externalCommitmentSha256']and r['st_size']==j['externalCommitmentBytes'],'journal independently retained commitment');retained[str(p)]=r;cases['journal']={'passed':True,'record':j}
  except BaseException as exc:cases['journal']={'passed':False,'reason':str(exc)[:1500]};failure(exc,'journal')
 except BaseException as exc:failure(exc,'preparation-or-original-guard')
 finally:
  for sig in handlers:
   try:signal.signal(sig,signal.SIG_IGN)
   except BaseException as exc:failure(exc,'closing-signal')
  try:final['originals']=old_guard()
  except BaseException as exc:final['originals']={'passed':False};failure(exc,'final-originals')
  for label,rows in(('newInputs',fresh),('retained',retained)):
   changed=[]
   for p,row in rows.items():
    try:need(m.pin(p)==row,'held source/output changed: '+p)
    except BaseException as exc:changed.append(p);failure(exc,'final-'+label)
   final[label]={'passed':not changed,'files':len(rows),'changed':changed}
  for sig,h in handlers.items():
   try:signal.signal(sig,h)
   except BaseException as exc:failure(exc,'restore-signal')
  if owned:
   try:m.put(O/'final-fences-public.json',final)
   except BaseException as exc:failure(exc,'fence-publication')
 if owned:
  report={'format':'nano-native-retained-core-journal-successor-v1','passed':first is None and len(cases)==1 and all(c['passed']for c in cases.values()),'cases':cases,'newInputAdmissions':admissions,'firstFailure':None if first is None else{'type':type(first).__name__,'message':str(first)[:2000]},'secondaryFailures':secondary,'finalFences':final,'buildEvents':build_events,'runtimeEvents':m.EVENTS,'oldSourceRows':9541,'oldInputs':3334,'oldDirectoryInventoryNamesTypes':1252,'directoryObservations':directory_observations,'qualifiedScope':'original-consumed-file-fd9-and-source-bytes-with-original-directory-identities','strictRuntimeOriginalFileRows':len(RUNTIME_RESOURCES),'historicalSourceFileFD9Qualified':False,'historicalDirectoryExactModesQualified':False,'historicalDirectoryTimestampsQualified':False,'wholeTreeDirectoryFD9Qualified':False,'sdkOrBuildParentDirectoryFD9Qualified':False,'priorNativeSuccessor01FAILED':True,'priorNativeSuccessor02FAILED':True,'priorNativeSuccessor03FAILED':True,'priorNativeSuccessor04FAILED':True,'nativePhysicalCohortQualified':False,'qualifiedRuntimeScope':'journal-cpp-v3-only','maximumRuntimeCliInvocations':1,'cppCompilations':sum(x['phase']=='compile'for x in build_events),'coreRebuilt':False,'runtimeCliInvocations':len(m.EVENTS),'maximumNewPublicSignatures':1,'priorNative04FAILED':True,'parent14bQualificationReusedForNewTu':False,'runtimeDutiesAuthorized':False,'operatorRecoveryQualified':False,'commonFinalityQualified':False,'productionQualified':False};m.put(O/'result-public.json',report);print(json.dumps({'passed':report['passed'],'runtimeCliInvocations':len(m.EVENTS),'productionQualified':False}),flush=True)
  return 0 if report['passed']else 1
 return 1
if __name__=='__main__':
 try:need(len(sys.argv)==3,'usage: native_successor.py SHA256 BYTES');raise SystemExit(main((sys.argv[1],int(sys.argv[2]))))
 except Exception as exc:print(json.dumps({'passed':False,'prestartFailure':str(exc)[:2000],'productionQualified':False}),file=sys.stderr);raise SystemExit(1)
