#!/usr/bin/env python3
"""Root-dispatched GC build only. No make, tests, runtime or campaign reads.
The plan, this program, public source index and optional test-only overlay must
be approved by independent literal SHA arguments. All outputs are separate.
"""
import argparse,hashlib,json,os,pathlib,shlex,stat,subprocess,time
SHA=lambda b:hashlib.sha256(b).hexdigest()
META=lambda s:{'dev':s.st_dev,'ino':s.st_ino,'uid':s.st_uid,'gid':s.st_gid,'mode':s.st_mode,'bytes':s.st_size,'mtimeNs':s.st_mtime_ns,'ctimeNs':s.st_ctime_ns}
def regular(path):
 p=pathlib.Path(path);assert p.is_absolute() and p.resolve()==p
 for parent in [p,*p.parents]:assert not parent.is_symlink()
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  before=os.fstat(fd);assert stat.S_ISREG(before.st_mode)
  h=hashlib.sha256();count=0
  while True:
   b=os.read(fd,65536)
   if not b:break
   count+=len(b);h.update(b)
  after=os.fstat(fd);assert META(before)==META(after) and count==before.st_size
  return {'sha256':h.hexdigest(),**META(before)}
 finally:os.close(fd)
def load(path,pin):
 assert len(pin)==64 and set(pin)<=set('0123456789abcdef')
 p=pathlib.Path(path);assert p.is_absolute() and p.resolve()==p
 for parent in [p,*p.parents]:assert not parent.is_symlink()
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  before=os.fstat(fd);assert stat.S_ISREG(before.st_mode) and before.st_size<=16*1024*1024
  pieces=[];count=0
  while True:
   b=os.read(fd,65536)
   if not b:break
   pieces.append(b);count+=len(b);assert count<=16*1024*1024
  assert META(before)==META(os.fstat(fd)) and count==before.st_size
  data=b''.join(pieces);assert SHA(data)==pin
  return json.loads(data.decode('utf8','strict'))
 finally:os.close(fd)
def deps(path):
 t=pathlib.Path(path).read_text().replace('\\\n','').replace('\\\r\n','');at=t.index(':');assert at>0
 return sorted(set(shlex.split(t[at+1:])))
def source_rel(path):
 assert path.startswith('/work/');return path[len('/work/'):]
def object_rel(row):
 p=pathlib.PurePosixPath(row['output']);assert not p.is_absolute() and '..' not in p.parts and p.parts[0]=='CMakeFiles';return p.as_posix()
def copy_bytes(src,dest):
 assert not dest.exists();dest.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
 before=regular(src)
 with open(src,'rb') as i,open(dest,'xb') as o:
  while True:
   b=i.read(65536)
   if not b:break
   o.write(b)
  o.flush();os.fsync(o.fileno())
 assert regular(src)==before and regular(dest)['sha256']==before['sha256']
 # Deliberately no copy2, utime, original mtime or make timestamp bypass.
 return before

def main():
 if not __debug__:raise RuntimeError('Optimized Python removes qualification safety checks')
 parser=argparse.ArgumentParser();parser.add_argument('--plan',required=True);parser.add_argument('--expected-plan-sha256',required=True);parser.add_argument('--expected-driver-sha256',required=True);a=parser.parse_args()
 assert SHA(pathlib.Path(__file__).read_bytes())==a.expected_driver_sha256
 assert not any(k in {'LD_PRELOAD','DYLD_INSERT_LIBRARIES','NANO_SYNC_FAULT','NANO_TEST_SYNC_FAIL'} or k.startswith('NANO_TEST_CAPELLA_NETWORK_KEY_') for k in os.environ)
 p=load(a.plan,a.expected_plan_sha256);assert p['format']=='nano-gc-arm-test-only-consumer-correction-plan-v1'
 parent_terminal=load(p['parentTerminal']['path'],p['parentTerminal']['sha256']);assert p['parentTerminal']['sha256']=='d53fb809a3cb14008838e754f6fa21a0c02afe1331e78f32314d3785ad74f1da' and parent_terminal['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and parent_terminal['archive']['sha256']=='02b718039b73f1f49d0f96ef4c09ebe410a55f2578d82755f6f48da6109d7a96'
 test_manifest=load(p['testOnlyManifest']['path'],p['testOnlyManifest']['sha256']);assert p['testOnlyManifest']['sha256']=='43bcb3e3610a474ed6902dca5befbb4bdf4253f256534193d49f117274c5c940'
 source=pathlib.Path(p['sourceCopy']);build=pathlib.Path(p['buildOutput']);parents=pathlib.Path(p['parentBuildRoot']);receipts=pathlib.Path(p['receiptOutput'])
 for x in [source,build,parents]:assert x.resolve()==x and x.is_dir() and x!=pathlib.Path('/')
 assert source!=build and source not in build.parents and build not in source.parents and parents not in build.parents and build not in parents.parents
 assert build.stat().st_uid==os.getuid() and stat.S_IMODE(build.stat().st_mode)==0o700 and not list(build.iterdir())
 index=load(p['sourceIndex']['path'],p['sourceIndex']['sha256']);previous=load(p['previousSourceIndex']['path'],p['previousSourceIndex']['sha256']);assert p['previousSourceIndex']['sha256']=='59cfc6f7d8071e6045fa912dc4c5faeb89e86d8e7aa48dd7aede737d3018ce91' and index['privateDataCopied'] is False
 files={r['path']:r for r in index['files']};prev={r['path']:r for r in previous['files']};assert set(files)==set(prev)
 expected={r['path']:r['sha256'] for r in test_manifest['sourceFiles']};assert set(expected)=={'tests/support/capella_blob_gc_adversarial.hpp','tests/test_capella_transition.cpp'}
 assert {path for path in files if files[path]['sha256']!=prev[path]['sha256']}==set(expected)
 for path,pin in expected.items():assert files[path]['sha256']==pin
 assert files['src/storage.cpp']['sha256']=='4611df07f863f1b97e48ed305b50981f396f302fb019e945e35e47b8dce53891'
 assert not receipts.exists();receipts.mkdir(mode=0o700);os.umask(0o077)
 def save(name,value):
  with open(receipts/name,'x') as out:json.dump(value,out,indent=2);out.write('\n');out.flush();os.fsync(out.fileno())
 save('observed-empty-output.json',{'path':str(build),'entries':[],'beforeAnyBuildPreparation':True,'stat':META(build.stat())})
 def source_snapshot():
  result={}
  for path,r in files.items():
   rel=pathlib.PurePosixPath(path);assert not rel.is_absolute() and '..' not in rel.parts
   value=regular(source/rel);assert value['sha256']==r['sha256'] and value['bytes']==r['bytes'];result[path]=value
  return result
 src_before=source_snapshot();save('source-before.json',src_before)
 protected=[pathlib.Path(r['path']) for r in p['copyFiles']]+[pathlib.Path(r['path']) for r in p['protectedEvidence']]+[pathlib.Path(r['depfile']['path']) for r in p['consumerUnits'].values()]
 parent_before={str(x):regular(x) for x in protected}
 for r in p['copyFiles']+p['protectedEvidence']:assert parent_before[r['path']]['sha256']==r['sha256'] and parent_before[r['path']]['bytes']==r['bytes']
 save('parents-before.json',parent_before)
 archive_before=parent_before[str(parents/'libnano_core.a')];assert archive_before==parent_terminal['archive']
 member_proof=load(p['parentMemberProof']['path'],p['parentMemberProof']['sha256']);assert member_proof['archive']['sha256']==archive_before['sha256'] and len(member_proof['members'])==140
 parent_programs={r['target']:r for r in parent_terminal['programs']};assert set(parent_programs)==set(p['freshConsumerTargets'])
 for target,u in p['consumerUnits'].items():assert parent_before[u['depfile']['path']]['sha256']==u['depfile']['sha256']
 for r in p['copyFiles']:
  copy_bytes(pathlib.Path(r['path']),build/r['relativePath'])
  if r['relativePath'] in p['reusedConsumerObjects']:os.chmod(build/r['relativePath'],stat.S_IMODE(r['mode']))
 save('copy-passed-runtime-assets.json',{'files':p['copyFiles'],'archiveByteCopiedFromPassedParent':True,'coreObjectCopies':0,'archiveCommands':0,'sixProgramsByteCopiedNotRelinked':True,'sixConsumerObjectsCopiedNotCompiled':True,'mtimeRestored':False})
 sequence=0
 def docker(args,label,timeout=180):
  nonlocal sequence
  sequence+=1;name='nano-gc-testfix2-'+str(os.getpid())+'-'+str(sequence)
  cmd=[p['dockerBinary'],'run','--rm','--pull=never','--platform','linux/arm64','--name',name,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--cpus','2','--memory','4g','--pids-limit','256','--tmpfs','/tmp:rw,noexec,nosuid,size=64m','--env','TMPDIR=/outputs/tmp','--mount','type=bind,source='+str(source)+',target=/work,readonly','--mount','type=bind,source='+str(build)+',target=/outputs','--mount','type=bind,source='+str(parents)+',target=/parents,readonly','--workdir','/outputs','--entrypoint',args[0],p['image'],*args[1:]]
  save(label+'.command.json',{'argv':cmd,'timeoutSeconds':timeout});start=time.monotonic()
  with open(receipts/(label+'.log'),'xb') as log:
   try:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
   except subprocess.TimeoutExpired:
    cleanup=subprocess.run([p['dockerBinary'],'rm','-f',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);save(label+'.timeout.json',{'ownedContainer':name,'cleanupExitCode':cleanup.returncode});raise
  save(label+'.exit.json',{'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start});assert r.returncode==0;return (receipts/(label+'.log')).read_bytes()
 def image_snapshot(paths,label):
  assert all(x.startswith(('/usr/','/lib/')) for x in paths);body='set -euo pipefail; '+''.join('test -f '+shlex.quote(x)+'; printf "%s\\t" '+shlex.quote(x)+'; sha256sum -- '+shlex.quote(x)+'; ' for x in paths)
  directory=build/'sdk-worker-scripts';directory.mkdir(mode=0o700,exist_ok=True);script=directory/(label+'.sh');assert not script.exists()
  with open(script,'x') as stream:stream.write(body+'\n');stream.flush();os.fsync(stream.fileno())
  before=regular(script);raw=docker(['/bin/bash','/outputs/sdk-worker-scripts/'+script.name],label,120).decode();after=regular(script);keys=['sha256','bytes','dev','ino','uid','gid','mode'];save(label+'.worker-before-after.json',{'before':before,'after':after,'timestampsDiagnosticOnlyForAuxiliaryScript':True});assert {k:before[k] for k in keys}=={k:after[k] for k in keys}
  result={}
  for line in raw.splitlines():
   path,value=line.split('\t');pin,separator,target=value.partition('  ');assert separator and path==target and path not in result;result[path]=pin
  assert set(result)==set(paths);return result
 target='nano-capella-transition-tests';db=json.loads((build/'compile_commands.json').read_text());row=next(r for r in db if r['file']=='/work/'+p['freshConsumerTargets'][target] and '/'+target+'.dir/' in r['output']);assert row==p['consumerUnits'][target]['rawCommand']
 argv=shlex.split(row['command']);assert argv[0]=='/usr/bin/clang++' and argv.count('-c')==argv.count('-o')==1 and row['directory']=='/outputs' and '-DNANO_SYNC_FAULT=1' in argv
 at=argv.index('-o');pre=argv[:];del pre[at:at+2];pre.remove('-c');(build/'dependency-preflight').mkdir(mode=0o700);pre+=['-M','-MT',row['output'],'-MF','dependency-preflight/transition.d'];docker(pre,'dependencies-transition',180)
 normalized=sorted(set(os.path.normpath(os.path.join('/outputs',x)) for x in deps(build/'dependency-preflight/transition.d')));systems=set(p['parentSdkBindings']);generated=set()
 def check_paths(paths,expected_project=None):
  project=set()
  for path in paths:
   if path.startswith('/work/'):
    rel=source_rel(path);assert rel in files and rel not in {'.DS_Store','tests/.DS_Store'};assert expected_project is None or src_before[rel]['sha256']==expected_project.get(rel);project.add(rel)
   elif path.startswith('/outputs/'):
    rel=path[len('/outputs/'):];assert regular(build/rel)['sha256']==regular(parents/rel)['sha256'];generated.add(rel)
   else:assert path.startswith(('/usr/','/lib/'));systems.add(path)
  if expected_project is not None:assert project==set(expected_project)
 check_paths(normalized)
 for name,u in p['consumerUnits'].items():
  if name==target:continue
  paths=sorted(set(os.path.normpath(os.path.join('/outputs',x)) for x in deps(u['depfile']['path'])));check_paths(paths,{d['path']:d['sha256'] for d in u['dependencies']})
  assert regular(build/object_rel(u['rawCommand']))['sha256']==parent_programs[name]['object']['sha256'] and regular(build/name)['sha256']==parent_programs[name]['binary']['sha256']
 system_before=image_snapshot(sorted(systems),'image-sdk-before-compilation');assert all(system_before[x]==pin for x,pin in p['parentSdkBindings'].items())
 generated_before={path:regular(build/path) for path in sorted(generated)};save('generated-inputs-before-compilation.json',generated_before);save('dependency-preflight-one.json',{'compilerPreprocessingInvocations':1,'paths':normalized,'sdkBindings':system_before,'sourceAndGeneratedAndImageInputsBound':True})
 out=build/object_rel(row);assert not out.exists();out.parent.mkdir(parents=True,exist_ok=True,mode=0o700);compile_argv=argv+['-MD','-MT',row['output'],'-MF',row['output']+'.d'];docker(compile_argv,'consumer-transition',420)
 assert sorted(set(os.path.normpath(os.path.join('/outputs',x)) for x in deps(build/(row['output']+'.d'))))==normalized
 assert {path:regular(build/path) for path in generated_before}==generated_before
 link=shlex.split((build/('CMakeFiles/'+target+'.dir/link.txt')).read_text());assert link[0]=='/usr/bin/clang++' and link.count('-o')==1 and link[link.index('-o')+1]==target and link.count('libnano_core.a')==link.count('libnano-test-sync-fault.so')==1 and '-Wl,-rpath,/outputs' in link
 assert [x for x in link if x.endswith('.o')]==[row['output'],'third_party/mcl/bint64.o','third_party/mcl/base64.o']
 for operand in link:
  if operand.endswith(('.a','.so')):assert operand in p['allowedCopiedLibraries'] or operand in system_before
 assert not (build/target).exists();docker(link,'link-transition',180)
 def observe_new_output(path):
  # Bounded held-FD stabilization is ONLY for this newly linked owned output.
  # Existing source/parent/generated inputs retain strict regular/META equality.
  path=pathlib.Path(path);assert path.resolve()==path and not path.is_symlink();fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW);attempts=[];last=None
  try:
   for attempt in range(4):
    before=os.fstat(fd);assert stat.S_ISREG(before.st_mode);os.lseek(fd,0,os.SEEK_SET);digest=hashlib.sha256();count=0
    while True:
     chunk=os.read(fd,65536)
     if not chunk:break
     count+=len(chunk);digest.update(chunk)
    after=os.fstat(fd);row={'before':META(before),'after':META(after),'sha256':digest.hexdigest(),'bytesRead':count};attempts.append(row)
    stable_keys=['dev','ino','uid','gid','mode','bytes','mtimeNs'];assert count==before.st_size==after.st_size and {k:META(before)[k] for k in stable_keys}=={k:META(after)[k] for k in stable_keys},'New output content/identity/mtime changed during observation'
    if last:assert last['sha256']==row['sha256'] and last['bytesRead']==count and {k:last['after'][k] for k in stable_keys}=={k:row['before'][k] for k in stable_keys},'New output changed between full reads'
    if META(before)==META(after) and last and last['before']==last['after']==row['before']==row['after']:
     assert META(os.stat(path,follow_symlinks=False))==META(after);save('new-transition-output-held-observation.json',{'attempts':attempts,'twoCompleteHashesAndFullStableMetadataRequired':True,'anyEarlierCtimeOnlyChangeRecordedNotIgnoredForFinalPin':True,'sourceParentGeneratedInputPolicyUnchanged':True});return {'sha256':row['sha256'],**META(after)}
    last=row;time.sleep(0.05)
   save('new-transition-output-held-observation-refusal.json',{'attempts':attempts});raise RuntimeError('New linked output metadata did not stabilize within four held-FD complete reads')
  finally:os.close(fd)
 new_transition=observe_new_output(build/target);programs=[]
 for name,source_name in p['freshConsumerTargets'].items():
  consumer=p['consumerUnits'][name];programs.append({'target':name,'object':regular(build/object_rel(consumer['rawCommand'])),'binary':new_transition if name==target else regular(build/name),'coreArchiveSha256':archive_before['sha256'],'objectReusedFromPassedParent':name!=target,'binaryReusedFromPassedParent':name!=target})
 system_after=image_snapshot(sorted(systems),'image-sdk-after');assert system_after==system_before
 assert {path:regular(build/path) for path in generated_before}==generated_before;save('generated-inputs-after-compilation.json',generated_before)
 for r in p['copyFiles']:assert regular(build/r['relativePath'])['sha256']==r['sha256']
 assert regular(build/'libnano_core.a')['sha256']==archive_before['sha256']
 parent_after={path:regular(path) for path in parent_before};assert parent_after==parent_before;save('parents-after.json',parent_after)
 src_after=source_snapshot();assert src_after==src_before;save('source-after.json',src_after)
 save('terminal-build-receipt.json',{'format':'nano-gc-targeted-arm-build-terminal-v1','status':'BUILD_PASS_TESTS_NOT_EXECUTED','driverSha256':a.expected_driver_sha256,'planSha256':a.expected_plan_sha256,'sourceIndexSha256':p['sourceIndex']['sha256'],'testOverlayManifestSha256':previous['testOverlayManifestSha256'],'testOnlyManifestSha256':p['testOnlyManifest']['sha256'],'parentTerminalSha256':p['parentTerminal']['sha256'],'coreFresh':0,'coreReused':140,'consumerFresh':1,'consumerReused':6,'archiveMembers':140,'archive':regular(build/'libnano_core.a'),'archiveCopiedFromPassedParent':True,'archiveCommandsExecuted':0,'coreObjectsCompiled':0,'coreObjectCopies':0,'programs':programs,'programsRelinked':1,'programsCopiedFromPassedParent':6,'cmakeConfiguredThisLaunch':False,'parentSourceLibraryImageInputsUnchanged':True,'testsExecuted':0,'runtimeStarted':False,'privateInputsRead':False,'productionQualified':False})
 print(json.dumps({'status':'BUILD_PASS_TESTS_NOT_EXECUTED','freshCore':0,'archiveCopied':True,'freshConsumer':1,'programsLinked':1,'programsCopied':6,'tests':0}))
if __name__=='__main__':main()
