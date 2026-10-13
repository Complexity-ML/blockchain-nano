#!/usr/bin/env python3
"""ROOT-dispatched only: seven new AMD64 CTests on one fresh offline container.
Prepared without Docker, SSH, compilation, tests, DB or campaign input reads.
"""
import argparse, hashlib, json, os, pathlib, re, signal, stat, subprocess, sys, time, uuid
BASE = None; SOURCE = None; BUILD = None
ORIGINAL_INPUT_SHA = '28cd003fdc76f4af8a6c831ae0c82d2f7e0f01a39a3e1c09af787f921f93e323'
EXTRA_SOURCE = {'tools/storage-maintenance/rocksdb-owned-copy.hpp':'793a4eeec7344436f81806b773cbb43f2136523e73b2de28f9d6bf793e357f31'}
GIT_COMMIT = 'f9d078d0d45d6081c8c8745a80575d898335ec3e'
IMAGE = 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
FAULT_SHA = '1c5fbe9a94c9e4137f0ae41999390a9a9690e63324998bfeff6368609345d86e'
AUDITOR_SHA = 'ce4a459628b096de2d538422d9d63bbf318f3cd921e0761f4ff8a6f9898a7d33'
DOCKER = ['/usr/bin/sudo','-n','/usr/bin/docker']
NEW = ['storage_capella_blob_gc_next_publication','storage_capella_blob_gc_adversarial_boundaries','storage_capella_blob_gc_natural_pruning','storage_capella_blob_gc_cli_closed_fixture']
REG = ['storage_checkpoint_immutable_blobs','consensus_capella_fragment_checkpoint_migration']
AVAILABILITY = 'consensus_capella_duty_projection_availability'
SELECTED = NEW + REG + [AVAILABILITY]; GRAPH_SELECTED = SELECTED
PROGRAMS = ['nano','nano-capella-transition-tests','nano-storage-maintenance','nano-checkpoint-blob-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit','nano-capella-duty-availability-tests']
ENV3 = ['NANO_TEST_CHECKPOINT_BLOB_GC=1','NANO_TEST_COLD_REACTOR=1','NANO_TEST_FRAGMENT_CHECKPOINT=1']
EXPECTED = {
 NEW[0]: ['/usr/bin/cmake','-E','env'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 NEW[1]: ['/usr/bin/cmake','-E','env','NANO_TEST_CHECKPOINT_BLOB_GC_ADVERSARIAL=1'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 NEW[2]: ['/outputs/nano-capella-transition-tests','--blob-gc-natural'],
 NEW[3]: ['/usr/bin/cmake','-E','env','NANO_TEST_CHECKPOINT_BLOB_GC_CLI=1','NANO_TEST_CHECKPOINT_GC_TOOL=/outputs/nano-storage-maintenance'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 REG[0]: ['/outputs/nano-checkpoint-blob-tests'],
 REG[1]: ['/usr/bin/cmake','-E','env','NANO_TEST_FRAGMENT_CHECKPOINT=1','/outputs/nano-capella-transition-tests','--prepared-block'],
 AVAILABILITY: ['/outputs/nano-capella-duty-availability-tests','--self-test']}
ADMISSION_CTIME_DIAGNOSTICS = []
ALLOWED_STAT = {'dev':'st_dev','ino':'st_ino','uid':'st_uid','gid':'st_gid','mode':'st_mode','bytes':'st_size','mtimeNs':'st_mtime_ns','ctimeNs':'st_ctime_ns'}
def require(ok, why):
 if not ok: raise ValueError(why)
def sha(data): return hashlib.sha256(data).hexdigest()
def canonical(path, exists=True):
 p = pathlib.Path(path); require(p.is_absolute() and str(p)==str(p.resolve(strict=exists)), 'canonical absolute path required: '+str(p)); return p
def bytes_file(path, maximum):
 p=canonical(path); s=os.lstat(p); require(stat.S_ISREG(s.st_mode) and not stat.S_ISLNK(s.st_mode), 'regular public artifact required')
 with p.open('rb') as f: data=f.read(maximum+1)
 require(len(data)<=maximum, 'public artifact size budget'); return data
def pinned(path, expected, maximum=8*1024*1024):
 require(isinstance(expected,str) and re.fullmatch('[0-9a-f]{64}',expected), 'strict external SHA required')
 data=bytes_file(path,maximum); require(sha(data)==expected,'artifact pin mismatch: '+str(path)); return data
def json_pin(path, expected, maximum=8*1024*1024): return json.loads(pinned(path,expected,maximum))
def digest_file(path):
 p=canonical(path); a=os.lstat(p); require(stat.S_ISREG(a.st_mode),'regular artifact required'); h=hashlib.sha256()
 with p.open('rb') as f:
  for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
 b=os.lstat(p); require((a.st_dev,a.st_ino,a.st_size,a.st_mtime_ns,a.st_ctime_ns)==(b.st_dev,b.st_ino,b.st_size,b.st_mtime_ns,b.st_ctime_ns),'artifact changed during hash')
 return {'sha256':h.hexdigest(),**{k:getattr(b,v) for k,v in ALLOWED_STAT.items()}}
def root_identity(path):
 p=canonical(path); s=os.lstat(p); require(stat.S_ISDIR(s.st_mode),'directory root required'); return {'path':str(p),'dev':s.st_dev,'ino':s.st_ino,'uid':s.st_uid,'gid':s.st_gid,'mode':stat.S_IMODE(s.st_mode)}
def compare_stat(actual, expected, why):
 require(isinstance(expected,dict),'bound stat record required'); require(actual['sha256']==expected.get('sha256'),why+' SHA')
 for key in ALLOWED_STAT:
  require(key in expected and type(expected[key]) is int,why+' incomplete stat '+key)
  if key=='ctimeNs':
   if actual[key]!=expected[key]: ADMISSION_CTIME_DIAGNOSTICS.append({'artifact':why,'boundCtimeNs':expected[key],'actualCtimeNs':actual[key],'sameContentAndIdentityRequired':True,'causalWriterNotEstablished':True})
   continue
  value = stat.S_IMODE(actual[key]) if key=='mode' and expected[key] <= 0o7777 else actual[key]
  require(value==expected[key],why+' stat '+key)
def source_inventory(inputs):
 expected=dict(inputs['parentSourceHashes'])
 for row in inputs['consolidatedSources']: expected[row['path']]=row['sha256']
 extra=inputs.get('additionalPublicSourceInputs'); require(isinstance(extra,list) and len(extra)==1 and extra[0]=={'path':'tools/storage-maintenance/rocksdb-owned-copy.hpp','sha256':EXTRA_SOURCE['tools/storage-maintenance/rocksdb-owned-copy.hpp'],'bytes':1942,'gitCommit':GIT_COMMIT,'historicallyIncludedInParentSourceMap':False},'single reviewed current Git public tool header required')
 expected.update(EXTRA_SOURCE)
 portable=inputs['portableAvailabilitySource']; expected[portable['path']]=portable['sha256']; out={}
 require(expected and len(expected)<=4096,'finite public source scope required')
 for rel,h in expected.items():
  require(isinstance(rel,str) and pathlib.PurePosixPath(rel).as_posix()==rel and not rel.startswith('/') and '..' not in pathlib.PurePosixPath(rel).parts and '\n' not in rel and not rel.endswith('.env'),'public source relative path')
  require(isinstance(h,str) and re.fullmatch('[0-9a-f]{64}',h),'exact source SHA required')
  data=digest_file(SOURCE/rel); require(data['sha256']==h,'exact delivered source mismatch: '+rel); out[rel]=data
 return out

def cmake_tokens(text):
 out=[]; i=0
 while i<len(text):
  if text[i].isspace(): i+=1; continue
  match=re.match(r'\[(=*)\[',text[i:])
  if match:
   end=']'+match.group(1)+']'; start=i+len(match.group(0)); at=text.find(end,start); require(at>=0,'CMake bracket token'); out.append(text[start:at]); i=at+len(end); continue
  if text[i]=='"':
   i+=1; token=[]
   while i<len(text) and text[i]!='"':
    if text[i]=='\\':
     i+=1; require(i<len(text),'CMake escape'); require(text[i] in '\\";nrt','unexpected CMake escape'); token.append({'n':'\n','r':'\r','t':'\t'}.get(text[i],text[i])); i+=1
    else: token.append(text[i]); i+=1
   require(i<len(text),'CMake quoted token'); i+=1; out.append(''.join(token)); continue
  at=i
  while i<len(text) and not text[i].isspace(): i+=1
  token=text[at:i]; require(not any(c in token for c in '$()'),'unexpected bare CMake token'); out.append(token)
 return out

def graph_ctest(destination):
 pending=[pathlib.Path('.')]; visited=set(); graph=[]; commands={}; properties={}; count_bytes=0
 while pending:
  rel=pending.pop(); require(rel not in visited,'CTest subdir cycle/duplicate'); visited.add(rel); require(len(visited)<=64,'CTest graph directory budget')
  source_dir=canonical(BUILD/rel); require(source_dir.is_relative_to(BUILD),'CTest graph escapes build'); dest=destination/rel; dest.mkdir(mode=0o700,parents=True,exist_ok=True); file=source_dir/'CTestTestfile.cmake'
  if not file.exists(): graph.append({'path':rel.as_posix()+'/CTestTestfile.cmake','present':False}); continue
  data=bytes_file(file,2*1024*1024); count_bytes+=len(data); require(count_bytes<=8*1024*1024,'CTest metadata budget'); target=dest/'CTestTestfile.cmake'
  fd=os.open(target,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
  with os.fdopen(fd,'wb') as f: f.write(data); f.flush(); os.fsync(f.fileno())
  require(target.read_bytes()==data,'CTest metadata copy changed'); graph.append({'path':rel.as_posix()+'/CTestTestfile.cmake','sha256':sha(data),'bytes':len(data),'present':True})
  for line in data.decode('utf-8','strict').splitlines():
   line=line.strip()
   if not line or line.startswith('#'): continue
   m=re.fullmatch(r'(add_test|set_tests_properties|subdirs)\((.*)\)',line); require(m is not None,'unsupported executable CTest metadata')
   args=cmake_tokens(m.group(2)); kind=m.group(1)
   if kind=='subdirs':
    for item in args:
     child=pathlib.PurePosixPath(item); require(not child.is_absolute() and '..' not in child.parts and child.as_posix()==item,'CTest relative child required'); pending.append(rel/pathlib.Path(item))
   elif kind=='add_test' and args and args[0] in GRAPH_SELECTED:
    name=args[0]; require(name not in commands,'duplicate selected test'); commands[name]=args[1:]
   elif kind=='set_tests_properties' and 'PROPERTIES' in args:
    at=args.index('PROPERTIES'); values=args[at+1:]; require(len(values)%2==0,'CTest properties pairs'); props=dict(zip(values[::2],values[1::2]))
    for name in args[:at]:
     if name in GRAPH_SELECTED: require(name not in properties,'duplicate selected properties'); properties[name]=props
 require(set(commands)==set(GRAPH_SELECTED),'seven registered selected tests missing')
 for name in GRAPH_SELECTED:
  require(commands[name]==EXPECTED[name],'selected command differs: '+name); props=properties.get(name,{}); require(set(props)<= {'TIMEOUT','LABELS','_BACKTRACE_TRIPLES'},'unsafe selected test property'); require(props.get('TIMEOUT')==('70' if name==AVAILABILITY else '90' if name==REG[0] else '180'),'selected timeout differs')
 return {'graph':graph,'selectedCommands':commands,'selectedProperties':properties}

def save(path,value):
 raw=(json.dumps(value,indent=2)+'\n').encode(); fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
 with os.fdopen(fd,'wb') as f: f.write(raw); f.flush(); os.fsync(f.fileno())
 return sha(raw)

def normalize_build_stat(row):
 require(isinstance(row,dict),'actual build stat required'); out={'sha256':row.get('sha256')}
 for dest,src in ALLOWED_STAT.items(): require(type(row.get(src)) is int,'incomplete actual build stat '+src); out[dest]=row[src]
 return out

def main():
 global BASE,SOURCE,BUILD
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['controller-sha','public-inputs','public-inputs-sha','build-root','build-controller-sha','build-terminal','build-terminal-sha','output']: p.add_argument('--'+name,required=True)
 a=p.parse_args(); self_path=canonical(__file__); pinned(self_path,a.controller_sha,1024*1024)
 BASE=canonical(a.build_root); require(BASE.parent==pathlib.Path('/home/ubuntu/nanobees-qualification') and re.fullmatch('checkpoint-blob-gc-coherent-amd64-v[0-9]{1,2}-20261013',BASE.name),'explicit owned current AMD build root required'); SOURCE=BASE/'source'; BUILD=BASE/'build'
 terminal_path=canonical(a.build_terminal); require(terminal_path==BASE/'receipts/terminal-build-receipt.json','exact passed build-root receipt location required')
 inputs_path=canonical(a.public_inputs); inputs=json_pin(inputs_path,a.public_inputs_sha,16*1024*1024)
 require(inputs.get('format')=='nano-gc-amd64-public-incremental-inputs-v2' and inputs.get('deliveredGitCommit')==GIT_COMMIT,'exact delivered public input bundle required')
 require(os.geteuid()==1000 and os.getegid()==1000,'VPS host UID1000:GID1000 required')
 out=canonical(a.output,False); require(out.parent==BASE and re.fullmatch('tests-root-dispatched-[a-z0-9-]{1,64}',out.name),'fresh passed-build test output namespace required'); os.mkdir(out,0o700)
 auditor=canonical(self_path.parent/'audit-amd64-seven-tests.py'); pinned(auditor,AUDITOR_SHA,1024*1024)
 events=out/'events.jsonl'; run_name='nano-gc-amd-'+uuid.uuid4().hex[:16]; ownership=uuid.uuid4().hex
 proc=None; launched=False; exit_code=None; process_completed=False; timed_out=False; container_absent=False; failure=None; invariants=False; audit=None; before=None; absence_observed=False
 env={k:v for k,v in os.environ.items() if not k.startswith(('NANO_','LD_','DYLD_'))}; env['LC_ALL']='C'
 def event(kind,**extra):
  row={'event':kind,'monotonicSeconds':time.monotonic(),**extra}
  with events.open('a',encoding='utf-8') as f: f.write(json.dumps(row)+'\n'); f.flush()
  print(json.dumps(row),flush=True)
 def command(label,args,timeout=25):
  save(out/(label+'.command.json'),args); start=time.monotonic(); result=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout,env=env)
  (out/(label+'.log')).write_bytes(result.stdout); save(out/(label+'.exit.json'),{'exitCode':result.returncode,'elapsedSeconds':time.monotonic()-start,'processCompleted':True}); return result
 def absent(label):
  r=command(label,DOCKER+['container','inspect','--format','{{json .Id}}',run_name])
  if r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout): return True
  if r.returncode==0:
   found=json.loads(r.stdout); require(isinstance(found,str) and re.fullmatch('[0-9a-f]{64}',found),'container ID binding'); return False
  raise ValueError('container absence was not proven')
 def cleanup():
  if absent('cleanup-inspect-before'): return True
  r=command('cleanup-owner-inspect',DOCKER+['container','inspect','--format','{{json .Config.Labels}}',run_name]); require(r.returncode==0,'container owner inspection failed'); found=json.loads(r.stdout); require(isinstance(found,dict) and found.get('nano.gc.owner')==ownership,'refuse removing unowned container')
  r=command('cleanup-owned-remove',DOCKER+['container','rm','--force',run_name]); require(r.returncode==0,'owned container removal failed'); return absent('cleanup-inspect-after')
 try:
  event('preflight-start',container=run_name)
  build=json_pin(terminal_path,a.build_terminal_sha)
  require(isinstance(a.build_controller_sha,str) and re.fullmatch('[0-9a-f]{64}',a.build_controller_sha),'external actual build-controller SHA required')
  require(build.get('format')=='nano-gc-amd64-incremental-build-terminal-v1' and build.get('status')=='BUILD_PASS_TESTS_NOT_EXECUTED','actual passed AMD build required')
  require(build.get('controllerSha256')==a.build_controller_sha and build.get('publicInputsSha256')==a.public_inputs_sha and build.get('gitCommit')==GIT_COMMIT and build.get('image')==IMAGE,'actual build provenance differs')
  require(build.get('coreFresh')==41 and build.get('coreReused')==99 and build.get('archiveMembers')==140 and build.get('freshConsumers')==8 and build.get('dependencyOnlyProbes')==49,'exact fresh/reused AMD closure required')
  for flag in ['parentSourceLibraryImageInputsUnchanged','sameImageSdkBeforeAfterObserved','actualCompiledDependenciesEqualPreflight','archiveRawBytesComparedTo140Objects','historicallyPinnedDependencyCopiesBeforeAfterEqual','parentGeneratedMetadataBeforeAfterEqual']: require(build.get(flag) is True,'actual build proof missing: '+flag)
  require(build.get('oldSdkHashesRetroactivelyInvented') is False and build.get('testsExecuted')==0 and build.get('runtimeStarted') is False and build.get('privateInputsRead') is False and build.get('imageBuiltOrPulled') is False and build.get('productionQualified') is False,'build scope invalid')
  require(set(inputs['consumers'])==set(PROGRAMS),'exact eight consumer paths required')
  source_before=source_inventory(inputs); artifacts={'libnano_core.a':digest_file(BUILD/'libnano_core.a')}; compare_stat(artifacts['libnano_core.a'],normalize_build_stat(build['archive']),'archive')
  bound={row['target']:row for row in build.get('programs',[])}; require(len(build.get('programs',[]))==8 and set(bound)==set(PROGRAMS),'exact eight actual ELF/object bindings required')
  for name in PROGRAMS:
   record=digest_file(BUILD/name); compare_stat(record,normalize_build_stat(bound[name]['binary']),name); require(bound[name].get('coreArchiveSha256')==artifacts['libnano_core.a']['sha256'],'consumer core archive differs'); artifacts[name]=record
   object_rel='CMakeFiles/'+name+'.dir/'+inputs['consumers'][name]+'.o'; record=digest_file(BUILD/object_rel); compare_stat(record,normalize_build_stat(bound[name]['object']),name+' object'); artifacts[object_rel]=record
  core_paths=[row['object'] for row in inputs['core']]+['CMakeFiles/nano_core.dir/'+inputs['newCoreSource']+'.o']; require(len(core_paths)==140 and len(set(core_paths))==140,'exact140 unique core objects required')
  for rel in core_paths: artifacts[rel]=digest_file(BUILD/rel)
  require(len(inputs['expectedDependencyArtifacts'])==7,'seven pinned dependency artifacts required')
  for rel,pin in inputs['expectedDependencyArtifacts'].items():
   record=digest_file(BUILD/rel); require(record['sha256']==pin['sha256'] and record['bytes']==pin['bytes'],'historically pinned dependency copy differs: '+rel); artifacts[rel]=record
  require(artifacts['libnano-test-sync-fault.so']['sha256']==FAULT_SHA,'actual AMD fault shim differs')
  meta={}
  for target in PROGRAMS:
   prefix=BUILD/'CMakeFiles'/(target+'.dir'); flags=bytes_file(prefix/'flags.make',1024*1024); link=bytes_file(prefix/'link.txt',1024*1024); meta[target]={'flagsSha256':sha(flags),'linkSha256':sha(link),'flagsText':flags.decode(),'linkText':link.decode()}
   if target in ['nano-capella-transition-tests','nano-checkpoint-blob-tests']:
    require(re.search(r'(?:^|\s)-DNANO_SYNC_FAULT=1(?:\s|$)',flags.decode()),'NANO_SYNC_FAULT compile definition missing'); require('libnano-test-sync-fault.so' in link.decode() and re.search(r'-Wl,-rpath[^\n]*?/outputs',link.decode()),'actual fault shim/RPATH missing')
   else: require('libnano-test-sync-fault' not in link.decode(),'fault shim linked into non-fault program')
  artifacts['compile_commands.json']=digest_file(BUILD/'compile_commands.json')
  save(out/'admission-ctime-diagnostics.json',{'scope':'actual test admission versus earlier build terminal only','differences':ADMISSION_CTIME_DIAGNOSTICS,'otherAdmissionFieldsStrict':True,'allRunBeforeAfterMetadataEqualityStrict':True})
  testing=out/'testing'; testing.mkdir(mode=0o700); (testing/'ctest').mkdir(mode=0o700); graph=graph_ctest(testing/'ctest'); save(out/'selected-ctest-graph.json',graph)
  before={'controllerSha256':a.controller_sha,'auditorSha256':AUDITOR_SHA,'buildControllerSha256':a.build_controller_sha,'buildTerminalSha256':a.build_terminal_sha,'publicInputsSha256':a.public_inputs_sha,'publicInputsPath':str(inputs_path),'originalPublicInputsSha256':ORIGINAL_INPUT_SHA,'gitCommit':GIT_COMMIT,'source':source_before,'roots':[root_identity(SOURCE),root_identity(BUILD)],'artifacts':artifacts,'metadata':meta,'ctestGraph':graph}; save(out/'before.json',before)
  r=command('image-inspect',DOCKER+['image','inspect','--format','{{json .}}',IMAGE]); require(r.returncode==0,'existing image inspect failed'); image=json.loads(r.stdout); require(isinstance(image,dict) and image['Id']==IMAGE and image.get('Architecture')=='amd64' and image.get('Os')=='linux','existing image identity/platform'); config=image.get('Config',{}); require(config.get('Entrypoint') in (None,[]),'image entrypoint refused'); require(config.get('Cmd')==['/bin/bash'],'unexpected image command'); names=[v.split('=',1)[0] for v in config.get('Env',[])]; require(len(names)==3 and set(names)=={'PATH','CC','CXX'},'unexpected image environment'); save(out/'image-binding.json',{'image':IMAGE,'architecture':'amd64','os':'linux','environmentNames':names,'imageBuiltOrPulled':False})
  absence_observed=absent('before-launch-container-inspect'); require(absence_observed,'named container already exists')
  selected='^('+'|'.join(SELECTED)+')$'
  argv=DOCKER+['run','--rm','--pull=never','--platform','linux/amd64','--name',run_name,'--label','nano.gc.owner='+ownership,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','1024m','--pids-limit','256','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m,uid=1000,gid=1000,mode=700','--mount',f'type=bind,src={SOURCE},dst=/work,readonly','--mount',f'type=bind,src={BUILD},dst=/outputs,readonly','--mount',f'type=bind,src={testing},dst=/testing','--env','TMPDIR=/tmp','--workdir','/testing/ctest',IMAGE,'/usr/bin/ctest','--test-dir','/testing/ctest','--tests-regex',selected,'--no-tests=error','--output-on-failure','--verbose','--output-junit','/testing/gc-selected.junit.xml']
  save(out/'ctest.command.json',argv); event('ctest-launch',selected=SELECTED,deadlineSeconds=1200)
  with (out/'ctest.log').open('wb',buffering=0) as output:
   proc=subprocess.Popen(argv,stdout=output,stderr=subprocess.STDOUT,env=env,start_new_session=True); launched=True
   try: exit_code=proc.wait(timeout=1200); process_completed=True
   except subprocess.TimeoutExpired:
    timed_out=True; os.killpg(proc.pid,signal.SIGTERM)
    try: exit_code=proc.wait(timeout=3)
    except subprocess.TimeoutExpired: os.killpg(proc.pid,signal.SIGKILL); exit_code=proc.wait(timeout=10)
    process_completed=True; raise ValueError('CTest total deadline exceeded; no relaunch')
  event('ctest-terminal',exitCode=exit_code); require(exit_code==0,'actual CTest command failed; no relaunch')
 except BaseException as error:
  failure=type(error).__name__+': '+str(error); event('failed',reason=failure)
 finally:
  if proc is not None and proc.poll() is None:
   try: os.killpg(proc.pid,signal.SIGKILL); exit_code=proc.wait(timeout=10); process_completed=True
   except BaseException as error: failure=(failure or '')+'; client cleanup '+str(error)
  try: container_absent=cleanup() if proc is not None else absence_observed
  except BaseException as error: failure=(failure or '')+'; owned container cleanup '+str(error)
  if before is not None:
   try:
    source_after=source_inventory(inputs); artifacts_after={name:digest_file(BUILD/name) for name in before['artifacts']}; roots_after=[root_identity(SOURCE),root_identity(BUILD)]; metadata_after={}
    for target in before['metadata']:
     prefix=BUILD/'CMakeFiles'/(target+'.dir'); metadata_after[target]={'flagsSha256':sha(bytes_file(prefix/'flags.make',1024*1024)),'linkSha256':sha(bytes_file(prefix/'link.txt',1024*1024))}
    graph_after={}
    for row in before['ctestGraph']['graph']:
     if row['present']:
      rel=pathlib.Path(row['path']); data=bytes_file(BUILD/rel,2*1024*1024); copy=bytes_file(testing/'ctest'/rel,2*1024*1024); require(sha(data)==row['sha256'] and copy==data,'CTest graph changed during test'); graph_after[row['path']]=sha(data)
    invariants=source_after==before['source'] and artifacts_after==before['artifacts'] and roots_after==before['roots'] and all(metadata_after[k]['flagsSha256']==v['flagsSha256'] and metadata_after[k]['linkSha256']==v['linkSha256'] for k,v in before['metadata'].items())
    pinned(inputs_path,a.public_inputs_sha,16*1024*1024); pinned(auditor,AUDITOR_SHA,1024*1024); pinned(self_path,a.controller_sha,1024*1024); pinned(terminal_path,a.build_terminal_sha)
    save(out/'after.json',{'source':source_after,'roots':roots_after,'artifacts':artifacts_after,'metadata':metadata_after,'ctestGraphSourceAndCopyUnchanged':True,'allBindingsUnchanged':invariants}); require(invariants,'source/artifact/root/metadata changed during tests')
   except BaseException as error: failure=(failure or '')+'; immutable binding '+str(error)
  execution={'format':'nano-gc-amd64-seven-selected-execution-terminal-v1','exitCode':exit_code,'processCompleted':process_completed,'containerAbsentAfterCleanup':container_absent,'timedOut':timed_out,'launched':launched,'bindingInvariantsHeld':invariants,'failure':failure,'privateInputsRead':False,'runtimeDutiesResumed':False,'productionQualified':False}; terminal_hash=save(out/'execution-terminal.json',execution)
 if failure is None and exit_code==0 and process_completed and container_absent and invariants:
  try:
   junit=out/'testing/gc-selected.junit.xml'; log=out/'ctest.log'; args=[sys.executable,'-I','-S',str(auditor),'--junit',str(junit),'--junit-sha',digest_file(junit)['sha256'],'--log',str(log),'--log-sha',digest_file(log)['sha256'],'--terminal',str(out/'execution-terminal.json'),'--terminal-sha',terminal_hash,'--before',str(out/'before.json'),'--before-sha',digest_file(out/'before.json')['sha256'],'--build-terminal',str(terminal_path),'--build-terminal-sha',a.build_terminal_sha]; r=command('public-result-audit',args,30); require(r.returncode==0,'scoped AMD public result audit failed'); audit=json.loads(r.stdout); require(audit.get('passedWithinTestScope') is True and audit.get('productionQualified') is False,'scoped audit verdict invalid'); save(out/'public-result-audit.json',audit)
  except BaseException as error: failure=type(error).__name__+': '+str(error)
 result={'format':'nano-gc-amd64-root-dispatched-seven-result-v1','passedWithinTestScope':failure is None and audit is not None,'selectedTests':SELECTED,'allSevenActuallyExecutedRequired':True,'priorArmResultsReused':False,'failure':failure,'executionTerminalSha256':terminal_hash,'actualContainerAbsent':container_absent,'actualBindingsUnchanged':invariants,'noRelaunchAttempted':True,'privateInputsRead':False,'runtimeDutiesResumed':False,'productionQualified':False}; save(out/'result.json',result); event('root-dispatched-terminal',passedWithinTestScope=result['passedWithinTestScope'],failure=failure); return 0 if result['passedWithinTestScope'] else 1
if __name__=='__main__': sys.exit(main())
