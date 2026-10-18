"""SOURCE_ONLY: one public USTAR -> new local volume -> optional C8 Core checker RO."""
import datetime,hashlib,io,json,os,pathlib,re,selectors,signal,stat,subprocess,sys,tarfile,time,types
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
FIELDS={'format','ready','runner','common','docker','runtimeImage','archive','archiveOwner','manifestSha256','outputRoot','volumeName','owner','checker'}
COMMON_PIN={'sha256':'09422e11287ce5f0357e3b3e9e5219d6c492ac2a3d3815aa6bbb6aa2ade7943c','bytes':10200}
LABEL='nano.cold-restore.owner';MAX_BYTES=128*1024*1024
SCAN=r'''/usr/bin/find /state/public-cohort -xdev -exec /bin/sh -ec '
fmt="%d|%i|%f|%h|%u|%g|%s|%y|%z"
for p do
 exec 3<"$p"
 a=$(/usr/bin/stat -L --printf="$fmt" /proc/self/fd/3)
 test "$a" = "$(/usr/bin/stat --printf="$fmt" -- "$p")"
 printf "%s\0" "$p"
 /usr/bin/stat -L --printf="%d\0%i\0%f\0%h\0%u\0%g\0%s\0%y\0%z\0" /proc/self/fd/3
 if [ -f /proc/self/fd/3 ]; then h=$(/usr/bin/sha256sum /proc/self/fd/3); printf "%s\0" "${h%% *}"; else printf "%s\0" "-"; fi
 test "$a" = "$(/usr/bin/stat -L --printf="$fmt" /proc/self/fd/3)"
 test "$a" = "$(/usr/bin/stat --printf="$fmt" -- "$p")"
 exec 3<&-
done' sh {} +'''
SETUP='/bin/chmod 0700 /state; /bin/chown 10001:10001 /state; test "$(/usr/bin/stat -c %u:%g:%a /state)" = 10001:10001:700'
EXTRACT='umask 0077; test -z "$(/usr/bin/find /state -mindepth 1 -maxdepth 1 -print -quit)"; exec /bin/tar --extract --file=- --directory=/state --no-same-owner --no-same-permissions --keep-old-files --delay-directory-restore'
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
def absent_stdout(raw):
 if type(raw)is not bytes:return False
 if not raw.strip():return True
 try:value=decode(raw)
 except (ValueError,TypeError,UnicodeDecodeError):return False
 return type(value)is list and len(value)==0
def row_of(s):return {k:getattr(s,k)for k in F}
def sha_text(v):return type(v)is str and re.fullmatch('[0-9a-f]{64}',v)is not None
def row_valid(r):return type(r)is dict and set(r)==set(F)|{'sha256'}and all(type(r[k])is int and r[k]>=0 for k in F)and sha_text(r['sha256'])and 0<r['st_size']<=MAX_BYTES

def main(config_path,config_pin):
 os.umask(0o077);sys.dont_write_bytecode=True
 need(type(config_pin)is dict and set(config_pin)=={'sha256','bytes'}and sha_text(config_pin['sha256'])and type(config_pin['bytes'])is int and 0<config_pin['bytes']<=1048576,'external bounded config SHA/bytes')
 held={};out=None;outfd=None;out_identity=None;log_total=0;first=None;secondary=[];events=[];containers={};prefix=None;image_id=None;image_info=None;volume=None;owner=None;volume_attempted=False;volume_verified=False;inventory=None;manifest=None;admitted=None;final_rows=None;records=None;import_complete=False;final_image=False;stage='configuration';started=time.monotonic();deadline=started+180
 def fail(exc,phase):
  nonlocal first
  item={'stage':phase,'type':type(exc).__name__,'message':str(exc)[:2000]}
  if first is None:first=item
  else:secondary.append(item)
 def bounded(closing=False):need(time.monotonic()<deadline,'closing deadline'if closing else'cold restore 180s deadline')
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
  output_guard();need(re.fullmatch('[a-zA-Z0-9_.-]+',name),'plain exclusive output leaf');return os.fdopen(os.open(name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o600,dir_fd=outfd),'wb')
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
   p=subprocess.Popen(argv,stdin=pipe_input if pipe_input is not None else subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True,env={'PATH':os.environ.get('PATH','/usr/bin:/bin'),'HOME':os.environ.get('HOME','/'),'LC_ALL':'C','LANG':'C','TZ':'UTC'})
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
 def docker(label,args,seconds=20,allowed=(0,),closing=False,stdin_archive=False):return command(label,prefix+args,seconds,allowed,stdin_archive,closing)
 def image(label,closing=False):
  _,raw,err=docker(label,['image','inspect',image_id],closing=closing);need(not err,'image inspect stderr');a=decode(raw);need(type(a)is list and len(a)==1,'one image');x=a[0]
  need(x['Id']==image_id and x['Os']=='linux'and x['Architecture']in('arm64','amd64')and x['Config'].get('User')=='10001:10001'and not x['Config'].get('Volumes'),'final immutable Linux image, UID10001, no anonymous volumes');return {'id':image_id,'architecture':x['Architecture'],'user':'10001:10001'}
 def volume_fence(label,closing=False):
  _,raw,err=docker(label,['volume','inspect',volume],closing=closing);need(not err,'volume inspect stderr');a=decode(raw);need(type(a)is list and len(a)==1,'one volume');x=a[0];need(x['Name']==volume and x['Driver']=='local'and not x.get('Options')and x.get('Labels',{}).get(LABEL)==owner,'exact owned local volume');return True
 def inspect(label,role,target,closing=False):
  rc,raw,err=docker(label,['container','inspect',target],allowed=(0,1),closing=closing)
  if rc==1:need(absent_stdout(raw)and(b'no such object'in err.lower()or b'no such container'in err.lower())and target.encode()in err,'exact owned-container absence');return None
  a=decode(raw);need(type(a)is list and len(a)==1,'one container');x=a[0];need(re.fullmatch('[0-9a-f]{64}',x['Id'])and x['Name']=='/'+containers[role]['name']and x['Image']==image_id and x['Config']['Labels'].get(LABEL)==owner and x['Config']['Labels'].get(LABEL+'.role')==role,'exact CID/name/image/owner/role');return x
 def remove(role,closing=False):
  item=containers[role];x=inspect(role+'-cleanup-inspect',role,item.get('cid')or item['name'],closing)
  if x is not None:
   cid=x['Id']
   if x['State']['Running']:docker(role+'-cleanup-stop',['stop','--time','5',cid],10,closing=closing)
   x=inspect(role+'-cleanup-stopped',role,cid,closing);need(x is not None and x['State']['Running']is False,'owned container stopped');docker(role+'-cleanup-remove',['rm',cid],closing=closing)
  need(inspect(role+'-cleanup-absent',role,item['name'],closing)is None,'owned container absent');item['removed']=True
 def run(role,args,writable=False,setup=False,checker=False,stdin_archive=False,closing=False):
  need(role not in containers,'unique container role');name='nano-cold-'+role+'-'+owner;containers[role]={'name':name,'cid':None,'removed':False}
  need(inspect(role+'-name-before',role,name,closing)is None,'unused owned container name');mounts=['--mount','type=volume,src='+volume+',dst=/state'+(''if writable else',readonly')]
  if checker:mounts+=['--mount','type=bind,src='+c['checker']['file']+',dst=/checker,readonly']
  user='0:0'if setup else'10001:10001';a=['create','--pull=never','--no-healthcheck','--name',name,'--label',LABEL+'='+owner,'--label',LABEL+'.role='+role,'--network','none','--read-only','--user',user,'--cpus','2','--memory','512m','--pids-limit','64','--cap-drop','ALL','--security-opt','no-new-privileges','--workdir','/','--env','HOME=/nonexistent','--env','LC_ALL=C','--env','LANG=C','--env','TZ=UTC',*mounts]
  if setup:a+=['--cap-add','CHOWN']
  if stdin_archive:a+=['--interactive']
  _,raw,err=docker(role+'-create',a+['--entrypoint',args[0],image_id,*args[1:]],closing=closing);need(not err,'create stderr');cid=raw.decode().strip();need(re.fullmatch('[0-9a-f]{64}',cid),'full created CID');containers[role]['cid']=cid;x=inspect(role+'-created',role,cid,closing);hc=x['HostConfig'];ms=x['Mounts'];vs=[m for m in ms if m['Type']=='volume'];bs={m['Destination']:(m['Source'],m['RW'])for m in ms if m['Type']=='bind'}
  expected_bind={'/checker':(c['checker']['file'],False)}if checker else{}
  if checker and bs=={'/checker':('/host_mnt'+c['checker']['file'],False)}:bs=expected_bind
  need(len(vs)==1 and vs[0]['Name']==volume and vs[0]['Destination']=='/state'and vs[0]['RW']is writable and bs==expected_bind and len(ms)==1+int(checker),'only new volume and optional exact checker bind')
  need(hc['NetworkMode']=='none'and hc['ReadonlyRootfs']is True and hc['Memory']==536870912 and hc['NanoCpus']==2000000000 and hc['PidsLimit']==64 and set(hc.get('CapDrop')or[])=={'ALL'}and set(hc.get('CapAdd')or[])==({'CAP_CHOWN'}if setup else set())and 'no-new-privileges'in(hc.get('SecurityOpt')or[])and not hc.get('PortBindings')and not hc.get('Tmpfs')and x['Config']['User']==user and x['Config'].get('Healthcheck',{}).get('Test')==['NONE'],'actual isolated UID/caps/resources/nohealthcheck')
  rc,stdout,stderr=docker(role+'-start',['start','--attach']+(['--interactive']if stdin_archive else[])+[cid],min(180,max(1,int(deadline-time.monotonic()))),allowed=None,closing=closing,stdin_archive=stdin_archive);x=inspect(role+'-terminal',role,cid,closing);s=x['State'];save(role+'-terminal-public.json',{'cliExitCode':rc,'containerId':cid,'state':s})
  need(rc==0 and s['Status']=='exited'and s['Running']is False and type(s['ExitCode'])is int and s['ExitCode']==0 and s.get('OOMKilled')is False and not s.get('Error')and not s.get('FinishedAt','0001').startswith('0001'),'CLI0 and independent StateExit0/noOOM');need(not stderr,'guest stderr empty');remove(role,closing);return stdout
 def ns(s):
  m=re.fullmatch(r'(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d)\.(\d{9}) \+0000',s);need(m is not None,'full GNUstat nanosecond UTC');return int(datetime.datetime.strptime(m[1],'%Y-%m-%d %H:%M:%S').replace(tzinfo=datetime.timezone.utc).timestamp())*1000000000+int(m[2])
 def survey(label,closing=False):
  raw=run(label,['/bin/sh','-ec',SCAN],closing=closing);parts=raw.decode('ascii').split('\0');need(parts.pop()==''and len(parts)==69*11,'exact69 guest FD9/SHA records');rows={}
  for i in range(0,len(parts),11):
   v=parts[i:i+11];need(v[0].startswith('/state/'),'closed guest prefix');name=v[0][7:];need(name in inventory['entries']and name not in rows,'known unique tar member');expected=inventory['entries'][name];r=dict(zip(F,[int(v[1]),int(v[2]),int(v[3],16),int(v[4]),int(v[5]),int(v[6]),int(v[7]),ns(v[8]),ns(v[9])]))
   need(r['st_uid']==10001 and r['st_gid']==10001 and all(k>=0 for k in r.values()),'guest10001/full9');isdir=expected['type']=='directory';need((stat.S_ISDIR(r['st_mode'])and stat.S_IMODE(r['st_mode'])==0o700 and v[10]=='-')if isdir else(stat.S_ISREG(r['st_mode'])and stat.S_IMODE(r['st_mode'])==0o600 and r['st_nlink']==1 and r['st_size']==expected['bytes']and v[10]==expected['sha256']),'new destination type/modes/known SHA bytes');r['kind']=expected['type']
   if not isdir:r['sha256']=v[10]
   rows[name]=r
  need(set(rows)==set(inventory['entries']),'exact69 imported names');return rows
 def parse_records(raw):
  need(len(raw)<=65536,'bounded checker3records');rs=[decode(x)for x in raw.decode().splitlines()];need(len(rs)==3,'two cohort records plus summary')
  for r,cohort,children in zip(rs[:2],manifest['cohorts'],(2,3)):
   audit={'canonicalHistoryVerified':True,'externalFinalizedAncestorVerified':False,'requiredFinalized':None,'finalized':cohort['observation']['finalized'],'historicalForkChoiceVerified':False,'historicalStandaloneVotesReplayed':False,'trustedAnchor':manifest['anchor']['root'],'firstExecutionBlock':'0x1','lastExecutionBlock':hex(children+1),'blocksIncludingTrustedAnchor':children+1,'transactions':0,'prunedExecutionStatesReplayed':0,'allHistoricalStatesMaterialized':True,'archiveBytes':sum(a['wire']['bytes']for a in cohort['archives']),'head':cohort['headRoot'],'checkpointSequence':int(cohort['primary']['token']['sequence'],16),'checkpointDigest':cohort['primary']['token']['digest']}
   expected={'scenario':'physical-authenticated-cohort','mode':'check','result':'PASS','name':cohort['name'],'archivesAuthenticated':children,'journalProposalBoundToArchivedBlock':True,'pendingBlsChanges':1,'coldClosureAuthenticated':True,'signedAnchorAuthenticatedOnBothStores':True,'executionCoupled':True,'allFourStoresReadOnly':True,'closedPhysicalInventoryUnchanged':True,'independentReadonlyPasses':2,'primaryToken':cohort['primary']['token'],'witnessToken':cohort['witness']['token'],'headRoot':cohort['headRoot'],'canonicalHistoryAudit':audit,'externalFinalizedBeforeAnchorRefused':True};need(same(r,expected),'closed typed cohort+Core audit bound to external manifest')
  expected={'scenario':'physical-authenticated-cohort-summary','mode':'check','result':'PASS','manifestSha256':c['manifestSha256'],'cohorts':2,'databases':8,'archivesAuthenticated':5,'independentReadonlyPasses':2,'newTestOnlySignaturesCreated':0,'newSignaturesInCheck':0,'maximumRssKiB':131072,'maximumWallSeconds':180,'noMissingStoreCreation':True,'automaticRecoveryUsed':False,'runtimeDutiesAuthorized':False,'networkTransport':False,'productionQualified':False,'finalityAdvanceQualified':False};r=rs[2];need(type(r)is dict and set(r)==set(expected)|{'actualPeakRssKiB'}and type(r['actualPeakRssKiB'])is int and 1<=r['actualPeakRssKiB']<=131072 and same({k:r[k]for k in expected},expected),'closed typed summary');return rs
 handlers={}
 def interrupted(sig,frame):raise KeyboardInterrupt('owned cold restore signal '+str(sig))
 try:
  raw=hold(config_path,known=config_pin);c=decode(raw);need(type(c)is dict and set(c)==FIELDS and c['format']=='nano-public-c8-cold-restore-control-v1'and c['ready']is True,'closed ready13-field Root control')
  need(c['runner']['file']==str(pathlib.Path(__file__).absolute()),'runner self binding');descriptor(c['runner']);common_raw=descriptor(c['common']);need(pin(common_raw)==COMMON_PIN,'unchanged common09422')
  need(all(stat.S_IMODE(held[c[k]['file']]['row']['st_mode'])==0o400 for k in('runner','common')),'Root sources frozen before admission')
  common=types.ModuleType('cold_restore_reviewed_common');common.__file__=c['common']['file'];exec(compile(common_raw,c['common']['file'],'exec'),common.__dict__);need(tuple(common.F)==F,'common full9 interface')
  d=c['docker'];need(type(d)is dict and set(d)=={'prefix','executable'}and type(d['prefix'])is list and d['prefix']and all(type(x)is str and x and '\0'not in x for x in d['prefix']),'Root Docker prefix');descriptor(d['executable']);need(d['executable']['file']in d['prefix']and pathlib.Path(d['prefix'][0]).is_absolute(),'prefix binds pinned Docker executable');prefix=list(d['prefix'])
  archive_raw=descriptor(c['archive']);need(type(c['archiveOwner'])is list and len(c['archiveOwner'])==2 and all(type(v)is int and v>0 for v in c['archiveOwner']),'closed positive integer archive UID/GID pair');need(sha_text(c['manifestSha256']),'independent manifest SHA256');need(c['checker']is None or type(c['checker'])is dict,'optional checker descriptor')
  checker_raw=None
  if c['checker']is not None:
   need(','not in c['checker']['file']and '\n'not in c['checker']['file']and '\r'not in c['checker']['file'],'plain bind source');checker_raw=descriptor(c['checker']);r=held[c['checker']['file']]['row'];need(checker_raw[:6]==b'\x7fELF\x02\x01'and len(checker_raw)>20 and int.from_bytes(checker_raw[18:20],'little')in(62,183)and stat.S_IMODE(r['st_mode'])&0o005==0o005 and r['st_mode']&0o022==0,'known ELF64LE guest-readable/executable; no old chmod')
  image_id=c['runtimeImage'];need(type(image_id)is str and re.fullmatch('sha256:[0-9a-f]{64}',image_id),'immutable runtime image ID');owner=c['owner'];volume=c['volumeName'];need(type(owner)is str and re.fullmatch('[0-9a-f]{32}',owner)and volume=='nano-cold-restore-'+owner,'new closed volume/owner names')
  candidate_out=pathlib.Path(c['outputRoot']);need(candidate_out.is_absolute()and candidate_out.parent.resolve(strict=True)==candidate_out.parent and not candidate_out.exists()and not candidate_out.is_symlink()and all(not pathlib.Path(p).is_relative_to(candidate_out)and not candidate_out.is_relative_to(p)for p in held),'fresh physical output outside all held inputs');candidate_out.mkdir(mode=0o700);out=candidate_out;outfd=os.open(out,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);out_identity=tuple(getattr(os.fstat(outfd),k)for k in('st_dev','st_ino','st_mode','st_uid','st_gid'));output_guard()
  save('source-input-pins-public.json',{'files':{p:{'sha256':h['row']['sha256'],'bytes':h['row']['st_size']}for p,h in held.items()},'originalRowsNeverRefreshed':True,'configurationHistoricalFD9Qualified':False})
  stage='strict-tar-validation';manifest_raw=None
  with tarfile.open(fileobj=io.BytesIO(archive_raw),mode='r:')as t:
   for i,m in enumerate(t):
    bounded();need(i<4096,'bounded archive entries')
    if m.name.rstrip('/')=='public-cohort/manifest.json':
     need(manifest_raw is None and m.isfile()and 0<m.size<=1048576,'one bounded manifest member');f=t.extractfile(m);need(f is not None,'manifest bytes');manifest_raw=f.read(1048577);need(len(manifest_raw)==m.size,'manifest size')
  need(manifest_raw is not None and pin(manifest_raw)['sha256']==c['manifestSha256'],'tar manifest externally committed SHA');manifest=decode(manifest_raw)
  inventory=common.validate_tar(c['archive']['file'],manifest_raw,deadline,pin(manifest_raw),tuple(c['archiveOwner']));need(inventory['entryCount']==69 and sum(x['type']=='directory'for x in inventory['entries'].values())==11 and len(inventory['portableRoleInventories'])==8,'known69 entries,11dirs,8 role digests');input_fence();save('archive-validated-public.json',inventory)
  for sig in(signal.SIGINT,signal.SIGTERM,signal.SIGHUP):handlers[sig]=signal.signal(sig,interrupted)
  stage='image';image_info=image('image-before')
  if checker_raw is not None:need(int.from_bytes(checker_raw[18:20],'little')=={'amd64':62,'arm64':183}[image_info['architecture']],'checker/image architecture')
  stage='new-volume';rc,raw,err=docker('volume-name-before',['volume','inspect',volume],allowed=(0,1));need(rc==1 and absent_stdout(raw)and b'no such volume'in err.lower()and volume.encode()in err,'unused named volume');volume_attempted=True;docker('volume-create',['volume','create','--driver','local','--label',LABEL+'='+owner,volume]);volume_verified=volume_fence('volume-created')
  stage='import';run('setup',['/bin/sh','-ec',SETUP],True,True);run('extract',['/bin/sh','-ec',EXTRACT],True,stdin_archive=True);need(all(x['removed']for x in containers.values()),'RW carriers removed before RO admission')
  begin=time.monotonic();base=None;last=None;since=None;streak=0
  for attempt in range(1,16):
   current=survey('admit-'+str(attempt));now=time.monotonic();fixed={p:{k:v for k,v in r.items()if k!='st_ctime_ns'}for p,r in current.items()};error=None
   if base is None:base=fixed
   elif not same(base,fixed):error='new guest bytes/eight fields changed before first admission'
   if last is not None and same(current,last):streak+=1
   else:streak=1;since=now
   save('guest-observation-'+str(attempt)+'-public.json',{'rows':current,'elapsedMs':round((now-begin)*1000),'stableSamples':streak,'stableElapsedMs':round((now-since)*1000),'refusal':error});need(error is None,error);need(time.monotonic()-begin<=30,'new guest admission30s includes persisted samples')
   if streak>=3 and now-since>=2:admitted=current;break
   last=current;time.sleep(1)
  need(admitted is not None,'three equal guest FD9/SHA samples>=2s within30s');input_fence();save('guest-admission-public.json',{'rows':admitted,'sourceArchiveOwner':c['archiveOwner'],'newGuestOwner':[10001,10001],'historicalSourceFD9Requalified':False,'stableSamples':streak,'elapsedMs':round((time.monotonic()-begin)*1000)})
  import_complete=True;save('import-result-public.json',{'passed':True,'phase':'import','volume':volume,'retained':True,'entries':69,'portableRoleInventories':8,'archivePin':pin(archive_raw),'manifestSha256':c['manifestSha256'],'sourceArchiveOwner':c['archiveOwner'],'allRWCarriersRemoved':True,'productionQualified':False})
  if c['checker']is not None:
   stage='check';before=survey('before-check');need(same(before,admitted),'admitted guest full9/SHA before check');records=parse_records(run('check',['/checker','check','/state/public-cohort',c['manifestSha256']],checker=True));save('checker-records-public.json',records);after=survey('after-check');need(same(after,admitted),'admitted guest full9/SHA after checker removal')
 except BaseException as exc:fail(exc,stage)
 finally:
  deadline=time.monotonic()+120
  for sig in handlers:
   try:signal.signal(sig,signal.SIG_IGN)
   except BaseException as exc:fail(exc,'closing-signals')
  for role in list(containers):
   if not containers[role]['removed']:
    try:remove(role,True)
    except BaseException as exc:fail(exc,'cleanup-'+role)
  if admitted is not None and all(x['removed']for x in containers.values()):
   try:final_rows=survey('after-cleanup-audit',True);save('guest-final-inventory-public.json',{'rows':final_rows,'afterCheckerCleanup':records is not None});need(same(final_rows,admitted),'strict admitted guest full9/SHA after cleanup')
   except BaseException as exc:fail(exc,'guest-final-fence')
  for role in list(containers):
   if not containers[role]['removed']:
    try:remove(role,True)
    except BaseException as exc:fail(exc,'final-auditor-cleanup-'+role)
  if prefix is not None and volume_attempted:
   try:volume_verified=volume_fence('volume-retained-final',True)
   except BaseException as exc:fail(exc,'volume-final-fence')
  if prefix is not None and image_info is not None:
   try:need(same(image('image-final',True),image_info),'immutable final image metadata');final_image=True
   except BaseException as exc:fail(exc,'image-final-fence')
  original_ok=True
  try:input_fence()
  except BaseException as exc:original_ok=False;fail(exc,'original-input-final-fence')
  for path,h in held.items():
   try:os.close(h['fd'])
   except BaseException as exc:original_ok=False;fail(exc,'held-fd-close-'+path)
  for sig,handler in handlers.items():
   try:signal.signal(sig,handler)
   except BaseException as exc:fail(exc,'restore-signals')
  cleanup={'passed':all(x['removed']for x in containers.values()),'containers':containers,'volumeRetained':volume_verified,'volumeName':volume};fence={'passed':original_ok and admitted is not None and final_rows is not None and same(admitted,final_rows),'originalHostFiles':len(held),'hostAndGuestMetadataViewsSeparate':True,'guestEntries':69,'afterCleanup':cleanup['passed']}
  if out is not None:
   try:save('input-final-fence-public.json',fence)
   except BaseException as exc:fail(exc,'fence-publication')
   checker_requested=isinstance(locals().get('c'),dict)and c.get('checker')is not None
   passed=first is None and import_complete and(checker_requested is False or records is not None)and cleanup['passed']and volume_verified and final_image and fence['passed']
   report={'format':'nano-public-c8-cold-restore-root-v1','passed':passed,'requiresControllerExit0':True,'importCompleted':import_complete,'checkerRequested':checker_requested,'checkerCompleted':records is not None,'checkRecords':records,'image':image_info,'volumeRetained':volume_verified,'volumeName':volume,'entries':69,'portableRoleInventories':8,'sourceArchiveOwner':c['archiveOwner'],'newGuestOwner':[10001,10001],'cppCompilations':0,'produceInvocations':0,'newSignaturesCreated':0,'installedNanoAuditCliQualified':False,'historicalFinalityQualified':False,'runtimeDutiesAuthorized':False,'operatorRecoveryQualified':False,'productionQualified':False,'inputFinalFence':fence,'cleanup':cleanup,'firstFailure':first,'secondaryFailures':secondary,'events':events}
   try:save('result-public.json',report)
   except BaseException as exc:fail(exc,'result-publication');passed=False
  else:
   passed=False;print(json.dumps({'passed':False,'prestartFailure':first,'secondaryFailures':secondary,'productionQualified':False}),file=sys.stderr)
  if outfd is not None:
   try:os.close(outfd)
   except BaseException as exc:fail(exc,'output-directory-close');passed=False
 return 0 if passed else 1
if __name__=='__main__':
 try:
  need(len(sys.argv)==4,'usage: runner.py CONFIG CONFIG_SHA256 CONFIG_BYTES');raise SystemExit(main(sys.argv[1],{'sha256':sys.argv[2],'bytes':int(sys.argv[3])}))
 except Exception as exc:print(json.dumps({'passed':False,'prestartFailure':str(exc)[:2000],'productionQualified':False}),file=sys.stderr);raise SystemExit(1)
