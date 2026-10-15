#!/usr/bin/env python3
"""Root-only VPS operator: public Git snapshot + NEW canonical AMD outputs.

No SSH, old build/cfg/SDK map, historical artifact or private fixture is read.
This source is not authorization to execute it. It prints typed counts only.
"""
import argparse,hashlib,json,os,pathlib,posixpath,re,resource,selectors,shlex,signal,stat,subprocess,tarfile,time,uuid

HERE=pathlib.Path(__file__).resolve().parent
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
REPO=pathlib.Path('/home/ubuntu/blockchain-nano-git')
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
BEGIN=time.monotonic();DEADLINE=BEGIN+1800
LOG_BOUND=64*1024*1024;ARCHIVE_BOUND=2*1024*1024*1024;NEW_BYTES_BOUND=8*1024*1024*1024
RUN_LOG_PREFIX='resume06-'
CONTROL=None;LOGS=None;QUALIFICATION=None;ADMITTED_COMPILERS=set();COMPILER_EXECUTABLES={};DEPENDENCY_RECORDS={}
namespace_admitted=False;mutable_bytes_bound=6*1024*1024*1024
stage='arguments';created_attempt=False;created_id=None;initial_absent=False;cleanup_ok=False;root=None;namespace_created=False;fixed_namespace_bytes=0;fixed_namespace_entries=0;log_bytes=0;source_rows={};aliases={};guest_result=None
def need(v,msg):
 if not v:raise RuntimeError(msg)
def source_error_frames(error):
 # Names/line numbers of this exact public operator only: no filename, message,
 # locals, SDK frames or stack text may leave the VPS.
 rows=[];tb=error.__traceback__
 while tb is not None:
  code=tb.tb_frame.f_code;function=code.co_name
  if code.co_filename==__file__ and(function=='<module>'or re.fullmatch('[A-Za-z_][A-Za-z0-9_]{0,63}',function))and isinstance(tb.tb_lineno,int)and 0<tb.tb_lineno<=2000:rows.append({'function':function,'line':tb.tb_lineno})
  tb=tb.tb_next
 return rows[-16:]
def tick():need(time.monotonic()<DEADLINE,'Whole operator wall deadline')
def new_write(path,raw,mode=0o400):
 fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,mode)
 try:
  os.fchmod(fd,mode);v=memoryview(raw)
  while v:
   n=os.write(fd,v);need(n>0,'New output write');v=v[n:]
  os.fsync(fd)
 finally:os.close(fd)
 fd=os.open(path.parent,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);os.fsync(fd);os.close(fd)
def doc(path,value):new_write(path,(json.dumps(value,sort_keys=True,indent=2)+'\n').encode(),0o600)
def read_new(path,expected=None,limit=ARCHIVE_BOUND):
 path=pathlib.Path(path);need(path.is_absolute() and path.resolve()==path,'New canonical path')
 fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode) and a.st_nlink==1 and a.st_uid==1000 and a.st_gid==1000 and 0<=a.st_size<=limit,'New owned bounded file')
  h=hashlib.sha256();n=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   h.update(b);n+=len(b);need(n<=a.st_size,'New file grew')
  row=[str(getattr(a,k)) for k in F];need(n==a.st_size and row==[str(getattr(os.fstat(fd),k)) for k in F]==[str(getattr(path.lstat(),k)) for k in F],'New FD/path material')
  r={'path':str(path),'material':row,'sha256':h.hexdigest(),'bytes':n}
  if expected:need(r==expected,'New held authority changed')
  return r
 finally:os.close(fd)
def valid_amd64_elf(path,pin,relocatable):
 # Validate bytes through the same held FD/full-nine; atime is never authority.
 fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK);row=None
 try:
  a=os.fstat(fd);row=[str(getattr(a,k))for k in F];need(row==pin['material']and str(path)==pin['path'],'ELF observed FD must match its exact file record')
  h=os.pread(fd,64,0)
  if len(h)!=64 or h[:7]!=b'\x7fELF\x02\x01\x01':return False
  typ=int.from_bytes(h[16:18],'little');machine=int.from_bytes(h[18:20],'little');version=int.from_bytes(h[20:24],'little');phoff=int.from_bytes(h[32:40],'little');shoff=int.from_bytes(h[40:48],'little');ehsize=int.from_bytes(h[52:54],'little');phentsize=int.from_bytes(h[54:56],'little');phnum=int.from_bytes(h[56:58],'little');shentsize=int.from_bytes(h[58:60],'little');shnum=int.from_bytes(h[60:62],'little');shstr=int.from_bytes(h[62:64],'little')
  if machine!=62 or version!=1 or ehsize!=64 or typ not in ((1,)if relocatable else (2,3)) or not(0<shnum<65535)or shstr>=shnum or shentsize!=64 or shoff<64 or shoff+shnum*64>a.st_size:return False
  if phnum and(phnum==65535 or phentsize!=56 or phoff<64 or phoff+phnum*56>a.st_size):return False
  for i in range(shnum):
   section=os.pread(fd,64,shoff+64*i)
   if len(section)!=64:return False
   kind=int.from_bytes(section[4:8],'little');offset=int.from_bytes(section[24:32],'little');size=int.from_bytes(section[32:40],'little')
   if kind!=8 and(offset>a.st_size or size>a.st_size-offset):return False
  return True
 finally:
  try:
   if row is not None:need(row==[str(getattr(os.fstat(fd),k))for k in F]==[str(getattr(pathlib.Path(path).lstat(),k))for k in F],'ELF held FD/path full-nine changed')
  finally:os.close(fd)
def mutable_walk_error(error):
 if not isinstance(error,FileNotFoundError):raise error
def run(label,args,seconds=60,maximum=2*1024*1024,binary_output=None,cleanup=False):
 global log_bytes
 if not cleanup:tick()
 until=time.monotonic()+seconds if cleanup else min(DEADLINE,time.monotonic()+seconds)
 fd=os.open(LOGS/(RUN_LOG_PREFIX+label+'.log'),os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600);child=None;captured=bytearray();out=None
 try:
  if binary_output is not None:out=os.open(binary_output,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
  child=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,cwd='/home/ubuntu',env={'PATH':'/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin','HOME':'/home/ubuntu','LANG':'C','LC_ALL':'C','GIT_CONFIG_NOSYSTEM':'1','GIT_CONFIG_GLOBAL':'/dev/null','GIT_OPTIONAL_LOCKS':'0','GIT_NO_REPLACE_OBJECTS':'1'},start_new_session=True)
  sel=selectors.DefaultSelector()
  for stream in (child.stdout,child.stderr):os.set_blocking(stream.fileno(),False);sel.register(stream,selectors.EVENT_READ)
  total=0;last_disk=time.monotonic()
  while sel.get_map():
   need(time.monotonic()<until,'Owned subprocess deadline')
   for key,_ in sel.select(0.2):
    b=os.read(key.fd,65536)
    if not b:sel.unregister(key.fileobj);continue
    if binary_output is not None and key.fileobj is child.stdout:
     total+=len(b);need(total<=ARCHIVE_BOUND,'Tracked archive byte bound');dest=out
    else:
     log_bytes+=len(b);need(len(captured)+len(b)<=maximum and log_bytes<=LOG_BOUND,'Operator log bound');captured.extend(b);dest=fd
    v=memoryview(b)
    while v:
     n=os.write(dest,v);need(n>0,'Stream write');v=v[n:]
   if time.monotonic()-last_disk>=5:
    entries=fixed_namespace_entries;size=fixed_namespace_bytes
    trees=[root/'core',root/'network',CONTROL,LOGS,QUALIFICATION]
    for base,dirs,files in (row for tree in trees for row in os.walk(tree,followlinks=False,onerror=mutable_walk_error)):
     entries+=len(dirs)+len(files);need(entries<=120000,'New namespace file bound')
     for leaf in files:
      try:size+=os.lstat(pathlib.Path(base)/leaf).st_size
      except FileNotFoundError:continue
    need(size<=NEW_BYTES_BOUND and size-fixed_namespace_bytes<=mutable_bytes_bound,'Retained public source plus new mutable disk bound');last_disk=time.monotonic()
  sel.close();code=child.wait(timeout=max(0.1,until-time.monotonic()));need(code==0,'Owned subprocess nonzero')
 finally:
  if child is not None and child.poll() is None:
   try:os.killpg(child.pid,signal.SIGTERM);child.wait(timeout=3)
   except (ProcessLookupError,subprocess.TimeoutExpired):
    try:os.killpg(child.pid,signal.SIGKILL)
    except ProcessLookupError:pass
    child.wait(timeout=3)
  if child is not None:child.stdout.close();child.stderr.close()
  if out is not None:os.fsync(out);os.close(out)
  os.fsync(fd);os.close(fd)
 return bytes(captured)
def docker(label,args,seconds=20,maximum=2*1024*1024,cleanup=False):return run(label,['sudo','-n','docker',*args],seconds,maximum,cleanup=cleanup)
def exact_ids(label,cleanup=False):return docker(label,['ps','-a','--filter','name=^/'+name+'$','--format','{{.ID}}'],8,65536,cleanup).decode().splitlines()
def cleanup():
 if not exact_ids('cleanup-presence',True):return True
 if not created_attempt:return False
 raw=docker('cleanup-inspect',['inspect','--type','container',name,'--format','{"id":{{json .Id}},"name":{{json .Name}},"image":{{json .Image}},"uuid":{{json (index .Config.Labels "nano.canonical-amd")}}}'],8,65536,True);r=json.loads(raw)
 need(r['name']=='/'+name and r['image']==IMAGE and r['uuid']==nonce and re.fullmatch('[0-9a-f]{64}',r['id']),'Cleanup identity')
 if created_id is not None:need(r['id']==created_id,'Cleanup producer full ID')
 docker('cleanup-rm',['rm','--force',r['id']],12,65536,True)
 return not exact_ids('cleanup-final-presence',True)
def source_fence():
 for p,r in source_rows.items():read_new(p,r)
 for p,r in aliases.items():
  p=pathlib.Path(p);s=p.lstat();need([str(getattr(s,k)) for k in F]==r['material'] and os.readlink(p)==r['target'],'New source alias changed')
def interrupted(*_):raise RuntimeError('Root operator cancelled')
signal.signal(signal.SIGTERM,interrupted);signal.signal(signal.SIGINT,interrupted)

TARGETS={'nano':'src/main.cpp','nano-capella-isolated-bootstrap-audit':'tests/capella/isolated_bootstrap_audit.cpp','nano-capella-isolated-bootstrap-import':'tests/capella/isolated_bootstrap_import.cpp','nano-capella-isolated-stopped-capture':'tests/capella/isolated_stopped_capture.cpp','nano-capella-deposit-audit':'tests/capella/deposit_audit.cpp','nano-capella-lifecycle-submit':'tests/multihost/lifecycle_submit.cpp','nano-capella-validator-read-tests':'tests/test_capella_validator_read.cpp'}
def mapped(path):
 p=pathlib.PurePosixPath(posixpath.normpath(path));need(p.is_absolute(),'Canonical guest output/source path')
 for prefix,leaf in [('/work','source'),('/outputs','core'),('/network-build','network'),('/qualification','qualification06')]:
  if str(p).startswith(prefix+'/'):return root/leaf/p.relative_to(prefix)
 raise RuntimeError('Path is not a NEW host mount')
def guest(label,args,seconds=120,maximum=16*1024*1024):
 return docker(label,['exec','--user','1000:1000',created_id,*args],seconds,maximum)
def compile_args(r):
 args=r.get('arguments') or shlex.split(r['command']);need(isinstance(args,list) and args.count('-o')==1 and args.count('-c')==1,'Canonical compile argv');return args
def object_guest(r):
 args=compile_args(r);p=pathlib.PurePosixPath(args[args.index('-o')+1]);return str(p if p.is_absolute() else pathlib.PurePosixPath(r['directory'])/p)
ASM_SOURCE='/work/third_party/ckzg/blst/build/assembly.S'
ASM_OBJECT='/outputs/third_party/ckzg/CMakeFiles/nano_blst.dir/blst/build/assembly.S.o'
def asm_dependency_record(r):
 # The original ASM producer has no MD sidecar. Produce ONE new object with
 # the same semantic argv, require byte identity, and qualify its real new MD.
 global stage
 obj=object_guest(r);need(r['file']==ASM_SOURCE and obj==ASM_OBJECT,'Only the one publicly declared blst ASM source may use the missing-MD exception')
 original=mapped(obj);missing=pathlib.Path(str(original)+'.d');need(not os.path.lexists(missing),'Original ASM MD absence must remain explicit')
 if obj in DEPENDENCY_RECORDS:
  record=DEPENDENCY_RECORDS[obj];read_new(original,record['object']);read_new(record['qualificationObject']['path'],record['qualificationObject']);read_new(record['dependencyFile']['path'],record['dependencyFile']);return record
 pin=read_new(original);need(valid_amd64_elf(original,pin,True),'Original assembler object must be a valid AMD64 ET_REL')
 args=compile_args(r);need(args[0]in ADMITTED_COMPILERS,'ASM uses the exact already admitted image compiler');replay=[];j=0
 while j<len(args):
  a=args[j]
  if a=='-o':need(j+1<len(args),'Original ASM output value');replay+=['-o','/qualification/blst-assembly.o'];j+=2;continue
  if a in ('-MF','-MT','-MQ'):need(j+1<len(args),'Original ASM dependency output value');j+=2;continue
  if a in ('-MD','-MMD','-MP','-M','-MM'):j+=1;continue
  need(not a.startswith(('-MF','-MT','-MQ','-MJ','-save-temps','-ftime-trace','--serialize-diagnostics')),'Unexpected ASM side-output flag');replay.append(a);j+=1
 need(replay.count(ASM_SOURCE)==1 and replay.count('-c')==1,'Original ASM source and compilation semantics preserved')
 replay[0]=COMPILER_EXECUTABLES[args[0]]
 replay+=['-MD','-MF','/qualification/blst-assembly.o.d','-MT','/qualification/blst-assembly.o'];directory=posixpath.normpath(r['directory']);need(directory=='/outputs/third_party/ckzg','Exact ASM producer working directory')
 script=CONTROL/'qualify-single-asm-md.sh';body='set -eu\ncd '+shlex.quote(directory)+'\nexec '+shlex.join(replay)+'\n';new_write(script,body.encode());script_pin=read_new(script)
 stage='one-asm-object-with-real-md';guest('one-asm-object-with-real-md',['/bin/sh','/control/'+script.name],120,1024*1024);read_new(script,script_pin)
 fresh=read_new(QUALIFICATION/'blst-assembly.o');dep=read_new(QUALIFICATION/'blst-assembly.o.d',limit=4*1024*1024);need(valid_amd64_elf(QUALIFICATION/'blst-assembly.o',fresh,True),'New assembler product AMD64 ET_REL sections')
 need(fresh['sha256']==pin['sha256']and fresh['bytes']==pin['bytes'],'New MD-bearing assembler object must exactly equal the original object and archive member')
 raw=(QUALIFICATION/'blst-assembly.o.d').read_bytes();need(hashlib.sha256(raw).hexdigest()==dep['sha256'],'Real new ASM MD producer bytes');names=normalized_dependency_names(raw,r);local={};sdk=[]
 for name_ in sorted(names):
  if name_.startswith(('/work/','/outputs/','/network-build/')):local[name_]=read_new(mapped(name_).resolve(strict=True))
  else:need(re.fullmatch(r'/(?:usr|lib|lib64|src/\.deps)/[A-Za-z0-9_+./:@=~-]+',name_),'ASM dependencies remain in public source or immutable image');sdk.append(name_)
 need(ASM_SOURCE in local,'New real ASM MD must include its original public source');read_new(original,pin);read_new(dep['path'],dep);need(not os.path.lexists(missing),'Never create a historical ASM MD sidecar')
 record={'compile':r,'object':pin,'dependencyFile':dep,'newSourceAndGeneratedDependencies':local,'imageDependencies':sdk,'qualificationObject':fresh,'originalCompilerDependencyFilePresent':False,'newMdProducerArgv':replay,'newObjectByteEqualsOriginal':True}
 DEPENDENCY_RECORDS[obj]=record;stage='canonical-closure';return record
def dependency_record(r):
 if r.get('file')==ASM_SOURCE:return asm_dependency_record(r)
 obj=object_guest(r);d=mapped(obj+'.d');b=d.read_bytes();need(0<len(b)<=4*1024*1024,'Actual compiler dependency bound')
 text=b.decode().replace('\\\n','');need(':'in text,'Compiler dependency target');_,body=text.split(':',1);tokens=shlex.split(body,comments=False);need(0<len(tokens)<=20000,'Compiler dependency count')
 local={};sdk=set()
 for token in tokens:
  p=pathlib.PurePosixPath(token);p=p if p.is_absolute() else pathlib.PurePosixPath(r['directory'])/p;name=posixpath.normpath(str(p));p=pathlib.PurePosixPath(name)
  need(p.is_absolute() and not any(c in name for c in '\0\r\n'),'Compiler dependency path')
  if name.startswith(('/work/','/outputs/','/network-build/')):
   h=mapped(name).resolve(strict=True);need(h.is_relative_to(root),'Dependency alias escapes NEW namespace');local[name]=read_new(h)
  else:
   need(re.fullmatch(r'/(?:usr|lib|lib64|src/\.deps)/[A-Za-z0-9_+./:@=~-]+',name),'Only immutable-image SDK paths');sdk.add(name)
 need(r['file']in local,'Compiler source missing in actual dependency file')
 record={'compile':r,'object':read_new(mapped(obj)),'dependencyFile':read_new(d),'newSourceAndGeneratedDependencies':local,'imageDependencies':sorted(sdk)}
 if obj in DEPENDENCY_RECORDS:need(DEPENDENCY_RECORDS[obj]==record,'Repeated compiler dependency record changed')
 DEPENDENCY_RECORDS[obj]=record
 return record
def normalized_dependency_names(raw,r):
 text=raw.decode().replace('\\\n','');need(':'in text,'Actual dependency target')
 _,body=text.split(':',1);tokens=shlex.split(body,comments=False);need(0<len(tokens)<=20000,'Actual dependency count');names=set()
 for token in tokens:
  p=pathlib.PurePosixPath(token);name=posixpath.normpath(str(p if p.is_absolute()else pathlib.PurePosixPath(r['directory'])/p))
  need(name.startswith(('/work/','/outputs/','/network-build/','/usr/','/lib/','/lib64/','/src/.deps/'))and not any(c in name for c in '\0\r\n'),'Actual dependency permitted namespace');names.add(name)
 return names
def independent_dependency_probes(admitted_compilers):
 # Dependency-only preprocessing, not object recompilation or protocol tests.
 # Preserve every original semantic compiler flag; change only output controls.
 body='set -eu\nmkdir /tmp/nano-dependency-only\n';ordered=sorted(DEPENDENCY_RECORDS);expected={};argvs={}
 need(151<=len(ordered)<=500,'Canonical151 plus bounded linked dependency compile records')
 for i,obj in enumerate(ordered):
  record=DEPENDENCY_RECORDS[obj];r=record['compile'];args=compile_args(r);need(args[0]in admitted_compilers,'Dependency probe uses an already admitted image compiler');out=[];j=1
  while j<len(args):
   a=args[j]
   if a in ('-o','-MF','-MT','-MQ'):
    need(j+1<len(args),'Compiler output control argument');j+=2;continue
   if a in ('-c','-MD','-MMD','-MP','-M','-MM'):j+=1;continue
   need(not a.startswith(('-MF','-MT','-MQ','-MJ','-save-temps','-ftime-trace','--serialize-diagnostics')),'Unexpected compiler output control');out.append(a);j+=1
  need(out.count(r['file'])==1,'Exact compiler source preserved for dependency-only probe')
  directory=posixpath.normpath(r['directory']);need(directory in ('/outputs','/network-build')or directory.startswith(('/outputs/','/network-build/')),'Actual compiler working directory')
  probe=[COMPILER_EXECUTABLES[args[0]]]+out+['-M','-MF','/tmp/nano-dependency-only/current.d','-MT','nano_dependency_probe']
  argvs[obj]={'argv':probe,'directory':directory};expected[obj]=normalized_dependency_names(pathlib.Path(record['dependencyFile']['path']).read_bytes(),r)
  begin='NANO_DEPENDENCY_BEGIN_'+str(i);end='NANO_DEPENDENCY_END_'+str(i)
  body+='cd '+shlex.quote(directory)+'\nif '+shlex.join(probe)+' > /tmp/nano-dependency-only/diagnostic 2>&1; then :; else head -c 65536 /tmp/nano-dependency-only/diagnostic; exit 1; fi\ntest "$(wc -c < /tmp/nano-dependency-only/current.d)" -le 4194304\nprintf \'%s\\n\' '+shlex.quote(begin)+'\ncat /tmp/nano-dependency-only/current.d\nprintf \'\\n%s\\n\' '+shlex.quote(end)+'\nrm /tmp/nano-dependency-only/current.d /tmp/nano-dependency-only/diagnostic\n'
 script=CONTROL/'independent-compiler-dependencies.sh';new_write(script,body.encode());script_pin=read_new(script)
 raw=guest('independent-compiler-dependencies',['/bin/sh','/control/'+script.name],600,24*1024*1024);read_new(script,script_pin);remaining=raw;proof={}
 for i,obj in enumerate(ordered):
  begin=('NANO_DEPENDENCY_BEGIN_'+str(i)+'\n').encode();end=('\nNANO_DEPENDENCY_END_'+str(i)+'\n').encode();need(remaining.startswith(begin),'Independent dependency batch order');remaining=remaining[len(begin):];cut=remaining.find(end);need(0<cut<=4*1024*1024,'Independent dependency record bound');actual=remaining[:cut];remaining=remaining[cut+len(end):]
  record=DEPENDENCY_RECORDS[obj];r=record['compile'];names=normalized_dependency_names(actual,r);need(names==expected[obj],'Independent -M differs from actual compiler .o.d dependencies');read_new(record['object']['path'],record['object']);read_new(record['dependencyFile']['path'],record['dependencyFile'])
  if 'qualificationObject'in record:read_new(record['qualificationObject']['path'],record['qualificationObject']);need(not os.path.lexists(pathlib.Path(record['object']['path']+'.d')),'Original ASM MD remains absent after independent -M')
  proof[obj]={**argvs[obj],'independentDependencyBytes':len(actual),'independentDependencySha256':hashlib.sha256(actual).hexdigest(),'normalizedDependencies':sorted(names),'matchesActualCompilerDependencyFile':True,'originalCompilerDependencyFilePresent':record.get('originalCompilerDependencyFilePresent',True)}
 need(not remaining,'Independent dependency batch has no extra output');doc(CONTROL/'independent-compiler-dependency-equality.private.json',proof)
 need(sum('qualificationObject'in r for r in DEPENDENCY_RECORDS.values())==1,'Exactly one justified ASM MD-bearing replay')
 return len(ordered)
def llvm_records(admitted_compilers):
 records={}
 for stem in ('bint','base'):
  obj='/outputs/third_party/mcl/'+stem+'64.o';src='/work/third_party/mcl/src/'+stem+'64.ll';make=mapped('/outputs/third_party/mcl/CMakeFiles/gen_'+stem+'.o.dir/build.make')
  raw=make.read_bytes();need(0<len(raw)<=1048576,'LLVM producer recipe bound');matches=[]
  for line in raw.decode().splitlines():
   if not line.startswith('\t'):continue
   args=shlex.split(line.strip())
   if obj in args and src in args:
    if '&&'in args:args=args[args.index('&&')+1:]
    if args[1:]==['-c','-o',obj,src,'-O3','-fPIC','-fno-ident']:matches.append(args)
  need(len(matches)==1 and matches[0][0]in admitted_compilers,'Exact pinned LLVM producer argv')
  pin=read_new(mapped(obj));need(valid_amd64_elf(mapped(obj),pin,True),'LLVM AMD64 ET_REL full ELF sections')
  records[obj]={'object':pin,'publicLlvmSource':read_new(mapped(src)),'producerRecipe':read_new(make),'argv':matches[0]}
 return records
def new_archive_records(paths,all_compile,llvm):
 records={};rows_by_object={object_guest(r):r for r in all_compile}
 recipes=list((root/'core').rglob('link.txt'))+list((root/'network').rglob('link.txt'));need(len(recipes)<=1200,'Generated link recipe bound')
 for index,path in enumerate(sorted(paths)):
  h=mapped(path);matches=[]
  for recipe in recipes:
   guest_base='/outputs/'+str(recipe.parent.parent.parent.relative_to(root/'core')) if recipe.is_relative_to(root/'core') else '/network-build/'+str(recipe.parent.parent.parent.relative_to(root/'network'))
   guest_base=posixpath.normpath(guest_base)
   for line in recipe.read_text().splitlines():
    args=shlex.split(line)
    if len(args)<4 or re.fullmatch(r'(?:llvm-)?ar(?:-[0-9]+)?',pathlib.PurePosixPath(args[0]).name)is None:continue
    out=posixpath.normpath(args[2] if args[2].startswith('/')else guest_base+'/'+args[2])
    if out==path:matches.append((recipe,guest_base,args))
  need(len(matches)==1,'One actual producer recipe per NEW linked archive');recipe,base,args=matches[0]
  objects=[posixpath.normpath(x if x.startswith('/')else base+'/'+x)for x in args[3:]];need(objects and all(x.endswith('.o')for x in objects),'Archive actual object inputs')
  members=guest('new-archive-members-'+str(index),['ar','t',path],15,65536).decode().splitlines();need(members==[pathlib.PurePosixPath(x).name for x in objects] and len(set(members))==len(members),'Exact NEW archive member order/namespace')
  compiled={}
  for obj in objects:
   if obj in llvm:compiled[obj]=llvm[obj]
   else:need(obj in rows_by_object,'NEW archive object lacks canonical compile row');compiled[obj]=dependency_record(rows_by_object[obj])
  body='set -eu\n'
  for obj in objects:body+='printf \'%s\\n\' '+shlex.quote(pathlib.PurePosixPath(obj).name)+'\nar p '+shlex.quote(path)+' '+shlex.quote(pathlib.PurePosixPath(obj).name)+' | sha256sum\n'
  script=CONTROL/('archive-dependencies-'+str(index)+'.sh');new_write(script,body.encode());pin=read_new(script)
  lines=guest('new-archive-digests-'+str(index),['/bin/sh','/control/'+script.name],30,65536).decode().splitlines();read_new(script,pin);need(len(lines)==2*len(objects),'NEW archive digest rows')
  for i,obj in enumerate(objects):need(lines[2*i]==pathlib.PurePosixPath(obj).name and lines[2*i+1].split()[0]==compiled[obj]['object']['sha256'],'NEW linked archive/member actual bytes')
  records[path]={'archive':read_new(h),'producerRecipe':read_new(recipe),'argv':args,'objects':compiled}
 return records
def sdk_snapshot(paths,label):
 # Only paths reached in the read-only image are observed here. No host SDK,
 # old build/cache, private input or Docker socket is ever mounted in it.
 paths=sorted(set(paths));need(0<len(paths)<=20000,'Image dependency count')
 for p in paths:need(re.fullmatch(r'/(?:usr|lib|lib64|src/\.deps)/[A-Za-z0-9_+./:@=~-]+',p),'Image path whitelist')
 body=b'''set -eu
while IFS= read -r p; do
 r=$(readlink -f -- "$p")
 test -f "$r"
 exec 3<"$r"
 a=$(stat -Lc '%d|%i|%f|%h|%u|%g|%s|%y|%z' /proc/self/fd/3)
 test "$a" = "$(stat -Lc '%d|%i|%f|%h|%u|%g|%s|%y|%z' -- "$r")"
 digest=$(sha256sum <&3); digest=${digest%% *}
 test "$a" = "$(stat -Lc '%d|%i|%f|%h|%u|%g|%s|%y|%z' /proc/self/fd/3)"
 test "$a" = "$(stat -Lc '%d|%i|%f|%h|%u|%g|%s|%y|%z' -- "$r")"
 exec 3<&-
 printf '%s\\n' "$p" "$r" "$a" "$digest"
done < "$1"
'''
 script=CONTROL/(label+'.sh');listing=CONTROL/(label+'.paths');new_write(script,body);new_write(listing,('\n'.join(paths)+'\n').encode())
 # Known producer bytes are held before the shell can consume the new files.
 held_script=read_new(script);held_listing=read_new(listing)
 raw=guest(label,['/bin/sh','/control/'+script.name,'/control/'+listing.name],120,16*1024*1024)
 read_new(script,held_script);read_new(listing,held_listing);lines=raw.decode().splitlines();need(len(lines)==4*len(paths),'Image FD snapshot rows');out={}
 for i,p in enumerate(paths):
  key,real,fields,digest=lines[4*i:4*i+4];parts=fields.split('|');need(key==p and real.startswith(('/usr/','/lib/','/lib64/','/src/.deps/')) and len(parts)==9 and re.fullmatch('[0-9a-f]{64}',digest),'Image material/SHA row')
  need(all(x.isdigit() for x in [parts[0],parts[1],parts[3],parts[4],parts[5],parts[6]]) and int(parts[3])>=1 and stat.S_ISREG(int(parts[2],16)),'Image regular material and retained nlink')
  out[p]={'resolvedPath':real,'materialOrder':['dev','ino','modeHex','nlink','uid','gid','size','mtimeNsText','ctimeNsText'],'material':parts,'sha256':digest}
 doc(CONTROL/(label+'.private.json'),out);return out
def canonical_build():
 global stage,created_id,created_attempt,ADMITTED_COMPILERS,COMPILER_EXECUTABLES
 stage='create-owned-build-container';source_fence();need(not exact_ids('immediate-precreate-name-absence'),'Exact owned name absent immediately before create');created_attempt=True
 raw=docker('container-create',['create','--pull','never','--platform','linux/amd64','--name',name,'--label','nano.canonical-amd='+nonce,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','256','--cgroupns','private','--ulimit','nofile=65536:65536','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount','type=bind,src='+str(root/'source')+',dst=/work,readonly','--mount','type=bind,src='+str(CONTROL)+',dst=/control,readonly','--mount','type=bind,src='+str(root/'core')+',dst=/outputs','--mount','type=bind,src='+str(root/'network')+',dst=/network-build,readonly','--mount','type=bind,src='+str(QUALIFICATION)+',dst=/qualification','--entrypoint','/bin/sh',IMAGE,'-ec','while :; do sleep 3600; done'],30)
 created_id=raw.decode().strip();need(re.fullmatch('[0-9a-f]{64}',created_id),'Docker producer full ID');docker('container-start',['start',created_id],15)
 stage='image-build-tools';toolnames=['cmake','clang','clang++','make','ar','ranlib','ld','ldd','stat','sha256sum','readlink']
 script='set -eu; test "$(id -u)" = 1000; test "$(id -g)" = 1000; for x in '+ ' '.join(toolnames)+'; do command -v "$x"; done; test -r /src/.deps/libp2p-build/CMakeCache.txt'
 tools=guest('image-required-tools',['/bin/sh','-ec',script],15,65536).decode().splitlines();need(len(tools)==len(toolnames),'Image tools exact');tools_before=sdk_snapshot(tools,'image-tools-before')
 COMPILER_EXECUTABLES={value:value for value in (tools[1],tools[2],tools_before[tools[1]]['resolvedPath'],tools_before[tools[2]]['resolvedPath'])};COMPILER_EXECUTABLES.update({'clang':tools[1],'clang++':tools[2]});ADMITTED_COMPILERS=set(COMPILER_EXECUTABLES)
 # Closure-only: all outputs come from the successful04 canonical build.
 # No configure, Make, Core/C++ compilation or extraction is invoked here.
 stage='canonical-closure'
 cc=json.loads((root/'core'/'compile_commands.json').read_bytes());nc=json.loads((root/'network'/'compile_commands.json').read_bytes());need(isinstance(cc,list) and len(cc)<=1000 and isinstance(nc,list) and len(nc)<=1000,'Compile databases bounded')
 expected={'/work/src/'+p.name for p in (root/'source'/'src').glob('*.cpp') if p.name!='main.cpp'};need(len(expected)==143,'Current canonical Core source count')
 core_rows=[r for r in cc if '/CMakeFiles/nano_core.dir/'in object_guest(r)];need(len(core_rows)==143 and {r['file']for r in core_rows}==expected,'Exact canonical143 compilation rows')
 records={r['file']:dependency_record(r) for r in core_rows};archive=root/'core'/'libnano_core.a'
 members=guest('archive-members',['ar','t','/outputs/libnano_core.a'],15,65536).decode().splitlines();need(len(members)==143 and len(set(members))==143,'Canonical143 archive members')
 # One batch hashes each exact member via image ar -> sha256sum. All names
 # derive from validated current object basenames, no guessed prior archive.
 body='set -eu\n'
 for r in core_rows:
  obj=pathlib.PurePosixPath(object_guest(r));need(re.fullmatch('[A-Za-z0-9_]+\\.cpp\\.o',obj.name) and obj.name in members,'Exact archive member basename')
  body+='printf \'%s\\n\' '+shlex.quote(obj.name)+'\nar p /outputs/libnano_core.a '+shlex.quote(obj.name)+' | sha256sum\n'
 script_path=CONTROL/'archive-members.sh';new_write(script_path,body.encode());pin=read_new(script_path)
 lines=guest('archive-member-digests',['/bin/sh','/control/archive-members.sh'],60,65536).decode().splitlines();read_new(script_path,pin);need(len(lines)==286,'Archive digest count')
 for i,r in enumerate(core_rows):
  member_name=pathlib.PurePosixPath(object_guest(r)).name;need(lines[2*i] ==member_name and lines[2*i+1].split()[0]==records[r['file']]['object']['sha256'],'Archive/compiler object bytes')
 links={};sdk=set();new_archives=set();llvm=llvm_records(ADMITTED_COMPILERS)
 for r in records.values():sdk.update(r['imageDependencies'])
 for target,source in [*TARGETS.items(),('nano-p2p','network/main.cpp')]:
  base='/network-build'if target=='nano-p2p'else'/outputs';db=nc if target=='nano-p2p'else cc;rows=[r for r in db if r['file']=='/work/'+source and '/CMakeFiles/'+target+'.dir/'in object_guest(r)];need(len(rows)==1,'Exact target compile row');record=dependency_record(rows[0]);sdk.update(record['imageDependencies'])
  link=mapped(base+'/CMakeFiles/'+target+'.dir/link.txt');b=link.read_bytes();need(0<len(b)<=1048576,'Link recipe bounded');args=shlex.split(b.decode());need(args.count('-o')==1,'Link output flag')
  absarg=lambda x:str(pathlib.PurePosixPath(x) if x.startswith('/') else pathlib.PurePosixPath(base)/x)
  object_inputs=[posixpath.normpath(absarg(x))for x in args if x.endswith('.o')]
  expected_objects=[object_guest(rows[0])]+([]if target=='nano-p2p'else list(llvm))
  need(absarg(args[args.index('-o')+1])==base+'/'+target and len(object_inputs)==len(expected_objects) and set(object_inputs)==set(expected_objects),'Exact consumer plus pinned portable LLVM objects')
  if target!='nano-p2p':need(sum(absarg(x)=='/outputs/libnano_core.a'for x in args if x.endswith('.a'))==1,'Current Core archive exactly once')
  for token in args:
   if token.endswith(('.a','.so'))or re.search(r'\.so\.[0-9.]+$',token):
    path=posixpath.normpath(absarg(token))
    if path.startswith(('/outputs/','/network-build/')):
     need(path.endswith('.a'),'Only known NEW static dependency archives');new_archives.add(path)
    else:need(path.startswith(('/usr/','/lib/','/lib64/','/src/.deps/')),'Link input outside image/new outputs');sdk.add(path)
  elf=mapped(base+'/'+target)
  need(valid_amd64_elf(elf,read_new(elf),False),'AMD64 executable full ELF section bounds')
  listing=guest('ldd-'+target,['ldd',base+'/'+target],15,65536);need(listing and b'not found'not in listing,'Loader resolved')
  # Retain link recipes and resolved DSO paths on VPS only, never in stdout.
  for line in listing.decode().splitlines():
   for token in line.split():
    if token.startswith(('/usr/','/lib/','/lib64/','/src/.deps/')):sdk.add(token)
  record['linkRecipe']={'path':str(link),'argv':args,'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)};record['executable']=read_new(elf);links[target]=record
 need('/outputs/libnano_core.a'in new_archives and len(new_archives)<=17,'Core plus bounded NEW dependency archives');archive_records=new_archive_records(new_archives,[*cc,*nc],llvm)
 need(set(archive_records['/outputs/libnano_core.a']['objects'])=={object_guest(r)for r in core_rows},'Core ar producer exactly the canonical143 object rows')
 for record in archive_records.values():
  for obj in record['objects'].values():sdk.update(obj.get('imageDependencies',[]))
 for record in llvm.values():sdk.add(COMPILER_EXECUTABLES[record['argv'][0]])
 stage='independent-compiler-dependency-equality';dependency_probes=independent_dependency_probes(ADMITTED_COMPILERS)
 sdk_before=sdk_snapshot(sdk,'actual-sdk-before-test');need(sdk_snapshot(tools,'image-tools-after-build')==tools_before,'Image tool before/after build')
 doc(CONTROL/'actual-core-compilation.private.json',records);doc(CONTROL/'actual-consumer-links.private.json',links);doc(CONTROL/'actual-linked-new-archives.private.json',archive_records);doc(CONTROL/'portable-llvm-producers.private.json',llvm)
 artifacts=[archive,*[root/'core'/t for t in TARGETS],root/'network'/'nano-p2p']
 # Preserve original04 artifact metadata, including permissions, byte-exact.
 known={str(p):read_new(p) for p in artifacts};before=None;count=0;since=0;until=time.monotonic()+15
 while time.monotonic()<until:
  current={str(p):read_new(p) for p in artifacts};need(all(current[p]['sha256']==r['sha256'] and current[p]['bytes']==r['bytes']for p,r in known.items()),'Known producer artifact bytes')
  now=time.monotonic()
  if current==before:count+=1
  else:before=current;count=1;since=now
  if count>=3 and now-since>=2:break
  time.sleep(1)
 need(count>=3 and time.monotonic()-since>=2 and time.monotonic()<until,'New artifact first admission')
 doc(CONTROL/'new-artifacts-first-admission.private.json',{'files':current,'samples':count,'stableSpanMs':int((time.monotonic()-since)*1000),'oldAuthorityReadOrRefreshed':False})
 partial_outputs_fence(partial)
 stage='one-focused-api-test';log=guest(stage,['ctest','--test-dir','/outputs','--verbose','--output-on-failure','-R','^consensus_capella_validator_state_read$','--timeout','70'],90,2*1024*1024)
 found=[]
 for line in log.decode().splitlines():
  at=line.find('{')
  if at>=0:
   try:r=json.loads(line[at:])
   except ValueError:continue
   if isinstance(r,dict) and 'scenario'in r:found.append(r)
 need(len(found)==6 and all(r.get('passed')is True for r in found),'Six actual focused scenario rows');summary=[r for r in found if r['scenario']=='validator_read_summary'];need(len(summary)==1,'One API summary');summary=summary[0]
 need(all(summary.get(k)==v for k,v in {'groups':5,'statuses':9,'httpReads':21,'expectedRefusals':16,'fixtureBlsSignatures':2,'auditSignatures':0,'databaseAccessed':False,'runtimeStarted':False,'networkStarted':False,'productionQualified':False}.items()),'Focused API exact coverage');need(b'100% tests passed, 0 tests failed out of 1'in log,'Exactly one CTest ran')
 stage='new-output-and-image-final-fences'
 for p,r in current.items():read_new(p,r)
 need(sdk_snapshot(sdk,'actual-sdk-after-test')==sdk_before,'Newly admitted image dependency closure changed');need(sdk_snapshot(tools,'image-tools-final')==tools_before,'Image tools changed')
 source_fence();doc(CONTROL/'focused-api-result.json',summary)
 return {'coreObjects':143,'archiveMembers':143,'coreConsumers':7,'networkConsumers':1,'coreAndConsumerCompileDependencyClosures':151,'canonicalCoreAndConsumerCompilationsThisAttempt':0,'asmObjectCompilationsThisAttempt':1,'asmNewObjectEqualsOriginal':True,'originalAsmMdWasAbsent':True,'llvmRecipeClosures':2,'independentCompilerDependencyProbes':dependency_probes,'independentCompilerDependenciesEqualActualMd':True,'independentCOrCxxDependencyProbes':dependency_probes-1,'independentAsmDependencyProbes':1,'asmMdCreatedOnNewByteIdenticalObjectOnly':True,'additionalDependencyCompileClosures':len({obj for rec in archive_records.values()for obj in rec['objects']if obj not in llvm and obj not in {object_guest(r)for r in core_rows}}),'focusedApiTests':1,'apiGroups':5,'apiReads':21,'apiRefusals':16,'fixtureBlsSignatures':2,'apiAuditSignatures':0,'newArtifactsFirstAdmitted':9,'sdkImageReadOnlyThroughoutBuild':True,'sdkDependencyRowsFirstRecordedAfterBuild':True,'sdkDependenciesUnchangedAfterQualification':True,'imageToolsNineShaUnchanged':True}

RECIPE_PINS={'Dockerfile': {'sha256': '1b435d6b2a2467a628fad24922ce96b3ed28acdd3b4a4698f55d7392742dd08b', 'bytes': 2289}, '.dockerignore': {'sha256': 'c4e00df7b24797f27cf15e6ab4f7a08748f19838cf345b5405ad574ea2cf9953', 'bytes': 943}, 'dependencies.lock.json': {'sha256': '50d0d2c4a845294c09cdf017d389b5934f39a6b1ac57486d7b7816536e08f67b', 'bytes': 1896}, 'CMakeLists.txt': {'sha256': '275b30b7f759bdd15d265db1303365a1069f0614f9eac40edc4b1767c9290946', 'bytes': 174261}, 'network/CMakeLists.txt': {'sha256': '4ddb9d1cf55f96dcc419a2f39932235dccef3906a310aed57634bf39b77624a8', 'bytes': 18014}, 'cmake/NanoSecp256k1.cmake': {'sha256': '6a84c9325ffc0c01805103d9a831a06c26d0694142ddf06f681d7f73ededb176', 'bytes': 771}, 'third_party/ckzg/CMakeLists.txt': {'sha256': '1a8d36fb1979c8e27f7260a8f6dd8b20e2133b02020ffd7fa7afc0b051b05e3e', 'bytes': 2217}, 'third_party/mcl/CMakeLists.txt': {'sha256': '31870cbcc5e3c04e04624c20c817e4feea0b88920b056409d64d3d30f22429a5', 'bytes': 19141}}

def partial_outputs_snapshot():
 # Observations of this attempt's OWN mutable outputs, never OP authority.
 files={};directories={};symlinks={}
 for leaf in ('core','network'):
  base_root=root/leaf
  def strict_walk_error(error):raise error
  for base,dirs,names_ in os.walk(base_root,followlinks=False,onerror=strict_walk_error):
   if pathlib.Path(base)==root/'core' and 'Testing'in dirs:dirs.remove('Testing')
   p=pathlib.Path(base);s=p.lstat();need(stat.S_ISDIR(s.st_mode) and s.st_uid==1000 and s.st_gid==1000,'Current public build directory ownership');directories[str(p)]=[str(getattr(s,k))for k in F]
   for leaf_name in [*dirs,*names_]:
    p=pathlib.Path(base)/leaf_name;s=p.lstat()
    if stat.S_ISDIR(s.st_mode):continue
    need(s.st_uid==1000 and s.st_gid==1000,'Current partial output ownership')
    if stat.S_ISLNK(s.st_mode):
     resolved=p.resolve(strict=True);need(any(resolved.is_relative_to(root/x)for x in ('source','core','network')),'Current output alias cannot escape three NEW public roots');symlinks[str(p)]={'material':[str(getattr(s,k))for k in F],'target':os.readlink(p)}
    else:need(stat.S_ISREG(s.st_mode),'Only current regular build outputs');files[str(p)]=read_new(p)
    need(len(files)+len(directories)+len(symlinks)<=40000,'Partial output inventory bound')
 return {'format':'nano-observed-partial-public-build-v1','qualification':False,'operationalAuthority':False,'files':files,'directories':directories,'symlinks':symlinks}
def partial_outputs_fence(snapshot):
 for p,r in snapshot['files'].items():read_new(p,r)
 for p,row in snapshot['directories'].items():need([str(getattr(pathlib.Path(p).lstat(),k))for k in F]==row,'Observed partial directory changed before build')
 for p,r in snapshot['symlinks'].items():need([str(getattr(pathlib.Path(p).lstat(),k))for k in F]==r['material'] and os.readlink(p)==r['target'],'Observed partial alias changed before build')
def current_cache_checks():
 for leaf,home,cache_dir in [('core','/work','/outputs'),('network','/work/network','/network-build')]:
  p=root/leaf/'CMakeCache.txt';need(p.is_file(),'Only reuse the current03 canonical CMake cache');raw=p.read_bytes();need(0<len(raw)<=8*1024*1024,'Current generated cache bound');values={}
  for line in raw.decode().splitlines():
   if line.startswith(('#','//'))or'='not in line or':'not in line.split('=',1)[0]:continue
   key,value=line.split('=',1);values[key.split(':',1)[0]]=value
  need(values.get('CMAKE_HOME_DIRECTORY')==home and values.get('CMAKE_CACHEFILE_DIR')==cache_dir and values.get('CMAKE_BUILD_TYPE')=='Release' and values.get('CMAKE_GENERATOR')=='Unix Makefiles','Current03 cache refers only to the same canonical guest mounts')
  need(values.get('CMAKE_C_COMPILER')in ('clang','/usr/bin/clang','/usr/bin/clang-18','/usr/bin/clang-17','/usr/bin/clang-16','/usr/bin/clang-15','/usr/bin/clang-14') and values.get('CMAKE_CXX_COMPILER')in ('clang++','/usr/bin/clang++','/usr/bin/clang++-18','/usr/bin/clang++-17','/usr/bin/clang++-16','/usr/bin/clang++-15','/usr/bin/clang++-14'),'Current canonical Clang compiler selection')
  need(not values.get('CMAKE_TOOLCHAIN_FILE'),'No old host toolchain configuration')
def partial_core_objects():
 db=root/'core'/'compile_commands.json';need(db.is_file() and db.stat().st_size<=16*1024*1024,'Current compile database bound');rows=json.loads(db.read_bytes());need(isinstance(rows,list) and len(rows)<=1000,'Current compile database list');selected=[r for r in rows if '/CMakeFiles/nano_core.dir/'in object_guest(r)]
 expected={'/work/src/'+p.name for p in (root/'source'/'src').glob('*.cpp')if p.name!='main.cpp'};need(len(selected)==143 and len(expected)==143 and {r['file']for r in selected}==expected,'Current partial build still has the canonical143 translation units')
 complete={};other=0
 for r in selected:
  obj=mapped(object_guest(r));dep=pathlib.Path(str(obj)+'.d')
  if not obj.exists()or not dep.exists():continue
  pin=read_new(obj)
  if not valid_amd64_elf(obj,pin,True):other+=1;continue
  dep_pin=read_new(dep,limit=4*1024*1024);raw=dep.read_bytes();need(0<len(raw)<=4*1024*1024 and hashlib.sha256(raw).hexdigest()==dep_pin['sha256'],'Current compiler dependency bounded known bytes');need(r['file']in normalized_dependency_names(raw,r),'Current completed object has its exact compiler source dependency');read_new(dep,dep_pin)
  complete[object_guest(r)]={'object':pin,'dependencyFile':dep_pin}
 return {'canonicalCompileRows':143,'completeObjects':complete,'incompleteObjectHeaders':other,'qualified':False,'operationalAuthority':False}
# A single explicitly retained PUBLIC preparation, not an arbitrary resume API.
PREPARED=pathlib.Path('/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a')
REVISION='712b8357d05f09cc28df6e04920c9955e59b7b69'
def read_json_fd(path,maximum):
 path=pathlib.Path(path);need(path.resolve()==path,'Retained public descriptor canonical path')
 fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode) and a.st_nlink==1 and a.st_uid==1000 and a.st_gid==1000 and stat.S_IMODE(a.st_mode)==0o600 and 0<a.st_size<=maximum,'Retained public descriptor owner/type/bound')
  b=bytearray()
  while len(b)<a.st_size:
   x=os.read(fd,min(1048576,a.st_size-len(b)));need(x,'Retained descriptor short read');b.extend(x)
  need(not os.read(fd,1),'Retained descriptor grew');row=[str(getattr(a,k))for k in F]
  need(row==[str(getattr(os.fstat(fd),k))for k in F]==[str(getattr(path.lstat(),k))for k in F],'Retained descriptor FD/path changed')
  raw=bytes(b);return json.loads(raw),{'path':str(path),'material':row,'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
 finally:os.close(fd)
def original_git_blob(path,row,oid):
 # Saved full-nine/SHA remains the authority. Never substitute a current row.
 path=pathlib.Path(path);fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode) and a.st_nlink==1 and a.st_uid==1000 and a.st_gid==1000 and 0<=a.st_size<=256*1024*1024,'Original public source type/owner/bound')
  before=[str(getattr(a,k))for k in F];need(before==row['material'] and str(path)==row['path'],'Original saved source identity differs')
  sha=hashlib.sha256();blob=hashlib.sha1(b'blob '+str(a.st_size).encode()+b'\0');n=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   sha.update(b);blob.update(b);n+=len(b);need(n<=a.st_size,'Original public source grew')
  need(before==[str(getattr(os.fstat(fd),k))for k in F]==[str(getattr(path.lstat(),k))for k in F],'Original public source FD/path differs')
  need(n==a.st_size==row['bytes'] and sha.hexdigest()==row['sha256'] and blob.hexdigest()==oid,'Saved SHA and independent Git blob differ')
 finally:os.close(fd)
result={'format':'nano-new-public-canonical-amd-build-v1','passed':False,'stage':stage,'imageBuilds':0,'imagePulls':0,'hostPrivateBuildInputsRead':False,'oldMetadataReadOrRefreshed':False,'runtimeStarted':False,'productionQualified':False,'artifactOrSdkHashesExported':False,'retainedNewNamespace':False,'ownedContainerAbsent':False,'specificPublicPreparationReused':True,'current03PartialBuildResumed':False,'current04ClosureOnlyContinuation':True,'old03SdkMapsMounted':False,'priorNewPublicSourceAuthorityRead':False,'priorNewPublicSourceAuthorityRefreshed':False,'sourceOidsVerifiedAgain':False,'newNamespaceCreated':False,'retainedOriginalPublicPreparation':True,'sourceAdmissionDigestPreviouslyRetained':False}
checkpoint_pin=None;old_result_pin=None;partial=None;partial_objects=None;retained03_pin=None
try:
 parser=argparse.ArgumentParser();parser.add_argument('--revision',default=REVISION,choices=[REVISION]);parser.add_argument('--uuid',required=True);a=parser.parse_args();need(str(uuid.UUID(a.uuid))==a.uuid and a.uuid!='e37f349c-5b37-444a-901c-7b7a8747c63a','New canonical container UUID');nonce=a.uuid;name='nano-canonical-amd-'+nonce
 need(os.geteuid()==1000 and os.getegid()==1000,'Explicit Ubuntu owner')
 for p,mode in [(PREPARED,0o700),(PREPARED/'source',0o500),(PREPARED/'core',0o700),(PREPARED/'network',0o700),(PREPARED/'control',0o700),(PREPARED/'logs',0o700)]:
  need(p.resolve()==p,'Fixed public preparation path');st=p.lstat();need(stat.S_ISDIR(st.st_mode) and st.st_uid==1000 and st.st_gid==1000 and stat.S_IMODE(st.st_mode)==mode,'Original preparation directory owner/mode')
 CONTROL=PREPARED/'control06';LOGS=PREPARED/'logs06';QUALIFICATION=PREPARED/'qualification06'
 need(all(not os.path.lexists(p)for p in (CONTROL,LOGS,QUALIFICATION,PREPARED/'resume06-public-result.json')),'Attempt06 control/log/qualification/result namespaces must be absent')
 old,old_result_pin=read_json_fd(PREPARED/'resume04-public-result.json',65536)
 need(old.get('format')=='nano-new-public-canonical-amd-build-v1' and old.get('passed')is False and old.get('stage')=='canonical-closure' and old.get('errorClass')=='FileNotFoundError' and old.get('ownedContainerAbsent')is True and old.get('runtimeStarted')is False and old.get('hostPrivateBuildInputsRead')is False and old.get('sourceOidsVerifiedAgain')is True and old.get('originalArchiveNineShaUnchanged')is True,'Only the current04 completed canonical build with missing ASM dependency metadata may be qualified')
 checkpoint,checkpoint_pin=read_json_fd(PREPARED/'control'/'source-first-admission.private.json',64*1024*1024);result['priorNewPublicSourceAuthorityRead']=True
 need(set(checkpoint)=={'revision','archive','files','aliases','samples','stableSpanMs','oldAuthorityReadOrRefreshed'} and checkpoint['revision']==REVISION and checkpoint['oldAuthorityReadOrRefreshed']is False and checkpoint['samples']>=3 and checkpoint['stableSpanMs']>=2000 and checkpoint['aliases']=={},'Original source admission scope')
 need(isinstance(checkpoint['files'],dict) and 0<len(checkpoint['files'])<=60000,'Original source admission count')
 # Pin only this descriptor's CURRENT raw digest, explicitly not a retroactive
 # producer/offsite commitment. The contained original source rows are retained.
 root=PREPARED
 CONTROL.mkdir(mode=0o700);LOGS.mkdir(mode=0o700);QUALIFICATION.mkdir(mode=0o700);namespace_admitted=True
 parent_fd=os.open(root,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);os.fsync(parent_fd);os.close(parent_fd)
 space=os.statvfs(root);available=space.f_bavail*space.f_frsize;mutable_bytes_bound=min(6*1024*1024*1024,available-1024*1024*1024);need(mutable_bytes_bound>=1024*1024*1024,'Insufficient mutable build headroom; no pruning')
 stage='retained-public-source-fences';need(not exact_ids('initial-name-absence'),'New owned container exact name absent');initial_absent=True
 need(docker('image-identity',['image','inspect',IMAGE,'--format','{{.Id}} {{.Architecture}} {{.Os}}']).decode().strip()==IMAGE+' amd64 linux','Exact existing image')
 git=lambda *args:['git','--no-optional-locks','-C',str(REPO),*args]
 need(REPO.resolve()==REPO and run('git-commit',git('rev-parse',REVISION+'^{commit}')).decode().strip()==REVISION,'Independent exact public commit')
 tree=run('git-tree',git('ls-tree','-r','-z',REVISION),60,16*1024*1024);blobs={}
 for item in tree.split(b'\0'):
  if not item:continue
  header,p=item.split(b'\t',1);mode,kind,oid=header.decode('ascii').split();rel=p.decode();parts=pathlib.PurePosixPath(rel)
  need(mode in ('100644','100755') and kind=='blob' and not parts.is_absolute() and '..'not in parts.parts and str(parts)==rel and not any(c in rel for c in '\0\r\n'),'This exact public preparation has regular tracked blobs only')
  need(rel not in blobs,'Unique Git blob');blobs[rel]=(mode,oid)
 expected={str(root/'source'/p)for p in blobs};need(expected==set(checkpoint['files']),'Original source row namespace equals independent Git tree')
 found=set();dir_count=0
 for base,dirs,files in os.walk(root/'source',followlinks=False):
  dir_count+=1
  for leaf in dirs:need(not (pathlib.Path(base)/leaf).is_symlink(),'No source directory aliases in this preparation')
  for leaf in files:found.add(str(pathlib.Path(base)/leaf))
 need(found==expected,'Exact unchanged retained source namespace')
 total=0
 for rel,(mode,oid)in blobs.items():
  p=root/'source'/rel;r=checkpoint['files'][str(p)];need(set(r)=={'path','material','sha256','bytes'} and len(r['material'])==9 and all(isinstance(x,str)and x.isdigit()for x in r['material']) and re.fullmatch('[0-9a-f]{64}',r['sha256']),'Original row shape')
  need(stat.S_IMODE(int(r['material'][2]))==(0o500 if mode=='100755'else 0o400),'Original source permissions match producer mode');original_git_blob(p,r,oid);total+=r['bytes']
 archive=root/'tracked-source.tar';need(checkpoint['archive']['path']==str(archive),'Original archive path');read_new(archive,checkpoint['archive'])
 source_rows=checkpoint['files'];aliases=checkpoint['aliases'];fixed_namespace_bytes=checkpoint['archive']['bytes']+total;fixed_namespace_entries=len(blobs)+dir_count+1
 need(fixed_namespace_bytes<=4*1024*1024*1024,'Fixed retained public preparation byte bound')
 for rel,pin in RECIPE_PINS.items():
  b=(root/'source'/rel).read_bytes();need(len(b)==pin['bytes']and hashlib.sha256(b).hexdigest()==pin['sha256'],'Reviewed canonical recipe differs')
 read_new(root/'control'/'source-first-admission.private.json',checkpoint_pin);read_new(root/'resume04-public-result.json',old_result_pin);source_fence()
 retained03,retained03_pin=read_json_fd(root/'control'/'resume03-source-retained-on-site.private.json',4*1024*1024)
 need(retained03.get('sourceAdmissionDescriptorCurrentPin')==checkpoint_pin and retained03.get('originalArchive')==checkpoint['archive'] and retained03.get('originalSourceRowsRetainedWithoutRefresh')is True,'Current03 retained source descriptor must still match exactly')
 # Old03 maps/logs are retained and NOT mounted. Their sizes only contribute
 # to disk accounting; no SDK snapshot or build admission is imported.
 for old_tree in (root/'control',root/'logs',root/'control04',root/'logs04',root/'control05',root/'logs05',root/'qualification05'):
  for base,dirs,files in os.walk(old_tree,followlinks=False):
   fixed_namespace_entries+=len(dirs)+len(files)
   for leaf in files:fixed_namespace_bytes+=(pathlib.Path(base)/leaf).lstat().st_size
 current_cache_checks()
 testing=root/'core'/'Testing';need(not os.path.lexists(testing)or(testing.is_dir()and not testing.is_symlink()and not any(testing.iterdir())),'Only a new or empty declared CTest output directory may be used');testing_created=not testing.exists()
 if testing_created:testing.mkdir(mode=0o700)
 testing_stat=testing.lstat();need(testing_stat.st_uid==1000 and testing_stat.st_gid==1000 and stat.S_IMODE(testing_stat.st_mode)==0o700,'Declared CTest output directory ownership')
 partial=partial_outputs_snapshot();partial_objects=partial_core_objects();need(len(partial_objects['completeObjects'])==143 and partial_objects['incompleteObjectHeaders']==0,'Require all143 completed canonical04 objects before closure-only qualification')
 for target in TARGETS:need(valid_amd64_elf(root/'core'/target,read_new(root/'core'/target),False),'All seven prior04 Core consumers must exist and be valid AMD64 ELF')
 need(valid_amd64_elf(root/'network'/'nano-p2p',read_new(root/'network'/'nano-p2p'),False),'Prior04 network consumer must exist and be valid AMD64 ELF')
 partial_pin_path=CONTROL/'observed-complete04-outputs.private.json';doc(partial_pin_path,{'inventory':partial,'coreObjects':partial_objects,'qualified':False,'operationalAuthority':False});partial_pin=read_new(partial_pin_path)
 result.update(observedPartialFiles=len(partial['files']),observedPartialDirectories=len(partial['directories']),observedPartialAliases=len(partial['symlinks']),observedCompleteCoreObjects=len(partial_objects['completeObjects']),observedIncompleteCoreObjectHeaders=partial_objects['incompleteObjectHeaders'],partialOutputsOperationalAuthority=False,partialOutputsQualifiedBeforeResume=False,partialCompileCacheReused=False,closureOnlyPreviousOutputsReused=True,declaredCtestTestingDirectoryCreated=testing_created)
 doc(CONTROL/'resume06-source-retained-on-site.private.json',{'sourceAdmissionDescriptorCurrentPin':checkpoint_pin,'sourceAdmissionDigestPreviouslyRetained':False,'originalTerminalCurrentPin':old_result_pin,'originalSourceRowsRetainedWithoutRefresh':True,'sourceRowsVerified':len(source_rows),'independentGitBlobOidsVerified':len(blobs),'originalArchive':checkpoint['archive'],'fixedSourceAndArchiveBytes':fixed_namespace_bytes,'availableBytesObserved':available,'mutableBuildMaximumBytes':mutable_bytes_bound,'spareBytes':1024*1024*1024})
 result['sourceOidsVerifiedAgain']=True
 result.update(originalSourceRowsVerifiedAgainstSavedNineSha=len(source_rows),independentGitBlobOidsVerified=len(blobs),originalArchiveNineShaUnchanged=True,mutableBuildMaximumBytes=mutable_bytes_bound,spareBytes=1024*1024*1024)
 partial_outputs_fence(partial);read_new(partial_pin_path,partial_pin)
 counts=canonical_build();result.update(counts)
 final_objects=partial_core_objects();need(len(final_objects['completeObjects'])==143 and final_objects['incompleteObjectHeaders']==0,'Final canonical143 actual AMD64 objects with compiler dependency files')
 unchanged=0
 for path,row in partial_objects['completeObjects'].items():
  if final_objects['completeObjects'].get(path)==row:unchanged+=1
 need(unchanged==143,'All original04 Core object/MD full-nine and SHA records retained')
 need(partial_outputs_snapshot()==partial,'All original04 outputs retained exactly except the declared Testing subtree')
 result.update(observedPartialCoreObjectsRetainedExact=unchanged,observedPartialCoreObjectsChanged=0,actualCoreTranslationUnitsCompiledThisAttempt=0,finalCoreObjectsAfterResume=143,partialOutputsAllowedToChangeDuringBuild=False,original04OutputsNineShaUnchanged=True,onlyDeclaredTestingSubtreeMayChange=True)
 result.update(passed=True,stage='complete',sourceRevision=REVISION,trackedPublicFiles=len(blobs),trackedPublicBytes=total,newOutputAuthorityRetainedOnVps=True,sourceNineAndShaUnchanged=True)
 metrics=guest('resource-observation',['/bin/sh','-ec',"if test -r /sys/fs/cgroup/memory.peak; then printf 'memoryPeakBytes '; cat /sys/fs/cgroup/memory.peak; fi; if test -r /sys/fs/cgroup/cpu.stat; then cat /sys/fs/cgroup/cpu.stat; fi"],10,65536).decode().splitlines()
 for line in metrics:
  pair=line.split()
  if len(pair)==2 and pair[1].isdigit()and pair[0]in ('memoryPeakBytes','usage_usec','user_usec','system_usec'):result['container_'+pair[0]]=int(pair[1])
 result['containerResourceObservationAvailable']='container_memoryPeakBytes'in result
 state=json.loads(docker('final-container-state',['inspect',created_id,'--format','{{json .State}}'],10,65536));need(state['Running']is True and state.get('OOMKilled')is False,'Owned idle build container')
 result['configuredBuildCpuLimit']=2;result['configuredBuildMemoryBytes']=3*1024*1024*1024
except BaseException as error:
 result['passed']=False;result['stage']=stage;result['errorClass']=type(error).__name__;result['errorSourceFrames']=source_error_frames(error)
finally:
 if namespace_admitted:
  try:cleanup_ok=cleanup()
  except BaseException:cleanup_ok=False
  try:
   if source_rows:source_fence();read_new(root/'tracked-source.tar',checkpoint['archive'])
   if partial is not None:need(partial_outputs_snapshot()==partial,'Original04 outputs full-nine and SHA unchanged even on refusal, except declared Testing');result['original04OutputsNineShaUnchanged']=True
   if checkpoint_pin:read_new(root/'control'/'source-first-admission.private.json',checkpoint_pin)
   if old_result_pin:read_new(root/'resume04-public-result.json',old_result_pin)
   if retained03_pin:read_new(root/'control'/'resume03-source-retained-on-site.private.json',retained03_pin)
  except BaseException as error:result['passed']=False;result['stage']='final-original-source-fence';result['errorClass']=type(error).__name__;result['errorSourceFrames']=source_error_frames(error)
 result['ownedContainerAbsent']=cleanup_ok;result['passed']=result['passed']and cleanup_ok;result['retainedNewNamespace']=namespace_admitted
 result.update(elapsedMs=int((time.monotonic()-BEGIN)*1000),hostPeakRssKiB=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,hostChildUserMs=int(resource.getrusage(resource.RUSAGE_CHILDREN).ru_utime*1000),hostChildSystemMs=int(resource.getrusage(resource.RUSAGE_CHILDREN).ru_stime*1000),hostLogBytes=log_bytes,cleanupIdentityGuarded=True)
 if namespace_admitted:doc(root/'resume06-public-result.json',result)
 print(json.dumps(result,sort_keys=True),flush=True)
raise SystemExit(0 if result['passed'] else 1)
