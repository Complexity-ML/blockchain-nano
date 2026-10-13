#!/usr/bin/env python3
"""ROOT-dispatched only: nine new AMD64 CTests on one fresh offline container.
Prepared without Docker, SSH, compilation, tests, DB or campaign input reads.
"""
import argparse, hashlib, json, os, pathlib, re, signal, stat, subprocess, sys, time, uuid, xml.etree.ElementTree as ET
BASE = None; SOURCE = None; BUILD = None
PLAN_SHA = 'd20cd9bc55badea80a535e982f08d4a8ad9270fb01babd726b600882e55fb260'
CORE_MANIFEST_SHA = 'da39fde2008dcbe960e67b10e0d538f2456aab9d19c86756d291533f99113b06'
TEST_MANIFEST_SHA = 'ab076fcb70078ea18d44184cd84f47ec526a9a2ae37260ea2eccd58f32d2aead'
BUILD_CONTROLLER_SHA = '98da021a459854f9af204014efeb3e39e57648ee271eed3e90b5489fa6ea66ad'
OVERLAYS = {'include/nano/consensus_capella_fork_store.hpp': 'a422943490f38c96146846557222369746a307394a4955a5701bb4fc583fd181', 'src/consensus_capella_fork_store.cpp': 'f1db700a1f7cb282bdc672498b71e71e01ca86de14b438f23d03e80349c9672e', 'src/consensus_capella_fork_state.cpp': 'bf35b0cfff9a3a0383378abbab15812ea94bf0704e1912e0bdb86e8f7e0133b8', 'src/consensus_capella_target_source_auth.hpp': 'e9fa05a172145751ca0c7153550af9a0921f64a6cdc3fcb3cbb1c198605abfea', 'CMakeLists.txt': '8ac60b3b7290cf0e53bd5b1bfb394ea7efe8c0e84495bd1c43012a4b140321d6', 'tests/test_capella_target_source_authentication.cpp': '73bb671a45987d129546c9b29b701f2c5b59f69afb5001ead009f52545faa9ab'}
GIT_COMMIT = '8be93c69e144ed6e7f8b8a33ecb308c6fe3d4fdd'
FIXTURE_GIT_COMMIT = 'f9d078d0d45d6081c8c8745a80575d898335ec3e'
IMAGE = 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
FAULT_SHA = '1c5fbe9a94c9e4137f0ae41999390a9a9690e63324998bfeff6368609345d86e'
DOCKER = ['/usr/bin/sudo','-n','/usr/bin/docker']
NEW = ['storage_capella_blob_gc_next_publication','storage_capella_blob_gc_adversarial_boundaries','storage_capella_blob_gc_natural_pruning','storage_capella_blob_gc_cli_closed_fixture']
REG = ['storage_checkpoint_immutable_blobs','consensus_capella_fragment_checkpoint_migration']
AVAILABILITY = 'consensus_capella_duty_projection_availability'
TINY = 'consensus_bls_positive_verification_cache'
AUTH = 'consensus_capella_target_source_authentication'
SELECTED = NEW + REG + [AVAILABILITY,TINY,AUTH]; GRAPH_SELECTED = SELECTED
PROGRAMS = ['nano','nano-capella-transition-tests','nano-storage-maintenance','nano-checkpoint-blob-tests','nano-capella-paused-capture','nano-capella-isolated-stopped-capture','nano-capella-isolated-bootstrap-audit','nano-capella-duty-availability-tests','nano-consensus-bls-verification-cache-tests','nano-capella-target-source-authentication-tests']
ENV3 = ['NANO_TEST_CHECKPOINT_BLOB_GC=1','NANO_TEST_COLD_REACTOR=1','NANO_TEST_FRAGMENT_CHECKPOINT=1']
EXPECTED = {
 NEW[0]: ['/usr/bin/cmake','-E','env'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 NEW[1]: ['/usr/bin/cmake','-E','env','NANO_TEST_CHECKPOINT_BLOB_GC_ADVERSARIAL=1'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 NEW[2]: ['/outputs/nano-capella-transition-tests','--blob-gc-natural'],
 NEW[3]: ['/usr/bin/cmake','-E','env','NANO_TEST_CHECKPOINT_BLOB_GC_CLI=1','NANO_TEST_CHECKPOINT_GC_TOOL=/outputs/nano-storage-maintenance'] + ENV3 + ['/outputs/nano-capella-transition-tests','--prepared-block'],
 REG[0]: ['/outputs/nano-checkpoint-blob-tests'],
 REG[1]: ['/usr/bin/cmake','-E','env','NANO_TEST_FRAGMENT_CHECKPOINT=1','/outputs/nano-capella-transition-tests','--prepared-block'],
 AVAILABILITY: ['/outputs/nano-capella-duty-availability-tests','--self-test'],
 TINY: ['/outputs/nano-consensus-bls-verification-cache-tests','--self-test'],
 AUTH: ['/outputs/nano-capella-target-source-authentication-tests','--self-test']}
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
  value = stat.S_IMODE(actual[key]) if key=='mode' and expected[key] <= 0o7777 else actual[key]
  require(value==expected[key],why+' stat '+key)
def source_inventory(inputs):
 require(inputs.get('format')=='nano-target-source-auth-amd64-incremental-plan-v1' and inputs.get('status')=='READY_FROZEN_OVERLAYS','specific frozen target-source-auth plan required')
 require(inputs.get('baseRevision')==GIT_COMMIT and inputs.get('image')==IMAGE,'frozen public source/image base pins differ')
 require({p:r['sha256'] for p,r in inputs['overlays'].items()}==OVERLAYS,'exact six reviewed coreV2/testV3 overlays required')
 require([r['manifest']['sha256'] for r in inputs['ownerReferences']]==[CORE_MANIFEST_SHA,TEST_MANIFEST_SHA],'literal external owner seals differ')
 expected={**inputs['sourceHashes'],**OVERLAYS}; require(len(expected)==inputs['sourceCountAfterOverlays']==1304,'exact1304 delivered source scope required'); out={}
 for rel,h in expected.items():
  require(isinstance(rel,str) and pathlib.PurePosixPath(rel).as_posix()==rel and not rel.startswith('/') and '..' not in pathlib.PurePosixPath(rel).parts and '\n' not in rel and '\0' not in rel and not rel.endswith('.env'),'public source relative path')
  require(isinstance(h,str) and re.fullmatch('[0-9a-f]{64}',h),'exact source SHA required')
  data=digest_file(SOURCE/rel); require(data['sha256']==h,'exact delivered source mismatch: '+rel)
  size=inputs['overlays'][rel]['bytes'] if rel in inputs['overlays'] else inputs['sourceBytes'][rel]
  require(data['bytes']==size,'source byte count differs: '+rel); out[rel]=data
 actual=set()
 for parent,dirs,files in os.walk(SOURCE,followlinks=False):
  require(not any((pathlib.Path(parent)/n).is_symlink() for n in dirs+files),'copied source symlink refused')
  actual.update((pathlib.Path(parent)/n).relative_to(SOURCE).as_posix() for n in files)
 require(actual==set(expected),'exact delivered public source allowlist required')
 require(out['include/nano/consensus_bls.hpp']['sha256']=='bf8668a4e5119529ec091e7fe5616d93767da243bf63c9a34b3ae4b384706cbb' and out['tests/test_capella_transition.cpp']['sha256']=='35d580c3f8a15337eb56413c9d15e7b6c0644b5572a45eb6a4f9be1ac2f34a5e','protected API and huge scenario source must be unchanged')
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
 require(set(commands)==set(GRAPH_SELECTED),'nine registered selected tests missing')
 for name in GRAPH_SELECTED:
  require(commands[name]==EXPECTED[name],'selected command differs: '+name); props=properties.get(name,{}); require(set(props)<= {'TIMEOUT','LABELS','_BACKTRACE_TRIPLES'},'unsafe selected test property'); require(props.get('TIMEOUT')==('70' if name in (AVAILABILITY,TINY,AUTH) else '90' if name==REG[0] else '180'),'selected timeout differs')
 require(properties[TINY].get('LABELS')=='qualification_consensus_crypto_cache','reviewed tiny crypto CTest label required')
 require(properties[AUTH].get('LABELS')=='qualification_consensus_source_authentication','reviewed source-auth CTest label required')
 return {'graph':graph,'selectedCommands':commands,'selectedProperties':properties}

def save(path,value):
 raw=(json.dumps(value,indent=2)+'\n').encode(); fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
 with os.fdopen(fd,'wb') as f: f.write(raw); f.flush(); os.fsync(f.fileno())
 return sha(raw)

def normalize_build_stat(row):
 require(isinstance(row,dict),'actual build stat required'); out={'sha256':row.get('sha256')}
 for dest,src in ALLOWED_STAT.items(): require(type(row.get(src)) is int,'incomplete actual build stat '+src); out[dest]=row[src]
 return out

FIXTURE_MANIFEST_SHA = 'ae9bb195e095cd5224d39782b2b88bae936c07f36ec51cb11baf5925c2d3ead2'
def fixture_inventory(manifest,root,source):
 require(manifest.get('format')=='nano-gc-amd64-public-runtime-fixtures-v1' and manifest.get('gitCommit')==FIXTURE_GIT_COMMIT and manifest.get('privateDataCopied') is False and manifest.get('cppCompilerSourcePinsModified') is False,'exact current Git public runtime fixture scope required')
 rows=manifest.get('files'); require(isinstance(rows,list) and len(rows)==23 and manifest.get('overlayPathCount')==23 and manifest.get('overlayPayloadBytes')<=16*1024*1024,'23-file/16MiB runtime fixture bound required'); out={};total=0
 for row in rows:
  rel=row.get('path');require(isinstance(rel,str) and pathlib.PurePosixPath(rel).as_posix()==rel and not rel.startswith('/') and '..' not in pathlib.PurePosixPath(rel).parts and '\n' not in rel and '\0' not in rel and rel not in out,'unique canonical public fixture path')
  require(type(row.get('bytes')) is int and 0<=row['bytes']<=16*1024*1024 and re.fullmatch('[0-9a-f]{64}',row.get('sha256','')) is not None,'runtime fixture size/SHA required');total+=row['bytes'];require(total<=16*1024*1024,'runtime fixture byte bound')
  record=digest_file(root/rel);require(record['sha256']==row['sha256'] and record['bytes']==row['bytes'],'runtime fixture bytes differ from exact Git manifest: '+rel);out[rel]=record
 require(total==manifest['overlayPayloadBytes'],'runtime fixture total differs')
 historical=manifest.get('historicalFixturePinsPreserved'); require(isinstance(historical,dict) and len(historical)==20,'twenty historical fixture pins required')
 own_fixtures={rel[len('tests/fixtures/'):]:record['sha256'] for rel,record in source.items() if rel.startswith('tests/fixtures/')}; require(len(own_fixtures)==23 and own_fixtures=={p:r['sha256'] for p,r in out.items()},'all23 fixtures must be in the bound own source')
 require(all(rel in out and out[rel]['sha256']==h for rel,h in historical.items()),'historical20 fixture bytes changed')
 # Only this explicitly created PUBLIC Git fixture root is scanned; no campaign
 # directory, environment file, key, database or external tree is traversed.
 actual=set();directories=0
 for parent,dirs,files in os.walk(root,followlinks=False):
  directories+=1;require(directories<=64 and len(dirs)+len(files)<=64,'public fixture directory budget')
  for name in dirs:require(not (pathlib.Path(parent)/name).is_symlink(),'fixture directory symlink refused')
  for name in files:
   p=pathlib.Path(parent)/name;rel=p.relative_to(root).as_posix();require(rel in out and not p.is_symlink(),'unlisted fixture file refused');actual.add(rel)
 require(actual==set(out),'exact public fixture root contents required');return {'root':root_identity(root),'files':out,'compilerSourcePinsRewritten':False,'runtimeFixtureOverlayOnly':False,'fixturesIntegratedIntoOwnSource':True}
def main():
 global BASE,SOURCE,BUILD
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['controller-sha','public-inputs','public-inputs-sha','build-root','build-controller-sha','build-terminal','build-terminal-sha','fixture-root','fixture-manifest','fixture-manifest-sha','output']: p.add_argument('--'+name,required=True)
 a=p.parse_args(); self_path=canonical(__file__); pinned(self_path,a.controller_sha,1024*1024)
 BASE=canonical(a.build_root); require(BASE==pathlib.Path('/home/ubuntu/nanobees-qualification/target-source-auth-amd64-20261013/build-pass-01'),'explicit target-source-auth passed build root required'); SOURCE=BASE/'source'; BUILD=BASE/'build'
 terminal_path=canonical(a.build_terminal); require(terminal_path==BASE/'receipts/terminal-build-receipt.json','exact passed build-root receipt location required')
 inputs_path=canonical(a.public_inputs); inputs=json_pin(inputs_path,a.public_inputs_sha,16*1024*1024)
 require(a.public_inputs_sha==PLAN_SHA and inputs.get('format')=='nano-target-source-auth-amd64-incremental-plan-v1' and inputs.get('baseRevision')==GIT_COMMIT and inputs.get('outputRoot')==str(BASE),'exact frozen target-source-auth public plan required')
 fixture_root=canonical(a.fixture_root); require(fixture_root==SOURCE/'tests/fixtures','fixtures must be in own delivered source; no shadow mount'); require(a.fixture_manifest_sha==FIXTURE_MANIFEST_SHA,'specific reviewed23fixture manifest required'); fixture_manifest=json_pin(canonical(a.fixture_manifest),a.fixture_manifest_sha)
 require(os.geteuid()==1000 and os.getegid()==1000,'VPS host UID1000:GID1000 required')
 out=canonical(a.output,False); require(out.parent==BASE and re.fullmatch('tests-root-dispatched-[a-z0-9-]{1,64}',out.name),'fresh passed-build test output namespace required'); os.mkdir(out,0o700)
 events=out/'events.jsonl'; run_name='nano-target-auth-test-'+uuid.uuid4().hex[:16]; ownership=uuid.uuid4().hex
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
  require(build.get('format')=='nano-target-source-auth-amd64-incremental-terminal-v1' and build.get('status')=='BUILD_PASS_TESTS_NOT_EXECUTED','actual passed target-source-auth AMD build required')
  require(a.build_controller_sha==BUILD_CONTROLLER_SHA and build.get('controllerSha256')==a.build_controller_sha and build.get('planSha256')==a.public_inputs_sha and build.get('baseRevision')==GIT_COMMIT and build.get('coreManifestSha256')==CORE_MANIFEST_SHA and build.get('testManifestSha256')==TEST_MANIFEST_SHA,'actual target-source-auth build provenance differs')
  counts={'coreFresh':27,'coreReused':113,'archiveMembers':140,'existingConsumersFresh':6,'existingConsumersReused':3,'newTinyConsumerFresh':1,'newLinks':10}
  require(inputs.get('expectedCounts')==counts and all(build.get(k)==v for k,v in counts.items()),'exact27/113/140 and6fresh3reused1newtiny10link closure required')
  require(build.get('actualDependenciesEqualPreflight') is True and build.get('allParentSourceLibraryGeneratedSdkBindingsUnchanged') is True and build.get('oldMdFabricated') is False,'actual source-auth build closure proof missing')
  require(build.get('testsExecuted')==0 and build.get('runtimeStarted') is False and build.get('privateInputsRead') is False and build.get('imageBuiltOrPulled') is False and build.get('productionQualified') is False,'build scope invalid')
  consumers={row['target']:row for row in inputs['consumers']}; require(len(inputs['consumers'])==9 and set(consumers)==set(PROGRAMS)-{'nano-capella-target-source-authentication-tests'},'exact9 existing consumer paths required'); consumers[inputs['newTarget']]={'object':'CMakeFiles/'+inputs['newTarget']+'.dir/'+inputs['newSource']+'.o','fresh':True}
  source_before=source_inventory(inputs); fixtures_before=fixture_inventory(fixture_manifest,fixture_root,source_before); artifacts={'libnano_core.a':digest_file(BUILD/'libnano_core.a')}; compare_stat(artifacts['libnano_core.a'],normalize_build_stat(build['archive']),'archive')
  bound={row['target']:row for row in build.get('programs',[])}; require(len(build.get('programs',[]))==10 and set(bound)==set(PROGRAMS),'exact ten actual ELF/object bindings required')
  for name in PROGRAMS:
   require(bound[name].get('objectReused') is (not consumers[name]['fresh']) and bound[name].get('binaryReused') is False,'exact fresh/reused object and fresh link provenance: '+name)
   record=digest_file(BUILD/name); compare_stat(record,normalize_build_stat(bound[name]['binary']),name); require(bound[name].get('coreArchiveSha256')==artifacts['libnano_core.a']['sha256'],'consumer core archive differs'); artifacts[name]=record
   object_rel=consumers[name]['object']; record=digest_file(BUILD/object_rel); compare_stat(record,normalize_build_stat(bound[name]['object']),name+' object'); artifacts[object_rel]=record
  core_paths=[row['object'] for row in inputs['core']]; require(len(core_paths)==140 and len(set(core_paths))==140,'exact140 unique core objects required')
  for rel in core_paths: artifacts[rel]=digest_file(BUILD/rel)
  proof_path=BASE/'receipts/archive-140-raw-member-proof.json';proof_bytes=bytes_file(proof_path,1024*1024);proof=json.loads(proof_bytes);compare_stat(artifacts['libnano_core.a'],normalize_build_stat(proof['archive']),'raw-member-proof archive');require(proof.get('arPInvocations')==280 and proof.get('objectPayloadExported') is False and len(proof.get('members',[]))==140,'actual140raw-member proof required');members={r['object']:r for r in proof['members']};require(set(members)==set(core_paths),'member-proof140object mapping differs')
  for rel,row in members.items():require(artifacts[rel]['sha256']==row.get('sha256') and artifacts[rel]['bytes']==row.get('bytes'),'current140coreobject differs from build member proof: '+rel)
  member_proof_before=digest_file(proof_path)
  artifacts['third_party/ckzg/include/nano_kzg_setup.hpp']=digest_file(BUILD/'third_party/ckzg/include/nano_kzg_setup.hpp');require(artifacts['third_party/ckzg/include/nano_kzg_setup.hpp']['sha256']==inputs['generated']['third_party/ckzg/include/nano_kzg_setup.hpp']['sha256'],'generated KZG differs')
  require(len(inputs['libraries'])==7,'seven pinned dependency artifacts required')
  for rel,pin in inputs['libraries'].items():
   record=digest_file(BUILD/rel); require(record['sha256']==pin['sha256'] and record['bytes']==pin['bytes'],'historically pinned dependency copy differs: '+rel); artifacts[rel]=record
  require(artifacts['libnano-test-sync-fault.so']['sha256']==FAULT_SHA,'actual AMD fault shim differs')
  meta={}
  for target in PROGRAMS:
   prefix=BUILD/'CMakeFiles'/(target+'.dir'); flags=bytes_file(prefix/'flags.make',1024*1024); link=bytes_file(prefix/'link.txt',1024*1024); meta[target]={'flagsSha256':sha(flags),'linkSha256':sha(link),'flagsText':flags.decode(),'linkText':link.decode()}
   if target in ['nano-capella-transition-tests','nano-checkpoint-blob-tests']:
    require(re.search(r'(?:^|\s)-DNANO_SYNC_FAULT=1(?:\s|$)',flags.decode()),'NANO_SYNC_FAULT compile definition missing'); require('libnano-test-sync-fault.so' in link.decode() and re.search(r'-Wl,-rpath[^\n]*?/outputs',link.decode()),'actual fault shim/RPATH missing')
   else: require('libnano-test-sync-fault' not in link.decode(),'fault shim linked into non-fault program')
  artifacts['compile_commands.json']=digest_file(BUILD/'compile_commands.json')
  sdk_binding_path=BASE/'receipts/sdk-before-after-binding.json'; sdk_binding_data=bytes_file(sdk_binding_path,1024*1024); sdk_binding=json.loads(sdk_binding_data)
  sdk_before_path=BASE/'receipts/sdk-before.log'; sdk_after_path=BASE/'receipts/sdk-after.log'; sdk_before_data=bytes_file(sdk_before_path,1024*1024); sdk_after_data=bytes_file(sdk_after_path,1024*1024)
  require(sdk_before_data==sdk_after_data and sdk_binding.get('equal') is True and sdk_binding.get('beforeSha256')==sdk_binding.get('afterSha256')==sha(sdk_before_data) and sdk_binding.get('parentQualifiedPaths')==2193 and sdk_binding.get('noHistoricalSdkInvented') is True,'actual build SDK before/after binding required')
  sdk_rows={name:h for h,name in (line.split('  ',1) for line in sdk_before_data.decode().splitlines())}; require(len(sdk_rows)==sdk_binding['currentPaths'] and all(sdk_rows.get(p)==h for p,h in inputs['oldSdkHashes'].items()),'current SDK bound to actual prior2193 observations')
  evidence_files={str(p):digest_file(p) for p in (sdk_binding_path,sdk_before_path,sdk_after_path,BASE/'receipts/source-before.json',BASE/'receipts/source-after.json',BASE/'receipts/parents-before-first-compile.json',BASE/'receipts/parents-after.json',BASE/'receipts/generated-metadata-before.json')}
  generated_bound=json.loads(bytes_file(BASE/'receipts/generated-metadata-before.json',1024*1024))
  for rel,record in generated_bound.items(): compare_stat(digest_file(BUILD/rel),normalize_build_stat(record),'build generated metadata '+rel)

  save(out/'admission-ctime-diagnostics.json',{'scope':'actual test admission versus earlier build terminal only','differences':ADMISSION_CTIME_DIAGNOSTICS,'otherAdmissionFieldsStrict':True,'allRunBeforeAfterMetadataEqualityStrict':True})
  testing=out/'testing'; testing.mkdir(mode=0o700); (testing/'ctest').mkdir(mode=0o700); graph=graph_ctest(testing/'ctest'); save(out/'selected-ctest-graph.json',graph)
  before={'controllerSha256':a.controller_sha,'semanticAuditorNotDispatched':True,'buildControllerSha256':a.build_controller_sha,'buildTerminalSha256':a.build_terminal_sha,'publicInputsSha256':a.public_inputs_sha,'publicInputsPath':str(inputs_path),'coreManifestSha256':CORE_MANIFEST_SHA,'testManifestSha256':TEST_MANIFEST_SHA,'gitCommit':GIT_COMMIT,'source':source_before,'roots':[root_identity(SOURCE),root_identity(BUILD)],'artifacts':artifacts,'metadata':meta,'ctestGraph':graph,'archiveMemberProof':member_proof_before,'fixtures':fixtures_before,'fixtureManifestSha256':a.fixture_manifest_sha,'fixtureManifestPath':str(canonical(a.fixture_manifest)),'buildSdkEvidence':evidence_files}; save(out/'before.json',before)
  r=command('image-inspect',DOCKER+['image','inspect','--format','{{json .}}',IMAGE]); require(r.returncode==0,'existing image inspect failed'); image=json.loads(r.stdout); require(isinstance(image,dict) and image['Id']==IMAGE and image.get('Architecture')=='amd64' and image.get('Os')=='linux','existing image identity/platform'); config=image.get('Config',{}); require(config.get('Entrypoint') in (None,[]),'image entrypoint refused'); require(config.get('Cmd')==['/bin/bash'],'unexpected image command'); names=[v.split('=',1)[0] for v in config.get('Env',[])]; require(len(names)==3 and set(names)=={'PATH','CC','CXX'},'unexpected image environment'); save(out/'image-binding.json',{'image':IMAGE,'architecture':'amd64','os':'linux','environmentNames':names,'imageBuiltOrPulled':False})
  absence_observed=absent('before-launch-container-inspect'); require(absence_observed,'named container already exists')
  selected='^('+'|'.join(SELECTED)+')$'
  argv=DOCKER+['run','--rm','--pull=never','--platform','linux/amd64','--name',run_name,'--label','nano.gc.owner='+ownership,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','1024m','--pids-limit','256','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m,uid=1000,gid=1000,mode=700','--mount',f'type=bind,src={SOURCE},dst=/work,readonly','--mount',f'type=bind,src={BUILD},dst=/outputs,readonly','--mount',f'type=bind,src={testing},dst=/testing','--env','TMPDIR=/tmp','--env','NANO_BLS_VERIFICATION_CACHE_METRICS=1','--workdir','/testing/ctest',IMAGE,'/usr/bin/ctest','--test-dir','/testing/ctest','--tests-regex',selected,'--no-tests=error','--output-on-failure','--verbose','--output-junit','/testing/gc-selected.junit.xml']
  save(out/'ctest.command.json',argv); event('ctest-launch',selected=SELECTED,deadlineSeconds=1260)
  with (out/'ctest.log').open('wb',buffering=0) as output:
   proc=subprocess.Popen(argv,stdout=output,stderr=subprocess.STDOUT,env=env,start_new_session=True); launched=True
   try: exit_code=proc.wait(timeout=1260); process_completed=True
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
    source_after=source_inventory(inputs); fixtures_after=fixture_inventory(fixture_manifest,fixture_root,source_after); artifacts_after={name:digest_file(BUILD/name) for name in before['artifacts']}; roots_after=[root_identity(SOURCE),root_identity(BUILD)]; metadata_after={}
    for target in before['metadata']:
     prefix=BUILD/'CMakeFiles'/(target+'.dir'); metadata_after[target]={'flagsSha256':sha(bytes_file(prefix/'flags.make',1024*1024)),'linkSha256':sha(bytes_file(prefix/'link.txt',1024*1024))}
    graph_after={}
    for row in before['ctestGraph']['graph']:
     if row['present']:
      rel=pathlib.Path(row['path']); data=bytes_file(BUILD/rel,2*1024*1024); copy=bytes_file(testing/'ctest'/rel,2*1024*1024); require(sha(data)==row['sha256'] and copy==data,'CTest graph changed during test'); graph_after[row['path']]=sha(data)
    invariants={p:digest_file(p) for p in before['buildSdkEvidence']}==before['buildSdkEvidence'] and digest_file(BASE/'receipts/archive-140-raw-member-proof.json')==before['archiveMemberProof'] and fixtures_after==before['fixtures'] and source_after==before['source'] and artifacts_after==before['artifacts'] and roots_after==before['roots'] and all(metadata_after[k]['flagsSha256']==v['flagsSha256'] and metadata_after[k]['linkSha256']==v['linkSha256'] for k,v in before['metadata'].items())
    pinned(inputs_path,a.public_inputs_sha,16*1024*1024); pinned(self_path,a.controller_sha,1024*1024); pinned(terminal_path,a.build_terminal_sha); pinned(canonical(a.fixture_manifest),a.fixture_manifest_sha)
    save(out/'after.json',{'source':source_after,'roots':roots_after,'artifacts':artifacts_after,'metadata':metadata_after,'fixtures':fixtures_after,'ctestGraphSourceAndCopyUnchanged':True,'allBindingsUnchanged':invariants}); require(invariants,'source/artifact/root/metadata changed during tests')
   except BaseException as error: failure=(failure or '')+'; immutable binding '+str(error)
  execution={'format':'nano-target-source-auth-amd64-nine-selected-execution-terminal-v1','exitCode':exit_code,'processCompleted':process_completed,'containerAbsentAfterCleanup':container_absent,'timedOut':timed_out,'launched':launched,'bindingInvariantsHeld':invariants,'failure':failure,'privateInputsRead':False,'runtimeDutiesResumed':False,'productionQualified':False}; terminal_hash=save(out/'execution-terminal.json',execution)
 # Only count actual registered JUnit execution here. The separate semantic
 # auditor will inspect real case JSON/logs later; this is no security audit.
 junit_summary=None
 if process_completed and (out/'testing/gc-selected.junit.xml').exists():
  try:
   junit_path=out/'testing/gc-selected.junit.xml'; junit_data=bytes_file(junit_path,16*1024*1024); document=ET.fromstring(junit_data); cases=document.findall('.//testcase'); names=[c.get('name') for c in cases]
   require(len(cases)==9 and len(set(names))==9 and set(names)==set(SELECTED),'exact nine actual JUnit tests required')
   rows=[{'name':c.get('name'),'status':c.get('status'),'time':c.get('time'),'failure':c.find('failure') is not None,'error':c.find('error') is not None,'skipped':c.find('skipped') is not None} for c in cases]
   junit_summary={'format':'nano-target-source-auth-nine-junit-execution-summary-v1','junitSha256':sha(junit_data),'tests':rows,'allNineStatusRun':all(r['status']=='run' for r in rows),'noSkip':not any(r['skipped'] for r in rows),'allNineRegisteredExecutionPassed':all(r['status']=='run' and not (r['failure'] or r['error'] or r['skipped']) for r in rows),'semanticCaseAssertionsIndependentlyAudited':False,'productionQualified':False}; save(out/'junit-execution-summary.json',junit_summary)
   if failure is None: require(junit_summary['allNineRegisteredExecutionPassed'],'actual JUnit9 failure/error/skip; no relaunch')
  except BaseException as error: failure=(failure or '')+'; JUnit scope '+type(error).__name__+': '+str(error)
 executed_pass=failure is None and exit_code==0 and process_completed and container_absent and invariants and junit_summary is not None and junit_summary['allNineRegisteredExecutionPassed']
 result={'format':'nano-target-source-auth-amd64-root-dispatched-nine-result-v1','registeredTestsExecutionPassed':executed_pass,'semanticAuditQualified':False,'passedWithinTestScope':False,'semanticAuditStatus':'PENDING_SEPARATE_REAL_OUTPUT_AUDIT','selectedTests':SELECTED,'allNineActuallyExecutedRequired':True,'priorOldCoreResultsReused':False,'priorArmResultsReused':False,'failure':failure,'executionTerminalSha256':terminal_hash,'actualContainerAbsent':container_absent,'actualBindingsUnchanged':invariants,'noRelaunchAttempted':True,'privateInputsRead':False,'runtimeDutiesResumed':False,'productionQualified':False}; save(out/'result.json',result); event('root-dispatched-terminal',registeredTestsExecutionPassed=executed_pass,semanticAuditQualified=False,failure=failure); return 0 if executed_pass else 1
if __name__=='__main__': sys.exit(main())
