"""SOURCE_ONLY DRAFT C8: singleton Core update, bounded leaves; existing ARM volume.
The 142-member reuse is an explicit semantic comment-only scope, not strict143M.
"""
import datetime,hashlib,json,os,pathlib,re,selectors,signal,stat,subprocess,sys,time,types,uuid
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
CASE_PINS={'src/chain.cpp': {'sha256': 'bc0a192263a4192c7363f4a65f126586504cbb74aff1af6084e69face8b78b46', 'bytes': 44702}, 'src/main.cpp': {'sha256': 'b4433387abb92445ca4b50540c83a7f515f2054281919c46ff2750c2bbbf6e70', 'bytes': 8164}, 'include/nano/chain.hpp': {'sha256': 'd19297f3c55bb9f98ed8ab5cfdbd976951e100b41483e2968e90d4d44382628e', 'bytes': 8250}, 'tests/test_cli_readonly.cpp': {'sha256': '6431cdca3cb643c4f25de1ac4a1508432af05302d98e0365b208c685f01cb35c', 'bytes': 11239}, 'tests/qualification/cli_readonly_probe.cpp': {'sha256': '84a17dadd9aa80b0bbccc44cfdaff218e01938708651af4ff57c589953448f2e', 'bytes': 19202}, 'tools/storage-maintenance/bootstrap-readonly-replica.hpp': {'sha256': 'd3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125', 'bytes': 20319}}
OLD_HEADER_PIN={'sha256': 'f0bdadfbc8aa1b0ca40086df0f03fa498fef3b05e12541d9a82213721bc2b9eb', 'bytes': 8098}
NONCOMMENT_PIN={'sha256': '980503be502b96537e15c819a8ca7c28a2d3886ee67cef4d22830eca7cc9de64', 'bytes': 7121}
ARCHIVE_HELPER_PIN={'sha256': 'aa20f1e88276e674de0414335393621a07ef7d26731bcb4799bf3f3d2cfaacd5', 'bytes': 10821}
CORE_RECIPE_PIN={'sha256': 'eec9f987d3501bfdca2d9d099aa23a6aa8eacac8fa61034b4378af9fe61262a6', 'bytes': 2558894}
CORE_PIN={'sha256':'65c91631ffe93515e500fd1eeacb4e6fd784fa04d431a193c40ec14733077938','bytes':41306456}
OLD_PIN={'sha256':'3a1697c45c0fde5257963392715268013767a8ff53cba9ce68b3f6e5d0f97a3a','bytes':186873}
SOURCE_PIN={'sha256':'0b4b3c84dc4ed99d297278e6ceda22422fa21d3941e08fed88332edf51219f8e','bytes':623344}
INVENTORY_PIN={'sha256':'19d3c604fb202602bc9d5a33e92aa89bb059034f229b419fd505973a88e38bae','bytes':18623}
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
VOLUME={'name':'nano-physical-public-64712f46d71f4587809d58a1bf7c26a3','driver':'local','ownerLabel':'nano.physical-native-volume.owner','ownerValue':'64712f46d71f4587809d58a1bf7c26a3'}
FIELDS={'format','ready','actualHead','oldControl','sourceAuthority','caseRoot','outputRoot','hostPin','worker','volume','fixtureInventory','timeoutSeconds','coreRecipeAuthority','archiveHelper','coreUnit','units','cases','archiveTool','reusePolicy'}
WORKER_PIN={'sha256':'202f7f4f03333a03125d5842426a2427696ec6c4680ad4e80635f1a3d96421a2','bytes':26535}
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
 need(valid_pin(WORKER_PIN),'reviewed C8 worker pin pending; no operations permitted')
 cp=pathlib.Path(control);_,raw=material(cp,True);need(valid_pin(expected)and expected['bytes']<=16777216 and pin(raw)==expected,'externally pinned Root control');c=body_json(raw)
 need(type(c)is dict and set(c)==FIELDS and c['format']=='nano-cli-readonly-arm-control-v1'and c['ready']is True,'exact nineteen-field ready control')
 need(c['volume']==VOLUME and type(c['actualHead'])is str and re.fullmatch('[0-9a-f]{40}',c['actualHead'])and type(c['timeoutSeconds'])is int and 660<=c['timeoutSeconds']<=900,'fixed volume/explicit HEAD/budget')
 out=pathlib.Path(c['outputRoot']);need(out.is_absolute()and out.parent.resolve(strict=True)==out.parent and not os.path.lexists(out)and re.fullmatch('cli-readonly-arm-root-20261018-[0-9]{2}',out.name),'fresh output');out.mkdir(mode=0o700)
 held={};source_original={};source_pins={};case_files=set();case_dirs={};project={};events=[];secondary=[];changes=[];first=None;wr=None;authority=None;prefix=None;docker_ready=False;image_final=None;volume_final=None;guest_before=None;guest_final=None;source_metadata_differences=[];global_source_checks=0;new_products=0;signal_handlers={};archive_proof=None;archive_helper_historical=False;semantic_header_proof=None;expected_products=set();configured_units=[];configured_cases=[]
 deadline=time.monotonic()+c['timeoutSeconds'];cleanup_end=None;nonce=uuid.uuid4().hex;label='nano.cli-readonly.owner';runroot='/state/cli-readonly-'+nonce
 states={role:{'name':'nano-cli-readonly-'+role+'-'+nonce,'cid':None,'attempted':False,'cleanup':None}for role in('main','audit')}
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
  state=states[role];volume(role+'-volume',closing);need(inspect(role,role+'-name-before',state['name'],closing)is None,'unused name');wanted={}if role=='audit'else{'/work':(str(src),False),'/case':(str(case),False),'/outputs':(str(outputs),True),**{v['guest']:(v['file'],False)for v in[core,*libs]}}
  mounts=['--mount','type=volume,src='+VOLUME['name']+',dst=/state,volume-nocopy'+(',readonly'if readonly else'')]
  for dest,(origin,rw)in wanted.items():mounts+=['--mount','type=bind,src='+origin+',dst='+dest+(''if rw else',readonly')]
  argv=prefix+['create','--pull=never','--name',state['name'],'--label',label+'='+nonce,'--label',label+'.role='+role,'--network','none','--read-only','--user','501:20','--cpus','2','--memory','2g','--pids-limit','128','--security-opt','no-new-privileges','--cap-drop','ALL','--tmpfs','/tmp:rw,nosuid,nodev,size=64m',*mounts,'--workdir','/state','--entrypoint','/bin/sh',IMAGE,'-ec','exec sleep '+str(c['timeoutSeconds']+120 if role=='main'else 120)];state['attempted']=True;_,b,_=command(role+'-create',argv,30,closing);candidate=b.decode().strip();need(re.fullmatch('[0-9a-f]{64}',candidate),'created full CID');state['cid']=candidate;x=inspect(role,role+'-created',candidate,closing);hc=x['HostConfig'];actual={m['Destination']:(m['Source'],m['RW'])for m in x['Mounts']if m['Type']=='bind'};vols=[m for m in x['Mounts']if m['Type']=='volume']
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
  admit({str(cp):expected,str(pathlib.Path(__file__).resolve()):c['hostPin']},'control-host')
  old=document(c['oldControl'],OLD_PIN,'old-control');need(old['format']=='nano-physical-cohort-arm-control-v1'and old['ready']is True,'pinned old control schema, not old runtime PASS');originals=old['originalFiles'];need(type(originals)is dict and len(originals)==9,'exact original9software rows')
  for p,row in originals.items():hold(p,row)
  src=pathlib.Path(old['sourceRoot']['root']);need(src.is_absolute()and src.resolve(strict=True)==src,'physical old source root');source_pins=old['sourceRoot']['pins'];need(type(source_pins)is dict and len(source_pins)==1047,'old source1047 pins');origin=document(c['sourceAuthority'],SOURCE_PIN,'source-authority');need(set(origin)=={'directories','elapsedMs','rows'}and len(origin['rows'])==1050,'original authority1050')
  source_original={str(src/rel):origin['rows'][str(src/rel)]for rel in source_pins};need(len(source_original)==1047 and all(field_row(r)and bp(r)==source_pins[str(pathlib.Path(p).relative_to(src))]for p,r in source_original.items()),'select exactly1047 original /work rows, not old case3');project={'/work/'+rel:v for rel,v in source_pins.items()};source_global('source-before')
  fixture=document(c['fixtureInventory'],INVENTORY_PIN,'original-export-inventory');need(fixture['format']=='nano-public-cohort-export-inventory-v1'and fixture['entryCount']==69 and type(fixture['entries'])is dict and len(fixture['entries'])==69 and len(fixture['portableRoleInventories'])==8,'original69/portable8 inventory')
  need(all((p=='public-cohort'or p.startswith('public-cohort/'))and '..'not in pathlib.PurePosixPath(p).parts and pathlib.PurePosixPath(p).as_posix()==p for p in fixture['entries']),'inventory confined below public-cohort; no/state row')
  case=pathlib.Path(c['caseRoot']['root']);need(type(c['caseRoot'])is dict and set(c['caseRoot'])=={'root','pins'}and type(c['caseRoot']['pins'])is dict,'closed case descriptor');case_pins=c['caseRoot']['pins'];need(6<=len(case_pins)<=128 and all(case_pins.get(r)==v for r,v in CASE_PINS.items()),'six exact C8 source/helper pins')
  for r,v in case_pins.items():need(type(r)is str and pathlib.PurePosixPath(r).as_posix()==r and not pathlib.PurePosixPath(r).is_absolute()and '..'not in pathlib.PurePosixPath(r).parts and not any(x in r for x in(',','\0','\n','\r'))and valid_pin(v)and(r in CASE_PINS or r.startswith('tests/')),'bounded explicit public test-only extras')
  need(case.is_absolute()and case.resolve(strict=True)==case,'physical new case root');need(all(not(a.is_relative_to(b)or b.is_relative_to(a))for a,b in((case,src),(case,out),(src,out))),'disjoint source/case/output');case_files={str(case/r)for r in case_pins};need(not case_files.intersection(origin['rows']),'new case never refreshes original rows');case_dirs=admit({str(case/r):v for r,v in case_pins.items()},'new-case',case);project.update({'/case/'+r:v for r,v in case_pins.items()})
  need(c['reusePolicy']==REUSE_POLICY,'explicit separate semantic reuse policy, no strict143M claim')
  old_header=hold(src/'include/nano/chain.hpp',source_original[str(src/'include/nano/chain.hpp')]);new_header=hold(case/'include/nano/chain.hpp',held[str(case/'include/nano/chain.hpp')]);need(pin(old_header)==OLD_HEADER_PIN and pin(new_header)==CASE_PINS['include/nano/chain.hpp'],'exact known header versions')
  def active_lines(raw):
   result=[]
   for line in raw.decode().splitlines(keepends=True):
    if line.lstrip().startswith('//'):need(not line.rstrip().endswith('\\'),'comment must not splice a following line')
    else:result.append(line)
   return ''.join(result).encode()
  need(active_lines(old_header)==active_lines(new_header)and pin(active_lines(old_header))==NONCOMMENT_PIN and all(x not in old_header and x not in new_header for x in(b'__LINE__',b'__FILE__',b'source_location')),'comment-only header proof; no code/layout or line-aware token changed');semantic_header_proof={'passed':True,'oldHeaderPin':OLD_HEADER_PIN,'newHeaderPin':CASE_PINS['include/nano/chain.hpp'],'nonCommentLinesPin':NONCOMMENT_PIN,'headerRowsRefreshed':False,**REUSE_POLICY};save('semantic-header-reuse-public.json',semantic_header_proof)
  c7=document(c['coreRecipeAuthority'],CORE_RECIPE_PIN,'core-recipe-authority');need(type(c7)is dict and type(c7.get('workerConfig'))is dict,'known C7 recipe authority');base_flags=c7['workerConfig']['coreCompilerFlags'];need(type(base_flags)is list and base_flags and all(type(x)is str and x for x in base_flags),'original C7 Core compile argv');expected_core_flags=list(base_flags);need(expected_core_flags.count('-I/work/include')==1 and not any('/case/include'in x for x in expected_core_flags),'one original include position; documentation header not compiled')
  ah=c['archiveHelper'];need(type(ah)is dict and set(ah)=={'file','pin'}and ah['pin']==ARCHIVE_HELPER_PIN,'fixed pure archive helper descriptor');original_helper=c7.get('originalFiles',{}).get(ah['file'])
  if original_helper is not None:helper_body=hold(ah['file'],original_helper);need(bp(original_helper)==ARCHIVE_HELPER_PIN,'original archive helper pin');archive_helper_historical=True
  else:admit({ah['file']:ARCHIVE_HELPER_PIN},'new-known-archive-helper');helper_body=hold(ah['file'],held[ah['file']])
  ah_module=types.ModuleType('verified_c8_archive_comparison');ah_module.__file__=ah['file'];exec(compile(helper_body,ah['file'],'exec'),ah_module.__dict__)
  worker=c['worker'];need(type(worker)is dict and set(worker)=={'file','pin'}and worker['pin']==WORKER_PIN,'known reviewed worker');admit({worker['file']:WORKER_PIN},'worker');worker_body=hold(worker['file'],held[worker['file']])
  d=old['docker'];prefix=d['prefix'];need(d['image']==IMAGE and d['architecture']=='arm64'and d['uid']==501 and d['gid']==20 and len(prefix)==5 and prefix[0]in held and set(prefix[1::2])=={'--config','--host'},'old original Docker recipe');need(dict(zip(prefix[1::2],prefix[2::2]))['--host'].startswith('unix:///'),'local daemon only');docker_ready=True
  core=old['core'];libs=old['libraries'];need(core['pin']==CORE_PIN and core['members']==143 and core['guest']=='/retained/libnano_core.a'and type(libs)is list and len(libs)==6,'original retained Linux Core143/sixlibs');known={}
  for item in[core,*libs]:need(item['file']in held and bp(held[item['file']])==item['pin']and item['guest']not in known and not any(x in item['file']for x in(',','\0','\n','\r')),'original provider identity/bytes');known[item['guest']]=item['pin']
  need(len(archive_members(hold(core['file'],originals[core['file']])))==143,'archive143 retained members')
  core_unit=c['coreUnit'];need(type(core_unit)is dict and set(core_unit)=={'source','compilerFlags','object','member'}and core_unit['source']=='/case/src/chain.cpp'and core_unit['object']==core_unit['member']=='chain.cpp.o'and core_unit['compilerFlags']==expected_core_flags,'single new Core body and original C7 flags exactly unchanged')
  need(c['archiveTool']=='/usr/bin/ar','known Linux ar path; guest admitted by worker');configured_units=c['units'];configured_cases=c['cases'];need(type(configured_units)is list and 3<=len(configured_units)<=8 and type(configured_cases)is list and 1<=len(configured_cases)<=24,'bounded leaves and cases')
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
  repo=pathlib.Path(old['repoRoot']);git=old['gitExecutable'];need(git in held,'original Git');_,b,_=command('git-head-before',[git,'--no-optional-locks','-c','core.fsmonitor=false','-C',str(repo),'rev-parse','HEAD']);need(b.decode().strip()==c['actualHead'],'explicit current HEAD');_,b,_=command('git-clean-before',[git,'--no-optional-locks','-c','core.fsmonitor=false','-C',str(repo),'status','--porcelain']);need(b==b'','clean source checkout')
  image('image-before');volume('volume-before');users('users-before',set());outputs=out/'outputs';outputs.mkdir(mode=0o700);need(outputs.stat().st_uid==501 and outputs.stat().st_gid==20,'owned build outputs');need(all(not any(x in str(p)for x in(',','\0','\n','\r'))for p in(src,case,outputs)),'safe mount paths');cid=create('main',False)
  typed_owners={'/work/'+str(pathlib.Path(p).relative_to(src)):{'uid':row['st_uid'],'gid':row['st_gid']}for p,row in source_original.items()}
  for relative in case_pins:
   row=held[str(case/relative)];typed_owners['/case/'+relative]={'uid':row['st_uid'],'gid':row['st_gid']}
  for item in[core,*libs]:
   row=held[item['file']];typed_owners[item['guest']]={'uid':row['st_uid'],'gid':row['st_gid']}
  need(set(typed_owners)==set(project)|set(known),'typed owners from original rows and only newly admitted case')
  retained_core_raw=hold(core['file'],originals[core['file']]);old_object_rows=ah_module.archive_object_rows(retained_core_raw);need(len(old_object_rows)==143,'trusted Core143 object rows')
  def prove_archive_singleton(old_guest,new_guest,member,object_guest):
   nonlocal archive_proof
   need(old_guest==core['guest']and new_guest=='/outputs/libnano_core.a'and member=='chain.cpp.o'and object_guest=='/outputs/chain.cpp.o'and archive_proof is None,'one exact archive proof invocation');before=hold(core['file'],originals[core['file']]);new_row,after=material(outputs/'libnano_core.a',True);object_row,object_body=material(outputs/'chain.cpp.o',True);proof=ah_module.prove_object_replacement(before,after,object_name=member,qualified_old_rows=old_object_rows,old_archive_pin=CORE_PIN,new_object_pin=pin(object_body),expected_members=143)
   need(proof['passed']is True and proof['members']==143 and proof['unchangedMembers']==142 and proof['replacedMembers']==[member]and proof['memberOrderPreserved']is True and proof['symbolTablesSemanticallyValidated']is False,'one member changed, other142 raw payloads exact');archive_proof={'passed':True,'memberCount':143,'changedMembers':1,'unchangedMembers':142,'replacedMember':member,'retainedArchivePin':CORE_PIN,'newArchivePin':bp(new_row),'objectPin':bp(object_row)};save('core-singleton-proof-public.json',{'proof':archive_proof,'pureHelperProof':proof,'originalArchiveFd9Qualified':True,'cleanBuildQualified':False,'productionQualified':False});return dict(archive_proof)
  guest_before=survey('main','fixture-before-WORKER');command('fresh-run-root',prefix+['exec',cid,'/bin/sh','-ec','test ! -e "$1" && test ! -L "$1"','run-root',runroot]);remaining=int(deadline-time.monotonic());need(remaining>=600,'at least600 worker seconds remain inside host budget')
  module=types.ModuleType('reviewed_arm_cli_readonly_worker');module.__file__=worker['file'];exec(compile(worker_body,worker['file'],'exec'),module.__dict__);cfg={'hostOutput':str(outputs),'dockerPrefix':prefix+['exec',cid],'projectPins':project,'knownGuestPins':known,'guestUid':501,'guestGid':20,'timeoutSeconds':remaining,'execute':lambda phase,args,limit,closing:command(phase,args,limit,closing,None),'verifyConsumed':verify_consumed,'beforeRun':before_run,'runRoot':runroot,'typedOwners':typed_owners,'coreUnit':core_unit,'units':configured_units,'cases':configured_cases,'archiveTool':c['archiveTool'],'retainedCore':core['guest'],'proveArchiveSingleton':prove_archive_singleton};wr,authority=module.run_qualification(cfg)
  if type(wr)is dict and wr.get('passed')is False:fail(RuntimeError(json.dumps(wr.get('firstFailure'),sort_keys=True,allow_nan=False)),'worker-primary-failure')
  need(type(authority)is dict and set(authority)=={'products','guestStatSha','guestAliases','guestFdIdentityVerified'}and authority['guestFdIdentityVerified']is False,'guestGNU9 distinct from host FD9');product_names={'chain.cpp.o.M','chain.cpp.o','chain.cpp.o.d','libnano_core.a'}
  for unit in configured_units:product_names.update((unit['name']+'.o.M',unit['name']+'.o',unit['name']+'.o.d',unit['executable']))
  expected_products={str(outputs/n)for n in product_names};need(set(authority['products'])<=expected_products,'only prospective Core/leaves products')
  for p,r in authority['products'].items():need(set(r)=={'hostFD9','guestStatSha'}and bp(r['hostFD9'])=={'sha256':r['guestStatSha']['sha256'],'bytes':r['guestStatSha']['bytes']},'new product host/guest bytes');hold(p,r['hostFD9']);new_products+=1
  if guest_before is not None:survey('main','fixture-after-WORKER')
  need(archive_proof is not None and bp(authority['products'][str(outputs/'libnano_core.a')]['hostFD9'])==archive_proof['newArchivePin']and bp(authority['products'][str(outputs/'chain.cpp.o')]['hostFD9'])==archive_proof['objectPin'],'archive proof joined to exact admitted product rows')
  want_counts={'independentM':len(configured_units),'cppCompilations':len(configured_units),'links':len(configured_units),'loaderChecks':len(configured_units),'runs':len(configured_cases),'CoreM':1,'CoreCpp':1,'archiveUpdates':1}
  need(wr['passed']is True and wr['counts']==want_counts and all(type(wr['counts'][k])is int for k in want_counts)and set(authority['products'])==expected_products,'complete singleton Core plus all configured leaves/cases passed');expected_mmd={'core-chain':('/case/src/chain.cpp','/outputs/chain.cpp.o'),**{u['name']:(u['source'],'/outputs/'+u['name']+'.o')for u in configured_units}};need(set(wr['mmd'])==set(expected_mmd)and all(wr['mmd'][k]['normalizedMEqualsMd']is True and wr['mmd'][k]['literalMEqualsMd']is True and wr['mmd'][k]['source']==v[0]and wr['mmd'][k]['target']==v[1]for k,v in expected_mmd.items()),'exact complete source/MT/MMD cohort');need(type(wr['caseResults'])is list and len(wr['caseResults'])==len(configured_cases)and all(set(r)=={'name','unit','passed','records'}and r['name']==q['name']and r['unit']==q['unit']and r['passed']is True for r,q in zip(wr['caseResults'],configured_cases)),'exact ordered case results');save('worker-return-public.json',wr)
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
  fence={'format':'nano-cli-readonly-arm-final-fence-v1','passed':not changes,'heldFiles':len(held),'consumedOriginalSourceFiles':len(set(held)&set(source_original)),'sourceNamesSizeShaChecks':global_source_checks,'historicalAllSourceFD9Qualified':False,'historicalGuestFD9Qualified':False,'guest69InRunUnchanged':guest_before is not None and guest_final==guest_before,'changed':changes,'afterCleanupAndImageInspection':image_final is not None}
  try:save('host-final-fence-public.json',fence)
  except BaseException as exc:fail(exc,'fence-publication')
  report={'format':'nano-cli-readonly-arm-host-v1','passed':first is None and wr is not None and wr.get('passed')is True and archive_proof is not None and guest_before is not None and guest_final==guest_before and image_final is not None and volume_final is not None,'actualHead':c['actualHead'],'worker':wr,'originalCorePin':CORE_PIN,'CoreMembers':143,'archiveUpdated':archive_proof is not None,'fullCoreBuild':False,'singletonCoreProof':archive_proof,'semanticHeaderReuseProof':semantic_header_proof,'headerDocumentationConsumed':False,'oldHeaderCompiled':wr is not None and 'core-chain'in wr.get('mmd',{}),'strict143MSelectorQualified':False,'cleanBuildQualified':False,'reproducibleBuildQualified':False,'archiveHelperHistoricalFileFD9Qualified':archive_helper_historical,'imagesBuilt':0,'newBuildProductsHeld':new_products,'configuredUnits':len(configured_units),'configuredCases':len(configured_cases),'consumedOriginalSourceFiles':len(set(held)&set(source_original)),'historicalAllSourceFD9Qualified':False,'historicalGuestFD9Qualified':False,'oldPhysicalControllerFailedPreserved':True,'originalPortableFixtureInventoryPin':INVENTORY_PIN,'originalFixtureEntries':69,'guest69InRunUnchanged':guest_before is not None and guest_final==guest_before,'volume':volume_final,'runRoot':runroot,'cleanup':{k:v['cleanup']for k,v in states.items()},'imageFinal':image_final,'hostFinalFenceAfterCleanup':not changes and image_final is not None,'CppExitSource':'owned docker exec actual exit and closed host process group','keepaliveStateExitCodeIsCppExitCode':False,'firstFailure':first,'secondaryFailures':secondary,'processEvents':events,'runtimeDutiesAuthorized':False,'networkTransport':False,'commonFinalityQualified':False,'operatorRecoveryQualified':False,'productionQualified':False}
  try:save('result-public.json',report)
  except BaseException as exc:fail(exc,'result-publication');report['passed']=False;report['firstFailure']=first
 return report

def main():
 need(len(sys.argv)==4,'usage host.py CONTROL_FILE CONTROL_SHA256 CONTROL_BYTES');r=run_host(sys.argv[1],{'sha256':sys.argv[2],'bytes':int(sys.argv[3])});print(json.dumps(r,sort_keys=True,allow_nan=False));return 0 if r['passed']else 1
if __name__=='__main__':
 try:sys.exit(main())
 except BaseException as exc:
  if isinstance(exc,SystemExit):raise
  print(json.dumps({'passed':False,'prestartFailure':str(exc)}),file=sys.stderr);sys.exit(1)
