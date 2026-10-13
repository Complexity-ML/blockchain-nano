#!/usr/bin/env python3
"""ROOT only: one actual pending-consumer CPP/link;0core,0archive,0CMake,0tests."""
import argparse,hashlib,json,os,pathlib,shlex,stat,subprocess,sys,time,types,uuid
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
CANDIDATE='b6b7c4504ab02bc20c83f90688f9038ff9d118b616ed41f2c7db4eff59e53ff8'
OLD='6f52a0ac4bd50b89540a21c4b7fb49862e67ff55973e702e44cfc1a4ed25fba6'
NEW='b3d5b0c1328ad563852df097e94f82f948eeb3bde022087b6ac101601b6bcb37'
SRC='tests/test_capella_bootstrap_archive_cas.cpp';TARGET='nano-capella-bootstrap-archive-cas-tests'
def require(ok,why):
 if not ok:raise ValueError(why)
def digest(b):return hashlib.sha256(b).hexdigest()
def canonical(s):
 p=pathlib.Path(s);require(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def main():
 ap=argparse.ArgumentParser()
 for k in ('controller-sha','guard','guard-sha','parent-plan','parent-plan-sha','parent-terminal','parent-terminal-sha','candidate','candidate-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();selfp=canonical(__file__);rawself=selfp.read_bytes();require(digest(rawself)==a.controller_sha,'literal own SHA');guardp=canonical(a.guard);rawguard=guardp.read_bytes();require(a.guard_sha==GUARD and digest(rawguard)==GUARD,'literal qualified guard');m=types.ModuleType('public_build_guards');m.__file__=str(guardp);exec(compile(rawguard,str(guardp),'exec'),m.__dict__)
 def control(path,expected):
  raw,seen=m.observed(canonical(path),'arm64',16*1024*1024,True);require(digest(raw)==expected,'external control SHA');return json.loads(raw),seen
 p,ps=control(a.parent_plan,a.parent_plan_sha);arch=p['arch'];t,ts=control(a.parent_terminal,a.parent_terminal_sha);candidate,cs=control(a.candidate,a.candidate_sha)
 require(p['image']==m.IMAGES[arch] and p['baselineGitCommit']==m.BASE and p['status']=='READY_ROOT_DISPATCH_ONLY' and t['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and t['controllerSha256']==GUARD and t['planSha256']==a.parent_plan_sha and t['arch']==arch and t['archiveMembers']==142 and t['sourceFiles']==1317,'actual142 parent only')
 require(len(t['coreObjects'])==142 and len(t['consumerObjects'])==3 and set(t['programs'])=={'nano',TARGET,'nano-capella-bootstrap-startup-tests'} and t['testsExecuted']==0 and t['allActualMdEqualPreflight'] and t['publicParentsAndSdkUnchanged'],'actual parent three coherent links')
 require(a.candidate_sha==CANDIDATE and candidate['baselineRevision']==m.BASE and candidate['overlayCount']==11 and len(candidate['sourceFiles'])==11,'frozen third candidate only')
 root=canonical(a.output);parent=canonical(p['outputRoot']);pb=canonical(parent/'build');origin=canonical(parent/'source');require(not root.exists() and root.parent.is_dir() and not any(overlap(root,x) for x in (parent,canonical(a.parent_plan).parent,canonical(a.candidate).parent,selfp.parent)),'fresh separate output')
 require(os.geteuid()==p['uid'] and os.getegid()==p['gid'] and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'owner/environment');protected={};diagnostics=[]
 def pin(path,hash_value=None,bytes_value=None,old=None):
  path=canonical(path);s=m.observed(path,arch);require((hash_value is None or s['sha256']==hash_value) and (bytes_value is None or s['st_size']==bytes_value),'public byte pin')
  if old:require(s['sha256']==old['sha256'] and all(s[k]==old[k] for k in m.material(arch)),'actual parent material pin')
  protected[str(path)]=s;return s
 def read(path):
  b,s=m.observed(canonical(path),arch,16*1024*1024,True);pin(path,s['sha256'],s['st_size']);return b
 def doc(path):return json.loads(read(path))
 def same(path,s):
  z=m.observed(path,arch);require(z['sha256']==s['sha256'] and all(z[k]==s[k] for k in m.material(arch)),'protected material readback')
  if z['st_ctime_ns']!=s['st_ctime_ns']:diagnostics.append({'path':str(path),'before':s['st_ctime_ns'],'after':z['st_ctime_ns'],'bytesAndMaterialEqual':True,'causeMeasured':False})
 for path,h in ((selfp,a.controller_sha),(guardp,GUARD),(canonical(a.parent_plan),a.parent_plan_sha),(canonical(a.parent_terminal),a.parent_terminal_sha),(canonical(a.candidate),CANDIDATE)):pin(path,h)
 oldsource=doc(parent/'receipts/source-before.json');oldpins={**p['sourcePins'],**p['overlays']};require(len(oldsource)==1317 and set(oldsource)==set(oldpins),'parent source1317')
 for n,s in oldsource.items():require(s['sha256']==oldpins[n]['sha256'] and s['st_size']==oldpins[n]['bytes'],'parent externally bound source');pin(origin/n,s['sha256'],s['st_size'],s)
 require(oldsource[SRC]['sha256']==OLD,'only expected failed-fixture parent');newfile=canonical(a.candidate).parent/next(r['sourcePath'] for r in candidate['sourceFiles'] if r['path']==SRC);require(pin(newfile,NEW)['st_size']==61708,'new immutable one-line Genesis source')
 for r in candidate['sourceFiles']:
  if r['path']!=SRC:require(r['sha256']==oldsource[r['path']]['sha256'] and r['bytes']==oldsource[r['path']]['st_size'],'other ten candidate source overlays unchanged')
 for field in ('coreObjects','consumerObjects','programs'):
  for n,s in t[field].items():pin(pb/n,s['sha256'],s['st_size'],s)
 archive=pin(pb/'libnano_core.a',t['archive']['sha256'],t['archive']['st_size'],t['archive']);proof=doc(parent/'receipts/archive-142-raw-member-proof.json');require(proof['archiveBeforeProof']['sha256']==archive['sha256'] and len(proof['members'])==142 and {r['object'] for r in proof['members']}==set(t['coreObjects']) and len({r['member'] for r in proof['members']})==142,'actual raw142 parent proof')
 for r in proof['members']:require(r['sha256']==t['coreObjects'][r['object']]['sha256'] and r['bytes']==t['coreObjects'][r['object']]['st_size'],'all raw members equal protected actual objects')
 for table in ('libraries','generated'):
  for n,r in p[table].items():pin(pb/n,r['sha256'],r['bytes'])
 selection=doc(parent/'receipts/selection.json');fresh=set(selection['freshCore']);require(selection['actualMdBased'] and p['newCore'] in fresh,'actual current parent MD provenance')
 for r in p['core']:
  path=pb/(r['object']+'.d') if r['source'] in fresh else canonical(r['mdInput']['path']);pin(path)
  if r['source'] not in fresh:require(protected[str(path)]['sha256']==r['mdInput']['sha256'],'exact inherited actual-MD origin')
 new_core_obj='CMakeFiles/nano_core.dir/'+p['newCore']+'.o';pin(pb/(new_core_obj+'.d'))
 for n in t['consumerObjects']:pin(pb/(n+'.d'))
 cc=doc(pb/'compile_commands.json');row=next(r for r in cc if r['file']=='/work/'+SRC and m.unit(r)[0].startswith('CMakeFiles/'+TARGET+'.dir/'));obj,argv=m.unit(row);olddeps=m.dep(read(pb/(obj+'.d')))
 meta=['compile_commands.json','CTestTestfile.cmake','CMakeFiles/nano_core.dir/flags.make','CMakeFiles/nano_core.dir/link.txt']+[f'CMakeFiles/{x}.dir/{leaf}' for x in t['programs'] for leaf in ('flags.make','link.txt')]
 for n in meta:pin(pb/n)
 link=shlex.split(read(pb/f'CMakeFiles/{TARGET}.dir/link.txt').decode());require(link[0]=='/usr/bin/clang++' and link.count('-o')==1 and link[link.index('-o')+1]==TARGET and link.count('libnano_core.a')==1 and obj in link,'exact existing consumer link');require(all(x in {obj,*p['libraries']} for x in link if x.endswith('.o')),'direct object operands closed')
 sdkbefore=read(parent/'receipts/sdk-before.log');sdkafter=read(parent/'receipts/sdk-after.log');binding=doc(parent/'receipts/sdk-binding.json');require(sdkbefore==sdkafter and digest(sdkbefore)==binding['beforeSha256']==binding['afterSha256'],'parent actual SDK byte proof');sdk={n:h for h,n in (l.split('  ',1) for l in sdkbefore.decode().splitlines())};require(len(sdk)==binding['paths'] and all(sdk.get(n)==h for n,h in p['sdkHashes'].items()),'all qualified SDK pins')
 os.mkdir(root,0o700);source=root/'source';build=root/'build';controls=root/'controls';receipts=root/'receipts'
 for d in (source,build,controls,receipts):d.mkdir(mode=0o700)
 m.save(receipts/'initial-empty-output.json',{'sourceEmpty':not any(source.iterdir()),'buildEmpty':not any(build.iterdir()),'beforeAnyCopy':True});(build/'tmp').mkdir(mode=0o700);before={};copies={};workers={};failure=None;start=time.monotonic();env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};cpp=False;linked=False;actual_sdk=None
 def cp(path,dst):
  b,s=m.observed(canonical(path),arch,None,True);require(s['sha256']==protected[str(path)]['sha256'] and all(s[k]==protected[str(path)][k] for k in m.material(arch)),'parent copy input stable');dst.parent.mkdir(parents=True,exist_ok=True);fd=os.open(dst,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,stat.S_IMODE(s['st_mode']))
  with os.fdopen(fd,'wb') as out:out.write(b);out.flush();os.fsync(out.fileno())
  z=m.observed(dst,arch);require(z['sha256']==s['sha256'] and z['st_size']==s['st_size'],'exact new copied bytes');return z
 def run(label,args,seconds):
  require(time.monotonic()-start<900,'finite900s build');token=uuid.uuid4().hex;name='nano-pending-repair-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name];cmd=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.pending.repair.owner='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/outputs/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs','--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',args[0],p['image']]+args[1:];m.save(receipts/(label+'.command.json'),{'argv':cmd,'timeoutSeconds':seconds})
  try:
   with (receipts/(label+'.log')).open('xb',buffering=0) as log:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,900-(time.monotonic()-start))))
   m.save(receipts/(label+'.exit.json'),{'exitCode':r.returncode});require(r.returncode==0,'failed phase '+label);print(json.dumps({'phase':label,'elapsedSeconds':time.monotonic()-start}),flush=True);return (receipts/(label+'.log')).read_bytes()
  finally:
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if z.returncode==0:require(json.loads(z.stdout).get('nano.pending.repair.owner')==token,'owned cleanup');require(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:require(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'owned state')
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);require(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'owned container remains')
 try:
  for n in oldsource:before[n]=cp(newfile if n==SRC else origin/n,source/n)
  require(len(before)==1317 and {n for n in before if before[n]['sha256']!=oldsource[n]['sha256']}=={SRC},'fresh full source only one CPP changed');m.save(receipts/'source-before.json',before);m.save(receipts/'parents-before.json',protected)
  for n in ['libnano_core.a',*p['libraries'],*p['generated'],*meta,'nano','nano-capella-bootstrap-startup-tests']:copies[n]=cp(pb/n,build/n)
  (build/'preflight'/obj).parent.mkdir(parents=True,exist_ok=True);probe=m.dependency_argv(argv,obj,True);run('dependency-preflight',probe,90);actualdeps=m.dep((build/'preflight'/(obj+'.d')).read_bytes());require(actualdeps==olddeps,'only source-body repair; original actual compilation dependency closure unchanged')
  for d in actualdeps:
   if d.startswith('/work/'):require(d[6:] in before,'source dependency closure')
   elif d.startswith('/outputs/'):require(d[9:] in copies,'generated dependency closure')
   else:require(d in sdk,'actual SDK dependency closure')
  sdkfile=controls/'sdk.sh';sdkfile.write_text('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(sdk)));workers[str(sdkfile)]=m.observed(sdkfile,arch);actual_sdk=run('sdk-before',['/bin/sh','/controls/sdk.sh'],90);require({n:h for h,n in (l.split('  ',1) for l in actual_sdk.decode().splitlines())}==sdk,'actual precodegen SDK')
  (build/obj).parent.mkdir(parents=True,exist_ok=True);run('compile-one-consumer',m.dependency_argv(argv,obj,False),180);cpp=True;require(m.dep((build/(obj+'.d')).read_bytes())==actualdeps,'actual MD equals real-M');objectpin=m.observed(build/obj,arch);mdpin=m.observed(build/(obj+'.d'),arch);probe_pin=m.observed(build/'preflight'/(obj+'.d'),arch);run('link-one-consumer',link,120);linked=True;binary=m.observed(build/TARGET,arch);require(binary['st_mode']&0o111,'linked executable permissions')
  require(run('sdk-after',['/bin/sh','/controls/sdk.sh'],90)==actual_sdk,'actual SDK equality');m.save(receipts/'sdk-binding.json',{'beforeSha256':digest(actual_sdk),'afterSha256':digest(actual_sdk),'paths':len(sdk),'equal':True})
  for path,s in protected.items():same(path,s)
  for n,s in before.items():same(source/n,s)
  for n,s in copies.items():same(build/n,s)
  for path,s in workers.items():same(path,s)
  same(build/obj,objectpin);same(build/(obj+'.d'),mdpin);same(build/'preflight'/(obj+'.d'),probe_pin);same(build/TARGET,binary);m.save(receipts/'parents-after.json',protected);m.save(receipts/'ctime-diagnostics.json',diagnostics)
  result={'format':'nano-pending-fixture-one-consumer-build-terminal-v1','status':'BUILD_PASS_PENDING_TEST_NOT_EXECUTED','arch':arch,'controllerSha256':a.controller_sha,'guardSha256':GUARD,'parentPlanSha256':a.parent_plan_sha,'parentTerminalSha256':a.parent_terminal_sha,'candidateSha256':CANDIDATE,'sourceFiles':1317,'sourceChanged':[SRC],'sourceSha256':NEW,'coreObjectsCompiled':0,'coreObjectsCopied':0,'parentCoreObjectsReobserved':142,'archiveCommandsExecuted':0,'cmakeConfigureExecuted':False,'consumerObjectsCompiled':1,'consumerLinks':1,'oldProgramsByteCopied':2,'programsExecuted':0,'archiveMembers':142,'archive':copies['libnano_core.a'],'archiveByteCopiedFromQualifiedParent':True,'rawMemberProofReusedOnlyForByteIdenticalArchive':True,'object':{'path':obj,**objectpin},'actualMd':{'path':obj+'.d',**mdpin},'preflightMd':{'path':'preflight/'+obj+'.d',**probe_pin},'program':{'path':TARGET,**binary},'programs':{TARGET:binary,'nano':copies['nano'],'nano-capella-bootstrap-startup-tests':copies['nano-capella-bootstrap-startup-tests']},'librariesGeneratedSdkAndAllParentsUnchanged':True,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
 except BaseException as error:
  result={'format':'nano-pending-fixture-one-consumer-build-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','arch':arch,'failure':type(error).__name__+': '+str(error),'coreObjectsCompiled':0,'consumerCompileCompleted':cpp,'consumerLinkCompleted':linked,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False};failure=result['failure']
  try:
   for path,s in protected.items():same(path,s)
   result['allParentsUnchangedAfterFailure']=True;m.save(receipts/'parents-after-failure.json',protected)
  except BaseException:result['allParentsUnchangedAfterFailure']=False
 m.save(receipts/'terminal-build.json',result);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
