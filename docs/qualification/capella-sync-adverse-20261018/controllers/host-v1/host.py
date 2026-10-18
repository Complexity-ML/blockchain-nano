"""Root-only retained-Core Capella adverse-sync leaf qualification. Source only."""
import hashlib,json,os,pathlib,re,signal,stat,subprocess,sys,time,types,uuid

F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
WORKER_PIN={'sha256':'e9bfc40c92f8659af4eda8a4ebcd306699531b058b56acb5d2a426e34ebb7301','bytes':37768}
ARM_CORE_PIN={'sha256':'11c1494f4e9fdd9dddd2ee12f8e43e6dffa0a8921f022d39b3d6400477055606','bytes':41225178}
UNIT='capella-sync-adverse';SOURCE='/work/tests/test_capella_sync_adverse.cpp';EXE='nano-capella-sync-adverse-tests'
FIELDS={'format','ready','repoRoot','gitExecutable','actualHead','sourceRoot','outputRoot','docker','worker','retainedCore','libraries','generatedProviders','originalFiles','workerConfig','coreSourcePins'}
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def valid_pin(v,empty=False):return type(v)is dict and set(v)=={'sha256','bytes'}and type(v['sha256'])is str and re.fullmatch('[0-9a-f]{64}',v['sha256'])and type(v['bytes'])is int and (0<=v['bytes']if empty else 0<v['bytes'])and v['bytes']<=536870912 and (v['bytes']!=0 or v['sha256']==hashlib.sha256(b'').hexdigest())
def field_row(v):return type(v)is dict and set(v)==set(F)|{'sha256'}and all(type(v[k])is int and v[k]>=0 for k in F)and type(v['sha256'])is str and re.fullmatch('[0-9a-f]{64}',v['sha256'])
def body_json(raw):
 def unique(pairs):
  names=[k for k,_ in pairs];need(len(names)==len(set(names)),'duplicate JSON keys');return dict(pairs)
 def nonfinite(_):raise ValueError('nonfinite JSON')
 return json.loads(raw,object_pairs_hook=unique,parse_constant=nonfinite)
def material(path,body=False,empty=False,links=1):
 p=pathlib.Path(path);need(p.is_absolute()and p.resolve(strict=True)==p,'physical absolute file');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};need(type(links)is int and 1<=links<=1024 and stat.S_ISREG(a.st_mode)and a.st_nlink==links and (0<=a.st_size if empty else 0<a.st_size)and a.st_size<=536870912,'bounded regular input with original link count');h=hashlib.sha256();chunks=[];n=0
  while b:=os.read(fd,1048576):
   n+=len(b);need(n<=a.st_size,'input grew');h.update(b)
   if body:chunks.append(b)
  need(n==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'strict FD9/path');return dict(row,sha256=h.hexdigest()),b''.join(chunks)if body else None
 finally:os.close(fd)
def archive_members(raw):
 need(raw.startswith(b'!<arch>\n'),'GNU archive magic');at=8;names=[];longnames=b'';records=0
 while at<len(raw):
  records+=1;need(records<=146,'bounded143-object archive records');h=raw[at:at+60];need(len(h)==60 and h[58:]==b'`\n','archive header');size=int(h[48:58].decode().strip());start=at+60;end=start+size;need(0<=size<=len(raw)and end<=len(raw),'archive member span');name=h[:16].decode().strip()
  if name=='//':longnames=raw[start:end]
  elif name not in ('/','/SYM64/'):
   if name.startswith('/'):
    need(name[1:].isdigit(),'GNU long name');i=int(name[1:]);need(i<len(longnames),'long-name index');j=longnames.find(b'/\n',i);need(j>=i,'long-name terminator');name=longnames[i:j].decode()
   else:name=name.removesuffix('/')
   need(re.fullmatch('[A-Za-z0-9_.+-]+[.]o',name)and name not in names,'unique plain object member');names.append(name)
  at=end+(size%2)
 need(at==len(raw)and len(names)==143,'retained Core exactly143 members');return names

def run_host(control,control_pin):
 cp=pathlib.Path(control);initial,raw=material(cp,True);need(valid_pin(control_pin)and control_pin['bytes']<=8388608 and pin(raw)==control_pin,'external control byte pin');c=body_json(raw)
 need(type(c)is dict and set(c)==FIELDS and c['format']=='nano-capella-sync-adverse-retained-control-v1'and c['ready']is True,'exact ready control')
 out=pathlib.Path(c['outputRoot']);need(out.is_absolute()and out.parent.resolve(strict=True)==out.parent and not out.exists()and not out.is_symlink()and re.fullmatch('capella-sync-adverse-(arm|amd)-root-20261018-[0-9]{2}',out.name),'fresh named output');out.mkdir(mode=0o700)
 held={};events=[];secondary=[];first=None;worker_result=None;cleanup=None;image_final=None;docker_ready=False;create_attempted=False;cid=None;name='nano-capella-sync-adverse-'+uuid.uuid4().hex;label='nano.capella-sync-adverse.owner';source_files=0;source_bytes=0;new_products=0;archive_count=0;changes=[];report=None
 deadline=time.monotonic()+1980;cleanup_end=None
 def failure(exc,stage):
  nonlocal first
  item={'stage':stage,'type':type(exc).__name__,'message':str(exc)}
  if first is None:first=item
  else:secondary.append(item)
 def save(name,value):
  p=out/name
  with p.open('x')as f:json.dump(value,f,sort_keys=True,indent=2,allow_nan=False);f.write('\n');f.flush();os.fsync(f.fileno())
 def hold(path,want,body=False,empty=False):
  key=str(path);need(field_row(want),'full Root row required');got,b=material(path,body,empty,want['st_nlink']);need(got==want,'Root/held row changed: '+key);need(key not in held or held[key]==want,'authority refresh forbidden');held.setdefault(key,dict(want));return b
 def new_document(path,expected):
  start=time.monotonic();last=None;streak=0;since=start;sample=0
  baseline=None
  while True:
   row,b=material(path,True);sample+=1;save('new-input-'+str(len(held))+'-'+str(sample)+'.private.json',{'file':str(path),'row':row,'elapsedMs':round((time.monotonic()-start)*1000)})
   now=time.monotonic();need(now-start<30 and pin(b)==expected,'new known input bytes/deadline');fixed={k:v for k,v in row.items()if k!='st_ctime_ns'}
   if baseline is None:baseline=fixed
   need(fixed==baseline,'new input changed before admission')
   if row==last:streak+=1
   else:streak=1;since=now
   if streak>=3 and now-since>=2:held[str(path)]=row;return b
   last=row;time.sleep(1.05)
 def command(phase,args,timeout=30,allowed=(0,),closing=False):
  limit=min(timeout,(cleanup_end if closing else deadline)-time.monotonic());need(limit>0,'host command deadline');save(phase+'.argv.private.json',args);p=None;error=None;closed=False;began=time.monotonic()
  def alive():
   try:os.killpg(p.pid,0);return True
   except ProcessLookupError:return False
  def kill(sig):
   try:os.killpg(p.pid,sig)
   except ProcessLookupError:pass
  try:
   with(out/(phase+'.stdout.log')).open('xb')as stdout,(out/(phase+'.stderr.log')).open('xb')as stderr:
    p=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr,start_new_session=True,env={'HOME':str(pathlib.Path.home()),'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','LANG':'C','LC_ALL':'C','GIT_OPTIONAL_LOCKS':'0'});need(p.pid>1 and p.pid!=28722,'owned safe command PID');p.wait(timeout=limit)
  except BaseException as exc:error=exc
  finally:
   if p is not None and p.pid>1 and p.pid!=28722:
    try:
     if alive():kill(signal.SIGTERM)
     try:p.wait(timeout=3)
     except subprocess.TimeoutExpired:kill(signal.SIGKILL);p.wait(timeout=3)
     if alive():kill(signal.SIGKILL)
     end=time.monotonic()+3
     while alive()and time.monotonic()<end:time.sleep(.05)
     closed=not alive();need(closed,'command process group remains')
    except BaseException as exc:
     if error is None:error=exc
     else:secondary.append({'stage':phase+'-join','type':type(exc).__name__,'message':str(exc)})
   event={'phase':phase,'pid':None if p is None else p.pid,'exitCode':None if p is None else p.returncode,'groupClosed':closed,'elapsedMs':round((time.monotonic()-began)*1000)};events.append(event)
   try:save(phase+'.exit-public.json',event)
   except BaseException as exc:
    if error is None:error=exc
    else:secondary.append({'stage':phase+'-terminal-publication','type':type(exc).__name__,'message':str(exc)})
  if error is not None:raise error
  need(event['exitCode']in allowed and closed,'command terminal refused: '+phase);a=out/(phase+'.stdout.log');b=out/(phase+'.stderr.log');need(a.stat().st_size<=16777216 and b.stat().st_size<=16777216,'bounded command logs');return event['exitCode'],a.read_bytes(),b.read_bytes()
 def inspect(phase,target,closing=False):
  rc,b,e=command(phase,prefix+['container','inspect',target],10,(0,1),closing)
  if rc==1:need(b'no such object'in e.lower()or b'no such container'in e.lower(),'inspect absence only');return None
  x=body_json(b);need(type(x)is list and len(x)==1,'one container');x=x[0];need(re.fullmatch('[0-9a-f]{64}',x['Id'])and x['Name']=='/'+name and x['Image']==d['image']and x['Config']['Labels'].get(label)==name,'owned CID/name/image/label');return x
 def image(phase,closing=False):
  _,b,_=command(phase,prefix+['image','inspect',d['image']],10,closing=closing);v=body_json(b);need(type(v)is list and len(v)==1 and v[0]['Id']==d['image']and v[0]['Architecture']==d['architecture']and v[0]['Os']=='linux','exact retained image');return {'passed':True,'image':d['image'],'architecture':d['architecture']}
 def source_inventory():
  found=set();todo=[src];count=0
  while todo:
   p=todo.pop();need(p.resolve(strict=True)==p and p.is_dir()and not p.is_symlink(),'physical source directory')
   with os.scandir(p)as entries:
    for item in entries:
     count+=1;need(count<=40000 and not item.is_symlink(),'bounded source projection without aliases');q=pathlib.Path(item.path)
     if item.is_dir(follow_symlinks=False):todo.append(q)
     else:need(item.is_file(follow_symlinks=False),'regular source projection');found.add(str(q))
  need(found==source_paths,'exact source projection inventory')
 def interrupted(sig,frame):raise KeyboardInterrupt('owned host signal '+str(sig))
 old_handlers={sig:signal.signal(sig,interrupted)for sig in(signal.SIGINT,signal.SIGTERM,signal.SIGHUP)}
 try:
  new_document(cp,control_pin);need(str(cp)not in c['originalFiles'],'control cannot self-authorize')
  repo=pathlib.Path(c['repoRoot']);src=pathlib.Path(c['sourceRoot']);need(all(p.is_absolute()and p.resolve(strict=True)==p and p.is_dir()for p in(repo,src))and not(src.is_relative_to(repo)or repo.is_relative_to(src))and not(out.is_relative_to(repo)or out.is_relative_to(src)or src.is_relative_to(out)),'isolated physical repo/source/output')
  need(type(c['actualHead'])is str and re.fullmatch('[0-9a-f]{40}',c['actualHead']),'actual source HEAD')
  originals=c['originalFiles'];need(type(originals)is dict and 995<=len(originals)<=40000,'Root selected full9 authorities')
  wc=c['workerConfig'];need(type(wc)is dict and set(wc)=={'coreMode','units','cases','runtimeInputs','sourcePins','projectPins','expectedObjectMembers','timeoutSeconds'}and wc['coreMode']=='retained'and wc['expectedObjectMembers']==143 and type(wc['timeoutSeconds'])is int and 60<=wc['timeoutSeconds']<=1500,'retained-only eight worker fields')
  project=wc['projectPins'];sources=wc['sourcePins'];need(type(project)is dict and 995<=len(project)<=20000 and type(sources)is dict and all(project.get(k)==v for k,v in sources.items())and all(valid_pin(v,True)for v in project.values())and sum(v['bytes']for v in project.values())<=268435456,'bounded joined source pins')
  source_paths=set();empty_paths=set()
  for guest,want in project.items():
   need(type(guest)is str and guest.startswith('/work/')and pathlib.PurePosixPath(guest).as_posix()==guest and '..'not in pathlib.PurePosixPath(guest).parts and not any(x in guest for x in('\0','\n','\r',',')),'canonical projection path');p=src/guest.removeprefix('/work/');source_paths.add(str(p))
   if want['bytes']==0:need(guest.startswith('/work/third_party/')and guest.endswith('/.emptydir'),'known empty marker only');empty_paths.add(str(p))
  core_sources=c['coreSourcePins'];need(type(core_sources)is dict and len(core_sources)==989 and all(re.match('/work/(src|include|third_party)/',k)and project.get(k)==v and valid_pin(v,True)for k,v in core_sources.items()),'989 unchanged retained-Core source pins')
  for p,want in originals.items():hold(p,want,empty=p in empty_paths)
  for guest,want in project.items():
   p=str(src/guest.removeprefix('/work/'));need(p in held and held[p]['st_nlink']==1 and {'sha256':held[p]['sha256'],'bytes':held[p]['st_size']}==want,'single-link projection admitted Root row/pin');source_files+=1;source_bytes+=want['bytes']
  source_inventory();need('/work/CMakeLists.txt'in sources and SOURCE in sources,'CMake/new test pinned');need(type(wc['runtimeInputs'])is list and len(wc['runtimeInputs'])<=128 and len(set(wc['runtimeInputs']))==len(wc['runtimeInputs'])and all(p in sources for p in wc['runtimeInputs']),'only explicit pinned public runtime inputs')
  d=c['docker'];need(type(d)is dict and set(d)=={'prefix','image','architecture','uid','gid','tmpfsMiB'}and re.fullmatch('sha256:[0-9a-f]{64}',d['image'])and d['architecture']in('arm64','amd64')and type(d['uid'])is int and 1<=d['uid']<=65535 and type(d['gid'])is int and 0<=d['gid']<=65535 and type(d['tmpfsMiB'])is int and 128<=d['tmpfsMiB']<=1024,'Docker image/owner/limits');prefix=d['prefix'];need(type(prefix)is list and all(type(x)is str and x and not any(ch in x for ch in('\0','\n','\r'))for x in prefix),'Docker prefix strings')
  if len(prefix)==5:toolpaths=[prefix[0]];tail=prefix[1:]
  else:need(len(prefix)==7 and prefix[1]=='-n','sudo noninteractive only');toolpaths=[prefix[0],prefix[2]];tail=prefix[3:]
  need(tail[0::2]in(['--config','--host'],['--host','--config']),'explicit local Docker host/config');options=dict(zip(tail[0::2],tail[1::2]));need(options['--host'].startswith('unix:///')and pathlib.Path(options['--config']).resolve(strict=True)==pathlib.Path(options['--config'])and pathlib.Path(options['--config']).is_dir(),'local daemon/config')
  git=c['gitExecutable'];need(type(git)is str and git in held and all(p in held and pathlib.Path(p).is_absolute()for p in toolpaths)and str(pathlib.Path(__file__).resolve())in held,'original Git/Docker/host rows');docker_ready=True
  w=c['worker'];need(type(w)is dict and set(w)=={'file','pin'}and w['pin']==WORKER_PIN and w['file']in held,'fixed reviewed worker');wb=hold(w['file'],originals[w['file']],True);need(pin(wb)==WORKER_PIN,'worker unchanged')
  core=c['retainedCore'];libs=c['libraries'];generated=c['generatedProviders'];need(type(core)is dict and set(core)=={'file','guest','pin','members'}and core['guest']=='/baseline/libnano_core.a'and core['members']==143 and type(libs)is list and len(libs)==6 and type(generated)is list and len(generated)<=32,'one retained143 Core/six providers')
  if d['architecture']=='arm64':need(core['pin']==ARM_CORE_PIN,'Root supplied retained ARM Core')
  mounts=[];known={};physical_mounts=set()
  for item in [core,*libs,*generated]:
   need(type(item)is dict and set(item)==({'file','guest','pin','members'}if item is core else{'file','guest','pin'})and valid_pin(item['pin'])and item['file']in held,'bound provider original row');g=item['guest'];p=item['file'];need(type(g)is str and g not in known and p not in physical_mounts and not any(ch in p for ch in(',','\0','\n','\r'))and (g==core['guest']or re.fullmatch('/old/[A-Za-z0-9_.+/-]+',g)and '..'not in pathlib.PurePosixPath(g).parts),'unique provider mount');need(item['pin']=={'sha256':held[p]['sha256'],'bytes':held[p]['st_size']},'provider old byte pin');known[g]=item['pin'];physical_mounts.add(p);mounts+=['--mount','type=bind,src='+p+',dst='+g+',readonly']
  archive_count=len(archive_members(hold(core['file'],originals[core['file']],True)))
  units=wc['units'];need(type(units)is list and len(units)==1,'one leaf only');u=units[0];need(type(u)is dict and set(u)=={'name','scope','source','compilerFlags','linkPrefix','linkTail','executable','object'}and u['name']==UNIT and u['scope']=='candidate'and u['source']==SOURCE and u['executable']==EXE and u['object']=={'kind':'compile'},'fixed new candidate TU, no object reuse')
  for k in('compilerFlags','linkPrefix','linkTail'):need(type(u[k])is list and u[k]and all(type(x)is str and x and '\0'not in x for x in u[k]),'explicit per-leaf recipe '+k)
  need(u['compilerFlags'][0].startswith('/')and u['linkPrefix'][0].startswith('/')and not any(x in('-o','-c','-M','-MM','-MD','-MMD','-MF','-MT')for x in u['compilerFlags'])and u['linkTail'].count(core['guest'])==1 and all(x['guest']in u['linkTail']for x in libs)and all(not x.startswith('/old/')or x in known for x in u['linkTail']),'retained link/providers; no compile suffix')
  cases=wc['cases'];need(type(cases)is list and len(cases)==1,'one full new self-test');case=cases[0];need(type(case)is dict and set(case)=={'name','unit','args','timeoutSeconds','expectedExit','stdout','stderr'}and case['name']=='capella-sync-adverse-self-test'and case['unit']==UNIT and case['args']==['--self-test']and case['expectedExit']==0 and type(case['timeoutSeconds'])is int and 1<=case['timeoutSeconds']<=300 and case['stderr']=={'kind':'empty'}and case['stdout'].get('kind')in('json-lines','json-field-records'),'fixed self-test/typed stream oracle')
  policy=case['stdout'];need(set(policy)=={'kind','records'}and type(policy['records'])is list and len(policy['records'])==3 and all(type(r)is dict for r in policy['records']),'two group records and summary oracle')
  for record,scenario in zip(policy['records'],('capella-sync-orphan-retry','capella-sync-deferred-session-reopen','capella-sync-adverse-summary')):
   if policy['kind']=='json-field-records':
    need(set(record)=={'expectedFields','integerFields'}and type(record['expectedFields'])is dict and type(record['integerFields'])is dict and not set(record['expectedFields'])&set(record['integerFields']),'typed field-record policy');fixed=record['expectedFields']
    for limits in record['integerFields'].values():need(type(limits)is dict and set(limits)=={'minimum','maximum'}and type(limits['minimum'])is int and type(limits['maximum'])is int and 0<=limits['minimum']<=limits['maximum']<=10000000,'bounded typed integer oracle')
   else:fixed=record
   need(fixed.get('scenario')==scenario and fixed.get('result')=='PASS','fixed self-test scenario/result oracle')
  summary=policy['records'][-1]if policy['kind']=='json-lines'else policy['records'][-1]['expectedFields'];need(summary.get('groups')==2 and summary.get('signedBlocks')==3 and summary.get('newTestOnlySignaturesCreated')==8 and summary.get('privateInputsRead')is False and summary.get('runtimeDutiesAuthorized')is False and summary.get('networkTransport')is False and summary.get('productionQualified')is False,'synthetic-only summary oracle');need(all(not any(ch in str(p)for ch in(',','\n','\r'))for p in(src,out)),'bind-safe physical roots')
  outputs=out/'outputs';outputs.mkdir(mode=0o700);(outputs/'home').mkdir();(outputs/'tmp').mkdir();cidfile=out/'container.cid';deadline=time.monotonic()+wc['timeoutSeconds']+180
  _,b,_=command('git-head-before',[git,'-c','core.fsmonitor=false','--no-optional-locks','-C',str(repo),'rev-parse','HEAD']);need(b.decode().strip()==c['actualHead'],'actual main revision');_,b,_=command('git-clean-before',[git,'-c','core.fsmonitor=false','--no-optional-locks','-C',str(repo),'status','--porcelain']);need(b==b'','clean repository required');image('image-before');need(inspect('name-before',name)is None,'unique absent container')
  args=prefix+['create','--pull=never','--name',name,'--cidfile',str(cidfile),'--label',label+'='+name,'--network','none','--read-only','--user',str(d['uid'])+':'+str(d['gid']),'--cpus','2','--memory','3g','--pids-limit','128','--security-opt','no-new-privileges','--cap-drop','ALL','--tmpfs','/tmp:rw,nosuid,nodev,size='+str(d['tmpfsMiB'])+'m','--mount','type=bind,src='+str(src)+',dst=/work,readonly','--mount','type=bind,src='+str(outputs)+',dst=/outputs',*mounts,'--workdir','/outputs','--entrypoint','/bin/sh',d['image'],'-ec','exec sleep '+str(wc['timeoutSeconds']+120)];create_attempted=True;_,b,_=command('create-own',args,30);cid=b.decode().strip();need(re.fullmatch('[0-9a-f]{64}',cid),'created full CID');x=inspect('created-own',cid);need(x is not None,'created owned container')
  hc=x['HostConfig'];need(hc['NetworkMode']=='none'and hc['ReadonlyRootfs']is True and hc['PidsLimit']==128 and hc['Memory']==3*1024**3 and hc['NanoCpus']==2000000000 and x['Config']['User']==str(d['uid'])+':'+str(d['gid'])and not x['NetworkSettings'].get('Ports')and not hc.get('PortBindings'),'actual isolated container limits')
  wantmounts={'/work':(str(src),False),'/outputs':(str(outputs),True),**{item['guest']:(item['file'],False)for item in[core,*libs,*generated]}};binds={m['Destination']:(m['Source'],m['RW'])for m in x['Mounts']if m['Type']=='bind'};need(binds==wantmounts and set(hc.get('CapDrop')or[])=={'ALL'}and 'no-new-privileges'in(hc.get('SecurityOpt')or[]),'actual RO inputs/RW output and capabilities');command('start-own',prefix+['start',cid],30);need(inspect('running-own',cid)['State']['Running']is True,'owned container running')
  module=types.ModuleType('retained_capella_sync_worker');module.__file__=w['file'];exec(compile(wb,w['file'],'exec'),module.__dict__);need(callable(getattr(module,'run_qualification',None)),'reviewed worker API');config=dict(wc,hostOutput=str(outputs),sourceGuest='/work',baselineSourceGuest='/baseline-work',outputGuest='/outputs',dockerPrefix=prefix+['exec',cid],baselineCoreGuest=core['guest'],baselineCorePin=core['pin'],candidateCoreGuest=core['guest'],candidateCorePin=core['pin'],knownGuestPins=known,guestUid=d['uid'],guestGid=d['gid']);save('worker-config.private.json',config);worker_result=module.run_qualification(config)
  expected={'independentM':1,'coreIndependentM':0,'testIndependentM':1,'compiledTranslationUnits':1,'coreCompiledTranslationUnits':0,'testCompiledTranslationUnits':1,'archiveUpdates':0,'links':1,'loaderChecks':1,'testInvocations':1,'runtimeCliInvocations':0,'unitInvocations':1,'runtimeIndependentM':0,'runtimeCompiledTranslationUnits':0,'reusedSubmitObjects':0}
  need(worker_result['passed']is True and worker_result['coreMode']=='retained'and worker_result['coreProof']is None and worker_result['allConfiguredCasesPassed']is True and worker_result['counts']==expected and all(type(v)is int for v in worker_result['counts'].values())and set(worker_result['mmd'])=={UNIT}and worker_result['mmd'][UNIT]['normalizedMEqualsMd']is True,'exact leaf-only terminal result')
  authority=outputs/'final-observations.private.json';a,ab=material(authority,True);v=body_json(new_document(authority,pin(ab)));need(set(v)=={'guestStatSha','guestAliases','products','guestFdIdentityVerified'}and v['guestFdIdentityVerified']is False,'worker product authority');products=v['products'];wanted={str(outputs/p)for p in('unit-'+UNIT+'.o','unit-'+UNIT+'.o.M','unit-'+UNIT+'.o.d',EXE)};need(set(products)==wanted,'exact four new M/object/MD/executable products')
  for p,r in products.items():
   need(set(r)=={'hostFD9','guestStatSha'}and field_row(r['hostFD9'])and r['guestStatSha']['sha256']==r['hostFD9']['sha256']and r['guestStatSha']['bytes']==r['hostFD9']['st_size'],'original worker admitted product row');hold(p,r['hostFD9']);new_products+=1
  rr,rb=material(outputs/'result-public.json',True);need(body_json(rb)==worker_result,'worker persisted actual result');new_document(outputs/'result-public.json',pin(rb));save('worker-return-public.json',worker_result)
  _,b,_=command('git-head-after',[git,'-c','core.fsmonitor=false','--no-optional-locks','-C',str(repo),'rev-parse','HEAD']);need(b.decode().strip()==c['actualHead'],'actual HEAD unchanged');_,b,_=command('git-clean-after',[git,'-c','core.fsmonitor=false','--no-optional-locks','-C',str(repo),'status','--porcelain']);need(b==b'','repository unchanged')
 except BaseException as exc:failure(exc,'qualification')
 finally:
  cleanup_end=time.monotonic()+60
  for sig in old_handlers:
   try:signal.signal(sig,signal.SIG_IGN)
   except BaseException as exc:failure(exc,'closing-signals')
  if docker_ready:
   if create_attempted:
    try:
     # A failed/partial cidfile must never prevent an ownership-checked name fallback.
     try:
      if cidfile.exists():
       cr,cb=material(cidfile,True);candidate=cb.decode().strip();need(re.fullmatch('[0-9a-f]{64}',candidate),'cidfile full CID');need(cid is None or cid==candidate,'cidfile matches returned CID');cid=candidate
     except BaseException as exc:secondary.append({'stage':'cidfile-diagnostic','type':type(exc).__name__,'message':str(exc)})
     x=None
     if cid is not None:
      try:x=inspect('cleanup-by-cid',cid,True)
      except BaseException as exc:secondary.append({'stage':'cid-ownership-diagnostic','type':type(exc).__name__,'message':str(exc)})
     if x is None:x=inspect('cleanup-by-name',name,True)
     if x is not None:
      cid=x['Id']
      if x['State']['Running']:command('stop-own',prefix+['stop','--time','10',cid],15,closing=True)
      stopped=inspect('cleanup-stopped',cid,True);need(stopped is not None and not stopped['State']['Running'],'owned container stopped');command('remove-own',prefix+['rm',cid],10,closing=True)
     need(inspect('cleanup-absence',name,True)is None,'owned name absent after cleanup');cleanup={'passed':True,'ownedContainerAbsent':True,'containerId':cid}
    except BaseException as exc:failure(exc,'owned-container-cleanup');cleanup={'passed':False,'error':str(exc)}
   else:cleanup={'passed':True,'neverCreated':True,'ownedContainerAbsent':True}
   try:image_final=image('image-final',True)
   except BaseException as exc:failure(exc,'image-final')
  for p,want in held.items():
   try:
    row,_=material(p,empty=want['st_size']==0,links=want['st_nlink']);need(row==want,'retained full9/SHA mismatch')
   except BaseException as exc:changes.append({'file':p,'error':str(exc)})
  if source_files:
   try:source_inventory()
   except BaseException as exc:changes.append({'file':'source-projection-inventory','error':str(exc)})
  fence_published=False
  try:save('host-final-fence-public.json',{'passed':not changes,'files':len(held),'changes':changes,'oldRowsRefreshed':False,'afterCleanupAndImageFinal':True});fence_published=True
  except BaseException as exc:failure(exc,'host-fence-publication')
  for sig,handler in old_handlers.items():
   try:signal.signal(sig,handler)
   except BaseException as exc:failure(exc,'restore-signals')
  report={'format':'nano-capella-sync-adverse-retained-host-result-v1','passed':first is None and not secondary and not changes and fence_published and worker_result is not None and worker_result.get('passed')is True and cleanup is not None and cleanup.get('passed')is True and image_final is not None and image_final.get('passed')is True and new_products==4,'actualHead':c['actualHead'],'coreMode':'retained','retainedCoreMembers':archive_count,'sourceFiles':source_files,'sourceBytes':source_bytes,'hostInputs':len(held),'newWorkerProductRowsHeldThroughCleanup':new_products,'worker':worker_result,'firstFailure':first,'secondaryFailures':secondary,'cleanup':cleanup,'imageFinalFence':image_final,'hostFinalFencePublished':fence_published,'hostFinalFenceAfterCleanup':True,'steps':events,'coreScansRepeated':0,'coreCompilationsRepeated':0,'archiveUpdatesRepeated':0,'newCppCompilations':None if worker_result is None else worker_result['counts']['compiledTranslationUnits'],'newImagesBuilt':0,'cmakeConfigures':0,'oldCppTestsReplayed':0,'networkTransport':False,'newCampaignStarted':False,'runtimeDutiesResumed':False,'privateCampaignInputsRead':False,'sdkGlobalQualified':False,'productionQualified':False}
  try:save('result-public.json',report)
  except BaseException as exc:failure(exc,'result-publication');report['passed']=False;report['firstFailure']=first;report['secondaryFailures']=secondary
 need(report['passed'],'qualification/cleanup/fence refused');return report

def main():
 need(len(sys.argv)==4 and re.fullmatch('[0-9a-f]{64}',sys.argv[2])and re.fullmatch('[1-9][0-9]*',sys.argv[3]),'Usage: host.py CONTROL SHA256 BYTES');result=run_host(sys.argv[1],{'sha256':sys.argv[2],'bytes':int(sys.argv[3])});print(json.dumps(result,sort_keys=True))
if __name__=='__main__':main()
