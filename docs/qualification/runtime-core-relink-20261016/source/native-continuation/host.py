#!/usr/bin/env python3
"""Root-only continuation: reuse five prior programs; relink only Tool/Submit."""
import argparse,hashlib,json,os,pathlib,re,shlex,signal,stat,sys,time,types
E=pathlib.Path(__file__).resolve().parents[1]
P=E/'current-native-runtime-relink-source-20261016-v2/qualify-native-runtime-relink-root.py'
VP={'sha256':'7ef818ba1c8c3f210ed2fcc6236438322e5492e7c4a9b617f66c8ca05125b293','bytes':24490}
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
OBS={
 'current-runtime-core':('a24c5bfb04333d6d770d540b40e77b4768ba1fddf10f6068ca2bc4cfcd422a36',1427),
 'runtime-object':('e3258c43899ed1a2a243075f48c1971dfcba219fdfa766b5e5351babd6d13556',2854),
 'nano':('8bb08725875ec82a7aa9bf8a0faaa9e003ee1acb3f7057517be3d45f36e26639',1396),
 'exporter':('33cc5c0249c083a1c9aa56c29fe5c732e1e3694c43271c7f19ad91efc8fbb335',1472),
 'bootstrapImport':('e786ae8d4892fdf6eccf78d2fc1c41d2c10754f294cff1f16735c34cd01f7d78',1495),
 'bootstrapAudit':('a1572c5a982b973488680c6ebb600a0c12d42345fbb56d7e2879edb504aa58f9',1492),
 'stoppedCapture':('b995cf491bb190e4714e58d2c2bf5d6d482aa3e58a4a79bd4ab47ca29b6f75b6',1492)}
LEAVES={'nano':'nano','exporter':'nano-capella-transition-tests','bootstrapImport':'nano-capella-isolated-bootstrap-import','bootstrapAudit':'nano-capella-isolated-bootstrap-audit','stoppedCapture':'nano-capella-isolated-stopped-capture','lifecycleTool':'nano-capella-deposit-audit','lifecycleSubmit':'nano-capella-lifecycle-submit'}
v=None;m=None;O=None;created=False
def need(ok,message):
 if not ok:raise RuntimeError(message)
def pin(body):return {'sha256':hashlib.sha256(body).hexdigest(),'bytes':len(body)}
def bounded(path,limit):
 path=pathlib.Path(path);need(path.is_absolute() and path.resolve(strict=True)==path,'physical control');fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);need(stat.S_ISREG(s.st_mode) and 0<s.st_size<=limit,'bounded regular control');row={k:getattr(s,k) for k in F};body=bytearray()
  while True:
   b=os.read(fd,min(65536,s.st_size+1-len(body)))
   if not b:break
   body.extend(b);need(len(body)<=s.st_size,'fixed control length')
  need(len(body)==s.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(path.lstat(),k) for k in F},'control FD/path nine');return dict(row,sha256=hashlib.sha256(body).hexdigest()),bytes(body)
 finally:os.close(fd)
def root_arg(value):
 p=pathlib.Path(value);need(p.is_absolute() and p.parent==E and re.fullmatch('current-native-runtime-relink-root-20261016-[a-z0-9-]{1,40}',p.name) is not None,'bounded output namespace');return p
def normalized_current(names):
 result=set()
 for name in names:
  need(isinstance(name,str) and pathlib.Path(name).is_absolute() and not any(c in name for c in ('\0','\r','\n')),'absolute dependency spelling');normal=os.path.normpath(name)
  need(pathlib.Path(name).resolve(strict=True)==pathlib.Path(normal).resolve(strict=True),'normalized current dependency changes resolution');result.add(normal)
 return sorted(result)
def observations(previous,name,products):
 sha,size=OBS[name];path=previous/(name+'-product-first-observations.private.json');row,body=bounded(path,16384);need(pin(body)=={'sha256':sha,'bytes':size},'producer observation pin');m.HELD[str(path)]=row;values=json.loads(body)
 need(isinstance(values,list) and len(values)>=3 and all(isinstance(x,dict) and isinstance(x.get('files'),dict) and type(x.get('elapsedMs')) is int for x in values),'producer observations shape')
 need(values[0]['elapsedMs']>=0 and values[-1]['elapsedMs']-values[0]['elapsedMs']>=2000 and all(a['elapsedMs']<=b['elapsedMs'] for a,b in zip(values,values[1:])),'producer three-sample span')
 need(all(x['files']==values[0]['files'] for x in values),'original first observations not identical');rows=values[-1]['files']
 want=[previous/'libnano_core.a'] if name=='current-runtime-core' else [previous/'consensus_capella_runtime.cpp.o',previous/'consensus_capella_runtime.cpp.o.d'] if name=='runtime-object' else [previous/LEAVES[name]]
 need(set(rows)==set(map(str,want)),'exact selected product observation namespace')
 for p in want:
  old=rows[str(p)];need(set(old)==set(F)|{'sha256'} and all(type(old[k]) is int for k in F),'retained product nine shape')
  need(stat.S_ISREG(old['st_mode']) and old['st_nlink']==1 and old['st_uid']==501 and old['st_gid']==20 and old['st_size']>0,'retained product owner/type')
  if name=='current-runtime-core':need(stat.S_IMODE(old['st_mode'])==0o400,'retained Core mode')
  elif name!='runtime-object':need(stat.S_IMODE(old['st_mode'])==0o500,'retained executable mode')
  need(m.material(p)[0]==old,'original product full-nine/SHA changed');need(str(p) not in m.PRODUCTS or m.PRODUCTS[str(p)]==old,'no product refresh');m.PRODUCTS[str(p)]=old;products[str(p)]=old
 return {'path':str(path),'sha256':sha,'bytes':size}
def main():
 global v,m,O,created
 parser=argparse.ArgumentParser();parser.add_argument('--previous-root',required=True);parser.add_argument('--output-root',required=True);parser.add_argument('--revision',required=True);parser.add_argument('--episode',required=True);parser.add_argument('--episode-sha256',required=True);parser.add_argument('--episode-bytes',required=True,type=int);a=parser.parse_args()
 previous=root_arg(a.previous_root);candidate=root_arg(a.output_root);need(previous.is_dir() and previous.resolve()==previous and candidate!=previous and not candidate.exists() and not candidate.is_symlink(),'old namespace/new namespace')
 need(re.fullmatch('[0-9a-f]{40}',a.revision) is not None and re.fullmatch('[0-9a-f]{64}',a.episode_sha256) is not None and 0<a.episode_bytes<=4096,'explicit known episode/revision')
 need(os.getuid()==501 and os.getgid()==20 and os.uname().sysname=='Darwin' and os.uname().machine=='arm64','Native platform/owner')
 prow,pbody=bounded(P,131072);need(pin(pbody)==VP,'frozen V2 public definitions');v=types.ModuleType('native_runtime_relink_v2_definitions');v.__file__=str(P);exec(compile(pbody,str(P),'exec'),v.__dict__)
 hrow,hbody=bounded(v.HP,131072);need(pin(hbody)==v.HELPER,'frozen FD/process definitions');m=types.ModuleType('native_runtime_continuation_helpers');m.__file__=str(v.HP);exec(compile(hbody,str(v.HP),'exec'),m.__dict__)
 O=candidate;m.O=O;v.O=O;v.m=m;m.START=time.monotonic();m.DEADLINE=m.START+700;signal.signal(signal.SIGHUP,m.stop)
 O.mkdir(mode=0o700);created=True;(O/'tmp').mkdir(mode=0o700);(O/'probe-home').mkdir(mode=0o700);m.ENV={'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','HOME':str(O/'probe-home'),'TMPDIR':str(O/'tmp'),'LANG':'C','LC_ALL':'C','DEVELOPER_DIR':'/Library/Developer/CommandLineTools'}
 m.HELD[str(P)]=prow;m.HELD[str(v.HP)]=hrow
 erow,ebody=bounded(a.episode,4096);need(pin(ebody)=={'sha256':a.episode_sha256,'bytes':a.episode_bytes},'Root partial episode pin');episode=json.loads(ebody);m.HELD[str(pathlib.Path(a.episode))]=erow
 need(episode=={'format':'nano-native-runtime-relink-partial-episode-v1','sourceRevision':a.revision,'previousOutputRoot':str(previous),'controllerSha256':VP['sha256'],'completedProgramRoles':['nano','exporter','bootstrapImport','bootstrapAudit','stoppedCapture'],'completedIndependentMCommands':6,'compiledTranslationUnits':1,'currentCoreLinks':5,'smokeCliCount':6,'refusedStage':'M-lifecycleTool-dependency-comparison','passed':False,'productionQualified':False},'exact Root partial episode facts')
 need(v.git('rev-parse','HEAD').decode().strip()==a.revision and v.git('status','--porcelain')==b'','clean current source');need(v.git('diff','--name-only',v.BASE,a.revision,'--','src','include','third_party','network').decode().splitlines()==['src/consensus_capella_runtime.cpp'],'selected runtime-only source delta')
 # Reuse the same original descriptors. Missing metadata cannot be replaced by stat.
 current=v.descriptor('native-rpc-and-nano-root-qualification-attempt02-20261016/input-post-fence.private.json','77ee8eb21a4be7accf6a070cd11ed9534d69787a993b49cca90669ba8a6a1fc5',1034934)
 exporter=v.descriptor('unsigned-preview-native-root-qualification-attempt02-20261016/input-post-fence.private.json','55d81e9f0f93fd0cbbde9a9e6fef5706a731dd05706ada05221af94675071e3b',1910878)
 helpers=v.descriptor('current-native-five-helpers-root-attempt01-20261016/software-admission.private.json','04b06bf5374ed28d7accf6549935993b7e3dccc3333e5c12776b04f654f4f64a',1037791)
 native2=v.descriptor('fresh-native-two-consumers-root-20261016/native-02/admitted.private.json','7eceff399d56849e2ef8107664b316455fdd96833bebb41b7c7a804abbab6735',2874988)
 legacy=v.descriptor('fresh-native-legacy-root-20261016/native-01/admitted.private.json','8bcd2f07082a5a36036e577702c444f4a36e9902923b07b5b96d7bedfcebb977',3207851)
 products=v.descriptor('fresh-native-two-consumers-root-20261016/native-02/new-software-admission.private.json','f8cf61dd322d92dbfd0d1d319d4e3f3ebcc1f70cb8beaff222d1d47efbc66f19',3223)
 for value in (current,exporter,helpers):
  for key in ('files','productMaterials','products'):v.merge(m.AUTH,value.get(key,{}))
  v.merge(m.AA,value.get('aliases',{}))
 for value in (native2,legacy):v.merge(m.AUTH,value['heldMaterials']);v.merge(m.AA,value['aliasGuards'])
 retained={};proofs={name:observations(previous,name,retained) for name in OBS}
 core=previous/'libnano_core.a';oldcore=E/'native-rpc-and-nano-root-qualification-attempt02-20261016/libnano_core.a';original=m.archive(m.hold(oldcore,m.AUTH[str(oldcore)]));now=m.archive(m.material(core)[1]);member='consensus_capella_runtime.cpp.o'
 need([x[0] for x in original]==[x[0] for x in now] and [x for x in original if x[0]!=member]==[x for x in now if x[0]!=member],'original Core143/142 proof');runtime=retained[str(previous/member)];need([x for x in now if x[0]==member]==[(member,runtime['st_size'],runtime['sha256'])],'runtime object equals exact replacement member')
 # Only the two selected current source closures are materialized as Git byte pins.
 def current_project(logical):
  p=pathlib.Path(os.path.normpath(logical))
  for root in (v.R,v.S):
   if p.is_relative_to(root):
    rel=p.relative_to(root).as_posix();m.EXPECTED[str(p)]=pin(v.git('show',a.revision+':'+rel));return
 def selected(logical):
  physical=str(pathlib.Path(logical).resolve(strict=True));need(physical in m.AUTH or physical in m.EXPECTED or physical in m.PRODUCTS,'missing selected retained input authority');return original_candidate(logical)
 original_candidate=m.candidate;m.candidate=selected
 dbpath=v.B/'compile_commands.json';database=json.loads(m.hold(dbpath,m.AUTH[str(dbpath)]));cmakepath=v.R/'CMakeLists.txt';current_project(cmakepath);cmake=m.hold(cmakepath).decode()
 m.stable(['/usr/bin/c++','/Library/Developer/CommandLineTools/usr/bin/clang','/Library/Developer/CommandLineTools/usr/bin/ld'],'selected-tools')
 qualified={role:dict(path=str(previous/LEAVES[role]),sha256=retained[str(previous/LEAVES[role])]['sha256'],bytes=retained[str(previous/LEAVES[role])]['st_size']) for role in episode['completedProgramRoles']};stats={};bindings={}
 for role,rel,template in [('lifecycleTool','tests/capella/deposit_audit.cpp','tests/test_capella_duty_availability.cpp'),('lifecycleSubmit','tests/multihost/lifecycle_submit.cpp','tests/capella/isolated_bootstrap_audit.cpp')]:
  target=LEAVES[role];flags,template_obj=v.recipe(database,template);flags=[flags[0],'-iquote'+str(v.S/('tests' if role=='lifecycleTool' else 'tests/capella')),*flags[1:]];source=v.R/rel
  need(cmake.count('add_executable('+target+' '+rel+')')==1 and cmake.count('target_link_libraries('+target+' PRIVATE nano_core)')==1,'current selected CMake target')
  if role=='lifecycleTool':need([x for x in flags if x.startswith('-DNANO_TEST_SOURCE=')]==['-DNANO_TEST_SOURCE="'+str(v.S)+'"'],'original Tool macro')
  wanted=native2['dependencies'][target];need(isinstance(wanted,list) and all(isinstance(x,str) for x in wanted),'retained actual-MD set')
  for name in wanted:current_project(name)
  current_project(source);raw,err=m.run('M-'+role,flags+['-M','-MT','current-'+role,'-o','/dev/stdout',str(source)]);actual=m.deps(raw);need(not err,'current M stderr')
  current_normal=normalized_current(actual);old_normal=normalized_current(wanted);need(current_normal==old_normal,'normalized current/retained dependency sets differ')
  for name in actual:current_project(name)
  m.stable(actual,role+'-dependencies')
  leaf='deposit_audit.cpp.o' if role=='lifecycleTool' else 'lifecycle_submit.cpp.o';obj=E/'fresh-native-two-consumers-root-20261016/native-02'/leaf;v.merge(m.AUTH,{str(obj):products['objectMaterials'][target]});v.old_object(obj)
  prefix,tail=v.link_recipe('nano-capella-duty-availability-tests' if role=='lifecycleTool' else 'nano-capella-isolated-bootstrap-audit',template_obj,core)
  exe=O/target;m.run('LINK-'+role,prefix+[str(obj),'-o',str(exe)]+tail);m.admit_products([exe],role,True);bindings[role]=v.loader_bindings(role,exe);v.arity(role,exe)
  need(not list((O/'tmp').iterdir()) and not list((O/'probe-home').iterdir()),'CLI no HOME/tmp writes');qualified[role]=dict(path=str(exe),**pin(m.material(exe)[1]));stats[role]={'reusedObject':True,'rawMdIdentityRecovered':False,'literalDependencySetEqual':actual==sorted(set(wanted)),'literalCurrentCount':len(actual),'literalRetainedCount':len(set(wanted)),'normalizedCurrentCount':len(current_normal),'normalizedRetainedCount':len(old_normal),'normalizedDependencySetEqual':True,'currentNormalizationResolutionVerified':True}
 m.fence();need(v.git('rev-parse','HEAD').decode().strip()==a.revision and v.git('status','--porcelain')==b'','source remained clean')
 m.save('software-admission.private.json',{'format':'nano-current-native-runtime-relink-continuation-binding-v1','sourceRevision':a.revision,'episode':dict(path=str(pathlib.Path(a.episode)),**pin(ebody)),'priorProductObservations':proofs,'priorProducts':retained,'files':m.HELD,'aliases':m.ALIASES,'products':m.PRODUCTS,'programs':qualified,'newProgramStats':stats,'directLoaderBindings':bindings,'core':dict(path=str(core),**pin(m.material(core)[1])),'oldAuthoritiesRefreshed':False})
 result={'format':'nano-current-native-runtime-relink-continuation-v1','passed':True,'sourceRevision':a.revision,'firstFailedEpisodePreserved':True,'episode':pin(ebody),'priorCompletedProgramsReusedWithoutExecution':5,'priorCompletedIndependentMCommands':6,'priorCompiledTranslationUnits':1,'priorCurrentCoreLinks':5,'priorSmokeCliCount':6,'newIndependentMCommands':2,'newCompiledTranslationUnits':0,'newCurrentCoreLinks':2,'newSmokeCliCount':2,'aggregateSuccessfulIndependentMCommands':8,'aggregateCompiledTranslationUnits':1,'aggregateCurrentCoreLinks':7,'aggregateSmokeCliCount':8,'priorRefusedToolMNotCountedAsSuccessful':True,'coreArchiveMembers':143,'otherMembersUnchanged':142,'runtimeMemberMatchesRetainedCompiledObject':True,'core':pin(m.material(core)[1]),'programs':qualified,'newProgramStats':stats,'softwareAdmission':pin(m.material(O/'software-admission.private.json')[1]),'steps':m.STEPS,'p2pCommands':0,'oldTestsReplayed':0,'runtimeCxxReplayed':False,'previousFiveLinksReplayed':False,'previousSixClisReplayed':False,'wholeSdkQualified':False,'fullCmakeConfigureExecuted':False,'historicalRawMdIdentityRecovered':False,'recursiveDylibClosureRequalified':False,'runtimeStarted':False,'dutiesResumed':False,'all24CampaignAdmissions':False,'newSeedOrLedgerCreated':False,'signaturesCreated':0,'productionQualified':False,'oldAuthoritiesRefreshed':False,'elapsedMs':int((time.monotonic()-m.START)*1000)}
 m.save('result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)
if __name__=='__main__':
 first=None
 try:main()
 except BaseException as error:first=error
 finally:
  if created and m is not None:
   for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP):signal.signal(sig,signal.SIG_IGN)
   try:v.final_fence();m.save('final-fence-public.json',{'passed':True,'firstFailurePreserved':first is not None,'oldAuthoritiesRefreshed':False})
   except BaseException as error:
    if first is None:first=error
    try:m.save('final-fence-failure-public.json',{'passed':False,'firstFailurePreserved':True})
    except BaseException:pass
   if first is not None:
    try:m.save('refusal.private.json',{'passed':False,'errorType':type(first).__name__,'errorDetailsSuppressed':True,'oldAuthoritiesRefreshed':False})
    except BaseException:pass
 if first is not None:sys.stderr.write('Native continuation refused; details suppressed.\n');sys.exit(1)
