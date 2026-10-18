"""SOURCE_ONLY Root host: one new AMD rollback TU, retained original C7 providers."""
import hashlib,json,os,pathlib,re,selectors,signal,stat,subprocess,sys,time,uuid
PYTHON_CONTROL=pathlib.Path('/home/ubuntu/main-http-python-provider-root-20261018-02/control.json');PYTHON_CONTROL_PIN={'sha256':'6d1969aac717c7f980a6845756158d52fe5ce0436c1c13511094ac019df97100','bytes':241262}
V=pathlib.Path('/home/ubuntu');PUBLIC=V/'nano-real-journal-rollback-public-root-20261018-04';OUT=V/'real-journal-rollback-amd-root-20261018-04'
CONTROL=V/'physical-cohort-amd-control-root-20261018-02/control.private.json';CONTROL_PIN={'sha256':'689c5951523ed44f281023b163d0c0654006f49d188b09e458066084f3ca05ba','bytes':185358}
CORE_PIN={'sha256':'dc4af8d2c04e684844205d2d6b4c5d99d9ebb6074a6197148e16e489006cad8e','bytes':41076890};CPP_PIN={'sha256':'2e5ef451f13cca770cf02128bbb389eaae60379974252859829e197d14521acd','bytes':28801};HELPER_PIN={'sha256':'d3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125','bytes':20319}
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
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
 p=pathlib.Path(path);need(p.is_absolute()and p.resolve(strict=True)==p,'physical absolute input');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);r={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and a.st_nlink>=1 and 0<=a.st_size<=134217728,'bounded regular input');parts=[];n=0
  while b:=os.read(fd,65536):n+=len(b);need(n<=a.st_size,'input grew');parts.append(b)
  raw=b''.join(parts);need(n==a.st_size and r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'full9/path changed');return dict(r,sha256=pin(raw)['sha256']),raw
 finally:os.close(fd)
def main(admission_file,admission_pin):
 need(os.geteuid()==os.getegid()==1000,'host1000:1000');os.umask(0o077);held={};first=None;secondary=[];events=[];prefix=None;cid=None;created=False;cleanup=None;final_image=None;wr=None;changes=[];deadline=time.monotonic()+600;out=OUT
 def error(exc,stage):
  nonlocal first
  v={'stage':stage,'type':type(exc).__name__,'message':str(exc)[:2000]}
  if first is None:first=v
  else:secondary.append(v)
 def hold(path,want=None,wantpin=None):
  row,raw=material(path)
  if want is not None:need(type(want)is dict and set(want)==set(F)|{'sha256'}and same(row,want),'ORIGINAL full9/SHA mismatch: '+str(path))
  if wantpin is not None:need(pin(raw)==wantpin,'known pin mismatch: '+str(path))
  need(str(path)not in held or same(held[str(path)],row),'held row refresh forbidden');held.setdefault(str(path),row);return raw
 need(type(admission_pin)is dict and set(admission_pin)=={'sha256','bytes'}and type(admission_pin['bytes'])is int and 0<admission_pin['bytes']<=1048576 and re.fullmatch('[0-9a-f]{64}',admission_pin['sha256']),'external source-admission pin');admission=body(hold(pathlib.Path(admission_file),wantpin=admission_pin));need(set(admission)=={'format','ready','files'}and admission['format']=='nano-real-journal-rollback-amd-source-admission-v1'and admission['ready']is True,'ready source admission')
 files={str(PUBLIC/x)for x in('amd_host_v5.py','amd_worker_v3.py','beacon_journal_physical_rollback.cpp','bootstrap-readonly-replica.hpp')};need(type(admission['files'])is dict and set(admission['files'])==files and pathlib.Path(__file__).absolute()==PUBLIC/'amd_host_v5.py','exact four staged sources/self path');raws={}
 for path,row in admission['files'].items():raws[path]=hold(path,row);need(row['st_nlink']==1 and stat.S_IMODE(row['st_mode'])==0o400,'sources frozen before first use')
 need(pin(raws[str(PUBLIC/'beacon_journal_physical_rollback.cpp')])==CPP_PIN and pin(raws[str(PUBLIC/'bootstrap-readonly-replica.hpp')])==HELPER_PIN,'fixed CPP/helper bytes');old=body(hold(CONTROL,wantpin=CONTROL_PIN));need(old['format']=='nano-physical-cohort-amd-control-v1'and old['ready']is True,'original physical AMD control')
 d=old['docker'];candidate=d['prefix'];need(type(candidate)is list and len(candidate)==7 and candidate[:3]==['/usr/bin/sudo','-n','/usr/bin/docker']and d['image']==IMAGE and d['architecture']=='amd64'and d['uid']==d['gid']==1000,'original Docker/image/owner');tail=candidate[3:];need(tail[0::2]in(['--config','--host'],['--host','--config']),'local Docker prefix');options=dict(zip(tail[0::2],tail[1::2]));need(options['--host']=='unix:///var/run/docker.sock'and pathlib.Path(options['--config']).resolve(strict=True)==pathlib.Path(options['--config']),'physical local Docker config');core=old['core'];libs=old['libraries'];need(core['guest']=='/retained/libnano_core.a'and core['members']==143 and core['pin']==CORE_PIN and type(libs)is list and len(libs)==6,'retained143 C7 Core/six libraries');originals=old['originalFiles'];selected=[candidate[0],candidate[2],core['file'],*[x['file']for x in libs]];need(len(set(selected))==9,'exact9 original software paths')
 for path in selected:need(path in originals,'genuine original software row');hold(path,originals[path])
 for x in[core,*libs]:need(pin(hold(x['file'],originals[x['file']]))==x['pin'],'old provider original pin')
 roots=set()
 for x in libs:
  need(x['guest'].startswith('/old/')and '..'not in pathlib.PurePosixPath(x['guest']).parts,'safe original library guest');suffix=pathlib.PurePosixPath(x['guest']).relative_to('/old');p=pathlib.Path(x['file']);root=p
  for _ in suffix.parts:root=root.parent
  need(root/suffix==p,'original library path join');roots.add(root)
 need(len(roots)==1,'one original library root mount');libroot=roots.pop();src=pathlib.Path(old['sourceRoot']['root']);sourcepins=old['sourceRoot']['pins'];need(type(sourcepins)is dict and len(sourcepins)==1047 and src.resolve(strict=True)==src and libroot.resolve(strict=True)==libroot,'known original source root/1047 content pins');known={x['guest']:x['pin']for x in[core,*libs]};need(len(known)==7,'unique provider guests');flags=old['compilerFlags'];lp=old['linkPrefix'];lt=old['linkTail'];need(all(type(x)is list and x and all(type(v)is str and v and '\0'not in v for v in x)for x in(flags,lp,lt))and flags[0]=='/usr/bin/clang++'and lt.count(core['guest'])==1 and all(x['guest']in lt for x in libs)and all(not x.startswith('/old/')or x in known for x in lt),'original explicit TU/link recipe')
 py=body(hold(PYTHON_CONTROL,wantpin=PYTHON_CONTROL_PIN));need(type(py)is dict and set(py)=={'format','ready','runtimeRoot','guestRoot','interpreter','version','files','directories','environment','flags','provenance'}and py['format']=='nano-test-python-runtime-control-v1'and py['ready']is True and py['guestRoot']=='/python-runtime'and py['interpreter']=='bin/python3'and py['version']=='3.12.3'and py['flags']==['-S'],'known exact test Python control')
 pyenv={'PYTHONHOME':'/python-runtime','PYTHONDONTWRITEBYTECODE':'1','PYTHONNOUSERSITE':'1','PYTHONSAFEPATH':'1','PYTHONPYCACHEPREFIX':'/tmp/nano-python-cache'};need(same(py['environment'],pyenv)and type(py['provenance'])is dict and len(json.dumps(py['provenance']).encode())<=65536,'explicit test-only Python environment');pyroot=pathlib.Path(py['runtimeRoot']);need(pyroot==V/'main-http-python-provider-root-20261018-02/runtime'and pyroot.resolve(strict=True)==pyroot,'fixed isolated provider02 root');need(type(py['files'])is dict and len(py['files'])==628 and type(py['directories'])is list and len(py['directories'])==len(set(py['directories']))==51,'known628files51dirs provider')
 pfiles=set();pdirs=set();total=0;empty=0
 for rel in py['directories']:
  part=pathlib.PurePosixPath(rel);need(type(rel)is str and rel and not part.is_absolute()and '..'not in part.parts and part.as_posix()==rel and len(rel)<=512,'canonical provider directory');pdirs.add(str(pyroot/rel))
 for rel,row in py['files'].items():
  part=pathlib.PurePosixPath(rel);need(type(rel)is str and rel not in('','.')and not part.is_absolute()and '..'not in part.parts and part.as_posix()==rel and len(rel)<=512,'canonical provider leaf');need(row['st_uid']==row['st_gid']==1000 and stat.S_IMODE(row['st_mode'])==(0o500 if rel=='bin/python3'else 0o400)and row['st_nlink']==1,'original copied provider owner/mode');hold(pyroot/rel,row);total+=row['st_size'];empty+=int(row['st_size']==0)
  if row['st_size']==0:need(row['sha256']==hashlib.sha256(b'').hexdigest(),'known empty provider SHA')
  pfiles.add(str(pyroot/rel));known['/python-runtime/'+rel]={'sha256':row['sha256'],'bytes':row['st_size']}
 need(total==31232658 and empty==5 and str(pyroot/'bin/python3')in pfiles and str(pyroot/'lib/python3.12/encodings/__init__.py')in pfiles,'complete bounded known Python provider')
 def python_inventory():
  found=set();dirs={};pending=[pyroot];entries=0
  while pending:
   path=pending.pop();need(path.resolve(strict=True)==path,'physical provider directory');fd=os.open(path,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
   try:
    stamp=os.fstat(fd);row={k:getattr(stamp,k)for k in F};need(stat.S_ISDIR(stamp.st_mode)and stat.S_IMODE(stamp.st_mode)==0o500 and stamp.st_uid==stamp.st_gid==1000,'provider directory owner/mode');dirs[str(path)]=row
    with os.scandir(path)as children:
     for child in children:
      entries+=1;need(entries<=20000 and not child.is_symlink(),'bounded alias-free provider inventory')
      if child.is_dir(follow_symlinks=False):pending.append(pathlib.Path(child.path))
      else:need(child.is_file(follow_symlinks=False),'regular provider file');found.add(child.path)
    need(row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(path.lstat(),k)for k in F},'provider directory FD9/path unchanged')
   finally:os.close(fd)
  need(found==pfiles and set(dirs)==pdirs,'exact628files51dirs closed provider inventory');return dirs
 python_before=python_inventory()
 need(not OUT.exists()and not OUT.is_symlink()and OUT.parent.resolve(strict=True)==OUT.parent,'fresh fixed output before mutation');OUT.mkdir(mode=0o700);outputs=OUT/'outputs';outputs.mkdir(mode=0o700);case=OUT/'case';case.mkdir(mode=0o700)
 def save(name,value):
  raw=(json.dumps(value,sort_keys=True,indent=2,allow_nan=False)+'\n').encode();fd=os.open(OUT/name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
  with os.fdopen(fd,'wb')as f:
   f.write(raw);f.flush();os.fsync(f.fileno())
   if name=='worker-config.json':stamp=os.fstat(f.fileno());creation[str(OUT/name)]={k:getattr(stamp,k)for k in F if k!='st_ctime_ns'}
  fd=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
  try:os.fsync(fd)
  finally:os.close(fd)
 project={'/work/'+r:p for r,p in sourcepins.items()};copies={};creation={}
 for rel,source,pinned in[('tests/qualification/beacon_journal_physical_rollback.cpp',PUBLIC/'beacon_journal_physical_rollback.cpp',CPP_PIN),('tools/storage-maintenance/bootstrap-readonly-replica.hpp',PUBLIC/'bootstrap-readonly-replica.hpp',HELPER_PIN)]:
  target=case/rel;target.parent.mkdir(mode=0o700,parents=True,exist_ok=True);fd=os.open(target,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
  with os.fdopen(fd,'wb')as f:f.write(raws[str(source)]);f.flush();os.fchmod(f.fileno(),0o400);os.fsync(f.fileno());s=os.fstat(f.fileno());creation[str(target)]={k:getattr(s,k)for k in F if k!='st_ctime_ns'}
  copies[str(target)]=pinned;project['/case/'+rel]=pinned
 cfg={'compilerFlags':flags,'linkPrefix':lp,'linkTail':lt,'projectPins':project,'knownPins':known,'workerPin':pin(raws[str(PUBLIC/'amd_worker_v3.py')])};save('worker-config.json',cfg);copies[str(OUT/'worker-config.json')]=pin((json.dumps(cfg,sort_keys=True,indent=2,allow_nan=False)+'\n').encode());start=time.monotonic();last=None;stable=0;since=0
 for n in range(1,11):
  table={p:material(p)[0]for p in copies};save('new-case-admission-'+str(n)+'.json',{'rows':table,'elapsedMs':round((time.monotonic()-start)*1000)});now=time.monotonic();need(now-start<=30,'new-case first admission deadline')
  for p,row in table.items():need({'sha256':row['sha256'],'bytes':row['st_size']}==copies[p]and(p not in creation or{k:row[k]for k in F if k!='st_ctime_ns'}==creation[p]),'new known bytes/creation8 unchanged')
  if same(table,last):stable+=1
  else:stable=1;since=now
  last=table
  if stable>=3 and now-since>=2:break
  time.sleep(1.05)
 need(stable>=3 and now-since>=2,'3 stable samples2s');
 for p,row in table.items():hold(p,row)
 prefix=list(candidate);name='nano-journal-rollback-'+uuid.uuid4().hex;label='nano.real-journal-rollback.owner'
 def command(phase,args,seconds=20,stdin_path=None,allowed=(0,)):
  limit=min(seconds,deadline-time.monotonic());need(limit>0,'bounded overall deadline');save(phase+'.argv-public.json',args);p=None;err=None;joined=False;start=time.monotonic();streams=[];inputfd=None;sel=selectors.DefaultSelector();counts={}
  try:
   if stdin_path is not None:inputfd=os.open(stdin_path,os.O_RDONLY|os.O_NOFOLLOW);need({k:getattr(os.fstat(inputfd),k)for k in F}=={k:held[str(stdin_path)][k]for k in F},'original archive FD before stdin')
   for key in('stdout','stderr'):streams.append((out/(phase+'.'+key+'.log')).open('xb'));counts[key]=0
   p=subprocess.Popen(args,stdin=subprocess.DEVNULL if inputfd is None else inputfd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True,env={'HOME':str(pathlib.Path.home()),'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','LC_ALL':'C','LANG':'C','TZ':'UTC'});need(p.pid>1 and p.pid!=28722,'owned process PID');sel.register(p.stdout,selectors.EVENT_READ,('stdout',streams[0]));sel.register(p.stderr,selectors.EVENT_READ,('stderr',streams[1]))
   while sel.get_map():
    need(time.monotonic()-start<limit,'owned process timeout')
    for key,_ in sel.select(.1):
     data=os.read(key.fd,65536)
     if not data:sel.unregister(key.fileobj);continue
     kind,dest=key.data;counts[kind]+=len(data);need(counts[kind]<=2097152,'bounded command logs');dest.write(data)
   p.wait(timeout=max(.1,limit-(time.monotonic()-start)))
  except BaseException as exc:err=exc
  finally:
   if p is not None:
    try:
     def alive():
      try:os.killpg(p.pid,0);return True
      except ProcessLookupError:return False
     if alive():os.killpg(p.pid,signal.SIGTERM)
     try:p.wait(timeout=3)
     except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=3)
     if alive():os.killpg(p.pid,signal.SIGKILL)
     end=time.monotonic()+3
     while alive()and time.monotonic()<end:time.sleep(.05)
     joined=not alive();need(joined,'owned group join')
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-join','message':str(exc)})
   try:sel.close()
   except BaseException as exc:
    if err is None:err=exc
    else:secondary.append({'stage':phase+'-selector-close','message':str(exc)})
   for index,f in enumerate(streams):
    try:f.flush()
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-log-'+str(index)+'-flush','message':str(exc)})
    try:os.fsync(f.fileno())
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-log-'+str(index)+'-fsync','message':str(exc)})
    try:f.close()
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-log-'+str(index)+'-close','message':str(exc)})
   if inputfd is not None:
    try:need({k:getattr(os.fstat(inputfd),k)for k in F}=={k:held[str(stdin_path)][k]for k in F},'original archive FD after stdin')
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-stdin-fence','message':str(exc)})
    try:os.close(inputfd)
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-stdin-close','message':str(exc)})
   event={'phase':phase,'pid':None if p is None else p.pid,'exitCode':None if p is None else p.returncode,'groupClosed':joined,'elapsedMs':round((time.monotonic()-start)*1000)};events.append(event)
   try:save(phase+'.exit-public.json',event)
   except BaseException as exc:
    if err is None:err=exc
    else:secondary.append({'stage':phase+'-event','message':str(exc)})
  if err is not None:raise err
  need(joined and(allowed is None or p.returncode in allowed),'command exit: '+phase);return p.returncode,(out/(phase+'.stdout.log')).read_bytes(),(out/(phase+'.stderr.log')).read_bytes()
 def image(phase):
  _,raw,_=command(phase,prefix+['image','inspect',IMAGE]);rows=body(raw);need(type(rows)is list and len(rows)==1 and rows[0]['Id']==IMAGE and rows[0]['Architecture']=='amd64'and rows[0]['Os']=='linux','immutable AMD image');return {'passed':True,'image':IMAGE,'architecture':'amd64'}
 def inspect(phase,target):
  rc,raw,err=command(phase,prefix+['container','inspect',target],allowed=(0,1))
  if rc==1:need(b'no such object'in err.lower()or b'no such container'in err.lower(),'container absence diagnostic');return None
  rows=body(raw);need(type(rows)is list and len(rows)==1,'single container');x=rows[0];need(re.fullmatch('[0-9a-f]{64}',x['Id'])and x['Name']=='/'+name and x['Image']==IMAGE and x['Config']['Labels'].get(label)==name,'owned CID/name/image/label');return x
 handlers={}
 def interrupted(sig,frame):raise KeyboardInterrupt('owned qualification signal '+str(sig))
 try:
  for sig in(signal.SIGINT,signal.SIGTERM,signal.SIGHUP):handlers[sig]=signal.signal(sig,interrupted)
  image('image-before');need(inspect('name-before',name)is None,'new owned name')
  mounts={'/work':(str(src),False),'/case':(str(case),False),'/old':(str(libroot),False),'/retained':(str(pathlib.Path(core['file']).parent),False),'/control':(str(PUBLIC),False),'/worker-config.json':(str(OUT/'worker-config.json'),False),'/out':(str(outputs),True),'/python-runtime':(str(pyroot),False)};need(all(not any(ch in path for ch in(',','\n','\r','\0'))for path,_ in mounts.values()),'safe physical bind paths');args=prefix+['create','--pull=never','--name',name,'--label',label+'='+name,'--network','none','--read-only','--user','1000:1000','--cpus','2','--memory','2g','--pids-limit','128','--security-opt','no-new-privileges','--cap-drop','ALL','--tmpfs','/tmp:rw,nosuid,nodev,size=64m']
  for guest,(path,rw)in mounts.items():args+=['--mount','type=bind,src='+path+',dst='+guest+(''if rw else',readonly')]
  args+=['--env','PYTHONPATH=']
  for key,value in sorted(pyenv.items()):args+=['--env',key+'='+value]
  args+=['--workdir','/out','--env','HOME=/tmp','--env','TMPDIR=/tmp','--env','LC_ALL=C','--env','LANG=C','--entrypoint','/python-runtime/bin/python3',IMAGE,'-S','/control/amd_worker_v3.py','/worker-config.json'];created=True;_,raw,_=command('create-owned',args,30);candidate_cid=raw.decode().strip();need(re.fullmatch('[0-9a-f]{64}',candidate_cid),'full returned CID');cid=candidate_cid;x=inspect('created-owned',cid);hc=x['HostConfig'];actual={m['Destination']:(m['Source'],m['RW'])for m in x['Mounts']if m['Type']=='bind'};need(actual==mounts and len(x['Mounts'])==len(mounts)and hc['NetworkMode']=='none'and hc['ReadonlyRootfs']is True and hc['Memory']==2147483648 and hc['NanoCpus']==2000000000 and hc['PidsLimit']==128 and set(hc.get('CapDrop')or[])=={'ALL'}and not hc.get('CapAdd')and not hc.get('Privileged')and 'no-new-privileges'in(hc.get('SecurityOpt')or[])and x['Config']['User']=='1000:1000'and not hc.get('PortBindings')and not x['NetworkSettings'].get('Ports'),'actual exact isolated mounts/limits');actualenv=dict(v.split('=',1)for v in x['Config']['Env']);need(all(actualenv.get(k)==v for k,v in pyenv.items())and actualenv.get('PYTHONPATH')==''and x['Config']['Entrypoint']==['/python-runtime/bin/python3']and x['Config']['Cmd']==['-S','/control/amd_worker_v3.py','/worker-config.json'],'actual isolated Python provider environment/flags')
  cli,stdout,stderr=command('worker-start-attached',prefix+['start','-a',cid],510,allowed=None);x=inspect('worker-terminal-independent',cid);state=x['State'];save('container-terminal-public.json',{'dockerCliExitCode':cli,'containerId':cid,'state':state});need(cli==0 and state['Running']is False and state['Status']=='exited'and type(state['ExitCode'])is int and state['ExitCode']==0 and state.get('OOMKilled')is False and not state.get('Error')and not state.get('FinishedAt','0001').startswith('0001'),'CLI0 AND independent container StateExit0/noOOM');need(stderr==b'','empty worker stderr');wr=body(hold(outputs/'result-public.json'));authority=body(hold(outputs/'final-observations.json'));need(type(wr)is dict and wr['passed']is True and wr['counts']=={'independentM':1,'cppCompilations':1,'links':1,'loaderChecks':1,'runs':1}and wr['mmd']['normalizedMEqualsMd']is True and wr['record']['result']=='PASS','one complete fresh TU/run with normalized MMD');need(set(authority)=={'rows','aliases','products','historicalImageFD9Claimed'}and authority['historicalImageFD9Claimed']is False,'scoped in-run guest observations')
  for guest,row in authority['rows'].items():
   if guest.startswith('/work/'):
    relative=guest.removeprefix('/work/');need(relative in sourcepins and {'sha256':row['sha256'],'bytes':row['st_size']}==sourcepins[relative],'consumed work source matches original known pin');hold(src/relative,wantpin=sourcepins[relative])
   elif guest.startswith('/case/'):need(guest in project and {'sha256':row['sha256'],'bytes':row['st_size']}==project[guest],'two case pins known')
  need(set(authority['products'])=={'/out/'+n for n in('journal.o.M','journal.o','journal.o.d','rollback')},'exact4 fresh build products')
  for guest,row in authority['products'].items():hold(outputs/pathlib.Path(guest).name,wantpin={'sha256':row['sha256'],'bytes':row['st_size']})
 except BaseException as exc:error(exc,'single-TU-qualification')
 finally:
  for sig in handlers:
   try:signal.signal(sig,signal.SIG_IGN)
   except BaseException as exc:error(exc,'closing-signals')
  if created:
   try:
    x=None
    if cid is not None:
     try:x=inspect('cleanup-by-cid',cid)
     except BaseException as exc:secondary.append({'stage':'cleanup-cid-lookup','message':str(exc)})
    if x is None:x=inspect('cleanup-by-name',name)
    if x is not None:
     cid=x['Id']
     if x['State']['Running']:command('stop-owned',prefix+['stop','--time','10',cid],15)
     x=inspect('stopped-owned',cid);need(x is not None and not x['State']['Running'],'owned stopped');command('remove-owned',prefix+['rm',cid],15)
    need(inspect('cleanup-absence',name)is None,'owned container absent');cleanup={'passed':True,'ownedContainerAbsent':True,'containerId':cid}
   except BaseException as exc:error(exc,'owned-cleanup');cleanup={'passed':False,'error':str(exc)}
  else:cleanup={'passed':True,'neverCreated':True,'ownedContainerAbsent':True}
  try:final_image=image('image-final')
  except BaseException as exc:error(exc,'image-final')
  for path,row in held.items():
   try:need(same(material(path)[0],row),'original/held input full9/SHA changed')
   except BaseException as exc:changes.append({'path':path,'message':str(exc)});error(exc,'postcleanup-input-fence')
  try:need(same(python_inventory(),python_before),'provider closed inventory/directories unchanged after cleanup/image')
  except BaseException as exc:changes.append({'scope':'python-provider-inventory','message':str(exc)});error(exc,'python-provider-final-fence')
  for sig,handler in handlers.items():
   try:signal.signal(sig,handler)
   except BaseException as exc:error(exc,'restore-handlers')
  fence={'passed':not changes,'heldFiles':len(held),'originalSoftwareFiles':9,'testPythonProviderFiles':628,'testPythonProviderDirectories':51,'changed':changes,'afterCleanupAndImageInspection':cleanup is not None and cleanup['passed']and final_image is not None}
  try:save('host-final-fence-public.json',fence)
  except BaseException as exc:error(exc,'fence-publication')
  report={'format':'nano-real-journal-rollback-amd-host-v5','passed':first is None and wr is not None and wr['passed']is True and cleanup['passed']and final_image is not None and fence['passed'],'CorePin':CORE_PIN,'CoreMembers':143,'CoreRebuilt':False,'imagesBuilt':0,'worker':wr,'cleanup':cleanup,'imageFinal':final_image,'inputFinalFence':fence,'firstFailure':first,'secondaryFailures':secondary,'events':events,'originalAuthorityRefreshed':False,'controlCarrierHistoricalFD9Verified':False,'testPythonProviderControlPin':PYTHON_CONTROL_PIN,'pythonBundledInImageClaimed':False,'productionPythonRuntimeQualified':False,'sourceFullHistoricalMetadataQualified':False,'runtimeDutiesAuthorized':False,'networkTransport':False,'commonFinalityQualified':False,'operatorRecoveryQualified':False,'productionQualified':False}
  try:save('result-public.json',report)
  except BaseException as exc:error(exc,'result-publication');report['passed']=False
 return 0 if report['passed']else 1
if __name__=='__main__':
 try:need(len(sys.argv)==4,'usage amd_host_v5.py SOURCE_ADMISSION_JSON SHA256 BYTES');raise SystemExit(main(sys.argv[1],{'sha256':sys.argv[2],'bytes':int(sys.argv[3])}))
 except Exception as exc:print(json.dumps({'passed':False,'prestartFailure':str(exc)}),file=sys.stderr);raise SystemExit(1)
