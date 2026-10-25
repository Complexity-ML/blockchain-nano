import hashlib,json,os,pathlib,re,stat,subprocess,sys,tarfile,time
D=pathlib.Path('/home/ubuntu/nano-lifecycle-proposer-budget-root-20261025-02')
R=pathlib.Path('/home/ubuntu/blockchain-nano-git')
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
NAME='nano-lifecycle-proposer-budget-20261025-02';LABEL='lifecycle-proposer-budget-20261025-02'
DOCKER=['sudo','-n','/usr/bin/docker'];held=[];runtime=[];first=None;secondary=[];terminal=None;began=time.monotonic();ran=False;counts=None;closed_count=0
def need(ok,reason):
 if not ok:raise ValueError(reason)
def run(a,**kw):return subprocess.run(a,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30,**kw)
def git(*a):return run(['/usr/bin/git',*a],cwd=R).stdout.decode().strip()
def mat(s):return [str(getattr(s,k))for k in ('st_dev','st_ino','st_mode','st_uid','st_gid','st_nlink','st_size','st_mtime_ns','st_ctime_ns')]
def hash_fd(fd,size):
 h=hashlib.sha256();at=0
 while at<size:
  b=os.pread(fd,min(1048576,size-at),at);need(bool(b),'short held-file read');h.update(b);at+=len(b)
 need(not os.pread(fd,1,at),'held file grew');return h.hexdigest()
def pin_file(p,expected=None):
 p=p.resolve(strict=True);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);need(stat.S_ISREG(s.st_mode)and s.st_nlink==1 and 0<s.st_size<=134217728,'bounded held regular file');m=mat(s);sha=hash_fd(fd,s.st_size)
  need(mat(os.fstat(fd))==m==mat(p.lstat()),'held file changed during first read')
  if expected:need(sha==expected['sha256']and s.st_size==expected['bytes'],'exact source pin')
  held.append((p,fd,m,sha));return {'path':str(p),'sha256':sha,'bytes':s.st_size}
 except BaseException:
  os.close(fd);raise
cfg=json.loads((D/'dispatch-public.json').read_text());REV=cfg['candidateRevision'];BASE=cfg['base']
try:
 need(git('rev-parse','HEAD')==BASE and not git('status','--porcelain'),'VPS main must remain clean at source base')
 bundle=D/'candidate.bundle';raw=bundle.read_bytes();need(len(raw)==cfg['bundle']['bytes']and hashlib.sha256(raw).hexdigest()==cfg['bundle']['sha256'],'known Git bundle bytes')
 run(['/usr/bin/git','bundle','verify',str(bundle)],cwd=R);run(['/usr/bin/git','fetch','--no-tags',str(bundle),cfg['bundleRef']],cwd=R);need(git('rev-parse','FETCH_HEAD')==REV,'candidate fetched by Git')
 archive=D/'candidate-tests.tar'
 roots=cfg['archiveRoots']
 run(['/usr/bin/git','archive','--format=tar','--output',str(archive),REV,*roots],cwd=R)
 source=D/'source';source.mkdir(mode=0o700)
 ancestors={str(p)for root in roots for p in pathlib.PurePosixPath(root).parents if str(p)!='.'}
 with tarfile.open(archive)as t:
  members=t.getmembers();need(len(members)<=1000 and sum(x.size for x in members)<=16777216,'bounded source archive')
  for x in members:
   p=pathlib.PurePosixPath(x.name);allowed=any(x.name==root or x.name.startswith(root+'/')for root in roots)or(x.isdir()and x.name in ancestors)
   need(not p.is_absolute()and '..'not in p.parts and allowed and(x.isfile()or x.isdir()),'regular allowlisted relative Git source members')
  t.extractall(source,members=members,filter='data')
 for name in cfg['testFiles']:need((source/name).is_file(),'declared test file from Git archive')
 # Node validates actual static imports; do not parse synthetic import strings in test fixtures as dependencies.
 for name,pin in cfg['sourcePins'].items():pin_file(source/name,pin)
 node=pathlib.Path('/usr/bin/node');version=run([str(node),'--version']).stdout.decode().strip();need(version=='v18.19.1','reviewed existing Node version')
 trace=run(['/usr/bin/ldd',str(node)]).stdout.decode();need('not found'not in trace,'resolved Node shared libraries')
 guests=['/usr/bin/node']
 for line in trace.splitlines():
  m=re.search(r'(?:=>\s+|^\s*)(/[^\s]+)\s+\(',line)
  if m:guests.append(m.group(1))
 need(len(guests)==len(set(guests))and 2<=len(guests)<=32,'bounded unique Node runtime providers')
 mounts=[]
 for guest in guests:
  p=pin_file(pathlib.Path(guest));runtime.append({'guestPath':guest,**p});mounts+=['--mount','type=bind,src='+p['path']+',dst='+guest+',readonly']
 need(pathlib.Path('/usr/share/nodejs').is_dir(),'existing external Node builtin resources');mounts+=['--mount','type=bind,src=/usr/share/nodejs,dst=/usr/share/nodejs,readonly']

 py=pathlib.Path('/usr/bin/python3');need(run([str(py),'--version']).stdout.decode().strip()=='Python 3.12.3','existing VPS Python3.12.3')
 python_guests=['/usr/bin/python3','/usr/bin/python3.12']
 for line in run(['/usr/bin/ldd',str(py)]).stdout.decode().splitlines():
  match=re.search(r'(?:=>\s+|^\s*)(/[^\s]+)\s+\(',line)
  if match:python_guests.append(match.group(1))
 for guest in dict.fromkeys(python_guests):
  if guest in guests:continue
  row=pin_file(pathlib.Path(guest));runtime.append({'guestPath':guest,**row});mounts+=['--mount','type=bind,src='+row['path']+',dst='+guest+',readonly']
 need(pathlib.Path('/usr/lib/python3.12').is_dir(),'existing Python stdlib');mounts+=['--mount','type=bind,src=/usr/lib/python3.12,dst=/usr/lib/python3.12,readonly']
 need(not run(DOCKER+['ps','-a','--filter','name=^/'+NAME+'$','--format','{{.Names}}']).stdout.strip(),'owned test container name absent')
 files=cfg['testFiles']; need(all(not p.startswith('/') and '..' not in p.split('/') for p in files),'reviewed relative test files')
 argv=DOCKER+['run','--rm','--pull=never','--init','--cidfile',str(D/'cid'),'--name',NAME,'--label','nano.qualification='+LABEL,'--network','none','--env','PYTHONDONTWRITEBYTECODE=1','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','512m','--cpus','1','--pids-limit','128','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=128m,mode=1777',*mounts,'--mount','type=bind,src='+str(source)+',dst=/case,readonly','--entrypoint','/usr/bin/node',IMAGE,'--test','--test-concurrency=1','--test-name-pattern',cfg['testNamePattern'],*['/case/'+x for x in files]]
 (D/'argv.private.json').write_text(json.dumps(argv,indent=2)+'\n');(D/'runtime-private.json').write_text(json.dumps(runtime,indent=2)+'\n')
 ran=True
 with open(D/'stdout.log','xb')as out,open(D/'stderr.log','xb')as err:q=subprocess.run(argv,stdout=out,stderr=err,timeout=240)
 terminal=q.returncode;need(terminal==0,'Docker selected test batch failed')
 tap=(D/'stdout.log').read_text();counts={k:int(re.search(r'^# '+k+r' ([0-9]+)$',tap,re.M).group(1))for k in ('tests','pass','fail','cancelled','skipped')}
 need(counts==cfg['expectedCounts'],'reviewed selected test count required')
 selected=[m.group(1)for m in re.finditer(r'^ok [0-9]+ - (.+)$',tap,re.M)if '# SKIP'not in m.group(1)]
 need(sorted(selected)==sorted(cfg['expectedTestNames']),'exact selected test names')
except BaseException as e:first={'type':type(e).__name__,'reason':str(e)[:1000]}
finally:
 try:
  cidfile=D/'cid'
  if cidfile.exists():
   cid=cidfile.read_text().strip();need(re.fullmatch('[0-9a-f]{64}',cid),'own exact test CID')
   if run(DOCKER+['ps','-a','--filter','id='+cid,'--format','{{.ID}}']).stdout.strip():
    v=json.loads(run(DOCKER+['inspect',cid,'--format','{"Id":{{json .Id}},"Name":{{json .Name}},"Image":{{json .Image}},"Config":{"Labels":{{json .Config.Labels}}}}']).stdout);need(v['Id']==cid and v['Name']=='/'+NAME and v['Image']==IMAGE and v['Config']['Labels'].get('nano.qualification')==LABEL,'own test CID/image/label')
    run(DOCKER+['rm','-f',cid]);raise ValueError('late owned test-container cleanup was required')
 except BaseException as e:
  failure={'phase':'owned-test-cleanup','type':type(e).__name__,'reason':str(e)[:500]}
  if first is None:first=failure
  else:secondary.append(failure)
 for p,fd,m,sha in held:
  try:need(mat(os.fstat(fd))==m==mat(p.lstat())and hash_fd(fd,int(m[6]))==sha,'strict final source/runtime fence')
  except BaseException as e:
   failure={'phase':'held-file-fence','type':type(e).__name__,'reason':str(e)[:500]}
   if first is None:first=failure
   else:secondary.append(failure)
 for _,fd,_,_ in held:
  try:os.close(fd);closed_count+=1
  except BaseException as e:
   failure={'phase':'close-held-file','type':type(e).__name__,'reason':str(e)[:500]}
   if first is None:first=failure
   else:secondary.append(failure)
 try:need(git('rev-parse','HEAD')==BASE and not git('status','--porcelain'),'VPS main unchanged by tests')
 except BaseException as e:
  failure={'phase':'main-unchanged','type':type(e).__name__,'reason':str(e)[:500]}
  if first is None:first=failure
  else:secondary.append(failure)
report={'format':'nano-root-lifecycle-proposer-budget-docker-tests-v1','passed':first is None,'firstFailure':first,'secondaryFailures':secondary,'candidateRevision':REV,'sourceMainUnchanged':BASE,'actualDockerExitCode':terminal,'elapsedMs':round((time.monotonic()-began)*1000),'counts':counts,'heldFilesOpened':len(held),'heldFilesClosed':closed_count,'nodeRuntimeProviderFiles':len(runtime),'runtimeAndSourceFilesStrictlyFenced':first is None,'nodeVersion':'v18.19.1','runtimeMountedReadOnly':True,'nodeSystemResourcesMountedReadOnly':True,'wholeNodeRuntimeClosureQualified':False,'pythonVersion':'3.12.3','pythonBinaryAndSharedProvidersMountedReadOnly':True,'pythonStdlibMountedReadOnly':True,'wholePythonRuntimeClosureQualified':False,'image':IMAGE,'dockerInitEnabled':True,'dockerNetwork':'none','imageBuilds':0,'keysCreated':False,'chainRuntimeStarted':False,'sourceFinalityAuthenticated':False,'economicTermsAdopted':False,'nativeNanoActivityTested':False,'nativeMacFilesystemQualified':False,'threeOwnerRuntimeRecoveryQualified':False,'productionQualified':False}
for name in('stdout.log','stderr.log'):
 p=D/name
 if p.exists():
  b=p.read_bytes();report[name]={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
(D/'result-public.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report,sort_keys=True))
if (D/'stdout.log').exists():print((D/'stdout.log').read_text()[-12000:])
if (D/'stderr.log').exists():print((D/'stderr.log').read_text()[:1000])
sys.exit(0 if report['passed']else 1)
