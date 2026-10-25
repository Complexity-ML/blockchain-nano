import pathlib,os,json,hashlib,stat,subprocess,tarfile,time,re,shlex,sys,resource
D=pathlib.Path(sys.argv[1]).resolve();cfg=json.loads((D/'dispatch-public.json').read_text());R=pathlib.Path(cfg['repository']);REV=cfg['sourceCandidate'];BASE=cfg['base'];DOCKER=cfg['docker'];IMAGE=cfg['image'];NAME=cfg['containerName'];LABEL=cfg['label'];start=time.monotonic();held=[];closed=0;first=None;secondary=[];docker_exit=None;archive_proof=None;test_rows=[];source_limits=resource.getrlimit(resource.RLIMIT_NOFILE);raised=False;git_inputs_exact=False;empty_git_inputs=0

def need(v,m):
 if not v:raise ValueError(m)
def run(a,timeout=30,**kw):return subprocess.run(a,check=True,capture_output=True,timeout=timeout,**kw)
def git(*a):return run(['git',*a],cwd=R).stdout.decode().strip()
def nine(s):return[str(getattr(s,k))for k in['st_dev','st_ino','st_mode','st_uid','st_gid','st_nlink','st_size','st_mtime_ns','st_ctime_ns']]
def hashfd(fd,size):
 h=hashlib.sha256();at=0
 while at<size:
  b=os.pread(fd,min(1048576,size-at),at);need(b,'short held read');h.update(b);at+=len(b)
 need(not os.pread(fd,1,at),'input grew');return h.hexdigest()
def pin(path,expected=None,expectedFullNine=None):
 path=pathlib.Path(path).resolve(strict=True);fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<=a.st_size<=134217728 and(a.st_size>0 or expected is not None),'bounded regular source/provider');m=nine(a);need(expectedFullNine is None or m==expectedFullNine,'NEW input stable metadata matches first admission');sha=hashfd(fd,a.st_size);need(nine(os.fstat(fd))==m==nine(path.lstat()),'source first FD/path fence')
  if expected:need(a.st_size==expected['bytes']and sha==expected['sha256'],'exact source/provider pin: '+path.name)
  held.append((path,fd,m,sha));return {'path':str(path),'bytes':a.st_size,'sha256':sha}
 except BaseException:os.close(fd);raise

def settle_known_git_files(files,pins,base=None):
 # Prospective admission of NEW extracted public Git payloads only. No retained
 # authority is refreshed, and no metadata relaxation applies after pin().
 base=source if base is None else base
 previous=None;stable=0;first_stable=None;deadline=time.monotonic()+10
 while time.monotonic()<deadline:
  snapshot=[]
  for p in files:
   need(time.monotonic()<deadline,'NEW Git intake deadline before file');expected=pins[str(p.relative_to(base))];fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
   try:
    a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and a.st_size==expected['bytes'],'known NEW Git fixture type/size');h=hashfd(fd,a.st_size);need(h==expected['sha256'],'known NEW Git payload exact');snapshot.append((str(p.relative_to(base)),nine(a),nine(os.fstat(fd)),nine(p.lstat())))
   finally:os.close(fd)
   need(time.monotonic()<deadline,'NEW Git intake deadline after file')
  need(time.monotonic()<deadline,'NEW Git intake deadline after scan')
  internal=all(a==b==c for _,a,b,c in snapshot)
  signature=[(n,a)for n,a,_,_ in snapshot]
  if internal and signature==previous:stable+=1
  elif internal:stable=1;first_stable=time.monotonic()
  else:stable=0;first_stable=None
  previous=signature if internal else None
  if stable>=3 and time.monotonic()-first_stable>=2:
   need(time.monotonic()<deadline,'NEW Git intake deadline at admission');return {'report':{'stableSamples':stable,'stableSpanMs':round((time.monotonic()-first_stable)*1000),'files':len(files),'payloadsBoundToGit':True,'retainedAuthoritiesRefreshed':False},'materials':dict(signature)}
  remaining=deadline-time.monotonic();need(remaining>0,'NEW Git intake deadline before wait');time.sleep(min(1,remaining))
 raise ValueError('NEW exact-Git input metadata did not settle before first admission')

def archive_members(b):
 need(b[:8]==b'!<arch>\n','GNU archive');at=8;names={};long=b''
 while at<len(b):
  h=b[at:at+60];need(len(h)==60 and h[58:]==b'`\n','archive header');size=int(h[48:58]);name=h[:16].decode().strip();at+=60;data=b[at:at+size];need(len(data)==size,'archive body');at+=size+(size%2)
  if name=='//':long=data;continue
  if name in['/','/SYM64/']:continue
  if name.startswith('/'):
   need(name[1:].isdigit(),'archive long name');pos=int(name[1:]);end=long.find(b'/\n',pos);need(end>=pos,'archive long-name terminator');name=long[pos:end].decode()
  else:name=name.removesuffix('/')
  need(re.fullmatch('[A-Za-z0-9_.-]+[.]o',name)and name not in names,'unique plain archive object name');names[name]=data
 need(at==len(b),'archive exact length');return names
try:
 pin(pathlib.Path(__file__),cfg['controllerSourcePin']);pin(D/'dispatch-public.json')
 need(git('rev-parse','HEAD')==BASE and not git('status','--porcelain'),'main remains clean at source base')
 prior=pathlib.Path(cfg['prior']);prior_cfg=json.loads((prior/'dispatch-public.json').read_text());prior_report=json.loads((prior/'result-public.json').read_text());pin(prior/'result-public.json',cfg['priorReportPin'])
 need(prior_cfg['sourceCandidate']==REV and prior_cfg['image']==IMAGE and prior_report['sourceCandidate']==REV,'same original source and image')
 need(prior_report['passed']is False and prior_report['actualDockerExitCode']==1 and prior_report['firstFailure']['reason']=='actual core compile/test Docker exit zero' and not prior_report['secondaryFailures'] and prior_report['gitArchivePayloadsByteExact']is True and prior_report['heldFilesClosed']==prior_report['heldSourceAndProviderFiles'],'original failed batch has exact source fences and completed closure')
 pin(prior/'held-inputs-public.json',cfg['priorHeldInputsPin']);previous=json.loads((prior/'held-inputs-public.json').read_text());wanted=len(previous)+1000;need(wanted<=4096 and(source_limits[1]==resource.RLIM_INFINITY or wanted<=source_limits[1]),'bounded continuation FD allowance')
 if wanted>source_limits[0]:resource.setrlimit(resource.RLIMIT_NOFILE,(wanted,source_limits[1]));raised=True
 for row in previous:pin(row['path'],{'bytes':int(row['full9'][6]),'sha256':row['sha256']},row['full9'])
 pin(prior/'steps-public.json',cfg['priorStepsPin']);pin(prior/'stdout.log',cfg['priorStdoutPin']);oldout=prior/'outputs';steps=json.loads((prior/'steps-public.json').read_text());need([s['name']for s in steps[-2:]]==['run-capella_transitionlocal-exits','run-capella_transitionexit-wire'],'only final Capella invocations remain')
 for s in steps[:-2]:
  marker=oldout/(s['name']+'.exit');pin(marker,{'bytes':2,'sha256':hashlib.sha256(b'0\n').hexdigest()});need(marker.read_text()=='0\n','exact previous stage terminal exit zero: '+s['name'])
  pin(oldout/(s['name']+'.stdout.log'),{'bytes':(oldout/(s['name']+'.stdout.log')).stat().st_size,'sha256':hashlib.sha256((oldout/(s['name']+'.stdout.log')).read_bytes()).hexdigest()})
 oldfailed=oldout/'run-capella_transitionlocal-exits.stderr.log';pin(oldfailed);need('consensus-altair-fork-head-official/manifest.json'in oldfailed.read_text(),'preserve exact missing-fixture cause')
 built=pin(oldout/'libnano_core.a',prior_report['newCore']);old=archive_members(pathlib.Path(prior_cfg['core']['path']).read_bytes());new=archive_members(pathlib.Path(built['path']).read_bytes());changed={pathlib.Path(p).name+'.o'for p in cfg['modifiedCoreUnits']};need(len(old)==len(new)==143 and set(old)==set(new),'same143 object names')
 for name in changed:
  artifact=pin(oldout/'obj'/name,{'bytes':len(new[name]),'sha256':hashlib.sha256(new[name]).hexdigest()});need(pathlib.Path(artifact['path']).read_bytes()==new[name],'exact replacement object bytes')
 need(all(old[name]==new[name]for name in old.keys()-changed),'134 historical payloads unchanged')
 archive_proof={'members':143,'replaced':sorted(changed),'unchangedObjectPayloads':134,'allKeptObjectsByteExact':True,'retainedObjectsCompiledWithHistoricalHeaders':True,'completeCurrentHeaderMmdClosureClaimed':False,'coreSourceBase':cfg['coreSourceBase']}
 mmd=[]
 names=[pathlib.Path(p).name for p in cfg['modifiedCoreUnits']]+[r['name']for r in cfg['tests']]
 for name in names:
  a=oldout/'obj'/(name+'.M');b=oldout/'obj'/(name+'.d');pin(a);pin(b)
  parse=lambda p:shlex.split(p.read_text().replace('\\\n',' ').split(':',1)[1])
  need(parse(a)==parse(b),'previous independent M and compile MD identical');mmd.append({'unit':name,'dependencies':len(parse(a))})
 inherited=[]
 for test in cfg['tests']:
  exe=pin(oldout/test['name']);
  if test['name']=='capella_transition':capella=exe;continue
  name='run-'+test['name'];textlog=(oldout/(name+'.stdout.log')).read_text();inherited.append({'name':name,'actualExitCode':0,'reexecuted':False,'executable':{k:exe[k]for k in['sha256','bytes']},'stdout':textlog})
 need(len(inherited)==8,'eight inherited exact runtime exits')
 cancellation=json.loads(inherited[0]['stdout']);need(cancellation['cases']==28 and len(cancellation['groups'])==8 and cancellation['syntheticRegistryEntries']==8192 and cancellation['consensusCampaignExecuted']is False,'28 previously executed cancellation cases')
 source=D/'runtime-source';source.mkdir(mode=0o700);arc=D/'runtime-fixtures.tar';run(['git','archive','--format=tar','--output',str(arc),REV,*cfg['runtimeFixtureRoots']],cwd=R)
 raw=arc.read_bytes();ap={'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()};astable=settle_known_git_files([arc],{arc.name:ap},D);pin(arc,ap,astable['materials'][arc.name]);expected={}
 with tarfile.open(arc)as t:
  members=t.getmembers();need(len(members)<2500 and sum(x.size for x in members)<33554432,'bounded canonical runtime fixtures');seen=set()
  for row in members:
   name=pathlib.PurePosixPath(row.name);need(not name.is_absolute()and '..'not in name.parts and row.name not in seen and(row.isdir()or row.isfile()),'unique relative canonical fixture');seen.add(row.name)
   if row.isfile():
    with t.extractfile(row)as f:b=f.read()
    expected[row.name]={'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
  t.extractall(source,members=members,filter='data')
 files=sorted(p for p in source.rglob('*')if p.is_file());need(len(files)==len(expected),'exact canonical fixture count');wanted=max(wanted,len(held)+len(files)+100);need(wanted<=4096 and(source_limits[1]==resource.RLIM_INFINITY or wanted<=source_limits[1]),'bounded complete continuation FD allowance')
 if wanted>resource.getrlimit(resource.RLIMIT_NOFILE)[0]:resource.setrlimit(resource.RLIMIT_NOFILE,(wanted,source_limits[1]));raised=True
 stable_inputs=settle_known_git_files(files,expected);admission=stable_inputs['report']
 for p in files:
  name=str(p.relative_to(source));pin(p,expected[name],stable_inputs['materials'][name])
 out=D/'outputs';out.mkdir(mode=0o700);script='set -eu\n'
 for selector in ['local-exits','exit-wire']:
  script+="printf '%s\\n' "+shlex.quote(json.dumps({'stage':selector}))+"\n";script+='/usr/bin/timeout --signal=TERM --kill-after=5s 180s /case/capella-regression --'+selector+' >/out/'+selector+'.stdout.log 2>/out/'+selector+'.stderr.log\n';script+="printf '0\\n' >/out/"+selector+'.exit\n'
 (D/'run-public.sh').write_text(script);need(not run(DOCKER+['ps','-a','--filter','name=^/'+NAME+'$','--format','{{.ID}}']).stdout.strip(),'fresh continuation container absent')
 argv=DOCKER+['run','--rm','--pull=never','--init','--cidfile',str(D/'cid'),'--name',NAME,'--label','nano.qualification='+LABEL,'--network','none','--read-only','--user',cfg['user'],'--cap-drop','ALL','--security-opt','no-new-privileges','--memory','1g','--cpus','1','--pids-limit','128','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m,mode=1777','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+capella['path']+',dst=/case/capella-regression,readonly','--mount','type=bind,src='+str(out)+',dst=/out','--entrypoint','/bin/sh',IMAGE,'-ec',script]
 (D/'argv-public.json').write_text(json.dumps(argv,indent=2)+'\n')
 with open(D/'stdout.log','xb')as stdout,open(D/'stderr.log','xb')as stderr:p=subprocess.run(argv,stdout=stdout,stderr=stderr,timeout=400)
 docker_exit=p.returncode;need(docker_exit==0,'actual Capella continuation Docker exit zero')
 for selector in['local-exits','exit-wire']:
  need((out/(selector+'.exit')).read_text()=='0\n','actual Capella terminal exit zero');test_rows.append({'name':selector,'actualExitCode':0,'stdout':(out/(selector+'.stdout.log')).read_text(),'executableSha256':capella['sha256']})
except BaseException as e:first={'type':type(e).__name__,'reason':str(e)[:1000]}
finally:
 try:
  if(D/'cid').exists():
   cid=(D/'cid').read_text().strip();need(re.fullmatch('[0-9a-f]{64}',cid),'exact own CID')
   if run(DOCKER+['ps','-a','--filter','id='+cid,'--format','{{.ID}}']).stdout.strip():
    v=json.loads(run(DOCKER+['inspect',cid,'--format','{{json .}}']).stdout);need(v['Id']==cid and v['Name']=='/'+NAME and v['Image']==IMAGE and v['Config']['Labels'].get('nano.qualification')==LABEL,'own continuation cleanup identity');run(DOCKER+['rm','-f',cid]);raise ValueError('late owned continuation cleanup required')
 except BaseException as e:secondary.append({'phase':'cleanup','reason':str(e)[:500]})
 for p,fd,m,h in held:
  try:need(nine(os.fstat(fd))==m==nine(p.lstat())and hashfd(fd,int(m[6]))==h,'strict original full9/SHA authority retained')
  except BaseException as e:secondary.append({'phase':'fence','path':str(p),'reason':str(e)[:500]})
 for _,fd,_,_ in held:
  try:os.close(fd);closed+=1
  except BaseException as e:secondary.append({'phase':'close','reason':str(e)[:500]})
 if raised:
  try:resource.setrlimit(resource.RLIMIT_NOFILE,source_limits)
  except BaseException as e:secondary.append({'phase':'fd-limit-restore','reason':str(e)[:500]})
report={'format':'nano-root-capella-fixture-only-continuation-v1','passed':first is None and not secondary,'firstFailure':first,'secondaryFailures':secondary,'sourceCandidate':REV,'architecture':cfg['architecture'],'priorBatchRemainsFailed':True,'priorInputAuthoritiesRefreshed':False,'newRuntimeFixtureAdmission':locals().get('admission'),'actualDockerExitCode':docker_exit,'elapsedMs':round((time.monotonic()-start)*1000),'heldFilesOpened':len(held),'heldFilesClosed':closed,'operatorFdLimitRestored':resource.getrlimit(resource.RLIMIT_NOFILE)==source_limits,'image':IMAGE,'imageBuilds':0,'cppCompilationsRepeated':0,'inheritedTestInvocations':locals().get('inherited',[]),'newTestInvocations':test_rows,'archiveProof':archive_proof,'independentMmdComparisons':locals().get('mmd',[]),'wholeCurrentHeaderClosureQualified':False,'networkCampaignStarted':False,'productionQualified':False}
(D/'result-public.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in['passed','firstFailure','actualDockerExitCode','elapsedMs','heldFilesOpened','heldFilesClosed']}));raise SystemExit(0 if report['passed']else 1)
