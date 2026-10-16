"""Root: replace only Native RPC member, qualify new context and relink consumers."""
import pathlib,json,hashlib,os,io,tarfile,types,signal,time,subprocess,stat
E=pathlib.Path(__file__).resolve().parent;R=pathlib.Path('/Users/boris/Dev/blockchain-nano');O=E/'rpc-runtime-native-root-qualification-20261017-01';REV='d26e4744416d033dad5a2519cc2ab8899b838faf';BASE='68d64a5330d16c971dd603b56e0f14c88c1459c8'
assert not O.exists();v=m=c=None;created=False;imports={}
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def module(path,sha,name):
 fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW);fields=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
 with os.fdopen(fd,'rb') as f:
  s=os.fstat(f.fileno());row={k:getattr(s,k) for k in fields};assert stat.S_ISREG(s.st_mode) and s.st_nlink==1 and s.st_size<=131072;b=f.read();assert row=={k:getattr(os.fstat(f.fileno()),k) for k in fields}=={k:getattr(path.lstat(),k) for k in fields} and hashlib.sha256(b).hexdigest()==sha
 imports[str(path)]=dict(row,sha256=sha);z=types.ModuleType(name);z.__file__=str(path);exec(compile(b,str(path),'exec'),z.__dict__);return z
def need(ok,why):
 if not ok:raise RuntimeError(why)
def main():
 global v,m,c,created
 need(os.getuid()==501 and os.getgid()==20 and os.uname().sysname=='Darwin' and os.uname().machine=='arm64','Native owner/architecture')
 v=module(E/'current-native-runtime-relink-source-20261016-v2/qualify-native-runtime-relink-root.py','7ef818ba1c8c3f210ed2fcc6236438322e5492e7c4a9b617f66c8ca05125b293','old_runtime_functions')
 m=module(v.HP,v.HELPER['sha256'],'native_fd_command_functions');cp=E/'current-native-runtime-relink-continuation-source-20261016/host.py';c=module(cp,'200aa37cfc8c6217b906b3691cdaf274074cf5c0d17b0fcc3d1ce6d8d69bd390','old_normalization_functions')
 O.mkdir(mode=0o700);created=True;(O/'tmp').mkdir();(O/'probe-home').mkdir();v.O=c.O=m.O=O;v.m=c.m=m;c.v=v;m.START=time.monotonic();m.DEADLINE=m.START+1600
 signal.signal(signal.SIGHUP,m.stop)
 m.ENV={'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','HOME':str(O/'probe-home'),'TMPDIR':str(O/'tmp'),'LANG':'C','LC_ALL':'C','DEVELOPER_DIR':'/Library/Developer/CommandLineTools'}
 for p in [v.HP,pathlib.Path(v.__file__),cp,pathlib.Path(__file__).resolve()]:
  row,b=m.material(p);need(str(p) not in imports or imports[str(p)]==row,'imported control unchanged');m.HELD[str(p)]=row
 need(v.git('rev-parse','HEAD').decode().strip()==REV and not v.git('status','--porcelain'),'clean current revision')
 need(v.git('diff','--name-only',BASE,REV,'--','src','include','network','third_party').decode().splitlines()==['src/rpc.cpp'],'only RPC changed since retained Native Core')
 current=v.descriptor('native-rpc-and-nano-root-qualification-attempt02-20261016/input-post-fence.private.json','77ee8eb21a4be7accf6a070cd11ed9534d69787a993b49cca90669ba8a6a1fc5',1034934)
 exporter=v.descriptor('unsigned-preview-native-root-qualification-attempt02-20261016/input-post-fence.private.json','55d81e9f0f93fd0cbbde9a9e6fef5706a731dd05706ada05221af94675071e3b',1910878)
 helpers=v.descriptor('current-native-five-helpers-root-attempt01-20261016/software-admission.private.json','04b06bf5374ed28d7accf6549935993b7e3dccc3333e5c12776b04f654f4f64a',1037791)
 native2=v.descriptor('fresh-native-two-consumers-root-20261016/native-02/admitted.private.json','7eceff399d56849e2ef8107664b316455fdd96833bebb41b7c7a804abbab6735',2874988)
 legacy=v.descriptor('fresh-native-legacy-root-20261016/native-01/admitted.private.json','8bcd2f07082a5a36036e577702c444f4a36e9902923b07b5b96d7bedfcebb977',3207851)
 old_products=v.descriptor('fresh-native-two-consumers-root-20261016/native-02/new-software-admission.private.json','f8cf61dd322d92dbfd0d1d319d4e3f3ebcc1f70cb8beaff222d1d47efbc66f19',3223)
 legacy_products=v.descriptor('fresh-native-legacy-root-20261016/native-01/new-software-admission.private.json','abc520a6155bc432dd63c095837d465a75d255a3bf11d62cfedef9a179c89ae5',4957)
 for value in (current,exporter,helpers):
  for key in ('files','productMaterials','products'):v.merge(m.AUTH,value.get(key,{}))
  v.merge(m.AA,value.get('aliases',{}))
 for value in (native2,legacy):v.merge(m.AUTH,value['heldMaterials']);v.merge(m.AA,value['aliasGuards'])
 for role,leaf in [('nano-capella-deposit-audit','deposit_audit.cpp.o'),('nano-capella-lifecycle-submit','lifecycle_submit.cpp.o')]:v.merge(m.AUTH,{str(E/'fresh-native-two-consumers-root-20261016/native-02'/leaf):old_products['objectMaterials'][role]})
 prior=E/'current-native-runtime-relink-root-20261016-02';retained={};proofs={x:c.observations(prior,x,retained) for x in ['current-runtime-core','runtime-object']};core=prior/'libnano_core.a';corebytes=m.material(core)[1];need(pin(corebytes)=={'sha256':'0808fa1320026d07c020ff1d1dfd3d4cd4a97ab2efa445e00caf8954cb0395b2','bytes':29263216},'retained runtime Core bytes');original=m.archive(corebytes)
 runtime=[x for x in original if x[0]=='consensus_capella_runtime.cpp.o'];need(runtime==[('consensus_capella_runtime.cpp.o',retained[str(prior/'consensus_capella_runtime.cpp.o')]['st_size'],retained[str(prior/'consensus_capella_runtime.cpp.o')]['sha256'])],'retained runtime member')
 trees=['src','include','network','third_party','CMakeLists.txt','tests/support','tests/capella','tests/multihost','tests/test_capella_transition.cpp','tests/capella_cold_memo_sync.hpp','tests/test_capella_duty_availability.cpp','tests/test_rpc_simulation_context.cpp'];project={};source=O/'source';source.mkdir()
 with tarfile.open(fileobj=io.BytesIO(v.git('archive',REV,*trees))) as tf:
  for entry in tf:
   rel=pathlib.PurePosixPath(entry.name);need(not rel.is_absolute() and '..' not in rel.parts and (entry.isdir() or entry.isfile()),'public Git projection entry')
   if entry.isfile():
    need(entry.size<=16777216,'source size');b=tf.extractfile(entry).read();p=source/entry.name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b);p.chmod(0o400);project[entry.name]=pin(b);m.EXPECTED[str(p)]=pin(b)
 need(project['src/rpc.cpp']=={'sha256':'4dfa7e0844c9ff6da2433d6b9ab3aa2c12ea06fa1a33ec11b06f9dcc79a1dc6f','bytes':81453},'RPC source pin')
 need(project['tests/test_rpc_simulation_context.cpp']=={'sha256':'1790fd79cca93cc807ef8539e1138d06eeacd53a8760cbc5be95ac30a0377b6c','bytes':18497},'new test source pin')
 mainroot=E/'native-rpc-and-nano-root-qualification-attempt02-20261016/source';exportroot=E/'unsigned-preview-native-root-qualification-attempt02-20261016/source'
 for root in (R,v.S,mainroot,exportroot):
  for rel,value in project.items():m.EXPECTED[str(root/rel)]=value
 original_candidate=m.candidate
 def selected(logical):
  physical=str(pathlib.Path(logical).resolve(strict=True));need(physical in m.AUTH or physical in m.EXPECTED or physical in m.PRODUCTS or physical in m.NEW,'missing selected original authority');return original_candidate(logical)
 m.candidate=selected
 db=v.B/'compile_commands.json';database=json.loads(m.hold(db,m.AUTH[str(db)]));m.stable(['/usr/bin/c++','/usr/bin/ar','/usr/bin/ranlib','/Library/Developer/CommandLineTools/usr/bin/clang','/Library/Developer/CommandLineTools/usr/bin/ld','/Library/Developer/CommandLineTools/usr/bin/ar','/Library/Developer/CommandLineTools/usr/bin/ranlib'],'selected-tools')
 rpcflags,_=v.recipe(database,'src/rpc.cpp');testflags,test_template=v.recipe(database,'tests/capella/isolated_bootstrap_audit.cpp')
 m.candidate=original_candidate
 try:
  rpcobj,rpcstats=m.compile_one('rpc',rpcflags,source/'src/rpc.cpp');m.admit_products([rpcobj,O/'rpc.cpp.o.d'],'rpc-object')
 finally:m.candidate=selected
 newcore=O/'libnano_core.a';newcore.write_bytes(corebytes);m.run('replace-rpc',['/usr/bin/ar','r',str(newcore),str(rpcobj)]);m.run('index-core',['/usr/bin/ranlib',str(newcore)]);new=m.archive(m.material(newcore)[1]);need([x[0] for x in new]==[x[0] for x in original] and [x for x in new if x[0]!='rpc.cpp.o']==[x for x in original if x[0]!='rpc.cpp.o'],'143/142 member continuity');need([x for x in new if x[0]=='rpc.cpp.o']==[('rpc.cpp.o',rpcobj.stat().st_size,pin(m.material(rpcobj)[1])['sha256'])],'RPC member equals new object');newcore.chmod(0o400);m.admit_products([newcore],'rpc-core')
 m.candidate=original_candidate
 try:testobj,teststats=m.compile_one('rpc-context',testflags,source/'tests/test_rpc_simulation_context.cpp');m.admit_products([testobj,O/'rpc-context.cpp.o.d'],'rpc-context-object')
 finally:m.candidate=selected
 prefix,tail=v.link_recipe('nano-capella-isolated-bootstrap-audit',test_template,newcore);testexe=O/'nano-rpc-simulation-context-tests';m.run('LINK-rpc-context',prefix+[str(testobj),'-o',str(testexe)]+tail);m.admit_products([testexe],'rpc-context-test',True);loaders={'rpcContext':v.loader_bindings('rpcContext',testexe)}
 cmake=m.hold(R/'CMakeLists.txt').decode();need(cmake.count('add_executable(nano-rpc-simulation-context-tests tests/test_rpc_simulation_context.cpp)')==1 and cmake.count('target_link_libraries(nano-rpc-simulation-context-tests PRIVATE nano_core)')==1,'current new test declaration')
 raw,err=m.run('CONTEXT-all',[str(testexe),'--simulation-context-self-test'],seconds=90);need(not err,'test stderr');events=[json.loads(x) for x in raw.splitlines()];need(len(events)==5 and events[-1]['passed'] is True and events[-1]['checks']==476 and events[-1]['expectedRefusals']==126 and sum(len(x['caseRecords']) for x in events[:4])==258,'new context checks');canonical=(json.dumps(events,sort_keys=True,separators=(',',':'),ensure_ascii=False)+'\n').encode();need(hashlib.sha256(canonical).hexdigest()=='96ca4c5586e5f53c3417fc5642c95f908036e5e179ad81b0865c423c9d4806cb','event bytes match qualified Linux records');(O/'candidate-events.json').write_bytes(canonical)
 roles=[('nano','nano','src/main.cpp','src/main.cpp'),('exporter','nano-capella-transition-tests','tests/test_capella_transition.cpp','tests/test_capella_transition.cpp'),('bootstrapImport','nano-capella-isolated-bootstrap-import','tests/capella/isolated_bootstrap_import.cpp','tests/capella/isolated_bootstrap_import.cpp'),('bootstrapAudit','nano-capella-isolated-bootstrap-audit','tests/capella/isolated_bootstrap_audit.cpp','tests/capella/isolated_bootstrap_audit.cpp'),('stoppedCapture','nano-capella-isolated-stopped-capture','tests/capella/isolated_stopped_capture.cpp','tests/capella/isolated_stopped_capture.cpp'),('lifecycleTool','nano-capella-deposit-audit','tests/capella/deposit_audit.cpp','tests/test_capella_duty_availability.cpp'),('lifecycleSubmit','nano-capella-lifecycle-submit','tests/multihost/lifecycle_submit.cpp','tests/capella/isolated_bootstrap_audit.cpp')];programs={};stats={}
 for role,target,rel,template in roles:
  flags,template_obj=v.recipe(database,template);src=R/rel;wanted=None;md=None;need(cmake.count('add_executable('+target+' '+rel+')')==1 and cmake.count('target_link_libraries('+target+' PRIVATE nano_core'+(' nano_ckzg' if role=='exporter' else '')+')')==1,'current consumer CMake')
  if role=='nano':obj=E/'native-rpc-and-nano-root-qualification-attempt02-20261016/main.cpp.o';md=obj.with_suffix(obj.suffix+'.d');src=mainroot/rel;flags=[x.replace(str(v.S),str(mainroot)) for x in flags]
  elif role=='exporter':
   obj=exportroot.parent/'capella-exporter.cpp.o';md=obj.with_suffix(obj.suffix+'.d');src=exportroot/rel;flags=[x.replace(str(v.S),str(exportroot)) for x in flags];macro='-DNANO_TEST_SOURCE="'+str(exportroot)+'"';need(flags.count(macro)==1,'Exporter macro');flags[flags.index(macro)]='-DNANO_TEST_SOURCE="'+str(R)+'"'
  elif role=='bootstrapImport':obj=template_obj;v.merge(m.AUTH,{str(obj):legacy_products['originalConsumerObjectMaterials'][target]});src=v.S/rel;wanted=legacy['dependencies'][target]
  elif role in ('bootstrapAudit','stoppedCapture'):obj=E/'current-native-five-helpers-root-attempt01-20261016'/('helper-'+('1' if role=='bootstrapAudit' else '2')+'.cpp.o');md=obj.with_suffix(obj.suffix+'.d')
  else:
   obj=E/'fresh-native-two-consumers-root-20261016/native-02'/('deposit_audit.cpp.o' if role=='lifecycleTool' else 'lifecycle_submit.cpp.o');wanted=native2['dependencies'][target];flags=[flags[0],'-iquote'+str(v.S/('tests' if role=='lifecycleTool' else 'tests/capella')),*flags[1:]]
  v.old_object(obj)
  if md is not None:md=v.old_object(md);wanted=m.deps(m.hold(md,m.AUTH[str(md)]))
  need(isinstance(wanted,list),'retained MD set');raw,err=m.run('M-'+role,flags+['-M','-MT','current-'+role,'-o','/dev/stdout',str(src)]);actual=m.deps(raw);need(not err and c.normalized_current(actual)==c.normalized_current(wanted),'M differs from normalized retained MD');m.stable(actual,role+'-dependencies')
  prefix,tail=v.link_recipe('nano-capella-duty-availability-tests' if role=='lifecycleTool' else 'nano-capella-isolated-bootstrap-audit' if role=='lifecycleSubmit' else target,template_obj,newcore);exe=O/target;m.run('LINK-'+role,prefix+[str(obj),'-o',str(exe)]+tail);m.admit_products([exe],role,True);loaders[role]=v.loader_bindings(role,exe)
  if role=='nano':
   raw,err=m.run('CLI-nano-help',[str(exe),'--help'],seconds=15);need(not err and raw.startswith(b'Nano local chain tools\nUsage: nano <init|status|audit|mine|validate|import|serve|run> <genesis.json> <database-directory> [arguments]\n'),'Nano help');raw,err=m.run('CLI-nano-rpc-arity',[str(exe),'rpc'],1,seconds=15);need(not raw and err==b'Nano: rpc <URL> <info|nodes|network|balance|send|deploy|receipt|mine> [arguments]\n','Nano rpc arity')
  else:v.arity(role,exe)
  need(not list((O/'tmp').iterdir()) and not list((O/'probe-home').iterdir()),'safe CLI/tmp cleanliness');programs[role]=dict(path=str(exe),**pin(m.material(exe)[1]));stats[role]={'objectReused':True,'independentMEqualsNormalizedRetainedMd':True,'literalMEqualsRetainedMd':actual==sorted(set(wanted)),'rawMdIdentityRecovered':md is not None,'normalizationResolutionVerified':True,'dependencies':len(c.normalized_current(actual))}
 m.fence();need(v.git('rev-parse','HEAD').decode().strip()==REV and not v.git('status','--porcelain'),'source remained clean')
 m.save('software-admission.private.json',{'format':'nano-native-rpc-runtime-software-v1','sourceRevision':REV,'files':m.HELD,'aliases':m.ALIASES,'products':m.PRODUCTS,'programs':programs,'newCore':dict(path=str(newcore),**pin(m.material(newcore)[1])),'corePriorObservations':proofs,'newCompileStats':{'rpc':rpcstats,'context':teststats},'reuseStats':stats,'directLoaderBindings':loaders,'steps':m.STEPS,'oldAuthoritiesRefreshed':False})
 m.save('result-public.json',{'format':'nano-native-rpc-runtime-qualification-v1','passed':True,'sourceRevision':REV,'compiledTranslationUnits':2,'replacedCoreMembers':['rpc.cpp.o'],'coreMembers':143,'unchangedCoreMembers':142,'runtimeMemberUnchanged':True,'newContextChecks':476,'newContextExpectedRefusals':126,'newContextCases':258,'canonicalEventsSHA256':hashlib.sha256(canonical).hexdigest(),'independentMCommands':9,'links':8,'runtimeConsumerLinks':7,'cliSmokes':8,'oldTestSuitesReplayed':0,'p2pCommands':0,'imageBuilds':0,'newConsensusKeys':0,'transactionOrConsensusSignaturesCreated':0,'syntheticTestDatabasesUsed':True,'privateDatabaseOpened':False,'runtimeStarted':False,'dutiesResumed':False,'campaignAdmission':False,'recursiveDylibClosureRequalified':False,'wholeSdkQualified':False,'fullCmakeConfigureExecuted':False,'oldAuthoritiesRefreshed':False,'newCore':pin(m.material(newcore)[1]),'programs':programs,'softwareAdmission':pin(m.material(O/'software-admission.private.json')[1]),'steps':m.STEPS,'productionQualified':False,'elapsedMs':round((time.monotonic()-m.START)*1000)})
 print('Native RPC context and seven runtime links qualified',flush=True)
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
   try:m.save('failure-public.json',{'passed':False,'errorType':type(first).__name__,'message':str(first),'productionQualified':False})
   except BaseException:pass
if first is not None:raise first
