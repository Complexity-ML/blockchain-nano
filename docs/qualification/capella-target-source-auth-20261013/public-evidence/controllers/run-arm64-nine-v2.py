#!/usr/bin/env python3
"""Root-only ARM9 CTest proposal. No build, SSH, campaign DB or prior-pass reuse.
Current byte/provenance completion is distinct from historical ctime qualification.
"""
import argparse,hashlib,json,os,pathlib,re,signal,stat,subprocess,sys,time,uuid,xml.etree.ElementTree as ET
BUILD_PLAN_SHA='6ec997308e7a08b579dc7497c76570cd9640af671a818fe8b2ad3c9115083e1e'
CORE_SHA='da39fde2008dcbe960e67b10e0d538f2456aab9d19c86756d291533f99113b06'
TEST_SHA='276674201c356da8d9d15a308c544bc86dcedbec609731d975ee53afb5b45dd7'
SELECTED=['storage_capella_blob_gc_next_publication','storage_capella_blob_gc_adversarial_boundaries','storage_capella_blob_gc_natural_pruning','storage_capella_blob_gc_cli_closed_fixture','storage_checkpoint_immutable_blobs','consensus_capella_fragment_checkpoint_migration','consensus_capella_duty_projection_availability','consensus_bls_positive_verification_cache','consensus_capella_target_source_authentication']
FIELDS=('st_dev','st_ino','st_uid','st_gid','st_mode','st_size','st_mtime_ns')
def require(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(p):
 p=pathlib.Path(p);require(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def raw(p,limit=16*1024*1024):
 p=canonical(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);require(stat.S_ISREG(a.st_mode) and a.st_size<=limit,'bounded regular public input');b=b''
  while True:
   part=os.read(fd,1024*1024)
   if not part:break
   b+=part;require(len(b)<=limit,'public byte bound')
  z=os.fstat(fd);require(all(getattr(a,k)==getattr(z,k) for k in FIELDS),'input stable fields changed');require(all(getattr(z,k)==getattr(p.lstat(),k) for k in FIELDS),'input replaced');return b
 finally:os.close(fd)
def pinned(p,h,limit=16*1024*1024):
 require(isinstance(h,str) and re.fullmatch('[0-9a-f]{64}',h),'literal external SHA');b=raw(p,limit);require(sha(b)==h,'public SHA mismatch');return b

def observe(p):
 p=canonical(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);require(stat.S_ISREG(a.st_mode) and a.st_size<=64*1024*1024,'bounded regular compiled/source input');h=hashlib.sha256()
  while True:
   b=os.read(fd,1024*1024)
   if not b:break
   h.update(b)
  z=os.fstat(fd);require(all(getattr(a,k)==getattr(z,k) for k in FIELDS),'observed stable fields changed');require(all(getattr(z,k)==getattr(p.lstat(),k) for k in FIELDS),'observed path replaced');return {'sha256':h.hexdigest(),**{k:getattr(z,k) for k in FIELDS},'st_ctime_ns':z.st_ctime_ns}
 finally:os.close(fd)
def compare(a,b,where,diagnostics):
 require(a['sha256']==b['sha256'] and all(a[k]==b[k] for k in FIELDS),'bytes/identity/owner/mode/mtime drift: '+where)
 if a['st_ctime_ns']!=b['st_ctime_ns']:diagnostics.append({'path':where,'beforeCtimeNs':b['st_ctime_ns'],'afterCtimeNs':a['st_ctime_ns'],'causeEstablished':False,'allOtherFieldsAndShaRequiredEqual':True})
def save(p,v):
 b=(json.dumps(v,indent=2)+'\n').encode();fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
 return sha(b)
def write(p,b):
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 for n in ['controller-sha','plan','plan-sha','docker','build-terminal-sha']:parser.add_argument('--'+n,required=True)
 a=parser.parse_args();selfpath=canonical(__file__);pinned(selfpath,a.controller_sha,1024*1024);plan=json.loads(pinned(a.plan,a.plan_sha));base=canonical(plan['buildRoot']);build=base/'build';source=base/'source'
 require(plan['format']=='nano-capella-source-auth-arm64-nine-test-plan-v1' and plan['selectedTests']==SELECTED and plan['outerTimeoutSeconds']==1330,'exact planned9/budget');require(plan.get('fixtureRootMustBeSetByRootBeforeDispatch') is False,'pending fixture draft not executable')
 require(os.geteuid()==501 and os.getegid()==20,'Mac owner501:20');require(not any(n.startswith(('NANO_','LD_','DYLD_')) for n in os.environ),'private/interposition environment names refused without reading values')
 docker=pathlib.Path(a.docker);require(docker.is_absolute() and docker.is_file() and os.access(docker,os.X_OK),'explicit Docker CLI');env={'PATH':'/usr/local/bin:/usr/bin:/bin','LC_ALL':'C'}
 terminalPath=canonical(plan['buildTerminal']);require(terminalPath==base/'receipts/terminal-build-receipt.json','exact new terminal path');terminal=json.loads(pinned(terminalPath,a.build_terminal_sha))
 require(terminal['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and terminal['format']=='nano-capella-target-source-auth-arm64-incremental-terminal-v1' and terminal['coreCandidateManifestSha256']==CORE_SHA and terminal['testCandidateManifestSha256']==TEST_SHA,'actual V2 core/V4 test build proof')
 require((terminal['coreFresh'],terminal['coreReused'],terminal['archiveMembers'],terminal['newLinks'],terminal['freshExistingConsumerObjects'],terminal['reusedConsumerObjects'],terminal['freshTinyConsumerObjects'])==(27,113,140,10,6,3,1) and terminal['testsExecuted']==0 and terminal['actualDependenciesEqualPreflight'] and terminal['allParentSourceLibraryGeneratedSdkMaterialBindingsUnchanged'] and terminal['darwinCtimeDiagnosticOnly'] and terminal['initialHistoricalStatsInvented'] is False and terminal['oldMdFabricated'] is False,'actual coherent build scope')
 bp=json.loads(pinned(plan['buildPlan'],BUILD_PLAN_SHA));require(terminal['planSha256']==BUILD_PLAN_SHA and bp['outputRoot']==str(base) and bp['coreCandidateManifest']['sha256']==CORE_SHA and bp['testCandidateManifest']['sha256']==TEST_SHA,'external build plan identity')
 expectedSource={**bp['sourceHashes'],**bp['overlays']};sourceRows=json.loads(raw(base/'receipts/source-after.json'));require(len(expectedSource)==len(sourceRows)==1304 and set(sourceRows)==set(expectedSource),'actual1304 source allowlist');protected={}
 for rel,row in sourceRows.items():
  require(isinstance(rel,str) and rel and not rel.startswith('/') and '..' not in pathlib.PurePosixPath(rel).parts and row['sha256']==expectedSource[rel],'exact public source row');protected[str(canonical(source/rel))]=row
 require(expectedSource['tests/test_capella_target_source_authentication.cpp']=='17e32c0f79c784e16d8561fa7812d1c75eff438f95842d1c3fb0303bd3c77525','actual V4 tiny source')
 member=json.loads(raw(base/'receipts/archive-140-raw-member-proof.json'));require(member['archive']==terminal['archive'] and member['arPInvocations']==280 and len(member['members'])==140 and {r['object'] for r in member['members']}=={r['object'] for r in bp['core']},'actual raw140 archive authority')
 for row in member['members']:
  p=canonical(build/row['object']);current=observe(p);require(current['sha256']==row['sha256'] and current['st_size']==row['bytes'],'current object raw member payload');protected[str(p)]=current
 protected[str(build/'libnano_core.a')]=terminal['archive'];bound={r['target']:r for r in terminal['programs']};require(len(terminal['programs'])==len(bound)==10 and set(bound)==set(plan['expectedPrograms']),'exact ten current programs')
 objectPaths={c['target']:c['object'] for c in bp['consumers']};objectPaths[bp['newTarget']]='CMakeFiles/'+bp['newTarget']+'.dir/'+bp['newSource']+'.o';programRows=json.loads(raw(base/'receipts/programs-final-readback.json'));buildReadbackCtime=[]
 for name,row in bound.items():
  require(row['coreArchiveSha256']==terminal['archive']['sha256'] and row['binaryReused'] is False,'all ten new links into exact archive')
  for part,p in [('object',build/objectPaths[name]),('binary',build/name)]:compare(programRows[name+'/'+part],row[part],name+'/'+part,buildReadbackCtime);protected[str(canonical(p))]=row[part]
 for receipt in ['generated-compiler-metadata-after.json','copied-dependencies-before-first-compile.json','generated-inputs-before-first-compile.json']:
  for rel,row in json.loads(raw(base/'receipts'/receipt)).items():protected[str(canonical(build/rel))]=row
 for name in ['compile-commands-original.json','sdk-hashes.sh','members.txt','member-proof.sh']:protected[str(canonical(base/'controls'/name))]=observe(base/'controls'/name)
 sdkExpected=raw(base/'receipts/sdk-before.log');require(sdkExpected==raw(base/'receipts/sdk-after.log') and len(sdkExpected.decode().splitlines())==2496,'actual SDK2496 build beforeafter')
 graph=canonical(build/'CTestTestfile.cmake');graphbytes=raw(graph);protected[str(graph)]=observe(graph);chosen=[line for line in graphbytes.decode().splitlines() if any(line.startswith(prefix+'([=['+n+']=]') for n in SELECTED for prefix in ['add_test','set_tests_properties'])];require(len(chosen)==18,'eighteen actual registered lines')
 fixtureManifest=json.loads(pinned(plan['fixtureManifest'],plan['fixtureManifestSha256']));require(len(fixtureManifest['files'])==23 and fixtureManifest['privateDataCopied'] is False,'public23fixtures')
 require(canonical(plan['fixtureRoot'])==source/'tests/fixtures','fixtures are already included in1304source closure')
 for row in fixtureManifest['files']:
  p=canonical(source/'tests/fixtures'/row['path']);require(str(p) in protected and protected[str(p)]['sha256']==row['sha256'] and protected[str(p)]['st_size']==row['bytes'],'bound fixture exact')
 for name in SELECTED:
  add=[l for l in chosen if l.startswith('add_test([=['+name+']=]')];prop=[l for l in chosen if l.startswith('set_tests_properties([=['+name+']=]')];timeout=70 if name in SELECTED[-3:] else 90 if name=='storage_checkpoint_immutable_blobs' else 180
  require(len(add)==len(prop)==1 and add[0]==plan['expectedAddTestLines'][name] and re.search(r'TIMEOUT "'+str(timeout)+'"',prop[0]),'actual selected command/timeout/name');require('${' not in add[0] and all(x not in add[0] for x in ['PRIVATE_KEY','NETWORK_KEY','--export','--run-network']),'no private/runtime registration')
 graphSha=sha(graphbytes);protectedCount=len(protected)
 out=canonical(plan['output']);require(out.parent==base and re.fullmatch('tests-root-dispatched-[a-z0-9-]+',out.name) and not out.exists(),'fresh test output');os.mkdir(out,0o700);testing=out/'testing';testing.mkdir(mode=0o700);(testing/'ctest').mkdir(mode=0o700)
 before={};after={};admission=[];postctime=[];proc=None;launched=False;completed=False;timedout=False;exitcode=None;ctestexit=None;containerAbsent=False;bindings=False;sdkEqual=False;failure=None;name='nano-bls-arm-'+uuid.uuid4().hex[:16];owner=uuid.uuid4().hex
 def command(label,argv):
  save(out/(label+'.command.json'),argv);r=subprocess.run(argv,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=25,env=env);write(out/(label+'.log'),r.stdout);save(out/(label+'.exit.json'),{'exitCode':r.returncode});return r
 def absent(label):
  r=command(label,[str(docker),'container','inspect','--format','{{json .Config.Labels}}',name])
  if r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout):return True
  require(r.returncode==0,'ambiguous container absence');return False
 def cleanup():
  if absent('cleanup-inspect-before'):return True
  r=command('cleanup-owner',[str(docker),'container','inspect','--format','{{json .Config.Labels}}',name]);require(r.returncode==0 and json.loads(r.stdout).get('nano.bls.arm.owner')==owner,'owned cleanup only');r=command('cleanup-remove',[str(docker),'container','rm','--force',name]);require(r.returncode==0,'owned container removal');return absent('cleanup-inspect-after')
 try:
  for p,row in protected.items():current=observe(p);compare(current,row,p,admission);before[p]=current
  write(testing/'ctest/CTestTestfile.cmake',('# Actual source graph SHA '+graphSha+'\n'+'\n'.join(chosen)+'\n').encode())
  write(testing/'run-nine.sh',('''#!/bin/sh
set -u
/bin/sh /controls/sdk-hashes.sh > /testing/sdk-before.log || exit 91
/usr/bin/ctest --test-dir /testing/ctest --no-tests=error --output-on-failure --verbose --output-junit /testing/gc-selected.junit.xml
code=$?
printf '%s\\n' "$code" > /testing/actual-ctest-exit.txt
/bin/sh /controls/sdk-hashes.sh > /testing/sdk-after.log || exit 92
exit "$code"
''').encode());scriptBefore=observe(testing/'run-nine.sh');copyGraphBefore=observe(testing/'ctest/CTestTestfile.cmake')
  save(out/'before.json',{'controllerSha256':a.controller_sha,'planSha256':a.plan_sha,'actualBuildTerminalSha256':a.build_terminal_sha,'protected':before,'admissionCtimeDiagnostics':admission,'buildFinalReadbackCtimeDiagnostics':buildReadbackCtime,'registrationSourceSha256':graphSha,'oldPassesReused':False,'historicalStatsRequalified':False})
  r=command('image-inspect',[str(docker),'image','inspect','--format','{{json .}}',plan['image']]);require(r.returncode==0,'existing image inspection');image=json.loads(r.stdout);require(image['Id']==plan['image'] and image['Architecture']=='arm64' and image['Os']=='linux','exact image');require(not any(s.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for s in image.get('Config',{}).get('Env',[])),'image private/interposition environment')
  require(absent('before-launch-inspect'),'container name already owned elsewhere')
  argv=[str(docker),'run','--rm','--pull=never','--platform','linux/arm64','--name',name,'--label','nano.bls.arm.owner='+owner,'--network','none','--read-only','--user','501:20','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m,uid=501,gid=20,mode=700','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(build)+',dst=/outputs,readonly','--mount','type=bind,src='+str(base/'controls')+',dst=/controls,readonly','--mount','type=bind,src='+str(testing)+',dst=/testing','--env','TMPDIR=/tmp','--env','NANO_BLS_VERIFICATION_CACHE_METRICS=1','--workdir','/testing/ctest','--entrypoint','/bin/sh',plan['image'],'/testing/run-nine.sh']
  save(out/'ctest.command.json',argv)
  fd=os.open(out/'ctest.log',os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
  with os.fdopen(fd,'wb',buffering=0) as log:
   proc=subprocess.Popen(argv,stdout=log,stderr=subprocess.STDOUT,env=env,start_new_session=True);launched=True
   try:exitcode=proc.wait(timeout=1330);completed=True
   except subprocess.TimeoutExpired:timedout=True;os.killpg(proc.pid,signal.SIGTERM);raise ValueError('outer1330s timeout, no retry')
  if (testing/'actual-ctest-exit.txt').exists():s=raw(testing/'actual-ctest-exit.txt',32).decode().strip();require(re.fullmatch('[0-9]+',s),'actual CTest exit record');ctestexit=int(s)
  require(exitcode==0 and ctestexit==0,'actual CTest/SDK command failed; no relaunch')
 except BaseException as e:failure=type(e).__name__+': '+str(e)
 finally:
  if proc is not None and proc.poll() is None:
   try:os.killpg(proc.pid,signal.SIGKILL);exitcode=proc.wait(timeout=10);completed=True
   except BaseException as e:failure=(failure or '')+'; client cleanup '+str(e)
  try:containerAbsent=cleanup()
  except BaseException as e:failure=(failure or '')+'; owned cleanup '+str(e)
  try:
   for p,row in before.items():current=observe(p);compare(current,row,p,postctime);after[p]=current
   pinned(graph,graphSha);pinned(terminalPath,a.build_terminal_sha);pinned(a.plan,a.plan_sha);pinned(selfpath,a.controller_sha,1024*1024)
   if before:compare(observe(testing/'run-nine.sh'),scriptBefore,'test wrapper',postctime);compare(observe(testing/'ctest/CTestTestfile.cmake'),copyGraphBefore,'filtered registration',postctime)
   bindings=len(after)==protectedCount;save(out/'after.json',{'protected':after,'currentCtimeDiagnostics':postctime,'allBytesStableIdentityOwnerModeMtimeHeld':bindings,'ctimeEquality':not postctime,'historicalStatsRequalified':False})
   if (testing/'sdk-before.log').exists() and (testing/'sdk-after.log').exists():sdkEqual=raw(testing/'sdk-before.log')==raw(testing/'sdk-after.log')==sdkExpected;require(sdkEqual,'actual SDK2496 beforeafter or build binding changed')
  except BaseException as e:failure=(failure or '')+'; input binding '+str(e)
  execution={'format':'nano-capella-source-auth-arm-nine-execution-terminal-v1','dockerExitCode':exitcode,'actualCtestExitCode':ctestexit,'launched':launched,'completed':completed,'outerTimedOut':timedout,'containerAbsent':containerAbsent,'stableBindingsHeld':bindings,'sdkBeforeAfterEqual':sdkEqual,'ctimeDifferences':len(postctime),'historicalMetadataRequalified':False,'failure':failure,'runtimeDutiesResumed':False,'privateInputsRead':False,'productionQualified':False};save(out/'execution-terminal.json',execution)
 cases=[]
 if (testing/'gc-selected.junit.xml').exists():
  for c in ET.fromstring(raw(testing/'gc-selected.junit.xml')).iter('testcase'):cases.append({'name':c.get('name'),'status':c.get('status'),'seconds':c.get('time'),'failureOrSkip':any(c.find(k) is not None for k in ['failure','error','skipped'])})
 exact=len(cases)==9 and {c['name'] for c in cases}==set(SELECTED);allpass=exact and all(c['status']=='run' and not c['failureOrSkip'] for c in cases)
 result={'format':'nano-capella-source-auth-arm-nine-registered-result-v1','cases':cases,'nineRegisteredCtestsPassed':allpass and failure is None and bindings and sdkEqual and containerAbsent,'semanticAuditStillRequired':True,'receiptIsNotSecurityAudit':True,'noOldPassesReused':True,'noRelaunchAttempted':True,'historyCtimeFailurePreserved':True,'failure':failure,'productionQualified':False};save(out/'result.json',result);return 0 if result['nineRegisteredCtestsPassed'] else 1
if __name__=='__main__':sys.exit(main())
