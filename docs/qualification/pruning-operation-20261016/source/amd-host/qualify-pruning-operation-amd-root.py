#!/usr/bin/env python3
"""Root-only pruning-operation adapter; source author performed no operations."""
import argparse, hashlib, json, os, pathlib, re, signal, stat, sys, tarfile, time, types, uuid

REPOSITORY=pathlib.Path('/home/ubuntu/blockchain-nano-git')
ADAPTER=REPOSITORY/'docs/qualification/current-runtime-assemblies-20261016/source/amd-host/qualify-current-amd-helpers-root.py'
ADAPTER_PIN={'sha256':'f28ae7a55fadc9fc9bfe04a70c18f991e7440d4c4ab1c4d860fd7aca872fc350','bytes':52822}
WORKER_PIN={'sha256':'fc0f3bddc134321f0eabc0b830e0a5ffdb484741321fa9cc3fd6a093d1b04082','bytes':36785}
HEAD='68d64a5330d16c971dd603b56e0f14c88c1459c8'
SOURCE_REV='bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d'
FINAL=pathlib.Path('/home/ubuntu/current-amd-final-runtime-20261016-01')
CORE_ADMISSION=FINAL/'runtime-bundle-admission.private.json'
CORE_ADMISSION_PIN={'sha256':'5c5bb5928a00fe4d5fafef2028c03c450aa25c61855b595fb0fe3c2c9e5226ab','bytes':8589}
CORE=FINAL/'core-build/libnano_core.a'
SOURCES={
 'src/consensus_capella_runtime.cpp':{'sha256':'57f18bb906274d62a8fc93f86059d0b28218660fc96129966dec9329c9761232','bytes':28682},
 'tests/test_capella_startup_deadline.cpp':{'sha256':'11fb02a13b2e80f5d4a11fab5336ba4492737fc0771b4745fb1b67c3777e4ad6','bytes':24875},
 'tests/test_owned_copy_physical_admission.cpp':{'sha256':'c18cc1b386bc65802a1ed1105f63765378b9885ff4feb6d3d04b3f6872bfdbd3','bytes':28547},
 'tools/storage-maintenance/compact-owned-copy.cpp':{'sha256':'0d17feb4c32be71886a4dce875176498c3d7b3cf5bb0bf7e75f3866275c4435e','bytes':12093}}
CMAKE={'sha256':'e052228d297d828cee82b283069624035e6db2e06b4321da8500f0099508a532','bytes':175698}
FIXTURE='tests/fixtures/capella-bootstrap-anchor-cb532/'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')

def need(ok):
 if not ok:raise RuntimeError('current AMD pruning operation adapter refused')

def load_adapter():
 # Verify the public source before import. The established module's __main__
 # body is not run; its helpers/signals/constants are reused without edits.
 need(ADAPTER.resolve()==ADAPTER)
 fd=os.open(ADAPTER,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  before=os.fstat(fd);row={k:getattr(before,k) for k in F};need(stat.S_ISREG(before.st_mode) and before.st_size==ADAPTER_PIN['bytes'])
  raw=b''
  while len(raw)<=before.st_size:
   part=os.read(fd,min(65536,before.st_size+1-len(raw)))
   if not part:break
   raw+=part
  need({'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}==ADAPTER_PIN and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(ADAPTER.lstat(),k) for k in F})
 finally:os.close(fd)
 # Execute the exact bytes just authenticated, avoiding a second loader read or
 # __pycache__ writes. The custom name keeps the established __main__ inactive.
 m=types.ModuleType('qualified_amd_operation_functions');m.__file__=str(ADAPTER);exec(compile(raw,str(ADAPTER),'exec'),m.__dict__)
 m.hold(ADAPTER,dict(row,sha256=ADAPTER_PIN['sha256']));return m

def main(m):
 p=argparse.ArgumentParser();p.add_argument('--output-root',required=True);p.add_argument('--worker',required=True);a=p.parse_args()
 m.need(sys.platform.startswith('linux') and os.getuid()==os.getgid()==1000)
 candidate=pathlib.Path(a.output_root);worker=pathlib.Path(a.worker)
 m.need(candidate.is_absolute() and candidate.parent.resolve()==candidate.parent and candidate.parent.is_dir() and re.fullmatch('current-amd-pruning-operation-[a-z0-9-]{1,100}',candidate.name) and not candidate.exists() and not candidate.is_symlink())
 m.need(candidate.parent.stat().st_uid==1000 and not candidate.is_relative_to(m.R) and not candidate.is_relative_to(m.BASE) and not candidate.is_relative_to(FINAL) and not candidate.is_relative_to(m.CORE_SOURCE.parent))
 m.need(worker.is_absolute() and worker.resolve()==worker and not worker.is_relative_to(candidate))
 candidate.mkdir(mode=0o700);m.OUT=candidate;m.OUT_CREATED=True
 for leaf in ('source','controls','outputs','baseline','docker-client'):(m.OUT/leaf).mkdir(mode=0o700)
 m.DEADLINE=time.monotonic()+1600
 tools={pathlib.Path(q):m.digest(m.material(q)[1]) for q in ('/usr/bin/git','/usr/bin/docker','/usr/bin/sudo')};m.stable_new('os-utilities',tools);m.hold(pathlib.Path(__file__).resolve())
 worker_raw=m.hold(worker,WORKER_PIN,65536)
 m.need(m.git('git-head',['rev-parse','HEAD']).decode().strip()==HEAD and m.git('git-clean',['status','--porcelain'])==b'')
 found={}
 for entry in m.git('git-selected-trees',['ls-tree','-z',HEAD,'--',*m.TREES]).split(b'\0'):
  if not entry:continue
  meta,path=entry.split(b'\t',1);mode,kind,oid=meta.decode().split();key=path.decode();m.need(mode=='040000' and kind=='tree' and key not in found);found[key]=oid
 m.need(set(found)==set(m.TREES) and all(found[q]==m.TREES[q] for q in found if q!='src'))
 m.need(m.git('source-core-delta',['diff','--name-only',SOURCE_REV,HEAD,'--','src','include']).decode().splitlines()==['src/consensus_capella_runtime.cpp'])
 runtime_guest='/outputs/CMakeFiles/nano_core.dir/src/consensus_capella_runtime.cpp.o'

 docs={key:json.loads(m.hold(m.BASE/rel,{'sha256':sha,'bytes':size},16777216)) for key,(rel,sha,size) in m.REFS.items()}
 binding=json.loads(m.hold(CORE_ADMISSION,CORE_ADMISSION_PIN,16384));m.need(binding['format']=='nano-current-amd-final-runtime-binding-v1' and binding['oldAuthoritiesRefreshed'] is False)
 core_row=binding['files'][str(CORE)];m.need(set(core_row)==set(F)|{'sha256'} and core_row['sha256']==m.CORE_PIN['sha256'] and core_row['st_size']==m.CORE_PIN['bytes'])
 core_raw=m.hold(CORE,core_row);core=m.OUT/'baseline/libnano_core.a';m.write_new(core,core_raw,0o400);m.stable_new('current-core-copy',{core:m.CORE_PIN})
 m.need(os.statvfs(m.OUT).f_bavail*os.statvfs(m.OUT).f_frsize>=3*1024**3)
 fixture_manifest=json.loads(m.git('public-fixture-manifest',['show',HEAD+':'+FIXTURE+'manifest.json']))
 selected=[*m.TREES,'CMakeLists.txt','tests/test_capella_startup_deadline.cpp','tests/test_owned_copy_physical_admission.cpp','tools/storage-maintenance',FIXTURE+'manifest.json',*[FIXTURE+fixture_manifest['files'][key]['path'] for key in ('profile','initial','genesis','genesisState')]];wanted={}
 for entry in m.git('git-selected-blobs',['ls-tree','-r','-z',HEAD,'--',*selected]).split(b'\0'):
  if not entry:continue
  meta,path=entry.split(b'\t',1);mode,kind,oid=meta.decode().split();key=path.decode();rel=pathlib.PurePosixPath(key);m.need(kind=='blob' and mode in ('100644','100755') and not rel.is_absolute() and '..'not in rel.parts and key not in wanted);wanted[key]=oid
 tar_path,err,code=m.run('git-archive',['/usr/bin/git','--no-optional-locks','-C',str(m.R),'archive',HEAD,*selected],180,limit=1073741824,capture=False);m.need(not err and code==0);os.chmod(tar_path,0o400)
 source=m.OUT/'source';expected={};seen=set();empty_seen=set();total=0
 with tarfile.open(tar_path,'r:') as tf:
  for item in tf:
   rel=pathlib.PurePosixPath(item.name);m.need(not rel.is_absolute() and '..'not in rel.parts and (item.isdir() or item.isfile()));dest=source/str(rel)
   if item.isdir():dest.mkdir(parents=True,exist_ok=True);continue
   m.need(str(rel) in wanted and str(rel) not in seen and item.size<=33554432);seen.add(str(rel));total+=item.size;m.need(total<=1073741824)
   raw=tf.extractfile(item).read(item.size+1);m.need(len(raw)==item.size and hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()==wanted[str(rel)])
   dest.parent.mkdir(parents=True,exist_ok=True);m.write_new(dest,raw,0o444);guest='/work/'+str(rel)
   if not raw:m.need(guest in m.EMPTY);empty_seen.add(guest)
   else:expected[guest]=m.digest(raw)
 m.need(seen==set(wanted) and empty_seen==m.EMPTY and all(expected['/work/'+q]==z for q,z in SOURCES.items()))
 for dest in sorted(source.rglob('*'),key=lambda q:len(q.parts),reverse=True):
  if dest.is_dir():os.chmod(dest,0o555)
 os.chmod(source,0o555);cmake_raw=m.material(source/'CMakeLists.txt')[1];m.need(m.digest(cmake_raw)==CMAKE);cmake=cmake_raw.decode()
 additions=[b'add_test(NAME consensus_capella_maintenance_startup_validation COMMAND nano-capella-startup-deadline-tests --maintenance-flags-self-test)\nset_tests_properties(consensus_capella_maintenance_startup_validation PROPERTIES TIMEOUT 70 LABELS "qualification_consensus_recovery")\n',b' add_test(NAME storage_pruning_sst_reclaim COMMAND nano-owned-copy-physical-admission-tests --pruning-sst-reclaim)\n set_tests_properties(storage_pruning_sst_reclaim PROPERTIES TIMEOUT 130 LABELS qualification_storage_pruning_reclaim)\n']
 stripped=cmake_raw
 for addition in additions:m.need(stripped.count(addition)==1);stripped=stripped.replace(addition,b'')
 m.need(stripped==m.git('prior-cmake',['show',HEAD+'^:CMakeLists.txt']))
 # Canonical runtime flags are taken from the selected Core object inside the
 # already-authorized fixed archive receipt. No physical compile database,
 # original source admission, current SDK table or old image row is opened.
 archive=docs['archives']['/outputs/libnano_core.a'];m.need(isinstance(archive['objects'],dict) and len(archive['objects'])==143)
 runtime_record=archive['objects'][runtime_guest];runtime_compile=runtime_record['compile'];m.need(runtime_compile['file']=='/work/src/consensus_capella_runtime.cpp' and runtime_compile['directory']=='/outputs')
 runtime_argv=runtime_compile.get('arguments') or m.shlex.split(runtime_compile['command']);m.need(runtime_argv[0]=='/usr/bin/clang++' and runtime_argv.count('-o')==runtime_argv.count('-c')==1 and runtime_argv[-4]=='-o' and runtime_argv[-2:]==['-c',runtime_compile['file']])
 runtime_object=os.path.normpath(runtime_argv[-3] if runtime_argv[-3].startswith('/') else '/outputs/'+runtime_argv[-3]);m.need(runtime_object==runtime_guest)
 runtime_flags=[q.replace('/outputs/third_party','/old/third_party') for q in runtime_argv[:-4]];m.need(not any('NANO_' in q for q in runtime_flags))
 for line in ('add_executable(nano-storage-maintenance tools/storage-maintenance/compact-owned-copy.cpp)','target_link_libraries(nano-storage-maintenance PRIVATE nano_core)','add_executable(nano-capella-startup-deadline-tests tests/test_capella_startup_deadline.cpp)','target_link_libraries(nano-capella-startup-deadline-tests PRIVATE nano_core)','add_executable(nano-owned-copy-physical-admission-tests tests/test_owned_copy_physical_admission.cpp)','target_link_libraries(nano-owned-copy-physical-admission-tests PRIVATE nano_core)'):m.need(sum(x.strip()==line for x in cmake.splitlines())==1)
 # Reuse the already-produced canonical Core-only consumer recipe, not its
 # object, MD, CLI or project/SDK dependency table.
 rec=docs['links']['nano-capella-isolated-bootstrap-audit'];comp=rec['compile'];m.need(comp['file']=='/work/tests/capella/isolated_bootstrap_audit.cpp' and comp['directory']=='/outputs')
 argv=comp.get('arguments') or m.shlex.split(comp['command']);m.need(argv[0]=='/usr/bin/clang++' and argv[-2:]==['-c',comp['file']] and argv[-4]=='-o' and argv.count('-o')==argv.count('-c')==1)
 own=os.path.normpath(argv[-3] if argv[-3].startswith('/') else '/outputs/'+argv[-3]);flags=[q.replace('/outputs/third_party','/old/third_party') for q in argv[:-4]];m.need(not any('NANO_'in q for q in flags))
 link=rec['linkRecipe']['argv'];m.need(link[:3]==['/usr/bin/clang++','-O3','-DNDEBUG'] and link.count('-o')==1 and link.index('-o')==4 and os.path.normpath(link[3] if link[3].startswith('/') else '/outputs/'+link[3])==own and os.path.normpath(link[5] if link[5].startswith('/') else '/outputs/'+link[5])=='/outputs/nano-capella-isolated-bootstrap-audit')
 m.need(set(docs['llvm'])=={'/outputs/third_party/mcl/bint64.o','/outputs/third_party/mcl/base64.o'})
 mounts={};tail=[];order=[];archives=[];llvm=[]
 for word in link[6:]:
  guest=os.path.normpath('/outputs/'+word) if not word.startswith(('/','-')) else word
  if guest.startswith('/outputs/'):
   current='/outputs/libnano_core.a' if guest=='/outputs/libnano_core.a' else '/old/'+guest[len('/outputs/'):]
   if guest in docs['llvm']:row=docs['llvm'][guest]['object'];llvm.append(guest);kind='llvm'
   else:m.need(guest in docs['archives'] and guest.endswith('.a'));row=docs['archives'][guest]['archive'];archives.append(guest);kind='archive'
   if guest!='/outputs/libnano_core.a':
    m.need(row['path']==str(m.BASE/'core'/guest[len('/outputs/'):]) and current not in mounts);m.old_file(row);mounts[current]=row['path'];expected[current]={'sha256':row['sha256'],'bytes':row['bytes']}
   order.append({'kind':kind,'old':guest,'current':current,'sha256':m.CORE_PIN['sha256'] if guest=='/outputs/libnano_core.a' else row['sha256']});tail.append(current)
  else:m.need(guest.startswith(('/usr/','/lib/','/lib64/','-')));tail.append(guest)
 m.need(len(archives)==5 and archives.count('/outputs/libnano_core.a')==1 and len(llvm)==2 and set(llvm)==set(docs['llvm']) and len(mounts)==6)
 def program(role,source,macros):return {'role':role,'source':source,'compileFlags':[*flags,*macros],'linkPrefix':link[:3],'linkTail':tail}
 programs=[program('startupValidation','/work/tests/test_capella_startup_deadline.cpp',['-DNANO_TEST_SOURCE="/work"']),program('sstReclaim','/work/tests/test_owned_copy_physical_admission.cpp',['-DNANO_TEST_COMPACT="/outputs/nano-storage-maintenance"','-DNANO_TEST_MIGRATE="/outputs/nano-pruned-schema-migration"']),program('maintenance','/work/tools/storage-maintenance/compact-owned-copy.cpp',[])]
 recipes={'format':'nano-linux-pruning-operation-recipes-v1','owner':'vps-docker','uid':1000,'gid':1000,'imageId':m.IMAGE,'elfMachine':62,'sourceRevision':HEAD,'runtime':{'source':'/work/src/consensus_capella_runtime.cpp','compileFlags':runtime_flags},'programs':programs}
 controls={'recipes.json':(json.dumps(recipes,sort_keys=True)+'\n').encode(),'source-expected.private.json':(json.dumps(expected,sort_keys=True)+'\n').encode(),'core-pin.json':(json.dumps(m.CORE_PIN,sort_keys=True)+'\n').encode(),'worker.pl':worker_raw}
 for leaf,raw in controls.items():m.write_new(m.OUT/'controls'/leaf,raw,0o444)
 os.chmod(m.OUT/'controls',0o555);os.chmod(m.OUT/'baseline',0o555)
 m.save('operation-source-recipe-joins.private.json',{'head':HEAD,'effectiveSourceBase':SOURCE_REV,'treeEntries':found,'sourceTuPins':SOURCES,'selectedCmake':CMAKE,'runtimeCompileRecord':runtime_compile,'runtimeRecordFromFixedArchivesReceipt':True,'templateTarget':'nano-capella-isolated-bootstrap-audit','linkOrder':order,'oldObjectOrMdUsed':False,'oldImageDependenciesUsed':False,'fullCmakeConfigureExecuted':False})
 docker=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',str(m.OUT/'docker-client')]
 def dk(name,args,seconds=30,expected=0,cleanup=False):return m.run(name,docker+args,seconds,expected,cleanup=cleanup)
 name='nano-pruning-operation-amd-'+uuid.uuid4().hex;labelkey='nano.pruning.operation.amd';cid=None;attempted=False;first_error=None;result=None
 try:
  raw,err,_=dk('image-inspect',['image','inspect',m.IMAGE,'--format','{{json .}}']);image=json.loads(raw);m.need(not err and image['Id']==m.IMAGE and image['Architecture']=='amd64' and image['Os']=='linux')
  raw,err,_=dk('name-absent',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}']);m.need(not raw.strip() and not err)
  extra=[]
  for guest,host in sorted(mounts.items()):extra+=['--mount','type=bind,src='+host+',dst='+guest+',readonly']
  args=['create','--pull=never','--name',name,'--label',labelkey+'='+name,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','128','--user','1000:1000','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(core)+',dst=/baseline/libnano_core.a,readonly','--mount','type=bind,src='+str(m.OUT/'controls')+',dst=/controls,readonly','--mount','type=bind,src='+str(m.OUT/'outputs')+',dst=/outputs',*extra,'--workdir','/outputs','--entrypoint','/bin/sh',m.IMAGE,'-ec','exec /bin/sleep 1800']
  m.fence();attempted=True;raw,err,_=dk('create-operation',args);cid=raw.decode().strip();m.need(not err and re.fullmatch('[0-9a-f]{64}',cid));raw,err,_=dk('start-operation',['start',cid]);m.need(not err and raw.decode().strip()==cid)
  prime=r"use strict;use warnings;use Digest::SHA ();use JSON::PP ();my @r;for my $p (@ARGV){open(my $f,'<',$p)or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);push @r,{sha256=>$d->hexdigest,bytes=>-s $p};}print JSON::PP->new->canonical->encode(\@r);"
  names=sorted(controls);raw,err,_=dk('prime-known-controls',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-e',prime,*['/controls/'+q for q in names]]);m.need(not err and json.loads(raw)==[m.digest(controls[q]) for q in names]);m.stable_new('controls',{m.OUT/'controls'/q:m.digest(controls[q]) for q in names})
  m.save('original-selected-inputs-before-worker.private.json',m.HELD)
  raw,err,_=dk('worker-syntax',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-c','/controls/worker.pl']);m.need(not raw and err.strip()==b'/controls/worker.pl syntax OK')
  raw,err,code=dk('pruning-operation-worker',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','/controls/worker.pl','--pruning-operation'],1250,expected=None)
  # The outputs are a direct, private UID1000 bind. Record their inventory before
  # checking worker exit: a failed invocation's logs/refusal are never masked by
  # trying to read a success-only artifact or an empty tar stream.
  observed=[];total=0
  for path in sorted((m.OUT/'outputs').iterdir()):
   st=path.lstat();m.need(path.name not in ('.','..') and re.fullmatch('[A-Za-z0-9_.-]+',path.name) and stat.S_ISREG(st.st_mode) and st.st_nlink==1 and st.st_size<67108864);total+=st.st_size;m.need(total<=268435456);observed.append({'name':path.name,'bytes':st.st_size})
  m.need(len(observed)<=128);m.save('worker-output-inventory.private.json',{'observationalOnly':True,'workerExitCode':code,'files':observed,'totalBytes':total,'noSuccessArtifactRequiredForCollection':True})
  m.need(code==0 and not err);body=m.material(m.OUT/'outputs/result-public.json',16777216)[1];result=json.loads(body);m.need(json.loads(raw)==result)
  m.need(result['format']=='nano-linux-pruning-operation-qualification-v1' and result['passed'] is True and result['owner']=='vps-docker' and result['imageId']==m.IMAGE and result['sourceRevisionDeclared']==HEAD)
  m.need(all(type(result[k]) is int and result[k]==4 for k in ('independentMCommands','compiledTranslationUnits','linkCommands')) and result['loaderCommands']==3 and result['archiveUpdates']==1 and result['argumentRefusals']==0)
  m.need(result['coreArchiveMembers']==143 and result['otherCoreMembersByteExact']==142 and result['replacedCoreMembers']==1 and result['selectedTestRuns']==3 and result['newPositiveModeRuns']==2 and result['expectedOldCoreRefusals']==1 and result['originalCoreArchiveMutated'] is False and result['ownedTmpEmptyAfterTests'] is True and result['productionQualified'] is False)
  m.need(result['startupSummary']['expectedRefusals']==31 and result['sstSummary']['deletedStates']==63 and result['startupBaseline']=={'expectedExitCode':1,'observedExitCode':1,'stderrExact':'maintenance refusal created witness or execution checkpoint\n','stdoutEmpty':True,'startupObjectCompiledOnce':True,'oldFiveGroupsExecuted':False})
  binding_raw=m.material(m.OUT/'outputs/software-admission.private.json',16777216)[1];m.need(m.digest(binding_raw)=={'sha256':result['softwareAdmissionSha256'],'bytes':result['softwareAdmissionBytes']})
  new_binding=json.loads(binding_raw);m.need(new_binding['format']=='nano-linux-pruning-operation-software-binding-v1' and new_binding['owner']=='vps-docker' and new_binding['imageId']==m.IMAGE and new_binding['sourceRevisionDeclared']==HEAD)
  m.need(len(result['products'])==4 and {p['source'] for p in result['products']}=={'/work/'+q for q in SOURCES})
  products={m.OUT/'outputs/software-admission.private.json':m.digest(binding_raw)}
  assets={'libnano_core.a':({'sha256':result['newCoreSha256'],'bytes':result['newCoreBytes']},0o400)}
  for product in result['products']:
   m.need(product['normalizedMEqualsMd'] is True and product['dependencyMode']=='MD')
   if 'softwarePath' in product:
    m.need(product['loaderPassed'] is True);assets[pathlib.PurePosixPath(product['softwarePath']).name]=({'sha256':product['softwareSha256'],'bytes':product['softwareBytes']},0o500)
   if 'baselineSoftwareSha256' in product:assets['nano-capella-startup-deadline-baseline']=({'sha256':product['baselineSoftwareSha256'],'bytes':product['baselineSoftwareBytes']},0o500)
  m.need(set(assets)=={'libnano_core.a','nano-capella-startup-deadline-tests','nano-capella-startup-deadline-baseline','nano-owned-copy-physical-admission-tests','nano-storage-maintenance'})
  for leaf,(known,mode) in assets.items():
   path=m.OUT/'outputs'/leaf;row,body=m.material(path);m.need(m.digest(body)==known and stat.S_IMODE(row['st_mode'])==mode and row['st_uid']==row['st_gid']==1000 and row['st_nlink']==1);products[path]=known
  # Guest products were already admitted by their original worker rows. This is
  # a separate first host observation of the new files, never an old-nine refresh.
  m.stable_new('qualified-host-products',products)
 except BaseException as error:first_error=error;raise
 finally:
  for sig in (signal.SIGTERM,signal.SIGINT,signal.SIGHUP):signal.signal(sig,signal.SIG_IGN)
  m.CLEAN_DEADLINE=time.monotonic()+60
  try:
   if attempted:
    if re.fullmatch('[0-9a-f]{64}',cid or '') is None:
     raw,err,_=dk('cleanup-discover',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--filter','label='+labelkey+'='+name,'--format','{{.ID}}'],10,cleanup=True);m.need(not err and len(raw.splitlines())<=1);cid=raw.decode().strip() or None
    if cid:
     m.need(re.fullmatch('[0-9a-f]{64}',cid));raw,err,_=dk('cleanup-inspect',['inspect',cid,'--format','{{json .}}'],10,cleanup=True);item=json.loads(raw);m.need(not err and item['Id']==cid and item['Name']=='/'+name and item['Config']['Labels'].get(labelkey)==name and item['Config']['Image']==m.IMAGE and item['Image']==m.IMAGE);dk('cleanup-remove',['rm','-f',cid],15,cleanup=True)
    raw,err,_=dk('cleanup-absence',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],10,cleanup=True);m.need(not raw.strip() and not err)
   m.save('cleanup-public.json',{'ownedContainerAbsent':True,'creationAttempted':attempted,'ownedOutputsRetained':True,'oldSdkMapsRead':False})
  except BaseException as error:
   m.save('cleanup-failure-public.json',{'passed':False,'firstFailurePreserved':first_error is not None,'errorType':type(error).__name__})
   if first_error is None:raise
 m.fence();m.need(result is not None and m.git('final-git-head',['rev-parse','HEAD']).decode().strip()==HEAD and m.git('final-git-clean',['status','--porcelain'])==b'')
 result.update({'rootHead':HEAD,'rootEffectiveSourceBase':SOURCE_REV,'rootSixRetainedTreeEntriesEqualQualifiedBase':True,'rootOnlyRuntimeCoreSourceDelta':True,'rootFourCTestLinesOnlyDeltaVerified':True,'rootSourceTuGitAndKnownBytesVerified':True,'rootSelectedTargetCmakeAsserted':True,'rootRuntimeFlagsFromFixedArchiveCompileRecord':True,'rootSelectedOriginalRowsFullNineAndShaUnchanged':True,'rootOriginalCoreFromFinalBundleFullNineVerified':True,'rootOwnedContainerAbsent':True,'rootPerlSyntaxPassed':True,'fixedProducerReceiptsRead':4,'oldSdkMapsRead':False,'oldSourceAdmissionRead':False,'oldHelperClisRun':0,'oldAuthoritiesRefreshed':False,'campaignAdmission':False,'productionQualified':False})
 m.save('result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 m=None;failed=False
 try:m=load_adapter();main(m)
 except BaseException:
  failed=True
  if m is not None and m.OUT_CREATED:m.save('failure-public.json',{'passed':False,'oldSdkMapsRead':False,'oldSourceAdmissionRead':False,'oldAuthoritiesRefreshed':False,'compilerOrTestSuccessClaimed':False,'rawErrorDetailsExported':False})
  print('current AMD pruning operation adapter refused',file=sys.stderr);sys.exit(1)
 finally:
  if m is not None:
   try:
    if m.ACTIVE is not None:m.stop(m.ACTIVE)
    m.fence()
    if m.OUT_CREATED:m.save('originals-final-fence-public.json',{'passed':True,'heldSelectedFiles':len(m.HELD),'firstFailurePreserved':failed,'oldAuthoritiesRefreshed':False})
   except BaseException as error:
    if m.OUT_CREATED:m.save('originals-final-fence-failure-public.json',{'passed':False,'firstFailurePreserved':failed,'errorType':type(error).__name__})
    if not failed:raise
