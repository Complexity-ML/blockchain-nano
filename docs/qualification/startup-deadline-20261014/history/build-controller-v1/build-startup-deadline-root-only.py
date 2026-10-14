#!/usr/bin/env python3
"""ROOT build-only: runtime1 + deadline1 compiled,141 core reused, three new links.
Original full142 and old CAS assets remain immutable. No tests or private inputs.
"""
import argparse,hashlib,json,os,pathlib,posixpath,re,shlex,stat,subprocess,sys,time,types,uuid
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
PROPOSAL='35e9bbbda83f78326ff868bf8183acc78456be7d268f8d6ac5548ae5567420bd'
CURRENT='b6b7c4504ab02bc20c83f90688f9038ff9d118b616ed41f2c7db4eff59e53ff8'
RUNTIME='src/consensus_capella_runtime.cpp';HEADER='src/consensus_capella_startup_budget.hpp';TEST='tests/test_capella_startup_deadline.cpp';TARGET='nano-capella-startup-deadline-tests';PENDING='tests/test_capella_bootstrap_archive_cas.cpp'
OVERLAYS={'CMakeLists.txt':'011a0ab3df13134f3de51f5e2e94c01462860f221f844b491dcc37ba6a5a3ad5',RUNTIME:'8e6cb49a36978961a9413f2d0f40f67f4ae0d05287530d691ee25dc4f834ae63',HEADER:'a585e6a9ec051761772795155aec063eaf566a5036c86cab638899e700f2f228',TEST:'66bac2539b1a12ea0e7762d2d99a241c930ed10bd2d29d016cd08ff2ea4a045e',PENDING:'b3d5b0c1328ad563852df097e94f82f948eeb3bde022087b6ac101601b6bcb37'}
CONSUMERS=['nano','nano-capella-bootstrap-startup-tests'];PROGRAMS=CONSUMERS+[TARGET]
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(s):
 p=pathlib.Path(s);need(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def main():
 ap=argparse.ArgumentParser()
 for k in ('controller-sha','guard','guard-sha','parent-plan','parent-plan-sha','parent-terminal','parent-terminal-sha','proposal','proposal-sha','current-candidate','current-candidate-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();need(all(re.fullmatch('[0-9a-f]{64}',getattr(a,k)) for k in ('controller_sha','guard_sha','parent_plan_sha','parent_terminal_sha','proposal_sha','current_candidate_sha')),'external SHA pins');selfp=canonical(__file__);need(sha(selfp.read_bytes())==a.controller_sha,'controller SHA');gp=canonical(a.guard);gb=gp.read_bytes();need(a.guard_sha==GUARD and sha(gb)==GUARD,'qualified guards');m=types.ModuleType('startup_deadline_build_guards');m.__file__=str(gp);exec(compile(gb,str(gp),'exec'),m.__dict__)
 def control(path,h):
  raw,s=m.observed(canonical(path),'arm64',16*1024*1024,True);need(sha(raw)==h,'external public input');return json.loads(raw)
 p=control(a.parent_plan,a.parent_plan_sha);arch=p['arch'];t=control(a.parent_terminal,a.parent_terminal_sha);proposal=control(a.proposal,a.proposal_sha);current=control(a.current_candidate,a.current_candidate_sha)
 need(arch in m.IMAGES and p['image']==m.IMAGES[arch] and p['format']=='nano-bootstrap-owner-runtime-incremental-plan-v1' and p['status']=='READY_ROOT_DISPATCH_ONLY' and p['baselineGitCommit']==m.BASE,'original full parent policy')
 need(t['format']=='nano-bootstrap-owner-runtime-incremental-build-terminal-v1' and t['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and t['controllerSha256']==GUARD and t['planSha256']==a.parent_plan_sha and t['arch']==arch and t['archiveMembers']==142 and len(t['coreObjects'])==142 and len(t['consumerObjects'])==3 and set(t['programs'])=={'nano','nano-capella-bootstrap-archive-cas-tests','nano-capella-bootstrap-startup-tests'} and t['sourceFiles']==1317 and t['allActualMdEqualPreflight'] and t['publicParentsAndSdkUnchanged'] and t['testsExecuted']==0,'actual full142 parent only')
 need(a.proposal_sha==PROPOSAL and {r['path'] for r in proposal['sourceFiles']}==set(OVERLAYS)-{PENDING} and proposal['protectedHeaderSha256']=='0692ae401d0f5a2ccc66ffba3fe8531644a20048a286536145ce9dd741cfc7f0' and a.current_candidate_sha==CURRENT and current['overlayCount']==11,'four production/test proposal paths plus explicit current pending source')
 parent=canonical(p['outputRoot']);need(parent==canonical(a.parent_terminal).parent.parent and parent.name=='run-02','exact full parent run');pb=canonical(parent/'build');origin=canonical(parent/'source');root=canonical(a.output);need(not root.exists() and root.parent.is_dir() and not any(overlap(root,q) for q in (parent,canonical(a.parent_plan).parent,canonical(a.proposal).parent,canonical(a.current_candidate).parent,selfp.parent)),'fresh output separate from parents');need(os.geteuid()==p['uid'] and os.getegid()==p['gid'] and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'public owner/environment');protected={};diagnostics=[]
 def pin(path,h=None,n=None,old=None):
  path=canonical(path);s=m.observed(path,arch);need((h is None or s['sha256']==h) and (n is None or s['st_size']==n),'public byte pin')
  if old:need(s['sha256']==old['sha256'] and all(s[k]==old[k] for k in m.material(arch)),'actual parent material')
  protected[str(path)]=s;return s
 def read(path):
  b,s=m.observed(canonical(path),arch,16*1024*1024,True);pin(path,s['sha256'],s['st_size']);return b
 def doc(path):return json.loads(read(path))
 def same(path,s):
  z=m.observed(path,arch);need(z['sha256']==s['sha256'] and all(z[k]==s[k] for k in m.material(arch)),'protected material readback')
  if z['st_ctime_ns']!=s['st_ctime_ns']:diagnostics.append({'path':str(path),'old':s['st_ctime_ns'],'new':z['st_ctime_ns'],'materialAndBytesEqual':True,'causeMeasured':False})
 for path,h in ((selfp,a.controller_sha),(gp,GUARD),(a.parent_plan,a.parent_plan_sha),(a.parent_terminal,a.parent_terminal_sha),(a.proposal,PROPOSAL),(a.current_candidate,CURRENT)):pin(path,h)
 oldsource=doc(parent/'receipts/source-before.json');oldpins={**p['sourcePins'],**p['overlays']};need(len(oldsource)==1317 and set(oldsource)==set(oldpins),'externally bound original source1317');source_inputs={}
 for n,s in oldsource.items():m.relative(n);need(s['sha256']==oldpins[n]['sha256'] and s['st_size']==oldpins[n]['bytes'],'original source pins');pin(origin/n,s['sha256'],s['st_size'],s);source_inputs[n]=origin/n
 for r in proposal['sourceFiles']:
  n=m.relative(r['path']);need(r['sha256']==OVERLAYS[n] and (r['baselineSha256'] is None and n not in oldsource or n in oldsource and r['baselineSha256']==oldsource[n]['sha256']),'exact four deadline overlays');f=canonical(a.proposal).parent/r['sourcePath'];pin(f,r['sha256'],r['bytes']);source_inputs[n]=f
 pending=next(r for r in current['sourceFiles'] if r['path']==PENDING);need(pending['sha256']==OVERLAYS[PENDING] and all(r['path']==PENDING or r['sha256']==oldsource[r['path']]['sha256'] for r in current['sourceFiles']),'only explicit current uncompiled pending overlay');f=canonical(a.current_candidate).parent/pending['sourcePath'];pin(f,pending['sha256'],pending['bytes']);source_inputs[PENDING]=f
 need(len(source_inputs)==1319 and oldsource['include/nano/consensus_capella_runtime.hpp']['sha256']==proposal['protectedHeaderSha256'],'public runtime ABI unchanged');reg=read(canonical(a.proposal).parent/'registration-block.txt');need(read(source_inputs['CMakeLists.txt'])==read(origin/'CMakeLists.txt')+b'\n'+reg and sha(reg)=='4d6c4445f7d7e5abb462a2de4b5b76b9ce908e4e1caf8ddcd29d9c5bf3f3bbaf','exact one added target/CTest registration')
 for table in ('coreObjects','consumerObjects','programs'):
  for n,s in t[table].items():pin(pb/n,s['sha256'],s['st_size'],s)
 archive=pin(pb/'libnano_core.a',t['archive']['sha256'],t['archive']['st_size'],t['archive']);proof=doc(parent/'receipts/archive-142-raw-member-proof.json');members=proof['members'];need(proof['archiveBeforeProof']['sha256']==archive['sha256'] and len(members)==142 and {r['object'] for r in members}==set(t['coreObjects']) and len({r['member'] for r in members})==142,'actual142 parent raw-byte member closure')
 for r in members:need(r['sha256']==t['coreObjects'][r['object']]['sha256'] and r['bytes']==t['coreObjects'][r['object']]['st_size'],'actual raw member/object binding')
 for table in ('libraries','generated'):
  for n,r in p[table].items():pin(pb/n,r['sha256'],r['bytes'])
 oldcc=doc(pb/'compile_commands.json');key=lambda r:(r['file'],r.get('output'),r['directory']);old={key(r):r for r in oldcc};selection=doc(parent/'receipts/selection.json');fresh=set(selection['freshCore']);need(selection['actualMdBased'] and p['newCore'] in fresh,'parent MD provenance');by_source={r['source']:r for r in p['core']};aliases=[];core_units={}
 def canonical_dep(d):
  if d.startswith('/work/'):
   n=m.relative(posixpath.normpath(d[6:]));need(n in oldsource,'old source dependency closure');aliases.append({'raw':d,'canonical':n}) if n!=d[6:] else None;return n
  return None
 for obj in t['coreObjects']:
  src=m.relative(obj.removeprefix('CMakeFiles/nano_core.dir/')[:-2]);rows=[r for r in oldcc if r['file']=='/work/'+src and m.unit(r)[0]==obj];need(len(rows)==1,'each actual core compiler command');core_units[obj]=m.unit(rows[0]);md=pb/(obj+'.d') if src in fresh else canonical(by_source[src]['mdInput']['path']);pin(md);need(src in fresh or protected[str(md)]['sha256']==by_source[src]['mdInput']['sha256'],'inherited MD exact pin');deps=m.dep(read(md))
  if src!=RUNTIME:
   for d in deps:
    n=canonical_dep(d);need(n is None or n not in {RUNTIME,PENDING},'one runtime footprint, no altered input in reused core unit')
 runtime_obj='CMakeFiles/nano_core.dir/'+RUNTIME+'.o';need(runtime_obj in core_units,'one selected runtime unit');consumer_units={}
 for target in CONSUMERS:
  obj=next(n for n in t['consumerObjects'] if n.startswith('CMakeFiles/'+target+'.dir/'));src=m.relative(obj.removeprefix('CMakeFiles/'+target+'.dir/')[:-2]);rows=[r for r in oldcc if r['file']=='/work/'+src and m.unit(r)[0]==obj];need(len(rows)==1,'one original main/startup object');obj,argv=m.unit(rows[0]);deps=m.dep(read(pb/(obj+'.d')))
  for d in deps:n=canonical_dep(d);need(n is None or n not in {RUNTIME,PENDING},'reuse main/startup with identical transitive source inputs')
  consumer_units[target]=(obj,argv)
 meta=['compile_commands.json','CMakeFiles/nano_core.dir/flags.make','CMakeFiles/nano_core.dir/link.txt']+[f'CMakeFiles/{target}.dir/{leaf}' for target in t['programs'] for leaf in ('flags.make','link.txt')]
 for n in meta:pin(pb/n)
 sb=read(parent/'receipts/sdk-before.log');binding=doc(parent/'receipts/sdk-binding.json');need(sb==read(parent/'receipts/sdk-after.log') and sha(sb)==binding['beforeSha256']==binding['afterSha256'],'actual parent SDK before/after');sdk={n:h for h,n in (l.split('  ',1) for l in sb.decode().splitlines())};need(len(sdk)==binding['paths'] and all(sdk.get(n)==h for n,h in p['sdkHashes'].items()),'known SDK binding')
 os.mkdir(root,0o700);source=root/'source';build=root/'build';controls=root/'controls';receipts=root/'receipts'
 for d in (source,build,controls,receipts):d.mkdir(mode=0o700)
 m.save(receipts/'initial-empty-output.json',{'sourceEmpty':not any(source.iterdir()),'buildEmpty':not any(build.iterdir()),'beforeAnyCopy':True});(build/'tmp').mkdir(mode=0o700);before={};copies={};workers={};failure=None;started=time.monotonic();env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};compiled=0;linked=0;objects={};programs={};newmeta={};units={}
 def cp(path,dst):
  b,s=m.observed(canonical(path),arch,None,True);expected=protected[str(path)];need(s['sha256']==expected['sha256'] and all(s[k]==expected[k] for k in m.material(arch)),'exact pinned copied buffer');dst.parent.mkdir(parents=True,exist_ok=True);fd=os.open(dst,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,stat.S_IMODE(s['st_mode']))
  with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
  z=m.observed(dst,arch);need(z['sha256']==s['sha256'] and z['st_size']==s['st_size'],'copy bytes exact');return z
 def host(label,argv,seconds):
  need(time.monotonic()-started<1800,'finite1800s build');m.save(receipts/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds});at=time.monotonic()
  with (receipts/(label+'.log')).open('xb',buffering=0) as log:
   try:r=subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,1800-(time.monotonic()-started))))
   except subprocess.TimeoutExpired:m.save(receipts/(label+'.exit.json'),{'exitCode':None,'timedOut':True});raise
  m.save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-at});need(r.returncode==0,'phase failed '+label);print(json.dumps({'phase':label,'elapsedSeconds':time.monotonic()-started}),flush=True);return (receipts/(label+'.log')).read_bytes()
 def run(label,args,seconds):
  token=uuid.uuid4().hex;name='nano-startup-budget-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name];cmd=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.startup.budget.owner='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/outputs/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs','--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',args[0],p['image']]+args[1:]
  try:return host(label,cmd,seconds)
  finally:
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if r.returncode==0:need(json.loads(r.stdout).get('nano.startup.budget.owner')==token,'owned cleanup');need(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:need(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'cleanup observation')
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);need(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'owned container remains')
 try:
  image=json.loads(host('image-inspect',p['dockerArgv']+['image','inspect','--format','{{json .}}',p['image']],20));need(image['Id']==p['image'] and image['Architecture']==arch and image['Os']=='linux' and not any(x.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for x in image.get('Config',{}).get('Env',[])),'same existing image without injected environment')
  for n,f in source_inputs.items():before[n]=cp(f,source/n)
  need(len(before)==1319 and {n for n in before if n not in oldsource or before[n]['sha256']!=oldsource[n]['sha256']}==set(OVERLAYS),'exact four deadline paths plus uncompiled pending current source');m.save(receipts/'source-before.json',before);m.save(receipts/'parents-before.json',protected);m.save(receipts/'inherited-md-aliases.json',aliases)
  run('configure',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],120)
  cc=json.loads(read(build/'compile_commands.json'));now={key(r):r for r in cc};need(len(now)==len(old)+1 and all(now.get(k)==r for k,r in old.items()),'only one new compiler command; all core/consumer argv unchanged')
  for n in meta[1:]:need(read(build/n)==read(pb/n),'old flags/link metadata byte identical')
  for n,s in t['coreObjects'].items():
   if n!=runtime_obj:copies[n]=cp(pb/n,build/n);objects[n]=copies[n]
  for target,(obj,argv) in consumer_units.items():copies[obj]=cp(pb/obj,build/obj)
  for n in p['libraries']:copies[n]=cp(pb/n,build/n)
  for n,r in p['generated'].items():pin(build/n,r['sha256'],r['bytes']);copies[n]=m.observed(build/n,arch)
  rows=[r for r in cc if r['file']=='/work/'+TEST and m.unit(r)[0].startswith('CMakeFiles/'+TARGET+'.dir/')];need(len(rows)==1,'one new deadline object');test_obj,test_argv=m.unit(rows[0]);units={runtime_obj:core_units[runtime_obj][1],test_obj:test_argv};probes={};sysfiles=set(sdk)|{'/usr/bin/clang++','/usr/bin/ar','/usr/bin/ranlib','/usr/bin/cmake'}
  for obj,argv in units.items():
   (build/'preflight'/(obj+'.d')).parent.mkdir(parents=True,exist_ok=True);run('dependency-'+('runtime' if obj==runtime_obj else 'deadline'),m.dependency_argv(argv,obj,True),90);probes[obj]=m.dep(read(build/'preflight'/(obj+'.d')))
   for d in probes[obj]:
    if d.startswith('/work/'):need(m.relative(posixpath.normpath(d[6:])) in before,'actual new project closure')
    elif d.startswith('/outputs/'):need(m.relative(posixpath.normpath(d[9:])) in copies,'actual generated closure')
    else:sysfiles.add(d)
  newmeta={f'CMakeFiles/{TARGET}.dir/{leaf}':m.observed(build/f'CMakeFiles/{TARGET}.dir/{leaf}',arch) for leaf in ('flags.make','link.txt')};links={}
  for target,obj in [(x,consumer_units[x][0]) for x in CONSUMERS]+[(TARGET,test_obj)]:
   link=shlex.split(read(build/f'CMakeFiles/{target}.dir/link.txt').decode());need(link[0]=='/usr/bin/clang++' and link.count('-o')==1 and link[link.index('-o')+1]==target and obj in link and link.count('libnano_core.a')==1 and all(x in {obj,*p['libraries']} for x in link if x.endswith('.o')),'three current coherent links');links[target]=link;sysfiles.update(x for x in link if x.startswith(('/usr/','/lib/')))
  corelink=[shlex.split(l) for l in read(build/'CMakeFiles/nano_core.dir/link.txt').decode().splitlines() if l];need(len(corelink)==2 and corelink[0][0]=='/usr/bin/ar' and corelink[0][1:3]==['qc','libnano_core.a'] and set(corelink[0][3:])==set(t['coreObjects']) and len(corelink[0][3:])==142 and corelink[1]==['/usr/bin/ranlib','libnano_core.a'],'actual142 archive construction argv')
  sdkfile=controls/'sdk.sh';sdkfile.write_text('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(sysfiles)));workers[str(sdkfile)]=m.observed(sdkfile,arch);actualsdk=run('sdk-before',['/bin/sh','/controls/sdk.sh'],120);sd={n:h for h,n in (l.split('  ',1) for l in actualsdk.decode().splitlines())};need(set(sd)==sysfiles and all(sd.get(n)==h for n,h in sdk.items()),'all parent and fresh compiler/link SDK inputs beforecodegen')
  fresh={}
  for obj,argv in units.items():
   (build/obj).parent.mkdir(parents=True,exist_ok=True);run('compile-'+('runtime' if obj==runtime_obj else 'deadline'),m.dependency_argv(argv,obj,False),180);compiled+=1;need(m.dep(read(build/(obj+'.d')))==probes[obj],'actual MD equals real-M');fresh[obj]=m.observed(build/obj,arch)
  objects[runtime_obj]=fresh[runtime_obj];run('archive-construct',corelink[0],120);run('archive-index',corelink[1],60);ar=m.observed(build/'libnano_core.a',arch);names=run('archive-names',['/usr/bin/ar','t','libnano_core.a'],60).decode().splitlines();oldmembers={r['member']:r for r in members};need(len(names)==142 and len(set(names))==142 and set(names)==set(oldmembers),'exact142 member identities');body='#!/bin/sh\nset -eu\n'
  for name in names:need(re.fullmatch('[A-Za-z0-9_.+-]+',name),'safe raw member name');body+='/usr/bin/ar p /outputs/libnano_core.a '+shlex.quote(name)+' > /outputs/tmp/member\nprintf '+shlex.quote(name+' ')+'\n/usr/bin/sha256sum /outputs/tmp/member\n/usr/bin/wc -c < /outputs/tmp/member\n'
  wf=controls/'raw-member-proof.sh';wf.write_text(body);workers[str(wf)]=m.observed(wf,arch);raw=run('archive-raw-byte-proof',['/bin/sh','/controls/raw-member-proof.sh'],120).decode().splitlines();need(len(raw)==284,'actual142 hash/size pairs');actual=[]
  for i in range(0,len(raw),2):name,h,_=raw[i].split(' ',2);obj=oldmembers[name]['object'];need(h==objects[obj]['sha256'] and int(raw[i+1])==objects[obj]['st_size'],'all current raw archive members equal current142 objects');actual.append({'member':name,'object':obj,'sha256':h,'bytes':int(raw[i+1])})
  same(build/'libnano_core.a',ar);m.save(receipts/'archive-142-raw-member-proof.json',{'archiveBeforeProof':ar,'members':actual,'actualArBytes':True,'runtimeOneFresh141Reused':True})
  for target,link in links.items():run('link-'+target,link,120);linked+=1;programs[target]=m.observed(build/target,arch)
  same(build/'libnano_core.a',ar);need(run('sdk-after',['/bin/sh','/controls/sdk.sh'],120)==actualsdk,'current SDK unchanged');m.save(receipts/'sdk-binding.json',{'beforeSha256':sha(actualsdk),'afterSha256':sha(actualsdk),'paths':len(sysfiles),'equal':True})
  for path,s in protected.items():same(path,s)
  for n,s in before.items():same(source/n,s)
  for n,s in {**copies,**objects,**fresh,**programs,**newmeta}.items():same(build/n,s)
  for path,s in workers.items():same(path,s)
  m.save(receipts/'parents-after.json',protected);m.save(receipts/'ctime-diagnostics.json',diagnostics)
  result={'format':'nano-capella-startup-deadline-incremental-build-terminal-v1','status':'BUILD_PASS_STARTUP_DEADLINE_TESTS_NOT_EXECUTED','arch':arch,'controllerSha256':a.controller_sha,'guardSha256':GUARD,'parentPlanSha256':a.parent_plan_sha,'parentTerminalSha256':a.parent_terminal_sha,'proposalSha256':PROPOSAL,'currentPendingCandidateSha256':CURRENT,'sourceFiles':1319,'sourceChangedOrAdded':sorted(OVERLAYS),'coreFresh':1,'coreReused':141,'newTestObjectsCompiled':1,'consumerObjectsReused':2,'links':3,'archiveMembers':142,'archive':ar,'archiveConstructionCommandsExecuted':2,'archiveVerificationCommandsExecuted':143,'coreObjects':objects,'consumerObjects':{**{obj:copies[obj] for obj,_ in consumer_units.values()},test_obj:fresh[test_obj]},'programs':programs,'oldCompilerCommandsUnchanged':True,'publicRuntimeHeaderUnchanged':True,'allFreshActualMdEqualPreflight':True,'publicSourceSdkLibrariesGeneratedAndParentsUnchanged':True,'pendingCurrentSourceOverlayNotCompiledOrExecuted':True,'oldCasObjectOnlyProtectedNotClaimedCurrent':True,'testsExecuted':0,'imageBuiltOrPulled':False,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
 except BaseException as e:
  failure=type(e).__name__+': '+str(e);result={'format':'nano-capella-startup-deadline-incremental-build-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','arch':arch,'failure':failure,'completedNewCompilerCommands':compiled,'completedLinks':linked,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
  try:
   for path,s in protected.items():same(path,s)
   result['allParentsUnchangedAfterFailure']=True;m.save(receipts/'parents-after-failure.json',protected)
  except BaseException:result['allParentsUnchangedAfterFailure']=False
 m.save(receipts/'terminal-build.json',result);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
