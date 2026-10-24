import hashlib,json,os,pathlib,re,stat,subprocess,sys,tarfile,time
D=pathlib.Path('/home/ubuntu/nano-c8-runtime-controls-test-root-20261024-01')
R=pathlib.Path('/home/ubuntu/blockchain-nano-git')
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
NAME='nano-c8-runtime-controls-test-20261024-01';LABEL='c8-runtime-controls-test-20261024-01'
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
 run(['/usr/bin/git','bundle','verify',str(bundle)],cwd=R);run(['/usr/bin/git','fetch','--no-tags',str(bundle),'HEAD'],cwd=R);need(git('rev-parse','FETCH_HEAD')==REV,'candidate fetched by Git')
 archive=D/'candidate-tests.tar'
 docs='docs/qualification/local-supervisor-linux-20261015/source/'
 roots=['tests/multihost','tests/capella','tests/assertoor/stability.mjs','scripts/operations',docs+'known-files.pl',docs+'copy-bootstrap-pairs.pl']
 run(['/usr/bin/git','archive','--format=tar','--output',str(archive),REV,*roots],cwd=R)
 source=D/'source';source.mkdir(mode=0o700)
 ancestors={str(p)for root in roots for p in pathlib.PurePosixPath(root).parents if str(p)!='.'}
 with tarfile.open(archive)as t:
  members=t.getmembers();need(len(members)<=1000 and sum(x.size for x in members)<=16777216,'bounded source archive')
  for x in members:
   p=pathlib.PurePosixPath(x.name);allowed=any(x.name==root or x.name.startswith(root+'/')for root in roots)or(x.isdir()and x.name in ancestors)
   need(not p.is_absolute()and '..'not in p.parts and allowed and(x.isfile()or x.isdir()),'regular allowlisted relative Git source members')
  t.extractall(source,members=members,filter='data')
 pending=[source/'tests/multihost/run.mjs',source/'tests/multihost/observe.mjs',source/'scripts/operations/monitor-and-deliver.mjs'];closure=set()
 while pending:
  module=pending.pop();need(module.is_relative_to(source),'relative source containment')
  if module in closure:continue
  need(module.is_file(),'relative import missing');closure.add(module)
  for specifier in re.findall(r"(?:from\s*|import\s*)['\"]([^'\"]+)['\"]",module.read_text()):
   if specifier.startswith('.'):pending.append((module.parent/specifier).resolve())
 (D/'import-closure-private.json').write_text(json.dumps(sorted(str(x.relative_to(source))for x in closure),indent=2)+'\n')
 for name,pin in cfg['sourcePins'].items():pin_file(source/name,pin)
 for leaf,expected in {'known-files.pl':'7dd88b7d99ab15e9f2b1e52aac796265e6ed9bed24323e66584b3f31138cc0f6','copy-bootstrap-pairs.pl':'9fcd47d3bee3530c163e00e8b6a8dd475645336ba48579db45bdc9e20bb3bdf3'}.items():
  pin=pin_file(source/docs/leaf);need(pin['sha256']==expected,'frozen guard source helper')
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
 need(not run(DOCKER+['ps','-a','--filter','name=^/'+NAME+'$','--format','{{.Names}}']).stdout.strip(),'owned test container name absent')
 files=cfg['testFiles']; need(all(not p.startswith('/') and '..' not in p.split('/') for p in files),'reviewed relative test files')
 argv=DOCKER+['run','--rm','--pull=never','--init','--cidfile',str(D/'cid'),'--name',NAME,'--label','nano.qualification='+LABEL,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','512m','--cpus','1','--pids-limit','128','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=128m,mode=1777',*mounts,'--mount','type=bind,src='+str(source)+',dst=/case,readonly','--entrypoint','/usr/bin/node',IMAGE,'--test','--test-concurrency=1','--test-name-pattern='+cfg['testPattern'],*['/case/'+x for x in files]]
 (D/'argv.private.json').write_text(json.dumps(argv,indent=2)+'\n');(D/'runtime-private.json').write_text(json.dumps(runtime,indent=2)+'\n')
 ran=True
 with open(D/'stdout.log','xb')as out,open(D/'stderr.log','xb')as err:q=subprocess.run(argv,stdout=out,stderr=err,timeout=150)
 terminal=q.returncode;need(terminal==0,'Docker selected test batch failed')
 tap=(D/'stdout.log').read_text();counts={k:int(re.search(r'^# '+k+r' ([0-9]+)$',tap,re.M).group(1))for k in ('tests','pass','fail','cancelled','skipped')}
 need(counts==cfg['expectedCounts'],'reviewed selected test count required')
 selected=[m.group(1)for m in re.finditer(r'^ok [0-9]+ - (.+)$',tap,re.M)if '# SKIP'not in m.group(1)]
 need(sorted(selected)==sorted(cfg['expectedTestNames']),'exact selected test names')
 integrationSource=source/'tests/multihost/docker-runtime-restart-guard.integration.mjs';pin_file(integrationSource,cfg['sourcePins']['tests/multihost/docker-runtime-restart-guard.integration.mjs'])
 with open(D/'integration.stdout.log','xb')as out,open(D/'integration.stderr.log','xb')as err:integration=subprocess.run(['/usr/bin/node',str(integrationSource),IMAGE,REV,str(D/'daemon-policy-proof')],stdout=out,stderr=err,timeout=125,env={'PATH':'/usr/bin:/bin','LANG':'C','LC_ALL':'C'})
 need(integration.returncode==0 and (D/'integration.stderr.log').stat().st_size==0,'real Docker policy integration terminal')
 integrationResult=json.loads((D/'daemon-policy-proof/result-public.json').read_text());need(integrationResult['passed'] and integrationResult['allOwnContainersRemoved'],'real Docker policy positive/negative cases and cleanup')
except BaseException as e:first={'type':type(e).__name__,'reason':str(e)[:1000]}
finally:
 try:
  cidfile=D/'cid'
  if cidfile.exists():
   cid=cidfile.read_text().strip();need(re.fullmatch('[0-9a-f]{64}',cid),'own exact test CID')
   if run(DOCKER+['ps','-a','--filter','id='+cid,'--format','{{.ID}}']).stdout.strip():
    v=json.loads(run(DOCKER+['inspect',cid,'--format','{{json .}}']).stdout);need(v['Id']==cid and v['Name']=='/'+NAME and v['Image']==IMAGE and v['Config']['Labels'].get('nano.qualification')==LABEL,'own test CID/image/label')
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
report={'format':'nano-root-c8-runtime-controls-docker-tests-v1','passed':first is None,'firstFailure':first,'secondaryFailures':secondary,'candidateRevision':REV,'sourceMainUnchanged':BASE,'actualDockerExitCode':terminal,'elapsedMs':round((time.monotonic()-began)*1000),'counts':counts,'heldFilesOpened':len(held),'heldFilesClosed':closed_count,'nodeRuntimeProviderFiles':len(runtime),'runtimeAndSourceFilesStrictlyFenced':first is None,'nodeVersion':'v18.19.1','runtimeMountedReadOnly':True,'nodeSystemResourcesMountedReadOnly':True,'wholeNodeRuntimeClosureQualified':False,'image':IMAGE,'dockerInitEnabled':True,'dockerNetwork':'none','imageBuilds':0,'keysCreated':False,'chainRuntimeStarted':False,'realDockerPolicyIntegration':locals().get('integrationResult',None),'nativeMacFilesystemQualified':False,'threeOwnerRuntimeRecoveryQualified':False,'productionQualified':False}
for name in('stdout.log','stderr.log'):
 p=D/name
 if p.exists():
  b=p.read_bytes();report[name]={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
(D/'result-public.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report,sort_keys=True))
if (D/'stdout.log').exists():print((D/'stdout.log').read_text()[-12000:])
if (D/'stderr.log').exists():print((D/'stderr.log').read_text()[:1000])
sys.exit(0 if report['passed']else 1)
