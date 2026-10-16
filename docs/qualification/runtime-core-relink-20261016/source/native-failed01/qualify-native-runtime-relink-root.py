#!/usr/bin/env python3
"""Root-only selected runtime-member replacement and seven Native relinks."""
import argparse,hashlib,io,json,os,pathlib,re,shlex,signal,stat,struct,subprocess,sys,tarfile,time,types

E=pathlib.Path(__file__).resolve().parents[1]
R=pathlib.Path('/Users/boris/Dev/blockchain-nano')
B=E/'historical-beacon-rpc-20261015/native-build'
S=E/'historical-beacon-rpc-20261015/source'
BASE='bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d'
OLD_CORE={'sha256':'e04c8c3f383963935cc60d2a658043e663e0393c0a6132ca304f41ba71db96fa','bytes':29262512}
RUNTIME={'sha256':'57f18bb906274d62a8fc93f86059d0b28218660fc96129966dec9329c9761232','bytes':28682}
SUPPORT={'sha256':'8d8ddc5412a8ad0223ddbb859b43a4c9b5b071b0868a91cba67f05bf9a794a67','bytes':34336}
HP=E/'qualify-native-rpc-and-nano-root-20261016.py'
HELPER={'sha256':'3c62250aeff218a9809b30d578ea7eaabfaecff26b984656aedf375919156414','bytes':17840}
O=None;m=None;created=False

def need(ok,message):
 if not ok:raise RuntimeError(message)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def merge(dst,src):
 need(isinstance(src,dict),'authority map')
 for p,row in src.items():need(p not in dst or dst[p]==row,'conflicting retained authority');dst[p]=row
def descriptor(rel,sha,size):
 p=E/rel;row,raw=m.material(p);need(pin(raw)=={'sha256':sha,'bytes':size},'retained descriptor pin');m.HELD[str(p)]=row;return json.loads(raw)
def git(*args):
 env={'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','HOME':str(O/'probe-home'),'LANG':'C','LC_ALL':'C','GIT_OPTIONAL_LOCKS':'0','GIT_NO_REPLACE_OBJECTS':'1','GIT_NO_LAZY_FETCH':'1'}
 q=subprocess.run(['/usr/bin/git',*args],cwd=R,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
 need(q.returncode==0 and len(q.stdout)<=134217728 and len(q.stderr)<=1048576,'bounded read-only Git');return q.stdout
def recipe(database,rel):
 rows=[z for z in database if z['file']==str(S/rel)];need(len(rows)==1,'one canonical compile recipe');argv=shlex.split(rows[0]['command'])
 need(len(argv)>4 and argv[-4]=='-o' and argv[-2:]==['-c',str(S/rel)],'canonical compile suffix')
 return argv[:-4],B/argv[-3]
def old_object(path):
 path=pathlib.Path(path);need(str(path) in m.AUTH,'original object or MD authority missing');m.hold(path,m.AUTH[str(path)]);return path
def md_reuse(name,flags,source,obj,md=None,retained_set=None):
 obj=old_object(obj)
 if md is not None:md=old_object(md);wanted=m.deps(m.hold(md,m.AUTH[str(md)]))
 else:
  need(isinstance(retained_set,list) and all(isinstance(p,str) for p in retained_set),'retained actual-MD set');wanted=sorted(set(retained_set))
 raw,err=m.run('M-'+name,flags+['-M','-MT','current-'+name,'-o','/dev/stdout',str(source)])
 actual=m.deps(raw);need(not err and actual==wanted,'current M differs from retained actual-MD set');m.stable(actual,name+'-dependencies')
 return obj,{'reusedObject':True,'dependencies':len(actual),'MEqualsRetainedActualMdSet':True,'rawMdIdentityRecovered':md is not None,'object':dict(path=str(obj),**pin(m.hold(obj,m.AUTH[str(obj)])))}
def link_recipe(template,original_obj,new_core):
 path=B/'CMakeFiles'/(template+'.dir')/'link.txt';need(str(path) in m.AUTH,'original link recipe authority missing')
 argv=shlex.split(m.hold(path,m.AUTH[str(path)]).decode());need(argv.count('-o')==1,'one canonical output')
 at=argv.index('-o');need(at>1 and argv[at+1]==template,'canonical target output')
 word=argv[at-1];need(word.endswith('.cpp.o'),'one adjacent own object')
 physical=pathlib.Path(word) if word.startswith('/') else B/word
 need(physical==original_obj,'canonical target own-object binding');tail=argv[at+2:];need(tail.count('libnano_core.a')==1,'one exact canonical Core')
 out=[]
 for word in tail:
  if word=='libnano_core.a':out.append(str(new_core))
  elif word.startswith('-'):out.append(word)
  else:
   p=pathlib.Path(word) if word.startswith('/') else B/word
   need(str(p.resolve(strict=True)) in m.AUTH,'original archive/ASM/dylib authority missing');m.hold(p,m.AUTH[str(p.resolve(strict=True))]);out.append(str(p))
 need(sum(v.endswith('.a') for v in out)==5 and sum(v.endswith('.o') for v in out)==2,'canonical five archives and two ASM inputs')
 return argv[:at-1],out
def macho(raw):
 need(len(raw)>=32 and struct.unpack_from('<I',raw)[0]==0xfeedfacf and struct.unpack_from('<I',raw,4)[0]==0x0100000c and struct.unpack_from('<I',raw,12)[0]==2,'thin ARM64 executable')
 count,total=struct.unpack_from('<II',raw,16);need(count<512 and 32+total<=len(raw),'Mach-O commands bound');at=32;loads=[];rpaths=[]
 for _ in range(count):
  command,size=struct.unpack_from('<II',raw,at);need(size>=8 and at+size<=32+total,'Mach-O command bounds');need(command not in (0x80000018,0x8000001f,0x80000023,0x20),'unsupported dylib command')
  if command in (0xc,0x8000001c):
   need(size>=12,'Mach-O name offset');off=struct.unpack_from('<I',raw,at+8)[0];need(8<=off<size,'Mach-O name bound');value=raw[at+off:at+size].split(b'\0',1)[0].decode();(rpaths if command==0x8000001c else loads).append(value)
  at+=size
 need(at==32+total,'Mach-O command span');return {'loads':loads,'rpaths':rpaths}
def loader_bindings(role,exe):
 parsed=macho(m.material(exe)[1]);resolved={};logical=[];shared=[]
 support=[v for v in parsed['loads'] if 'nano-test-sync-fault' in v]
 need(support==(['@rpath/libnano-test-sync-fault.dylib'] if role=='exporter' else []),'Exporter-only original sync-fault binding')
 for value in parsed['loads']:
  if value.startswith(('/usr/lib/','/System/Library/')):shared.append(value);continue
  if value.startswith('@rpath/'):
   choices=[]
   for rp in parsed['rpaths']:
    root=rp.replace('@loader_path',str(exe.parent)).replace('@executable_path',str(exe.parent));q=pathlib.Path(root)/value[len('@rpath/'):]
    if q.exists():choices.append(q)
   need(choices,'original rpath resolves');q=choices[0]
  else:q=pathlib.Path(value.replace('@loader_path',str(exe.parent)).replace('@executable_path',str(exe.parent)))
  physical=str(q.resolve(strict=True));need(physical in m.AUTH,'original direct dylib physical authority');raw=m.hold(q,m.AUTH[physical]);resolved[value]=physical;logical.append(q)
  if 'nano-test-sync-fault' in value:need(pin(raw)==SUPPORT,'original support dylib bytes')
 if logical:m.stable(logical,role+'-direct-loader-bindings')
 return {'commands':parsed,'resolvedDirectNonSystemLoads':resolved,'sharedCacheLoadsNotRequalified':shared,'recursiveDylibClosureRequalified':False}
def arity(role,exe):
 args=[str(exe)]+(['--preview-isolated-capella-identities'] if role=='exporter' else [])
 raw,err=m.run('CLI-'+role,args,1,seconds=15)
 if role=='exporter':need(not raw and err==b'identity preview requires only a seed file\n','Exporter safe selector arity')
 elif role=='lifecycleTool':need(not raw and err==b'usage: --audit synthetic-genesis node-config stopped-db external-audit-request deposit-metadata\n','Tool arity')
 elif role=='lifecycleSubmit':need(not raw and err==b'{"event":"fresh-lifecycle-child-submission-refused","accepted":false,"detailsExported":false,"productionQualified":false}\n','Submit arity')
 else:
  expected={
   'bootstrapImport':{'format':'nano-isolated-fixture-private-binding-audit-v1','passed':False,'refusedStage':'arguments','errorDetailsSuppressed':True,'newSignaturesCreated':0,'signerCallbacksInvoked':0,'runtimeStarted':False,'networkLaunched':False,'productionQualified':False},
   'bootstrapAudit':{'format':'nano-isolated-bootstrap-physical-audit-v1','passed':False,'refusedStage':'arguments','diagnostic':'expected manifest plan anchors source-inventory owner dataRoot witnessRoot','newSignaturesCreated':0,'journalWritesRequested':False,'runtimeStarted':False,'productionQualified':False},
   'stoppedCapture':{'format':'nano-isolated-stopped-recovery-pins-v1','passed':False,'refusedStage':'arguments','diagnostic':'usage: --capture public-manifest public-runtime-config external-stop-receipt dataOwnerRoot witnessOwnerRoot owner','newSignaturesCreated':0,'journalWritesRequested':False,'runtimeStarted':False,'productionQualified':False}}
  need(not err and json.loads(raw)==expected[role],'exact helper arity')
def final_fence():
 for p,row in m.HELD.items():need(m.material(p)[0]==row,'final held nine/SHA changed')
 for p,row in m.ALIASES.items():need(m.alias(p)==row,'final held alias changed')
 for p,row in m.PRODUCTS.items():need(m.material(p)[0]==row,'final product nine/SHA changed')
 for p,value in m.NEW.items():need(pin(m.material(p)[1])==value,'final new bytes changed')

def main():
 global O,m,created
 parser=argparse.ArgumentParser();parser.add_argument('--revision',required=True);parser.add_argument('--output-root',required=True);args=parser.parse_args()
 need(re.fullmatch('[0-9a-f]{40}',args.revision) is not None,'exact current revision')
 candidate=pathlib.Path(args.output_root);need(candidate.is_absolute() and candidate.parent==E and re.fullmatch('current-native-runtime-relink-root-20261016-[a-z0-9-]{1,40}',candidate.name) is not None and not candidate.exists() and not candidate.is_symlink(),'new bounded output namespace')
 need(os.getuid()==501 and os.getgid()==20 and os.uname().sysname=='Darwin' and os.uname().machine=='arm64','Native owner/platform')
 # Import only the exact public definitions; the old module main is never invoked.
 raw=HP.read_bytes();need(pin(raw)==HELPER,'frozen public helper source');m=types.ModuleType('native_runtime_relink_helpers');m.__file__=str(HP);exec(compile(raw,str(HP),'exec'),m.__dict__)
 O=candidate;m.O=O;m.START=time.monotonic();m.DEADLINE=m.START+1100
 signal.signal(signal.SIGHUP,m.stop)
 hr,hb=m.material(HP);need(pin(hb)==HELPER,'frozen helper FD bytes');m.HELD[str(HP)]=hr
 O.mkdir(mode=0o700);created=True;(O/'tmp').mkdir(mode=0o700);(O/'probe-home').mkdir(mode=0o700)
 m.ENV={'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','HOME':str(O/'probe-home'),'TMPDIR':str(O/'tmp'),'LANG':'C','LC_ALL':'C','DEVELOPER_DIR':'/Library/Developer/CommandLineTools'}
 need(git('rev-parse','HEAD').decode().strip()==args.revision and git('status','--porcelain')==b'','clean current revision')
 need(git('diff','--name-only',BASE,args.revision,'--','src','include','third_party','network').decode().splitlines()==['src/consensus_capella_runtime.cpp'],'only selected product source changed since qualified base')
 current=descriptor('native-rpc-and-nano-root-qualification-attempt02-20261016/input-post-fence.private.json','77ee8eb21a4be7accf6a070cd11ed9534d69787a993b49cca90669ba8a6a1fc5',1034934)
 exporter=descriptor('unsigned-preview-native-root-qualification-attempt02-20261016/input-post-fence.private.json','55d81e9f0f93fd0cbbde9a9e6fef5706a731dd05706ada05221af94675071e3b',1910878)
 helpers=descriptor('current-native-five-helpers-root-attempt01-20261016/software-admission.private.json','04b06bf5374ed28d7accf6549935993b7e3dccc3333e5c12776b04f654f4f64a',1037791)
 native2=descriptor('fresh-native-two-consumers-root-20261016/native-02/admitted.private.json','7eceff399d56849e2ef8107664b316455fdd96833bebb41b7c7a804abbab6735',2874988)
 legacy=descriptor('fresh-native-legacy-root-20261016/native-01/admitted.private.json','8bcd2f07082a5a36036e577702c444f4a36e9902923b07b5b96d7bedfcebb977',3207851)
 products=descriptor('fresh-native-two-consumers-root-20261016/native-02/new-software-admission.private.json','f8cf61dd322d92dbfd0d1d319d4e3f3ebcc1f70cb8beaff222d1d47efbc66f19',3223)
 legacy_products=descriptor('fresh-native-legacy-root-20261016/native-01/new-software-admission.private.json','abc520a6155bc432dd63c095837d465a75d255a3bf11d62cfedef9a179c89ae5',4957)
 for value in (current,exporter,helpers):
  for key in ('files','productMaterials','products'):merge(m.AUTH,value.get(key,{}))
  merge(m.AA,value.get('aliases',{}))
 for value in (native2,legacy):merge(m.AUTH,value['heldMaterials']);merge(m.AA,value['aliasGuards'])
 # Add only producer-retained object rows; never derive an old row from current stat.
 merge(m.AUTH,{str(E/'fresh-native-two-consumers-root-20261016/native-02'/leaf):products['objectMaterials'][role] for role,leaf in [('nano-capella-deposit-audit','deposit_audit.cpp.o'),('nano-capella-lifecycle-submit','lifecycle_submit.cpp.o')]})
 # Public bytes are compared at each reached producer path to this exact Git tree.
 trees=['include','src','network','third_party','CMakeLists.txt','tests/support','tests/capella','tests/multihost','tests/test_capella_transition.cpp','tests/capella_cold_memo_sync.hpp','tests/test_capella_duty_availability.cpp']
 project={}
 with tarfile.open(fileobj=io.BytesIO(git('archive',args.revision,*trees))) as archive:
  for entry in archive:
   need(entry.isfile() or entry.isdir(),'regular Git projection');rel=pathlib.PurePosixPath(entry.name);need(not rel.is_absolute() and '..' not in rel.parts,'Git relative leaf')
   if entry.isfile():need(entry.size<=16777216,'source leaf bound');project[entry.name]=pin(archive.extractfile(entry).read())
 mainroot=E/'native-rpc-and-nano-root-qualification-attempt02-20261016/source';exportroot=E/'unsigned-preview-native-root-qualification-attempt02-20261016/source'
 for root in (R,S,mainroot,exportroot):
  for rel,value in project.items():m.EXPECTED[str(root/rel)]=value
 need(project['src/consensus_capella_runtime.cpp']==RUNTIME,'reviewed runtime source exact')
 original_candidate=m.candidate
 def selected_candidate(logical):
  physical=str(pathlib.Path(logical).resolve(strict=True));need(physical in m.AUTH or physical in m.EXPECTED or physical in m.PRODUCTS or physical in m.NEW,'missing selected original authority; no SDK fallback');return original_candidate(logical)
 m.candidate=selected_candidate
 core=E/'native-rpc-and-nano-root-qualification-attempt02-20261016/libnano_core.a';need(pin(m.hold(core,m.AUTH[str(core)]))==OLD_CORE,'retained current Core exact');original=m.archive(m.hold(core))
 dbpath=B/'compile_commands.json';need(str(dbpath) in m.AUTH,'retained canonical DB authority');database=json.loads(m.hold(dbpath,m.AUTH[str(dbpath)]))
 runtime_flags,_=recipe(database,'src/consensus_capella_runtime.cpp')
 tools=['/usr/bin/c++','/usr/bin/ar','/usr/bin/ranlib','/Library/Developer/CommandLineTools/usr/bin/clang','/Library/Developer/CommandLineTools/usr/bin/ld','/Library/Developer/CommandLineTools/usr/bin/ar','/Library/Developer/CommandLineTools/usr/bin/ranlib']
 m.stable(tools,'selected-tools')
 cmake=m.hold(R/'CMakeLists.txt').decode()
 # The old R/runtime row remains historical. Compile a new known-byte projection,
 # never refresh or discard the original row merely because R has advanced.
 runtime_source=O/'source/src/consensus_capella_runtime.cpp';runtime_source.parent.mkdir(parents=True,mode=0o700)
 runtime_bytes=git('show',args.revision+':src/consensus_capella_runtime.cpp');need(pin(runtime_bytes)==RUNTIME,'new runtime projection known bytes');m.EXPECTED[str(runtime_source)]=RUNTIME
 with runtime_source.open('xb') as stream:stream.write(runtime_bytes);stream.flush();os.fsync(stream.fileno())
 os.chmod(runtime_source,0o400);directory=os.open(runtime_source.parent,os.O_RDONLY)
 try:os.fsync(directory)
 finally:os.close(directory)
 m.stable([runtime_source],'new-runtime-source')
 runtime_obj,runtime_stats=m.compile_one('consensus_capella_runtime',runtime_flags,runtime_source)
 m.admit_products([runtime_obj,O/'consensus_capella_runtime.cpp.o.d'],'runtime-object')
 newcore=O/'libnano_core.a'
 with newcore.open('xb') as stream:stream.write(m.hold(core));stream.flush();os.fsync(stream.fileno())
 m.run('replace-runtime',['/usr/bin/ar','r',str(newcore),str(runtime_obj)]);m.run('index-core',['/usr/bin/ranlib',str(newcore)])
 new_members=m.archive(m.material(newcore)[1]);member='consensus_capella_runtime.cpp.o'
 need([v[0] for v in original]==[v[0] for v in new_members],'Core member order unchanged')
 need([v for v in original if v[0]!=member]==[v for v in new_members if v[0]!=member],'other142 member payloads unchanged')
 row=m.material(runtime_obj)[0];need([v for v in new_members if v[0]==member]==[(member,row['st_size'],row['sha256'])],'replacement is exact compiled runtime member')
 os.chmod(newcore,0o400);m.admit_products([newcore],'current-runtime-core')
 roles=[('nano','nano','src/main.cpp','src/main.cpp'),('exporter','nano-capella-transition-tests','tests/test_capella_transition.cpp','tests/test_capella_transition.cpp'),('bootstrapImport','nano-capella-isolated-bootstrap-import','tests/capella/isolated_bootstrap_import.cpp','tests/capella/isolated_bootstrap_import.cpp'),('bootstrapAudit','nano-capella-isolated-bootstrap-audit','tests/capella/isolated_bootstrap_audit.cpp','tests/capella/isolated_bootstrap_audit.cpp'),('stoppedCapture','nano-capella-isolated-stopped-capture','tests/capella/isolated_stopped_capture.cpp','tests/capella/isolated_stopped_capture.cpp'),('lifecycleTool','nano-capella-deposit-audit','tests/capella/deposit_audit.cpp','tests/test_capella_duty_availability.cpp'),('lifecycleSubmit','nano-capella-lifecycle-submit','tests/multihost/lifecycle_submit.cpp','tests/capella/isolated_bootstrap_audit.cpp')]
 qualified={};statistics={};loaders={}
 for role,target,rel,template_rel in roles:
  flags,template_obj=recipe(database,template_rel);source=R/rel;md=None;dependency_set=None
  need(cmake.count('add_executable('+target+' '+rel+')')==1,'current canonical target source')
  declaration='target_link_libraries('+target+' PRIVATE nano_core'+(' nano_ckzg' if role=='exporter' else '')+')';need(cmake.count(declaration)==1,'current canonical target Core link')
  if role=='nano':obj=E/'native-rpc-and-nano-root-qualification-attempt02-20261016/main.cpp.o';md=obj.with_suffix(obj.suffix+'.d');source=mainroot/rel;flags=[v.replace(str(S),str(mainroot)) for v in flags]
  elif role=='exporter':
   obj=exportroot.parent/'capella-exporter.cpp.o';md=obj.with_suffix(obj.suffix+'.d');source=exportroot/rel;flags=[v.replace(str(S),str(exportroot)) for v in flags];oldmacro='-DNANO_TEST_SOURCE="'+str(exportroot)+'"';need(flags.count(oldmacro)==1,'Exporter fixture macro');flags[flags.index(oldmacro)]='-DNANO_TEST_SOURCE="'+str(R)+'"'
  elif role=='bootstrapImport':obj=template_obj;merge(m.AUTH,{str(obj):legacy_products['originalConsumerObjectMaterials'][target]});source=S/rel;dependency_set=legacy['dependencies'][target]
  elif role in ('bootstrapAudit','stoppedCapture'):
   index=1 if role=='bootstrapAudit' else 2;obj=E/'current-native-five-helpers-root-attempt01-20261016'/('helper-'+str(index)+'.cpp.o');md=obj.with_suffix(obj.suffix+'.d')
  else:
   leaf='deposit_audit.cpp.o' if role=='lifecycleTool' else 'lifecycle_submit.cpp.o';obj=E/'fresh-native-two-consumers-root-20261016/native-02'/leaf;dependency_set=native2['dependencies'][target];flags=[flags[0],'-iquote'+str(S/('tests' if role=='lifecycleTool' else 'tests/capella')),*flags[1:]]
  if role=='lifecycleTool':need([v for v in flags if v.startswith('-DNANO_TEST_SOURCE=')]==['-DNANO_TEST_SOURCE="'+str(S)+'"'],'Tool original fixture macro')
  obj,details=md_reuse(role,flags,source,obj,md,dependency_set)
  prefix,tail=link_recipe('nano-capella-duty-availability-tests' if role=='lifecycleTool' else 'nano-capella-isolated-bootstrap-audit' if role=='lifecycleSubmit' else target,template_obj,newcore)
  exe=O/target;m.run('LINK-'+role,prefix+[str(obj),'-o',str(exe)]+tail);m.admit_products([exe],role,True);loaders[role]=loader_bindings(role,exe)
  if role=='nano':
   raw,err=m.run('CLI-nano-help',[str(exe),'--help'],seconds=15);need(not err and raw.startswith(b'Nano local chain tools\nUsage: nano <init|status|audit|mine|validate|import|serve|run> <genesis.json> <database-directory> [arguments]\n'),'Nano help oracle')
   raw,err=m.run('CLI-nano-rpc-arity',[str(exe),'rpc'],1,seconds=15);need(not raw and err==b'Nano: rpc <URL> <info|nodes|network|balance|send|deploy|receipt|mine> [arguments]\n','Nano rpc arity')
  else:arity(role,exe)
  need(not list((O/'tmp').iterdir()) and not list((O/'probe-home').iterdir()),'safe CLI no HOME/tmp writes');qualified[role]=dict(path=str(exe),**pin(m.material(exe)[1]));statistics[role]=details
 # Preserve the selected original fixture roots, without moving their macro binding.
 fixture_rel=['tests/fixtures/capella-bootstrap-anchor-cb532/manifest.json','tests/fixtures/capella-bootstrap-anchor-cb532/profile.json','tests/fixtures/capella-bootstrap-anchor-cb532/genesis-state.ssz','tests/fixtures/capella-bootstrap-anchor-cb532/genesis.json','tests/fixtures/consensus-capella-slots-official/manifest.json','tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz','tests/fixtures/consensus-altair-fork-head-official/manifest.json','tests/fixtures/consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml','tests/fixtures/consensus-bellatrix-containers-official/minimal-bellatrix.yaml','tests/fixtures/consensus-capella-withdrawals-official/minimal-capella.yaml']
 for root in (R,S):
  for rel in fixture_rel:m.EXPECTED[str(root/rel)]=pin(git('show',args.revision+':'+rel))
 m.stable([root/rel for root in (R,S) for rel in fixture_rel],'public-runtime-fixtures')
 m.fence();need(git('rev-parse','HEAD').decode().strip()==args.revision and git('status','--porcelain')==b'','source remained clean')
 m.save('software-admission.private.json',{'format':'nano-current-native-runtime-relink-software-binding-v1','sourceRevision':args.revision,'files':m.HELD,'aliases':m.ALIASES,'products':m.PRODUCTS,'programs':qualified,'stats':statistics,'directLoaderBindings':loaders,'core':dict(path=str(newcore),**pin(m.material(newcore)[1])),'oldAuthoritiesRefreshed':False,'historicalRawMdIdentityRecoveredForThreeOldHelpers':False})
 result={'format':'nano-current-native-runtime-relink-qualification-v1','passed':True,'sourceRevision':args.revision,'compiledTranslationUnits':1,'independentDependencyScans':8,'authenticatedObjectReuses':7,'currentCoreLinks':7,'smokeCliCount':8,'coreArchiveMembers':143,'otherMembersUnchanged':142,'runtimeMemberMatchesCompiledObject':True,'runtime':runtime_stats,'core':pin(m.material(newcore)[1]),'programs':qualified,'stats':statistics,'softwareAdmission':pin(m.material(O/'software-admission.private.json')[1]),'steps':m.STEPS,'p2pDependencyScans':0,'p2pLinks':0,'p2pExecutions':0,'oldTestsReplayed':0,'rpc120ChecksReplayed':False,'startup31RefusalsExecuted':False,'newSeedOrLedgerCreated':False,'signaturesCreated':0,'runtimeStarted':False,'dutiesResumed':False,'all24CampaignAdmissions':False,'wholeSdkQualified':False,'recursiveDylibClosureRequalified':False,'fullCmakeConfigureExecuted':False,'fullLoaderQualified':False,'productionQualified':False,'oldAuthoritiesRefreshed':False,'elapsedMs':int((time.monotonic()-m.START)*1000)}
 m.save('result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 first=None
 try:main()
 except BaseException as error:first=error
 finally:
  if m is not None and created:
   for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP):signal.signal(sig,signal.SIG_IGN)
   try:final_fence();m.save('final-fence-public.json',{'passed':True,'firstFailurePreserved':first is not None,'oldAuthoritiesRefreshed':False})
   except BaseException as error:
    if first is None:first=error
    try:m.save('final-fence-failure-public.json',{'passed':False,'firstFailurePreserved':True})
    except BaseException:pass
   if first is not None:
    try:m.save('refusal.private.json',{'passed':False,'errorType':type(first).__name__,'errorDetailsSuppressed':True,'oldAuthoritiesRefreshed':False})
    except BaseException:pass
 if first is not None:sys.stderr.write('Native runtime relink refused; details suppressed.\n');sys.exit(1)
