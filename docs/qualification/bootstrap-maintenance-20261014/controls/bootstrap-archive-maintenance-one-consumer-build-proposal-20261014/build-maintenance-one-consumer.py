#!/usr/bin/env python3
"""ROOT only: one repaired maintenance-test CPP/link, unchanged archive/CLI/shim.
No CMake, core compilation, archive commands or tests. Completed parents preserved.
"""
import argparse,hashlib,json,os,pathlib,posixpath,re,shlex,stat,subprocess,sys,time,types,uuid
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
PARENT='cb9acac75470ff0ab8627060e8c7de9db7f2312f30d252ed9f01ec82c06731c2'
HARNESS='dae95f972b019595e295e8a14c7afeca8773c84b232234cd8aee7773c76c5da0'
OLD='f7c74c1b3f324843d12e2db4395e0c55b15dae69f94f60a93d97dcbe07765a63';NEW='e5f664121c238c1db2e94283fa96d9959ecabcd566028dcdd5deb0df0f9ce1ee'
SRC='tests/test_bootstrap_archive_maintenance.cpp';TARGET='nano-bootstrap-archive-maintenance-tests'
PROGRAMS={'nano-bootstrap-archive-maintenance',TARGET,'libnano-bootstrap-maintenance-retention-fault.so'}
TARGETS=['nano-bootstrap-archive-maintenance',TARGET,'nano-bootstrap-maintenance-retention-fault']
GRAPHS=['build/CTestTestfile.cmake','build/third_party/secp256k1/CTestTestfile.cmake','build/third_party/secp256k1/src/CTestTestfile.cmake']
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(s):
 p=pathlib.Path(s);need(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def main():
 ap=argparse.ArgumentParser()
 for k in ('controller-sha','guard','guard-sha','parent-plan','parent-plan-sha','parent-terminal','parent-terminal-sha','parent-metadata','parent-metadata-sha','harness','harness-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();need(all(re.fullmatch('[0-9a-f]{64}',getattr(a,k)) for k in ('controller_sha','guard_sha','parent_plan_sha','parent_terminal_sha','parent_metadata_sha','harness_sha')),'literal external SHA pins');selfp=canonical(__file__);need(sha(selfp.read_bytes())==a.controller_sha,'controller SHA');gp=canonical(a.guard);gb=gp.read_bytes();need(a.guard_sha==GUARD and sha(gb)==GUARD,'qualified guard');m=types.ModuleType('one_maintenance_consumer_guards');m.__file__=str(gp);exec(compile(gb,str(gp),'exec'),m.__dict__)
 def control(path,h):
  b,s=m.observed(canonical(path),'arm64',16*1024*1024,True);need(sha(b)==h,'external public input SHA');return json.loads(b)
 p=control(a.parent_plan,a.parent_plan_sha);arch=p['arch'];t=control(a.parent_terminal,a.parent_terminal_sha);meta=control(a.parent_metadata,a.parent_metadata_sha);h=control(a.harness,a.harness_sha)
 need(arch in m.IMAGES and p['image']==m.IMAGES[arch] and p['baselineGitCommit']==m.BASE and p['status']=='READY_ROOT_DISPATCH_ONLY' and p['format']=='nano-bootstrap-owner-runtime-incremental-plan-v1','same existing image and parent policy')
 need(t['format']=='nano-bootstrap-maintenance-three-target-build-terminal-v1' and t['status']=='BUILD_PASS_MAINTENANCE_TEST_NOT_EXECUTED' and t['arch']==arch and t['controllerSha256']==PARENT and t['guardSha256']==GUARD and t['parentPlanSha256']==a.parent_plan_sha and t['sourceFiles']==1320 and t['archiveMembers']==142 and len(t['objects'])==3 and set(t['programs'])==PROGRAMS and t['newObjectsCompiled']==t['newLinks']==3 and t['coreObjectsCompiled']==t['coreObjectsCopied']==t['testsExecuted']==0 and t['allActualMdEqualPreflight'] and t['sourceSdkLibrariesGeneratedAndParentsUnchanged'],'actual original maintenance three-target build')
 need(a.harness_sha==HARNESS and h['sourceChanged']==[SRC] and h['newCoreUnits']==0 and h['productionToolChanged'] is False and h['priorProposal']['sha256']==t['harnessProposalSha256']=='5c83d5e31803ddbdb6ea3026ec99f7ed460cf1c6db6e7f61f6c9bd77e1a6deba','only frozen harness repair')
 parent=canonical(a.parent_terminal).parent.parent;need(parent.name=='run-01' and canonical(a.parent_metadata)==parent/'root-build-public-bindings.json','same externally sealed actual parent run');pb=canonical(parent/'build');origin=canonical(parent/'source');root=canonical(a.output);need(not root.exists() and root.parent.is_dir() and not any(overlap(root,x) for x in (parent,canonical(a.parent_plan).parent,canonical(a.harness).parent,selfp.parent)),'fresh isolated correction output');need(os.geteuid()==p['uid'] and os.getegid()==p['gid'] and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'public owner/environment');protected={};diagnostics=[]
 def pin(path,hv=None,n=None,old=None):
  path=canonical(path);s=m.observed(path,arch);need((hv is None or s['sha256']==hv) and (n is None or s['st_size']==n),'public pinned bytes')
  if old:need(s['sha256']==old['sha256'] and all(s[k]==old[k] for k in m.material(arch)),'actual parent material')
  protected[str(path)]=s;return s
 def read(path):
  b,s=m.observed(canonical(path),arch,16*1024*1024,True);pin(path,s['sha256'],s['st_size']);return b
 def same(path,s):
  z=m.observed(path,arch);need(z['sha256']==s['sha256'] and all(z[k]==s[k] for k in m.material(arch)),'protected material readback')
  if z['st_ctime_ns']!=s['st_ctime_ns']:diagnostics.append({'path':str(path),'old':s['st_ctime_ns'],'new':z['st_ctime_ns'],'bytesAndMaterialEqual':True,'causeMeasured':False})
 for path,hv in ((selfp,a.controller_sha),(gp,GUARD),(a.parent_plan,a.parent_plan_sha),(a.parent_terminal,a.parent_terminal_sha),(a.parent_metadata,a.parent_metadata_sha),(a.harness,HARNESS)):pin(path,hv)
 allow={'receipts/source-before.json','receipts/sdk-binding.json','receipts/sdk-before.log','receipts/sdk-after.log','receipts/archive-142-actual-byte-proof.json','receipts/image-inspect.log','build/compile_commands.json',*GRAPHS};allow.update('build/CMakeFiles/'+x+'.dir/'+leaf for x in TARGETS for leaf in ('flags.make','link.txt'))
 for obj in t['objects']:m.relative(obj);allow.update({'build/'+obj+'.d','build/preflight/'+obj+'.d'})
 need(set(meta)=={'format','terminalSha256','buildControllerSha256','parentPlanSha256','files'} and meta['format']=='nano-bootstrap-maintenance-public-build-bindings-v1' and meta['terminalSha256']==a.parent_terminal_sha and meta['buildControllerSha256']==PARENT and meta['parentPlanSha256']==a.parent_plan_sha and len(meta['files'])==len(allow)==22 and {r['path'] for r in meta['files']}==allow,'exact external parent22 metadata authority');bound={}
 for r in meta['files']:
  need(set(r)=={'path','sha256','bytes'} and re.fullmatch('[0-9a-f]{64}',r['sha256']) and type(r['bytes']) is int and r['bytes']>0,'metadata row');n=m.relative(r['path']);b,s=m.observed(parent/n,arch,16*1024*1024,True);need(s['sha256']==r['sha256'] and s['st_size']==r['bytes'],'authenticated same metadata buffer');protected[str(parent/n)]=s;bound[n]=b
 oldsource=json.loads(bound['receipts/source-before.json']);need(len(oldsource)==1320 and oldsource[SRC]['sha256']==OLD,'actual original test source and full view')
 for n,s in oldsource.items():m.relative(n);pin(origin/n,s['sha256'],s['st_size'],s)
 newrow=next(r for r in h['files'] if r['path']=='source/'+SRC);newfile=canonical(a.harness).parent/newrow['path'];need(newrow['sha256']==NEW and pin(newfile,NEW,newrow['bytes'])['st_size']==30364,'specific canonical-state test repair')
 for n,s in {**t['objects'],**t['programs'],'libnano_core.a':t['archive']}.items():pin(pb/n,s['sha256'],s['st_size'],s)
 for table in ('libraries','generated'):
  for n,r in p[table].items():m.relative(n);pin(pb/n,r['sha256'],r['bytes'])
 proof=json.loads(bound['receipts/archive-142-actual-byte-proof.json']);need(proof['archiveBeforeProof']['sha256']==t['archive']['sha256'] and proof['archiveCreatedOrModified'] is False and len(proof['members'])==142 and len({r['object'] for r in proof['members']})==len({r['member'] for r in proof['members']})==142,'actual142 member-byte proof reused only through immutable archive SHA')
 cc=json.loads(bound['build/compile_commands.json']);selected=[r for r in cc if r['file']=='/work/'+SRC and m.unit(r)[0].startswith('CMakeFiles/'+TARGET+'.dir/')];need(len(selected)==1,'one existing consumer compiler command');obj,argv=m.unit(selected[0]);olddeps=m.dep(bound['build/'+obj+'.d']);need(obj in t['objects'] and olddeps==m.dep(bound['build/preflight/'+obj+'.d']),'actual original consumer dependency closure');link=shlex.split(bound['build/CMakeFiles/'+TARGET+'.dir/link.txt'].decode());need(link[0]=='/usr/bin/clang++' and link.count('-o')==1 and link[link.index('-o')+1]==TARGET and link.count('libnano_core.a')==1 and obj in link and all(x in {obj,*p['libraries']} for x in link if x.endswith('.o')),'exact unchanged consumer link')
 sb=bound['receipts/sdk-before.log'];binding=json.loads(bound['receipts/sdk-binding.json']);need(sb==bound['receipts/sdk-after.log'] and sha(sb)==binding['beforeSha256']==binding['afterSha256'] and binding['equal'] is True,'actual parent SDK');sdklines=[l.split('  ',1) for l in sb.decode().splitlines()];sdk={n:hv for hv,n in sdklines};need(len(sdk)==len(sdklines)==binding['paths'] and all(sdk.get(n)==hv for n,hv in p['sdkHashes'].items()),'same SDK provenance')
 os.mkdir(root,0o700);source=root/'source';build=root/'build';controls=root/'controls';receipts=root/'receipts'
 for d in (source,build,controls,receipts):d.mkdir(mode=0o700)
 m.save(receipts/'initial-empty-output.json',{'sourceEmpty':not any(source.iterdir()),'buildEmpty':not any(build.iterdir()),'beforeAnyCopy':True});(build/'tmp').mkdir(mode=0o700);before={};copies={};workers={};failure=None;cpp=False;linked=False;start=time.monotonic();env={'PATH':'/usr/bin:/bin','LC_ALL':'C'}
 def cp(path,dst):
  b,s=m.observed(canonical(path),arch,None,True);expected=protected[str(path)];need(s['sha256']==expected['sha256'] and all(s[k]==expected[k] for k in m.material(arch)),'exact copied buffer material/pin');dst.parent.mkdir(parents=True,exist_ok=True);fd=os.open(dst,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,stat.S_IMODE(s['st_mode']))
  with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
  z=m.observed(dst,arch);need(z['sha256']==s['sha256'] and z['st_size']==s['st_size'],'byte-identical fresh copy');return z
 def run(label,args,seconds):
  need(time.monotonic()-start<900,'finite900s batch');token=uuid.uuid4().hex;name='nano-maint-one-build-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name];cmd=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.maintenance.one.build='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/outputs/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs','--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',args[0],p['image']]+args[1:];m.save(receipts/(label+'.command.json'),{'argv':cmd,'timeoutSeconds':seconds});phase=time.monotonic()
  try:
   with (receipts/(label+'.log')).open('xb',buffering=0) as log:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,900-(time.monotonic()-start))))
   m.save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-phase});need(r.returncode==0,'phase refused '+label);print(json.dumps({'phase':label,'elapsedSeconds':time.monotonic()-start}),flush=True);return (receipts/(label+'.log')).read_bytes()
  except subprocess.TimeoutExpired:m.save(receipts/(label+'.exit.json'),{'exitCode':None,'timedOut':True});raise
  finally:
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if z.returncode==0:need(json.loads(z.stdout).get('nano.maintenance.one.build')==token,'owned cleanup');need(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'cleanup observation')
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'owned container remains')
 try:
  for n in oldsource:before[n]=cp(newfile if n==SRC else origin/n,source/n)
  need(len(before)==1320 and {n for n in before if before[n]['sha256']!=oldsource[n]['sha256']}=={SRC},'one source-body repair only');m.save(receipts/'source-before.json',before);m.save(receipts/'parents-before.json',protected)
  copied_metadata=[n for n in allow if n.startswith('build/') and n not in {'build/'+obj+'.d','build/preflight/'+obj+'.d'}]
  for n in copied_metadata:copies[n[6:]]=cp(parent/n,build/n[6:])
  for n in ['libnano_core.a',*p['libraries'],*p['generated'],*[n for n in t['objects'] if n!=obj],*[n for n in t['programs'] if n!=TARGET]]:copies[n]=cp(pb/n,build/n)
  for n in ('receipts/archive-142-actual-byte-proof.json','receipts/image-inspect.log'):copies[n]=cp(parent/n,root/n)
  for n in ('third_party/ckzg','third_party/mcl'):need((pb/n).is_dir(),'actual optional CTest directory');(build/n).mkdir(parents=True,exist_ok=True)
  (build/'preflight'/(obj+'.d')).parent.mkdir(parents=True,exist_ok=True);run('dependency-preflight',m.dependency_argv(argv,obj,True),90);actual=m.dep((build/'preflight'/(obj+'.d')).read_bytes());need(actual==olddeps,'same actual dependency closure after source-only fix')
  for d in actual:
   if d.startswith('/work/'):need(m.relative(posixpath.normpath(d[6:])) in before,'project closure')
   elif d.startswith('/outputs/'):need(m.relative(posixpath.normpath(d[9:])) in copies,'generated closure')
   else:need(d in sdk,'SDK closure')
  sdkfile=controls/'sdk.sh';sdkfile.write_text('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(sdk)));workers[str(sdkfile)]=m.observed(sdkfile,arch);current=run('sdk-before',['/bin/sh','/controls/sdk.sh'],90);need({n:hv for hv,n in (l.split('  ',1) for l in current.decode().splitlines())}==sdk,'actual precodegen SDK')
  (build/obj).parent.mkdir(parents=True,exist_ok=True);run('compile-one-consumer',m.dependency_argv(argv,obj,False),180);cpp=True;need(m.dep((build/(obj+'.d')).read_bytes())==actual,'actual MD equals genuine-M');objects={n:copies[n] for n in t['objects'] if n!=obj};objects[obj]=m.observed(build/obj,arch);fresh={obj+'.d':m.observed(build/(obj+'.d'),arch),'preflight/'+obj+'.d':m.observed(build/'preflight'/(obj+'.d'),arch)};run('link-one-consumer',link,120);linked=True;programs={n:copies[n] for n in PROGRAMS if n!=TARGET};programs[TARGET]=m.observed(build/TARGET,arch);need(programs[TARGET]['st_mode']&0o111,'new harness executable')
  need(run('sdk-after',['/bin/sh','/controls/sdk.sh'],90)==current,'SDK after exact');m.save(receipts/'sdk-binding.json',{'beforeSha256':sha(current),'afterSha256':sha(current),'paths':len(sdk),'equal':True})
  for path,s in protected.items():same(path,s)
  for n,s in before.items():same(source/n,s)
  for n,s in copies.items():same(root/n if n.startswith('receipts/') else build/n,s)
  for n,s in {**objects,**programs,**fresh}.items():same(build/n,s)
  for path,s in workers.items():same(path,s)
  m.save(receipts/'parents-after.json',protected);m.save(receipts/'ctime-diagnostics.json',diagnostics)
  result={'format':'nano-bootstrap-maintenance-one-consumer-build-terminal-v1','status':'BUILD_PASS_MAINTENANCE_TEST_NOT_EXECUTED','arch':arch,'controllerSha256':a.controller_sha,'guardSha256':GUARD,'parentPlanSha256':a.parent_plan_sha,'parentTerminalSha256':a.parent_terminal_sha,'parentMetadataSha256':a.parent_metadata_sha,'harnessProposalSha256':HARNESS,'sourceFiles':1320,'sourceChanged':[SRC],'sourceSha256':NEW,'coreObjectsCompiled':0,'coreObjectsCopied':0,'parentCoreObjectsReobserved':0,'archiveMembers':142,'archive':copies['libnano_core.a'],'archiveByteCopiedFromQualifiedParent':True,'archiveConstructionCommandsExecuted':0,'archiveVerificationCommandsExecuted':0,'rawMemberProofReusedOnlyForByteIdenticalArchive':True,'newObjectsCompiled':1,'oldObjectsByteCopied':2,'newLinks':1,'oldProgramsByteCopied':2,'oldCompilerCommandsUnchanged':True,'allActualMdEqualPreflight':True,'objects':objects,'programs':programs,'sourceSdkLibrariesGeneratedAndParentsUnchanged':True,'imageBuiltOrPulled':False,'testsExecuted':0,'programsExecuted':0,'cmakeConfigureExecuted':False,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
 except BaseException as e:
  failure=type(e).__name__+': '+str(e);result={'format':'nano-bootstrap-maintenance-one-consumer-build-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','arch':arch,'failure':failure,'consumerCompileCompleted':cpp,'consumerLinkCompleted':linked,'coreObjectsCompiled':0,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
  try:
   for path,s in protected.items():same(path,s)
   result['allParentsUnchangedAfterFailure']=True;m.save(receipts/'parents-after-failure.json',protected)
  except BaseException:result['allParentsUnchangedAfterFailure']=False
 m.save(receipts/'terminal-build.json',result);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
