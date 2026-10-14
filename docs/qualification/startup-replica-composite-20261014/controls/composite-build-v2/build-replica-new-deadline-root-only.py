#!/usr/bin/env python3
"""ROOT only. Public parents; 0 core/ar, CLI+tiny CPP, three coherent links.
No tests, private data, validators, image builds/pulls or implicit retries.
"""
import argparse,hashlib,json,os,pathlib,posixpath,re,shlex,stat,subprocess,sys,time,types,uuid
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
DEADLINE='4b018ba8bd7dffb4c9f581addadedf73293785c2aae085221977477199ba4d17'
MAINT='ce27006e9c62118a4d4b82362a6b9e11a0ec103e4290627446143ea2432b80f5'
PROPOSAL='78a3e907bde06e43c05d79239297e261078cdcc2b8ed9d5fedae033b11cb22f8'
BASECM='d0b9232b4de7adb4a080e38737fb1989af4bdd3b1e3b9d885a4af42bacf1f6ac'
REPLICAREG='b366f990b95cc661379b08a57d3ccf43d7714eb69027b8626516c10ff8793e89'
DEADLINEREG='4d6c4445f7d7e5abb462a2de4b5b76b9ce908e4e1caf8ddcd29d9c5bf3f3bbaf'
CLI='nano-bootstrap-archive-maintenance';WRAPPER=CLI+'-tests';SHIM='nano-bootstrap-maintenance-retention-fault';TINY='nano-bootstrap-readonly-replica-tests'
CLISRC='tools/storage-maintenance/bootstrap-archive-owned-copies.cpp';TESTSRC='tests/test_bootstrap_readonly_replica.cpp';HEAD='tools/storage-maintenance/bootstrap-readonly-replica.hpp'
OLDTARGETS=[CLI,WRAPPER,SHIM];DEADTARGETS=['nano','nano-capella-bootstrap-startup-tests','nano-capella-startup-deadline-tests'];GRAPHS=['build/CTestTestfile.cmake','build/third_party/secp256k1/CTestTestfile.cmake','build/third_party/secp256k1/src/CTestTestfile.cmake']
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(s):
 p=pathlib.Path(s);need(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def main():
 if not __debug__:raise RuntimeError('Optimized Python refused')
 ap=argparse.ArgumentParser()
 for k in ('controller-sha','guard','guard-sha','parent-plan','parent-plan-sha','deadline-terminal','deadline-terminal-sha','deadline-metadata','deadline-metadata-sha','maintenance-terminal','maintenance-terminal-sha','maintenance-metadata','maintenance-metadata-sha','proposal','proposal-sha','deadline-registration','deadline-registration-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();need(all(re.fullmatch('[0-9a-f]{64}',v) for k,v in vars(a).items() if k.endswith('_sha')),'literal external SHAs');selfp=canonical(__file__);need(sha(selfp.read_bytes())==a.controller_sha,'controller SHA');gp=canonical(a.guard);gb=gp.read_bytes();need(a.guard_sha==GUARD and sha(gb)==GUARD,'qualified guard');m=types.ModuleType('replica_composite_guards');m.__file__=str(gp);exec(compile(gb,str(gp),'exec'),m.__dict__)
 def control(path,h):
  b,s=m.observed(canonical(path),'arm64',16*1024*1024,True);need(sha(b)==h,'external public control SHA');return json.loads(b)
 p=control(a.parent_plan,a.parent_plan_sha);arch=p['arch'];dt=control(a.deadline_terminal,a.deadline_terminal_sha);mt=control(a.maintenance_terminal,a.maintenance_terminal_sha);proposal=control(a.proposal,a.proposal_sha)
 need(arch in m.IMAGES and p['image']==m.IMAGES[arch] and p['format']=='nano-bootstrap-owner-runtime-incremental-plan-v1' and p['status']=='READY_ROOT_DISPATCH_ONLY','original full142 policy')
 need(dt['format']=='nano-capella-startup-deadline-incremental-build-terminal-v1' and dt['status']=='BUILD_PASS_STARTUP_DEADLINE_TESTS_NOT_EXECUTED' and dt['arch']==arch and dt['controllerSha256']==DEADLINE and dt['guardSha256']==GUARD and dt['parentPlanSha256']==a.parent_plan_sha and dt['sourceFiles']==1319 and dt['archiveMembers']==len(dt['coreObjects'])==142 and dt['coreFresh']==1 and dt['coreReused']==141 and dt['allFreshActualMdEqualPreflight'] and dt['publicSourceSdkLibrariesGeneratedAndParentsUnchanged'],'actual new deadline142 parent')
 need(mt['format']=='nano-bootstrap-maintenance-one-consumer-build-terminal-v1' and mt['status']=='BUILD_PASS_MAINTENANCE_TEST_NOT_EXECUTED' and mt['arch']==arch and mt['controllerSha256']==MAINT and mt['guardSha256']==GUARD and mt['parentPlanSha256']==a.parent_plan_sha and mt['sourceFiles']==1320 and mt['archiveMembers']==142 and len(mt['objects'])==3 and mt['allActualMdEqualPreflight'] and mt['sourceSdkLibrariesGeneratedAndParentsUnchanged'],'actual maintenance objects parent')
 need(a.proposal_sha==PROPOSAL and proposal['status']=='FROZEN_SOURCE_PROPOSAL_NOT_COMPILED_OR_EXECUTED' and {r['path'] for r in proposal['sourceFiles']}=={CLISRC,HEAD,TESTSRC,'CMakeLists.txt'},'frozen four-source V6')
 dr=canonical(a.deadline_terminal).parent.parent;mr=canonical(a.maintenance_terminal).parent.parent;db=dr/'build';mb=mr/'build';root=canonical(a.output);prop=canonical(a.proposal).parent
 need(dr.name==mr.name=='run-01' and canonical(a.deadline_metadata)==dr/'root-build-public-bindings.json' and canonical(a.maintenance_metadata)==mr/'root-build-public-bindings.json' and not overlap(dr,mr),'two concrete independent public parents');need(not root.exists() and root.parent.is_dir() and not any(overlap(root,q) for q in (dr,mr,prop,selfp.parent,canonical(a.parent_plan).parent)),'fresh isolated output');need(os.geteuid()==p['uid'] and os.getegid()==p['gid'] and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'owner/environment')
 protected={};diagnostics=[]
 def pin(path,h=None,n=None,old=None):
  path=canonical(path);z=m.observed(path,arch);need((h is None or z['sha256']==h) and (n is None or z['st_size']==n),'public byte pin')
  if old:need(z['sha256']==old['sha256'] and all(z[k]==old[k] for k in m.material(arch)),'actual parent material')
  protected[str(path)]=z;return z
 def read(path):
  b,z=m.observed(canonical(path),arch,16*1024*1024,True);pin(path,z['sha256'],z['st_size']);return b
 def same(path,z):
  x=m.observed(path,arch);need(x['sha256']==z['sha256'] and all(x[k]==z[k] for k in m.material(arch)),'protected readback')
  if x['st_ctime_ns']!=z['st_ctime_ns']:diagnostics.append({'path':str(path),'old':z['st_ctime_ns'],'new':x['st_ctime_ns'],'bytesAndMaterialEqual':True,'causeMeasured':False})
 for path,h in [(selfp,a.controller_sha),(gp,GUARD),(a.parent_plan,a.parent_plan_sha),(a.deadline_terminal,a.deadline_terminal_sha),(a.maintenance_terminal,a.maintenance_terminal_sha),(a.proposal,PROPOSAL)]:pin(path,h)
 def metadata(which,r,t,expected_format,controller,targets):
  f=getattr(a,which+'_metadata');h=getattr(a,which+'_metadata_sha');doc=control(f,h);pin(f,h);allow={'receipts/source-before.json','receipts/sdk-binding.json','receipts/sdk-before.log','receipts/sdk-after.log','receipts/image-inspect.log','build/compile_commands.json',*GRAPHS}
  allow.add('receipts/archive-142-raw-member-proof.json' if which=='deadline' else 'receipts/archive-142-actual-byte-proof.json')
  allow.update('build/CMakeFiles/'+target+'.dir/'+leaf for target in targets for leaf in ('flags.make','link.txt'))
  if which=='deadline':objs=['CMakeFiles/nano_core.dir/src/consensus_capella_runtime.cpp.o','CMakeFiles/nano-capella-startup-deadline-tests.dir/tests/test_capella_startup_deadline.cpp.o']
  else:objs=list(t['objects'])
  for obj in objs:allow.update({'build/'+obj+'.d','build/preflight/'+obj+'.d'})
  need(set(doc)=={'format','terminalSha256','buildControllerSha256','parentPlanSha256','files'} and doc['format']==expected_format and doc['terminalSha256']==getattr(a,which+'_terminal_sha') and doc['buildControllerSha256']==controller and doc['parentPlanSha256']==a.parent_plan_sha and len(doc['files'])==len(allow)==22 and {x['path'] for x in doc['files']}==allow,'exact externally bound public22')
  bound={}
  for row in doc['files']:
   need(set(row)=={'path','sha256','bytes'},'metadata shape');n=m.relative(row['path']);pin(r/n,row['sha256'],row['bytes']);bound[n]=read(r/n)
  return bound
 d=metadata('deadline',dr,dt,'nano-capella-startup-deadline-public-build-bindings-v1',DEADLINE,['nano_core',*DEADTARGETS]);v=metadata('maintenance',mr,mt,'nano-bootstrap-maintenance-public-build-bindings-v1',MAINT,OLDTARGETS);ds=json.loads(d['receipts/source-before.json']);ms=json.loads(v['receipts/source-before.json']);need(len(ds)==1319 and len(ms)==1320,'actual complete source maps');inputs={}
 for origin,rows in ((dr/'source',ds),(mr/'source',ms)):
  for n,z in rows.items():m.relative(n);pin(origin/n,z['sha256'],z['st_size'],z)
 inputs={n:dr/'source'/n for n in ds};extras=[CLISRC,'tests/test_bootstrap_archive_maintenance.cpp','tests/support/bootstrap_maintenance_retention_fault.cpp'];need(all(n not in ds and n in ms for n in extras),'exact maintenance extras')
 for n in extras:inputs[n]=mr/'source'/n
 replica=read(prop/'registration-block.txt');reg=read(a.deadline_registration);need(sha(replica)==REPLICAREG and len(replica)==577 and a.deadline_registration_sha==DEADLINEREG and sha(reg)==DEADLINEREG and len(reg)==515,'two registrations literal');basecm=read(mr/'source/CMakeLists.txt');need(sha(basecm)==BASECM and len(basecm)==160051,'current maintenance CMake base');derived=basecm+b'\n'+reg+b'\n'+replica;need(len(derived)==161145,'exact derived CMake size')
 for row in proposal['sourceFiles']:
  n=m.relative(row['path']);f=prop/'source'/n;pin(f,row['sha256'],row['bytes'])
  if n=='CMakeLists.txt':need(read(f)==basecm+b'\n'+replica,'proposal CMake baseline+one block')
  else:inputs[n]=f
 need(len(inputs)==1324 and inputs['CMakeLists.txt']==dr/'source/CMakeLists.txt','merged source1324; explicit CMake derivation follows')
 for table in ('coreObjects','consumerObjects','programs'):
  for n,z in dt[table].items():pin(db/n,z['sha256'],z['st_size'],z)
 for table in ('objects','programs'):
  for n,z in mt[table].items():pin(mb/n,z['sha256'],z['st_size'],z)
 pin(db/'libnano_core.a',dt['archive']['sha256'],dt['archive']['st_size'],dt['archive']);pin(mb/'libnano_core.a',mt['archive']['sha256'],mt['archive']['st_size'],mt['archive'])
 for directory in (db,mb):
  for table in ('libraries','generated'):
   for n,z in p[table].items():pin(directory/n,z['sha256'],z['bytes'])
 rawproof=json.loads(d['receipts/archive-142-raw-member-proof.json']);members=rawproof['members'];need(rawproof['archiveBeforeProof']['sha256']==dt['archive']['sha256'] and len(members)==142 and {r['object'] for r in members}==set(dt['coreObjects']) and len({r['member'] for r in members})==142,'actual new142 archive members')
 for row in members:need(row['sha256']==dt['coreObjects'][row['object']]['sha256'] and row['bytes']==dt['coreObjects'][row['object']]['st_size'],'raw member equals current142object')
 def sdk(bound):
  b=bound['receipts/sdk-before.log'];j=json.loads(bound['receipts/sdk-binding.json']);need(b==bound['receipts/sdk-after.log'] and sha(b)==j['beforeSha256']==j['afterSha256'] and j['equal'],'actual SDK before/after');rows=[x.split('  ',1) for x in b.decode().splitlines()];z={n:h for h,n in rows};need(len(z)==len(rows)==j['paths'],'SDK rows exact');return z
 oldsdk=sdk(d);msdk=sdk(v)
 for n,h in msdk.items():need(n not in oldsdk or oldsdk[n]==h,'two parent SDK collision');oldsdk[n]=h
 oldd=json.loads(d['build/compile_commands.json']);oldm=json.loads(v['build/compile_commands.json']);key=lambda r:(r['file'],r.get('output'),r['directory']);old={key(r):r for r in oldd}
 for r in oldm:need(key(r) not in old or old[key(r)]==r,'parent compiler command collision');old[key(r)]=r
 reuse={}
 for target in (WRAPPER,SHIM):
  obj=next(n for n in mt['objects'] if n.startswith('CMakeFiles/'+target+'.dir/'));deps=m.dep(v['build/'+obj+'.d']);need(deps==m.dep(v['build/preflight/'+obj+'.d']),'actual old MD equals preflight')
  for dependency in deps:
   if dependency.startswith('/work/'):
    n=m.relative(posixpath.normpath(dependency[6:]));need(n in inputs and n in ms and n!='CMakeLists.txt' and pin(inputs[n])['sha256']==ms[n]['sha256'],'unchanged source of reused wrapper/shim')
   elif dependency.startswith('/outputs/'):need(m.relative(posixpath.normpath(dependency[9:])) in p['generated'] or m.relative(posixpath.normpath(dependency[9:])) in p['libraries'],'known reused generated input')
   else:need(dependency in oldsdk,'reused SDK input')
  reuse[target]=(obj,deps)
 os.mkdir(root,0o700);source=root/'source';build=root/'build';controls=root/'controls';receipts=root/'receipts'
 for x in (source,build,controls,receipts):x.mkdir(mode=0o700)
 m.save(receipts/'initial-empty-output.json',{'sourceEmpty':not any(source.iterdir()),'buildEmpty':not any(build.iterdir()),'beforeAnyCopy':True});(build/'tmp').mkdir(mode=0o700);before={};copies={};workers={};diagnostics=[];compiled=0;linked=0;failed=None;start=time.monotonic();env={'PATH':'/usr/bin:/bin','LC_ALL':'C'}
 def write_owned(dst,b,mode=0o600):
  dst.parent.mkdir(parents=True,exist_ok=True);fd=os.open(dst,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,mode)
  with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
  return m.observed(dst,arch)
 def cp(path,dst):
  b,z=m.observed(canonical(path),arch,None,True);expected=protected[str(path)];need(z['sha256']==expected['sha256'] and all(z[k]==expected[k] for k in m.material(arch)),'copied held buffer exact');x=write_owned(dst,b,stat.S_IMODE(z['st_mode']));need(x['sha256']==z['sha256'] and x['st_size']==z['st_size'],'copy readback');return x
 def run(label,args,seconds):
  need(time.monotonic()-start<900,'finite900s batch');token=uuid.uuid4().hex;name='nano-replica-build-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name];argv=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.replica.build.owner='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/outputs/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs','--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',args[0],p['image']]+args[1:];m.save(receipts/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds});at=time.monotonic()
  try:
   with (receipts/(label+'.log')).open('xb',buffering=0) as log:r=subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,900-(time.monotonic()-start))))
   m.save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-at});need(r.returncode==0,'phase failed '+label);print(json.dumps({'phase':label,'elapsedSeconds':time.monotonic()-start}),flush=True);return (receipts/(label+'.log')).read_bytes()
  except subprocess.TimeoutExpired:m.save(receipts/(label+'.exit.json'),{'exitCode':None,'timedOut':True});raise
  finally:
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if z.returncode==0:need(json.loads(z.stdout).get('nano.replica.build.owner')==token,'owned cleanup');need(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'cleanup observation')
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'container remains')
 try:
  for n,f in inputs.items():before[n]=write_owned(source/n,derived) if n=='CMakeLists.txt' else cp(f,source/n)
  m.save(receipts/'source-before.json',before);m.save(receipts/'parents-before.json',protected);m.save(receipts/'derived-cmake.json',{'baseSha256':BASECM,'deadlineRegistrationSha256':DEADLINEREG,'replicaRegistrationSha256':REPLICAREG,'sha256':sha(derived),'bytes':len(derived),'order':'base+LF+deadline+LF+replica'})
  image_argv=p['dockerArgv']+['image','inspect','--format','{{json .}}',p['image']];m.save(receipts/'image-inspect.command.json',{'argv':image_argv,'timeoutSeconds':20})
  with (receipts/'image-inspect.log').open('xb',buffering=0) as log:image_result=subprocess.run(image_argv,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=20)
  m.save(receipts/'image-inspect.exit.json',{'exitCode':image_result.returncode,'timedOut':False});need(image_result.returncode==0,'actual pinned image inspection');image=json.loads((receipts/'image-inspect.log').read_bytes());need(image['Id']==p['image'] and image['Architecture']==arch and image['Os']=='linux' and not any(x.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for x in image.get('Config',{}).get('Env',[])),'existing pinned image without injected environment')
  run('configure',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],120)
  cc=json.loads((build/'compile_commands.json').read_bytes());now={key(r):r for r in cc};need(len(now)==len(old)+1 and all(now.get(k)==r for k,r in old.items()),'all parent argv unchanged; tiny sole addition')
  for bound,targets in [(d,['nano_core',*DEADTARGETS]),(v,OLDTARGETS)]:
   for target in targets:
    for leaf in ('flags.make','link.txt'):n='CMakeFiles/'+target+'.dir/'+leaf;need((build/n).read_bytes()==bound['build/'+n],'generated flags/link exact')
  generated_metadata={n:m.observed(build/n,arch) for n in ['compile_commands.json','CTestTestfile.cmake','CMakeFiles/nano_core.dir/flags.make','CMakeFiles/nano_core.dir/link.txt',*[f'CMakeFiles/{target}.dir/{leaf}' for target in [*DEADTARGETS,*OLDTARGETS,TINY] for leaf in ('flags.make','link.txt')]]};m.save(receipts/'generated-metadata-before-compilation.json',generated_metadata)
  for n,z in dt['coreObjects'].items():copies[n]=cp(db/n,build/n)
  copies['libnano_core.a']=cp(db/'libnano_core.a',build/'libnano_core.a')
  for n in dt['programs']:copies[n]=cp(db/n,build/n)
  for n in dt['consumerObjects']:copies[n]=cp(db/n,build/n)
  reused_md={}
  for target,(obj,deps) in reuse.items():
   copies[obj]=cp(mb/obj,build/obj)
   for n in (obj+'.d','preflight/'+obj+'.d'):copies[n]=cp(mb/n,build/n)
   reused_md[target]={'object':obj,'actualMdOrigin':str(mb/(obj+'.d')),'actualMdSha256':copies[obj+'.d']['sha256'],'preflightMdOrigin':str(mb/'preflight'/(obj+'.d')),'preflightMdSha256':copies['preflight/'+obj+'.d']['sha256'],'newMdGenerated':False,'dependencies':sorted(deps)}
  m.save(receipts/'reused-md-provenance.json',{'maintenanceTerminalSha256':a.maintenance_terminal_sha,'maintenanceMetadataSha256':a.maintenance_metadata_sha,'objects':reused_md})
  so='lib'+SHIM+'.so';copies[so]=cp(mb/so,build/so)
  for n in p['libraries']:copies[n]=cp(db/n,build/n)
  for n,z in p['generated'].items():pin(build/n,z['sha256'],z['bytes']);copies[n]=m.observed(build/n,arch)
  units={};sysfiles=set(oldsdk)|{'/usr/bin/clang++','/usr/bin/cmake','/usr/bin/ar','/usr/bin/sha256sum','/usr/bin/wc','/bin/sh'};probes={};generated=set();links={}
  for target,src in [(CLI,CLISRC),(TINY,TESTSRC)]:
   rows=[r for r in cc if r['file']=='/work/'+src and m.unit(r)[0].startswith('CMakeFiles/'+target+'.dir/')];need(len(rows)==1,'one selected new consumer');obj,argv=m.unit(rows[0]);units[target]=(obj,argv);(build/'preflight'/(obj+'.d')).parent.mkdir(parents=True,exist_ok=True);run('dependency-'+target,m.dependency_argv(argv,obj,True),90);probes[obj]=m.dep((build/'preflight'/(obj+'.d')).read_bytes())
   for dep in probes[obj]:
    if dep.startswith('/work/'):need(m.relative(posixpath.normpath(dep[6:])) in before,'fresh project closure')
    elif dep.startswith('/outputs/'):n=m.relative(posixpath.normpath(dep[9:]));need(n in copies,'fresh generated closure');generated.add(n)
    else:sysfiles.add(dep)
  for target,obj in [(x,units[x][0]) for x in units]+[(WRAPPER,reuse[WRAPPER][0])]:
   link=shlex.split((build/f'CMakeFiles/{target}.dir/link.txt').read_text());need(link[0]=='/usr/bin/clang++' and link.count('-o')==1 and link[link.index('-o')+1]==target and link.count('libnano_core.a')==1 and obj in link and all(x in {obj,*p['libraries']} for x in link if x.endswith('.o')),'exact coherent consumer link');links[target]=link;sysfiles.update(x for x in link if x.startswith(('/usr/','/lib/')))
  generated_before={n:m.observed(build/n,arch) for n in generated};m.save(receipts/'generated-inputs-before-compilation.json',generated_before)
  sdkfile=controls/'sdk.sh';workers[str(sdkfile)]=write_owned(sdkfile,('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(sysfiles))).encode());actualsdk=run('sdk-before',['/bin/sh','/controls/sdk.sh'],90);sd={n:h for h,n in (l.split('  ',1) for l in actualsdk.decode().splitlines())};need(set(sd)==sysfiles and all(sd.get(n)==h for n,h in oldsdk.items()),'parent union + fresh SDK before codegen')
  fresh={}
  for target,(obj,argv) in units.items():
   (build/obj).parent.mkdir(parents=True,exist_ok=True);run('compile-'+target,m.dependency_argv(argv,obj,False),180);compiled+=1;need(m.dep((build/(obj+'.d')).read_bytes())==probes[obj],'actual MD == genuine-M');fresh[obj]=m.observed(build/obj,arch);fresh[obj+'.d']=m.observed(build/(obj+'.d'),arch)
   for n,z in generated_before.items():same(build/n,z)
  names=run('archive-names',['/usr/bin/ar','t','libnano_core.a'],60).decode().splitlines();byname={r['member']:r for r in members};need(len(names)==142 and len(set(names))==142 and set(names)==set(byname),'actual142 member identities');body='#!/bin/sh\nset -eu\n'
  for name in names:need(re.fullmatch('[A-Za-z0-9_.+-]+',name),'safe member');body+='/usr/bin/ar p /outputs/libnano_core.a '+shlex.quote(name)+' > /outputs/tmp/member\nprintf '+shlex.quote(name+' ')+'\n/usr/bin/sha256sum /outputs/tmp/member\n/usr/bin/wc -c < /outputs/tmp/member\n'
  wf=controls/'raw-members.sh';workers[str(wf)]=write_owned(wf,body.encode());raw=run('archive-raw-byte-proof',['/bin/sh','/controls/raw-members.sh'],120).decode().splitlines();need(len(raw)==284,'142 actual member hash/size pairs');actual=[]
  for i in range(0,len(raw),2):name,h,_=raw[i].split(' ',2);obj=byname[name]['object'];need(h==copies[obj]['sha256'] and int(raw[i+1])==copies[obj]['st_size'],'actual member equals copied current142object');actual.append({'member':name,'object':obj,'sha256':h,'bytes':int(raw[i+1])})
  same(build/'libnano_core.a',copies['libnano_core.a']);m.save(receipts/'archive-142-actual-byte-proof.json',{'archiveBeforeProof':copies['libnano_core.a'],'members':actual,'archiveCreatedOrModified':False,'sourceDeadlineTerminalSha256':a.deadline_terminal_sha})
  programs={}
  for target,link in links.items():run('link-'+target,link,120);linked+=1;programs[target]=m.observed(build/target,arch);need(programs[target]['st_mode']&0o111,'executable consumer')
  need(run('sdk-after',['/bin/sh','/controls/sdk.sh'],90)==actualsdk,'actual SDK unchanged');m.save(receipts/'sdk-binding.json',{'beforeSha256':sha(actualsdk),'afterSha256':sha(actualsdk),'paths':len(sysfiles),'equal':True})
  for path,z in protected.items():same(path,z)
  for n,z in before.items():same(source/n,z)
  for n,z in {**copies,**fresh,**programs,**generated_metadata}.items():same(build/n,z)
  for path,z in workers.items():same(path,z)
  m.save(receipts/'parents-after.json',protected);m.save(receipts/'ctime-diagnostics.json',diagnostics)
  result={'format':'nano-bootstrap-readonly-replica-deadline-composite-build-terminal-v1','status':'BUILD_PASS_REPLICA_AND_WRAPPER_TESTS_NOT_EXECUTED','arch':arch,'controllerSha256':a.controller_sha,'guardSha256':GUARD,'parentPlanSha256':a.parent_plan_sha,'deadlineTerminalSha256':a.deadline_terminal_sha,'deadlineMetadataSha256':a.deadline_metadata_sha,'maintenanceTerminalSha256':a.maintenance_terminal_sha,'maintenanceMetadataSha256':a.maintenance_metadata_sha,'proposalSha256':PROPOSAL,'sourceFiles':1324,'derivedCmakeSha256':sha(derived),'coreObjectsCompiled':0,'coreObjectsByteCopied':142,'archiveMembers':142,'archive':copies['libnano_core.a'],'archiveConstructionCommandsExecuted':0,'actualRawMemberProofVerified':True,'archiveVerificationCommandsExecuted':1+len(actual),'reusedMdCopiedWithHistoricalProvenance':True,'objectsCompiled':compiled,'objectsReused':{obj:copies[obj] for obj,_ in reuse.values()},'objects':{units[x][0]:fresh[units[x][0]] for x in units},'links':linked,'programs':programs,'shim':copies[so],'newDeadlineProgramsPreserved':{n:copies[n] for n in dt['programs']},'oldCompilerCommandsUnchanged':True,'allActualMdEqualPreflight':True,'sourceSdkLibrariesGeneratedAndParentsUnchanged':True,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
 except BaseException as e:
  failed=type(e).__name__+': '+str(e);result={'format':'nano-bootstrap-readonly-replica-deadline-composite-build-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','arch':arch,'failure':failed,'consumerCompilerCommandsCompleted':compiled,'linksCompleted':linked,'coreObjectsCompiled':0,'archiveConstructionCommandsExecuted':0,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
  try:
   for path,z in protected.items():same(path,z)
   result['parentsUnchangedAfterFailure']=True;m.save(receipts/'parents-after-failure.json',protected)
   for n,z in before.items():same(source/n,z)
   for n,z in copies.items():same(build/n,z)
   for path,z in workers.items():same(path,z)
   result['copiedInputsAndWorkersUnchangedAfterFailure']=True
  except BaseException:result['parentsUnchangedAfterFailure']=False
 m.save(receipts/'terminal-build.json',result);return 1 if failed else 0
if __name__=='__main__':sys.exit(main())
