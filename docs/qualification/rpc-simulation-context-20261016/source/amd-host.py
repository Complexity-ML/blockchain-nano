#!/usr/bin/env python3
"""Root-only AMD RPC-context adapter; source author performs no operations."""
import argparse,calendar,datetime,hashlib,io,json,os,pathlib,re,signal,stat,sys,tarfile,time,types,uuid
REPOSITORY=pathlib.Path('/home/ubuntu/blockchain-nano-git')
ADAPTER=REPOSITORY/'docs/qualification/current-runtime-assemblies-20261016/source/amd-host/qualify-current-amd-helpers-root.py'
ADAPTER_PIN={'sha256':'f28ae7a55fadc9fc9bfe04a70c18f991e7440d4c4ab1c4d860fd7aca872fc350','bytes':52822}
HEAD='cc51bc0ce37b36724f11a9e5fc1c3504056a6f74'
CORE_SOURCE_REVISION='68d64a5330d16c971dd603b56e0f14c88c1459c8'
PRUNING=pathlib.Path('/home/ubuntu/current-amd-pruning-operation-20261016-01')
PRUNING_RESULT_PIN={'sha256':'77af36bfdb27c49bb49b66c1b1430a9cf62ce5fa81969bba3f568aaaa9385911','bytes':7612}
CORE=PRUNING/'outputs/libnano_core.a'
CORE_PIN={'sha256':'1687f1876ea784006b0d687db0f94e3a49b01f8edaa3a599fdf942a9fc4d1525','bytes':40820160}
CORE_BINDING_PIN={'sha256':'65f8ccb6a4cd812f702596fb1d9349eaa385f04ffa4f79a71fa16ac8c7f08e9c','bytes':249294}
CORE_HOST_PIN={'sha256':'d0d7ddbcfbc79dbc5276eb3bd5ef2083da72f88596f8a0fd15a391fff6c3ed96','bytes':6806}
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
REGRESSIONS=[{'role':'nano-rpc-tests','source':'/work/tests/test_rpc.cpp','arguments':[]},{'role':'nano-rpc-access-list-tests','source':'/work/tests/test_rpc_access_list.cpp','arguments':['--self-test']},{'role':'nano-rpc-simulation-block-id-tests','source':'/work/tests/test_rpc_simulation_block_id.cpp','arguments':['--eip1898-self-test']}]
SOURCE_PINS={'src/rpc.cpp': {'sha256': '4dfa7e0844c9ff6da2433d6b9ab3aa2c12ea06fa1a33ec11b06f9dcc79a1dc6f', 'bytes': 81453}, 'tests/test_rpc.cpp': {'sha256': 'fc4ef5325949dc28e8a1f472ed0cf73344d28b340a95784f681d3772778b3413', 'bytes': 76624}, 'tests/test_rpc_simulation_context.cpp': {'sha256': '1790fd79cca93cc807ef8539e1138d06eeacd53a8760cbc5be95ac30a0377b6c', 'bytes': 18497}, 'tests/test_rpc_access_list.cpp': {'sha256': '1493f078cb14e0df2218ba2abde4b23e2b34b2c48cfc9349ac5489c498ba2f49', 'bytes': 26208}, 'tests/test_rpc_simulation_block_id.cpp': {'sha256': 'b8e7625d0dc3b34c2156a5e62180630442edc49cef922bf703bbb9e5a54fe491', 'bytes': 26990}, 'tests/fixtures/python-rpc.json': {'sha256': 'edcaf3916ea0c67954bd2df2ef5cca00732c9d47e871a07757efdaef8a56deef', 'bytes': 10687}}
OPTIONAL_CMAKE_PIN={'sha256':'20b4b7c666777c389d401dd31b79755673a4aa4fac684800f6ee9a586b28f69f','bytes':176250}

def need(ok):
 if not ok:raise RuntimeError('AMD RPC context adapter refused')

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
 m=types.ModuleType('qualified_amd_rpc_context_functions');m.__file__=str(ADAPTER);exec(compile(raw,str(ADAPTER),'exec'),m.__dict__)
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

def external_pin(a,key,maximum):
 sha=getattr(a,key+'_sha256');size=getattr(a,key+'_bytes')
 need(re.fullmatch('[0-9a-f]{64}',sha) is not None and 0<size<=maximum)
 return {'sha256':sha,'bytes':size}

def unique_json(raw):
 def pairs(items):
  out={}
  for key,value in items:need(key not in out);out[key]=value
  return out
 return json.loads(raw,object_pairs_hook=pairs)

def main(m):
 p=argparse.ArgumentParser();p.add_argument('--output-root',required=True)
 for key in ('worker','source_tar','expected_json'):
  option=key.replace('_','-');p.add_argument('--'+option,required=True);p.add_argument('--'+option+'-sha256',required=True);p.add_argument('--'+option+'-bytes',required=True,type=int)
 a=p.parse_args();m.need(sys.platform.startswith('linux') and os.getuid()==os.getgid()==1000)
 candidate=pathlib.Path(a.output_root);inputs={key:pathlib.Path(getattr(a,key)) for key in ('worker','source_tar','expected_json')}
 m.need(candidate.is_absolute() and candidate.parent.resolve()==candidate.parent and candidate.parent.is_dir() and re.fullmatch('rpc-context-amd-root-[a-z0-9-]{1,100}',candidate.name) and not candidate.exists() and not candidate.is_symlink())
 m.need(candidate.parent.stat().st_uid==1000 and all(not candidate.is_relative_to(root) and not root.is_relative_to(candidate) for root in (m.R,m.BASE,PRUNING,m.CORE_SOURCE.parent)))
 m.need(all(q.is_absolute() and q.resolve()==q and not q.is_relative_to(candidate) for q in inputs.values()))
 pins={key:external_pin(a,key,131072 if key=='worker' else 536870912 if key=='source_tar' else 16777216) for key in inputs}
 candidate.mkdir(mode=0o700);m.OUT=candidate;m.OUT_CREATED=True;m.DEADLINE=time.monotonic()+1350
 for leaf in ('source','controls','outputs','baseline','docker-client'):(m.OUT/leaf).mkdir(mode=0o700)
 tools={pathlib.Path(q):m.digest(m.material(q)[1]) for q in ('/usr/bin/git','/usr/bin/docker','/usr/bin/sudo')};m.stable_new('os-utilities',tools);m.hold(pathlib.Path(__file__).resolve())
 worker_raw=m.hold(inputs['worker'],pins['worker'],131072);source_raw=m.hold(inputs['source_tar'],pins['source_tar']);expected=unique_json(m.hold(inputs['expected_json'],pins['expected_json'],16777216))
 m.need(isinstance(expected,dict) and 0<len(expected)<=20000)
 for guest,z in expected.items():
  m.need(isinstance(guest,str) and guest.startswith('/work/') and len(guest)<=1024 and '\x00' not in guest and '\n' not in guest and '\r' not in guest)
  rel=pathlib.PurePosixPath(guest[6:]);m.need(str(rel)==guest[6:] and not rel.is_absolute() and '..' not in rel.parts and '.' not in rel.parts)
  m.need(isinstance(z,dict) and set(z)=={'sha256','bytes'} and isinstance(z['sha256'],str) and re.fullmatch('[0-9a-f]{64}',z['sha256']) is not None and type(z['bytes']) is int and 0<=z['bytes']<=33554432)
 m.need(all(expected.get('/work/'+rel)==z for rel,z in SOURCE_PINS.items()))
 if '/work/CMakeLists.txt' in expected:m.need(expected['/work/CMakeLists.txt']==OPTIONAL_CMAKE_PIN)
 m.need(all('/work/network/'+leaf in expected for leaf in ('altair_rpc_service.hpp','bellatrix_rpc_service.hpp','capella_consensus_service.hpp','capella_inventory_sessions.hpp')))
 m.need(m.git('git-head',['rev-parse','HEAD']).decode().strip()==HEAD and m.git('git-clean',['status','--porcelain'])==b'')
 m.need(m.git('unchanged-core-source',['diff','--name-only',CORE_SOURCE_REVISION,HEAD,'--','include','src','network','third_party'])==b'')
 # Only the four already-permitted receipts are read. No SDK/source-admission table.
 docs={key:json.loads(m.hold(m.BASE/rel,{'sha256':sha,'bytes':size},16777216)) for key,(rel,sha,size) in m.REFS.items()}
 proof=json.loads(m.hold(PRUNING/'result-public.json',PRUNING_RESULT_PIN,16384))
 m.need(proof['passed'] is True and proof['newCoreSha256']==CORE_PIN['sha256'] and proof['newCoreBytes']==CORE_PIN['bytes'] and proof['coreArchiveMembers']==143 and proof['otherCoreMembersByteExact']==142)
 m.need({'sha256':proof['softwareAdmissionSha256'],'bytes':proof['softwareAdmissionBytes']}==CORE_BINDING_PIN)
 binding=json.loads(m.hold(PRUNING/'outputs/software-admission.private.json',CORE_BINDING_PIN,16777216))
 m.need(binding['format']=='nano-linux-pruning-operation-software-binding-v1' and binding['owner']=='vps-docker' and binding['imageId']==m.IMAGE and binding['sourceRevisionDeclared']==CORE_SOURCE_REVISION)
 history=json.loads(m.hold(PRUNING/'qualified-host-products-first-admission.private.json',CORE_HOST_PIN,16384));core_row=history['observations'][-1]['table'][str(CORE)]
 m.need(set(core_row)==set(F)|{'sha256'} and core_row==guest_row(binding['newCore']) and core_row['sha256']==CORE_PIN['sha256'] and core_row['st_size']==CORE_PIN['bytes'])
 core_raw=m.hold(CORE,core_row);core=m.OUT/'baseline/libnano_core.a';m.write_new(core,core_raw,0o400);m.stable_new('known-current-core-copy',{core:CORE_PIN})
 m.need(os.statvfs(m.OUT).f_bavail*os.statvfs(m.OUT).f_frsize>=3*1024**3)
 source=m.OUT/'source';seen=set();total=0;allowed_dirs=set()
 for guest in expected:
  rel=pathlib.PurePosixPath(guest[6:]);allowed_dirs.update(str(q) for q in rel.parents if str(q)!='.')
 with tarfile.open(fileobj=io.BytesIO(source_raw),mode='r:') as tf:
  entries=0
  for item in tf:
   entries+=1;m.need(entries<=40000 and not item.issym() and not item.islnk() and (item.isdir() or item.isfile()))
   if item.name in ('','.','./'):m.need(item.isdir());continue
   rel=pathlib.PurePosixPath(item.name);m.need(not rel.is_absolute() and '..' not in rel.parts and str(rel)==item.name.rstrip('/') and len(item.name)<=1024)
   if item.isdir():m.need(str(rel) in allowed_dirs);(source/str(rel)).mkdir(parents=True,exist_ok=True);continue
   guest='/work/'+str(rel);m.need(guest in expected and guest not in seen and item.size==expected[guest]['bytes']);seen.add(guest);total+=item.size;m.need(total<=536870912)
   stream=tf.extractfile(item);m.need(stream is not None);raw=stream.read(item.size+1);m.need(len(raw)==item.size and m.digest(raw)==expected[guest]);stream.close()
   dest=source/str(rel);dest.parent.mkdir(parents=True,exist_ok=True);m.write_new(dest,raw,0o444)
 m.need(seen==set(expected))
 del source_raw,core_raw
 for dest in sorted(source.rglob('*'),key=lambda q:len(q.parts),reverse=True):
  if dest.is_dir():os.chmod(dest,0o555)
 os.chmod(source,0o555)
 # Keep the canonical Core-TU flags separate from a Core-only consumer's flags.
 archive=docs['archives']['/outputs/libnano_core.a'];m.need(isinstance(archive['objects'],dict) and len(archive['objects'])==143)
 rpc_guest='/outputs/CMakeFiles/nano_core.dir/src/rpc.cpp.o';comp=archive['objects'][rpc_guest]['compile'];m.need(comp['file']=='/work/src/rpc.cpp' and comp['directory']=='/outputs')
 argv=comp.get('arguments') or m.shlex.split(comp['command']);m.need(argv[0]=='/usr/bin/clang++' and argv.count('-o')==argv.count('-c')==1 and argv[-4]=='-o' and argv[-2:]==['-c',comp['file']])
 m.need(os.path.normpath(argv[-3] if argv[-3].startswith('/') else '/outputs/'+argv[-3])==rpc_guest)
 core_flags=[q.replace('/outputs/third_party','/old/third_party') for q in argv[:-4]];m.need(not any('NANO_' in q for q in core_flags))
 rec=docs['links']['nano-capella-isolated-bootstrap-audit'];comp=rec['compile'];m.need(comp['file']=='/work/tests/capella/isolated_bootstrap_audit.cpp' and comp['directory']=='/outputs')
 argv=comp.get('arguments') or m.shlex.split(comp['command']);m.need(argv[0]=='/usr/bin/clang++' and argv[-2:]==['-c',comp['file']] and argv[-4]=='-o' and argv.count('-o')==argv.count('-c')==1)
 own=os.path.normpath(argv[-3] if argv[-3].startswith('/') else '/outputs/'+argv[-3]);flags=[q.replace('/outputs/third_party','/old/third_party') for q in argv[:-4]];m.need(not any('NANO_' in q for q in flags))
 link=rec['linkRecipe']['argv'];m.need(link[:3]==['/usr/bin/clang++','-O3','-DNDEBUG'] and link.count('-o')==1 and link.index('-o')==4 and os.path.normpath(link[3] if link[3].startswith('/') else '/outputs/'+link[3])==own and os.path.normpath(link[5] if link[5].startswith('/') else '/outputs/'+link[5])=='/outputs/nano-capella-isolated-bootstrap-audit')
 m.need(set(docs['llvm'])=={'/outputs/third_party/mcl/bint64.o','/outputs/third_party/mcl/base64.o'})
 mounts={};tail=[];order=[];archives=[];llvm=[];retained={'/baseline/libnano_core.a':CORE_PIN}
 for word in link[6:]:
  guest=os.path.normpath('/outputs/'+word) if not word.startswith(('/','-')) else word
  if guest.startswith('/outputs/'):
   current='/baseline/libnano_core.a' if guest=='/outputs/libnano_core.a' else '/old/'+guest[len('/outputs/'):]
   if guest in docs['llvm']:row=docs['llvm'][guest]['object'];llvm.append(guest);kind='llvm'
   else:m.need(guest in docs['archives'] and guest.endswith('.a'));row=docs['archives'][guest]['archive'];archives.append(guest);kind='archive'
   if guest!='/outputs/libnano_core.a':
    m.need(row['path']==str(m.BASE/'core'/guest[len('/outputs/'):]) and current not in mounts);m.old_file(row);mounts[current]=row['path'];retained[current]={'sha256':row['sha256'],'bytes':row['bytes']}
   order.append({'kind':kind,'old':guest,'current':current,'sha256':CORE_PIN['sha256'] if guest=='/outputs/libnano_core.a' else row['sha256']});tail.append(current)
  else:m.need(guest.startswith(('/usr/','/lib/','/lib64/','-')));tail.append(guest)
 m.need(len(archives)==5 and archives.count('/outputs/libnano_core.a')==1 and len(llvm)==2 and set(llvm)==set(docs['llvm']) and len(mounts)==6 and len(retained)==7 and tail.count('/baseline/libnano_core.a')==1)
 recipe={'format':'nano-rpc-simulation-context-recipe-v1','uid':1000,'gid':1000,'architecture':'linux-amd64','image':m.IMAGE,'elfMachine':62,'compileFlags':flags,'coreFlags':core_flags,'linkPrefix':link[:3],'linkTail':tail,'retainedPins':retained,'regressions':REGRESSIONS}
 controls={'recipe.json':(json.dumps(recipe,sort_keys=True)+'\n').encode(),'expected.json':(json.dumps(expected,sort_keys=True)+'\n').encode(),'worker.pl':worker_raw}
 for leaf,raw in controls.items():m.write_new(m.OUT/'controls'/leaf,raw,0o444)
 source_pins={path:expected['/work/'+str(path.relative_to(source))] for path in source.rglob('*') if path.is_file()}
 m.stable_new('new-source-projection',source_pins);m.stable_new('known-controls',{m.OUT/'controls'/leaf:m.digest(raw) for leaf,raw in controls.items()})
 os.chmod(m.OUT/'controls',0o555);os.chmod(m.OUT/'baseline',0o555)
 m.save('source-recipe-joins.private.json',{'sourceBase':HEAD,'coreSourceBase':CORE_SOURCE_REVISION,'sourceTar':pins['source_tar'],'sourceExpected':pins['expected_json'],'worker':pins['worker'],'selectedSourcePins':SOURCE_PINS,'coreCompileRecord':archive['objects'][rpc_guest]['compile'],'testCompileRecord':rec['compile'],'linkOrder':order,'coreProducer':CORE_BINDING_PIN,'coreHostAuthority':CORE_HOST_PIN,'baselineEvidenceReused':False,'oldImageDependenciesUsed':False,'fullCmakeConfigureExecuted':False})
 docker=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',str(m.OUT/'docker-client')]
 def dk(name,args,seconds=30,expected=0,cleanup=False):return m.run(name,docker+args,seconds,expected,cleanup=cleanup)
 name='nano-rpc-context-amd-'+uuid.uuid4().hex;labelkey='nano.rpc.context.amd';cid=None;attempted=False;first_error=None;result=None
 try:
  raw,err,_=dk('image-inspect',['image','inspect',m.IMAGE,'--format','{{json .}}']);image=json.loads(raw);m.need(not err and image['Id']==m.IMAGE and image['Architecture']=='amd64' and image['Os']=='linux')
  raw,err,_=dk('name-absent',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}']);m.need(not raw.strip() and not err)
  extra=[]
  for guest,host in sorted(mounts.items()):extra+=['--mount','type=bind,src='+host+',dst='+guest+',readonly']
  args=['create','--pull=never','--name',name,'--label',labelkey+'='+name,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','128','--user','1000:1000','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(core)+',dst=/baseline/libnano_core.a,readonly','--mount','type=bind,src='+str(m.OUT/'controls')+',dst=/controls,readonly','--mount','type=bind,src='+str(m.OUT/'outputs')+',dst=/outputs',*extra,'--workdir','/outputs','--entrypoint','/bin/sh',m.IMAGE,'-ec','exec /bin/sleep 1800']
  m.fence();attempted=True;raw,err,_=dk('create-context',args);cid=raw.decode().strip();m.need(not err and re.fullmatch('[0-9a-f]{64}',cid));raw,err,_=dk('start-context',['start',cid]);m.need(not err and raw.decode().strip()==cid)
  raw,err,_=dk('worker-syntax',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-c','/controls/worker.pl']);m.need(not raw and err.strip()==b'/controls/worker.pl syntax OK')
  m.save('original-selected-inputs-before-worker.private.json',m.HELD)
  raw,err,code=dk('rpc-context-worker',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','/controls/worker.pl','--rpc-simulation-context-qualification'],1200,expected=None)
  # Inspect the retained outputs before requiring success; refusal logs survive.
  observed=[];output_pins={};total=0
  for path in sorted((m.OUT/'outputs').iterdir()):
   row,body=m.material(path,67108864);m.need(re.fullmatch('[A-Za-z0-9_.-]+',path.name) and row['st_nlink']==1 and row['st_uid']==row['st_gid']==1000);total+=row['st_size'];m.need(total<=268435456);observed.append({'name':path.name,'bytes':row['st_size']});output_pins[path]=m.digest(body)
  m.need(len(observed)<=192);m.save('worker-output-inventory.private.json',{'observationalOnly':True,'workerExitCode':code,'files':observed,'totalBytes':total,'noSuccessArtifactRequiredForCollection':True})
  m.need(code==0 and not raw and not err)
  result_body=m.material(m.OUT/'outputs/result-public.json',16777216)[1];result=json.loads(result_body)
  m.need(result['format']=='nano-rpc-simulation-context-qualification-v1' and result['passed'] is True and result['architecture']=='linux-amd64' and result['image']==m.IMAGE and result['causalBaselineFailures']==4 and result['newGroups']==4 and result['regressions']==[q['role'] for q in REGRESSIONS])
  m.need(result['coreMembers']==143 and result['unchangedCoreMembers']==142 and result['replacedCoreMembers']==['rpc.cpp.o'] and result['consensusQualified'] is False and result['productionQualified'] is False and result['oldAuthoritiesRefreshed'] is False)
  admission_body=m.material(m.OUT/'outputs/software-admission.private.json',16777216)[1];admission=json.loads(admission_body)
  m.need(admission['format']=='nano-rpc-simulation-context-software-v1' and admission['oldAuthoritiesRefreshed'] is False and admission['guestMaterials']['/outputs/libnano_core.a']==result['newCore'])
  expected_steps={}
  labels=['context','rpc.cpp',*[q['role'] for q in REGRESSIONS]]
  for label in labels:
   for suffix in ('-m','-cxx'):expected_steps[label+suffix]=0
  executable_labels=['context-baseline','nano-rpc-simulation-context-tests',*[q['role'] for q in REGRESSIONS]]
  for label in executable_labels:
   for suffix in ('-link','-loader'):expected_steps[label+suffix]=0
  for group in ('fees','balances','nonces','bounds'):expected_steps['baseline-'+group]=1
  for name2 in ('context-all','copy-core','replace-rpc-member',*[q['role']+'-regression' for q in REGRESSIONS]):expected_steps[name2]=0
  steps=admission['steps'];m.need(isinstance(steps,list) and len(steps)==len(expected_steps)==30 and len({z['step'] for z in steps})==30 and {z['step']:z['exitCode'] for z in steps}==expected_steps)
  causal=json.loads(m.material(m.OUT/'outputs/causal-baseline.json',16777216)[1]);m.need(causal['expectedCausalFailures']==4 and causal['oldSuiteExecuted'] is False and len(causal['newDisjointGroups'])==4)
  messages={'fees':'zero-price simulation must zero BASEFEE only in its copy and transfer value\n','balances':'upfront effective gas purchase/context differs\n','nonces':'sender nonce must increment before nested CREATE\n'}
  for q,group,checks in zip(causal['newDisjointGroups'],('fees','balances','nonces','bounds'),(3,2,2,1)):
   z=q['error'];m.need(q['group']==group and q['exitCode']==1 and z['passed'] is False and z['stage']==group and z['groups']==0 and z['checks']==checks and z['expectedRefusals']==0 and len(z['caseRecords'])==1)
   response=z['caseRecords'][0]['response'];m.need(response['jsonrpc']=='2.0' and response['id']==17 and 'result' in response and 'error' not in response)
   m.need(q['diagnostic']==messages[group] if group in messages else q['diagnostic'].startswith('eth_call expected refusal -32602: '))
  events=json.loads(m.material(m.OUT/'outputs/candidate-events.json',16777216)[1]);m.need(isinstance(events,list) and len(events)==5 and events[-1]['passed'] is True and events[-1]['groups']==4 and events[-1]['checks']==476 and events[-1]['expectedRefusals']==126)
  m.need([len(z['caseRecords']) for z in events[:4]]==[34,66,32,126])
  continuity=json.loads(m.material(m.OUT/'outputs/core-member-continuity.json',16777216)[1]);m.need(continuity['passed'] is True and continuity['members']==143 and continuity['unchangedMembers']==142 and continuity['replacedMember']=='rpc.cpp.o' and continuity['newCore']==result['newCore'])
  # First host admission is prospective. Original worker rows remain mandatory.
  m.stable_new('qualified-host-outputs',output_pins)
  for leaf in ('libnano_core.a',*executable_labels):
   path=m.OUT/'outputs'/leaf;worker_row=admission['guestMaterials']['/outputs/'+leaf];m.need(m.HELD[str(path)]==guest_row(worker_row))
  result.update({'rootHead':HEAD,'rootCoreSourceBase':CORE_SOURCE_REVISION,'rootCurrentCoreProducerAndHostNineJoined':True,'rootExternalSourceProjectionPinsVerified':True,'sourceGitProvenanceVerifiedByHost':False,'rootCanonicalRpcCompileRecordSelected':True,'rootCanonicalCoreOnlyConsumerRecipeSelected':True,'rootSelectedOriginalRowsFullNineAndShaUnchanged':True,'rootNewHostOutputsMatchedOriginalGuestRows':True,'rootPerlSyntaxPassed':True,'independentMCommands':5,'compiledTranslationUnits':5,'linkCommands':5,'loaderCommands':5,'archiveUpdates':1,'selectedTestCommands':8,'newContextChecks':476,'newContextExpectedRefusals':126,'sourceProjectionSha256':pins['source_tar']['sha256'],'workerSha256':pins['worker']['sha256'],'workerResultSha256':m.digest(result_body)['sha256'],'workerResultBytes':len(result_body),'softwareAdmissionSha256':m.digest(admission_body)['sha256'],'softwareAdmissionBytes':len(admission_body),'fixedProducerReceiptsRead':4,'oldSdkMapsRead':False,'oldSourceAdmissionRead':False,'fullCmakeConfigureExecuted':False,'imageBuilds':0,'consensusDutiesStarted':0,'campaignAdmission':False,'productionQualified':False})
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
 result['rootOwnedContainerAbsent']=True;m.save('result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 m=None;failed=False
 try:m=load_adapter();main(m)
 except BaseException:
  failed=True
  if m is not None and m.OUT_CREATED:
   try:m.save('failure-public.json',{'passed':False,'oldSdkMapsRead':False,'oldSourceAdmissionRead':False,'oldAuthoritiesRefreshed':False,'compilerOrTestSuccessClaimed':False,'rawErrorDetailsExported':False})
   except BaseException:pass
  print('AMD RPC context adapter refused',file=sys.stderr);sys.exit(1)
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
