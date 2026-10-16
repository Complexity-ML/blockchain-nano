#!/usr/bin/env python3
"""Root-only RPC target adapter; source author performed no operations."""
import argparse, hashlib, json, os, pathlib, re, signal, stat, sys, tarfile, time, types, uuid

REPOSITORY=pathlib.Path('/home/ubuntu/blockchain-nano-git')
ADAPTER=REPOSITORY/'docs/qualification/current-runtime-assemblies-20261016/source/amd-host/qualify-current-amd-helpers-root.py'
ADAPTER_PIN={'sha256':'f28ae7a55fadc9fc9bfe04a70c18f991e7440d4c4ab1c4d860fd7aca872fc350','bytes':52822}
WORKER_PIN={'sha256':'ab037013c047a497adf16ba7914c4a09d22d346982acefd14db4365130a29205','bytes':36029}
HEAD='87a320946f1d3ed494bf54e1bb16b6aa67df685c'
SOURCE_REV='bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d'
FINAL=pathlib.Path('/home/ubuntu/current-amd-final-runtime-20261016-01')
CORE_ADMISSION=FINAL/'runtime-bundle-admission.private.json'
CORE_ADMISSION_PIN={'sha256':'5c5bb5928a00fe4d5fafef2028c03c450aa25c61855b595fb0fe3c2c9e5226ab','bytes':8589}
CORE=FINAL/'core-build/libnano_core.a'
TU='tests/test_rpc_simulation_block_id.cpp'
TU_PIN={'sha256':'3f3dc2bc6f7891f95e9aa2228be5639afcbf935b930c1e96d3071d8ca293f65f','bytes':12665}
TARGET='nano-rpc-simulation-block-id-tests'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')

def need(ok):
 if not ok:raise RuntimeError('current AMD RPC adapter refused')

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
 m=types.ModuleType('qualified_amd_helper_functions');m.__file__=str(ADAPTER);exec(compile(raw,str(ADAPTER),'exec'),m.__dict__)
 m.hold(ADAPTER,dict(row,sha256=ADAPTER_PIN['sha256']));return m

def main(m):
 p=argparse.ArgumentParser();p.add_argument('--output-root',required=True);p.add_argument('--worker',required=True);a=p.parse_args()
 m.need(sys.platform.startswith('linux') and os.getuid()==os.getgid()==1000)
 candidate=pathlib.Path(a.output_root);worker=pathlib.Path(a.worker)
 m.need(candidate.is_absolute() and candidate.parent.resolve()==candidate.parent and candidate.parent.is_dir() and re.fullmatch('current-amd-rpc-[a-z0-9-]{1,100}',candidate.name) and not candidate.exists() and not candidate.is_symlink())
 m.need(candidate.parent.stat().st_uid==1000 and not candidate.is_relative_to(m.R) and not candidate.is_relative_to(m.BASE) and not candidate.is_relative_to(FINAL) and not candidate.is_relative_to(m.CORE_SOURCE.parent))
 m.need(worker.is_absolute() and worker.resolve()==worker and not worker.is_relative_to(candidate))
 candidate.mkdir(mode=0o700);m.OUT=candidate;m.OUT_CREATED=True
 for leaf in ('source','controls','outputs','baseline','docker-client'):(m.OUT/leaf).mkdir(mode=0o700)
 m.DEADLINE=time.monotonic()+1100
 tools={pathlib.Path(q):m.digest(m.material(q)[1]) for q in ('/usr/bin/git','/usr/bin/docker','/usr/bin/sudo')};m.stable_new('os-utilities',tools);m.hold(pathlib.Path(__file__).resolve())
 worker_raw=m.hold(worker,WORKER_PIN,65536)
 m.need(m.git('git-head',['rev-parse','HEAD']).decode().strip()==HEAD and m.git('git-clean',['status','--porcelain'])==b'')
 found={}
 for entry in m.git('git-selected-trees',['ls-tree','-z',HEAD,'--',*m.TREES]).split(b'\0'):
  if not entry:continue
  meta,path=entry.split(b'\t',1);mode,kind,oid=meta.decode().split();key=path.decode();m.need(mode=='040000' and kind=='tree' and key not in found);found[key]=oid
 m.need(found==m.TREES) # Same seven reached source trees as the qualified bd11 base.
 docs={key:json.loads(m.hold(m.BASE/rel,{'sha256':sha,'bytes':size},16777216)) for key,(rel,sha,size) in m.REFS.items()}
 binding=json.loads(m.hold(CORE_ADMISSION,CORE_ADMISSION_PIN,16384));m.need(binding['format']=='nano-current-amd-final-runtime-binding-v1' and binding['oldAuthoritiesRefreshed'] is False)
 core_row=binding['files'][str(CORE)];m.need(set(core_row)==set(F)|{'sha256'} and core_row['sha256']==m.CORE_PIN['sha256'] and core_row['st_size']==m.CORE_PIN['bytes'])
 core_raw=m.hold(CORE,core_row);core=m.OUT/'baseline/libnano_core.a';m.write_new(core,core_raw,0o400);m.stable_new('current-core-copy',{core:m.CORE_PIN})
 m.need(os.statvfs(m.OUT).f_bavail*os.statvfs(m.OUT).f_frsize>=3*1024**3)
 selected=[*m.SELECTED,TU];wanted={}
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
 m.need(seen==set(wanted) and empty_seen==m.EMPTY and expected['/work/'+TU]==TU_PIN)
 for dest in sorted(source.rglob('*'),key=lambda q:len(q.parts),reverse=True):
  if dest.is_dir():os.chmod(dest,0o555)
 os.chmod(source,0o555);cmake=m.material(source/'CMakeLists.txt')[1];m.need(m.digest(cmake)==m.CMAKE_PIN);cmake=cmake.decode()
 for line in ('add_executable(nano-rpc-simulation-block-id-tests tests/test_rpc_simulation_block_id.cpp)','target_link_libraries(nano-rpc-simulation-block-id-tests PRIVATE nano_core)','add_test(NAME rpc_simulation_eip1898 COMMAND nano-rpc-simulation-block-id-tests --eip1898-self-test)','set_tests_properties(rpc_simulation_eip1898 PROPERTIES TIMEOUT 45 LABELS qualification_rpc_simulation_eip1898)','add_executable(nano-capella-isolated-bootstrap-audit tests/capella/isolated_bootstrap_audit.cpp)','target_link_libraries(nano-capella-isolated-bootstrap-audit PRIVATE nano_core)'):m.need(cmake.splitlines().count(line)==1)
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
   current='/baseline/libnano_core.a' if guest=='/outputs/libnano_core.a' else '/old/'+guest[len('/outputs/'):]
   if guest in docs['llvm']:row=docs['llvm'][guest]['object'];llvm.append(guest);kind='llvm'
   else:m.need(guest in docs['archives'] and guest.endswith('.a'));row=docs['archives'][guest]['archive'];archives.append(guest);kind='archive'
   if guest!='/outputs/libnano_core.a':
    m.need(row['path']==str(m.BASE/'core'/guest[len('/outputs/'):]) and current not in mounts);m.old_file(row);mounts[current]=row['path'];expected[current]={'sha256':row['sha256'],'bytes':row['bytes']}
   order.append({'kind':kind,'old':guest,'current':current,'sha256':m.CORE_PIN['sha256'] if guest=='/outputs/libnano_core.a' else row['sha256']});tail.append(current)
  else:m.need(guest.startswith(('/usr/','/lib/','/lib64/','-')));tail.append(guest)
 m.need(len(archives)==5 and archives.count('/outputs/libnano_core.a')==1 and len(llvm)==2 and set(llvm)==set(docs['llvm']) and len(mounts)==6)
 recipes={'format':'nano-current-linux-rpc-test-recipes-v1','owner':'vps-docker','uid':1000,'gid':1000,'imageId':m.IMAGE,'elfMachine':62,'sourceRevision':HEAD,'sourceTreeEntries':[{'path':q,'oid':found[q]} for q in sorted(found)],'programs':[{'role':'rpcSimulationTest','source':'/work/'+TU,'compileFlags':flags,'linkPrefix':link[:3],'linkTail':tail,'reuse':None}]}
 controls={'recipes.json':(json.dumps(recipes,sort_keys=True)+'\n').encode(),'source-expected.private.json':(json.dumps(expected,sort_keys=True)+'\n').encode(),'core-pin.json':(json.dumps(m.CORE_PIN,sort_keys=True)+'\n').encode(),'worker.pl':worker_raw}
 for leaf,raw in controls.items():m.write_new(m.OUT/'controls'/leaf,raw,0o444)
 os.chmod(m.OUT/'controls',0o555);os.chmod(m.OUT/'baseline',0o555)
 m.save('rpc-source-recipe-joins.private.json',{'head':HEAD,'effectiveSourceBase':SOURCE_REV,'treeEntries':found,'sourceTu':TU_PIN,'selectedCmake':m.CMAKE_PIN,'templateTarget':'nano-capella-isolated-bootstrap-audit','linkOrder':order,'oldObjectOrMdUsed':False,'oldImageDependenciesUsed':False,'fullCmakeConfigureExecuted':False})
 docker=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',str(m.OUT/'docker-client')]
 def dk(name,args,seconds=30,expected=0,cleanup=False):return m.run(name,docker+args,seconds,expected,cleanup=cleanup)
 name='nano-current-amd-rpc-'+uuid.uuid4().hex;labelkey='nano.current.amd.rpc';cid=None;attempted=False;first_error=None;result=None
 try:
  raw,err,_=dk('image-inspect',['image','inspect',m.IMAGE,'--format','{{json .}}']);image=json.loads(raw);m.need(not err and image['Id']==m.IMAGE and image['Architecture']=='amd64' and image['Os']=='linux')
  raw,err,_=dk('name-absent',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}']);m.need(not raw.strip() and not err)
  extra=[]
  for guest,host in sorted(mounts.items()):extra+=['--mount','type=bind,src='+host+',dst='+guest+',readonly']
  args=['create','--pull=never','--name',name,'--label',labelkey+'='+name,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','128','--user','1000:1000','--tmpfs','/tmp:rw,nosuid,nodev,size=128m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(core)+',dst=/baseline/libnano_core.a,readonly','--mount','type=bind,src='+str(m.OUT/'controls')+',dst=/controls,readonly','--mount','type=bind,src='+str(m.OUT/'outputs')+',dst=/outputs',*extra,'--workdir','/outputs','--entrypoint','/bin/sh',m.IMAGE,'-ec','exec /bin/sleep 900']
  m.fence();attempted=True;raw,err,_=dk('create-rpc',args);cid=raw.decode().strip();m.need(not err and re.fullmatch('[0-9a-f]{64}',cid));raw,err,_=dk('start-rpc',['start',cid]);m.need(not err and raw.decode().strip()==cid)
  prime=r"use strict;use warnings;use Digest::SHA ();use JSON::PP ();my @r;for my $p (@ARGV){open(my $f,'<',$p)or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);push @r,{sha256=>$d->hexdigest,bytes=>-s $p};}print JSON::PP->new->canonical->encode(\@r);"
  names=sorted(controls);raw,err,_=dk('prime-known-controls',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-e',prime,*['/controls/'+q for q in names]]);m.need(not err and json.loads(raw)==[m.digest(controls[q]) for q in names]);m.stable_new('controls',{m.OUT/'controls'/q:m.digest(controls[q]) for q in names})
  m.save('original-selected-inputs-before-worker.private.json',m.HELD)
  raw,err,code=dk('qualify-rpc',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','/controls/worker.pl','--rpc-eip1898-current'],610,expected=None)
  m.need(code==0 and not err);body=m.material(m.OUT/'outputs/result-public.json',16777216)[1];result=json.loads(body);m.need(json.loads(raw)==result)
  m.need(result['format']=='nano-current-linux-rpc-simulation-block-id-qualification-v1' and result['passed'] is True and result['owner']=='vps-docker' and result['uid']==result['gid']==1000 and result['imageId']==m.IMAGE and result['sourceRevisionDeclared']==HEAD and result['sourceTranslationUnit']==TU_PIN)
  for key in ('independentMCommands','compiledTranslationUnits','linkCommands','loaderCommands','rpcTargetedRuns'):m.need(type(result[key]) is int and result[key]==1)
  m.need(result['argumentRefusals']==0 and result['normalizedMEqualsMd'] is True and result['loaderPassed'] is True and result['coreArchiveMutated'] is False and result['coreMembersRequalified'] is False and result['productionQualified'] is False and {'sha256':result['coreSha256'],'bytes':result['coreBytes']}==m.CORE_PIN)
  m.need(result['rpcTargetedSummary']=={'format':'nano-rpc-simulation-eip1898-targeted-v1','passed':True,'groups':5,'checks':120,'expectedRefusals':58,'oldSuiteExecuted':False,'networkStarted':False,'productionQualified':False})
  binding_raw=m.material(m.OUT/'outputs/software-admission.private.json',16777216)[1];m.need(m.digest(binding_raw)=={'sha256':result['softwareAdmissionSha256'],'bytes':result['softwareAdmissionBytes']});new_binding=json.loads(binding_raw);m.need(new_binding['format']=='nano-current-linux-rpc-test-software-binding-v1' and new_binding['owner']=='vps-docker' and new_binding['imageId']==m.IMAGE and new_binding['sourceRevision']==HEAD);dep=new_binding['dependencyFile'];products={m.OUT/'outputs/software-admission.private.json':m.digest(binding_raw)}
  for leaf,sha,size,mode in ((TARGET,result['softwareSha256'],result['softwareBytes'],0o500),('rpcSimulationTest.cpp.o',result['objectSha256'],result['objectBytes'],0o400),('rpcSimulationTest.cpp.o.d',dep['sha256'],dep['bytes'],0o400)):
   path=m.OUT/'outputs'/leaf;row,body=m.material(path);pin={'sha256':sha,'bytes':size};m.need(m.digest(body)==pin and stat.S_IMODE(row['st_mode'])==mode and row['st_uid']==row['st_gid']==1000 and row['st_nlink']==1);products[path]=pin
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
 result.update({'rootHead':HEAD,'rootEffectiveSourceBase':SOURCE_REV,'rootSevenTreeEntriesEqualQualifiedBase':True,'rootSourceTuGitAndKnownBytesVerified':True,'rootSelectedTargetCmakeAsserted':True,'rootSelectedOriginalRowsFullNineAndShaUnchanged':True,'rootCurrentCoreFromFinalBundleFullNineVerified':True,'rootOwnedContainerAbsent':True,'fixedProducerReceiptsRead':4,'oldSdkMapsRead':False,'oldSourceAdmissionRead':False,'oldHelperClisRun':0,'oldAuthoritiesRefreshed':False,'campaignAdmission':False,'productionQualified':False})
 m.save('result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 m=None;failed=False
 try:m=load_adapter();main(m)
 except BaseException:
  failed=True
  if m is not None and m.OUT_CREATED:m.save('failure-public.json',{'passed':False,'oldSdkMapsRead':False,'oldSourceAdmissionRead':False,'oldAuthoritiesRefreshed':False,'compilerOrTestSuccessClaimed':False,'rawErrorDetailsExported':False})
  print('current AMD RPC adapter refused',file=sys.stderr);sys.exit(1)
 finally:
  if m is not None:
   try:
    if m.ACTIVE is not None:m.stop(m.ACTIVE)
    m.fence()
    if m.OUT_CREATED:m.save('originals-final-fence-public.json',{'passed':True,'heldSelectedFiles':len(m.HELD),'firstFailurePreserved':failed,'oldAuthoritiesRefreshed':False})
   except BaseException as error:
    if m.OUT_CREATED:m.save('originals-final-fence-failure-public.json',{'passed':False,'firstFailurePreserved':failed,'errorType':type(error).__name__})
    if not failed:raise
