#!/usr/bin/env python3
"""Root-only AMD RPC-Core runtime relink; source work only by the author."""
import argparse, calendar, datetime, hashlib, json, os, pathlib, re, signal, stat, sys, tarfile, time, types, uuid
REPOSITORY=pathlib.Path('/home/ubuntu/blockchain-nano-git')
ADAPTER=REPOSITORY/'docs/qualification/current-runtime-assemblies-20261016/source/amd-host/qualify-current-amd-helpers-root.py'
ADAPTER_PIN={'sha256':'f28ae7a55fadc9fc9bfe04a70c18f991e7440d4c4ab1c4d860fd7aca872fc350','bytes':52822}
WORKER_PIN={'sha256':'f60a63c70d5d2fab65647917458d9829f6966df840275c2d778291aaef8b3154','bytes':27010}
SOURCE_REV='d26e4744416d033dad5a2519cc2ab8899b838faf'
SRC_TREE='7c82858ec741c7076a4f89ca336dee574e11244d'
RPC_ROOT=pathlib.Path('/home/ubuntu/rpc-context-amd-root-20261016-01')
RPC_RESULT_PIN={'sha256':'ad0fd6a977b3450932532fd962c42718e8ed52cb2a8720bb9f64b88baec08a00','bytes':2188}
CORE=RPC_ROOT/'outputs/libnano_core.a'
CORE_PIN={'sha256':'2678b628528295dd7d729e8917572d3076a1068399e58b12f97fa3849bb497bd','bytes':40805844}
CORE_BINDING_PIN={'sha256':'f1202685a2880c62ad81b3d007938a64df1960ec6ddb7cbdd87e89604cb40ed0','bytes':805044}
HELPERS=pathlib.Path('/home/ubuntu/nano-current-helper-amd-dbf51ad9cfd14c82a0aaaae635a7e14e/current-amd-helpers-dbf51ad9cfd14c82a0aaaae635a7e14e')
HELPER_BINDING_PIN={'sha256':'cfe9dc36c2dddb9f14cef7f1047d7ab3054d3ebed082baccdd374c01d3934d49','bytes':234863}
MAIN_ROOT=pathlib.Path('/home/ubuntu/nano-complete-nano-amd-38d46c8fa8684530a2669bd8c8922895/attempt01')
MAIN_BINDING_PIN={'sha256':'38016b85f9a36522e2a745cf5eef13e48df98fa73695b64d10ba0193e2db0afb','bytes':134243}
CMAKE={'sha256':'20b4b7c666777c389d401dd31b79755673a4aa4fac684800f6ee9a586b28f69f','bytes':176250}
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')

def need(ok):
 if not ok:raise RuntimeError('current AMD runtime relink adapter refused')

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

def ns(text):
 match=re.fullmatch(r'(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})[.](\d{9}) ([+-])(\d{2})(\d{2})',text);need(match is not None)
 y,mo,d,h,mi,s=map(int,match.groups()[:6]);zone=(int(match[9])*60+int(match[10]))*60;need(int(match[9])<=23 and int(match[10])<=59)
 if match[8]=='-':zone=-zone
 return (calendar.timegm(datetime.datetime(y,mo,d,h,mi,s).timetuple())-zone)*1000000000+int(match[7])
def guest_row(row):
 need(isinstance(row,dict) and set(row)=={'material','bytes','sha256'} and type(row['bytes']) is int and row['bytes']>0 and re.fullmatch('[0-9a-f]{64}',row['sha256']))
 r=row['material'];need(isinstance(r,list) and len(r)==9 and all(isinstance(q,str) for q in r))
 need(all(re.fullmatch('[0-9]+',r[i]) for i in (0,1,3,4,5,6)) and re.fullmatch('[0-9a-f]+',r[2]))
 values=[int(r[0]),int(r[1]),int(r[2],16),int(r[3]),int(r[4]),int(r[5]),int(r[6]),ns(r[7]),ns(r[8])]
 need(values[3]==1 and values[4]==values[5]==1000 and values[6]==row['bytes'] and stat.S_ISREG(values[2]))
 return dict(zip(F,values),sha256=row['sha256'])

def main(m):
 p=argparse.ArgumentParser();p.add_argument('--output-root',required=True);p.add_argument('--worker',required=True);a=p.parse_args()
 m.need(sys.platform.startswith('linux') and os.getuid()==os.getgid()==1000)
 candidate=pathlib.Path(a.output_root);worker=pathlib.Path(a.worker)
 m.need(candidate.is_absolute() and candidate.parent.resolve()==candidate.parent and candidate.parent.is_dir() and re.fullmatch('current-amd-rpc-runtime-relink-[a-z0-9-]{1,100}',candidate.name) and not candidate.exists() and not candidate.is_symlink())
 m.need(candidate.parent.stat().st_uid==1000 and not candidate.is_relative_to(m.R) and not candidate.is_relative_to(m.BASE) and all(not candidate.is_relative_to(q) and not q.is_relative_to(candidate) for q in (RPC_ROOT,HELPERS,MAIN_ROOT)) and not candidate.is_relative_to(m.CORE_SOURCE.parent))
 m.need(worker.is_absolute() and worker.resolve()==worker and not worker.is_relative_to(candidate))
 candidate.mkdir(mode=0o700);m.OUT=candidate;m.OUT_CREATED=True
 for leaf in ('source','controls','outputs','baseline','docker-client'):(m.OUT/leaf).mkdir(mode=0o700)
 m.DEADLINE=time.monotonic()+1600
 tools={pathlib.Path(q):m.digest(m.material(q)[1]) for q in ('/usr/bin/git','/usr/bin/docker','/usr/bin/sudo')};m.stable_new('os-utilities',tools);m.hold(pathlib.Path(__file__).resolve())
 worker_raw=m.hold(worker,WORKER_PIN,65536)
 HEAD=m.git('git-head',['rev-parse','HEAD']).decode().strip();m.need(re.fullmatch('[0-9a-f]{40}',HEAD) and m.git('git-clean',['status','--porcelain'])==b'')
 m.git('qualified-source-ancestor',['merge-base','--is-ancestor',SOURCE_REV,HEAD])
 cpp_trees={k:v for k,v in m.TREES.items() if k!='tests/multihost'};m.need(len(cpp_trees)==6)
 found={}
 for entry in m.git('git-selected-trees',['ls-tree','-z',HEAD,'--',*cpp_trees]).split(b'\0'):
  if not entry:continue
  meta,path=entry.split(b'\t',1);mode,kind,oid=meta.decode().split();key=path.decode();m.need(mode=='040000' and kind=='tree' and key not in found);found[key]=oid
 wanted_trees=dict(cpp_trees,src=SRC_TREE);m.need(found==wanted_trees)
 m.need(m.git('source-core-delta',['diff','--name-only',m.REV,HEAD,'--','src','include']).decode().splitlines()==['src/consensus_capella_runtime.cpp','src/rpc.cpp'])

 docs={key:json.loads(m.hold(m.BASE/rel,{'sha256':sha,'bytes':size},16777216)) for key,(rel,sha,size) in m.REFS.items()}
 # This external producer pin already binds the retained output's original
 # Linux GNU-nine row. Root RPC01 proved host/guest equality; do not refresh it.
 proof=json.loads(m.hold(RPC_ROOT/'result-public.json',RPC_RESULT_PIN,16384));m.need(proof['passed'] is True and proof['format']=='nano-rpc-simulation-context-qualification-v1' and proof['architecture']=='linux-amd64' and proof['image']==m.IMAGE and proof['coreMembers']==143 and proof['unchangedCoreMembers']==142 and proof['replacedCoreMembers']==['rpc.cpp.o'] and proof['rootNewHostOutputsMatchedOriginalGuestRows'] is True and proof['oldAuthoritiesRefreshed'] is False)
 producer_pin={'sha256':proof['softwareAdmissionSha256'],'bytes':proof['softwareAdmissionBytes']};m.need(producer_pin==CORE_BINDING_PIN)
 binding=json.loads(m.hold(RPC_ROOT/'outputs/software-admission.private.json',producer_pin,16777216));m.need(binding['format']=='nano-rpc-simulation-context-software-v1' and binding['oldAuthoritiesRefreshed'] is False and binding['guestMaterials']['/outputs/libnano_core.a']==proof['newCore'])
 core_row=guest_row(binding['guestMaterials']['/outputs/libnano_core.a'])
 m.need(set(core_row)==set(F)|{'sha256'} and core_row['sha256']==CORE_PIN['sha256'] and core_row['st_size']==CORE_PIN['bytes'])
 core_raw=m.hold(CORE,core_row);core=m.OUT/'baseline/libnano_core.a';m.write_new(core,core_raw,0o400);m.stable_new('current-core-copy',{core:CORE_PIN})
 helper=json.loads(m.hold(HELPERS/'outputs/software-admission.private.json',HELPER_BINDING_PIN,16777216));m.need(helper['format']=='nano-current-linux-selected-helper-software-binding-v1' and helper['owner']=='vps-docker' and helper['imageId']==m.IMAGE)
 tool=[q for q in helper['programs'] if q['role']=='lifecycleTool'];m.need(len(tool)==1);tool=tool[0];m.need(tool['source']=='/work/tests/capella/deposit_audit.cpp' and tool['target']=='nano-capella-deposit-audit' and tool['objectReused'] is False)
 main_binding=json.loads(m.hold(MAIN_ROOT/'outputs/software-admission.private.json',MAIN_BINDING_PIN,16777216));m.need(main_binding['format']=='nano-current-core-linux-software-binding-v1' and main_binding['mainReused'] is True)
 m.need(os.statvfs(m.OUT).f_bavail*os.statvfs(m.OUT).f_frsize>=3*1024**3)
 selected=[*cpp_trees,'tests/multihost/lifecycle_submit.cpp','CMakeLists.txt'];wanted={}
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
 m.need(seen==set(wanted) and empty_seen==m.EMPTY and expected[m.HEADER]==m.AFTER)
 for dest in sorted(source.rglob('*'),key=lambda q:len(q.parts),reverse=True):
  if dest.is_dir():os.chmod(dest,0o555)
 os.chmod(source,0o555);cmake_raw=m.material(source/'CMakeLists.txt')[1];m.need(m.digest(cmake_raw)==CMAKE);cmake=cmake_raw.decode()

 roles={'nano':('nano','src/main.cpp',None),**m.ROLES};mounts={};programs=[];joins={};orders={}
 def mount_row(path,guest,row):
  raw=m.hold(path,row);pin=m.digest(raw);m.need(pin=={'sha256':row['sha256'],'bytes':row['st_size']})
  m.need(guest not in mounts or mounts[guest]==str(path));mounts[guest]=str(path);return dict(path=guest,**pin)
 def mount_old(row,guest):
  m.old_file(row);m.need(guest not in mounts or mounts[guest]==row['path']);mounts[guest]=row['path'];expected[guest]={'sha256':row['sha256'],'bytes':row['bytes']}
 for role,(target,relative,count) in roles.items():
  for line in ('add_executable('+target+' '+relative+')','target_link_libraries('+target+' PRIVATE nano_core)'):m.need(sum(q.strip()==line for q in cmake.splitlines())==1)
  rec=docs['links'][target];m.need(set(rec)=={'compile','dependencyFile','executable','imageDependencies','linkRecipe','newSourceAndGeneratedDependencies','object'} and rec['executable']==docs['artifacts']['files'][str(m.BASE/'core'/target)])
  comp=rec['compile'];m.need(comp['file']=='/work/'+relative and comp['directory']=='/outputs')
  argv=comp.get('arguments') or m.shlex.split(comp['command']);m.need(argv[0]=='/usr/bin/clang++' and argv[-2:]==['-c',comp['file']] and argv[-4]=='-o' and argv.count('-o')==argv.count('-c')==1)
  own=os.path.normpath(argv[-3] if argv[-3].startswith('/') else '/outputs/'+argv[-3]);m.need(own.startswith('/outputs/') and own.endswith('.cpp.o'))
  m.need(rec['object']['path']==str(m.BASE/'core'/own[len('/outputs/'):]) and rec['dependencyFile']['path']==rec['object']['path']+'.d')
  flags=[q.replace('/outputs/third_party','/old/third_party') for q in argv[:-4]];macros=[q for q in flags if q.startswith('-DNANO_TEST_SOURCE=')]
  if role=='lifecycleTool':
   m.need(sum(q.strip()=='target_compile_definitions('+target+' PRIVATE NANO_TEST_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}")' for q in cmake.splitlines())==1 and len(macros)<=1)
   if macros:m.need(macros==['-DNANO_TEST_SOURCE="/work"'])
   else:flags.insert(1,'-DNANO_TEST_SOURCE="/work"')
  else:m.need(not macros)
  m.need(not any('NANO_SYNC_FAULT'in q for q in flags));projects=0;deltas=[]
  for guest,row in rec['newSourceAndGeneratedDependencies'].items():
   if guest.startswith('/work/'):
    m.need(row['path']==str(m.BASE/'source'/guest[len('/work/'):]) and guest in expected);projects+=1;oldpin={'sha256':row['sha256'],'bytes':row['bytes']}
    if expected[guest]!=oldpin:m.need(role=='lifecycleTool' and guest==m.HEADER and oldpin==m.BEFORE and expected[guest]==m.AFTER);deltas.append(guest)
   elif guest.startswith('/outputs/'):mount_old(row,'/old/'+guest[len('/outputs/'):])
   else:m.need(False)
  m.need((projects==count if count is not None else projects>0) and (deltas==[m.HEADER] if role=='lifecycleTool' else not deltas))
  link=rec['linkRecipe']['argv'];m.need(link[:3]==['/usr/bin/clang++','-O3','-DNDEBUG'] and link.count('-o')==1 and link.index('-o')==4 and os.path.normpath(link[3] if link[3].startswith('/') else '/outputs/'+link[3])==own and os.path.normpath(link[5] if link[5].startswith('/') else '/outputs/'+link[5])=='/outputs/'+target)
  tail=[];order=[];ars=[];ll=[]
  for word in link[6:]:
   guest=os.path.normpath('/outputs/'+word) if not word.startswith(('/','-')) else word
   if guest.startswith('/outputs/'):
    current='/baseline/libnano_core.a' if guest=='/outputs/libnano_core.a' else '/old/'+guest[len('/outputs/'):]
    if guest in docs['llvm']:row=docs['llvm'][guest]['object'];ll.append(guest)
    else:m.need(guest in docs['archives'] and guest.endswith('.a'));row=docs['archives'][guest]['archive'];ars.append(guest)
    if guest!='/outputs/libnano_core.a':m.need(row['path']==str(m.BASE/'core'/guest[len('/outputs/'):])) ;mount_old(row,current)
    tail.append(current);order.append({'old':guest,'current':current,'sha256':CORE_PIN['sha256'] if guest=='/outputs/libnano_core.a' else row['sha256']})
   else:m.need(guest.startswith(('/usr/','/lib/','/lib64/','-')));tail.append(guest)
  m.need(len(ars)==5 and ars.count('/outputs/libnano_core.a')==1 and len(ll)==2 and set(ll)==set(docs['llvm'])=={'/outputs/third_party/mcl/bint64.o','/outputs/third_party/mcl/base64.o'})
  object_guest='/inputs/'+role+'.cpp.o';md_guest=object_guest+'.d'
  if role=='lifecycleTool':
   obj=mount_row(HELPERS/'outputs/lifecycleTool.cpp.o',object_guest,guest_row(tool['object']));md=mount_row(HELPERS/'outputs/lifecycleTool.cpp.o.d',md_guest,guest_row(tool['dependencyFile']))
  else:
   if role=='nano':m.need(guest_row(main_binding['mainObject'])==m.original(rec['object']) and guest_row(main_binding['mainDependencyFile'])==m.original(rec['dependencyFile']))
   for row,g in ((rec['object'],object_guest),(rec['dependencyFile'],md_guest)):
    m.old_file(row);m.need(g not in mounts);mounts[g]=row['path']
   obj=dict(path=object_guest,sha256=rec['object']['sha256'],bytes=rec['object']['bytes']);md=dict(path=md_guest,sha256=rec['dependencyFile']['sha256'],bytes=rec['dependencyFile']['bytes'])
  reuse={'object':obj,'md':md,'dependencyPrefixMap':[{'from':'/outputs/third_party','to':'/old/third_party'}]}
  programs.append({'role':role,'source':'/work/'+relative,'compileFlags':flags,'linkPrefix':link[:3],'linkTail':tail,'reuse':reuse});joins[role]={'projectPins':projects,'explicitHeaderDeltas':deltas,'recordFromFixedReceipt':True,'retainedObjectAndMdStrict':True};orders[role]=order
 recipes={'format':'nano-current-linux-runtime-relink-recipes-v1','owner':'vps-docker','uid':1000,'gid':1000,'imageId':m.IMAGE,'elfMachine':62,'sourceRevision':HEAD,'programs':programs}
 controls={'recipes.json':(json.dumps(recipes,sort_keys=True)+'\n').encode(),'source-expected.private.json':(json.dumps(expected,sort_keys=True)+'\n').encode(),'core-pin.json':(json.dumps(CORE_PIN,sort_keys=True)+'\n').encode(),'worker.pl':worker_raw}
 for leaf,raw in controls.items():m.write_new(m.OUT/'controls'/leaf,raw,0o444)
 os.chmod(m.OUT/'controls',0o555);os.chmod(m.OUT/'baseline',0o555)
 m.save('runtime-relink-source-joins.private.json',{'head':HEAD,'effectiveSourceBase':SOURCE_REV,'treeEntries':found,'submitSourceBlob':wanted['tests/multihost/lifecycle_submit.cpp'],'wholeMultihostTreeQualified':False,'selectedCmake':CMAKE,'roleSourceJoins':joins,'linkOrders':orders,'toolCurrentProducer':HELPER_BINDING_PIN,'mainCurrentProducer':MAIN_BINDING_PIN,'coreCurrentProducer':producer_pin,'oldImageDependenciesUsedAsHistoricalAuthority':False,'fullCmakeConfigureExecuted':False})
 docker=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',str(m.OUT/'docker-client')]
 def dk(name,args,seconds=30,expected=0,cleanup=False):return m.run(name,docker+args,seconds,expected,cleanup=cleanup)
 name='nano-rpc-runtime-relink-amd-'+uuid.uuid4().hex;labelkey='nano.rpc.runtime.relink.amd';cid=None;attempted=False;first_error=None;result=None
 try:
  raw,err,_=dk('image-inspect',['image','inspect',m.IMAGE,'--format','{{json .}}']);image=json.loads(raw);m.need(not err and image['Id']==m.IMAGE and image['Architecture']=='amd64' and image['Os']=='linux')
  raw,err,_=dk('name-absent',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}']);m.need(not raw.strip() and not err)
  extra=[]
  for guest,host in sorted(mounts.items()):extra+=['--mount','type=bind,src='+host+',dst='+guest+',readonly']
  args=['create','--pull=never','--name',name,'--label',labelkey+'='+name,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','128','--user','1000:1000','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(core)+',dst=/baseline/libnano_core.a,readonly','--mount','type=bind,src='+str(m.OUT/'controls')+',dst=/controls,readonly','--mount','type=bind,src='+str(m.OUT/'outputs')+',dst=/outputs',*extra,'--workdir','/outputs','--entrypoint','/bin/sh',m.IMAGE,'-ec','exec /bin/sleep 1800']
  m.fence();attempted=True;raw,err,_=dk('create-relink',args);cid=raw.decode().strip();m.need(not err and re.fullmatch('[0-9a-f]{64}',cid));raw,err,_=dk('start-relink',['start',cid]);m.need(not err and raw.decode().strip()==cid)
  prime=r"use strict;use warnings;use Digest::SHA ();use JSON::PP ();my @r;for my $p (@ARGV){open(my $f,'<',$p)or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);push @r,{sha256=>$d->hexdigest,bytes=>-s $p};}print JSON::PP->new->canonical->encode(\@r);"
  names=sorted(controls);raw,err,_=dk('prime-known-controls',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-e',prime,*['/controls/'+q for q in names]]);m.need(not err and json.loads(raw)==[m.digest(controls[q]) for q in names]);m.stable_new('controls',{m.OUT/'controls'/q:m.digest(controls[q]) for q in names})
  m.save('original-selected-inputs-before-worker.private.json',m.HELD)
  raw,err,_=dk('worker-syntax',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-c','/controls/worker.pl']);m.need(not raw and err.strip()==b'/controls/worker.pl syntax OK')
  raw,err,code=dk('runtime-relink-worker',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','/controls/worker.pl','--current-core-runtime-relink'],1250,expected=None)
  # The outputs are a direct, private UID1000 bind. Record their inventory before
  # checking worker exit: a failed invocation's logs/refusal are never masked by
  # trying to read a success-only artifact or an empty tar stream.
  observed=[];total=0
  for path in sorted((m.OUT/'outputs').iterdir()):
   st=path.lstat();m.need(path.name not in ('.','..') and re.fullmatch('[A-Za-z0-9_.-]+',path.name) and stat.S_ISREG(st.st_mode) and st.st_nlink==1 and st.st_size<67108864);total+=st.st_size;m.need(total<=268435456);observed.append({'name':path.name,'bytes':st.st_size})
  m.need(len(observed)<=128);m.save('worker-output-inventory.private.json',{'observationalOnly':True,'workerExitCode':code,'files':observed,'totalBytes':total,'noSuccessArtifactRequiredForCollection':True})
  m.need(code==0 and not err);stdout_result=json.loads(raw)
  # These two JSON files are newly produced, never historical rows. Stabilize
  # their known bytes/SHA BEFORE first hold, without rebasing any old HELD row.
  json_pins={m.OUT/'outputs/result-public.json':m.digest(raw),m.OUT/'outputs/software-admission.private.json':{'sha256':stdout_result['softwareAdmissionSha256'],'bytes':stdout_result['softwareAdmissionBytes']}}
  m.stable_new('worker-json-products',json_pins)
  body=m.material(m.OUT/'outputs/result-public.json',16777216)[1];result=json.loads(body);m.need(stdout_result==result)
  m.need(result['format']=='nano-current-linux-runtime-relink-qualification-v1' and result['passed'] is True and result['owner']=='vps-docker' and result['uid']==result['gid']==1000 and result['imageId']==m.IMAGE and result['sourceRevisionDeclared']==HEAD)
  m.need(result['selectedPrograms']==5 and result['compiledTranslationUnits']==0 and result['archiveUpdates']==0 and result['coreCompilations']==0 and all(result[k]==5 for k in ('independentMCommands','linkCommands','loaderCommands','argumentRefusals')) and result['helpCommands']==1 and result['smokeCommands']==6)
  m.need(all(result[k]==0 for k in ('p2pMCommands','p2pLinks','p2pProgramRuns')) and result['databaseOpened'] is False and result['coreArchiveMutated'] is False and result['coreMembersRequalified'] is False and result['productionQualified'] is False and {'sha256':result['coreSha256'],'bytes':result['coreBytes']}==CORE_PIN)
  binding_raw=m.material(m.OUT/'outputs/software-admission.private.json',16777216)[1];m.need(m.digest(binding_raw)=={'sha256':result['softwareAdmissionSha256'],'bytes':result['softwareAdmissionBytes']});new_binding=json.loads(binding_raw)
  m.need(new_binding['format']=='nano-current-linux-runtime-relink-software-binding-v1' and new_binding['owner']=='vps-docker' and new_binding['imageId']==m.IMAGE and new_binding['sourceRevisionDeclared']==HEAD)
  public={q['role']:q for q in result['programs']};m.need(len(public)==5 and set(public)==set(roles));products={m.OUT/'outputs/software-admission.private.json':m.digest(binding_raw)}
  for role,(target,_,_) in roles.items():
   q=public[role];m.need(q['target']==target and q['objectReused'] is True and q['normalizedMEqualsRemappedMd'] is True and q['loaderPassed'] is True and q['argumentRefusalPassed'] is True and q['helpPassed']==(role=='nano') and q['smokeCommands']==(2 if role=='nano' else 1))
   path=m.OUT/'outputs'/target;row,body=m.material(path);pin={'sha256':q['softwareSha256'],'bytes':q['softwareBytes']};m.need(m.digest(body)==pin and stat.S_IMODE(row['st_mode'])==0o500 and row['st_uid']==row['st_gid']==1000 and row['st_nlink']==1);products[path]=pin
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
   try:m.save('cleanup-failure-public.json',{'passed':False,'firstFailurePreserved':first_error is not None,'errorType':type(error).__name__})
   except BaseException:pass
   if first_error is None:raise
 m.fence();m.need(result is not None and m.git('final-git-head',['rev-parse','HEAD']).decode().strip()==HEAD and m.git('final-git-clean',['status','--porcelain'])==b'')
 result.update({'rootHead':HEAD,'rootEffectiveSourceBase':SOURCE_REV,'rootSixSelectedCppTreesMatchCurrentCoreSource':True,'rootSubmitSourcePinnedToOriginalRetainedObject':True,'rootWholeMultihostTreeQualified':False,'rootOnlyRuntimeAndRpcDeltaSinceBd11':True,'rootCurrentMainAndToolProducerBound':True,'rootCurrentCoreProducerAndHostRowJoined':True,'rootSelectedTargetCmakeAsserted':True,'rootSelectedOriginalRowsFullNineAndShaUnchanged':True,'rootOwnedContainerAbsent':True,'rootPerlSyntaxPassed':True,'fixedProducerReceiptsRead':4,'oldSdkMapsRead':False,'oldSourceAdmissionRead':False,'oldAuthoritiesRefreshed':False,'p2pBoundByThisHost':False,'campaignAdmission':False,'productionQualified':False})
 m.save('result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 m=None;failed=False
 try:m=load_adapter();main(m)
 except BaseException:
  failed=True
  if m is not None and m.OUT_CREATED:
   try:m.save('failure-public.json',{'passed':False,'oldSdkMapsRead':False,'oldSourceAdmissionRead':False,'oldAuthoritiesRefreshed':False,'compilerOrTestSuccessClaimed':False,'rawErrorDetailsExported':False})
   except BaseException:pass
  print('current AMD runtime relink adapter refused',file=sys.stderr);sys.exit(1)
 finally:
  if m is not None:
   try:
    if m.ACTIVE is not None:m.stop(m.ACTIVE)
    m.fence()
    if m.OUT_CREATED:m.save('originals-final-fence-public.json',{'passed':True,'heldSelectedFiles':len(m.HELD),'firstFailurePreserved':failed,'oldAuthoritiesRefreshed':False})
   except BaseException as error:
    if m.OUT_CREATED:
     try:m.save('originals-final-fence-failure-public.json',{'passed':False,'firstFailurePreserved':failed,'errorType':type(error).__name__})
     except BaseException:pass
    if not failed:raise
