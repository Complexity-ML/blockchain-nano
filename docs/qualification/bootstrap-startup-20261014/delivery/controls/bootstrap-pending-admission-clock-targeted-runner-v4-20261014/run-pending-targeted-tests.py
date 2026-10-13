#!/usr/bin/env python3
"""ROOT only: new pending once; startup first time on AMD only. No old test reuse."""
import argparse,hashlib,json,os,pathlib,re,shlex,stat,subprocess,sys,time,types,uuid,xml.etree.ElementTree as ET
BUILD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
NAMES=['consensus_capella_bootstrap_archive_pending_recovery','consensus_capella_bootstrap_startup']
REPAIR='1da7fbfb6e5651ba6b6f8ac61215426ca75abcd0bc2ff26e0d187faf523c69b4'
TARGETS=['nano','nano-capella-bootstrap-archive-cas-tests','nano-capella-bootstrap-startup-tests']
def require(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(s):
 p=pathlib.Path(s);require(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def held(p,limit=16*1024*1024):
 p=canonical(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);require(stat.S_ISREG(a.st_mode) and a.st_size<=limit,'bounded regular control');chunks=[];n=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   n+=len(b);require(n<=limit,'control byte limit');chunks.append(b)
  z=os.fstat(fd);fields=('st_dev','st_ino','st_uid','st_gid','st_mode','st_size','st_mtime_ns');require(all(getattr(a,k)==getattr(z,k)==getattr(p.lstat(),k) for k in fields),'control changed');return b''.join(chunks)
 finally:os.close(fd)
def main():
 ap=argparse.ArgumentParser()
 for k in ('controller-sha','build-controller','build-controller-sha','plan','plan-sha','build-terminal','build-terminal-sha','repair-terminal','repair-terminal-sha','repair-root','repair-controller','repair-controller-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();selfp=canonical(__file__);selfraw=held(selfp,1024*1024);require(sha(selfraw)==a.controller_sha,'external runner SHA');modulep=canonical(a.build_controller);module_bytes=held(modulep,1024*1024);require(a.build_controller_sha==BUILD and sha(module_bytes)==BUILD,'exact frozen build guard SHA')
 m=types.ModuleType('bootstrap_owner_build_guards');m.__file__=str(modulep);exec(compile(module_bytes,str(modulep),'exec'),m.__dict__)
 planp=canonical(a.plan);raw=held(planp);require(sha(raw)==a.plan_sha,'external plan SHA');p=json.loads(raw);arch=p['arch'];require(arch in m.IMAGES and p['image']==m.IMAGES[arch] and p['baselineGitCommit']==m.BASE and p['format']=='nano-bootstrap-owner-runtime-incremental-plan-v1' and p['status']=='READY_ROOT_DISPATCH_ONLY','exact candidate build policy')
 terminalp=canonical(a.build_terminal);rawt=held(terminalp);require(sha(rawt)==a.build_terminal_sha,'externally pinned actual terminal');t=json.loads(rawt);require(t['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and t['format']=='nano-bootstrap-owner-runtime-incremental-build-terminal-v1' and t['planSha256']==a.plan_sha and t['controllerSha256']==BUILD and t['arch']==arch and t['archiveMembers']==142 and t['sourceFiles']==1317,'actual new142 build only')
 require(len(t['coreObjects'])==142 and len(t['consumerObjects'])==3 and set(t['programs'])==set(TARGETS) and t['counts']['links']==3 and t['testsExecuted']==0 and t['allActualMdEqualPreflight'] and t['publicParentsAndSdkUnchanged'],'actual codegen/link evidence, no inherited test passes');require(t['counts']['freshCore']+t['counts']['newCore']+t['counts']['reusedCore']==142 and t['counts']['newCore']==1 and t['counts']['newTest']==1,'actual142 fresh/reuse/new tally')
 repair_controller=canonical(a.repair_controller);repair_controller_raw=held(repair_controller,1024*1024);require(a.repair_controller_sha==REPAIR and sha(repair_controller_raw)==REPAIR,'exact externally approved one-consumer controller');parent=canonical(p['outputRoot']);pb=canonical(parent/'build');repairp=canonical(a.repair_terminal);repairraw=held(repairp);require(sha(repairraw)==a.repair_terminal_sha,'externally pinned actual repair terminal');rt=json.loads(repairraw);require(rt['format']=='nano-pending-fixture-one-consumer-build-terminal-v1' and rt['status']=='BUILD_PASS_PENDING_TEST_NOT_EXECUTED' and rt['controllerSha256']==REPAIR and rt['guardSha256']==BUILD and rt['arch']==arch and rt['parentPlanSha256']==a.plan_sha and rt['parentTerminalSha256']==a.build_terminal_sha and rt['candidateSha256']=='b6b7c4504ab02bc20c83f90688f9038ff9d118b616ed41f2c7db4eff59e53ff8' and rt['sourceSha256']=='b3d5b0c1328ad563852df097e94f82f948eeb3bde022087b6ac101601b6bcb37' and rt['coreObjectsCompiled']==0 and rt['archiveCommandsExecuted']==0 and rt['consumerObjectsCompiled']==1 and rt['consumerLinks']==1 and rt['archiveMembers']==142 and rt['sourceFiles']==1317,'new true one-consumer build evidence');require(rt['archive']['sha256']==t['archive']['sha256'] and rt['librariesGeneratedSdkAndAllParentsUnchanged'],'same qualified core archive after repair')
 root=canonical(a.repair_root);build=canonical(root/'build');source=canonical(root/'source');receipts=canonical(root/'receipts');out=canonical(a.output);names=NAMES[:1]
 require(not out.exists() and out.parent.is_dir() and not any(overlap(out,q) for q in (build,source,root/'controls',parent,selfp.parent,planp.parent,repair_controller.parent)),'fresh separated test output')
 require(os.geteuid()==p['uid'] and os.getegid()==p['gid'] and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'public test owner/environment');expected={};ctimes=[]
 def pin(path,hash_value=None,length=None,old=None):
  path=canonical(path);s=m.observed(path,arch);require((hash_value is None or s['sha256']==hash_value) and (length is None or s['st_size']==length),'public input bytes')
  if old is not None:require(s['sha256']==old['sha256'] and all(s[k]==old[k] for k in m.material(arch)),'actual build material identity')
  expected[str(path)]=s;return held(path) if length is not None and length<=16*1024*1024 else s
 def doc(path):
  b=held(path);pin(path,sha(b),len(b));return json.loads(b)
 for path,b in ((selfp,selfraw),(modulep,module_bytes),(planp,raw),(terminalp,rawt),(repairp,repairraw),(repair_controller,repair_controller_raw)):pin(path,sha(b),len(b))
 sourcepins={**p['sourcePins'],**p['overlays']};observed_source=doc(receipts/'source-before.json');require(len(observed_source)==1317 and set(observed_source)==set(sourcepins),'all current candidate source inputs')
 for n,s in observed_source.items():
  expected_sha='b3d5b0c1328ad563852df097e94f82f948eeb3bde022087b6ac101601b6bcb37' if n=='tests/test_capella_bootstrap_archive_cas.cpp' else sourcepins[n]['sha256'];expected_size=61708 if n=='tests/test_capella_bootstrap_archive_cas.cpp' else sourcepins[n]['bytes'];require(s['sha256']==expected_sha and s['st_size']==expected_size,'source map externally bound');pin(source/n,s['sha256'],s['st_size'],s)
 pin(build/'libnano_core.a',rt['archive']['sha256'],rt['archive']['st_size'],rt['archive'])
 for field in ('coreObjects','consumerObjects','programs'):
  for n,s in t[field].items():pin(pb/n,s['sha256'],s['st_size'],s)
 for n,s in rt['programs'].items():pin(build/n,s['sha256'],s['st_size'],s)
 require(set(rt['programs'])==set(TARGETS) and all(rt['programs'][n]['sha256']==t['programs'][n]['sha256'] for n in ['nano','nano-capella-bootstrap-startup-tests']),'startup/main byte-copied unchanged')
 for field in ('object','actualMd','preflightMd'):
  r=rt[field];pin(build/r['path'],r['sha256'],r['st_size'],r)
 require(m.dep(held(build/rt['actualMd']['path']))==m.dep(held(build/rt['preflightMd']['path']))==m.dep(held(pb/rt['actualMd']['path'])),'single repair real-M/actualMD/original compilation closure')
 parent_source=doc(parent/'receipts/source-before.json')
 for n,r in parent_source.items():pin(parent/'source'/n,r['sha256'],r['st_size'],r)
 for n in ['compile_commands.json','CTestTestfile.cmake','CMakeFiles/nano_core.dir/flags.make','CMakeFiles/nano_core.dir/link.txt']+[f'CMakeFiles/{target}.dir/{leaf}' for target in TARGETS for leaf in ('flags.make','link.txt')]:pin(pb/n)
 for table in ('libraries','generated'):
  for n,s in p[table].items():pin(build/n,s['sha256'],s['bytes']);pin(pb/n,s['sha256'],s['bytes'])
 for r in p['authorManifests']+p['references']:pin(r['path'],r['sha256'],r['bytes'])
 rawproof=doc(parent/'receipts/archive-142-raw-member-proof.json');require(rawproof['archiveBeforeProof']['sha256']==rt['archive']['sha256'] and len(rawproof['members'])==142 and len({r['member'] for r in rawproof['members']})==142 and {r['object'] for r in rawproof['members']}==set(t['coreObjects']),'raw142 proof reused only for byte-identical copied archive')
 for r in rawproof['members']:require(r['sha256']==t['coreObjects'][r['object']]['sha256'] and r['bytes']==t['coreObjects'][r['object']]['st_size'],'qualified raw member/object identity')
 for n in ['compile_commands.json','CTestTestfile.cmake','CMakeFiles/nano_core.dir/flags.make','CMakeFiles/nano_core.dir/link.txt']+[f'CMakeFiles/{target}.dir/{leaf}' for target in TARGETS for leaf in ('flags.make','link.txt')]:pin(build/n)
 startup_flags=held(build/'CMakeFiles/nano-capella-bootstrap-startup-tests.dir/flags.make').decode();require('-DNANO_TEST_NANO_BINARY=\\\"/outputs/nano\\\"' in startup_flags,'actual nano generator path /outputs/nano')
 before_sdk=held(receipts/'sdk-before.log');after_sdk=held(receipts/'sdk-after.log');binding=doc(receipts/'sdk-binding.json');require(before_sdk==after_sdk and sha(before_sdk)==binding['beforeSha256']==binding['afterSha256'] and binding['equal'],'actual build SDK equality');pin(receipts/'sdk-before.log',sha(before_sdk),len(before_sdk));pin(receipts/'sdk-after.log',sha(after_sdk),len(after_sdk))
 sdkrows=[l.split('  ',1) for l in before_sdk.decode().splitlines()];sdk={path:h for h,path in sdkrows};require(len(sdk)==len(sdkrows)==binding['paths'] and all(re.fullmatch('[0-9a-f]{64}',h) and n.startswith(('/usr/','/lib/')) and '\n' not in n for n,h in sdk.items()) and all(sdk.get(n)==h for n,h in p['sdkHashes'].items()),'complete bound compiler/link SDK')
 os.mkdir(out,0o700);m.save(out/'inputs-before.json',expected);tmpfs=f'uid={p["uid"]},gid={p["gid"]},mode=700';env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};start=time.monotonic();failure=None;testexit=None;timedout=False;dispatch_attempted=False;observed_count=0;count_evidence="dispatch-not-attempted";absence=[]
 testing=build/'Testing';require(not testing.is_symlink() and (not testing.exists() or testing.is_dir()),'ordinary CTest tmpfs mount point')
 if not testing.exists():testing.mkdir(mode=0o700)
 sdk_script=out/'sdk.sh';sdk_script.write_text('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(sdk)));pin(sdk_script)
 def container(label,args,seconds):
  require(time.monotonic()-start<200,'200s bounded campaign');token=uuid.uuid4().hex;name='nano-owner-two-tests-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name]
  argv=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.owner.two.tests='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','1g','--pids-limit','256','--tmpfs','/tmp:rw,nosuid,nodev,size=512m,'+tmpfs,'--tmpfs','/outputs/Testing:rw,nosuid,nodev,noexec,size=32m,'+tmpfs,'--env','TMPDIR=/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs,readonly','--mount',f'type=bind,src={out},dst=/proof','--workdir','/tmp','--entrypoint',args[0],p['image']]+args[1:];m.save(out/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds})
  try:
   with (out/(label+'.log')).open('xb',buffering=0) as log:r=subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,200-(time.monotonic()-start))))
   m.save(out/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False});return r.returncode,held(out/(label+'.log'))
  finally:
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if r.returncode==0:require(json.loads(r.stdout).get('nano.owner.two.tests')==token,'owned cleanup label');require(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:require(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'cleanup inspection')
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);require(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'owned container remains');absence.append(label)
 try:
  code,current=container('sdk-before',['/bin/sh','/proof/sdk.sh'],20);require(code==0 and {n:h for h,n in (l.split('  ',1) for l in current.decode().splitlines())}==sdk,'actual pretest SDK bytes')
  pattern='^('+'|'.join(names)+')$';code,discovery=container('discovery',['/usr/bin/ctest','--test-dir','/outputs','--show-only=json-v1','-R',pattern],20);require(code==0,'real CTest discovery');graph=json.loads(discovery);tests={r['name']:r for r in graph['tests']};require(len(graph['tests'])==len(names) and set(tests)==set(names),'only two new registered tests')
  for name,target,selector in zip(names,TARGETS[1:],['--pending-recovery-tests','--self-test']):require(tests[name]['command']==['/outputs/'+target,selector] and next(v['value'] for v in tests[name]['properties'] if v['name']=='TIMEOUT')==70,'exact selectors/timeouts70')
  launcher=out/'ctest.sh';launcher.write_text('#!/bin/sh\nset -u\n/usr/bin/ctest --test-dir /outputs --verbose --output-on-failure --output-junit /proof/junit.xml -R '+shlex.quote(pattern)+'\nstatus=$?\nif [ -f /outputs/Testing/Temporary/LastTest.log ]; then /bin/cp /outputs/Testing/Temporary/LastTest.log /proof/LastTest.log; fi\nexit "$status"\n');pin(launcher);dispatch_attempted=True;observed_count=None;count_evidence="unknown-after-dispatch-attempt";testexit,log=container('ctest',['/bin/sh','/proof/ctest.sh'],150);rows=[]
  for line in log.decode().splitlines():
   match=re.match(r'^\d+: (\{.*\})$',line)
   if match:rows.append(json.loads(match.group(1)))
  m.save(out/'scenario-rows.json',rows);require(testexit==0,'new two-test CTest failure')
  junit=ET.fromstring(held(out/'junit.xml'));cases=list(junit.iter('testcase'));require(len(cases)==len(names) and {x.attrib['name'] for x in cases}==set(names) and all(x.find('skipped') is None and x.find('failure') is None and x.find('error') is None and x.attrib.get('status') not in ('notrun','disabled') for x in cases),'two genuine no-skip JUnit results')
  byname={r['scenario']:r for r in rows};pending=['pending-hot-archive-only-supplement','pending-fragmented-sigkill-retained-wal','pending-fragmented-sigkill-simulated-wal-loss','pending-cold-sigkill-retained-wal','pending-cold-sigkill-simulated-wal-loss','bootstrap-archive-pending-recovery-tests'];startup=['pinned-proof-inputs-and-refusals','post-validation-artifact-rebinding-refused','strict-existing-owner-never-initializes','explicit-two-pin-runtime-startup-before-clock','fresh-typed-bootstrap-archives-before-clock','main-cli-typed-recovery-refuses-absent-primary','bootstrap-startup-self-test'];require(len(rows)==6 and set(byname)==set(pending+[]) and all(r['result']=='PASS' and r['productionQualified'] is False for r in rows),'actual thirteen new scenario/summary rows only')
  summary=byname[pending[-1]];require(summary['newCases']==5 and summary['syntheticSignaturesCreated']==3 and summary['childSignaturesCreated']==0 and summary['actualSigkillCases']==4 and summary['elapsedMs']<60000 and summary['maxRssKiB']<=131072 and summary['nonemptyDurablePendingQueueCovered'] and summary['coldAndFragmentedInterruptedRecoveryCovered'] and summary['testOnlyKeysAndDomain'] and not summary['physicalPowerLossQualified'] and not summary['networkUsed'] and not summary['runtimeDutiesResumed'] and not summary['privateKeysOrSigningEnvRead'],'pending genuine signature/resource/scope guards')
  for name in pending[1:5]:
   r=byname[name];require(r['queueEntries']==1 and r['actualPrimarySyncInterceptionAndSigkill'] and r['volatileWalSuffixLossSimulated']==('simulated' in name) and r['childSignaturesCreated']==0 and not r['physicalPowerLossQualified'] and r['rawEnvelopeDigestAllBlobsAndLogicalContentPreserved'] and r['genericRecoveryRefusedWithoutImplicitRepair'] and r['typedProofAndTwoNextPinsRecover'] and r['witnessNeverRewrittenByRecovery'] and r['operationBytesAndIdempotencePreserved'],'actual interrupted nonempty invariants')
  if len(names)>1:
   final=byname[startup[-1]];require(final['newSignaturesCreated']==0 and not any(final[k] for k in ('hostClockRead','clockRebased','workersStarted','networkUsed','privateInputsRead','productionQualified')),'startup zero-signature/no-clock/no-duty scope')
 except subprocess.TimeoutExpired as e:timedout=True;failure='TimeoutExpired: '+str(e)
 except BaseException as e:failure=type(e).__name__+': '+str(e)
 finally:
  if dispatch_attempted and (out/'junit.xml').is_file():
   try:
    actual_cases=list(ET.fromstring(held(out/'junit.xml')).iter('testcase'));require(1<=len(actual_cases)<=len(names) and len({r.attrib['name'] for r in actual_cases})==len(actual_cases) and all(r.attrib['name'] in names for r in actual_cases),'observed JUnit names');require(all(r.attrib.get('status') in ('run','fail','notrun','disabled') for r in actual_cases),'unambiguous JUnit run status');observed_count=sum(r.attrib['status'] in ('run','fail') and r.find('skipped') is None for r in actual_cases);count_evidence='actual-ctest-junit-run-records'
   except BaseException:observed_count=None;count_evidence='unknown-unusable-or-incomplete-junit'
  if failure is None:observed_count=len(names);count_evidence='exact-selected-junit-and-all-actual-scenario-rows'
  try:
   code,current=container('sdk-after',['/bin/sh','/proof/sdk.sh'],20);require(code==0 and {n:h for h,n in (l.split('  ',1) for l in current.decode().splitlines())}==sdk,'posttest SDK byte equality')
   after={}
   for path,s in expected.items():
    z=m.observed(path,arch);require(z['sha256']==s['sha256'] and all(z[k]==s[k] for k in m.material(arch)),'public test input readback');after[path]=z
    if z['st_ctime_ns']!=s['st_ctime_ns']:ctimes.append({'path':path,'before':s['st_ctime_ns'],'after':z['st_ctime_ns'],'materialBytesEqual':True,'causeMeasured':False})
   m.save(out/'inputs-after.json',after);m.save(out/'ctime-diagnostics.json',ctimes)
  except BaseException as e:failure=(failure or '')+'; readback '+str(e)
  m.save(out/'terminal.json',{'format':'nano-pending-fixture-targeted-tests-terminal-v1','passedWithinOwnedTestScope':failure is None,'arch':arch,'runnerSha256':a.controller_sha,'buildControllerSha256':BUILD,'planSha256':a.plan_sha,'buildTerminalSha256':a.build_terminal_sha,'repairTerminalSha256':a.repair_terminal_sha,'testsRequested':names,'dispatchAttempted':dispatch_attempted,'testsExecuted':observed_count,'executionCountEvidence':count_evidence,'testsPassed':len(names) if failure is None else None,'ctestExitCode':testexit,'timedOut':timedout,'elapsedSeconds':time.monotonic()-start,'failure':failure,'inputReadbackPassed':(out/'inputs-after.json').exists(),'containersObservedAbsent':absence,'syntheticParentSignaturesExpected':3,'syntheticChildSignaturesExpected':0,'startupNewSignaturesExpected':0,'startupFirstQualificationOnAmdOnly':False,'armStartupRepeated':False,'nanoExecutedOnlyForNegativeCliGuard':False,'actualSigkillCasesExpected':4,'volatileWalSuffixLossExplicitlySimulated':True,'physicalPowerLossQualified':False,'privateInputsRead':False,'runtimeDutiesResumed':False,'currentUtcQualified':False,'finalityProgressionQualified':False,'productionQualified':False})
 return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
