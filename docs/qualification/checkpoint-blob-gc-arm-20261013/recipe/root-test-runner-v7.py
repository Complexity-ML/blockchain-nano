#!/usr/bin/env python3
"""ROOT-dispatched only: one fresh offline container, two GC retries; four prior PASS cases remain separate.
This source was prepared without executing Docker, C++, tests or DB readers.
"""
import argparse, hashlib, json, os, pathlib, re, signal, stat, subprocess, sys, time, uuid
E = pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
BASE = E / 'checkpoint-blob-gc-arm-qualification-20261013'
SOURCE = BASE / 'source-testfix2'; BUILD = BASE / 'build-testfix2'
INDEX = BASE / 'source-after-testfix2.json'; INDEX_SHA = 'e89d6eab8eca7faf8492b26c8971e4a566ce159659fb3835c31ef5beb7e054f8'
HARNESS = E / 'checkpoint-blob-gc-cli-harness-20261013'
HARNESS_SHA = 'df46eecf052e3d97b09441c622213c1bba893116880e66588a53328f33ddeeb4'
BASE_PLAN = E / 'checkpoint-blob-gc-candidate-v2-20261013/docker-arm-build-full-plan.json'
BASE_PLAN_SHA = '29e86bb8272b48640656f560cdeed08112c6f555c0ed6f6ef4f15707f95fb7b8'
PARENT_TERMINAL = BASE / 'build-fixed1-driver-receipts-completion1/terminal-build-receipt.json'
PARENT_TERMINAL_SHA = 'd53fb809a3cb14008838e754f6fa21a0c02afe1331e78f32314d3785ad74f1da'
PARENT_ARCHIVE = BASE / 'build-fixed1/libnano_core.a'
PARENT_ARCHIVE_SHA = '02b718039b73f1f49d0f96ef4c09ebe410a55f2578d82755f6f48da6109d7a96'
TEST_ONLY_MANIFEST = E / 'checkpoint-blob-gc-crash-gate-natural56-20261013/manifest.json'
TEST_ONLY_MANIFEST_SHA = '43bcb3e3610a474ed6902dca5befbb4bdf4253f256534193d49f117274c5c940'
AUDITOR = E / 'checkpoint-blob-gc-root-test-runner-20261013/audit-two-retries-with-four-prior-passes.py'
AUDITOR_SHA = '628507fd0ae9fa11d16652df9061db9690adf93d327f1d4f1eafae93e99c23cf'
IMAGE = 'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
FAULT_SHA = 'cb04d92fae79aa16c26158ddecdddfc70d45e4a53247762129fcf540bfdaebda'
DOCKER = '/Users/boris/.docker/bin/docker'
NEW = ['storage_capella_blob_gc_next_publication','storage_capella_blob_gc_adversarial_boundaries','storage_capella_blob_gc_natural_pruning','storage_capella_blob_gc_cli_closed_fixture']
REG = ['storage_checkpoint_immutable_blobs','consensus_capella_fragment_checkpoint_migration']; GRAPH_SELECTED = NEW + REG; SELECTED = [NEW[1],NEW[2]]
PROGRAMS = ['nano','nano-capella-transition-tests','nano-storage-maintenance','nano-checkpoint-blob-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit']
ENV3 = ['NANO_TEST_CHECKPOINT_BLOB_GC=1','NANO_TEST_COLD_REACTOR=1','NANO_TEST_FRAGMENT_CHECKPOINT=1']
EXPECTED = {
 NEW[0]: ['/usr/bin/cmake','-E','env'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 NEW[1]: ['/usr/bin/cmake','-E','env','NANO_TEST_CHECKPOINT_BLOB_GC_ADVERSARIAL=1'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 NEW[2]: ['/outputs/nano-capella-transition-tests','--blob-gc-natural'],
 NEW[3]: ['/usr/bin/cmake','-E','env','NANO_TEST_CHECKPOINT_BLOB_GC_CLI=1','NANO_TEST_CHECKPOINT_GC_TOOL=/outputs/nano-storage-maintenance'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 REG[0]: ['/outputs/nano-checkpoint-blob-tests'],
 REG[1]: ['/usr/bin/cmake','-E','env','NANO_TEST_FRAGMENT_CHECKPOINT=1','/outputs/nano-capella-transition-tests','--prepared-block']}
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
def source_inventory(index):
 require(index.get('privateDataCopied') is False,'source scope not public'); rows=index.get('files'); require(isinstance(rows,list) and rows,'source index files required'); out={}
 for row in rows:
  rel=row.get('path'); require(isinstance(rel,str) and pathlib.PurePosixPath(rel).as_posix()==rel and not rel.startswith('/') and '..' not in pathlib.PurePosixPath(rel).parts,'source index relative path')
  require(rel not in out,'duplicate source index'); data=digest_file(SOURCE/rel); require(data['sha256']==row.get('sha256') and data['bytes']==row.get('bytes'),'source byte mismatch: '+rel); out[rel]={'sha256':data['sha256'],'bytes':data['bytes']}
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
 require(set(commands)==set(GRAPH_SELECTED),'six registered selected tests missing')
 for name in GRAPH_SELECTED:
  require(commands[name]==EXPECTED[name],'selected command differs: '+name); props=properties.get(name,{}); require(set(props)<= {'TIMEOUT','LABELS','_BACKTRACE_TRIPLES'},'unsafe selected test property'); require(props.get('TIMEOUT')==('90' if name==REG[0] else '180'),'selected timeout differs')
 return {'graph':graph,'selectedCommands':commands,'selectedProperties':properties}

def save(path,value):
 raw=(json.dumps(value,indent=2)+'\n').encode(); fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
 with os.fdopen(fd,'wb') as f: f.write(raw); f.flush(); os.fsync(f.fileno())
 return sha(raw)

def main():
 p=argparse.ArgumentParser(description=__doc__); p.add_argument('--controller-sha',required=True); p.add_argument('--build-plan',required=True); p.add_argument('--build-plan-sha',required=True); p.add_argument('--build-terminal',required=True); p.add_argument('--build-terminal-sha',required=True); p.add_argument('--output',required=True); a=p.parse_args()
 self_path=canonical(__file__); pinned(self_path,a.controller_sha,1024*1024); terminal_path=canonical(a.build_terminal); require(terminal_path.parent.parent==BASE and terminal_path.parent.name=='build-testfix2-driver-receipts','unexpected build receipt location')
 out=canonical(a.output,False); require(out.parent==BASE and re.fullmatch('tests-root-dispatched-[a-z0-9-]{1,64}',out.name),'fresh test output namespace required'); os.mkdir(out,0o700); require(os.geteuid()==501 and os.getegid()==20,'host must be UID501:GID20')
 events=out/'events.jsonl'; run_name='nano-gc-'+uuid.uuid4().hex[:16]; ownership=uuid.uuid4().hex; proc=None; launched=False; exit_code=None; process_completed=False; timed_out=False; container_absent=False; failure=None; invariants=False; audit=None; before=None; absence_observed=False
 env={k:v for k,v in os.environ.items() if not k.startswith(('NANO_','LD_','DYLD_'))}; env['LC_ALL']='C'
 def event(kind,**extra):
  row={'event':kind,'monotonicSeconds':time.monotonic(),**extra}
  with events.open('a',encoding='utf-8') as f: f.write(json.dumps(row)+'\n'); f.flush()
  print(json.dumps(row),flush=True)
 def command(label,args,timeout=25):
  save(out/(label+'.command.json'),args); start=time.monotonic(); result=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout,env=env)
  (out/(label+'.log')).write_bytes(result.stdout); save(out/(label+'.exit.json'),{'exitCode':result.returncode,'elapsedSeconds':time.monotonic()-start,'processCompleted':True}); return result
 def absent(label):
  r=command(label,[DOCKER,'container','inspect','--format','{{json .Id}}',run_name]);
  if r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout): return True
  if r.returncode==0:
   found=json.loads(r.stdout); require(isinstance(found,str) and re.fullmatch('[0-9a-f]{64}',found),'container ID binding'); return False
  raise ValueError('container absence was not proven')
 def cleanup():
  if absent('cleanup-inspect-before'): return True
  r=command('cleanup-owner-inspect',[DOCKER,'container','inspect','--format','{{json .Config.Labels}}',run_name]); found=json.loads(r.stdout); require(r.returncode==0 and isinstance(found,dict) and found.get('nano.gc.owner')==ownership,'refuse removing an unowned container')
  r=command('cleanup-owned-remove',[DOCKER,'container','rm','--force',run_name]); require(r.returncode==0,'owned container removal failed'); return absent('cleanup-inspect-after')
 try:
  event('preflight-start',container=run_name)
  index=json_pin(INDEX,INDEX_SHA); harness=json_pin(HARNESS/'manifest.json',HARNESS_SHA); base_plan=json_pin(BASE_PLAN,BASE_PLAN_SHA,8*1024*1024); plan_path=canonical(a.build_plan); plan=json_pin(plan_path,a.build_plan_sha,8*1024*1024); build=json_pin(terminal_path,a.build_terminal_sha)
  require(plan.get('format')=='nano-gc-arm-test-only-consumer-correction-plan-v1','specific corrected build plan required')
  require(plan.get('sourceCopy')==str(SOURCE) and plan.get('buildOutput')==str(BUILD) and plan.get('receiptOutput')==str(terminal_path.parent) and plan.get('image')==IMAGE,'corrected build plan paths/image')
  require(plan.get('sourceIndex')=={'path':str(INDEX),'sha256':INDEX_SHA} and plan.get('parentTerminal')=={'path':str(PARENT_TERMINAL),'sha256':PARENT_TERMINAL_SHA} and plan.get('parentArchive')=={'path':str(PARENT_ARCHIVE),'sha256':PARENT_ARCHIVE_SHA} and plan.get('testOnlyManifest')=={'path':str(TEST_ONLY_MANIFEST),'sha256':TEST_ONLY_MANIFEST_SHA},'specific correction parent/input bindings')
  json_pin(PARENT_TERMINAL,PARENT_TERMINAL_SHA); require(digest_file(PARENT_ARCHIVE)['sha256']==PARENT_ARCHIVE_SHA,'passed parent archive content changed'); member_proof=plan.get('parentMemberProof'); require(isinstance(member_proof,dict) and set(member_proof)=={'path','sha256'},'parent member proof binding'); members=json_pin(canonical(member_proof['path']),member_proof['sha256']); require(members.get('archive',{}).get('sha256')==PARENT_ARCHIVE_SHA and isinstance(members.get('members'),list) and len(members['members'])==140,'exact parent140 archive-member proof')
  require(plan.get('freshConsumerTargets')==base_plan.get('freshConsumerTargets') and set(plan.get('freshConsumerTargets',{}))==set(PROGRAMS),'corrected seven consumer paths differ')
  require(plan.get('freshConsumerObjects')==['nano-capella-transition-tests'] and plan.get('reusedConsumerObjects')==[name for name in PROGRAMS if name!='nano-capella-transition-tests'],'specific fresh/reused consumer objects')
  correction=plan.get('sourceCorrectionManifest'); require(correction=={'path':str(TEST_ONLY_MANIFEST),'sha256':TEST_ONLY_MANIFEST_SHA},'specific test-only manifest binding'); json_pin(TEST_ONLY_MANIFEST,TEST_ONLY_MANIFEST_SHA)
  require(index.get('testOverlayManifestSha256')==HARNESS_SHA and index.get('gcManifestSha256')=='ddf7712825b47aaaafa968df523b2ea6f77d108cc467a14f01f234238614de1c','source copy candidate pins')
  require(build.get('format')=='nano-gc-targeted-arm-build-terminal-v1' and build.get('status')=='BUILD_PASS_TESTS_NOT_EXECUTED','actual build not passed')
  require(build.get('planSha256')==a.build_plan_sha and build.get('sourceIndexSha256')==INDEX_SHA and build.get('testOverlayManifestSha256')==HARNESS_SHA,'actual build input pins')
  require(build.get('coreFresh')==0 and build.get('coreReused')==140 and build.get('archiveMembers')==140 and build.get('consumerFresh')==1 and build.get('consumerReused')==6 and build.get('archiveCopiedFromPassedParent') is True and build.get('coreObjectCopies')==0 and build.get('archiveCommandsExecuted')==0 and build.get('programsRelinked')==1 and build.get('programsCopiedFromPassedParent')==6,'actual core closure count'); require(build.get('parentSourceLibraryImageInputsUnchanged') is True and build.get('testsExecuted')==0 and build.get('runtimeStarted') is False and build.get('privateInputsRead') is False and build.get('productionQualified') is False,'build scope/invariants')
  for r in harness['sourceFiles']: pinned(SOURCE/r['path'],r['sha256'],2*1024*1024)
  auditor_row=next(r for r in harness['assets'] if r['path']=='audit-test-results.py'); pinned(HARNESS/'audit-test-results.py',auditor_row['sha256'],1024*1024); auditor=AUDITOR; pinned(auditor,AUDITOR_SHA,1024*1024)
  source_before=source_inventory(index); artifacts={}; artifacts['libnano_core.a']=digest_file(BUILD/'libnano_core.a'); compare_stat(artifacts['libnano_core.a'],build['archive'],'archive'); require(artifacts['libnano_core.a']['sha256']==PARENT_ARCHIVE_SHA,'actual archive must be exact parent copy')
  bound={r['target']:r for r in build.get('programs',[])}; require(set(bound)==set(PROGRAMS),'exact seven program bindings')
  for name in PROGRAMS:
   require(bound[name].get('objectReusedFromPassedParent') is (name!='nano-capella-transition-tests') and bound[name].get('binaryReusedFromPassedParent') is (name!='nano-capella-transition-tests'),'specific copied consumer provenance differs: '+name)
   record=digest_file(BUILD/name); compare_stat(record,bound[name]['binary'],name); require(bound[name].get('coreArchiveSha256')==artifacts['libnano_core.a']['sha256'],'consumer core archive differs'); artifacts[name]=record
   object_rel='CMakeFiles/'+name+'.dir/'+plan['freshConsumerTargets'][name]+'.o'; object_record=digest_file(BUILD/object_rel); compare_stat(object_record,bound[name]['object'],name+' object'); artifacts[object_rel]=object_record
  artifacts['libnano-test-sync-fault.so']=digest_file(BUILD/'libnano-test-sync-fault.so'); require(artifacts['libnano-test-sync-fault.so']['sha256']==FAULT_SHA,'actual fault shim differs')
  meta={}
  for target in ['nano-capella-transition-tests','nano-checkpoint-blob-tests','nano','nano-storage-maintenance']:
   prefix=BUILD/'CMakeFiles'/ (target+'.dir'); flags=bytes_file(prefix/'flags.make',1024*1024); link=bytes_file(prefix/'link.txt',1024*1024); meta[target]={'flagsSha256':sha(flags),'linkSha256':sha(link),'flagsText':flags.decode(),'linkText':link.decode()}
   if target in ['nano-capella-transition-tests','nano-checkpoint-blob-tests']:
    require(re.search(r'(?:^|\s)-DNANO_SYNC_FAULT=1(?:\s|$)',flags.decode()),'NANO_SYNC_FAULT compile definition missing'); require('libnano-test-sync-fault.so' in link.decode() and re.search(r'-Wl,-rpath[^\n]*?/outputs',link.decode()),'actual linked shim/RPATH missing')
   else: require('libnano-test-sync-fault' not in link.decode(),'fault shim linked into production program')
  save(out/'admission-ctime-diagnostics.json',{'scope':'initial admission versus earlier build receipt only','differences':ADMISSION_CTIME_DIAGNOSTICS,'otherInitialBindingFieldsRemainStrict':True,'allRunBeforeAfterMetadataEqualityRemainsStrict':True,'buildOrCppTestsRepeated':False})
  testing=out/'testing'; testing.mkdir(mode=0o700); (testing/'tmp').mkdir(mode=0o700); (testing/'ctest').mkdir(mode=0o700); graph=graph_ctest(testing/'ctest'); save(out/'selected-ctest-graph.json',graph)
  before={'controllerSha256':a.controller_sha,'buildPlanSha256':a.build_plan_sha,'basePlanSha256':BASE_PLAN_SHA,'parentTerminalSha256':PARENT_TERMINAL_SHA,'parentArchiveSha256':PARENT_ARCHIVE_SHA,'auditorSha256':AUDITOR_SHA,'buildTerminalSha256':a.build_terminal_sha,'sourceIndexSha256':INDEX_SHA,'harnessManifestSha256':HARNESS_SHA,'source':source_before,'roots':[root_identity(SOURCE),root_identity(BUILD)],'artifacts':artifacts,'metadata':meta}; save(out/'before.json',before)
  image_format='{{json .}}'
  r=command('image-inspect',[DOCKER,'image','inspect','--format',image_format,IMAGE]); require(r.returncode==0,'existing image inspect failed'); image=json.loads(r.stdout); require(isinstance(image,dict) and image['Id']==IMAGE and image.get('Architecture')=='arm64' and image.get('Os')=='linux','image identity/platform'); config=image.get('Config',{}); require(config.get('Entrypoint') in (None,[]),'image entrypoint refused'); require(config.get('Cmd')==['/bin/bash'],'unexpected image default command'); names=[v.split('=',1)[0] for v in config.get('Env',[])]; require(len(names)==3 and set(names)=={'PATH','CC','CXX'},'unexpected image environment names'); save(out/'image-binding.json',{'image':IMAGE,'architecture':'arm64','os':'linux','entrypoint':config.get('Entrypoint'),'cmd':config['Cmd'],'environmentNames':names})
  absence_observed=absent('before-launch-container-inspect'); require(absence_observed,'named container already exists')
  selected='^('+'|'.join(SELECTED)+')$'
  argv=[DOCKER,'run','--rm','--pull=never','--platform','linux/arm64','--name',run_name,'--label','nano.gc.owner='+ownership,'--network','none','--read-only','--user','501:20','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','1024m','--pids-limit','256','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m,uid=501,gid=20,mode=700','--mount',f'type=bind,src={SOURCE},dst=/work,readonly','--mount',f'type=bind,src={BUILD},dst=/outputs,readonly','--mount',f'type=bind,src={testing},dst=/testing','--env','TMPDIR=/tmp','--workdir','/testing/ctest',IMAGE,'/usr/bin/ctest','--test-dir','/testing/ctest','--tests-regex',selected,'--no-tests=error','--output-on-failure','--verbose','--output-junit','/testing/gc-selected.junit.xml']
  save(out/'ctest.command.json',argv); event('ctest-launch',selected=SELECTED,deadlineSeconds=480)
  with (out/'ctest.log').open('wb',buffering=0) as output:
   proc=subprocess.Popen(argv,stdout=output,stderr=subprocess.STDOUT,env=env,start_new_session=True); launched=True
   try: exit_code=proc.wait(timeout=480); process_completed=True
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
  except BaseException as error: failure=(failure or '')+'; container cleanup '+str(error)
  if before is not None:
   try:
    source_after=source_inventory(index); artifacts_after={name:digest_file(BUILD/name) for name in before['artifacts']}; roots_after=[root_identity(SOURCE),root_identity(BUILD)]; metadata_after={}
    for target in before['metadata']:
     prefix=BUILD/'CMakeFiles'/(target+'.dir'); metadata_after[target]={'flagsSha256':sha(bytes_file(prefix/'flags.make',1024*1024)),'linkSha256':sha(bytes_file(prefix/'link.txt',1024*1024))}
    invariants=source_after==before['source'] and artifacts_after==before['artifacts'] and roots_after==before['roots'] and all(metadata_after[k]['flagsSha256']==v['flagsSha256'] and metadata_after[k]['linkSha256']==v['linkSha256'] for k,v in before['metadata'].items())
    pinned(INDEX,INDEX_SHA); pinned(HARNESS/'manifest.json',HARNESS_SHA); pinned(plan_path,a.build_plan_sha); pinned(BASE_PLAN,BASE_PLAN_SHA); pinned(PARENT_TERMINAL,PARENT_TERMINAL_SHA); pinned(canonical(correction['path']),correction['sha256']); pinned(canonical(member_proof['path']),member_proof['sha256']); pinned(AUDITOR,AUDITOR_SHA,1024*1024); pinned(self_path,a.controller_sha,1024*1024); pinned(terminal_path,a.build_terminal_sha)
    save(out/'after.json',{'source':source_after,'roots':roots_after,'artifacts':artifacts_after,'metadata':metadata_after,'allBindingsUnchanged':invariants}); require(invariants,'source/artifact/root/metadata changed during tests')
   except BaseException as error: failure=(failure or '')+'; immutable binding '+str(error)
  execution={'format':'nano-gc-two-selected-execution-terminal-v1','exitCode':exit_code,'processCompleted':process_completed,'containerAbsentAfterCleanup':container_absent,'timedOut':timed_out,'launched':launched,'bindingInvariantsHeld':invariants,'failure':failure,'runtimeDutiesResumed':False,'productionQualified':False}; terminal_hash=save(out/'execution-terminal.json',execution)
 if failure is None and exit_code==0 and process_completed and container_absent and invariants:
  try:
   junit=out/'testing/gc-selected.junit.xml'; log=out/'ctest.log'; junit_sha=digest_file(junit)['sha256']; log_sha=digest_file(log)['sha256']; args=[sys.executable,'-I','-S',str(auditor),'--junit',str(junit),'--junit-sha',junit_sha,'--log',str(log),'--log-sha',log_sha,'--terminal',str(out/'execution-terminal.json'),'--terminal-sha',terminal_hash,'--before',str(out/'before.json'),'--before-sha',digest_file(out/'before.json')['sha256'],'--build-terminal',str(terminal_path),'--build-terminal-sha',a.build_terminal_sha]; r=command('public-result-audit',args,30); require(r.returncode==0,'scoped public auditor failed'); audit=json.loads(r.stdout); require(audit.get('passedWithinTestScope') is True and audit.get('productionQualified') is False,'scoped audit verdict invalid'); save(out/'public-result-audit.json',audit)
  except BaseException as error: failure=type(error).__name__+': '+str(error)
 result={'format':'nano-gc-root-dispatched-two-retry-plus-four-prior-result-v1','passedWithinTestScope':failure is None and audit is not None,'selectedTests':SELECTED,'registeredCommandsValidated':GRAPH_SELECTED,'singleSixTestExecution':False,'failure':failure,'executionTerminalSha256':terminal_hash,'actualContainerAbsent':container_absent,'actualBindingsUnchanged':invariants,'noRelaunchAttempted':True,'privateInputsRead':False,'runtimeDutiesResumed':False,'productionQualified':False}; save(out/'result.json',result); event('root-dispatched-terminal',passedWithinTestScope=result['passedWithinTestScope'],failure=failure); return 0 if result['passedWithinTestScope'] else 1
if __name__=='__main__': sys.exit(main())
