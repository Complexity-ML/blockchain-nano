#!/usr/bin/env python3
"""ROOT-only Native legacy consumer relink and one support reproduction. No operation on import.

Uses the frozen c586 helper definitions, never their main. No configure, Core compilation, archive mutation, long-stream replay or runtime.
Only the small Apple sync-fault support TU/dylib is reproduced byte-identically.
Private authorities and executables are consumed only by the authorized ROOT.
"""
import argparse, ast, hashlib, json, os, pathlib, re, selectors, shlex
import signal, stat, subprocess, sys, time

THIN_SHA = 'c586c596fe2b9cf44600f2559d24b5fa7dfa007c4e480750902aba9e4607d28d'
THIN_BYTES = 31909
CORE_SHA = '7264c21296c60e12c73b9927ed9f87d7b0366f91caa8acadc4f1eb56a8e7bac4'
CORE_BYTES = 29259384
NATIVE_TERMINAL_SHA = 'a753f2ea126df3967e94206fc94a932152af06571ae35727ef54a52e2c847af6'
NATIVE_TERMINAL_BYTES = 3846
NATIVE_INPUT_SHA = 'e5b454d02b291e259bfd57486e2269f8eb2e1e3a18e5dfaa401667ab23af64e8'
NATIVE_INPUT_BYTES = 5672
CLOSURE_SHA = 'd43f2486981bbfc5fa82b51071a9ae9e9a09f6b8b4719c8fd49aef8e64c4c7b3'
CLOSURE_BYTES = 2162
FIRST_SHA = '4d1d0f4ef6d16fff6574d07555a4dd2a6b4b6965b843394b3fd5b2d9d656f206'
FIRST_BYTES = 1903
CFG_SHA = '3ec9e61e1820d9b838554bb85b95573467de00cd1bbc5712f2cf4a8f852a5e40'
CFG_BYTES = 1934711
MATERIAL_SHA = '25c5441c78a55f88f4d4c4255d498f7ceb174ff075833164f8be7e1e2679a633'
MATERIAL_BYTES = 6869424
SDK_SHA = '963765ee106da27929144fdf095f8523a840610ce16cc726498e27f33eff7a10'
SDK_BYTES = 1514893
F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
SOURCE_REVISION = '7c004a00c25a5d7a7e48e115e2eeb0c0368d14fa'
CORE_SOURCE_BASE = 'def5cda61919bbb40e91744a18661a2ad7a76eba'
ROLES = {
 'nano-capella-transition-tests': ('tests/test_capella_transition.cpp','fbf2d126f7977b51917d0bd57ded93cf4542618411f7d081566dabf90daa6431',546903,'4b5150320c6c42e168a1518e8a5a26e29de0c5fda5d28f18fc9568611e43a6c7',20947024,'46782dcd17d05ca83d79f77ab970ae877f1fdc1056cca6cb1ce696c0826820dd',857),
 'nano-capella-isolated-bootstrap-import': ('tests/capella/isolated_bootstrap_import.cpp','c91e6224b3cea9747c829f1267fc9aa8fd1f8fad423380e194b5a2b559048bef',22872,'e7e93b79361bf9978e194554af6a6f6c92821834459d67810a34ac6c64637016',4816760,'8d47167b15e84e937b181f00aafb7d460bd8a6a37dacccb3aaed30881b6c3ce3',696),
}
SUPPORT_TARGET = 'nano-test-sync-fault'
SUPPORT_SOURCE = 'tests/support/sync_fault_macos.cpp'
SUPPORT_PINS = {
 'source':('7697487f84c579460f9241f8973b7eecdf70525d3864dd733e1bc93c5d7ebec0',2649),
 'object':('ac8a48a854db31eebdb9828999cf15d0d90cedfbc4f10a26f4570d074d0c2364',3208),
 'md':('0f930947c7477b0f750b357d8fbf92c162274aa406963c10b30cf38b6e1bff91',38115),
 'link':('03fd4657e455495fa9a6e59b38a2e85c8ed615d1e4ea5799184e44139afd7e1e',308),
 'dylib':('8d8ddc5412a8ad0223ddbb859b43a4c9b5b071b0868a91cba67f05bf9a794a67',34336),
}
H, OUT, PHASE = None, None, 'admission'
BEGAN, DEADLINE, EVENTS = time.monotonic(), None, []


def need(value, why):
 if not value: raise ValueError(why)


def exact(value, names, why):
 need(isinstance(value,dict) and set(value)==set(names),why)


def physical(value):
 need(isinstance(value,str) and 0<len(value)<4096 and not re.search(r'[\x00\r\n\\]',value),'locator shape')
 p=pathlib.Path(value)
 need(p.is_absolute() and str(p)==os.path.normpath(value) and p!=pathlib.Path('/') and p.resolve(strict=True)==p,'physical absolute locator')
 return p


def material(path):
 p=physical(str(path)); fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd); need(stat.S_ISREG(a.st_mode) and a.st_nlink>0 and 0<=a.st_size<=268435456,'bounded regular file')
  raw=bytearray()
  while True:
   part=os.read(fd,1048576)
   if not part: break
   raw.extend(part); need(len(raw)<=a.st_size,'read size bound')
  row={k:getattr(a,k) for k in F}
  need(len(raw)==a.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(p.lstat(),k) for k in F},'strict FD/path full nine fields')
  return dict(row,sha256=hashlib.sha256(raw).hexdigest()),bytes(raw)
 finally: os.close(fd)


def pin(ref):
 exact(ref,('path','sha256','bytes'),'Artifact three fields')
 need(re.fullmatch('[0-9a-f]{64}',ref['sha256']) and type(ref['bytes']) is int and 0<=ref['bytes']<=268435456,'known Artifact bytes')
 return physical(ref['path'])


def nine(value):
 exact(value,(*F,'sha256'),'recorded full-nine plus SHA')
 need(all(type(value[k]) is int for k in F) and re.fullmatch('[0-9a-f]{64}',value['sha256']),'recorded row values')
 return value


def stable_batch(expected):
 """NEW known bytes only, before first admission; old rows never reset."""
 pending={str(physical(p)):v for p,v in expected.items() if str(physical(p)) not in H['HELD']}
 for p,v in expected.items():
  if str(physical(p)) in H['HELD']:
   seen,_=material(p); need(seen==H['HELD'][str(physical(p))] and seen['sha256']==v['sha256'] and seen['st_size']==v['bytes'],'already-held row remains exact')
 if not pending:return
 deadline=time.monotonic()+15; previous=None; count=0; first=None
 while True:
  observed={}
  for p,v in pending.items():
   row,_=material(p)
   need(row['st_uid']==501 and row['st_nlink']==1 and row['sha256']==v['sha256'] and row['st_size']==v['bytes'],'known NEW bytes/owner before first admission')
   observed[p]=row
  if observed==previous: count+=1
  else: previous=observed; count=1; first=time.monotonic()
  if count>=3 and time.monotonic()-first>=2: break
  need(time.monotonic()<deadline,'NEW first-admission quiescence bound');time.sleep(.75)
 H['HELD'].update(observed)


def known(ref, fixed=None):
 p=pin(ref)
 if fixed:need((ref['sha256'],ref['bytes'])==fixed,'externally fixed descriptor')
 stable_batch({str(p):ref});return material(p)[1]


def recorded(path, row):
 p=physical(str(path));nine(row);seen,raw=material(p)
 need(seen==row,'original recorded authority changed')
 need(str(p) not in H['HELD'] or H['HELD'][str(p)]==row,'held original authority differs')
 H['HELD'][str(p)]=dict(row);return raw


def fence():
 for p,row in list(H['HELD'].items()):recorded(p,row)
 for p,row in H['ALIASES'].items():
  a=pathlib.Path(p);s=a.lstat()
  need({k:getattr(s,k) for k in F}==row['material'] and str(a.resolve(strict=True))==row['resolved'] and (os.readlink(a) if stat.S_ISLNK(s.st_mode) else None)==row['link'],'retained alias guard changed')
 for p,v in list(H['NEW'].items()):
  now,_=material(p);need(now['sha256']==v['sha256'] and now['st_size']==v['bytes'],'immutable new output changed')


def publish(name,value):H['publish'](OUT/name,value)


def run(name,argv,cwd,env,seconds=180,allowed=(0,),stdin_bytes=None):
 """Bounded pipe drain, stream caps, TERM/KILL process group and bounded joins."""
 global PHASE
 PHASE=name;fence();limit=min(seconds,DEADLINE-time.monotonic());need(0<limit<=300,'command wall limit')
 publish(name+'-argv.private.json',{'argv':argv,'cwd':str(cwd),'maximumSeconds':limit})
 logpaths=[OUT/(name+'-stdout.private.log'),OUT/(name+'-stderr.private.log')]
 fds=[];child=None;input_fd=None;sel=selectors.DefaultSelector();sizes=[0,0];start=time.monotonic();failed=False
 try:
  if stdin_bytes is not None:
   need(isinstance(stdin_bytes,bytes) and len(stdin_bytes)<=1048576,'bounded input batch')
   input_path=OUT/(name+'-stdin.private.txt');H['write_new'](input_path,stdin_bytes)
   input_fd=os.open(input_path,os.O_RDONLY|os.O_NOFOLLOW)
  for p in logpaths:fds.append(os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600))
  child=subprocess.Popen(argv,cwd=cwd,env=env,stdin=subprocess.DEVNULL if input_fd is None else input_fd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
  for i,s in enumerate((child.stdout,child.stderr)):
   os.set_blocking(s.fileno(),False);sel.register(s,selectors.EVENT_READ,i)
  while sel.get_map():
   need(time.monotonic()-start<limit,'command timeout')
   for key,_ in sel.select(.1):
    part=os.read(key.fileobj.fileno(),65536)
    if not part:sel.unregister(key.fileobj);key.fileobj.close();continue
    i=key.data;need(sizes[i]+len(part)<=33554432,'stream bound');sizes[i]+=len(part)
    view=memoryview(part)
    while view:n=os.write(fds[i],view);need(n>0,'log write');view=view[n:]
  code=child.wait(timeout=max(.01,min(5,limit-(time.monotonic()-start))))
  need(code in allowed,'command exit refused')
 except BaseException:
  failed=True
  if child is not None:
   try:os.killpg(child.pid,signal.SIGTERM)
   except ProcessLookupError:pass
   try:child.wait(timeout=5)
   except subprocess.TimeoutExpired:
    try:os.killpg(child.pid,signal.SIGKILL)
    except ProcessLookupError:pass
    child.wait(timeout=5)
  raise
 finally:
  if child is not None:
   for s in (child.stdout,child.stderr):
    if s is not None:s.close()
  sel.close()
  if input_fd is not None:os.close(input_fd)
  for fd in fds:
   try:os.fsync(fd)
   finally:os.close(fd)
  event={'phase':name,'exitCode':None if child is None else child.returncode,'passed':not failed,'elapsedMs':int((time.monotonic()-start)*1000),'stdoutBytes':sizes[0],'stderrBytes':sizes[1]}
  EVENTS.append(event)
  if len(fds)==2:
   for p in logpaths:H['output'](p)
  publish(name+'-exit.json',event);fence()
 need(time.monotonic()<DEADLINE,'whole-controller deadline')
 return material(logpaths[0])[1],material(logpaths[1])[1]


def load_helpers(ref):
 global H
 p=pin(ref);previous=None;count=0;first=None;deadline=time.monotonic()+15
 while True:
  row,raw=material(p)
  need(row['st_uid']==501 and row['st_nlink']==1 and (row['sha256'],len(raw))==(THIN_SHA,THIN_BYTES),'known helper bytes before evaluating definitions')
  if row==previous:count+=1
  else:previous=row;count=1;first=time.monotonic()
  if count>=3 and time.monotonic()-first>=2:break
  need(time.monotonic()<deadline,'helper first-admission quiescence');time.sleep(.75)
 tree=ast.parse(raw.decode());nodes=[]
 for node in tree.body:
  if isinstance(node,ast.FunctionDef) and node.name=='main':break
  need(not isinstance(node,(ast.If,ast.For,ast.While,ast.With,ast.Try)),'definitions-only helper prefix')
  nodes.append(node)
 H={'__name__':'native_two_consumers_frozen_helpers','__file__':str(pin(ref))}
 exec(compile(ast.Module(body=nodes,type_ignores=[]),str(pin(ref)),'exec'),H)
 H['material']=material
 H['HELD'][str(p)]=row


def main(a):
 global OUT, DEADLINE
 DEADLINE=BEGAN+1100
 need(sys.platform=='darwin' and os.getuid()==501,'authorized Native owner only')
 load_helpers({'path':a.thin_controller,'sha256':THIN_SHA,'bytes':THIN_BYTES})
 c=json.loads(known({'path':a.inputs,'sha256':a.sha256,'bytes':a.bytes}))
 exact(c,('format','controller','nativeInput','firstSoftwareAdmission','nativeTerminal','currentSourceRoot','sourceRepository','gitTool','outputRoot'),'exact input9')
 need(c['format']=='nano-native-legacy-relink-input-v1' and pin(c['controller'])==physical(__file__),'exact controller input')
 known(c['controller'])
 closure_ref={'path':a.core_closure_result,'sha256':a.core_closure_sha256,'bytes':a.core_closure_bytes}
 closure=json.loads(known(closure_ref,(CLOSURE_SHA,CLOSURE_BYTES)))
 need(closure['format']=='nano-native-selected-closure-read-only-public-v1' and closure['passed'] is True and closure['effectiveSourceRevision']==CORE_SOURCE_BASE and closure['coreMembers']==143 and closure['inheritedCoreMembersVerified']==142 and closure['newApiMemberFromRetainedProducer']==1 and closure['newPreprocessorScans']==5 and closure['historicalUnknownAliasBindingRecovered'] is False and closure['oldAuthoritiesRefreshed'] is False and closure['productionQualified'] is False,'actual producer-known selected Core closure')
 private_ref=dict(closure['privateReceipt'],path=str(pin(closure_ref).parent/'closure.private.json'))
 need((private_ref['sha256'],private_ref['bytes'])==('2e01389cb6a6800b1fbf90ad608ce088884c49518c18cb852f9895f3b322b481',19933541),'producer-committed Core private receipt')
 cp=json.loads(known(private_ref));old=cp['heldMaterials'];aliases=cp['prospectiveAliasGuards']
 need(cp['oldAuthoritiesRefreshed'] is False and cp['newStorageReproductionPublicSha256']==closure['newStorageReproductionPublicSha256'] and isinstance(old,dict) and 0<len(old)<=65536 and isinstance(aliases,dict) and 0<len(aliases)<=8192,'retained closure provenance')
 for p,row in old.items():recorded(p,row)
 for p,row in aliases.items():
  exact(row,('resolved','material','link'),'retained alias descriptor');exact(row['material'],F,'alias full nine');need(pathlib.Path(p).is_absolute(),'absolute alias')
  H['ALIASES'][p]=row
 fence()
 ni=json.loads(known(c['nativeInput'],(NATIVE_INPUT_SHA,NATIVE_INPUT_BYTES)))
 need(ni['format']=='nano-cross-arch-validator-native-build-input-v1' and ni['sourceBaseRevision']==closure['originalApiProducerBase']=='ae039e1c6fa4dbc6b1ed705a6f27a96956ba8050' and (ni['controller']['sha256'],ni['controller']['bytes'])==(THIN_SHA,THIN_BYTES),'original Native producer input')
 cfg=json.loads(known(ni['oldConfig'],(CFG_SHA,CFG_BYTES)))
 projection=json.loads(known(ni['oldAdmission']))
 original=json.loads(known(projection['provenance']['originalInputs'],(MATERIAL_SHA,MATERIAL_BYTES)))
 sdk=json.loads(known(ni['nativeSdkAuthority'],(SDK_SHA,SDK_BYTES)))
 need(projection['files']==original and projection['provenance']['oldConfig']==ni['oldConfig'] and projection['provenance']['nativeSdkAuthority']==ni['nativeSdkAuthority'] and projection['aliases']==sdk['sdkAliases'],'original25c/SDK/projection join')
 for p,row in original.items():
  if p in old:need(row==old[p],'selected original regular row unchanged')
 for p,row in sdk['sdkFiles'].items():need(p in old and old[p]==row,'all original SDK regular rows retained')
 for p,target in sdk['sdkAliases'].items():need(p in aliases and aliases[p]['resolved']==target,'retained exact SDK target guards')
 first=json.loads(known(c['firstSoftwareAdmission'],(FIRST_SHA,FIRST_BYTES)))
 need(first['format']=='nano-new-native-software-first-root-admission-v1' and first['stableSamples']==3 and first['stableSpanMs']>=2000 and first['oldAuthoritiesRefreshed'] is False,'retained first software admission')
 terminal_raw=recorded(pin(c['nativeTerminal']),first['terminalMaterial'])
 need((hashlib.sha256(terminal_raw).hexdigest(),len(terminal_raw))==(NATIVE_TERMINAL_SHA,NATIVE_TERMINAL_BYTES),'original actual terminal')
 terminal=json.loads(terminal_raw)
 need(terminal['passed'] is True and terminal['newCoreArchive']=={'sha256':CORE_SHA,'bytes':CORE_BYTES} and physical(ni['controlRoot'])==pin(c['nativeTerminal']).parent,'same original Core producer')
 core=pin(c['nativeTerminal']).parent/'libnano_core.a';core_raw=recorded(core,first['newSoftwareMaterials']['libnano_core.a'])
 need((hashlib.sha256(core_raw).hexdigest(),len(core_raw))==(CORE_SHA,CORE_BYTES) and len(H['archive_members'](core_raw))==143,'immutable Core7264')
 b,s,current,repo=(physical(str(p)) for p in (cfg['buildRoot'],cfg['sourceRoot'],c['currentSourceRoot'],c['sourceRepository']))
 need(b==physical(ni['buildRoot']) and s==physical(ni['sourceRoot']) and current!=s and not current.is_relative_to(s) and not s.is_relative_to(current),'separate old/current roots')
 out=physical(c['outputRoot']);outstat=out.lstat()
 need(stat.S_ISDIR(outstat.st_mode) and outstat.st_uid==501 and stat.S_IMODE(outstat.st_mode)==0o700 and not list(out.iterdir()),'explicit NEW empty owned0700 output root')
 need(all(not out.is_relative_to(p) and not p.is_relative_to(out) for p in (b,s,current,repo)),'outputs disjoint from inputs')
 OUT=out;H['CONTROL']=out;H['DEADLINE']=DEADLINE
 # Preserve configured Git policies. Only repository-locator overrides and unsafe
 # compiler/runtime injection are removed; no hooks/config/signing override.
 excluded={'LD_PRELOAD','LD_LIBRARY_PATH','CPATH','CPLUS_INCLUDE_PATH','C_INCLUDE_PATH','LIBRARY_PATH','CXXFLAGS','CFLAGS','CPPFLAGS','LDFLAGS','GIT_DIR','GIT_WORK_TREE','GIT_INDEX_FILE','GIT_OBJECT_DIRECTORY','GIT_ALTERNATE_OBJECT_DIRECTORIES'}
 env={k:v for k,v in os.environ.items() if not k.startswith(('NANO_','DYLD_','MALLOC_')) and k not in excluded}
 env.update(GIT_OPTIONAL_LOCKS='0',GIT_NO_REPLACE_OBJECTS='1',GIT_NO_LAZY_FETCH='1',GIT_TERMINAL_PROMPT='0')
 exact(c['gitTool'],('path','sha256','bytes','material'),'OS Git explicit authority')
 git_ref={k:c['gitTool'][k] for k in ('path','sha256','bytes')};git=pin(git_ref)
 need(str(git)=='/usr/bin/git','fixed readonly OS Git')
 git_raw=recorded(git,c['gitTool']['material'])
 need(c['gitTool']['material']['st_uid']==0 and os.access(git,os.X_OK) and (hashlib.sha256(git_raw).hexdigest(),len(git_raw))==(git_ref['sha256'],git_ref['bytes']),'Root OS utility exact full9/bytes')
 git_cache={};git_batches=0
 def relative_blob(relative):
  need(isinstance(relative,str) and relative and not relative.startswith('/') and all(x not in ('','.','..') for x in relative.split('/')) and not re.search(r'[\x00\r\n\\:]',relative),'public relative blob')
 def ensure_blobs(relatives):
  nonlocal git_batches
  for relative in relatives:relative_blob(relative)
  todo=sorted(set(relatives)-set(git_cache))
  if not todo:return
  need(len(todo)<=2048,'bounded exact Git source batch')
  raw,err=run('git-source-'+str(git_batches),[str(git),'-C',str(repo),'cat-file','--batch'],repo,env,60,stdin_bytes=''.join(SOURCE_REVISION+':'+x+'\n' for x in todo).encode());git_batches+=1
  need(not err,'Git source batch stderr');at=0
  for relative in todo:
   end=raw.find(b'\n',at);need(end>=at and end-at<=128,'bounded Git header');match=re.fullmatch('([0-9a-f]{40}) blob ([0-9]+)',raw[at:end].decode('ascii'));need(match is not None,'exact Git blob response')
   size=int(match[2]);at=end+1;need(0<=size<=33554432 and at+size<len(raw) and raw[at+size:at+size+1]==b'\n','Git blob body bound');body=raw[at:at+size];at+=size+1
   need(hashlib.sha1(b'blob '+str(size).encode()+b'\0'+body).hexdigest()==match[1],'independent Git blob OID')
   git_cache[relative]={'sha256':hashlib.sha256(body).hexdigest(),'bytes':size,'gitBlobOid':match[1]}
  need(at==len(raw),'no extra Git batch bytes')
 def prior(path):
  p=physical(str(path));need(str(p) in original,'reached old regular file absent from original25c');return recorded(p,original[str(p)])
 def sdk_path(path):
  p=pathlib.Path(path);resolved=str(p.resolve(strict=True));need(resolved in sdk['sdkFiles'],'unrecorded SDK input')
  if str(p)!=resolved:need(str(p) in aliases and aliases[str(p)]['resolved']==resolved,'SDK alias guard retained')
  recorded(resolved,sdk['sdkFiles'][resolved]);return str(p)
 def build_path(value):
  p=pathlib.Path(value);return pathlib.Path(os.path.normpath(str(p if p.is_absolute() else b/p)))
 def digest(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
 def macho(raw,kind):
  need(len(raw)>=32 and raw[:4]==b'\xcf\xfa\xed\xfe' and int.from_bytes(raw[4:8],'little')==0x0100000c and int.from_bytes(raw[12:16],'little')==kind,'Native arm64 Mach-O64 exact kind')
 compiler=sdk_path(ni['tools']['compiler']);sdk_path(ni['tools']['linker'])
 project=cp['projectSourceBytes'];need(isinstance(project,dict) and len(project)==363,'selected Core exact363 project byte pins')
 ensure_blobs([*project,'CMakeLists.txt',SUPPORT_SOURCE,*[x[0] for x in ROLES.values()]])
 for rel,row in project.items():
  need({k:git_cache[rel][k] for k in ('sha256','bytes')}==row,'Core selected source equals exact current Git7c blob')
 stable_batch({str(current/rel):row for rel,row in project.items()})
 need((git_cache['CMakeLists.txt']['sha256'],git_cache['CMakeLists.txt']['bytes'])==('275b30b7f759bdd15d265db1303365a1069f0614f9eac40edc4b1767c9290946',174261),'reviewed canonical CMake unchanged')
 recipes=json.loads(prior(b/'compile_commands.json'));need(isinstance(recipes,list) and len(recipes)==400,'original compileDB exact400 rows')
 specs={role:{'source':x[0],'sourcePin':(x[1],x[2]),'programPin':(x[3],x[4]),'linkPin':(x[5],x[6])} for role,x in ROLES.items()}
 specs[SUPPORT_TARGET]={'source':SUPPORT_SOURCE,'sourcePin':SUPPORT_PINS['source'],'programPin':SUPPORT_PINS['dylib'],'linkPin':SUPPORT_PINS['link']}
 commands={};links={};objects={};old_mds={};old_programs={};orders={};old_core=None;old_support=None
 for role,spec in specs.items():
  rel=spec['source'];rows=[x for x in recipes if x['file']==str(s/rel)]
  need(len(rows)==1 and rows[0]['directory']==str(b),'unique exact canonical compile row')
  argv=rows[0].get('arguments') or shlex.split(rows[0]['command']);need(isinstance(argv,list) and argv[0]==compiler and argv.count('-o')==argv.count('-c')==argv.count(str(s/rel))==1 and not any(x in argv for x in ('-M','-MM','-MD','-MMD','-MF','-MT','-MQ','-MP','-MJ','-save-temps','-ftime-trace','-serialize-diagnostics')),'canonical compiler args without output side effects')
  obj=build_path(argv[argv.index('-o')+1]);need(obj.is_relative_to(b) and obj.name.endswith('.o'),'canonical original object under build')
  obj_raw=prior(obj);macho(obj_raw,1);md=obj.with_name(obj.name+'.d');md_raw=prior(md)
  need(digest(prior(s/rel))=={'sha256':spec['sourcePin'][0],'bytes':spec['sourcePin'][1]} and {k:git_cache[rel][k] for k in ('sha256','bytes')}==digest(prior(s/rel)),'old exact TU is actual current Git blob')
  stable_batch({str(current/rel):git_cache[rel]})
  link_raw=prior(b/'CMakeFiles'/(role+'.dir')/'link.txt');need(digest(link_raw)=={'sha256':spec['linkPin'][0],'bytes':spec['linkPin'][1]},'known canonical link recipe bytes')
  link=shlex.split(link_raw.decode());need(link[0]==compiler and link.count('-o')==1,'canonical compiler/link output')
  output_arg=link[link.index('-o')+1];program=build_path(output_arg);program_raw=prior(program)
  need(digest(program_raw)=={'sha256':spec['programPin'][0],'bytes':spec['programPin'][1]},'known original software bytes')
  macho(program_raw,6 if role==SUPPORT_TARGET else 2)
  object_args=[x for x in link[1:] if x.endswith('.o') and build_path(x)==obj]
  need(len(object_args)==1,'exact single canonical own object link argument')
  macro=[x for x in argv if x.startswith('-DNANO_TEST_SOURCE=')];sync=[x for x in argv if x.startswith('-DNANO_SYNC_FAULT')]
  install_name_index=None
  if role=='nano-capella-transition-tests':need(len(macro)==1 and macro[0].split('=',1)[1].strip('"')==str(s) and sync==['-DNANO_SYNC_FAULT=1'] and len(link)==27,'exporter exact fixture/sync macros and canonical27 args')
  elif role==SUPPORT_TARGET:
   need(not macro and not sync and len(link)==14 and '-dynamiclib' in link and link.count('-install_name')==1,'support canonical14 shared-library args')
   install_name_index=link.index('-install_name')+1
   need(install_name_index<len(link) and link[install_name_index]=='@rpath/libnano-test-sync-fault.dylib','canonical Mach-O install identity is not an input file')
  else:need(not macro and not sync and len(link)==25,'importer no fixture/sync macros and canonical25 args')
  archive_count=asm_count=0;order=[]
  for link_index,value in enumerate(link[1:],1):
   if link_index==install_name_index:continue
   if value==object_args[0]:order.append({'kind':'OWN_OBJECT','argument':value});continue
   if value==output_arg:continue
   if value.endswith(('.a','.o','.dylib','.tbd')):
    p=build_path(value)
    if p.is_relative_to(b):raw=prior(p)
    else:raw=recorded(p.resolve(strict=True),sdk['sdkFiles'][str(p.resolve(strict=True))]);sdk_path(str(p))
    if p==b/'libnano_core.a':
     need(digest(raw)=={'sha256':'25192162e6f82adcd8921470ce28ab1565aafab6edc361eb11c93d559bdd4908','bytes':29236208},'old Core2519 retained separately');old_core=value;kind='REPLACE_WITH_CORE7264'
    else:kind='RETAINED_LINK_INPUT'
    archive_count+=value.endswith('.a');asm_count+=value.endswith('.o');order.append({'kind':kind,'argument':value,'pin':digest(raw)})
  if role!=SUPPORT_TARGET:need(archive_count==5 and asm_count==2 and sum(x['kind']=='REPLACE_WITH_CORE7264' for x in order)==1,'five canonical archives/two assembly objects/exact Core substitution')
  if role==SUPPORT_TARGET:
   need(digest(obj_raw)=={'sha256':SUPPORT_PINS['object'][0],'bytes':SUPPORT_PINS['object'][1]} and digest(md_raw)=={'sha256':SUPPORT_PINS['md'][0],'bytes':SUPPORT_PINS['md'][1]},'known support object and historical actual MD')
   old_support=program
  commands[role]=list(argv);links[role]=link;objects[role]=(obj,object_args[0]);old_mds[role]=(md,H['dependency_set'](md_raw,b));old_programs[role]=program;orders[role]=order
 need(old_support is not None and old_core is not None,'both legacy/support recipes identified')
 need(any(x.get('argument') and build_path(x['argument'])==old_support for x in orders['nano-capella-transition-tests']) and not any(x.get('argument') and build_path(x['argument'])==old_support for x in orders['nano-capella-isolated-bootstrap-import']),'exporter original sync-fault binding retained/importer absent')
 dependencies={};reached=set()
 for i,(role,argv) in enumerate(commands.items()):
  command=list(argv);j=command.index('-o');del command[j:j+2];command.remove('-c')
  raw,err=run('M-'+str(i),command+['-M','-MT','legacy-proof-'+str(i)],b,env,120)
  need(not err,'M dependency stderr empty');found=H['dependency_set'](raw,b)
  need(str(s/specs[role]['source']) in found and found==old_mds[role][1],'real independent M equals actual original MD')
  dependencies[role]=found
  for value in found:
   p=pathlib.Path(value)
   if p.is_relative_to(s):reached.add(str(p.relative_to(s)))
 ensure_blobs(reached)
 current_reached={}
 for role,found in dependencies.items():
  for value in sorted(found):
   p=pathlib.Path(value)
   if p.is_relative_to(s):
    rel=str(p.relative_to(s));raw=prior(p);need(digest(raw)=={k:git_cache[rel][k] for k in ('sha256','bytes')},'all reached original project bytes match effective Git7c');current_reached[str(current/rel)]=git_cache[rel]
   elif p.is_relative_to(b):prior(p)
   else:sdk_path(value)
  fence()
 stable_batch(current_reached);fence()
 publish('admitted.private.json',{'format':'nano-native-legacy-source-admission-v1','effectiveSourceRevision':SOURCE_REVISION,'coreSourceBaseRevision':CORE_SOURCE_BASE,'coreClosurePublic':closure_ref,'coreClosurePrivate':private_ref,'gitSourceBlobs':git_cache,'dependencies':{k:sorted(v) for k,v in dependencies.items()},'linkOrders':orders,'heldMaterials':H['HELD'],'aliasGuards':H['ALIASES'],'oldAuthoritiesRefreshed':False,'historicalUnknownAliasBindingRecovered':False})
 support_obj=out/'sync_fault_macos.cpp.o';support_md=out/'sync_fault_macos.cpp.o.d';support_lib=out/'libnano-test-sync-fault.dylib'
 command=list(commands[SUPPORT_TARGET]);command[command.index('-o')+1]=str(support_obj);command+=['-MD','-MT','legacy-proof-2','-MF',str(support_md)]
 run('support-CXX',command,b,env,120)
 new_md=H['output'](support_md);need(H['dependency_set'](new_md,b)==dependencies[SUPPORT_TARGET]==old_mds[SUPPORT_TARGET][1],'new real support MD equals independent M and historical actual MD')
 new_obj=H['output'](support_obj);macho(new_obj,1);need(digest(new_obj)=={'sha256':SUPPORT_PINS['object'][0],'bytes':SUPPORT_PINS['object'][1]},'support object reproduced byte-identically')
 stable_batch({str(support_obj):digest(new_obj),str(support_md):digest(new_md)})
 command=[str(support_obj) if x==objects[SUPPORT_TARGET][1] else x for x in links[SUPPORT_TARGET]];command[command.index('-o')+1]=str(support_lib)
 run('support-LINK',command,b,env,120);new_lib=H['output'](support_lib);macho(new_lib,6)
 need(digest(new_lib)=={'sha256':SUPPORT_PINS['dylib'][0],'bytes':SUPPORT_PINS['dylib'][1]},'support dylib reproduced byte-identically without altering install-name')
 stable_batch({str(support_lib):digest(new_lib)});outputs={}
 for i,role in enumerate(ROLES):
  command=[str(core) if x==old_core else x for x in links[role]];command[command.index('-o')+1]=str(out/role)
  run('consumer-LINK-'+str(i),command,b,env,180);raw=H['output'](out/role);macho(raw,2)
  stable_batch({str(out/role):digest(raw)});outputs[role]=dict(digest(raw),architecture='arm64',format='Mach-O64');fence()
 home=out/'probe-home';tmp=out/'probe-tmp';home.mkdir(mode=0o700);tmp.mkdir(mode=0o700)
 probe_env={'PATH':'/usr/bin:/bin','HOME':str(home),'TMPDIR':str(tmp),'LANG':'C','LC_ALL':'C'}
 destination=out/'REFUSED-NO-SEED-NO-LEDGER';need(not destination.exists(),'probe destination absent')
 raw,err=run('probe-export-refusal',[str(out/'nano-capella-transition-tests'),'--export-isolated-deposit-network',str(destination)],out,probe_env,20,allowed=(1,))
 need(raw==b'' and err==b'isolated fixture seed file and persistent ledger paths required\n' and not destination.exists() and not list(home.iterdir()) and not list(tmp.iterdir()),'exporter exact early missing-env refusal without fixture allocation')
 raw,err=run('probe-import-refusal',[str(out/'nano-capella-isolated-bootstrap-import')],out,probe_env,20,allowed=(1,))
 expected={'format':'nano-isolated-fixture-private-binding-audit-v1','passed':False,'refusedStage':'arguments','errorDetailsSuppressed':True,'newSignaturesCreated':0,'signerCallbacksInvoked':0,'runtimeStarted':False,'networkLaunched':False,'productionQualified':False}
 got=json.loads(raw);exact(got,expected,'import refusal exact nine fields');need(not err and got==expected and not list(home.iterdir()) and not list(tmp.iterdir()) and not destination.exists(),'importer exact argc refusal without fixture/runtime')
 fence()
 admission={'format':'nano-native-legacy-software-admission-v1','effectiveSourceRevision':SOURCE_REVISION,'coreClosurePublic':closure_ref,'coreClosurePrivate':private_ref,'nativeInput':c['nativeInput'],'sourceAdmission':dict(H['NEW'][str(out/'admitted.private.json')],path=str(out/'admitted.private.json')),'coreArchive':dict(first['newSoftwareMaterials']['libnano_core.a'],path=str(core)),'programMaterials':{role:H['HELD'][str(out/role)] for role in ROLES},'originalConsumerObjectMaterials':{role:H['HELD'][str(objects[role][0])] for role in ROLES},'supportReproductionMaterials':{str(p):H['HELD'][str(p)] for p in (support_obj,support_md,support_lib)},'originalSupportLibrary':dict(original[str(old_support)],path=str(old_support)),'softwareStableSamples':3,'softwareStableSpanAtLeastTwoSeconds':True,'historicalUnknownAliasBindingRecovered':False,'oldAuthoritiesRefreshed':False,'productionQualified':False}
 publish('new-software-admission.private.json',admission);admission_pin=H['NEW'][str(out/'new-software-admission.private.json')]
 stable_batch({str(out/'new-software-admission.private.json'):admission_pin});fence()
 result={'format':'nano-native-legacy-relink-terminal-v1','passed':True,'owner':'mac-native','physicalArchitecture':'Darwin-arm64','effectiveSourceRevision':SOURCE_REVISION,'coreSourceBaseRevision':CORE_SOURCE_BASE,'coreClosurePublicSha256':CLOSURE_SHA,'coreArchive':{'sha256':CORE_SHA,'bytes':CORE_BYTES},'programs':outputs,'newSoftwareAdmission':dict(admission_pin),'genuineMPairsEqualHistoricalActualMd':3,'supportNewActualMdEqualsMAndHistoricalMd':True,'supportCompilations':1,'supportDylibLinks':1,'supportObjectByteIdentical':True,'supportDylibByteIdentical':True,'originalSupportBindingPreserved':True,'consumerCompilations':0,'consumerObjectReuses':2,'consumerLinks':2,'focusedCliRefusalProbes':2,'loaderAndMainEntryQualified':True,'allLazySymbolBindingsQualified':False,'realFixtureExportOrImportExecuted':False,'fixtureBlsSignaturesCreated':0,'operationalSignaturesCreated':0,'canonicalFiveArchivesTwoAssemblyOrderPreserved':True,'oldInputsUsedNineShaUnchanged':True,'oldAuthoritiesRefreshed':False,'sdkPinsAutomaticallyRefreshed':False,'historicalUnknownAliasBindingRecovered':False,'newSoftwareStableSamples':3,'newSoftwareStableSpanAtLeastTwoSeconds':True,'gitPublicSourceBlobs':len(git_cache),'selectedCoreProjectGitBlobsVerified':363,'osGitUtilityExplicitNewRootAuthority':True,'osGitUtilityClaimedAsNativeSdkOrOwner501Software':False,'gitConfiguredPoliciesPreserved':True,'configureCommands':0,'coreCompilations':0,'archiveMutations':0,'oldSuiteReplays':0,'longStreamInvocations':0,'dockerInvocations':0,'privateVpsAccessed':False,'runtimeStarted':False,'dutiesResumed':False,'multiownerFinalityQualified':False,'all24RolesAdmitted':False,'productionQualified':False,'events':EVENTS}
 fence();publish('terminal.json',result);fence();print(json.dumps(result),flush=True)


def interrupted(signum, frame):
 raise InterruptedError('controller terminated')


if __name__=='__main__':
 parser=argparse.ArgumentParser()
 parser.add_argument('--inputs',required=True);parser.add_argument('--sha256',required=True);parser.add_argument('--bytes',required=True,type=int)
 parser.add_argument('--thin-controller',required=True)
 parser.add_argument('--core-closure-result',required=True);parser.add_argument('--core-closure-sha256',required=True);parser.add_argument('--core-closure-bytes',required=True,type=int)
 args=parser.parse_args()
 signal.signal(signal.SIGTERM,interrupted)
 try:main(args)
 except BaseException as error:
  refusal={'format':'nano-native-legacy-relink-refused-v1','passed':False,'phase':PHASE,'errorType':type(error).__name__,'detailsExported':False,'events':EVENTS,'realFixtureExportOrImportExecuted':False,'runtimeStarted':False,'productionQualified':False}
  if OUT is not None:
   try:publish('terminal-refusal.private.json',refusal)
   except BaseException:pass
  print(json.dumps(refusal),flush=True);raise SystemExit(1)

