"""SOURCE_ONLY: runtime-only continuation of a failed C8 controller.
Original 36 products/case rows remain strict; no compilation/link/ldd/archive command.
"""
import datetime,hashlib,json,os,pathlib,posixpath,re,selectors,shlex,signal,stat,subprocess,sys,time,types,uuid
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
CASE_PINS={'src/chain.cpp': {'sha256': 'bc0a192263a4192c7363f4a65f126586504cbb74aff1af6084e69face8b78b46', 'bytes': 44702}, 'src/main.cpp': {'sha256': 'b4433387abb92445ca4b50540c83a7f515f2054281919c46ff2750c2bbbf6e70', 'bytes': 8164}, 'include/nano/chain.hpp': {'sha256': 'd19297f3c55bb9f98ed8ab5cfdbd976951e100b41483e2968e90d4d44382628e', 'bytes': 8250}, 'tests/test_cli_readonly.cpp': {'sha256': '6431cdca3cb643c4f25de1ac4a1508432af05302d98e0365b208c685f01cb35c', 'bytes': 11239}, 'tests/qualification/cli_readonly_probe.cpp': {'sha256': '84a17dadd9aa80b0bbccc44cfdaff218e01938708651af4ff57c589953448f2e', 'bytes': 19202}, 'tools/storage-maintenance/bootstrap-readonly-replica.hpp': {'sha256': 'd3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125', 'bytes': 20319}}
OLD_HEADER_PIN={'sha256': 'f0bdadfbc8aa1b0ca40086df0f03fa498fef3b05e12541d9a82213721bc2b9eb', 'bytes': 8098}
NONCOMMENT_PIN={'sha256': '980503be502b96537e15c819a8ca7c28a2d3886ee67cef4d22830eca7cc9de64', 'bytes': 7121}
CORE_PIN={'sha256':'65c91631ffe93515e500fd1eeacb4e6fd784fa04d431a193c40ec14733077938','bytes':41306456}
OLD_PIN={'sha256':'3a1697c45c0fde5257963392715268013767a8ff53cba9ce68b3f6e5d0f97a3a','bytes':186873}
SOURCE_PIN={'sha256':'0b4b3c84dc4ed99d297278e6ceda22422fa21d3941e08fed88332edf51219f8e','bytes':623344}
INVENTORY_PIN={'sha256':'19d3c604fb202602bc9d5a33e92aa89bb059034f229b419fd505973a88e38bae','bytes':18623}
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
VOLUME={'name':'nano-physical-public-64712f46d71f4587809d58a1bf7c26a3','driver':'local','ownerLabel':'nano.physical-native-volume.owner','ownerValue':'64712f46d71f4587809d58a1bf7c26a3'}
PARENT_FIELDS={'format','ready','actualHead','oldControl','sourceAuthority','caseRoot','outputRoot','hostPin','worker','volume','fixtureInventory','timeoutSeconds','coreRecipeAuthority','archiveHelper','coreUnit','units','cases','archiveTool','reusePolicy'}
WORKER_PIN={'sha256':'65064e0c8b89747bb44cff7f416eb32b5ab36efa6d3a8f3c97b558e0793d9ce1','bytes':20262}
FIELDS={'format','ready','actualHead','parentControl','retainedBuild','outputRoot','hostPin','worker','timeoutSeconds'}
BUILD_FIELDS={'authority','workerResult','caseAdmission','archiveProof','priorFailure'}
REUSE_POLICY={'kind': 'comment-only-header-singleton-core-update', 'memberCount': 143, 'replacementMember': 'chain.cpp.o', 'unchangedObjectMembers': 142, 'headerDocumentationConsumed': False, 'oldHeaderCompilationRequired': True, 'strict143MSelectorQualified': False, 'cleanBuildQualified': False, 'reproducibleBuildQualified': False}

REQUIRED_ORACLES={'readonly': {'kind': 'json-records', 'records': [{'fixed': {'scenario': 'cli-readonly-core', 'result': 'PASS', 'groups': 6, 'expectedRefusals': 11, 'actualWriteRefusals': 2, 'positiveReadonlyOpens': 5, 'genuinePowBlocksCreated': 1, 'sourceFull9ShaUnchanged': True, 'inMemoryStatePreservedAfterWriteRefusal': True, 'auditScratchSeparateAndRemoved': True, 'beaconReadinessBypassed': False, 'realCliInvocations': 0, 'newSignaturesCreated': 0, 'runtimeDutiesAuthorized': False, 'productionQualified': False}, 'integers': {'elapsedMs': [0, 120000], 'peakRssKiB': [0, 131072]}}]}, 'cli-probe': {'kind': 'json-records', 'records': [{'fixed': {'scenario': 'real-cli-readonly', 'result': 'PASS', 'actualCliCommands': 18, 'successfulCliExits': 8, 'expectedCliRefusals': 10, 'readonlySourceChecks': 15, 'sourceFull9ShaUnchanged': True, 'candidatePinHeldInRun': True, 'candidateHistoricalFd9Qualified': False, 'auditScratchSeparateAndRemoved': True, 'genuinePowBlocksCreated': 1, 'metadataFixtureMutations': 2, 'beaconFixtureWriterOpens': 1, 'newSignaturesCreated': 0, 'oldDataOpened': False, 'runtimeDutiesAuthorized': False, 'productionQualified': False}, 'integers': {'elapsedMs': [0, 179999], 'peakRssKiB': [0, 131072]}}]}}
SCAN='/usr/bin/find -P /state/public-cohort -xdev -exec /bin/sh -ec \'\nfmt="%d|%i|%f|%h|%u|%g|%s|%y|%z"\nfor p do\n [ ! -L "$p" ]; exec 3<"$p"\n a=$(/usr/bin/stat -L --printf="$fmt" /proc/self/fd/3)\n test "$a" = "$(/usr/bin/stat --printf="$fmt" -- "$p")"\n printf "%s\\0" "$p"\n /usr/bin/stat -L --printf="%d\\0%i\\0%f\\0%h\\0%u\\0%g\\0%s\\0%y\\0%z\\0" /proc/self/fd/3\n if [ -f /proc/self/fd/3 ]; then h=$(/usr/bin/sha256sum <&3); printf "%s\\0" "${h%% *}"; else test -d /proc/self/fd/3; printf "%s\\0" "-"; fi\n test "$a" = "$(/usr/bin/stat -L --printf="$fmt" /proc/self/fd/3)"\n test "$a" = "$(/usr/bin/stat --printf="$fmt" -- "$p")"\n exec 3<&-\ndone\' sh {} +'
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
def run_host(control,expected):
 need(valid_pin(WORKER_PIN),'reviewed runtime worker pin pending; no operations permitted')
 cp=pathlib.Path(control);_,raw=material(cp,True);need(valid_pin(expected)and expected['bytes']<=16777216 and pin(raw)==expected,'externally pinned Root runtime control');runtime=body_json(raw)
 need(type(runtime)is dict and set(runtime)==FIELDS and runtime['format']=='nano-cli-readonly-arm-runtime-control-v1'and runtime['ready']is True,'exact nine-field ready runtime control')
 parent=runtime['parentControl'];need(type(parent)is dict and set(parent)=={'file','pin','originalRow'}and valid_pin(parent['pin'])and field_row(parent['originalRow']),'original parent control row required')
 original_parent,raw_parent=material(parent['file'],True,links=parent['originalRow']['st_nlink']);need(original_parent==parent['originalRow']and pin(raw_parent)==parent['pin'],'parent control original FD9/SHA, no replacement authority');c=body_json(raw_parent)
 need(type(c)is dict and set(c)==PARENT_FIELDS and c['format']=='nano-cli-readonly-arm-control-v1'and c['ready']is True,'exact previously admitted C8 parent configuration')
 need(c['volume']==VOLUME and type(runtime['actualHead'])is str and re.fullmatch('[0-9a-f]{40}',runtime['actualHead'])and runtime['actualHead']==c['actualHead']and type(runtime['timeoutSeconds'])is int and 660<=runtime['timeoutSeconds']<=900,'same explicit HEAD/retained volume/bounded runtime budget')
 need(type(runtime['retainedBuild'])is dict and set(runtime['retainedBuild'])==BUILD_FIELDS,'closed retained-build provenance descriptors')
 out=pathlib.Path(runtime['outputRoot']);need(out.is_absolute()and out.parent.resolve(strict=True)==out.parent and not os.path.lexists(out)and re.fullmatch('cli-readonly-arm-runtime-root-20261018-[0-9]{2}',out.name),'fresh runtime output');out.mkdir(mode=0o700)
 held={};source_original={};source_pins={};case_files=set();case_dirs={};project={};events=[];secondary=[];changes=[];first=None;wr=None;authority=None;prefix=None;docker_ready=False;image_final=None;volume_final=None;guest_before=None;guest_final=None;source_metadata_differences=[];global_source_checks=0;retained_products_held=set();retained_mmd_complete=False;signal_handlers={};archive_proof=None;semantic_header_proof=None;expected_products=set();configured_units=[];configured_cases=[]
 deadline=time.monotonic()+runtime['timeoutSeconds'];cleanup_end=None;nonce=uuid.uuid4().hex;label='nano.cli-readonly.runtime.owner';runroot='/state/cli-readonly-runtime-'+nonce
 states={role:{'name':'nano-cli-readonly-runtime-'+role+'-'+nonce,'cid':None,'attempted':False,'cleanup':None}for role in('main','audit')}
 def fail(exc,stage):
  nonlocal first
  row={'stage':stage,'type':type(exc).__name__,'message':str(exc)[:2000]}
  if first is None:first=row
  else:secondary.append(row)
 def save(leaf,value):
  with(out/leaf).open('x')as f:json.dump(value,f,sort_keys=True,indent=2,allow_nan=False);f.write('\n');f.flush();os.fsync(f.fileno())
  fd=os.open(out,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
  try:os.fsync(fd)
  finally:os.close(fd)
 def bp(row):return {'sha256':row['sha256'],'bytes':row['st_size']}
 def hold(p,want,empty=False):
  p=str(p);need(field_row(want),'genuine original full9+SHA required');got,b=material(p,True,empty,want['st_nlink']);need(got==want and(p not in held or held[p]==want),'original authority changed/refresh forbidden: '+p);held.setdefault(p,dict(want));return b
 def inventory(root,maximum):
  files=set();dirs={};todo=[root];n=0
  while todo:
   p=todo.pop();a=p.lstat();need(p.resolve(strict=True)==p and stat.S_ISDIR(a.st_mode),'physical inventory directory');dirs[str(p)]={k:getattr(a,k)for k in F}
   with os.scandir(p)as entries:
    for e in entries:
     n+=1;need(n<=maximum and not e.is_symlink(),'bounded closed inventory');q=pathlib.Path(e.path)
     if e.is_dir(follow_symlinks=False):todo.append(q)
     else:need(e.is_file(follow_symlinks=False),'regular inventory file');files.add(str(q))
  return files,dirs
 def admit(known,phase,root=None):
  need(known and all(p not in held and p not in source_original and p not in originals and valid_pin(v,True)for p,v in known.items()),'only new unheld known paths admitted')
  start=time.monotonic();fixed=None;last=None;streak=0;since=0;attempt=0
  while True:
   rows={p:material(p,empty=v['bytes']==0)[0]for p,v in known.items()};dirs={}
   if root is not None:files,dirs=inventory(root,100);need(files==set(known),'exact new case inventory')
   attempt+=1;save(phase+'-sample-'+str(attempt)+'.private.json',{'rows':rows,'directories':dirs,'elapsedMs':round((time.monotonic()-start)*1000)})
   identities=({p:{k:v for k,v in r.items()if k!='st_ctime_ns'}for p,r in rows.items()},{p:{k:v for k,v in r.items()if k!='st_ctime_ns'}for p,r in dirs.items()})
   if fixed is None:fixed=identities
   need(fixed==identities and all(bp(rows[p])==v and rows[p]['st_nlink']==1 for p,v in known.items()),'new bytes/eight identities changed');now=time.monotonic();need(now<min(deadline,start+30),'new admission30s including durable sample')
   if(rows,dirs)==last:streak+=1
   else:streak=1;since=now
   if streak>=3 and now-since>=2:break
   last=(rows,dirs);time.sleep(1.05)
  for p,r in rows.items():hold(p,r,known[p]['bytes']==0)
  return dirs
 def document(desc,want,phase):
  need(type(desc)is dict and set(desc)=={'file','pin'}and desc['pin']==want,'fixed pinned public/authority descriptor');admit({desc['file']:want},phase);return body_json(hold(desc['file'],held[desc['file']]))
 def command(phase,args,timeout=30,closing=False,allowed=(0,)):
  end=(cleanup_end if cleanup_end is not None else deadline+60)if closing else deadline;limit=min(timeout,end-time.monotonic());need(limit>0,'host deadline');save(phase+'.argv-public.json',args);p=None;err=None;joined=False;sel=selectors.DefaultSelector();streams=[];totals={};began=time.monotonic()
  try:
   for stream in('stdout','stderr'):streams.append((out/(phase+'.'+stream+'.log')).open('xb'));totals[stream]=0
   p=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True,env={'HOME':str(pathlib.Path.home()),'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','LC_ALL':'C','LANG':'C','GIT_OPTIONAL_LOCKS':'0'});need(p.pid>1 and p.pid!=28722,'safe owned PID');sel.register(p.stdout,selectors.EVENT_READ,('stdout',streams[0]));sel.register(p.stderr,selectors.EVENT_READ,('stderr',streams[1]))
   while sel.get_map():
    need(time.monotonic()-began<limit,'owned command timeout')
    for key,_ in sel.select(.1):
     data=os.read(key.fd,65536)
     if not data:sel.unregister(key.fileobj);continue
     stream,f=key.data;totals[stream]+=len(data);need(totals[stream]<=16777216,'bounded host log');f.write(data)
   p.wait(timeout=max(.1,limit-(time.monotonic()-began)))
  except BaseException as exc:err=exc
  finally:
   if p is not None and p.pid>1 and p.pid!=28722:
    try:
     try:os.killpg(p.pid,signal.SIGTERM)
     except ProcessLookupError:pass
     try:p.wait(timeout=3)
     except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=3)
     end=time.monotonic()+3
     while time.monotonic()<end:
      try:os.killpg(p.pid,0)
      except ProcessLookupError:joined=True;break
      time.sleep(.05)
     need(joined,'owned process group remains')
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
   event={'phase':phase,'pid':None if p is None else p.pid,'exitCode':None if p is None else p.returncode,'groupClosed':joined,'elapsedMs':round((time.monotonic()-began)*1000)};events.append(event)
   try:save(phase+'.exit-public.json',event)
   except BaseException as exc:
    if err is None:err=exc
    else:secondary.append({'stage':phase+'-event','message':str(exc)})
  if err is not None:raise err
  need(joined and(allowed is None or event['exitCode']in allowed),'command refused: '+phase);return event['exitCode'],(out/(phase+'.stdout.log')).read_bytes(),(out/(phase+'.stderr.log')).read_bytes()
 def image(phase,closing=False):
  _,b,_=command(phase,prefix+['image','inspect',IMAGE],10,closing);x=body_json(b);need(type(x)is list and len(x)==1 and x[0]['Id']==IMAGE and x[0]['Architecture']=='arm64'and x[0]['Os']=='linux','unchanged retained ARM Linux image');return {'passed':True,'image':IMAGE,'architecture':'arm64'}
 def volume(phase,closing=False):
  _,b,_=command(phase,prefix+['volume','inspect',VOLUME['name']],10,closing);x=body_json(b);need(type(x)is list and len(x)==1,'one existing volume');v=x[0];need(v['Name']==VOLUME['name']and v['Driver']=='local'and v.get('Scope')=='local'and v.get('Labels',{}).get(VOLUME['ownerLabel'])==VOLUME['ownerValue']and not v.get('Options'),'existing owned local volume');return {'passed':True,**VOLUME,'retained':True}
 def inspect(role,phase,target,closing=False):
  rc,b,e=command(phase,prefix+['container','inspect',target],10,closing,(0,1))
  if rc==1:need(b'no such object'in e.lower()or b'no such container'in e.lower(),'known absence diagnostic');return None
  x=body_json(b);need(type(x)is list and len(x)==1,'one owned container');x=x[0];need(re.fullmatch('[0-9a-f]{64}',x['Id'])and x['Name']=='/'+states[role]['name']and x['Image']==IMAGE and x['Config']['Labels'].get(label)==nonce and x['Config']['Labels'].get(label+'.role')==role,'owned CID/name/image/labels');return x
 def users(phase,expected,closing=False):
  _,b,e=command(phase,prefix+['ps','--all','--quiet','--no-trunc','--filter','volume='+VOLUME['name']],10,closing);ids=b.decode().splitlines();need(e==b''and all(re.fullmatch('[0-9a-f]{64}',x)for x in ids)and set(ids)==expected,'no unknown concurrent volume users')
 def remove(role):
  state=states[role]
  if not state['attempted']:state['cleanup']={'passed':True,'neverCreated':True};return
  x=None
  if state['cid']is not None:
   try:x=inspect(role,role+'-cleanup-cid',state['cid'],True)
   except BaseException as exc:secondary.append({'stage':role+'-cid-diagnostic','message':str(exc)})
  if x is None:x=inspect(role,role+'-cleanup-name',state['name'],True)
  if x is not None:
   state['cid']=x['Id']
   if x['State']['Running']:command(role+'-stop',prefix+['stop','--time','10',state['cid']],15,True)
   x=inspect(role,role+'-stopped',state['cid'],True);need(x is not None and not x['State']['Running'],'owned stop complete');command(role+'-remove',prefix+['rm',state['cid']],10,True)
  need(inspect(role,role+'-absent',state['name'],True)is None,'owned container absent');state['cleanup']={'passed':True,'ownedContainerAbsent':True}
 def create(role,readonly,closing=False):
  state=states[role];volume(role+'-volume',closing);need(inspect(role,role+'-name-before',state['name'],closing)is None,'unused name');wanted={}if role=='audit'else{'/work':(str(src),False),'/case':(str(case),False),'/outputs':(str(buildroot),False),**{v['guest']:(v['file'],False)for v in[core,*libs]}}
  mounts=['--mount','type=volume,src='+VOLUME['name']+',dst=/state,volume-nocopy'+(',readonly'if readonly else'')]
  for dest,(origin,rw)in wanted.items():mounts+=['--mount','type=bind,src='+origin+',dst='+dest+(''if rw else',readonly')]
  argv=prefix+['create','--pull=never','--name',state['name'],'--label',label+'='+nonce,'--label',label+'.role='+role,'--network','none','--read-only','--user','501:20','--cpus','2','--memory','2g','--pids-limit','128','--security-opt','no-new-privileges','--cap-drop','ALL','--tmpfs','/tmp:rw,nosuid,nodev,size=64m',*mounts,'--workdir','/state','--entrypoint','/bin/sh',IMAGE,'-ec','exec sleep '+str(runtime['timeoutSeconds']+120 if role=='main'else 120)];state['attempted']=True;_,b,_=command(role+'-create',argv,30,closing);candidate=b.decode().strip();need(re.fullmatch('[0-9a-f]{64}',candidate),'created full CID');state['cid']=candidate;x=inspect(role,role+'-created',candidate,closing);hc=x['HostConfig'];actual={m['Destination']:(m['Source'],m['RW'])for m in x['Mounts']if m['Type']=='bind'};vols=[m for m in x['Mounts']if m['Type']=='volume']
  need(set(actual)==set(wanted)and all(actual[g][1]is rw and actual[g][0]in(origin,'/host_mnt'+origin)for g,(origin,rw)in wanted.items())and len(vols)==1 and len(x['Mounts'])==len(wanted)+1 and vols[0]['Name']==VOLUME['name']and vols[0]['Destination']=='/state'and vols[0]['Driver']=='local'and vols[0]['RW']is(not readonly),'exact Docker Desktop binds and volume mode')
  need(hc['NetworkMode']=='none'and hc['ReadonlyRootfs']is True and hc['Memory']==2*1024**3 and hc['NanoCpus']==2000000000 and hc['PidsLimit']==128 and set(hc.get('CapDrop')or[])=={'ALL'}and not hc.get('CapAdd')and 'no-new-privileges'in(hc.get('SecurityOpt')or[])and x['Config']['User']=='501:20'and not hc.get('PortBindings')and not x['NetworkSettings'].get('Ports'),'actual isolation');command(role+'-start',prefix+['start',candidate],20,closing);need(inspect(role,role+'-running',candidate,closing)['State']['Running']is True,'keepalive running');users(role+'-users',{candidate},closing);return candidate
 def ns(value):
  m=re.fullmatch(r'(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d)\.(\d{9}) \+0000',value);need(m is not None,'GNU full nanosecond UTC');delta=datetime.datetime.strptime(m[1],'%Y-%m-%d %H:%M:%S')-datetime.datetime(1970,1,1);return (delta.days*86400+delta.seconds)*1000000000+int(m[2])
 def survey(role,phase,closing=False):
  cid=states[role]['cid'];_,b,e=command(phase,prefix+['exec','--env','LC_ALL=C','--env','LANG=C','--env','TZ=UTC',cid,'/bin/sh','-ec',SCAN],90,closing);need(e==b''and len(b)<=1048576,'bounded passive scan empty stderr');parts=b.decode('ascii').split('\0');need(parts.pop()==''and len(parts)==69*11,'exact69 FD/path records');rows={}
  for at in range(0,len(parts),11):
   v=parts[at:at+11];p=v[0];need(p.startswith('/state/public-cohort')and p[len('/state/'):]in fixture['entries'],'known closed fixture path');rel=p[len('/state/'):];want=fixture['entries'][rel];r=dict(zip(F,[int(v[1]),int(v[2]),int(v[3],16),int(v[4]),int(v[5]),int(v[6]),int(v[7]),ns(v[8]),ns(v[9])]))
   need(rel not in rows and r['st_uid']==501 and r['st_gid']==20 and stat.S_IMODE(r['st_mode'])==want['mode']and r['st_uid']==want['uid']and r['st_gid']==want['gid']and r['st_mtime_ns']//1000000000==want['mtime'],'original portable metadata')
   if want['type']=='directory':need(stat.S_ISDIR(r['st_mode'])and v[10]=='-','known directory');r['kind']='directory'
   else:need(want['type']=='file'and stat.S_ISREG(r['st_mode'])and r['st_nlink']==1 and r['st_size']==want['bytes']and v[10]==want['sha256'],'original bytes/hash');r.update(kind='file',sha256=v[10])
   rows[rel]=r
  need(set(rows)==set(fixture['entries']),'exact original69 inventory')
  if guest_before is not None:need(rows==guest_before,'in-run guest69 full9/SHA unchanged; no historical guest claim')
  descriptions={}
  for role,want in fixture['portableRoleInventories'].items():
   root='public-cohort/'+role;need(root in rows and rows[root]['kind']=='directory','original portable role root');table=[['','directory']];total=0
   children={p:[]for p,r in rows.items()if r['kind']=='directory'}
   for p in rows:
    if p!='public-cohort':parent=p.rsplit('/',1)[0];need(parent in children,'explicit directory parent');children[parent].append(p)
   def visit(parent,relative,depth):
    nonlocal total
    need(depth<=128,'portable DFS depth')
    for p in sorted(children[parent]):
     leaf=p.rsplit('/',1)[1];rel=relative+'/'+leaf if relative else leaf;r=rows[p]
     if r['kind']=='directory':table.append([rel,'directory']);visit(p,rel,depth+1)
     else:table.append([rel,'file',r['st_size'],r['sha256']]);total+=r['st_size']
   visit(root,'',0);digest=hashlib.sha256(json.dumps(table,ensure_ascii=False,separators=(',',':'),allow_nan=False).encode()).hexdigest();desc={'sha256':digest,'entries':len(table),'bytes':total};need(desc==want,'original portable8 commitment');descriptions[role]=desc
  save(phase+'-public.json',{'rows':rows,'portableRoleInventories':descriptions,'originalPortableInventoryPin':INVENTORY_PIN,'historicalGuestFD9Qualified':False});return rows
 def verify_consumed(paths):
  for g in sorted(set(paths)):
   if g=='/case/include/nano/chain.hpp':raise ValueError('new documentation-only header must not enter compiler dependencies')
   if g.startswith('/work/'):
    rel=g[len('/work/'):];p=str(src/rel);need(p in source_original and project.get(g)==source_pins[rel],'consumed /work path maps original authority');hold(p,source_original[p],source_pins[rel]['bytes']==0)
   elif g.startswith('/case/'):
    p=str(case/g[len('/case/'):]);need(p in case_files and p in held,'consumed case was admitted');hold(p,held[p])
  if any(p in paths for p in('/case/src/chain.cpp','/case/src/main.cpp','/case/tests/test_cli_readonly.cpp','/case/tests/qualification/cli_readonly_probe.cpp')):need('/work/include/nano/chain.hpp'in paths,'actual original header required in C8 compiler closure')
 def source_global(phase):
  nonlocal global_source_checks,source_metadata_differences
  files,_=inventory(src,12000);need(files==set(source_original),'exact1047 original source names');different=[]
  for p,want in source_original.items():
   row,_=material(p,empty=want['st_size']==0);need(bp(row)==source_pins[str(pathlib.Path(p).relative_to(src))],'all1047 source bytes/size unchanged')
   if row!=want:different.append(p)
  global_source_checks+=1;source_metadata_differences=different;save(phase+'-public.json',{'passed':True,'files':1047,'namesSizeShaQualified':True,'historicalAllSourceFD9Qualified':False,'metadataDifferencesObserved':len(different),'sourceRowsReadopted':False})
 run_checks=0
 def before_run():
  nonlocal run_checks
  run_checks+=1;survey('main','fixture-before-case-'+str(run_checks))
  for p,row in held.items():need(material(p,empty=row['st_size']==0,links=row['st_nlink'])[0]==row,'held input changed before case')
 def interrupted(sig,frame):raise KeyboardInterrupt('owned host signal '+str(sig))
 originals={}
 try:
  for sig in(signal.SIGINT,signal.SIGTERM,signal.SIGHUP):signal_handlers[sig]=signal.signal(sig,interrupted)
  admit({str(cp):expected,str(pathlib.Path(__file__).resolve()):runtime['hostPin']},'runtime-control-host');hold(parent['file'],parent['originalRow'])
  old=document(c['oldControl'],OLD_PIN,'old-control');need(old['format']=='nano-physical-cohort-arm-control-v1'and old['ready']is True,'pinned old control schema, not old runtime PASS');originals=old['originalFiles'];need(type(originals)is dict and len(originals)==9,'exact original9software rows')
  for p,row in originals.items():hold(p,row)
  src=pathlib.Path(old['sourceRoot']['root']);need(src.is_absolute()and src.resolve(strict=True)==src,'physical old source root');source_pins=old['sourceRoot']['pins'];need(type(source_pins)is dict and len(source_pins)==1047,'old source1047 pins');origin=document(c['sourceAuthority'],SOURCE_PIN,'source-authority');need(set(origin)=={'directories','elapsedMs','rows'}and len(origin['rows'])==1050,'original authority1050')
  source_original={str(src/rel):origin['rows'][str(src/rel)]for rel in source_pins};need(len(source_original)==1047 and all(field_row(r)and bp(r)==source_pins[str(pathlib.Path(p).relative_to(src))]for p,r in source_original.items()),'select exactly1047 original /work rows, not old case3');project={'/work/'+rel:v for rel,v in source_pins.items()};source_global('source-before')
  fixture=document(c['fixtureInventory'],INVENTORY_PIN,'original-export-inventory');need(fixture['format']=='nano-public-cohort-export-inventory-v1'and fixture['entryCount']==69 and type(fixture['entries'])is dict and len(fixture['entries'])==69 and len(fixture['portableRoleInventories'])==8,'original69/portable8 inventory')
  need(all((p=='public-cohort'or p.startswith('public-cohort/'))and '..'not in pathlib.PurePosixPath(p).parts and pathlib.PurePosixPath(p).as_posix()==p for p in fixture['entries']),'inventory confined below public-cohort; no/state row')
  case=pathlib.Path(c['caseRoot']['root']);need(type(c['caseRoot'])is dict and set(c['caseRoot'])=={'root','pins'}and type(c['caseRoot']['pins'])is dict,'closed case descriptor');case_pins=c['caseRoot']['pins'];need(len(case_pins)==19 and all(case_pins.get(r)==v for r,v in CASE_PINS.items()),'six exact C8 source/helper pins')
  for r,v in case_pins.items():need(type(r)is str and pathlib.PurePosixPath(r).as_posix()==r and not pathlib.PurePosixPath(r).is_absolute()and '..'not in pathlib.PurePosixPath(r).parts and not any(x in r for x in(',','\0','\n','\r'))and valid_pin(v)and(r in CASE_PINS or r.startswith('tests/')),'bounded explicit public test-only extras')
  need(case.is_absolute()and case.resolve(strict=True)==case,'physical retained case root');need(all(not(a.is_relative_to(b)or b.is_relative_to(a))for a,b in((case,src),(case,out),(src,out))),'disjoint source/case/new runtime output');case_files={str(case/q)for q in case_pins}
  retained={}
  for key,desc in runtime['retainedBuild'].items():
   need(type(desc)is dict and set(desc)=={'file','pin'}and valid_pin(desc['pin']),'known Root retained-build descriptor');retained[key]=document(desc,desc['pin'],'retained-'+key)
  sample=retained['caseAdmission'];need(type(sample)is dict and set(sample)=={'rows','directories','elapsedMs'}and type(sample['rows'])is dict and set(sample['rows'])==case_files and type(sample['directories'])is dict,'exact accepted original case sample')
  case_dirs=sample['directories'];need(case_dirs and all(type(v)is dict and set(v)==set(F)and all(type(x)is int for x in v.values())for v in case_dirs.values()),'original case directory rows')
  fs,ds=inventory(case,256);need(fs==case_files and ds==case_dirs,'original nineteen case inventory/directories unchanged')
  for p,row in sample['rows'].items():need(field_row(row)and bp(row)==case_pins[str(pathlib.Path(p).relative_to(case))],'original case sample byte pins');hold(p,row)
  project.update({'/case/'+q:v for q,v in case_pins.items()})
  need(c['reusePolicy']==REUSE_POLICY,'explicit separate semantic reuse policy, no strict143M claim')
  old_header=hold(src/'include/nano/chain.hpp',source_original[str(src/'include/nano/chain.hpp')]);new_header=hold(case/'include/nano/chain.hpp',held[str(case/'include/nano/chain.hpp')]);need(pin(old_header)==OLD_HEADER_PIN and pin(new_header)==CASE_PINS['include/nano/chain.hpp'],'exact known header versions')
  def active_lines(raw):
   result=[]
   for line in raw.decode().splitlines(keepends=True):
    if line.lstrip().startswith('//'):need(not line.rstrip().endswith('\\'),'comment must not splice a following line')
    else:result.append(line)
   return ''.join(result).encode()
  need(active_lines(old_header)==active_lines(new_header)and pin(active_lines(old_header))==NONCOMMENT_PIN and all(x not in old_header and x not in new_header for x in(b'__LINE__',b'__FILE__',b'source_location')),'comment-only header proof; no code/layout or line-aware token changed');semantic_header_proof={'passed':True,'oldHeaderPin':OLD_HEADER_PIN,'newHeaderPin':CASE_PINS['include/nano/chain.hpp'],'nonCommentLinesPin':NONCOMMENT_PIN,'headerRowsRefreshed':False,**REUSE_POLICY};save('semantic-header-reuse-public.json',semantic_header_proof)
  worker=runtime['worker'];need(type(worker)is dict and set(worker)=={'file','pin'}and worker['pin']==WORKER_PIN,'known reviewed runtime-only worker');admit({worker['file']:WORKER_PIN},'runtime-worker');worker_body=hold(worker['file'],held[worker['file']])
  d=old['docker'];prefix=d['prefix'];need(d['image']==IMAGE and d['architecture']=='arm64'and d['uid']==501 and d['gid']==20 and len(prefix)==5 and prefix[0]in held and set(prefix[1::2])=={'--config','--host'},'old original Docker recipe');need(dict(zip(prefix[1::2],prefix[2::2]))['--host'].startswith('unix:///'),'local daemon only');docker_ready=True
  core=old['core'];libs=old['libraries'];need(core['pin']==CORE_PIN and core['members']==143 and core['guest']=='/retained/libnano_core.a'and type(libs)is list and len(libs)==6,'original retained Linux Core143/sixlibs');known={}
  for item in[core,*libs]:need(item['file']in held and bp(held[item['file']])==item['pin']and item['guest']not in known and not any(x in item['file']for x in(',','\0','\n','\r')),'original provider identity/bytes');known[item['guest']]=item['pin']
  need(len(archive_members(hold(core['file'],originals[core['file']])))==143,'archive143 retained members')
  need(c['coreUnit']['source']=='/case/src/chain.cpp'and c['coreUnit']['object']==c['coreUnit']['member']=='chain.cpp.o','same retained Core body identity; compiler not called')
  need(c['archiveTool']=='/usr/bin/ar','known Linux ar path; guest admitted by worker');configured_units=c['units'];configured_cases=c['cases'];need(type(configured_units)is list and len(configured_units)==8 and type(configured_cases)is list and len(configured_cases)==8,'same bounded leaves and cases')
  unit_names=set();executables=set();required={'nano':'/case/src/main.cpp','readonly':'/case/tests/test_cli_readonly.cpp','cli-probe':'/case/tests/qualification/cli_readonly_probe.cpp'};regressions={'chain':'/case/tests/test_chain.cpp','block-auth':'/case/tests/test_block_auth.cpp','bootstrap-startup':'/case/tests/test_capella_bootstrap_startup.cpp','beacon-gate':'/case/tests/test_beacon_execution_gate.cpp','recovery-audit':'/case/tests/test_recovery_audit.cpp'}
  for unit in configured_units:
   need(type(unit)is dict and set(unit)=={'name','source','compilerFlags','linkPrefix','linkTail','executable'},'closed leaf recipe');name=unit['name'];exe=unit['executable'];need(type(name)is str and re.fullmatch('[a-z][a-z0-9-]{0,47}',name)and name not in unit_names and type(exe)is str and re.fullmatch('[A-Za-z0-9_-]{1,64}',exe)and exe not in executables,'unique safe leaf names');unit_names.add(name);executables.add(exe)
   need(unit['source']in project and(unit['source']==required[name]if name in required else name in regressions and unit['source']==regressions[name]),'exact required leaf or explicit test regression')
   for key in('compilerFlags','linkPrefix','linkTail'):need(type(unit[key])is list and unit[key]and all(type(x)is str and x and not any(v in x for v in('\0','\n','\r'))for x in unit[key]),'explicit leaf argv')
   flags=unit['compilerFlags'];need(flags[0]=='/usr/bin/clang++'and flags.count('-I/work/include')==1 and not any('/case/include'in x for x in flags),'compile original header, never documentation-only case header')
   need(unit['linkPrefix'][0]=='/usr/bin/clang++'and unit['linkTail'].count('/outputs/libnano_core.a')==1 and core['guest']not in unit['linkTail']and all(v['guest']in unit['linkTail']for v in libs),'new singleton Core and original six providers in every link')
   if name=='nano':need(exe=='nano','fixed nano executable basename')
   if name=='cli-probe':need('-I/case'in flags,'probe root include resolves its exact helper')
   if name=='bootstrap-startup':need('-DNANO_TEST_SOURCE="/case"'in flags and '-DNANO_TEST_NANO_BINARY="/outputs/nano"'in flags,'startup macro binds public case fixtures and new nano')
  need(set(required)<=unit_names,'nano/API/probe leaves present')
  case_names=set();case_units=set()
  for item in configured_cases:
   need(type(item)is dict and set(item)=={'name','unit','args','timeoutSeconds','oracle'},'closed worker case');need(type(item['name'])is str and re.fullmatch('[a-z][a-z0-9-]{0,63}',item['name'])and item['name']not in case_names and item['unit']in unit_names-{'nano'}and type(item['args'])is list and all(type(x)is str and not any(v in x for v in('\0','\n','\r'))for x in item['args'])and type(item['timeoutSeconds'])is int and 1<=item['timeoutSeconds']<=240,'bounded explicit cases');case_names.add(item['name']);case_units.add(item['unit'])
   if item['unit']=='readonly':need(item['args']==['--self-test'],'exact API self-test')
   if item['unit']=='cli-probe':need(item['args']==['/outputs/nano','$NANO_SHA','$NANO_BYTES','$PROBE_ROOT'],'exact actual CLI probe inputs')
   if item['unit']=='bootstrap-startup':need(item['args']==['--self-test'],'exact startup selector')
   if item['unit']in('chain','block-auth','beacon-gate'):need(item['args']==[],'exact default regression selector')
   if item['unit']=='recovery-audit':need(item['args']in[['Paris'],['Shanghai']],'only active recovery-audit profiles')
   if item['unit']in REQUIRED_ORACLES:need(json.dumps(item['oracle'],sort_keys=True,allow_nan=False)==json.dumps(REQUIRED_ORACLES[item['unit']],sort_keys=True,allow_nan=False),'source-derived closed mandatory case oracle')
  need(case_units==unit_names-{'nano'},'all test leaves invoked; nano only through probe')
  buildroot=pathlib.Path(c['outputRoot'])/'outputs';need(buildroot.is_absolute()and buildroot.resolve(strict=True)==buildroot and buildroot.is_dir()and not buildroot.is_relative_to(out)and not out.is_relative_to(buildroot),'original physical build root, separate runtime output')
  original_authority=retained['authority'];old_result=retained['workerResult'];prior=retained['priorFailure'];public_proof=retained['archiveProof']
  need(type(original_authority)is dict and set(original_authority)=={'products','guestStatSha','guestAliases','guestFdIdentityVerified'}and original_authority['guestFdIdentityVerified']is False,'original worker rows/guest routing, no historical guest FD claim')
  need(type(old_result)is dict and old_result.get('format')=='nano-cli-readonly-worker-v1'and old_result.get('passed')is False and type(old_result.get('firstFailure'))is dict,'original worker FAILED retained, not requalified')
  need(type(prior)is dict and prior.get('format')=='nano-cli-readonly-arm-host-v1'and prior.get('passed')is False and prior.get('worker')==old_result and type(prior.get('firstFailure'))is dict,'original whole-controller FAILED joined to exact worker result')
  want_counts={'independentM':8,'cppCompilations':8,'links':8,'loaderChecks':8,'runs':0,'CoreM':1,'CoreCpp':1,'archiveUpdates':1}
  need(old_result.get('counts')==want_counts and all(type(old_result['counts'][k])is int for k in want_counts),'retained build complete, no prior cases claimed')
  need(type(old_result.get('steps'))is list,'original completed build events');build_phases=['core-chain-M','core-chain-CXX','Core-replace']
  for unit in configured_units:build_phases.extend(unit['name']+'-'+suffix for suffix in('M','CXX','LINK','LDD'))
  for phase in build_phases:
   matches=[v for v in old_result['steps']if type(v)is dict and v.get('phase')==phase];need(len(matches)==1 and type(matches[0].get('actualExitCode'))is int and matches[0]['actualExitCode']==0,'one retained successful build terminal per phase')
  product_names={'chain.cpp.o.M','chain.cpp.o','chain.cpp.o.d','libnano_core.a'}
  for unit in configured_units:product_names.update((unit['name']+'.o.M',unit['name']+'.o',unit['name']+'.o.d',unit['executable']))
  expected_products={str(buildroot/n)for n in product_names};need(len(expected_products)==36 and type(original_authority['products'])is dict and set(original_authority['products'])==expected_products,'exact thirty-six original retained products')
  retained_product_pins={}
  for p,v in original_authority['products'].items():
   need(type(v)is dict and set(v)=={'hostFD9','guestStatSha'}and field_row(v['hostFD9'])and bp(v['hostFD9'])=={'sha256':v['guestStatSha']['sha256'],'bytes':v['guestStatSha']['bytes']},'original product host/guest byte joint');hold(p,v['hostFD9']);retained_products_held.add(p);retained_product_pins['/outputs/'+pathlib.Path(p).name]=bp(v['hostFD9'])
  for unit in configured_units:
   binary=hold(buildroot/unit['executable'],held[str(buildroot/unit['executable'])]);need(len(binary)>=20 and binary[:6]==b'\x7fELF\x02\x01'and int.from_bytes(binary[18:20],'little')==183,'retained ARM64 ELF')
  need(type(public_proof)is dict and set(public_proof)=={'proof','pureHelperProof','originalArchiveFd9Qualified','cleanBuildQualified','productionQualified'}and public_proof['originalArchiveFd9Qualified']is True and public_proof['cleanBuildQualified']is False and public_proof['productionQualified']is False,'original public singleton proof wrapper')
  archive_proof=public_proof['proof'];need(type(archive_proof)is dict and set(archive_proof)=={'passed','memberCount','changedMembers','unchangedMembers','replacedMember','retainedArchivePin','newArchivePin','objectPin'}and archive_proof==old_result.get('archiveProof')==prior.get('singletonCoreProof'),'same original singleton proof across all three receipts')
  need(archive_proof['passed']is True and type(archive_proof['memberCount'])is int and archive_proof['memberCount']==143 and type(archive_proof['changedMembers'])is int and archive_proof['changedMembers']==1 and type(archive_proof['unchangedMembers'])is int and archive_proof['unchangedMembers']==142 and archive_proof['replacedMember']=='chain.cpp.o'and archive_proof['retainedArchivePin']==CORE_PIN and archive_proof['newArchivePin']==retained_product_pins['/outputs/libnano_core.a']and archive_proof['objectPin']==retained_product_pins['/outputs/chain.cpp.o'],'retained143/1/142 exact old/new/object byte joint')
  pure=public_proof['pureHelperProof'];need(pure.get('passed')is True and pure.get('members')==143 and pure.get('unchangedMembers')==142 and pure.get('replacedMembers')==['chain.cpp.o']and pure.get('memberOrderPreserved')is True and pure.get('symbolTablesSemanticallyValidated')is False,'original pure143 proof retained; no helper file re-admitted')
  aliases=original_authority['guestAliases'];grows=original_authority['guestStatSha'];need(type(aliases)is dict and type(grows)is dict and len(grows)==688 and all(type(k)is str and type(v)is str and v in grows for k,v in aliases.items())and set(aliases.values())==set(grows),'closed retained688 guest providers/routing')
  guest_input_pins={p:{'sha256':v['sha256'],'bytes':v['bytes']}for p,v in grows.items()};need(all(valid_pin(v,True)for v in guest_input_pins.values()),'retained guest byte pins only, not new-container FD9 authority')
  consumed=set();mmd={};expected_mmd={'core-chain':('/case/src/chain.cpp','chain.cpp.o'),**{u['name']:(u['source'],u['name']+'.o')for u in configured_units}}
  def dependency_file(leaf,target):
   raw=hold(buildroot/leaf,held[str(buildroot/leaf)]);need(len(raw)<=4194304,'bounded retained M/MD');mt,sep,text=raw.decode().replace('\\\n',' ').partition(':');need(sep and shlex.split(mt)==[target],'retained exact dependency MT');return sorted(set(posixpath.normpath(posixpath.join('/outputs',p))for p in shlex.split(text)))
  need(set(old_result.get('mmd',{}))==set(expected_mmd),'retained nine M/MD records')
  for name,(source,object_name)in expected_mmd.items():
   target='/outputs/'+object_name;left=dependency_file(object_name+'.M',target);right=dependency_file(object_name+'.d',target);want=old_result['mmd'][name]
   need(left==right and source in left and '/work/include/nano/chain.hpp'in left and '/case/include/nano/chain.hpp'not in left and all(p in aliases for p in left),'retained normalized M=MD and original compiled header')
   need(type(want)is dict and set(want)=={'normalizedMEqualsMd','literalMEqualsMd','dependencyCount','target','source'}and want['normalizedMEqualsMd']is True and want['literalMEqualsMd']is True and type(want['dependencyCount'])is int and want['dependencyCount']==len({aliases[p]for p in left})and want['source']==source and want['target']==target,'retained MMD record joins original files');verify_consumed(left);consumed.update(left);mmd[name]=want
  retained_mmd_complete=True
  for literal,physical in aliases.items():
   expected=project.get(literal)or known.get(literal)or retained_product_pins.get(literal)
   if expected is not None:need(guest_input_pins[physical]==expected,'retained provider joins host-authenticated known bytes')
  known.update(retained_product_pins);retained_build_proof={'archiveProof':archive_proof,'mmd':mmd,'retainedGuestInputs':{'aliases':aliases,'pins':guest_input_pins},'consumedPaths':sorted(consumed)}
  repo=pathlib.Path(old['repoRoot']);git=old['gitExecutable'];need(git in held,'original Git');_,b,_=command('git-head-before',[git,'--no-optional-locks','-c','core.fsmonitor=false','-C',str(repo),'rev-parse','HEAD']);need(b.decode().strip()==runtime['actualHead'],'explicit unchanged HEAD');_,b,_=command('git-clean-before',[git,'--no-optional-locks','-c','core.fsmonitor=false','-C',str(repo),'status','--porcelain']);need(b==b'','clean source checkout')
  image('image-before');volume('volume-before');users('users-before',set());outputs=out/'outputs';outputs.mkdir(mode=0o700);need(outputs.stat().st_uid==501 and outputs.stat().st_gid==20,'owned runtime log outputs');need(all(not any(x in str(p)for x in(',','\0','\n','\r'))for p in(src,case,buildroot)),'safe retained mount paths');cid=create('main',False)
  typed_owners={'/work/'+str(pathlib.Path(p).relative_to(src)):{'uid':row['st_uid'],'gid':row['st_gid']}for p,row in source_original.items()}
  for relative in case_pins:
   row=held[str(case/relative)];typed_owners['/case/'+relative]={'uid':row['st_uid'],'gid':row['st_gid']}
  for item in[core,*libs]:
   row=held[item['file']];typed_owners[item['guest']]={'uid':row['st_uid'],'gid':row['st_gid']}
  for p in retained_product_pins:
   row=held[str(buildroot/p.removeprefix('/outputs/'))];typed_owners[p]={'uid':row['st_uid'],'gid':row['st_gid']}
  need(set(typed_owners)==set(project)|set(known),'typed owners original host rows only')
  guest_before=survey('main','fixture-before-WORKER');command('fresh-run-root',prefix+['exec',cid,'/bin/sh','-ec','test ! -e "$1" && test ! -L "$1"','run-root',runroot]);remaining=int(deadline-time.monotonic());need(remaining>=600,'at least600 runtime worker seconds remain inside host budget')
  module=types.ModuleType('reviewed_arm_cli_readonly_runtime_worker');module.__file__=worker['file'];exec(compile(worker_body,worker['file'],'exec'),module.__dict__)
  cfg={'hostOutput':str(outputs),'dockerPrefix':prefix+['exec',cid],'projectPins':project,'knownGuestPins':known,'guestUid':501,'guestGid':20,'timeoutSeconds':remaining,'execute':lambda phase,args,limit,closing:command(phase,args,limit,closing,None),'verifyConsumed':verify_consumed,'beforeRun':before_run,'runRoot':runroot,'typedOwners':typed_owners,'units':configured_units,'cases':configured_cases,'retainedProductPins':retained_product_pins,'retainedBuildProof':retained_build_proof};wr,authority=module.run_qualification(cfg)
  if type(wr)is dict and wr.get('passed')is False:fail(RuntimeError(json.dumps(wr.get('firstFailure'),sort_keys=True,allow_nan=False)),'worker-primary-failure')
  if guest_before is not None:survey('main','fixture-after-WORKER')
  need(wr['passed']is True and wr['counts']=={'runs':8,'CoreM':0,'CoreCpp':0,'independentM':0,'cppCompilations':0,'links':0,'loaderChecks':0,'archiveUpdates':0}and all(type(v)is int for v in wr['counts'].values()),'eight runtime cases and no new build commands');save('runtime-worker-return-public.json',wr)

 except BaseException as exc:fail(exc,'qualification')
 finally:
  cleanup_end=time.monotonic()+180
  for sig in signal_handlers:
   try:signal.signal(sig,signal.SIG_IGN)
   except BaseException as exc:fail(exc,'ignore-signals')
  if docker_ready:
   try:remove('main')
   except BaseException as exc:fail(exc,'main-cleanup')
   if guest_before is not None and states['main']['cleanup']and states['main']['cleanup'].get('passed'):
    try:users('users-before-audit',set(),True);create('audit',True,True);guest_final=survey('audit','fixture-after-cleanup',True)
    except BaseException as exc:fail(exc,'aftercleanup-passive-fixture-fence')
   try:remove('audit')
   except BaseException as exc:fail(exc,'audit-cleanup')
   try:users('users-final',set(),True)
   except BaseException as exc:fail(exc,'volume-users-final')
   try:volume_final=volume('volume-final',True)
   except BaseException as exc:fail(exc,'volume-final')
   try:image_final=image('image-final',True)
   except BaseException as exc:fail(exc,'image-final')
  if source_original:
   try:source_global('source-after-cleanup')
   except BaseException as exc:changes.append({'scope':'sourceNamesSizeSha','error':str(exc)});fail(exc,'source-global-final')
  for p,row in held.items():
   try:need(material(p,empty=row['st_size']==0,links=row['st_nlink'])[0]==row,'held full9/SHA changed')
   except BaseException as exc:changes.append({'path':p,'error':str(exc)});fail(exc,'held-postcleanup-fence')
  if case_dirs:
   try:fs,ds=inventory(case,100);need(fs==case_files and ds==case_dirs,'new case exact inventory/dirs unchanged')
   except BaseException as exc:changes.append({'scope':'case','error':str(exc)});fail(exc,'case-final-fence')
  for sig,handler in signal_handlers.items():
   try:signal.signal(sig,handler)
   except BaseException as exc:fail(exc,'restore-handlers')
  fence={'format':'nano-cli-readonly-arm-runtime-final-fence-v1','passed':not changes,'heldFiles':len(held),'consumedOriginalSourceFiles':len(set(held)&set(source_original)),'sourceNamesSizeShaChecks':global_source_checks,'historicalAllSourceFD9Qualified':False,'historicalGuestFD9Qualified':False,'guest69InRunUnchanged':guest_before is not None and guest_final==guest_before,'changed':changes,'afterCleanupAndImageInspection':image_final is not None}
  try:save('host-final-fence-public.json',fence)
  except BaseException as exc:fail(exc,'fence-publication')
  report={'format':'nano-cli-readonly-arm-runtime-host-v1','passed':first is None and wr is not None and wr.get('passed')is True and guest_before is not None and guest_final==guest_before and image_final is not None and volume_final is not None,'actualHead':runtime['actualHead'],'worker':wr,'retainedProducts':len(retained_products_held),'retainedOriginalProductFD9Qualified':len(retained_products_held)==36 and not any(v.get('path')in expected_products for v in changes),'retainedBuildProof':archive_proof,'newBuildCommands':0,'fullCoreBuild':False,'priorControllerWholeFailedPreserved':True,'priorWorkerWholeFailedPreserved':True,'oldWholeResultsRequalified':False,'oldControllerAndArchiveHelperHistoricalFD9Qualified':False,'parentControlOriginalRowQualified':parent['file']in held and not any(v.get('path')==parent['file']for v in changes),'retainedCaseOriginalRowsHeld':len(case_files&set(held)),'descriptorHistoricalFD9Qualified':False,'oldHeaderCompiledInRetainedBuild':retained_mmd_complete,'headerDocumentationConsumed':False,'semanticCommentOnlyCoreReuse':True,'strict143MSelectorQualified':False,'cleanBuildQualified':False,'reproducibleBuildQualified':False,'imagesBuilt':0,'configuredUnits':len(configured_units),'configuredCases':len(configured_cases),'consumedOriginalSourceFiles':len(set(held)&set(source_original)),'historicalAllSourceFD9Qualified':False,'historicalGuestFD9Qualified':False,'originalPortableFixtureInventoryPin':INVENTORY_PIN,'originalFixtureEntries':69,'guest69InRunUnchanged':guest_before is not None and guest_final==guest_before,'volume':volume_final,'runRoot':runroot,'cleanup':{k:v['cleanup']for k,v in states.items()},'imageFinal':image_final,'hostFinalFenceAfterCleanup':not changes and image_final is not None,'CppExitSource':'owned docker exec actual exit and closed host process group','keepaliveStateExitCodeIsCppExitCode':False,'firstFailure':first,'secondaryFailures':secondary,'processEvents':events,'runtimeDutiesAuthorized':False,'networkTransport':False,'commonFinalityQualified':False,'operatorRecoveryQualified':False,'productionQualified':False}
  try:save('result-public.json',report)
  except BaseException as exc:fail(exc,'result-publication');report['passed']=False;report['firstFailure']=first
 return report

def main():
 need(len(sys.argv)==4,'usage runtime_host.py CONTROL_FILE CONTROL_SHA256 CONTROL_BYTES');r=run_host(sys.argv[1],{'sha256':sys.argv[2],'bytes':int(sys.argv[3])});print(json.dumps(r,sort_keys=True,allow_nan=False));return 0 if r['passed']else 1
if __name__=='__main__':
 try:sys.exit(main())
 except BaseException as exc:
  if isinstance(exc,SystemExit):raise
  print(json.dumps({'passed':False,'prestartFailure':str(exc)}),file=sys.stderr);sys.exit(1)
