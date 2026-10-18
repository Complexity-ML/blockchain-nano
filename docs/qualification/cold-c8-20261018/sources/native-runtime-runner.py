"""SOURCE_ONLY: one new native copy; one retained C8 Mach-O check, no produce/build."""
import hashlib,io,json,os,pathlib,re,selectors,signal,stat,subprocess,sys,tarfile,time,types
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
FIELDS={'format','ready','runner','common','archive','archiveOwner','manifestSha256','reader','providers','nativeOwner','outputRoot','restoreRoot'}
COMMON_PIN={'sha256':'09422e11287ce5f0357e3b3e9e5219d6c492ac2a3d3815aa6bbb6aa2ade7943c','bytes':10200}
CORE_PIN={'sha256':'ec2d12064b6f8606823b7d6ac53857dbd9953cca9046b800853c335a46acfc2a','bytes':29416184}
MANIFEST_PIN={'sha256':'e37934823aa91b2ea44b4259451fde17d807ede634d6d01888f2860646b2b02a','bytes':12445}
MAX_BYTES=128*1024*1024
IDENTITY=('st_dev','st_ino','st_mode','st_uid','st_gid')
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def decode(raw):
 def pairs(items):
  d={}
  for k,v in items:need(k not in d,'duplicate JSON key');d[k]=v
  return d
 return json.loads(raw,object_pairs_hook=pairs,parse_constant=lambda _:(_ for _ in()).throw(ValueError('nonfinite JSON')))
def same(a,b):return json.dumps(a,sort_keys=True,allow_nan=False)==json.dumps(b,sort_keys=True,allow_nan=False)
def row_of(s):return {k:getattr(s,k)for k in F}
def sha_text(v):return type(v)is str and re.fullmatch('[0-9a-f]{64}',v)is not None
def row_valid(r):return type(r)is dict and set(r)==set(F)|{'sha256'}and all(type(r[k])is int and r[k]>=0 for k in F)and sha_text(r['sha256'])and 0<r['st_size']<=MAX_BYTES

def main(config_path,config_pin):
 os.umask(0o077);sys.dont_write_bytecode=True
 need(type(config_pin)is dict and set(config_pin)=={'sha256','bytes'}and sha_text(config_pin['sha256'])and type(config_pin['bytes'])is int and 0<config_pin['bytes']<=1048576,'external bounded config SHA/bytes')
 held={};out=None;outfd=None;out_identity=None;log_total=0;first=None;secondary=[];events=[];inventory=None;manifest=None;admitted=None;final_rows=None;records=None;restore=None;restore_fd=None;restore_identity=None;directories={};creators={};case_started=False;case_closed=False;sources_frozen=False;stage='configuration';started=time.monotonic();deadline=started+300
 def fail(exc,phase):
  nonlocal first
  item={'stage':phase,'type':type(exc).__name__,'message':str(exc)[:2000]}
  if first is None:first=item
  else:secondary.append(item)
 def bounded(closing=False):need(time.monotonic()<deadline,'closing deadline'if closing else'native cold restore 300s deadline')
 def read_fd(fd,n):
  pieces=[];offset=0
  while offset<n:
   bounded();chunk=os.pread(fd,min(65536,n-offset),offset);need(chunk,'held input truncated');pieces.append(chunk);offset+=len(chunk)
  need(not os.pread(fd,1,n),'held input grew');return b''.join(pieces)
 def hold(path,original=None,known=None):
  p=pathlib.Path(path);need(p.is_absolute()and p.resolve(strict=True)==p and str(p)not in held,'unique canonical physical input')
  fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
  try:
   s=os.fstat(fd);r=row_of(s);need(stat.S_ISREG(s.st_mode)and s.st_nlink==1 and 0<s.st_size<=MAX_BYTES,'bounded regular single-link input')
   raw=read_fd(fd,s.st_size);need(r==row_of(os.fstat(fd))==row_of(p.lstat()),'initial input FD9/path changed');r['sha256']=pin(raw)['sha256']
   if original is not None:need(row_valid(original)and same(r,original),'original full9/SHA: '+str(p))
   if known is not None:need(pin(raw)==known,'known bytes: '+str(p))
   held[str(p)]={'fd':fd,'row':r};return raw
  except BaseException:
   os.close(fd);raise
 def input_fence():
  for p,h in held.items():
   s=os.fstat(h['fd']);need(row_of(s)=={k:h['row'][k]for k in F}==row_of(pathlib.Path(p).lstat()),'held original FD9/path: '+p)
   raw=read_fd(h['fd'],s.st_size);need(pin(raw)['sha256']==h['row']['sha256']and row_of(os.fstat(h['fd']))==row_of(s)==row_of(pathlib.Path(p).lstat()),'held bytes/FD9 after read: '+p)
 def descriptor(d):
  need(type(d)is dict and set(d)=={'file','originalRow'}and type(d['file'])is str and row_valid(d['originalRow']),'closed Root originalRow descriptor');return hold(d['file'],d['originalRow'])
 def output_guard():
  need(outfd is not None and out is not None and out.resolve(strict=True)==out,'owned canonical output root')
  current=os.fstat(outfd);entry=out.lstat();need(tuple(getattr(current,k)for k in('st_dev','st_ino','st_mode','st_uid','st_gid'))==out_identity==tuple(getattr(entry,k)for k in('st_dev','st_ino','st_mode','st_uid','st_gid')),'owned output root identity unchanged')
 def output_file(name):
  output_guard();need(re.fullmatch('[a-zA-Z0-9_.-]+',name),'plain exclusive output leaf');fd=os.open(name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o600,dir_fd=outfd)
  try:os.fchown(fd,-1,20);return os.fdopen(fd,'wb')
  except BaseException:
   try:os.close(fd)
   except BaseException as exc:secondary.append({'stage':'new-output-close','type':type(exc).__name__,'message':str(exc)[:2000]})
   raise
 def save(name,value):
  need(out is not None and re.fullmatch('[a-zA-Z0-9_.-]+',name),'owned plain output leaf');raw=(json.dumps(value,indent=2,sort_keys=True,allow_nan=False)+'\n').encode();need(len(raw)<=4*1024*1024,'bounded JSON output')
  with output_file(name)as f:f.write(raw);f.flush();os.fsync(f.fileno())
  os.fsync(outfd);output_guard()
 def command(label,argv,seconds=20,allowed=(0,),stdin_archive=False,closing=False):
  nonlocal log_total
  bounded(closing);input_fence();start=time.monotonic();limit=min(seconds,max(.1,deadline-start));p=None;sel=selectors.DefaultSelector();streams=[];counts=[0,0];error=None;joined=False;pipe_input=None
  def note(exc,phase):
   nonlocal error
   if error is None:error=exc
   else:secondary.append({'stage':label+'-'+phase,'type':type(exc).__name__,'message':str(exc)[:2000]})
  try:
   for suffix in('stdout','stderr'):streams.append(output_file(label+'.'+suffix+'.log'))
   if stdin_archive:
    pipe_input=os.open(c['archive']['file'],os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK);need(row_of(os.fstat(pipe_input))=={k:held[c['archive']['file']]['row'][k]for k in F},'original archive FD before stdin')
   p=subprocess.Popen(argv,stdin=pipe_input if pipe_input is not None else subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True,env={'PATH':'/usr/bin:/bin','HOME':'/nonexistent','TMPDIR':str(out),'LC_ALL':'C','LANG':'C','TZ':'UTC'})
   need(p.pid>1 and p.pid!=28722,'only controller-owned safe process groups')
   for i,pipe in enumerate((p.stdout,p.stderr)):os.set_blocking(pipe.fileno(),False);sel.register(pipe,selectors.EVENT_READ,(i,streams[i]))
   while sel.get_map():
    need(time.monotonic()-start<limit,'owned command timeout')
    for key,_ in sel.select(.05):
     chunk=os.read(key.fd,65536)
     if not chunk:sel.unregister(key.fileobj);continue
     i,dest=key.data;counts[i]+=len(chunk);log_total+=len(chunk);need(counts[i]<=2*1024*1024 and log_total<=16*1024*1024,'per-stream2MiB/aggregate16MiB log bound');dest.write(chunk)
   p.wait(timeout=max(.1,limit-(time.monotonic()-start)))
  except BaseException as exc:error=exc
  finally:
   if p is not None:
    try:
     need(p.pid>1 and p.pid!=28722,'safe owned group cleanup')
     def alive():
      try:os.killpg(p.pid,0);return True
      except ProcessLookupError:return False
     if alive():os.killpg(p.pid,signal.SIGTERM)
     try:p.wait(timeout=3)
     except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=3)
     if alive():os.killpg(p.pid,signal.SIGKILL)
     until=time.monotonic()+3
     while alive()and time.monotonic()<until:time.sleep(.02)
     joined=not alive();need(joined,'owned group joined/absent')
    except BaseException as exc:note(exc,'join')
    for name,pipe in(('stdout',p.stdout),('stderr',p.stderr)):
     if pipe is not None:
      try:pipe.close()
      except BaseException as exc:note(exc,name+'-pipe-close')
   try:sel.close()
   except BaseException as exc:note(exc,'selector-close')
   for i,f in enumerate(streams):
    for action in('flush','fsync','close'):
     try:
      if action=='flush':f.flush()
      elif action=='fsync':os.fsync(f.fileno())
      else:f.close()
     except BaseException as exc:note(exc,'log-'+str(i)+'-'+action)
   if pipe_input is not None:
    try:need(row_of(os.fstat(pipe_input))=={k:held[c['archive']['file']]['row'][k]for k in F},'archive FD after stdin')
    except BaseException as exc:note(exc,'stdin-fence')
    try:os.close(pipe_input)
    except BaseException as exc:note(exc,'stdin-close')
   try:input_fence()
   except BaseException as exc:note(exc,'original-input-fence')
   event={'phase':label,'pid':None if p is None else p.pid,'exitCode':None if p is None else p.returncode,'groupClosed':joined,'elapsedMs':round((time.monotonic()-start)*1000),'stdoutBytes':counts[0],'stderrBytes':counts[1]};events.append(event)
   try:save(label+'.exit-public.json',event)
   except BaseException as exc:note(exc,'event-publication')
  if error is not None:raise error
  need(joined and(allowed is None or p.returncode in allowed),'command exit: '+label)
  output_guard();return p.returncode,(out/(label+'.stdout.log')).read_bytes(),(out/(label+'.stderr.log')).read_bytes()
 def parse_records(raw):
  need(len(raw)<=65536,'bounded checker3records');rs=[decode(x)for x in raw.decode().splitlines()];need(len(rs)==3,'two cohort records plus summary')
  for r,cohort,children in zip(rs[:2],manifest['cohorts'],(2,3)):
   audit={'canonicalHistoryVerified':True,'externalFinalizedAncestorVerified':False,'requiredFinalized':None,'finalized':cohort['observation']['finalized'],'historicalForkChoiceVerified':False,'historicalStandaloneVotesReplayed':False,'trustedAnchor':manifest['anchor']['root'],'firstExecutionBlock':'0x1','lastExecutionBlock':hex(children+1),'blocksIncludingTrustedAnchor':children+1,'transactions':0,'prunedExecutionStatesReplayed':0,'allHistoricalStatesMaterialized':True,'archiveBytes':sum(a['wire']['bytes']for a in cohort['archives']),'head':cohort['headRoot'],'checkpointSequence':int(cohort['primary']['token']['sequence'],16),'checkpointDigest':cohort['primary']['token']['digest']}
   expected={'scenario':'physical-authenticated-cohort','mode':'check','result':'PASS','name':cohort['name'],'archivesAuthenticated':children,'journalProposalBoundToArchivedBlock':True,'pendingBlsChanges':1,'coldClosureAuthenticated':True,'signedAnchorAuthenticatedOnBothStores':True,'executionCoupled':True,'allFourStoresReadOnly':True,'closedPhysicalInventoryUnchanged':True,'independentReadonlyPasses':2,'primaryToken':cohort['primary']['token'],'witnessToken':cohort['witness']['token'],'headRoot':cohort['headRoot'],'canonicalHistoryAudit':audit,'externalFinalizedBeforeAnchorRefused':True};need(same(r,expected),'closed typed cohort+Core audit bound to external manifest')
  expected={'scenario':'physical-authenticated-cohort-summary','mode':'check','result':'PASS','manifestSha256':c['manifestSha256'],'cohorts':2,'databases':8,'archivesAuthenticated':5,'independentReadonlyPasses':2,'newTestOnlySignaturesCreated':0,'newSignaturesInCheck':0,'maximumRssKiB':131072,'maximumWallSeconds':180,'noMissingStoreCreation':True,'automaticRecoveryUsed':False,'runtimeDutiesAuthorized':False,'networkTransport':False,'productionQualified':False,'finalityAdvanceQualified':False};r=rs[2];need(type(r)is dict and set(r)==set(expected)|{'actualPeakRssKiB'}and type(r['actualPeakRssKiB'])is int and 1<=r['actualPeakRssKiB']<=131072 and same({k:r[k]for k in expected},expected),'closed typed summary');return rs
 def directory_check():
  need(restore_fd is not None and restore.resolve(strict=True)==restore,'new canonical restore namespace')
  need(tuple(getattr(os.fstat(restore_fd),k)for k in IDENTITY)==restore_identity==tuple(getattr(restore.lstat(),k)for k in IDENTITY),'new restore namespace creation identity')
  for name,item in directories.items():
   p=restore/pathlib.PurePosixPath(name);need(tuple(getattr(os.fstat(item['fd']),k)for k in IDENTITY)==item['identity']==tuple(getattr(p.lstat(),k)for k in IDENTITY),'created directory identity: '+name)
 def survey():
  bounded();input_fence();directory_check();need(set(os.listdir(restore))=={'public-cohort'},'exact sole restored tree')
  captured=common.snapshot(restore/'public-cohort',(501,20));rows={'public-cohort'+('/'+n if n else''):r for n,r in captured.items()}
  need(set(rows)==set(inventory['entries'])and len(rows)==69,'exact69 native imported names')
  for name,r in rows.items():
   expected=inventory['entries'][name];need(r['kind']==expected['type']and(r['st_uid'],r['st_gid'])==(501,20),'known native type/owner')
   if r['kind']=='file':need(stat.S_ISREG(r['st_mode'])and stat.S_IMODE(r['st_mode'])==0o600 and r['st_nlink']==1 and r['st_size']==expected['bytes']and r['sha256']==expected['sha256'],'known native file mode/bytes/SHA')
   else:need(stat.S_ISDIR(r['st_mode'])and stat.S_IMODE(r['st_mode'])==0o700,'native directory0700')
  directory_check();input_fence();bounded();return rows
 def extract(archive_raw):
  for name,row in sorted(inventory['entries'].items(),key=lambda item:(item[0].count('/'),item[0])):
   if row['type']!='directory':continue
   bounded();directory_check();parts=name.rsplit('/',1);parent=restore_fd if len(parts)==1 else directories[parts[0]]['fd'];leaf=parts[-1]
   os.mkdir(leaf,0o700,dir_fd=parent);fd=os.open(leaf,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW,dir_fd=parent);directories[name]={'fd':fd,'identity':None};os.fchown(fd,-1,20);s=os.fstat(fd);identity=tuple(getattr(s,k)for k in IDENTITY);directories[name]['identity']=identity
   need(stat.S_ISDIR(s.st_mode)and stat.S_IMODE(s.st_mode)==0o700 and(s.st_uid,s.st_gid)==(501,20),'new directory owner/mode');creators[name]={'kind':'directory','identity':dict(zip(IDENTITY,identity))};save('creator-'+str(len(creators))+'.json',{'name':name,**creators[name]});os.fsync(parent)
  seen=set()
  with tarfile.open(fileobj=io.BytesIO(archive_raw),mode='r:')as t:
   for m in t:
    bounded();name=m.name.rstrip('/');need(name in inventory['entries']and name not in seen,'validated unique extraction member');seen.add(name);expected=inventory['entries'][name]
    if expected['type']=='directory':need(m.isdir()and m.size==0,'validated directory member');continue
    need(m.isfile()and m.size==expected['bytes'],'validated regular member size/type');parent_name,leaf=name.rsplit('/',1);parent=directories[parent_name]['fd'];fd=os.open(leaf,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o600,dir_fd=parent);f=None;error=None
    try:
     os.fchown(fd,-1,20);f=t.extractfile(m);need(f is not None,'regular archive stream');h=hashlib.sha256();n=0
     while n<m.size:
      bounded();chunk=f.read(min(65536,m.size-n));need(chunk and len(chunk)<=m.size-n,'bounded complete member bytes');h.update(chunk);offset=0
      while offset<len(chunk):written=os.write(fd,chunk[offset:]);need(written>0,'complete new file write');offset+=written
      n+=len(chunk)
     need(not f.read(1),'no extra member bytes');os.fsync(fd);s=os.fstat(fd);r=row_of(s);creator={k:r[k]for k in F[:8]};creator['sha256']=h.hexdigest();creators[name]={'kind':'file','creationEightAndSha':creator};save('creator-'+str(len(creators))+'.json',{'name':name,**creators[name]})
     need(stat.S_ISREG(s.st_mode)and stat.S_IMODE(s.st_mode)==0o600 and s.st_nlink==1 and(s.st_uid,s.st_gid)==(501,20)and n==s.st_size==expected['bytes']and h.hexdigest()==expected['sha256'],'new file creation8/known bytes')
     actual=pin(read_fd(fd,s.st_size));need(actual=={'sha256':expected['sha256'],'bytes':expected['bytes']}and {k:getattr(os.fstat(fd),k)for k in F[:8]}=={k:r[k]for k in F[:8]}=={k:getattr(os.stat(leaf,dir_fd=parent,follow_symlinks=False),k)for k in F[:8]},'creator FD8/path and byte readback');os.fsync(parent)
    except BaseException as exc:error=exc
    finally:
     for label,close in(('archive-stream',None if f is None else f.close),('new-file',lambda:os.close(fd))):
      if close is not None:
       try:close()
       except BaseException as exc:
        if error is None:error=exc
        else:secondary.append({'stage':'extract-'+label,'type':type(exc).__name__,'message':str(exc)[:2000]})
    if error is not None:raise error
  need(seen==set(inventory['entries'])and set(creators)==seen,'all69 entries created exactly once')
  for item in reversed(list(directories.values())):os.fsync(item['fd'])
  os.fsync(restore_fd);directory_check();input_fence();save('new-creation-anchors-public.json',{'entries':creators,'historicalAuthority':False,'admitted':False})
 handlers={}
 def interrupted(sig,frame):raise KeyboardInterrupt('owned native cold restore signal '+str(sig))
 try:
  raw=hold(config_path,known=config_pin);c=decode(raw);need(type(c)is dict and set(c)==FIELDS and c['format']=='nano-public-c8-native-cold-control-v1'and c['ready']is True,'closed ready12-field Root native control')
  need(c['nativeOwner']==[501,20]and all(type(x)is int for x in c['nativeOwner'])and os.geteuid()==501 and os.getegid()==20 and sys.platform=='darwin'and os.uname().machine=='arm64','actual native Darwin ARM64 UID501:GID20')
  need(c['runner']['file']==str(pathlib.Path(__file__).absolute()),'runner self binding');descriptor(c['runner']);common_raw=descriptor(c['common']);need(pin(common_raw)==COMMON_PIN,'byte-exact common09422')
  need(all(stat.S_IMODE(held[c[k]['file']]['row']['st_mode'])==0o400 for k in('runner','common')),'Root source files frozen0400 before execution');sources_frozen=True
  common=types.ModuleType('native_cold_reviewed_common');common.__file__=c['common']['file'];exec(compile(common_raw,c['common']['file'],'exec'),common.__dict__);need(tuple(common.F)==F,'common full9 interface')
  archive_raw=descriptor(c['archive']);reader_raw=descriptor(c['reader']);need(reader_raw[:4]==bytes.fromhex('cffaedfe')and len(reader_raw)>=32 and int.from_bytes(reader_raw[4:8],'little')==0x100000c and held[c['reader']['file']]['row']['st_mode']&0o111 and not held[c['reader']['file']]['row']['st_mode']&0o022,'Root original executable ARM64 Mach-O; no chmod')
  need(type(c['providers'])is list and len(c['providers'])==7,'one original Native C8 Core then six original providers')
  for i,d in enumerate(c['providers']):
   data=descriptor(d)
   if i==0:need(pin(data)==CORE_PIN,'original qualified Native C8 Core')
  need(c['archiveOwner']==[1000,1000]and all(type(x)is int for x in c['archiveOwner'])and c['manifestSha256']==MANIFEST_PIN['sha256'],'explicit source AMD archive owner and external manifest')
  for sig in(signal.SIGINT,signal.SIGTERM,signal.SIGHUP):handlers[sig]=signal.signal(sig,interrupted)
  stage='strict-tar-validation';manifest_raw=None
  with tarfile.open(fileobj=io.BytesIO(archive_raw),mode='r:')as t:
   for i,m in enumerate(t):
    bounded();need(i<4096,'bounded archive entries')
    if m.name.rstrip('/')=='public-cohort/manifest.json':
     need(manifest_raw is None and m.isfile()and m.size==MANIFEST_PIN['bytes'],'one bounded committed manifest');f=t.extractfile(m);need(f is not None,'manifest stream')
     with f:manifest_raw=f.read(MANIFEST_PIN['bytes']+1)
     need(len(manifest_raw)==m.size,'manifest exact size')
  need(manifest_raw is not None and pin(manifest_raw)==MANIFEST_PIN,'external known AMD manifest bytes');manifest=decode(manifest_raw)
  inventory=common.validate_tar(c['archive']['file'],manifest_raw,deadline,MANIFEST_PIN,tuple(c['archiveOwner']));need(inventory['entryCount']==69 and sum(x['type']=='directory'for x in inventory['entries'].values())==11 and len(inventory['portableRoleInventories'])==8,'exact69 entries11dirs8 portable role inventories');input_fence()
  candidate_out=pathlib.Path(c['outputRoot']);candidate_restore=pathlib.Path(c['restoreRoot'])
  for p in(candidate_out,candidate_restore):need(p.is_absolute()and p!=pathlib.Path(p.root) and p==pathlib.Path(os.path.normpath(p))and p.parent.resolve(strict=True)==p.parent and not p.exists()and not p.is_symlink()and all(not pathlib.Path(x).is_relative_to(p)and not p.is_relative_to(x)for x in held),'fresh physical namespaces outside held originals')
  need(candidate_restore.parent==pathlib.Path('/private/tmp')and candidate_restore.name.startswith('nano-cold-c8-native-')and not candidate_out.is_relative_to(candidate_restore)and not candidate_restore.is_relative_to(candidate_out),'copy only under private/tmp; separate output')
  candidate_out.mkdir(mode=0o700);out=candidate_out;outfd=os.open(out,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);os.fchown(outfd,-1,20);out_identity=tuple(getattr(os.fstat(outfd),k)for k in IDENTITY);need((os.fstat(outfd).st_uid,os.fstat(outfd).st_gid)==(501,20)and stat.S_IMODE(os.fstat(outfd).st_mode)==0o700,'output owner/mode');output_guard();os.fsync(outfd)
  save('source-input-pins-public.json',{'files':{p:{'sha256':h['row']['sha256'],'bytes':h['row']['st_size']}for p,h in held.items()},'originalRowsNeverRefreshed':True,'configurationHistoricalFD9Qualified':False});save('archive-validated-public.json',inventory)
  stage='fresh-extraction';candidate_restore.mkdir(mode=0o700);restore=candidate_restore;restore_fd=os.open(restore,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);os.fchown(restore_fd,-1,20);restore_identity=tuple(getattr(os.fstat(restore_fd),k)for k in IDENTITY);need((os.fstat(restore_fd).st_uid,os.fstat(restore_fd).st_gid)==(501,20)and stat.S_IMODE(os.fstat(restore_fd).st_mode)==0o700,'new copy owner/mode');directory_check();extract(archive_raw)
  stage='prospective-copy-admission';begin=time.monotonic();base=None;last=None;since=None;streak=0
  for attempt in range(1,31):
   current=survey();now=time.monotonic();fixed={p:{k:v for k,v in r.items()if k!='st_ctime_ns'}for p,r in current.items()};error=None
   for name,r in current.items():
    creator=creators[name]
    if r['kind']=='file'and not same({k:r[k]for k in F[:8]}|{'sha256':r['sha256']},creator['creationEightAndSha']):error='new file creation8/SHA changed before first admission'
    if r['kind']=='directory'and not same({k:r[k]for k in IDENTITY},creator['identity']):error='new directory creation identity changed'
   if base is None:base=fixed
   elif not same(base,fixed):error='new native eight fields/bytes/names changed before first admission'
   if last is not None and same(current,last):streak+=1
   else:streak=1;since=now
   save('new-copy-sample-'+str(attempt)+'-public.json',{'rows':current,'elapsedMs':round((now-begin)*1000),'stableSamples':streak,'stableElapsedMs':round((now-since)*1000),'refusal':error});need(error is None,error);need(time.monotonic()-begin<=30,'admission30s includes saved observations')
   if streak>=3 and now-since>=2:admitted=current;break
   last=current;time.sleep(1)
  need(admitted is not None,'three equal new full9/SHA samples>=2s within30s');need(same(survey(),admitted),'immediate strict fence of exact adopted sample');save('new-copy-admission-public.json',{'rows':admitted,'newNativeOwner':[501,20],'stableSamples':streak,'elapsedMs':round((time.monotonic()-begin)*1000),'exactFinalSampleAdopted':True,'oldMetadataRefreshed':False})
  stage='native-check';need(same(survey(),admitted),'admitted native full9 before check');case_started=True
  _,stdout,stderr=command('reader-check',[c['reader']['file'],'check',str(restore/'public-cohort'),c['manifestSha256']],180);case_closed=events[-1]['groupClosed'];need(not stderr,'native checker stderr empty');records=parse_records(stdout);save('checker-records-public.json',records);need(same(survey(),admitted),'strict new native full9/SHA after check')
 except BaseException as exc:fail(exc,stage)
 finally:
  deadline=time.monotonic()+60
  case_closed=any(e['phase']=='reader-check'and e['groupClosed']is True for e in events)
  for sig in handlers:
   try:signal.signal(sig,signal.SIG_IGN)
   except BaseException as exc:fail(exc,'closing-signals')
  if admitted is not None:
   try:final_rows=survey();save('native-final-inventory-public.json',{'rows':final_rows,'afterOwnedGroupClosed':case_closed});need(same(final_rows,admitted),'strict native69 full9/SHA final fence')
   except BaseException as exc:fail(exc,'native-final-fence')
  original_ok=True
  try:input_fence()
  except BaseException as exc:original_ok=False;fail(exc,'original-input-final-fence')
  for name,item in reversed(list(directories.items())):
   try:os.close(item['fd'])
   except BaseException as exc:fail(exc,'new-directory-close-'+name)
  if restore_fd is not None:
   try:os.close(restore_fd)
   except BaseException as exc:fail(exc,'restore-namespace-close')
  for path,h in held.items():
   try:os.close(h['fd'])
   except BaseException as exc:original_ok=False;fail(exc,'held-input-close-'+path)
  for sig,handler in handlers.items():
   try:signal.signal(sig,handler)
   except BaseException as exc:fail(exc,'restore-signals')
  fence={'passed':original_ok and admitted is not None and final_rows is not None and same(admitted,final_rows),'originalHostFiles':len(held),'newNativeEntries':69,'originalRowsNeverRefreshed':True,'nativeIdentitiesNotImportedFromLinux':True,'afterOwnedGroupClosed':case_closed}
  if out is not None:
   try:save('input-final-fence-public.json',fence)
   except BaseException as exc:fail(exc,'fence-publication')
   passed=first is None and sources_frozen and records is not None and case_closed and fence['passed']
   report={'format':'nano-public-c8-native-cold-root-v1','passed':passed,'requiresControllerExit0':True,'checkRecords':records,'checkInvocationAttempted':case_started,'checkCompleted':records is not None,'ownedGroupClosed':case_closed,'entries':69,'portableRoleInventories':8,'newNativeOwner':[501,20],'restoredTree':None if restore is None else str(restore/'public-cohort'),'retainedCopy':restore is not None,'sourceArchiveOwner':[1000,1000],'cppCompilations':0,'produceInvocations':0,'newSignaturesCreated':0,'declaredOriginalProviders':7,'providerScope':'Root-declared-original-files-only','loadedLibraryClosureIndependentlyMeasured':False,'operatingSystemReadonlyMount':False,'installedNanoAuditCliQualified':False,'historicalFinalityQualified':False,'runtimeDutiesAuthorized':False,'networkStarted':False,'operatorRecoveryQualified':False,'productionQualified':False,'inputFinalFence':fence,'firstFailure':first,'secondaryFailures':secondary,'events':events}
   try:save('result-public.json',report)
   except BaseException as exc:fail(exc,'result-publication');passed=False
  else:passed=False;print(json.dumps({'passed':False,'prestartFailure':first,'secondaryFailures':secondary,'productionQualified':False}),file=sys.stderr)
  if outfd is not None:
   try:os.close(outfd)
   except BaseException as exc:fail(exc,'output-directory-close');passed=False
 return 0 if passed else 1
if __name__=='__main__':
 try:
  need(len(sys.argv)==4,'usage: runner.py CONFIG CONFIG_SHA256 CONFIG_BYTES');raise SystemExit(main(sys.argv[1],{'sha256':sys.argv[2],'bytes':int(sys.argv[3])}))
 except Exception as exc:print(json.dumps({'passed':False,'prestartFailure':str(exc)[:2000],'productionQualified':False}),file=sys.stderr);raise SystemExit(1)
