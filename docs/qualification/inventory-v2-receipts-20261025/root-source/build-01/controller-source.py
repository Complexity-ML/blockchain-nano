import pathlib,json,hashlib,os,stat,subprocess,tarfile,time,re,shlex,sys
D=pathlib.Path('/home/ubuntu/nano-inventory-v2-receipt-build-root-20261025-01');R=pathlib.Path('/home/ubuntu/blockchain-nano-git');IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';NAME='nano-inventory-v2-receipt-build-20261025-01';LABEL='inventory-v2-receipt-build-20261025-01';DOCKER=['sudo','-n','/usr/bin/docker'];began=time.monotonic();held=[];first=None;errors=[];exitcode=None;closed=0
cfg=json.loads((D/'dispatch-public.json').read_text());BASE=cfg['base'];REV=cfg['sourceCandidate']
def need(v,s):
 if not v:raise ValueError(s)
def run(a,timeout=30,**kw):return subprocess.run(a,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=timeout,check=True,**kw)
def git(*a):return run(['git',*a],cwd=R).stdout.decode().strip()
def mat(s):return [str(getattr(s,k))for k in ('st_dev','st_ino','st_mode','st_uid','st_gid','st_nlink','st_size','st_mtime_ns','st_ctime_ns')]
def digest(fd,size):
 h=hashlib.sha256();at=0
 while at<size:
  b=os.pread(fd,min(1048576,size-at),at);need(b,'short source read');at+=len(b);h.update(b)
 need(not os.pread(fd,1,at),'source grew');return h.hexdigest()
def pin(p,expected=None):
 p=pathlib.Path(p).resolve(strict=True);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);need(stat.S_ISREG(s.st_mode)and s.st_nlink==1 and 0<=s.st_size<=134217728,'bounded source regular file');m=mat(s);sha=digest(fd,s.st_size);need(mat(os.fstat(fd))==m==mat(p.lstat()),'source changed')
  if expected:need(s.st_size==expected['bytes']and sha==expected['sha256'],'known source/provider bytes')
  held.append((p,fd,m,sha));return {'path':str(p),'sha256':sha,'bytes':s.st_size}
 except BaseException:os.close(fd);raise
try:
 need(git('rev-parse','HEAD')==BASE and not git('status','--porcelain'),'current VPS main clean')
 bundle=D/'candidate.bundle';pin(bundle,cfg['bundle']);run(['git','bundle','verify',str(bundle)],cwd=R);run(['git','fetch','--no-tags',str(bundle),cfg['bundleRef']],cwd=R);need(git('rev-parse','FETCH_HEAD')==REV,'candidate fetched through Git')
 source=D/'source';source.mkdir(mode=0o700);archive=D/'source.tar';run(['git','archive','--format=tar','--output',str(archive),REV,'include','third_party','tests/inventory_v2_execution_runner.cpp'],cwd=R)
 with tarfile.open(archive)as t:
  rows=t.getmembers();need(len(rows)<4000 and sum(r.size for r in rows)<67108864,'bounded public source archive')
  for r in rows:
   p=pathlib.PurePosixPath(r.name);need(not p.is_absolute()and '..'not in p.parts and(r.isfile()or r.isdir()),'regular relative source archive')
  t.extractall(source,members=rows,filter='data')
 for p in sorted(source.rglob('*')):
  if p.is_file():pin(p,cfg['sourceCpp']if p.name=='inventory_v2_execution_runner.cpp'else None)
 core=pathlib.Path('/home/ubuntu/cli-readonly-amd-root-20261018-01/outputs/libnano_core.a');pin(core,cfg['core']);mounts=['--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(core)+',dst=/core/libnano_core.a,readonly']
 for row in cfg['providers']:
  need(row['matchesRetainedSourceControl']and row['retainedExpectedHashes']==[row['sha256']],'original tail control binding');pin(row['hostPath'],row);mounts+=['--mount','type=bind,src='+row['hostPath']+',dst='+row['guestPath']+',readonly']
 out=D/'outputs';out.mkdir(mode=0o700);mounts+=['--mount','type=bind,src='+str(out)+',dst=/out']
 flags=['-DMCL_BINT_ASM_X64=0','-DMCL_DONT_USE_XBYAK','-DMCL_FP_BIT=384','-DMCL_FR_BIT=256','-DMCL_MSM=0','-DMCL_PORTABLE=1','-DMCL_USE_LLVM=1']
 includes=['/work/include','/work/third_party','/work/third_party/ckzg/src','/work/third_party/ckzg/blst/bindings','/work/third_party/ckzg/include','/work/third_party/secp256k1/include','/work/third_party/mcl/include']
 cpp=['/usr/bin/clang++',*flags,*['-I'+x for x in includes],'-O3','-DNDEBUG','-std=gnu++20','-Wall','-Wextra','-Wpedantic','-MD','-MF','/out/native-v2.o.d','-o','/out/native-v2.o','-c','/work/tests/inventory_v2_execution_runner.cpp']
 tail=['/old/third_party/mcl/bint64.o','/old/third_party/mcl/base64.o','/core/libnano_core.a','/old/third_party/ckzg/libnano_ckzg.a','/old/third_party/ckzg/libnano_blst.a','/old/third_party/secp256k1/lib/libsecp256k1.a','/usr/lib/x86_64-linux-gnu/libgmpxx.so','/usr/lib/x86_64-linux-gnu/libgmp.so','/usr/lib/x86_64-linux-gnu/libssl.so','/usr/lib/x86_64-linux-gnu/libcrypto.so','/usr/lib/x86_64-linux-gnu/libicuuc.so','-ldl','/usr/lib/x86_64-linux-gnu/librocksdb.so','/old/lib/libmcl.a']
 link=['/usr/bin/clang++','-O3','-DNDEBUG','/out/native-v2.o','-o','/out/nano-inventory-v2-execution-runner',*tail]
 script='set -eu\n'+shlex.join(['/usr/bin/clang++','--version'])+'\n'+shlex.join(cpp)+'\n'+shlex.join(link)+'\n'+shlex.join(['/usr/bin/ldd','/out/nano-inventory-v2-execution-runner'])+'\n'
 (D/'build-script-public.sh').write_text(script);(D/'compile-argv-public.json').write_text(json.dumps(cpp,indent=2)+'\n');(D/'link-argv-public.json').write_text(json.dumps(link,indent=2)+'\n')
 need(not run(DOCKER+['ps','-a','--filter','name=^/'+NAME+'$','--format','{{.ID}}']).stdout.strip(),'new build name absent')
 argv=DOCKER+['run','--rm','--pull=never','--init','--cidfile',str(D/'cid'),'--name',NAME,'--label','nano.qualification='+LABEL,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','2g','--cpus','2','--pids-limit','128','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=128m,mode=1777',*mounts,'--entrypoint','/bin/sh',IMAGE,'-c',script]
 (D/'docker-argv-public.json').write_text(json.dumps(argv,indent=2)+'\n')
 with open(D/'stdout.log','xb')as stdout,open(D/'stderr.log','xb')as stderr:p=subprocess.run(argv,stdout=stdout,stderr=stderr,timeout=480)
 exitcode=p.returncode;need(exitcode==0,'compile/link actual exit zero');need(out.joinpath('nano-inventory-v2-execution-runner').is_file(),'linked test helper')
 need('not found'not in(D/'stdout.log').read_text(),'all linked runtime providers resolved in existing image')
except BaseException as e:first={'type':type(e).__name__,'reason':str(e)[:1000]}
finally:
 try:
  if(D/'cid').exists():
   cid=(D/'cid').read_text().strip();need(re.fullmatch('[0-9a-f]{64}',cid),'exact owned build CID')
   if run(DOCKER+['ps','-a','--filter','id='+cid,'--format','{{.ID}}']).stdout.strip():
    v=json.loads(run(DOCKER+['inspect',cid,'--format','{"Id":{{json .Id}},"Name":{{json .Name}},"Image":{{json .Image}},"Labels":{{json .Config.Labels}}}']).stdout);need(v['Id']==cid and v['Name']=='/'+NAME and v['Image']==IMAGE and v['Labels'].get('nano.qualification')==LABEL,'owned build cleanup only');run(DOCKER+['rm','-f',cid]);raise ValueError('late build cleanup required')
 except BaseException as e:errors.append({'phase':'cleanup','reason':str(e)})
 for p,fd,m,sha in held:
  try:need(mat(os.fstat(fd))==m==mat(p.lstat())and digest(fd,int(m[6]))==sha,'strict final source/provider fence')
  except BaseException as e:errors.append({'phase':'fence','reason':str(e)})
 for _,fd,_,_ in held:
  try:os.close(fd);closed+=1
  except BaseException as e:errors.append({'phase':'close','reason':str(e)})
report={'format':'nano-inventory-v2-native-shim-build-v1','passed':first is None and not errors,'firstFailure':first,'secondaryFailures':errors,'sourceCandidate':REV,'coreArchiveSha256':cfg['core']['sha256'],'staticProviderCount':len(cfg['providers']),'heldSourceAndProviderFiles':len(held),'heldFilesClosed':closed,'actualDockerBuildExitCode':exitcode,'elapsedMs':round((time.monotonic()-began)*1000),'image':IMAGE,'imageBuilds':0,'coreRecompiled':False,'helperCompiled':exitcode==0,'helperExecuted':False,'nativeContractScenarioQualified':False,'productionQualified':False}
for name in('native-v2.o','native-v2.o.d','nano-inventory-v2-execution-runner'):
 p=D/'outputs'/name
 if p.is_file():b=p.read_bytes();report[name]={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
(D/'result-public.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
for n in('stdout.log','stderr.log'):
 if(D/n).exists():print((D/n).read_text()[-6000:])
sys.exit(0 if report['passed']else 1)
