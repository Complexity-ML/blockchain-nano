#!/usr/bin/env python3
"""ROOT-only public diagnostic build: one CPP and one link, no execution."""
import argparse,hashlib,json,os,pathlib,posixpath,re,shlex,stat,subprocess,sys,time,types,uuid
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
SOURCE='d496f9e5ea94579cec22dc4f9cc43288b42d203e5704284a17cf88928784251f'
COMPOSITE='6ea1caeb48ff1e5fa4ff6b0126dfff35a157203c3e25fdba8c1140baca06d6b5'
PARENTS={
 'arm64':('f988e9e18838cdef127619f1c68bd2109272e855e3f88bbb1bfe04665077e2fe','8b81e4facd6df1f4f76a27c1877f835f9c15001be87384af45453d4d169cf759','ad5570c57d3a8977d43d62a6b481000f6cfb4d603399c89bf9b9814564744723',501,20),
 'amd64':('c7d21478473df6c72d86b99269ed7260998df6e3c8b8a9f4f531301c40ae9c36','fb1d5f4accaa00b35d5292d275fb8a4f1855c3c02136a8acdeb1582f67bb8e5c','2ed1dc963f9313da8201a2fa2bf105e0a791efbffe49501f195832904eaf61b2',1000,1000)}
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(p):
 p=pathlib.Path(p);need(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def dependencies(b):
 s=b.decode();need(':' in s,'MD target');rows=sorted(set(shlex.split(s.replace('\\\n',' ').split(':',1)[1])));need(rows,'nonempty MD')
 for n in rows:need(n.startswith(('/work/','/outputs/','/diagnostic/','/usr/','/lib/')) and (posixpath.normpath(n).startswith(('/usr/','/lib/')) if n.startswith(('/usr/','/lib/')) else posixpath.normpath(n)==n) and '\n' not in n and '\0' not in n,'MD namespace')
 return rows
def main():
 if not __debug__:raise RuntimeError('Optimized Python refused')
 ap=argparse.ArgumentParser();ap.add_argument('--arch',choices=PARENTS,required=True)
 for k in ('controller-sha','guard','guard-sha','parent-run','terminal-sha','metadata-sha','parent-plan','parent-plan-sha','source','source-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();arch=a.arch;th,mh,ah,uid,gid=PARENTS[arch]
 need(all(re.fullmatch('[0-9a-f]{64}',v) for k,v in vars(a).items() if k.endswith('_sha')),'literal external SHA format')
 need(a.terminal_sha==th and a.metadata_sha==mh and a.source_sha==SOURCE and a.guard_sha==GUARD,'specific qualified parent and frozen source')
 need(os.geteuid()==uid and os.getegid()==gid and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'owner and public environment')
 selfp=canonical(__file__);gp=canonical(a.guard);gb=gp.read_bytes();need(sha(gb)==GUARD,'qualified guard SHA');m=types.ModuleType('utc_build_guards');m.__file__=str(gp);exec(compile(gb,str(gp),'exec'),m.__dict__)
 root=canonical(a.output);pr=canonical(a.parent_run);ps=pr/'source';pb=pr/'build';src=canonical(a.source);plan=canonical(a.parent_plan)
 need(pr.name=='run-01' and pr.parent.name=='bootstrap-replica-composite-v2-'+arch+'-20261014','exact public parent namespace')
 need(src.name=='project-current-utc.cpp' and not root.exists() and root.parent.is_dir() and not any(overlap(root,x) for x in (pr,src.parent,selfp.parent,gp.parent,plan.parent)),'fresh separate output')
 protected={};diagnostics=[];began=time.monotonic()
 def pin(path,h=None,n=None,old=None):
  z=m.observed(canonical(path),arch);need((h is None or z['sha256']==h) and (n is None or z['st_size']==n),'public byte pin')
  if old:need(z['sha256']==old['sha256'] and all(z[k]==old[k] for k in m.material(arch)),'parent historical material')
  protected[str(path)]=z;return z
 def read(path,h=None,limit=16*1024*1024):
  b,z=m.observed(canonical(path),arch,limit,True);need(h is None or sha(b)==h,'same held bytes SHA');pin(path,z['sha256'],z['st_size']);return b
 def same(path,z):
  now=m.observed(path,arch);need(now['sha256']==z['sha256'] and all(now[k]==z[k] for k in m.material(arch)),'protected input changed')
  if now['st_ctime_ns']!=z['st_ctime_ns']:diagnostics.append({'path':str(path),'old':z['st_ctime_ns'],'new':now['st_ctime_ns'],'materialAndBytesEqual':True,'causeMeasured':False})
 read(selfp,a.controller_sha,1024*1024);read(gp,GUARD,1024*1024);source_bytes=read(src,SOURCE,1024*1024);p=json.loads(read(plan,a.parent_plan_sha));t=json.loads(read(pr/'receipts/terminal-build.json',th));md=json.loads(read(pr/'root-build-public-bindings.json',mh))
 need(p['format']=='nano-bootstrap-owner-runtime-incremental-plan-v1' and p['arch']==arch and p['image']==m.IMAGES[arch] and (p['uid'],p['gid'])==(uid,gid),'original image/owner policy')
 need(t['format']=='nano-bootstrap-readonly-replica-deadline-composite-build-terminal-v1' and t['status']=='BUILD_PASS_REPLICA_AND_WRAPPER_TESTS_NOT_EXECUTED' and t['arch']==arch and t['controllerSha256']==COMPOSITE and t['guardSha256']==GUARD and t['parentPlanSha256']==a.parent_plan_sha and t['sourceFiles']==1324 and t['archiveMembers']==142 and t['archive']['sha256']==ah and t['sourceSdkLibrariesGeneratedAndParentsUnchanged'] and t['allActualMdEqualPreflight'],'actual qualified composite terminal')
 need(md['format']=='nano-bootstrap-readonly-replica-composite-public-build-bindings-v1' and md['terminalSha256']==th and md['buildControllerSha256']==COMPOSITE and md['parentPlanSha256']==a.parent_plan_sha and len(md['files'])==32 and len({x['path'] for x in md['files']})==32,'actual bound32 metadata')
 bound={}
 for row in md['files']:
  need(set(row)=={'path','sha256','bytes'},'metadata row shape');n=m.relative(row['path']);bound[n]=read(pr/n,row['sha256']);need(len(bound[n])==row['bytes'],'metadata exact bytes')
 source_map=json.loads(bound['receipts/source-before.json']);need(len(source_map)==1324,'full parent source view')
 for n,z in source_map.items():m.relative(n);pin(ps/n,z['sha256'],z['st_size'],z)
 generated=json.loads(bound['receipts/generated-metadata-before-compilation.json'])
 for n,z in generated.items():m.relative(n);pin(pb/n,z['sha256'],z['st_size'],z)
 for table in ('libraries','generated'):
  for n,z in p[table].items():m.relative(n);pin(pb/n,z['sha256'],z['bytes'])
 need(len(p['libraries'])==7 and {'third_party/mcl/bint64.o','third_party/mcl/base64.o'}<=set(p['libraries']),'seven dependency artifacts incl two explicit MCL objects')
 pin(pb/'libnano_core.a',ah,t['archive']['st_size'],t['archive']);raw=json.loads(bound['receipts/archive-142-actual-byte-proof.json']);members=raw['members']
 need(raw['archiveBeforeProof']['sha256']==ah and len(members)==142 and len({r['member'] for r in members})==len({r['object'] for r in members})==142,'historical actual142 raw proof')
 cc=json.loads(bound['build/compile_commands.json']);core={m.unit(r)[0] for r in cc if 'CMakeFiles/nano_core.dir/' in r['command']};need(core=={r['object'] for r in members},'complete142 actual compiler object identities')
 for row in members:need(pathlib.PurePosixPath(row['object']).name==row['member'],'unique member basename');pin(pb/m.relative(row['object']),row['sha256'],row['bytes'])
 for table in ('programs','newDeadlineProgramsPreserved','objects','objectsReused'):
  for n,z in t[table].items():pin(pb/m.relative(n),z['sha256'],z['st_size'],z)
 pin(pb/'libnano-bootstrap-maintenance-retention-fault.so',t['shim']['sha256'],t['shim']['st_size'],t['shim'])
 old_sdk=bound['receipts/sdk-before.log'];sdk_doc=json.loads(bound['receipts/sdk-binding.json']);need(old_sdk==bound['receipts/sdk-after.log'] and sha(old_sdk)==sdk_doc['beforeSha256']==sdk_doc['afterSha256'] and sdk_doc['equal'],'actual parent SDK equality')
 sdk_rows=[s.split('  ',1) for s in old_sdk.decode().splitlines()];sdk={n:h for h,n in sdk_rows};need(len(sdk)==len(sdk_rows)==sdk_doc['paths'] and all(re.fullmatch('[0-9a-f]{64}',h) for h in sdk.values()),'unique SDK hash rows')
 row=[r for r in cc if r['file']=='/work/src/consensus_capella_clock.cpp' and m.unit(r)[0]=='CMakeFiles/nano_core.dir/src/consensus_capella_clock.cpp.o'];need(len(row)==1,'actual clock compiler recipe');old_obj,argv=m.unit(row[0]);need(argv.count('/work/src/consensus_capella_clock.cpp')==1,'sole old TU')
 obj='/probe/project-current-utc.o';argv[argv.index('/work/src/consensus_capella_clock.cpp')]='/diagnostic/project-current-utc.cpp';argv[argv.index('-o')+1]=obj
 clean=[];i=0
 while i<len(argv):
  if argv[i] in ('-MF','-MT','-MQ'):i+=2;continue
  if argv[i] in ('-MD','-MMD','-MP'):i+=1;continue
  clean.append(argv[i]);i+=1
 probe=list(clean);probe[probe.index('-c')]='-M';probe[probe.index('-o')+1]='/probe/preflight.d';probe+=['-MT',obj];compile_args=clean+['-MD','-MF',obj+'.d','-MT',obj]
 old_link=read(pb/'CMakeFiles/nano.dir/link.txt');old_flags=read(pb/'CMakeFiles/nano_core.dir/flags.make');read(pb/'CMakeFiles/nano.dir/flags.make');link=shlex.split(old_link.decode());main_obj='CMakeFiles/nano.dir/src/main.cpp.o'
 need(link[0]=='/usr/bin/clang++' and link.count('-o')==link.count(main_obj)==link.count('libnano_core.a')==1 and link[link.index('-o')+1]=='nano','actual nano link recipe')
 need({x for x in link if x.endswith('.o')}=={main_obj,'third_party/mcl/bint64.o','third_party/mcl/base64.o'},'exact three direct parent objects');pin(pb/main_obj)
 link[link.index(main_obj)]=obj;link[link.index('-o')+1]='/probe/project-current-utc';system=set(sdk)|{'/usr/bin/clang++','/usr/bin/sha256sum','/bin/sh'}
 for x in link:
  if x.startswith(('/usr/','/lib/')):system.add(x)
  elif x.endswith(('.a','.o','.so')) and not x.startswith('/probe/'):need(x in p['libraries'] or x=='libnano_core.a','bound relative linker operand')
 os.mkdir(root,0o700);receipts=root/'receipts';out=root/'build';controls=root/'controls'
 for d in (receipts,out,controls):d.mkdir(mode=0o700)
 m.save(receipts/'initial-empty-output.json',{'outputWasAbsent':True,'outputBuildEntriesBeforeAnyWrite':sorted(x.name for x in out.iterdir()),'parentSourceCopied':False,'parentBuildCopied':False});(out/'tmp').mkdir(mode=0o700)
 m.save(receipts/'parents-before.json',protected);m.save(receipts/'derived-commands.json',{'parentCompileRow':row[0],'parentLinkSha256':sha(old_link),'parentCoreFlagsSha256':sha(old_flags),'probe':probe,'compile':compile_args,'link':link,'substitutionOnly':'TU/object/-o plus genuine MD options'})
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};workers={};counts={'consumerCpp':0,'links':0};failure=None;fresh={}
 def command(label,args,seconds):
  remaining=900-(time.monotonic()-began);need(remaining>0,'finite900s total');m.save(receipts/(label+'.command.json'),{'argv':args,'timeoutSeconds':seconds});at=time.monotonic()
  try:
   with (receipts/(label+'.log')).open('xb',buffering=0) as log:r=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,remaining))
   m.save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-at});need(r.returncode==0,'phase failed '+label);return m.observed(receipts/(label+'.log'),arch,8*1024*1024,True)[0]
  except subprocess.TimeoutExpired:m.save(receipts/(label+'.exit.json'),{'exitCode':None,'timedOut':True});raise
 def container(label,args,seconds):
  token=uuid.uuid4().hex;name='nano-utc-build-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name]
  run=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.utc.build.owner='+token,'--network','none','--read-only','--user',str(uid)+':'+str(gid),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--tmpfs','/tmp:rw,noexec,nosuid,size=128m','--env','TMPDIR=/probe/tmp','--mount',f'type=bind,src={ps},dst=/work,readonly','--mount',f'type=bind,src={pb},dst=/outputs,readonly','--mount',f'type=bind,src={src},dst=/diagnostic/project-current-utc.cpp,readonly','--mount',f'type=bind,src={out},dst=/probe','--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',args[0],p['image']]+args[1:]
  try:return command(label,run,seconds)
  finally:
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if z.returncode==0:need(json.loads(z.stdout).get('nano.utc.build.owner')==token,'owned cleanup');need(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'cleanup observation')
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'container remains')
 def worker(name,body):
  file=controls/name;data=('#!/bin/sh\nset -eu\n'+body).encode();fd=os.open(file,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
  with os.fdopen(fd,'wb') as f:f.write(data);f.flush();os.fsync(f.fileno())
  workers[str(file)]=m.observed(file,arch);return '/controls/'+name
 try:
  image=json.loads(command('image-inspect',p['dockerArgv']+['image','inspect','--format','{{json .}}',p['image']],20));need(image['Id']==p['image'] and image['Architecture']==arch and image['Os']=='linux' and not any(v.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for v in image.get('Config',{}).get('Env',[])),'existing pinned image')
  container('dependency-preflight',probe,60);deps=dependencies(m.observed(out/'preflight.d',arch,8*1024*1024,True)[0]);generated_inputs={}
  for dep in deps:
   if dep.startswith('/work/'):n=m.relative(dep[6:]);need(n in source_map,'project dependency in sealed source view');same(ps/n,protected[str(ps/n)])
   elif dep.startswith('/outputs/'):n=m.relative(dep[9:]);need(n in p['generated'],'declared generated header');generated_inputs[n]=protected[str(pb/n)]
   elif dep.startswith('/diagnostic/'):need(dep=='/diagnostic/project-current-utc.cpp','only new diagnostic input');same(src,protected[str(src)])
   else:system.add(dep)
  for n in system:need(n.startswith(('/usr/','/lib/','/bin/')) and posixpath.normpath(n).startswith(('/usr/','/lib/','/bin/')) and '\n' not in n and '\0' not in n,'SDK path namespace')
  sdk_worker=worker('sdk.sh',''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(system)));actual=container('sdk-before',['/bin/sh',sdk_worker],120);rows=[s.split('  ',1) for s in actual.decode().splitlines()];observed_sdk={n:h for h,n in rows};need(len(rows)==len(observed_sdk)==len(system) and all(observed_sdk.get(n)==h for n,h in sdk.items()),'actual old SDK pins and fresh union')
  m.save(receipts/'generated-inputs-before-compilation.json',generated_inputs);m.save(receipts/'dependency-binding.json',{'preflight':m.observed(out/'preflight.d',arch),'dependencies':deps,'sourceSha256':SOURCE,'sourceViewFiles':1324,'newHeaderInputsPresealed':True})
  for path,z in protected.items():same(path,z)
  container('compile-one-diagnostic',compile_args,90);counts['consumerCpp']=1;need(dependencies(m.observed(out/'project-current-utc.o.d',arch,8*1024*1024,True)[0])==deps,'actual MD equals genuine preflight');fresh['project-current-utc.o']=m.observed(out/'project-current-utc.o',arch,128*1024*1024);fresh['project-current-utc.o.d']=m.observed(out/'project-current-utc.o.d',arch,8*1024*1024);fresh['preflight.d']=m.observed(out/'preflight.d',arch,8*1024*1024)
  container('link-one-diagnostic',link,120);counts['links']=1;fresh['project-current-utc']=m.observed(out/'project-current-utc',arch,128*1024*1024);need(fresh['project-current-utc']['st_mode']&0o111,'new executable output')
  need(container('sdk-after',['/bin/sh',sdk_worker],120)==actual,'actual SDK unchanged');m.save(receipts/'sdk-binding.json',{'beforeSha256':sha(actual),'afterSha256':sha(actual),'paths':len(system),'equal':True})
  for path,z in protected.items():same(path,z)
  for path,z in workers.items():same(path,z)
  for n,z in fresh.items():same(out/n,z)
  m.save(receipts/'parents-after.json',protected);m.save(receipts/'ctime-diagnostics.json',diagnostics)
  result={'format':'nano-current-utc-projection-one-target-build-terminal-v1','status':'BUILD_PASS_DIAGNOSTIC_NOT_EXECUTED','arch':arch,'controllerSha256':a.controller_sha,'guardSha256':GUARD,'sourceSha256':SOURCE,'parentTerminalSha256':th,'parentMetadataSha256':mh,'parentPlanSha256':a.parent_plan_sha,'archiveSha256':ah,'archiveMembers':142,'sourceViewFiles':1324,'historicalRaw142ProofMatchedCurrentArchiveAndObjects':True,'coreObjectsCompiled':0,'archiveCommandsExecuted':0,'cmakeCommandsExecuted':0,'consumerCppCompiled':1,'linksExecuted':1,'allActualMdEqualPreflight':True,'sourceHeadersGeneratedSdkLibrariesAndParentsUnchanged':True,'object':fresh['project-current-utc.o'],'binary':fresh['project-current-utc'],'elapsedSeconds':time.monotonic()-began,'diagnosticExecuted':False,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
 except BaseException as e:
  failure=type(e).__name__+': '+str(e);result={'format':'nano-current-utc-projection-one-target-build-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','arch':arch,'failure':failure,'completedCounters':counts,'diagnosticExecuted':False,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
  try:
   for path,z in protected.items():same(path,z)
   for path,z in workers.items():same(path,z)
   result['protectedInputsUnchangedAfterFailure']=True;m.save(receipts/'parents-after-failure.json',protected)
  except BaseException:result['protectedInputsUnchangedAfterFailure']=False
 m.save(receipts/'terminal-build.json',result);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
