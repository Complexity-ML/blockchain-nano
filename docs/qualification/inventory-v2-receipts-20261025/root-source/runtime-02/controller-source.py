import pathlib,json,hashlib,os,stat,subprocess,tarfile,time,re,sys
D=pathlib.Path('/home/ubuntu/nano-inventory-v2-receipt-runtime-root-20261025-02');R=pathlib.Path('/home/ubuntu/blockchain-nano-git');IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';NAME='nano-inventory-v2-receipt-runtime-20261025-02';LABEL='inventory-v2-receipt-runtime-20261025-02';DOCKER=['sudo','-n','/usr/bin/docker'];held=[];first=None;errors=[];closed=0;exitcode=None;began=time.monotonic();cfg=json.loads((D/'dispatch-public.json').read_text());BASE=cfg['base'];REV=cfg['sourceCandidate']
def need(v,s):
 if not v:raise ValueError(s)
def run(a,timeout=30,**kw):return subprocess.run(a,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=timeout,check=True,**kw)
def git(*a):return run(['git',*a],cwd=R).stdout.decode().strip()
def mat(s):return [str(getattr(s,k))for k in ('st_dev','st_ino','st_mode','st_uid','st_gid','st_nlink','st_size','st_mtime_ns','st_ctime_ns')]
def hashfd(fd,size):
 h=hashlib.sha256();at=0
 while at<size:
  b=os.pread(fd,min(1048576,size-at),at);need(b,'short source');h.update(b);at+=len(b)
 need(not os.pread(fd,1,at),'source grew');return h.hexdigest()
def pin(p,expected=None):
 p=pathlib.Path(p).resolve(strict=True);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<=a.st_size<=134217728,'bounded regular input');m=mat(a);s=hashfd(fd,a.st_size);need(mat(os.fstat(fd))==m==mat(p.lstat()),'first input fence')
  if expected:need(s==expected['sha256']and a.st_size==expected['bytes'],'known source or executable')
  held.append((p,fd,m,s));return {'path':str(p),'sha256':s,'bytes':a.st_size}
 except BaseException:os.close(fd);raise
try:
 need(git('rev-parse','HEAD')==BASE and not git('status','--porcelain'),'VPS main clean and unchanged')
 bundle=pin(D/'candidate.bundle',cfg['bundle']);run(['git','bundle','verify',bundle['path']],cwd=R);run(['git','fetch','--no-tags',bundle['path'],cfg['bundleRef']],cwd=R);need(git('rev-parse','FETCH_HEAD')==REV,'corrected receipt driver fetched through Git')
 for part in ('include','third_party','tests/inventory_v2_execution_runner.cpp'):need(git('rev-parse',REV+':'+part)==git('rev-parse',cfg['helperBuildCandidate']+':'+part),'helper source and all archived headers/providers unchanged; no rebuild')
 source=D/'source';source.mkdir(mode=0o700);archive=D/'source.tar';roots=['tests/hardhat/inventory-v2/native/receipt-coverage.mjs','tests/multihost/campaign-deadline.mjs','tests/multihost/campaign-commands.mjs'];run(['git','archive','--format=tar','--output',str(archive),REV,*roots],cwd=R)
 with tarfile.open(archive)as t:
  rows=t.getmembers();need(len(rows)<30 and sum(x.size for x in rows)<262144,'bounded public source archive')
  for x in rows:
   p=pathlib.PurePosixPath(x.name);need(not p.is_absolute()and '..'not in p.parts and(x.isfile()or x.isdir()),'public regular source')
  t.extractall(source,members=rows,filter='data')
 for p in sorted(source.rglob('*')):
  if p.is_file():pin(p,cfg['rawDriver']if p.name=='receipt-coverage.mjs'else None)
 build=pathlib.Path('/home/ubuntu/nano-inventory-v2-receipt-build-root-20261025-01');bpin=pin(build/'result-public.json',cfg['buildResult']);b=json.loads((build/'result-public.json').read_text());need(b['passed']and b['sourceCandidate']==cfg['helperBuildCandidate'] and b['actualDockerBuildExitCode']==0 and b['coreArchiveSha256']==cfg['core']['sha256']and b['heldSourceAndProviderFiles']==b['heldFilesClosed']==836 and not b['coreRecompiled'],'actual helper build bound to unchanged Core and original providers')
 helper=pin(build/'outputs/nano-inventory-v2-execution-runner',cfg['helper']);native=pin('/home/ubuntu/nano-inventory-v2-native-runtime-root-20261025-01/outputs/native-run/native-accounting.json',cfg['nativeAccounting'])
 troot=pathlib.Path('/home/ubuntu/nano-inventory-v2-t8n-reference-root-20261025-01');trace=pin(troot/'outputs/result/t8n-reference-trace.json',cfg['referenceTrace']);adoption=pin('/home/ubuntu/nano-inventory-v2-t8n-reference-metadata-root-20261025-01/reference-adoption-public.json',cfg['referenceAdoption']);rootResult=pin(troot/'result-public.json',cfg['referenceRootResult'])
 node=pathlib.Path('/usr/bin/node');need(run([str(node),'--version']).stdout.decode().strip()=='v18.19.1','existing Node18.19.1');guests=['/usr/bin/node'];ldd=run(['ldd',str(node)]).stdout.decode();need('not found'not in ldd,'Node providers resolved');guests+=[w for w in ldd.split()if w.startswith('/')];need(len(guests)==len(set(guests))and len(guests)<=32,'bounded providers');mounts=[]
 for guest in guests:
  p=pin(pathlib.Path(guest));mounts+=['--mount','type=bind,src='+p['path']+',dst='+guest+',readonly']
 out=D/'outputs';out.mkdir(mode=0o700)
 for host,guest in [('/usr/share/nodejs','/usr/share/nodejs'),('/home/ubuntu/nano-public-test-dependencies-root-20261025-01/deps/hardhat/node_modules','/deps/hardhat/node_modules'),(str(source),'/case'),(helper['path'],'/bin/native-receipts'),(native['path'],'/inputs/native-accounting.json'),(trace['path'],'/inputs/reference-trace.json'),(adoption['path'],'/inputs/reference-adoption.json'),(rootResult['path'],'/inputs/root-reference-result.json'),(str(troot/'outputs/result'),'/reference')]:mounts+=['--mount','type=bind,src='+host+',dst='+guest+',readonly']
 mounts+=['--mount','type=bind,src='+str(out)+',dst=/out']
 need(not run(DOCKER+['ps','-a','--filter','name=^/'+NAME+'$','--format','{{.ID}}']).stdout.strip(),'new owned receipt runtime absent')
 argv=DOCKER+['run','--rm','--pull=never','--init','--cidfile',str(D/'cid'),'--name',NAME,'--label','nano.qualification='+LABEL,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','2g','--cpus','2','--pids-limit','128','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=128m,mode=1777',*mounts,'--entrypoint','/usr/bin/node',IMAGE,'/case/tests/hardhat/inventory-v2/native/receipt-coverage.mjs','/out/result','/deps/hardhat/node_modules','/case','/bin/native-receipts',helper['sha256'],'/inputs/native-accounting.json',native['sha256'],'/reference','/inputs/reference-trace.json',trace['sha256'],'/inputs/reference-adoption.json',adoption['sha256'],'/inputs/root-reference-result.json']
 (D/'argv-public.json').write_text(json.dumps(argv,indent=2)+'\n')
 with open(D/'stdout.log','xb')as stdout,open(D/'stderr.log','xb')as stderr:p=subprocess.run(argv,stdout=stdout,stderr=stderr,timeout=600)
 exitcode=p.returncode;need(exitcode==0,'actual receipt Docker/driver exit zero');report=json.loads((out/'result/receipt-coverage-controller.json').read_text());terminal=json.loads((D/'stdout.log').read_text().splitlines()[-1]);rows=json.loads((out/'result/receipt-coverage-trace.json').read_text())
 need(report['terminalEvidenceReady']and report['failure']is None and report['transactions']==report['strictMatches']==65 and report['firstMismatch']is None and report['requests']==132 and report['ownedChildGroupJoined']and report['childExit']==report['childClose']=={'code':0,'signal':None},'all65 exact receipts plus actual child EOF/exit/close/group join')
 need(terminal['terminalEvidenceReady']and terminal['failure']is None and terminal['openedDescriptors']==terminal['closedDescriptors'],'actual final FD publication closure');need(len(rows)==65 and all(all(r['checks'].values())for r in rows),'every receipt/raw/accounting check')
except BaseException as e:first={'type':type(e).__name__,'reason':str(e)[:1000]}
finally:
 try:
  if(D/'cid').exists():
   cid=(D/'cid').read_text().strip();need(re.fullmatch('[0-9a-f]{64}',cid),'exact own differential CID')
   if run(DOCKER+['ps','-a','--filter','id='+cid,'--format','{{.ID}}']).stdout.strip():
    v=json.loads(run(DOCKER+['inspect',cid,'--format','{"Id":{{json .Id}},"Name":{{json .Name}},"Image":{{json .Image}},"Labels":{{json .Config.Labels}}}']).stdout);need(v['Id']==cid and v['Name']=='/'+NAME and v['Image']==IMAGE and v['Labels'].get('nano.qualification')==LABEL,'owned cleanup only');run(DOCKER+['rm','-f',cid]);raise ValueError('late differential-container cleanup required')
 except BaseException as e:errors.append({'phase':'cleanup','reason':str(e)})
 for p,fd,m,sha in held:
  try:need(mat(os.fstat(fd))==m==mat(p.lstat())and hashfd(fd,int(m[6]))==sha,'strict final differential inputs fence')
  except BaseException as e:errors.append({'phase':'fence','reason':str(e)})
 for _,fd,_,_ in held:
  try:os.close(fd);closed+=1
  except BaseException as e:errors.append({'phase':'close','reason':str(e)})

report={'format':'nano-root-inventory-v2-receipt-coverage-v1','passed':first is None and not errors,'firstFailure':first,'secondaryFailures':errors,'actualDockerExitCode':exitcode,'sourceCandidate':REV,'elapsedMs':round((time.monotonic()-began)*1000),'knownInputFDsOpened':len(held),'knownInputFDsClosed':closed,'image':IMAGE,'imageBuilds':0,'coreCompiled':False,'helperCompiledInSeparateBuild':True,'nativeTransactionsReplayed':65 if exitcode==0 else None,'referenceTransactionsReplayed':0,'solidityScenarioReplayed':False,'referenceVersion':'Geth1.16.7 retained','helperSha256':cfg['helper']['sha256'],'consensusQualified':False,'liveNetworkQualified':False,'economicTermsAdopted':False,'productionQualified':False}
(D/'result-public.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
for name in('stdout.log','stderr.log'):
 if(D/name).exists():print((D/name).read_text()[-3000:])
if(D/'outputs/result/receipt-coverage-controller.json').exists():print((D/'outputs/result/receipt-coverage-controller.json').read_text()[-3000:])
sys.exit(0 if report['passed']else 1)
