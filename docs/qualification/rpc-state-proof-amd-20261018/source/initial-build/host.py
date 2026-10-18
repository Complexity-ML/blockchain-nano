#!/usr/bin/env python3
"""Root-only AMD C6 Core/units successor; public source, no automatic imports."""
import argparse, hashlib, json, os, pathlib, re, shlex, signal, stat, sys, time, types, uuid

R=pathlib.Path('/home/ubuntu/blockchain-nano-git')
ADAPTER=R/'docs/qualification/current-runtime-assemblies-20261016/source/amd-host/qualify-current-amd-helpers-root.py'
ADAPTER_PIN={'sha256':'f28ae7a55fadc9fc9bfe04a70c18f991e7440d4c4ab1c4d860fd7aca872fc350','bytes':52822}
WORKER_PIN={'sha256':'e9bfc40c92f8659af4eda8a4ebcd306699531b058b56acb5d2a426e34ebb7301','bytes':37768}
HELPER_PIN={'sha256':'aa20f1e88276e674de0414335393621a07ef7d26731bcb4799bf3f3d2cfaacd5','bytes':10821}
CORE_PIN={'sha256':'a3fdf87a71e05264058fb8bbb7894082330ef6042dd23f68007cf9d782ae2c48','bytes':40811064}
PRODUCER_PIN={'sha256':'06a34b84b4bdd41a282ce59bd232f70e4fe6b8b1677c9e6133461d3ac797c896','bytes':284564}
CANDIDATE='c6cdf780c68eab8b06306d76257bda9028928ea3'
BASE='1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
UNITS={
 'nano':('runtime','nano','src/main.cpp'),
 'checkpointExporter':('runtime','nano-bellatrix-checkpoint-export','tests/bellatrix_checkpoint_export.cpp'),
 'bootstrapImport':('runtime','nano-capella-isolated-bootstrap-import','tests/capella/isolated_bootstrap_import.cpp'),
 'bootstrapAudit':('runtime','nano-capella-isolated-bootstrap-audit','tests/capella/isolated_bootstrap_audit.cpp'),
 'stoppedCapture':('runtime','nano-capella-isolated-stopped-capture','tests/capella/isolated_stopped_capture.cpp'),
 'depositAudit':('runtime','nano-capella-deposit-audit','tests/capella/deposit_audit.cpp'),
 'slashingFixture':('runtime','nano-capella-slashing-fixture','tests/capella/slashing_fixture.cpp'),
 'lifecycleSubmit':('runtime','nano-capella-lifecycle-submit','tests/multihost/lifecycle_submit.cpp'),
 'candidate-proof':('candidate','nano-rpc-get-proof-tests','tests/test_rpc_get_proof.cpp'),
 'candidate-cost':('candidate','nano-rpc-state-read-cost-tests','tests/test_rpc_state_read_cost.cpp'),
 'candidate-state-readers':('candidate','nano-rpc-state-block-id-tests','tests/test_rpc_state_block_id.cpp'),
 'candidate-legacy-rpc':('candidate','nano-rpc-tests','tests/test_rpc.cpp')}

def need(ok,why='AMD C6 successor refused'):
 if not ok:raise RuntimeError(why)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def valid_pin(p,empty=False):return type(p)is dict and set(p)=={'sha256','bytes'}and type(p['sha256'])is str and re.fullmatch('[0-9a-f]{64}',p['sha256'])and type(p['bytes'])is int and (0<=p['bytes']if empty else 0<p['bytes'])and p['bytes']<=536870912 and (p['bytes']!=0 or p['sha256']==hashlib.sha256(b'').hexdigest())
def physical(p):
 p=pathlib.Path(p);need(p.is_absolute()and p.resolve(strict=True)==p,'physical absolute path');return p
def json_body(raw):
 def unique(pairs):
  names=[k for k,_ in pairs];need(len(names)==len(set(names)),'duplicate JSON keys');return dict(pairs)
 def nonfinite(_):raise ValueError('nonfinite JSON')
 return json.loads(raw,object_pairs_hook=unique,parse_constant=nonfinite)
def material(p):
 p=physical(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<a.st_size<=16777216,'bounded public control/module');row={k:getattr(a,k)for k in F};parts=[];n=0
  while b:=os.read(fd,1048576):n+=len(b);need(n<=a.st_size,'fixed input length');parts.append(b)
  raw=b''.join(parts);need(n==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'FD/path changed');return dict(row,sha256=pin(raw)['sha256']),raw
 finally:os.close(fd)
def control_input(path,want):
 need(valid_pin(want)and want['bytes']<=8388608,'external control pin');last=None;count=0;since=None;start=time.monotonic();samples=[]
 while True:
  row,raw=material(path);now=time.monotonic();need(now-start<30 and pin(raw)==want,'new control known bytes/deadline')
  if row==last:count+=1
  else:count=1;since=now
  samples.append({'elapsedMs':round((now-start)*1000),'row':row})
  if count>=3 and now-since>=2:return json_body(raw),row,samples
  last=row;time.sleep(1.05)
def load_adapter():
 row,raw=material(ADAPTER);need(pin(raw)==ADAPTER_PIN,'f28 public source pin')
 m=types.ModuleType('qualified_amd_selected_functions');m.__file__=str(ADAPTER);exec(compile(raw,str(ADAPTER),'exec'),m.__dict__);m.hold(ADAPTER,row);return m
def field_row(row):
 return type(row)is dict and set(row)==set(F)|{'sha256'}and all(type(row[k])is int and row[k]>=0 for k in F)and type(row['sha256'])is str and re.fullmatch('[0-9a-f]{64}',row['sha256'])
def descriptor(item):return type(item)is dict and set(item)=={'file','guest','pin'}and type(item['file'])is str and type(item['guest'])is str and valid_pin(item['pin'])

def run_host(c,control_path,control_row,control_samples):
 c=json_body(json.dumps(c,allow_nan=False).encode())
 fields={'format','ready','actualHead','outputRoot','toolDir','sourceRoots','originalFiles','retainedCore','coreRecipe','libraries','generatedProviders','units','cases','runtimeInputs','timeoutSeconds'}
 need(type(c)is dict and set(c)==fields and c['format']=='nano-rpc-candidate-amd-control-v1'and c['ready']is True,'exact ready 15-field control')
 need(sys.platform.startswith('linux')and os.getuid()==os.getgid()==1000,'AMD Linux owner')
 need(type(c['actualHead'])is str and re.fullmatch('[0-9a-f]{40}',c['actualHead'])and type(c['timeoutSeconds'])is int and 240<=c['timeoutSeconds']<=1980,'HEAD and host/worker budgets')
 out=pathlib.Path(c['outputRoot']);tools=physical(c['toolDir']);need(tools.is_dir(),'staged tool directory')
 need(out.is_absolute()and out.parent.resolve(strict=True)==out.parent and out.parent.stat().st_uid==1000 and re.fullmatch('rpc-candidate-amd-root-[a-z0-9-]{1,80}',out.name)and not out.exists()and not out.is_symlink(),'new output namespace')
 roots=c['sourceRoots'];need(type(roots)is dict and set(roots)=={'candidate','baseline'},'two source roots')
 for k,rev in [('candidate',CANDIDATE),('baseline',BASE)]:
  x=roots[k];need(type(x)is dict and set(x)=={'root','revision','authority'}and x['revision']==rev,'source root declaration');x['root']=str(physical(x['root']));need(pathlib.Path(x['root']).is_dir(),'source directory');a=x['authority'];need(type(a)is dict and set(a)=={'file','pin'}and physical(a['file'])==pathlib.Path(x['root']).parent/'source-authorities.private.json'and valid_pin(a['pin']),'source original authority descriptor')
 protected=[R,tools,*[pathlib.Path(x['root'])for x in roots.values()]]
 need(all(not(out.is_relative_to(p)or p.is_relative_to(out))for p in protected),'disjoint output inputs')
 need(not pathlib.Path(roots['candidate']['root']).is_relative_to(roots['baseline']['root'])and not pathlib.Path(roots['baseline']['root']).is_relative_to(roots['candidate']['root']),'disjoint source projections')
 m=load_adapter();need(not(out.is_relative_to(m.BASE)or m.BASE.is_relative_to(out)),'old producer disjoint')
 original=c['originalFiles'];need(type(original)is dict and 10<=len(original)<=30000 and str(control_path)not in original,'explicit old/prospective Root authority rows')
 for p,row in original.items():need(type(p)is str and field_row(row),'full9/SHA input table');m.hold(physical(p),row)
 for p in [ADAPTER,pathlib.Path(__file__).resolve(),tools/'worker.py',tools/'archive_object_replacements.py',pathlib.Path('/usr/bin/git'),pathlib.Path('/usr/bin/docker'),pathlib.Path('/usr/bin/sudo')]:need(str(p)in original,'Root input row required: '+str(p));m.hold(p,original[str(p)])
 m.hold(control_path,control_row);os.umask(0o077);out.mkdir(mode=0o700);m.OUT=out;m.OUT_CREATED=True;m.DEADLINE=time.monotonic()+c['timeoutSeconds']
 for leaf in ('outputs','docker-client'):(out/leaf).mkdir(mode=0o700)
 m.save('control-first-admission.private.json',{'observations':control_samples,'knownPin':{'sha256':control_row['sha256'],'bytes':control_row['st_size']},'oldAuthoritiesRefreshed':False})
 docker=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',str(out/'docker-client')]
 name='nano-rpc-candidate-amd-'+uuid.uuid4().hex;label='nano.rpc.candidate.amd';cid=None;attempted=False;first=None;report=None;cleanup={};diff=[];image_final=None;image_checked=False
 def dk(tag,args,seconds=30,expected=0,clean=False):
  raw,err,code=m.run(tag,docker+args,seconds,expected=None,cleanup=clean);need(expected is None or code==expected,'Docker command refused: '+tag);return raw,err,code
 def owned(identifier,clean=False):
  need(type(identifier)is str and re.fullmatch('[0-9a-f]{64}',identifier),'full own CID');raw,err,_=dk('inspect-owned',['inspect',identifier],30,clean=clean);need(not err,'inspect stderr');rows=json_body(raw);need(type(rows)is list and len(rows)==1,'one own container');q=rows[0];need(q['Id']==identifier and q['Name']=='/'+name and q['Image']==m.IMAGE and(q['Config'].get('Labels')or{}).get(label)==name,'own container identity');return q
 def image(tag,clean=False):
  raw,err,_=dk(tag,['image','inspect',m.IMAGE,'--format','{{json .}}'],30,clean=clean);v=json_body(raw);need(not err and v['Id']==m.IMAGE and v['Architecture']=='amd64'and v['Os']=='linux','fixed SDK image');return {'passed':True,'image':m.IMAGE,'architecture':'amd64'}
 def selected(path,expected=None,body=True):
  key=str(physical(path));need(key in original,'selected descriptor/provider row missing: '+key);raw=m.hold(key,original[key]);need(expected is None or pin(raw)==expected,'selected byte pin');return raw if body else None
 def select(value,keys):
  for key in keys:value=value[key]
  return value
 project={};source_pins={};inventories={};known={};mounts={};library_guests=[];unit_results_expected={};core=None
 def inventory():
  for root,wanted in inventories.items():
   found=set();pending=[pathlib.Path(root)];entries=0
   while pending:
    with os.scandir(pending.pop())as children:
     for e in children:
      entries+=1;need(entries<=40000 and not e.is_symlink(),'bounded alias-free snapshot')
      if e.is_dir(follow_symlinks=False):pending.append(pathlib.Path(e.path))
      else:need(e.is_file(follow_symlinks=False),'regular source leaf');found.add(e.path)
   need(found==wanted,'exact source inventory')
 try:
  worker=tools/'worker.py';helper=tools/'archive_object_replacements.py';wraw=selected(worker,WORKER_PIN);hraw=selected(helper,HELPER_PIN)
  need(m.git('head',['rev-parse','HEAD']).decode().strip()==c['actualHead']and m.git('clean',['status','--porcelain'])==b'','clean dynamic main HEAD')
  docs={key:json_body(selected(m.BASE/rel,{'sha256':sha,'bytes':size}))for key,(rel,sha,size)in m.REFS.items()}
  for k,guest in [('candidate','/work'),('baseline','/baseline-work')]:
   x=roots[k];root=pathlib.Path(x['root']);a=x['authority'];authority=json_body(selected(a['file'],a['pin']));need(authority['format']=='nano-public-candidate-source-authorities-v1'and authority['candidate']==x['revision']and authority['sourceRoot']==str(root)and authority['originalRowsRefreshed']is False and authority['privateRuntimeDataIncluded']is False,'source authority exact scope')
   pins=authority['pins'];samples=authority['observations'];need(type(pins)is dict and 10<=len(pins)<=20000 and type(samples)is list and len(samples)>=3 and samples[-1]['stableSamples']>=3 and samples[-1]['stableSpanMs']>=2000,'source admitted observations');rows=samples[-1]['files'];need(type(rows)is dict and len(rows)==len(pins),'source rows/pins bijection');found=set()
   for rel,want in pins.items():
    r=pathlib.PurePosixPath(rel);need(type(rel)is str and not r.is_absolute()and '..'not in r.parts and r.as_posix()==rel and valid_pin(want,True),'canonical source pin');p=root/rel;need(str(p)in rows and field_row(rows[str(p)]),'original source FD9');raw=m.hold(p,rows[str(p)]);need(pin(raw)==want,'original source bytes');project[guest+'/'+rel]=want;source_pins[guest+'/'+rel]=want;found.add(str(p))
   need(sum(p['bytes']for p in pins.values())<=268435456,'source byte budget');inventories[str(root)]=found
  need(len(inventories[roots['candidate']['root']])==1276 and len(inventories[roots['baseline']['root']])==1271,'Root full source projections 1276/1271')
  cp=c['retainedCore'];need(type(cp)is dict and set(cp)=={'file','pin','producerPin'}and cp['pin']==CORE_PIN and cp['producerPin']==PRODUCER_PIN,'Core a3 and original producer');core=physical(cp['file']);need(not(out.is_relative_to(core.parent)or core.is_relative_to(out)),'Core and output disjoint');producer=core.parent/'final-observations.private.json';binding=json_body(selected(producer,PRODUCER_PIN));row=binding['products'][str(core)]['hostFD9'];need(field_row(row),'original Core hostFD9');rawcore=m.hold(core,row);need(pin(rawcore)==CORE_PIN,'original Core bytes');known['/baseline/libnano_core.a']=CORE_PIN
  recipe=c['coreRecipe'];need(type(recipe)is dict and set(recipe)=={'recordsAt','compilerFlags','coreSources'}and recipe['recordsAt']==['/outputs/libnano_core.a','objects'],'original Core recipe selector');records=select(docs['archives'],recipe['recordsAt']);need(type(records)is dict and len(records)==143,'143 retained compiler records');prefixes=[];sources=[]
  for key,rec in records.items():
   comp=rec['compile'];argv=comp.get('arguments')or shlex.split(comp['command']);rel=comp['file'].removeprefix('/work/');need(comp['directory']=='/outputs'and re.fullmatch(r'src/[A-Za-z0-9_+.-]+[.]cpp',rel)and rel!='src/main.cpp'and key=='/outputs/CMakeFiles/nano_core.dir/'+rel+'.o'and len(argv)>4 and argv[-4:]==['-o','CMakeFiles/nano_core.dir/'+rel+'.o','-c','/work/'+rel],'canonical retained Core row');prefixes.append(argv[:-4]);sources.append(rel)
  need(len(set(sources))==143 and all(v==prefixes[0]for v in prefixes),'one original common Core prefix');flags=[v.replace('/outputs/third_party','/old/third_party')for v in prefixes[0]];need(recipe['compilerFlags']==flags and type(recipe['coreSources'])is list and len(recipe['coreSources'])==143 and set(recipe['coreSources'])==set(sources),'Root flags/sources equal original records')
  old=docs['links']['nano-capella-isolated-bootstrap-audit'];link=old['linkRecipe']['argv'];need(link[:3]==['/usr/bin/clang++','-O3','-DNDEBUG']and link.count('-o')==1 and link.index('-o')==4,'canonical old Audit link');expected_libraries={}
  for word in link[6:]:
   g=os.path.normpath('/outputs/'+word)if not word.startswith(('/','-'))else word
   if not g.startswith('/outputs/')or g=='/outputs/libnano_core.a':continue
   rr=docs['llvm'][g]['object']if g in docs['llvm']else docs['archives'][g]['archive'];m.old_file(rr);gg='/old/'+g.removeprefix('/outputs/');expected_libraries[gg]={'file':rr['path'],'guest':gg,'pin':{'sha256':rr['sha256'],'bytes':rr['bytes']}}
  need(len(expected_libraries)==6,'four original archives plus two ASM providers');libs=c['libraries'];gen=c['generatedProviders'];need(type(libs)is list and len(libs)==6 and all(descriptor(x)for x in libs)and{v['guest']:v for v in libs}==expected_libraries,'exact six original providers');need(type(gen)is list and 1<=len(gen)<=16 and all(descriptor(x)for x in gen),'bounded separately pinned generated providers')
  for item in [*libs,*gen]:
   g=item['guest'];p=physical(item['file']);need(g.startswith('/old/')and pathlib.PurePosixPath(g).as_posix()==g and '..'not in pathlib.PurePosixPath(g).parts and g not in mounts and str(p)not in mounts.values(),'unique normalized old provider binds');selected(p,item['pin']);mounts[g]=str(p);known[g]=item['pin']
  library_guests=[x['guest']for x in libs];cmake=m.hold(pathlib.Path(roots['candidate']['root'])/'CMakeLists.txt').decode();units=c['units'];cases=c['cases'];need(type(units)is list and len(units)==12 and len({u['name']for u in units})==12 and{u['name']for u in units}==set(UNITS),'twelve distinct units');need(type(cases)is list and len(cases)==13,'four tests and nine CLI')
  for u in units:
   need(type(u)is dict and set(u)=={'name','scope','source','compilerFlags','linkPrefix','linkTail','executable','object'},'worker unit eight fields');scope,target,rel=UNITS[u['name']];need(u['scope']==scope and u['source']=='/work/'+rel and u['executable']==target and u['object']=={'kind':'compile'},'fresh ABI unit/source');need(cmake.count('add_executable('+target+' '+rel+')')==1 and cmake.count('target_link_libraries('+target+' PRIVATE nano_core'+(' nano_ckzg'if u['name']=='checkpointExporter'else'')+')')==1,'target CMake source and Core binding')
   a=u['compilerFlags'];need(type(a)is list and a and all(type(x)is str and x and '\0'not in x for x in a),'explicit compile argv');macro='target_compile_definitions('+target+' PRIVATE NANO_TEST_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}")';want=cmake.count(macro)==1;macros=[x for x in a if x.startswith('-DNANO_TEST_SOURCE=')];need(macros==(['-DNANO_TEST_SOURCE="/work"']if want else[]),'source fixture macro matches target');blst='target_include_directories('+target+' PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ckzg/blst/bindings")'
   if cmake.count(blst)==1:need('-I/work/third_party/ckzg/blst/bindings'in a or any(a[i:i+2]==['-I','/work/third_party/ckzg/blst/bindings']for i in range(len(a)-1)),'target BLST include')
   if u['name']=='checkpointExporter':
    for inc in ['/work/third_party/ckzg/src','/work/third_party/ckzg/blst/bindings','/old/third_party/ckzg/include']:
     need('-I'+inc in a or any(a[i:i+2]in(['-I',inc],['-isystem',inc])for i in range(len(a)-1)),'checkpoint direct CKZG public include: '+inc)
    need('/old/third_party/ckzg/include/nano_kzg_setup.hpp'in known,'checkpoint generated setup bound before CXX')
   need(type(u['linkPrefix'])is list and u['linkPrefix']and type(u['linkTail'])is list and all(type(x)is str and x and '\0'not in x for x in u['linkPrefix']+u['linkTail'])and u['linkTail'].count('/baseline/libnano_core.a')==1,'one Core per explicit link recipe');need(set(library_guests)<=set(u['linkTail']),'selected six providers present in each link');need(all(not x.startswith('/old/')or x in known for x in u['linkTail']),'no unbound old provider')
  case_fields={'name','unit','args','timeoutSeconds','expectedExit','stdout','stderr'};need(all(type(t)is dict and set(t)==case_fields for t in cases),'case seven fields');need(len({t['name']for t in cases})==13 and{t['unit']for t in cases}==set(UNITS),'all units exercised');runtime=[t for t in cases if UNITS[t['unit']][0]=='runtime'];tests=[t for t in cases if UNITS[t['unit']][0]=='candidate'];need(len(runtime)==9 and len(tests)==4,'exact scopes');need(sorted(tuple(t['args'])for t in runtime if t['unit']=='nano')==[(),('--help',)]and all(sum(t['unit']==n and t['args']==[]for t in runtime)==1 and sum(t['unit']==n for t in runtime)==1 for n in UNITS if UNITS[n][0]=='runtime'and n!='nano'),'arity/help only')
  need(all(type(t['expectedExit'])is int and t['expectedExit']==(0 if t['unit']=='nano'and t['args']==['--help']else 1)and type(t['timeoutSeconds'])is int and 1<=t['timeoutSeconds']<=30 for t in runtime),'safe exact runtime exits/budgets')
  need(type(c['runtimeInputs'])is list and len(c['runtimeInputs'])<=128 and len(set(c['runtimeInputs']))==len(c['runtimeInputs'])and '/work/tests/fixtures/python-rpc.json'in c['runtimeInputs']and all(x in source_pins for x in c['runtimeInputs']),'explicit held runtime fixtures');need(project['/work/tests/test_rpc_get_proof.cpp']=={'sha256':'ede46daa138e9edf9d8a4135dd732daff62e19fc0a80a2411ee3b2c9f8c93019','bytes':27244},'proof oracle V2')
  inventory();helper_ns={};exec(compile(hraw,str(helper),'exec'),helper_ns);rows=helper_ns['archive_object_rows'](rawcore);need(len(rows)==143,'143 original members');image('image-before');image_checked=True;raw,err,_=dk('absent',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}']);need(not raw.strip()and not err,'fresh container name')
  extra=[]
  for guest,host in sorted(mounts.items()):extra+=['--mount','type=bind,src='+host+',dst='+guest+',readonly']
  args=['create','--pull=never','--name',name,'--label',label+'='+name,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','128','--tmpfs','/tmp:rw,nosuid,nodev,size=1536m','--mount','type=bind,src='+roots['candidate']['root']+',dst=/work,readonly','--mount','type=bind,src='+roots['baseline']['root']+',dst=/baseline-work,readonly','--mount','type=bind,src='+str(core)+',dst=/baseline/libnano_core.a,readonly','--mount','type=bind,src='+str(out/'outputs')+',dst=/outputs',*extra,'--workdir','/outputs','--entrypoint','/bin/sh',m.IMAGE,'-ec','exec /bin/sleep '+str(c['timeoutSeconds']+180)]
  attempted=True;raw,err,_=dk('create',args,60);cid=raw.decode().strip();need(not err and re.fullmatch('[0-9a-f]{64}',cid),'new CID');raw,err,_=dk('start',['start',cid]);need(not err and raw.decode().strip()==cid,'start own container');q=owned(cid);need(q['State']['Running']and q['Config']['User']=='1000:1000'and q['HostConfig']['NetworkMode']=='none'and q['HostConfig']['ReadonlyRootfs'],'container guards');got={v['Destination']:v for v in q['Mounts']};expected={'/work':roots['candidate']['root'],'/baseline-work':roots['baseline']['root'],'/baseline/libnano_core.a':str(core),'/outputs':str(out/'outputs'),**mounts};need(all(got[g]['Type']=='bind'and got[g]['Source']==p and got[g]['RW']==(g=='/outputs')for g,p in expected.items()),'read-only input mounts')
  config={'coreMode':'incremental','hostOutput':str(out/'outputs'),'sourceGuest':'/work','baselineSourceGuest':'/baseline-work','outputGuest':'/outputs','dockerPrefix':[*docker,'exec',cid],'baselineCoreGuest':'/baseline/libnano_core.a','baselineCorePin':CORE_PIN,'knownGuestPins':known,'expectedObjectMembers':143,'sourcePins':source_pins,'projectPins':project,'units':units,'cases':cases,'runtimeInputs':c['runtimeInputs'],'guestUid':1000,'guestGid':1000,'timeoutSeconds':c['timeoutSeconds']-180,'coreCompilerFlags':flags,'coreSources':recipe['coreSources'],'qualifiedOldObjectRows':rows}
  need(len(config)==21,'21 serialized fields plus one runtime callback');m.save('worker-config.private.json',config);m.save('selected-host-inputs-before-worker.private.json',dict(m.HELD));m.save('prepared-public.json',{'sourceRevision':CANDIDATE,'actualHead':c['actualHead'],'image':m.IMAGE,'sourceFiles':{'candidate':1276,'baseline':1271},'core':CORE_PIN,'worker':WORKER_PIN,'archiveHelper':HELPER_PIN,'runtimeLibraries':6,'generatedProviders':len(gen),'coreScansRequested':143,'freshUnitsRequested':12,'candidateTestsRequested':4,'runtimeCliRequested':9,'newImagesBuilt':0,'productionQualified':False})
  worker_ns={};exec(compile(wraw,str(worker),'exec'),worker_ns);config['proveArchiveSet']=helper_ns['prove_object_replacements'];report=worker_ns['run_qualification'](config);counts=report['counts'];K=report['coreProof']['selectedReplacementCount'];want={'independentM':155,'coreIndependentM':143,'testIndependentM':4,'compiledTranslationUnits':K+12,'coreCompiledTranslationUnits':K,'testCompiledTranslationUnits':4,'archiveUpdates':1,'links':12,'loaderChecks':12,'testInvocations':4,'runtimeCliInvocations':9,'unitInvocations':13,'runtimeIndependentM':8,'runtimeCompiledTranslationUnits':8,'reusedSubmitObjects':0};need(report['passed']is True and report['allConfiguredCasesPassed']is True and report['coreMode']=='incremental'and counts==want and all(type(counts[k])is int for k in want)and report['coreProof']['members']==143 and report['coreProof']['unchangedOutsideReplacementSet']==143-K,'exact full AMD batch')
  need(m.git('head-after',['rev-parse','HEAD']).decode().strip()==c['actualHead']and m.git('clean-after',['status','--porcelain'])==b'','main unchanged');inventory()
 except BaseException as error:first=error
 finally:
  try:inventory()
  except BaseException as error:
   diff.append({'scope':'sourceInventory','errorType':type(error).__name__})
   if first is None:first=error
  for path,want in list(m.HELD.items()):
   try:
    now=m.material(path)[0]
    if now!=want:diff.append({'path':path,'changedFields':[k for k in want if now[k]!=want[k]]})
   except BaseException as error:diff.append({'path':path,'errorType':type(error).__name__})
  if diff and first is None:first=RuntimeError('original host inputs changed')
  m.CLEAN_DEADLINE=time.monotonic()+60
  try:
   if attempted:
    if not isinstance(cid,str)or not re.fullmatch('[0-9a-f]{64}',cid):
     raw,err,_=dk('discover-owned',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],15,clean=True);need(not err,'discover own stderr');ids=raw.decode().splitlines();need(len(ids)<=1,'one own namespace');cid=ids[0]if ids else None
    if cid:
     q=owned(cid,True)
     if q['State']['Running']:dk('stop-owned',['stop','--time','5',cid],15,clean=True)
     owned(cid,True);dk('remove-owned',['rm',cid],15,clean=True)
    raw,err,_=dk('absence',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],15,clean=True);need(not raw.strip()and not err,'own container absent')
   cleanup={'passed':True,'ownedContainerAbsent':True,'imagesOrVolumesPruned':False}
  except BaseException as error:
   cleanup={'passed':False,'errorType':type(error).__name__}
   if first is None:first=error
  if image_checked:
   try:image_final=image('image-final',True)
   except BaseException as error:
    image_final={'passed':False,'errorType':type(error).__name__}
    if first is None:first=error
  post_cleanup_fence=True;changed_paths={v['path']for v in diff if 'path'in v}
  for path,want in list(m.HELD.items()):
   try:
    now=m.material(path)[0]
    if now!=want:
     post_cleanup_fence=False
     if path not in changed_paths:diff.append({'path':path,'changedFields':[k for k in want if now[k]!=want[k]]});changed_paths.add(path)
   except BaseException as error:
    post_cleanup_fence=False
    if path not in changed_paths:diff.append({'path':path,'errorType':type(error).__name__});changed_paths.add(path)
  if not post_cleanup_fence and first is None:first=RuntimeError('original host inputs changed after cleanup/image-final')
  result={'format':'nano-rpc-candidate-amd-host-result-v1','passed':first is None,'workerPassed':bool(report and report.get('passed')),'allConfiguredCasesPassed':bool(report and report.get('allConfiguredCasesPassed')),'sourceRevision':CANDIDATE,'actualHead':c['actualHead'],'baselineSourceRevision':BASE,'oldCore':CORE_PIN,'image':m.IMAGE,'imageFinalFence':image_final,'hostFinalFenceAfterCleanup':post_cleanup_fence,'originalHostInputsUnchanged':not diff,'changedHostInputs':len(diff),'cleanup':cleanup,'workerCounts':None if report is None else report['counts'],'selectedReplacementCount':None if report is None else report['coreProof']['selectedReplacementCount'],'runtimeLibraries':len(library_guests),'generatedProviders':len(c['generatedProviders']),'newImagesBuilt':0,'cmakeConfigures':0,'currentDbReclassifiedAsOriginalAuthority':False,'rootIndependentProofVectorsVerified':False,'runtimeServeOrRpcQualified':False,'runtimeDutiesResumed':False,'productionQualified':False}
  if first is not None:result.update(firstFailureType=type(first).__name__,firstFailureMessage=str(first))
  try:m.save('host-final-observations.private.json',{'inputs':dict(m.HELD),'differences':diff});m.save('host-final-fence-public.json',{'passed':not diff,'selectedFiles':len(m.HELD),'oldAuthoritiesRefreshed':False});m.save('result-public.json',result);print(json.dumps(result),flush=True)
  except BaseException:
   if first is None:raise
 if first is not None:raise first
 return result

def main():
 p=argparse.ArgumentParser();p.add_argument('control');p.add_argument('sha256');p.add_argument('bytes',type=int);a=p.parse_args();path=physical(a.control);c,row,samples=control_input(path,{'sha256':a.sha256,'bytes':a.bytes});run_host(c,path,row,samples)
if __name__=='__main__':main()
