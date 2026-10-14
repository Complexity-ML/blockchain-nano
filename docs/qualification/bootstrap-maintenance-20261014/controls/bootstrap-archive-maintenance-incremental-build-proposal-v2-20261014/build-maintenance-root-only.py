#!/usr/bin/env python3
"""ROOT build-only: three new maintenance objects/links; zero core/archive/old tests.
Uses the qualified cd1b guards and explicit parent142 + one-consumer lineage.
"""
import argparse,hashlib,json,os,pathlib,posixpath,re,shlex,stat,subprocess,sys,time,types,uuid
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
REPAIR_GUARD='1da7fbfb6e5651ba6b6f8ac61215426ca75abcd0bc2ff26e0d187faf523c69b4'
CANDIDATE='b6b7c4504ab02bc20c83f90688f9038ff9d118b616ed41f2c7db4eff59e53ff8'
CLI='4611d22aa891e9b6368bc31cd6f7432635fc1dc0cb1ec4cf295a354e3a2c192d'
HARNESS='5c83d5e31803ddbdb6ea3026ec99f7ed460cf1c6db6e7f61f6c9bd77e1a6deba'
PENDING='tests/test_capella_bootstrap_archive_cas.cpp'
PENDING_SHA='b3d5b0c1328ad563852df097e94f82f948eeb3bde022087b6ac101601b6bcb37'
TARGETS={'nano-bootstrap-archive-maintenance':'tools/storage-maintenance/bootstrap-archive-owned-copies.cpp','nano-bootstrap-archive-maintenance-tests':'tests/test_bootstrap_archive_maintenance.cpp','nano-bootstrap-maintenance-retention-fault':'tests/support/bootstrap_maintenance_retention_fault.cpp'}
NEW_SHA={'tools/storage-maintenance/bootstrap-archive-owned-copies.cpp':'1b034acdd8b0cc177af5188eb2724e460012295248b645b48541bc30601d8743','tests/test_bootstrap_archive_maintenance.cpp':'f7c74c1b3f324843d12e2db4395e0c55b15dae69f94f60a93d97dcbe07765a63','tests/support/bootstrap_maintenance_retention_fault.cpp':'f398a0c14f22521b79ecb56775d3acba1d71a301de2d0745ddd94173cef2595c'}
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(s):
 p=pathlib.Path(s);need(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def main():
 ap=argparse.ArgumentParser()
 for k in ('controller-sha','guard','guard-sha','repair-guard','repair-guard-sha','parent-plan','parent-plan-sha','parent-terminal','parent-terminal-sha','pending-terminal','pending-terminal-sha','candidate','candidate-sha','cli','cli-sha','harness','harness-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();selfp=canonical(__file__);need(sha(selfp.read_bytes())==a.controller_sha,'external controller SHA');guardp=canonical(a.guard);gb=guardp.read_bytes();need(a.guard_sha==GUARD and sha(gb)==GUARD,'qualified guard exact');m=types.ModuleType('maintenance_public_guards');m.__file__=str(guardp);exec(compile(gb,str(guardp),'exec'),m.__dict__)
 def control(p,h):
  raw,s=m.observed(canonical(p),'arm64',16*1024*1024,True);need(sha(raw)==h,'external input SHA');return json.loads(raw)
 p=control(a.parent_plan,a.parent_plan_sha);arch=p['arch'];need(arch in m.IMAGES and p['image']==m.IMAGES[arch] and p['baselineGitCommit']==m.BASE and p['status']=='READY_ROOT_DISPATCH_ONLY','parent exact image/baseline');t=control(a.parent_terminal,a.parent_terminal_sha);q=control(a.pending_terminal,a.pending_terminal_sha);candidate=control(a.candidate,a.candidate_sha);cli=control(a.cli,a.cli_sha);harness=control(a.harness,a.harness_sha)
 repairp=canonical(a.repair_guard);need(a.repair_guard_sha==REPAIR_GUARD and sha(repairp.read_bytes())==a.repair_guard_sha,'qualified pending repair guard')
 need(t['format']=='nano-bootstrap-owner-runtime-incremental-build-terminal-v1' and t['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and t['arch']==arch and t['controllerSha256']==GUARD and t['planSha256']==a.parent_plan_sha and t['archiveMembers']==142 and len(t['coreObjects'])==142 and len(t['consumerObjects'])==3 and set(t['programs'])=={'nano','nano-capella-bootstrap-archive-cas-tests','nano-capella-bootstrap-startup-tests'} and t['sourceFiles']==1317 and t['allActualMdEqualPreflight'] and t['publicParentsAndSdkUnchanged'],'actual qualified142 parent topology')
 need(q['format']=='nano-pending-fixture-one-consumer-build-terminal-v1' and q['status']=='BUILD_PASS_PENDING_TEST_NOT_EXECUTED' and q['arch']==arch and q['controllerSha256']==a.repair_guard_sha and q['guardSha256']==GUARD and q['parentPlanSha256']==a.parent_plan_sha and q['parentTerminalSha256']==a.parent_terminal_sha and q['candidateSha256']==CANDIDATE and q['sourceSha256']==PENDING_SHA and q['sourceChanged']==[PENDING] and q['coreObjectsCompiled']==q['coreObjectsCopied']==q['archiveCommandsExecuted']==q['testsExecuted']==0 and q['consumerObjectsCompiled']==q['consumerLinks']==1 and q['archiveByteCopiedFromQualifiedParent'] and q['rawMemberProofReusedOnlyForByteIdenticalArchive'] and q['archiveMembers']==142 and q['archive']['sha256']==t['archive']['sha256'] and q['librariesGeneratedSdkAndAllParentsUnchanged'],'actual successful one-consumer continuation')
 need(a.candidate_sha==CANDIDATE and candidate['overlayCount']==11 and candidate['baselineRevision']==m.BASE and a.cli_sha==CLI and a.harness_sha==HARNESS and cli['toolTarget']=='nano-bootstrap-archive-maintenance' and harness['target']=='nano-bootstrap-archive-maintenance-tests' and cli['newCoreUnits']==harness['newCoreUnits']==0,'literal proposed source seals')
 parent=canonical(p['outputRoot']);need(parent==canonical(a.parent_terminal).parent.parent and parent.name=='run-02','exact recorded successful parent run');pb=parent/'build';origin=parent/'source';root=canonical(a.output);need(not root.exists() and root.parent.is_dir(),'new output namespace');inputs=[parent,canonical(a.parent_plan).parent,canonical(a.pending_terminal).parent.parent,canonical(a.candidate).parent,canonical(a.cli).parent,canonical(a.harness).parent,selfp.parent];need(not any(overlap(root,x) for x in inputs),'output protected-parent overlap')
 need(os.geteuid()==p['uid'] and os.getegid()==p['gid'] and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'public owner/environment');protected={};diagnostics=[]
 def pin(path,h=None,n=None,old=None):
  path=canonical(path);s=m.observed(path,arch);need((h is None or s['sha256']==h) and (n is None or s['st_size']==n),'public byte pin')
  if old:need(s['sha256']==old['sha256'] and all(s[k]==old[k] for k in m.material(arch)),'qualified material pin')
  protected[str(path)]=s;return s
 def read(path):
  b,s=m.observed(canonical(path),arch,16*1024*1024,True);pin(path,s['sha256'],s['st_size']);return b
 def doc(path):return json.loads(read(path))
 def same(path,s):
  z=m.observed(path,arch);need(z['sha256']==s['sha256'] and all(z[k]==s[k] for k in m.material(arch)),'protected material readback')
  if z['st_ctime_ns']!=s['st_ctime_ns']:diagnostics.append({'path':str(path),'old':s['st_ctime_ns'],'new':z['st_ctime_ns'],'materialAndBytesEqual':True,'causeMeasured':False})
 for path,h in [(selfp,a.controller_sha),(guardp,GUARD),(repairp,a.repair_guard_sha),(a.parent_plan,a.parent_plan_sha),(a.parent_terminal,a.parent_terminal_sha),(a.pending_terminal,a.pending_terminal_sha),(a.candidate,CANDIDATE),(a.cli,CLI),(a.harness,HARNESS)]:pin(path,h)
 oldsource=doc(parent/'receipts/source-before.json');oldpins={**p['sourcePins'],**p['overlays']};need(len(oldsource)==1317 and set(oldsource)==set(oldpins),'actual1317 source view');source_inputs={}
 for n,s in oldsource.items():m.relative(n);need(s['sha256']==oldpins[n]['sha256'] and s['st_size']==oldpins[n]['bytes'],'externally bound parent source');pin(origin/n,s['sha256'],s['st_size'],s);source_inputs[n]=origin/n
 for r in candidate['sourceFiles']:
  n=m.relative(r['path']);f=canonical(a.candidate).parent/r['sourcePath'];pin(f,r['sha256'],r['bytes']);need(n in oldsource and (n==PENDING or r['sha256']==oldsource[n]['sha256']),'only qualified pending source differs');source_inputs[n]=f
 need(sha(read(source_inputs[PENDING]))==PENDING_SHA and q['sourceFiles']==1317,'actual current pending source')
 basecm=read(source_inputs['CMakeLists.txt']);toolcm=read(canonical(a.cli).parent/'source/CMakeLists.txt');testcm=read(canonical(a.harness).parent/'source/CMakeLists.txt');registration=read(canonical(a.harness).parent/'registration.cmake');need(testcm==toolcm+registration,'exact test registration only')
 addition=b'# Explicit stopped-copy bootstrap archive maintenance; never starts runtime.\nadd_executable(nano-bootstrap-archive-maintenance tools/storage-maintenance/bootstrap-archive-owned-copies.cpp)\ntarget_link_libraries(nano-bootstrap-archive-maintenance PRIVATE nano_core)\n';undo=toolcm.replace(addition,b'',1).replace(b'nano nano-storage-maintenance nano-bootstrap-archive-maintenance PROPERTIES',b'nano nano-storage-maintenance PROPERTIES',1).replace(b'nano nano-storage-maintenance nano-bootstrap-archive-maintenance RUNTIME',b'nano nano-storage-maintenance RUNTIME',1);need(toolcm.count(addition)==1 and undo==basecm,'only maintenance install registration over qualified CMake')
 def sealed(owner,manifest,n):
  row=next(r for r in manifest['files'] if r['path']=='source/'+n);f=canonical(owner).parent/row['path'];pin(f,row['sha256'],row['bytes']);return f
 need(sealed(a.cli,cli,'CMakeLists.txt').read_bytes()==toolcm and sealed(a.harness,harness,'CMakeLists.txt').read_bytes()==testcm,'CMake author seals')
 source_inputs['CMakeLists.txt']=canonical(a.harness).parent/'source/CMakeLists.txt'
 for target,n in TARGETS.items():
  f=sealed(a.cli if target=='nano-bootstrap-archive-maintenance' else a.harness,cli if target=='nano-bootstrap-archive-maintenance' else harness,n);need(n not in source_inputs and pin(f)['sha256']==NEW_SHA[n],'specific new unit');source_inputs[n]=f
 need(len(source_inputs)==1320,'actual1317 plus three public sources')
 for table in ('coreObjects','consumerObjects','programs'):
  for n,s in t[table].items():pin(pb/n,s['sha256'],s['st_size'],s)
 archive=pin(pb/'libnano_core.a',t['archive']['sha256'],t['archive']['st_size'],t['archive']);proof=doc(parent/'receipts/archive-142-raw-member-proof.json');members=proof['members'];need(proof['archiveBeforeProof']['sha256']==archive['sha256'] and len(members)==142 and {r['object'] for r in members}==set(t['coreObjects']) and len({r['member'] for r in members})==142,'actual142 raw member topology')
 for r in members:need(r['sha256']==t['coreObjects'][r['object']]['sha256'] and r['bytes']==t['coreObjects'][r['object']]['st_size'],'actual raw-member/object binding')
 selection=doc(parent/'receipts/selection.json');fresh=set(selection['freshCore']);need(selection['actualMdBased'] and p['newCore'] in fresh,'parent compiler-MD provenance')
 for r in p['core']:
  md=pb/(r['object']+'.d') if r['source'] in fresh else canonical(r['mdInput']['path']);pin(md);need(r['source'] in fresh or protected[str(md)]['sha256']==r['mdInput']['sha256'],'actual ancestral MD')
 pin(pb/('CMakeFiles/nano_core.dir/'+p['newCore']+'.o.d'))
 for n in t['consumerObjects']:pin(pb/(n+'.d'))
 for table in ('libraries','generated'):
  for n,r in p[table].items():pin(pb/n,r['sha256'],r['bytes'])
 oldcc=doc(pb/'compile_commands.json');metadata=['compile_commands.json','CMakeFiles/nano_core.dir/flags.make','CMakeFiles/nano_core.dir/link.txt']+[f'CMakeFiles/{target}.dir/{leaf}' for target in t['programs'] for leaf in ('flags.make','link.txt')]
 for n in metadata:pin(pb/n)
 sb=read(parent/'receipts/sdk-before.log');sa=read(parent/'receipts/sdk-after.log');binding=doc(parent/'receipts/sdk-binding.json');need(sb==sa and sha(sb)==binding['beforeSha256']==binding['afterSha256'],'actual parent SDK');sdk={n:h for h,n in (l.split('  ',1) for l in sb.decode().splitlines())};need(len(sdk)==binding['paths'] and all(sdk.get(n)==h for n,h in p['sdkHashes'].items()),'SDK historical pins')
 pendingroot=canonical(a.pending_terminal).parent.parent;pendingbuild=pendingroot/'build';need(pin(pendingbuild/'libnano_core.a',q['archive']['sha256'],q['archive']['st_size'],q['archive'])['sha256']==archive['sha256'],'continuation byte-identical archive')
 pin(pendingbuild/q['object']['path'],q['object']['sha256'],q['object']['st_size'],q['object']);pin(pendingbuild/q['program']['path'],q['program']['sha256'],q['program']['st_size'],q['program']);pin(pendingbuild/q['actualMd']['path'],q['actualMd']['sha256'],q['actualMd']['st_size'],q['actualMd']);need(read(pendingroot/'receipts/sdk-before.log')==read(pendingroot/'receipts/sdk-after.log')==sb,'qualified one-consumer SDK unchanged')
 os.mkdir(root,0o700);source=root/'source';build=root/'build';controls=root/'controls';receipts=root/'receipts'
 for d in (source,build,controls,receipts):d.mkdir(mode=0o700)
 m.save(receipts/'initial-empty-output.json',{'sourceEmpty':not any(source.iterdir()),'buildEmpty':not any(build.iterdir()),'beforeAnyCopy':True});(build/'tmp').mkdir(mode=0o700);sourcebefore={};copies={};workers={};failure=None;start=time.monotonic();env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};objects={};programs={};compiled=0;linked=0
 def cp(path,dst):
  b,s=m.observed(canonical(path),arch,None,True);expected=protected[str(path)];need(s['sha256']==expected['sha256'] and all(s[k]==expected[k] for k in m.material(arch)),'exact held copied buffer matches qualified pin');same(path,expected);dst.parent.mkdir(parents=True,exist_ok=True);fd=os.open(dst,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,stat.S_IMODE(s['st_mode']))
  with os.fdopen(fd,'wb') as out:out.write(b);out.flush();os.fsync(out.fileno())
  z=m.observed(dst,arch);need(z['sha256']==s['sha256'] and z['st_size']==s['st_size'],'byte-identical new copy');return z
 def host(label,argv,seconds):
  m.save(receipts/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds});now=time.monotonic()
  with (receipts/(label+'.log')).open('xb',buffering=0) as out:
   try:r=subprocess.run(argv,stdout=out,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,1200-(time.monotonic()-start))))
   except subprocess.TimeoutExpired:m.save(receipts/(label+'.exit.json'),{'exitCode':None,'timedOut':True});raise
  m.save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-now});need(r.returncode==0,'phase refused '+label);print(json.dumps({'phase':label,'elapsedSeconds':time.monotonic()-start}),flush=True);return (receipts/(label+'.log')).read_bytes()
 def run(label,args,seconds):
  need(time.monotonic()-start<1200,'finite total1200s');token=uuid.uuid4().hex;name='nano-maint-build-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name];cmd=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.maintenance.build.owner='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/outputs/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs','--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',args[0],p['image']]+args[1:]
  try:return host(label,cmd,seconds)
  finally:
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=15)
   if z.returncode==0:need(json.loads(z.stdout).get('nano.maintenance.build.owner')==token,'owned cleanup');need(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=15).returncode==0,'owned removal')
   else:need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'owned state')
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=15);need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'owned container remains')
 def worker(label,commands,seconds):
  body='#!/bin/sh\nset -eu\n'
  for name,argv,limit in commands:body+='printf '+shlex.quote('%s\n')+' '+shlex.quote(name)+'\n/usr/bin/timeout --signal=TERM --kill-after=5s '+str(limit)+'s '+shlex.join(argv)+'\n'
  f=controls/(label+'.sh');f.write_text(body);workers[str(f)]=m.observed(f,arch);m.save(receipts/(label+'-actual-argv.json'),commands);return run(label,['/bin/sh','/controls/'+f.name],seconds)
 try:
  image=json.loads(host('image-inspect',p['dockerArgv']+['image','inspect','--format','{{json .}}',p['image']],25));need(image['Id']==p['image'] and image['Architecture']==arch and image['Os']=='linux' and not any(v.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for v in image.get('Config',{}).get('Env',[])),'exact public existing image')
  for n,f in source_inputs.items():sourcebefore[n]=cp(f,source/n)
  m.save(receipts/'source-before.json',sourcebefore);m.save(receipts/'parents-before.json',protected)
  run('configure',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],120)
  cc=json.loads(read(build/'compile_commands.json'));key=lambda r:(r['file'],r.get('output'),r['directory']);old={key(r):r for r in oldcc};current={key(r):r for r in cc};need(len(current)==len(old)+3 and all(current.get(k)==r for k,r in old.items()),'only three new compiler argv; all existing commands unchanged')
  for n in metadata[1:]:need(read(build/n)==read(pb/n),'old core/consumer flags/link byte-identical')
  copies['libnano_core.a']=cp(pb/'libnano_core.a',build/'libnano_core.a');archivepin=copies['libnano_core.a']
  for n in p['libraries']:copies[n]=cp(pb/n,build/n)
  for n,r in p['generated'].items():need(pin(build/n,r['sha256'],r['bytes'])['sha256']==r['sha256'],'generated KZG byte identity');copies[n]=m.observed(build/n,arch)
  units={target:m.unit(next(r for r in cc if r['file']=='/work/'+src and m.unit(r)[0].startswith('CMakeFiles/'+target+'.dir/'))) for target,src in TARGETS.items()};proberows=[]
  for target,(obj,argv) in units.items():(build/'preflight'/(obj+'.d')).parent.mkdir(parents=True,exist_ok=True);proberows.append((target,m.dependency_argv(argv,obj,True),60))
  worker('dependency-preflight',proberows,240);probes={obj:m.dep(read(build/'preflight'/(obj+'.d'))) for obj,_ in units.values()};sysfiles=set(sdk)|{'/usr/bin/clang++','/usr/bin/ar','/usr/bin/ranlib','/usr/bin/cmake'}
  for ds in probes.values():
   for d in ds:
    if d.startswith('/work/'):need(m.relative(posixpath.normpath(d[6:])) in sourcebefore,'new project dependency closure')
    elif d.startswith('/outputs/'):need(m.relative(posixpath.normpath(d[9:])) in copies,'new generated dependency closure')
    else:sysfiles.add(d)
  links={};newmeta={}
  for target,(obj,argv) in units.items():
   for leaf in ('flags.make','link.txt'):newmeta[f'CMakeFiles/{target}.dir/{leaf}']=m.observed(build/f'CMakeFiles/{target}.dir/{leaf}',arch)
   link=shlex.split(read(build/f'CMakeFiles/{target}.dir/link.txt').decode());need(link[0]=='/usr/bin/clang++' and link.count('-o')==1 and obj in link,'new exact link argv');output=m.relative(link[link.index('-o')+1]);need(output==(target if target!='nano-bootstrap-maintenance-retention-fault' else 'lib'+target+'.so'),'three expected binary names');need(link.count('libnano_core.a')==(0 if target=='nano-bootstrap-maintenance-retention-fault' else 1),'core only for tool/test, shim standalone');need(all(v in {obj,*p['libraries']} for v in link if v.endswith('.o')),'direct-object operand closure');links[target]=(output,link);sysfiles.update(v for v in link if v.startswith(('/usr/','/lib/')))
  resolver=controls/'dl-resolver.sh';resolver.write_text('#!/bin/sh\nset -eu\n/usr/bin/clang++ -print-file-name=libdl.so\n/usr/bin/clang++ -print-file-name=libdl.a\n');workers[str(resolver)]=m.observed(resolver,arch);dlrows=run('resolve-new-dl',['/bin/sh','/controls/'+resolver.name],30).decode().splitlines();need(len(dlrows)==2,'actual dl resolver two rows');resolved=[]
  for rawdl,basename in zip(dlrows,['libdl.so','libdl.a']):
   if rawdl==basename:continue
   dl=posixpath.normpath(rawdl);need(rawdl.startswith(('/usr/','/lib/')) and dl.startswith(('/usr/','/lib/')) and '\0' not in rawdl,'actual dl resolution contained');sysfiles.add(rawdl);resolved.append({'raw':rawdl,'normalized':dl})
  need(resolved,'a real dl link input resolved');m.save(receipts/'new-dl-resolver-binding.json',{'resolved':resolved,'observedCurrentOnly':True,'historicalSdkObservationNotInvented':True})
  sdkfile=controls/'sdk.sh';sdkfile.write_text('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(sysfiles)));workers[str(sdkfile)]=m.observed(sdkfile,arch);actualsdk=run('sdk-before',['/bin/sh','/controls/sdk.sh'],120);observed_sdk={n:h for h,n in (l.split('  ',1) for l in actualsdk.decode().splitlines())};need(all(observed_sdk.get(n)==h for n,h in sdk.items()) and set(observed_sdk)==sysfiles,'parent/new compiler/link SDK bytes beforecodegen')
  names=run('archive-names',['/usr/bin/ar','t','libnano_core.a'],60).decode().splitlines();byname={r['member']:r for r in members};need(len(names)==142 and len(set(names))==142 and set(names)==set(byname),'current byte-copied archive142 membership')
  body='#!/bin/sh\nset -eu\n'
  for name in names:need(re.fullmatch('[A-Za-z0-9_.+-]+',name),'safe member');body+='/usr/bin/ar p /outputs/libnano_core.a '+shlex.quote(name)+' > /outputs/tmp/member\nprintf '+shlex.quote(name+' ')+'\n/usr/bin/sha256sum /outputs/tmp/member\n/usr/bin/wc -c < /outputs/tmp/member\n'
  f=controls/'raw-member-proof.sh';f.write_text(body);workers[str(f)]=m.observed(f,arch);raw=run('archive-raw-byte-proof',['/bin/sh','/controls/'+f.name],120).decode().splitlines();need(len(raw)==284,'142 actual hash/size pairs');actual=[]
  for i in range(0,len(raw),2):name,h,_=raw[i].split(' ',2);need(h==byname[name]['sha256'] and int(raw[i+1])==byname[name]['bytes'],'actual raw bytes equal qualified core object');actual.append(byname[name])
  same(build/'libnano_core.a',archivepin);m.save(receipts/'archive-142-actual-byte-proof.json',{'archiveBeforeProof':archivepin,'members':actual,'archiveCreatedOrModified':False,'parentProofByteCopiedArchiveOnly':True})
  compilerows=[]
  for target,(obj,argv) in units.items():(build/obj).parent.mkdir(parents=True,exist_ok=True);compilerows.append((target,m.dependency_argv(argv,obj,False),180))
  worker('compile-three-new',compilerows,600);compiled=3
  for target,(obj,argv) in units.items():need(m.dep(read(build/(obj+'.d')))==probes[obj],'actual MD equals actual-M');objects[obj]=m.observed(build/obj,arch)
  same(build/'libnano_core.a',archivepin);worker('link-three-new',[(target,argv,120) for target,(name,argv) in links.items()],400);linked=3
  for target,(name,argv) in links.items():programs[name]=m.observed(build/name,arch)
  same(build/'libnano_core.a',archivepin);need(run('sdk-after',['/bin/sh','/controls/sdk.sh'],120)==actualsdk,'actual SDK bytes unchanged');m.save(receipts/'sdk-binding.json',{'beforeSha256':sha(actualsdk),'afterSha256':sha(actualsdk),'paths':len(sysfiles),'equal':True})
  for path,s in protected.items():same(path,s)
  for n,s in sourcebefore.items():same(source/n,s)
  for n,s in {**copies,**objects,**programs,**newmeta}.items():same(build/n,s)
  for path,s in workers.items():same(path,s)
  m.save(receipts/'parents-after.json',protected);m.save(receipts/'ctime-diagnostics.json',diagnostics)
  result={'format':'nano-bootstrap-maintenance-three-target-build-terminal-v1','status':'BUILD_PASS_MAINTENANCE_TEST_NOT_EXECUTED','arch':arch,'controllerSha256':a.controller_sha,'guardSha256':GUARD,'repairGuardSha256':a.repair_guard_sha,'parentPlanSha256':a.parent_plan_sha,'parentTerminalSha256':a.parent_terminal_sha,'pendingTerminalSha256':a.pending_terminal_sha,'candidateSha256':CANDIDATE,'cliProposalSha256':CLI,'harnessProposalSha256':HARNESS,'sourceFiles':1320,'coreObjectsCompiled':0,'coreObjectsCopied':0,'parentCoreObjectsReobserved':142,'archiveMembers':142,'archive':archivepin,'archiveByteCopiedFromQualifiedParent':True,'archiveConstructionCommandsExecuted':0,'archiveVerificationCommandsExecuted':143,'newObjectsCompiled':3,'newLinks':3,'oldCompilerCommandsUnchanged':True,'allActualMdEqualPreflight':True,'objects':objects,'programs':programs,'sourceSdkLibrariesGeneratedAndParentsUnchanged':True,'imageBuiltOrPulled':False,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
 except BaseException as e:
  failure=type(e).__name__+': '+str(e);result={'format':'nano-bootstrap-maintenance-three-target-build-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','arch':arch,'failure':failure,'completedObjects':compiled,'completedLinks':linked,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
  try:
   for path,s in protected.items():same(path,s)
   result['parentsUnchangedAfterFailure']=True;m.save(receipts/'parents-after-failure.json',protected)
  except BaseException:result['parentsUnchangedAfterFailure']=False
 m.save(receipts/'terminal-build.json',result);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
