#!/usr/bin/env python3
"""ROOT-only Native two-consumer thin build. No operation on import.

Uses the frozen c586 helper definitions, never their main. No configure,
Core/third-party compilation, archive mutation, long-stream replay or runtime.
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
SOURCE_REVISION = 'def5cda61919bbb40e91744a18661a2ad7a76eba'
OVERLAY_RELATIVE = 'tests/support/fresh_lifecycle_terminal.hpp'
OVERLAY_BEFORE = ('07993c65ce97b3e2db1bd640abaabd7abce393e1dea57135bb08c0ad8f2729b8',24334)
OVERLAY_AFTER = ('6681926f496290afc2b98495315b8ae5ba1c646095aece0e21287cca83b28629',24376)
OVERLAY_MANIFEST = ('6706dd2c02cde8024c4c2e6581ecf48fdd19bda298fc50ffea2d7c57119838c8',480)
F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
ROLES = {
 'nano-capella-deposit-audit': ('tests/capella/deposit_audit.cpp','tests/test_capella_duty_availability.cpp','nano-capella-duty-availability-tests'),
 'nano-capella-lifecycle-submit': ('tests/multihost/lifecycle_submit.cpp','tests/capella/isolated_bootstrap_audit.cpp','nano-capella-isolated-bootstrap-audit'),
}
TUS = {
 'tests/capella/deposit_audit.cpp': ('00d6b280ffde5c5fa9f10ee25b0cb87399f111adae7a5d5a8a274787bcf4d33e',16560),
 'tests/multihost/lifecycle_submit.cpp': ('43687eebe75dc51f31cf55371bd901d34236b8e6908a1a4533a9f3d29eccf153',583),
}
FIXTURES = {
 'tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json',
 'tests/fixtures/capella-bootstrap-anchor-cb532/profile.json',
 'tests/fixtures/capella-bootstrap-anchor-cb532/genesis-state.ssz',
 'tests/fixtures/capella-bootstrap-anchor-cb532/genesis.json',
 'tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz',
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
 global OUT,DEADLINE,PHASE
 DEADLINE=BEGAN+1100
 signal.signal(signal.SIGTERM,lambda *_:(_ for _ in ()).throw(InterruptedError()))
 need(sys.platform=='darwin' and os.uname().machine=='arm64' and os.getuid()==501 and os.getgid()==20,'native owner/architecture')
 # The helper pin is supplied separately so no untrusted input chooses code.
 helper={'path':a.thin_controller,'sha256':THIN_SHA,'bytes':THIN_BYTES};load_helpers(helper)
 c=json.loads(known({'path':a.inputs,'sha256':a.sha256,'bytes':a.bytes}))
 fields=('format','controller','nativeInput','firstSoftwareAdmission','nativeTerminal','currentSourceRoot','sourceRepository','gitTool','outputRoot','sourceOverlayArtifact')
 exact(c,fields,'minimal exact input10 with one external overlay')
 need(c['format']=='nano-native-two-consumers-input-v2','input format');need(pin(c['controller'])==physical(__file__),'controller exact locator');known(c['controller'])
 closure_ref={'path':a.core_closure_result,'sha256':a.core_closure_sha256,'bytes':a.core_closure_bytes}
 closure=json.loads(known(closure_ref,(CLOSURE_SHA,CLOSURE_BYTES)))
 need(closure['format']=='nano-native-selected-closure-read-only-public-v1' and closure['passed'] is True and closure['coreMembers']==143 and closure['inheritedCoreMembersVerified']==142 and closure['newApiMemberFromRetainedProducer']==1 and closure['newPreprocessorScans']==5 and closure['historicalUnknownAliasBindingRecovered'] is False and closure['oldAuthoritiesRefreshed'] is False and closure['productionQualified'] is False,'actual external selected-Core closure PASS')
 revision=closure['effectiveSourceRevision'];need(revision==SOURCE_REVISION,'fixed effective public source revision')
 # Producer-known descriptor, not a fresh digest promoted to prior authority.
 private_ref=dict(closure['privateReceipt'],path=str(pin(closure_ref).parent/'closure.private.json'))
 closure_private=json.loads(known(private_ref))
 need(closure_private['oldAuthoritiesRefreshed'] is False and closure_private['newStorageReproductionPublicSha256']==closure['newStorageReproductionPublicSha256'],'selected private/public producer join')
 old=closure_private['heldMaterials'];aliases=closure_private['prospectiveAliasGuards']
 need(isinstance(old,dict) and 0<len(old)<=65536 and isinstance(aliases,dict) and 0<len(aliases)<=8192,'retained authority closure bounds')
 for p,row in old.items():recorded(p,row)
 for p,row in aliases.items():
  exact(row,('resolved','material','link'),'retained alias record');exact(row['material'],F,'alias full nine');need(pathlib.Path(p).is_absolute(),'alias absolute')
  H['ALIASES'][p]=row
 fence()
 ni=json.loads(known(c['nativeInput'],(NATIVE_INPUT_SHA,NATIVE_INPUT_BYTES)))
 need(ni['format']=='nano-cross-arch-validator-native-build-input-v1' and ni['sourceBaseRevision']==closure['originalApiProducerBase']=='ae039e1c6fa4dbc6b1ed705a6f27a96956ba8050' and (ni['controller']['sha256'],ni['controller']['bytes'])==(THIN_SHA,THIN_BYTES),'original Native input and actual helper producer')
 cfg=json.loads(known(ni['oldConfig'],(CFG_SHA,CFG_BYTES)))
 projection=json.loads(known(ni['oldAdmission']))
 original=json.loads(known(projection['provenance']['originalInputs'],(MATERIAL_SHA,MATERIAL_BYTES)))
 sdk=json.loads(known(ni['nativeSdkAuthority'],(SDK_SHA,SDK_BYTES)))
 need(projection['files']==original and projection['provenance']['oldConfig']==ni['oldConfig'] and projection['provenance']['nativeSdkAuthority']==ni['nativeSdkAuthority'] and projection['aliases']==sdk['sdkAliases'],'original25c/projection/config/SDK equality')
 for p,row in original.items():
  if p in old:need(row==old[p],'retained original record must not rebase')
 for p,row in sdk['sdkFiles'].items():need(p in old and old[p]==row,'all original SDK regular files held by selected closure')
 for p,target in sdk['sdkAliases'].items():need(p in aliases and aliases[p]['resolved']==target,'all original SDK target bindings retained; historical unknown stays unknown')
 first=json.loads(known(c['firstSoftwareAdmission'],(FIRST_SHA,FIRST_BYTES)))
 need(first['format']=='nano-new-native-software-first-root-admission-v1' and first['stableSamples']==3 and first['stableSpanMs']>=2000 and first['oldAuthoritiesRefreshed'] is False,'original first software admission')
 terminal_raw=recorded(pin(c['nativeTerminal']),first['terminalMaterial'])
 need((hashlib.sha256(terminal_raw).hexdigest(),len(terminal_raw))==(NATIVE_TERMINAL_SHA,NATIVE_TERMINAL_BYTES),'actual retained Native terminal')
 terminal=json.loads(terminal_raw);need(terminal['passed'] is True and terminal['newCoreArchive']=={'sha256':CORE_SHA,'bytes':CORE_BYTES},'original Native Core producer')
 need(physical(ni['controlRoot'])==pin(c['nativeTerminal']).parent,'same original Native producer output root')
 core=pin(c['nativeTerminal']).parent/'libnano_core.a'
 core_raw=recorded(core,first['newSoftwareMaterials']['libnano_core.a'])
 need((hashlib.sha256(core_raw).hexdigest(),len(core_raw))==(CORE_SHA,CORE_BYTES) and len(H['archive_members'](core_raw))==143,'same immutable Core7264')
 b,s,current,repo=(physical(str(p)) for p in (cfg['buildRoot'],cfg['sourceRoot'],c['currentSourceRoot'],c['sourceRepository']))
 need(b==physical(ni['buildRoot']) and s==physical(ni['sourceRoot']) and current!=s and not current.is_relative_to(s) and not s.is_relative_to(current),'old and current source roots')
 overlay=json.loads(known(c['sourceOverlayArtifact'],OVERLAY_MANIFEST))
 exact(overlay,('format','sourceBaseRevision','files'),'single overlay manifest fields')
 need(overlay['format']=='nano-native-lifecycle-u64-portability-overlay-v1' and overlay['sourceBaseRevision']==revision and isinstance(overlay['files'],list) and len(overlay['files'])==1,'explicit consumer-only portability overlay')
 overlay_row=overlay['files'][0];exact(overlay_row,('relative','before','after'),'single overlay file fields')
 exact(overlay_row['before'],('sha256','bytes'),'original Git header byte pin')
 need(overlay_row['relative']==OVERLAY_RELATIVE and overlay_row['before']==dict(zip(('sha256','bytes'),OVERLAY_BEFORE)),'exact one original portability header')
 after_ref=overlay_row['after'];need(pin(after_ref)==current/OVERLAY_RELATIVE,'overlay applies to the actual consumer source tree')
 need(str(current/OVERLAY_RELATIVE) not in old and OVERLAY_RELATIVE not in closure_private['projectSourceBytes'] and str(current/OVERLAY_RELATIVE) not in closure_private['projectSourceBytes'],'consumer overlay is outside unchanged selected Core source closure')
 known(after_ref,OVERLAY_AFTER)
 out=pathlib.Path(c['outputRoot']);need(out.is_absolute() and str(out)==os.path.normpath(str(out)) and not out.exists() and out.parent.resolve(strict=True)==out.parent and out.parent.stat().st_uid==501,'new owned output namespace')
 need(all(not out.is_relative_to(p) and not p.is_relative_to(out) for p in (b,s,current,repo)),'outputs disjoint from all inputs')
 out.mkdir(mode=0o700);OUT=out;H['CONTROL']=out;H['DEADLINE']=DEADLINE
 env={k:v for k,v in os.environ.items() if not k.startswith(('NANO_','DYLD_','GIT_')) and k not in ('LD_PRELOAD','CPATH','CPLUS_INCLUDE_PATH','C_INCLUDE_PATH','LIBRARY_PATH','CXXFLAGS','LDFLAGS')}
 env.update(GIT_CONFIG_NOSYSTEM='1',GIT_CONFIG_GLOBAL='/dev/null',GIT_OPTIONAL_LOCKS='0',GIT_NO_REPLACE_OBJECTS='1',GIT_NO_LAZY_FETCH='1',GIT_TERMINAL_PROMPT='0')
 exact(c['gitTool'],('path','sha256','bytes','material'),'explicit OS Git utility authority')
 git_ref={k:c['gitTool'][k] for k in ('path','sha256','bytes')};git=pin(git_ref)
 need(str(git)=='/usr/bin/git','fixed read-only OS utility')
 git_raw=recorded(git,c['gitTool']['material'])
 need(c['gitTool']['material']['st_uid']==0 and os.access(git,os.X_OK) and (hashlib.sha256(git_raw).hexdigest(),len(git_raw))==(git_ref['sha256'],git_ref['bytes']),'Root-admitted OS Git full-nine/bytes/executable')
 git_cache={};git_batches=0
 def relative_blob(relative):
  need(isinstance(relative,str) and relative and not relative.startswith('/') and all(x not in ('','.','..') for x in relative.split('/')) and not re.search(r'[\x00\r\n\\:]',relative),'relative public blob locator')
 def ensure_blobs(relatives):
  nonlocal git_batches
  for relative in relatives:relative_blob(relative)
  todo=sorted(set(relatives)-set(git_cache))
  if not todo:return
  need(len(todo)<=2048,'bounded public Git source set')
  requests=''.join(revision+':'+x+'\n' for x in todo).encode()
  raw,err=run('git-batch-'+str(git_batches),[str(git),'-C',str(repo),'cat-file','--batch'],repo,env,60,stdin_bytes=requests);git_batches+=1
  need(not err,'Git source-only batch stderr empty');at=0
  for relative in todo:
   end=raw.find(b'\n',at);need(end>=at and end-at<=128,'Git batch bounded header')
   header=raw[at:end].decode('ascii');match=re.fullmatch('([0-9a-f]{40}) blob ([0-9]+)',header);need(match is not None,'exact Git blob response')
   size=int(match[2]);at=end+1;need(0<=size<=33554432 and at+size<len(raw) and raw[at+size:at+size+1]==b'\n','Git batch body bounds')
   body=raw[at:at+size];at+=size+1
   need(hashlib.sha1(b'blob '+str(size).encode()+b'\0'+body).hexdigest()==match[1],'Git OID binds exact blob bytes')
   git_cache[relative]={'sha256':hashlib.sha256(body).hexdigest(),'bytes':size,'gitBlobOid':match[1]}
  need(at==len(raw),'no extra Git batch output')
 def blob(relative):
  relative_blob(relative);need(relative in git_cache,'public source blob must be read before admission')
  return git_cache[relative]
 def consumer_source_pin(relative):
  base=blob(relative)
  if relative==OVERLAY_RELATIVE:
   need((base['sha256'],base['bytes'])==OVERLAY_BEFORE,'Git remains the original before-header')
   return after_ref
  return base
 def prior(path):
  p=physical(str(path));need(str(p) in old,'old reached file missing retained authority');return recorded(p,old[str(p)])
 def sdk_path(path):
  p=pathlib.Path(path);resolved=str(p.resolve(strict=True));need(resolved in sdk['sdkFiles'],'unrecorded SDK input refused')
  if str(p)!=resolved:need(str(p) in aliases and aliases[str(p)]['resolved']==resolved,'retained SDK alias missing')
  recorded(resolved,sdk['sdkFiles'][resolved]);return str(p)
 compiler=sdk_path(ni['tools']['compiler']);sdk_path(ni['tools']['linker'])
 fixtures=ni['runtimePublicFixtures'];need(isinstance(fixtures,list) and len(fixtures)==5 and {x['relative'] for x in fixtures}==FIXTURES,'five runtime fixtures outside M closure')
 fixture_rows={x['relative']:x for x in fixtures}
 for rel,ref in fixture_rows.items():
  raw=prior(s/rel);need((hashlib.sha256(raw).hexdigest(),len(raw))==(ref['sha256'],ref['bytes']),'old runtime fixture byte pins')
 need(fixture_rows['tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json']['sha256']=='ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6' and fixture_rows['tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz']['sha256']=='f5a464175565a8ed6a74790a60b84477c16a5b1bf71ece55736f192142cbca3d','fixture literal identities')
 manifest=json.loads(prior(s/'tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json'))
 for kind,leaf in (('profile','profile.json'),('genesisState','genesis-state.ssz'),('genesis','genesis.json')):
  ref=fixture_rows['tests/fixtures/capella-bootstrap-anchor-cb532/'+leaf];need(manifest['files'][kind]=={'path':leaf,'sha256':ref['sha256'],'bytes':ref['bytes']},'fixture manifest selected leaf')
 recipes=json.loads(prior(b/'compile_commands.json'));commands={};links={};objects={};orders={}
 ensure_blobs(['CMakeLists.txt',OVERLAY_RELATIVE,*TUS])
 need((blob(OVERLAY_RELATIVE)['sha256'],blob(OVERLAY_RELATIVE)['bytes'])==OVERLAY_BEFORE,'fixed Git before-overlay header')
 need((blob('CMakeLists.txt')['sha256'],blob('CMakeLists.txt')['bytes'])==('275b30b7f759bdd15d265db1303365a1069f0614f9eac40edc4b1767c9290946',174261),'exact public CMake properties reviewed for the two templates')
 for role,(actual,template,target) in ROLES.items():
  rows=[x for x in recipes if x['file']==str(s/template)];need(len(rows)==1 and rows[0]['directory']==str(b),'unique canonical template')
  argv=rows[0].get('arguments') or shlex.split(rows[0]['command']);need(argv[0]==compiler and argv.count('-o')==argv.count('-c')==argv.count(str(s/template))==1 and not any(x in argv for x in ('-M','-MM','-MD','-MMD','-MF','-MT','-MQ','-MP')),'canonical semantic compiler argv')
  argv=list(argv);argv[argv.index(str(s/template))]=str(current/actual);argv.insert(1,'-iquote'+str((s/template).parent));commands[role]=argv;objects[role]=argv[argv.index('-o')+1]
  macro=[x for x in argv if x.startswith('-DNANO_TEST_SOURCE=')]
  need((role.endswith('deposit-audit') and len(macro)==1 and macro[0].split('=',1)[1].strip('"')==str(s)) or (role.endswith('lifecycle-submit') and not macro),'unchanged Tool macro / no Submit macro')
  got=blob(actual);need((got['sha256'],got['bytes'])==TUS[actual],'current exact public TU')
  stable_batch({str(current/actual):got})
  link=shlex.split(prior(b/'CMakeFiles'/(target+'.dir')/'link.txt').decode());obj=objects[role]
  need(link[0]==compiler and link.count('-o')==link.count(obj)==link.count('libnano_core.a')==1 and link[link.index('-o')+1]==target,'canonical object/Core/output link substitutions')
  archive_count=asm_count=0;order=[]
  for value in link[1:]:
   if value==obj:order.append({'kind':'NEW_CONSUMER','argument':value});continue
   if value=='libnano_core.a':archive_count+=1;order.append({'kind':'RETAINED_CORE7264','argument':value});continue
   if value.endswith(('.a','.o','.dylib','.tbd')):
    p=pathlib.Path(value);p=p if p.is_absolute() else b/p;p=pathlib.Path(os.path.normpath(str(p)))
    if p.is_relative_to(b):prior(p)
    else:sdk_path(str(p))
    archive_count+=value.endswith('.a');asm_count+=value.endswith('.o');order.append({'kind':'RETAINED_LINK_INPUT','argument':value})
  need(archive_count==5 and asm_count==2,'five canonical archives and two ASM objects')
  links[role]=link;orders[role]=order
 dependencies={};reached_relatives=set()
 for i,(role,argv) in enumerate(commands.items()):
  base=list(argv);j=base.index('-o');del base[j:j+2];base.remove('-c')
  raw,err=run('M-'+str(i),base+['-M','-MT','native-lifecycle-'+str(i)],b,env,120);need(not err,'dependency stderr empty')
  found=H['dependency_set'](raw,b);need(str(current/ROLES[role][0]) in found,'actual TU reached by genuine M')
  if role=='nano-capella-deposit-audit':need(str(current/OVERLAY_RELATIVE) in found,'Tool must actually reach the authorized after-header')
  dependencies[role]=found
  for value in found:
   p=pathlib.Path(value)
   if p.is_relative_to(current):reached_relatives.add(str(p.relative_to(current)))
   elif p.is_relative_to(s):reached_relatives.add(str(p.relative_to(s)))
 ensure_blobs(reached_relatives)
 for role,found in dependencies.items():
  new={}
  for value in sorted(found):
   p=pathlib.Path(value)
   if p.is_relative_to(current):
    rel=str(p.relative_to(current));new[str(p)]=consumer_source_pin(rel)
   elif p.is_relative_to(s):
    raw=prior(p);wanted=consumer_source_pin(str(p.relative_to(s)));need(hashlib.sha256(raw).hexdigest()==wanted['sha256'] and len(raw)==wanted['bytes'],'old reached project source equals exact effective Git blob')
   elif p.is_relative_to(b):prior(p)
   else:sdk_path(value)
  stable_batch(new);fence()
 publish('admitted.private.json',{'format':'nano-native-two-consumers-admission-v2','sourceBaseRevision':revision,'sourceOverlayArtifact':c['sourceOverlayArtifact'],'sourceOverlaySha256':c['sourceOverlayArtifact']['sha256'],'sourceOverlayBeforeGitBlob':git_cache[OVERLAY_RELATIVE],'coreClosurePublic':closure_ref,'heldMaterials':H['HELD'],'aliasGuards':H['ALIASES'],'gitSourceBlobs':git_cache,'dependencies':{k:sorted(v) for k,v in dependencies.items()},'linkOrders':orders,'originalCoreArchive':{'sha256':CORE_SHA,'bytes':CORE_BYTES},'oldAuthoritiesRefreshed':False,'historicalUnknownAliasBindingRecovered':False})
 outputs={};new_objects={}
 for i,(role,argv) in enumerate(commands.items()):
  obj=out/(pathlib.Path(ROLES[role][0]).name+'.o');md=out/(obj.name+'.d');command=list(argv);command[command.index('-o')+1]=str(obj);command+=['-MD','-MT','native-lifecycle-'+str(i),'-MF',str(md)]
  run('CXX-'+str(i),command,b,env,240)
  need(H['dependency_set'](H['output'](md),b)==dependencies[role],'actual new MD equals genuine M')
  raw=H['output'](obj);stable_batch({str(obj):{'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}})
  new_objects[role]=obj
  command=[str(obj) if x==objects[role] else str(core) if x=='libnano_core.a' else x for x in links[role]];command[command.index('-o')+1]=str(out/role)
  run('LINK-'+str(i),command,b,env,180);raw=H['output'](out/role)
  need(len(raw)>=32 and raw[:4]==b'\xcf\xfa\xed\xfe' and int.from_bytes(raw[4:8],'little')==0x0100000c,'actual Native arm64 Mach-O64 consumer')
  stable_batch({str(out/role):{'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}});outputs[role]=dict(H['NEW'][str(out/role)],architecture='arm64',format='Mach-O64');fence()
 tests=[('action-policy','nano-capella-deposit-audit','--fresh-lifecycle-self-test',30,['fresh-lifecycle-public-identity','fresh-lifecycle-real-operation-policy','fresh-lifecycle-immutable-file-refusals','fresh-lifecycle-summary']),('sealed-observe','nano-capella-deposit-audit','--fresh-lifecycle-observe-self-test',60,['fresh-lifecycle-observe-authenticated-head','fresh-lifecycle-observe-only-null-scope','fresh-lifecycle-observe-corrupt-head-refusal','fresh-lifecycle-observe-summary']),('managed-submit','nano-capella-lifecycle-submit','--self-test',30,['fresh-lifecycle-managed-submit-policy','fresh-lifecycle-managed-submit-ipc','fresh-lifecycle-managed-submit-input-fences','fresh-lifecycle-managed-submit-summary'])]
 summaries={}
 for name,role,selector,seconds,scenarios in tests:
  raw,err=run('test-'+name,[str(out/role),selector],b,env,seconds)
  rows=[json.loads(x) for x in raw.decode().splitlines()];need(not err and len(rows)==4 and [x.get('scenario') for x in rows]==scenarios and all(x.get('passed') is True for x in rows),'four successful exact selector rows')
  summary=rows[-1];need(summary['groups']==3 and summary['productionQualified'] is False and summary['envRead'] is False and summary['databaseAccessed'] is False and summary['networkUsed'] is False,'selector no env/database/network/production')
  need(type(summary['groups']) is int and type(summary['expectedRefusals']) is int,'typed test counters')
  if name=='action-policy':
   need(summary['expectedRefusals']==12 and summary['fixtureBlsSignatures']==3 and summary['operationalSignatures']==0 and summary['runtimeStarted'] is False and rows[0]['publicKeyJoins']==2 and rows[0]['nullIndexFallbackUsed'] is False and rows[1]['signedExitWires']==rows[1]['signedSlashingWires']==1 and rows[1]['authenticatedInputUnchanged'] is True and rows[2]['originalAssetsRead'] is False,'action policy typed assertions')
  elif name=='sealed-observe':
   need(summary['expectedRefusals']==9 and summary['fixtureBlsSignatures']==4 and summary['operationalSignatures']==0 and summary['positiveHeadChecks']==4 and summary['archiveReads']==5 and summary['genuineSignedChildren']==1 and summary['runtimeStarted'] is False and summary['oldActionPolicyOrLongStreamExecuted'] is False and rows[0]['completeClosuresDecoded']==2 and rows[1]['exactActionHeadAccepted'] is True and rows[1]['exactTerminalHeadAccepted'] is True and rows[2]['authenticatedInputsUnchanged'] is True,'sealed observe typed assertions')
  else:
   need(summary['expectedRefusals']==26 and summary['signaturesCreated']==0 and summary['execExecuted'] is False and rows[0]['cryptographicWireVerificationClaimed'] is False and rows[1]['callbackCalls']==5 and rows[1]['realHttpUsed'] is False and rows[1]['execExecuted'] is False and rows[2]['actualOpaqueFilesystemFixtures'] is True,'managed callback-only typed assertions')
  summaries[name]=summary;fence()
 fence()
 software_admission={'format':'nano-native-two-consumers-software-admission-v2','sourceBaseRevision':revision,'sourceOverlayArtifact':c['sourceOverlayArtifact'],'sourceOverlaySha256':c['sourceOverlayArtifact']['sha256'],'coreClosurePublic':closure_ref,'nativeInput':c['nativeInput'],'sourceAdmission':dict(H['NEW'][str(out/'admitted.private.json')],path=str(out/'admitted.private.json')),'coreArchive':dict(first['newSoftwareMaterials']['libnano_core.a'],path=str(core)),'programMaterials':{role:H['HELD'][str(out/role)] for role in ROLES},'objectMaterials':{role:H['HELD'][str(new_objects[role])] for role in ROLES},'softwareStableSamples':3,'softwareStableSpanAtLeastTwoSeconds':True,'historicalUnknownAliasBindingRecovered':False,'oldAuthoritiesRefreshed':False,'productionQualified':False}
 publish('new-software-admission.private.json',software_admission)
 admission_pin=H['NEW'][str(out/'new-software-admission.private.json')]
 stable_batch({str(out/'new-software-admission.private.json'):admission_pin});fence()
 result={'format':'nano-native-two-consumers-build-terminal-v2','passed':True,'owner':'mac-native','physicalArchitecture':'Darwin-arm64','effectiveSourceBaseRevision':revision,'consumerSourceProfile':'base-plus-one-u64-portability-header-overlay-v1','sourceOverlaySha256':c['sourceOverlayArtifact']['sha256'],'sourceOverlayIsNewGitBlobAtBaseRevision':False,'coreClosurePublicSha256':CLOSURE_SHA,'coreArchive':{'sha256':CORE_SHA,'bytes':CORE_BYTES},'programs':outputs,'newSoftwareAdmission':dict(admission_pin),'consumerCompilations':2,'programLinks':2,'genuineMPairsEqualActualMd':2,'focusedCliTests':3,'runtimeFixtureLeavesFenced':5,'canonicalFiveArchivesTwoAssemblyOrderPreserved':True,'oldInputsUsedNineShaUnchanged':True,'sdkPinsAutomaticallyRefreshed':False,'historicalUnknownAliasBindingRecovered':False,'newSoftwareStableSamples':3,'newSoftwareStableSpanAtLeastTwoSeconds':True,'gitPublicSourceBlobs':len(git_cache),'osGitUtilityExplicitNewRootAuthority':True,'osGitUtilityClaimedAsNativeSdkOrOwner501Software':False,'testSummaries':summaries,'configureCommands':0,'coreCompilations':0,'archiveMutations':0,'thirdPartyCompilations':0,'oldSuiteReplays':0,'longStreamInvocations':0,'dockerInvocations':0,'privateVpsAccessed':False,'httpExecTransportQualified':False,'runtimeStarted':False,'dutiesResumed':False,'multiownerFinalityQualified':False,'all24RolesAdmitted':False,'productionQualified':False,'events':EVENTS}
 fence();publish('terminal.json',result);fence();print(json.dumps(result),flush=True)


if __name__=='__main__':
 parser=argparse.ArgumentParser()
 parser.add_argument('--inputs',required=True);parser.add_argument('--sha256',required=True);parser.add_argument('--bytes',required=True,type=int)
 parser.add_argument('--thin-controller',required=True)
 parser.add_argument('--core-closure-result',required=True);parser.add_argument('--core-closure-sha256',required=True);parser.add_argument('--core-closure-bytes',required=True,type=int)
 args=parser.parse_args()
 try:main(args)
 except BaseException as error:
  refusal={'format':'nano-native-two-consumers-refused-v2','passed':False,'phase':PHASE,'errorType':type(error).__name__,'detailsExported':False,'events':EVENTS,'runtimeStarted':False,'productionQualified':False}
  if OUT is not None:
   try:publish('terminal-refusal.private.json',refusal)
   except BaseException:pass
  print(json.dumps(refusal),flush=True);raise SystemExit(1)
