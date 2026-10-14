#!/usr/bin/env python3
"""ROOT only: two selected startup/deadline CTests on the new coherent runtime; never CAS/pending."""
import argparse,collections,hashlib,json,os,pathlib,posixpath,re,shlex,stat,subprocess,sys,time,types,uuid,xml.etree.ElementTree as ET
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
BUILD='4b018ba8bd7dffb4c9f581addadedf73293785c2aae085221977477199ba4d17'
NAMES=['consensus_capella_bootstrap_startup','consensus_capella_startup_deadline']
SOURCES={'nano_core':'src/consensus_capella_runtime.cpp','nano-capella-startup-deadline-tests':'tests/test_capella_startup_deadline.cpp'}
PROGRAMS={'nano','nano-capella-bootstrap-startup-tests','nano-capella-startup-deadline-tests'}
META_TARGETS=['nano_core','nano','nano-capella-bootstrap-startup-tests','nano-capella-startup-deadline-tests']
FRESH={'CMakeFiles/nano_core.dir/src/consensus_capella_runtime.cpp.o','CMakeFiles/nano-capella-startup-deadline-tests.dir/tests/test_capella_startup_deadline.cpp.o'}
GRAPHS=['build/CTestTestfile.cmake','build/third_party/secp256k1/CTestTestfile.cmake','build/third_party/secp256k1/src/CTestTestfile.cmake']
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(s):
 p=pathlib.Path(s);need(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def held(p,limit=16*1024*1024):
 p=canonical(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode) and a.st_size<=limit,'bounded regular control');parts=[];n=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   n+=len(b);need(n<=limit,'control byte bound');parts.append(b)
  z=os.fstat(fd);need(all(getattr(a,k)==getattr(z,k)==getattr(p.lstat(),k) for k in ('st_dev','st_ino','st_uid','st_gid','st_mode','st_size','st_mtime_ns')),'control changed');return b''.join(parts)
 finally:os.close(fd)
def main():
 ap=argparse.ArgumentParser()
 for k in ('controller-sha','guard','guard-sha','parent-plan','parent-plan-sha','build-root','build-terminal','build-terminal-sha','expected-build-controller-sha','metadata-manifest','metadata-manifest-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();need(all(re.fullmatch('[0-9a-f]{64}',getattr(a,k)) for k in ('controller_sha','guard_sha','parent_plan_sha','build_terminal_sha','expected_build_controller_sha','metadata_manifest_sha')),'strict external SHA pins')
 selfp=canonical(__file__);selfraw=held(selfp,1024*1024);need(sha(selfraw)==a.controller_sha,'runner SHA');guardp=canonical(a.guard);gb=held(guardp,1024*1024);need(a.guard_sha==GUARD and sha(gb)==GUARD,'qualified guards');m=types.ModuleType('maintenance_test_guards');m.__file__=str(guardp);exec(compile(gb,str(guardp),'exec'),m.__dict__)
 def sealed(path,h):
  raw=held(path);need(sha(raw)==h,'external public metadata SHA');return raw,json.loads(raw)
 planp=canonical(a.parent_plan);pr,p=sealed(planp,a.parent_plan_sha);arch=p['arch'];need(arch in m.IMAGES and p['image']==m.IMAGES[arch] and p['format']=='nano-bootstrap-owner-runtime-incremental-plan-v1' and p['status']=='READY_ROOT_DISPATCH_ONLY' and p['baselineGitCommit']==m.BASE,'existing image and parent policy')
 terminalp=canonical(a.build_terminal);tr,t=sealed(terminalp,a.build_terminal_sha);need(t['format']=='nano-capella-startup-deadline-incremental-build-terminal-v1' and t['status']=='BUILD_PASS_STARTUP_DEADLINE_TESTS_NOT_EXECUTED' and t['arch']==arch and a.expected_build_controller_sha==t['controllerSha256']==BUILD and t['guardSha256']==GUARD and t['parentPlanSha256']==a.parent_plan_sha and t['proposalSha256']=='9b5b160facb9fcef0a8e38b957ead7b5203dd458af9aee7fff349ede3d416d90','actual deadline V2 build authority')
 need(t['sourceFiles']==1319 and t['coreFresh']==1 and t['coreReused']==141 and t['newTestObjectsCompiled']==1 and t['consumerObjectsReused']==2 and t['links']==3 and t['archiveMembers']==142 and len(t['coreObjects'])==142 and len(t['consumerObjects'])==3 and set(t['programs'])==PROGRAMS and t['archiveConstructionCommandsExecuted']==2 and t['archiveVerificationCommandsExecuted']==143 and t['testsExecuted']==0,'actual fresh/reused142 and three coherent new links')
 need(all(t[k] is True for k in ('oldCompilerCommandsUnchanged','publicRuntimeHeaderUnchanged','allFreshActualMdEqualPreflight','publicSourceSdkLibrariesGeneratedAndParentsUnchanged','pendingCurrentSourceOverlayNotCompiledOrExecuted','oldCasObjectOnlyProtectedNotClaimedCurrent')) and all(t[k] is False for k in ('imageBuiltOrPulled','privateInputsRead','runtimeStarted','productionQualified')),'actual scoped build guards')
 root=canonical(a.build_root);need(root.name=='run-01' and terminalp==root/'receipts/terminal-build.json','exact completed run-01 build root');build=canonical(root/'build');source=canonical(root/'source');mp=canonical(a.metadata_manifest);mr,metadata=sealed(mp,a.metadata_manifest_sha)
 allow={'receipts/source-before.json','receipts/sdk-binding.json','receipts/sdk-before.log','receipts/sdk-after.log','receipts/archive-142-raw-member-proof.json','receipts/image-inspect.log','build/compile_commands.json',*GRAPHS}
 allow.update('build/CMakeFiles/'+target+'.dir/'+leaf for target in META_TARGETS for leaf in ('flags.make','link.txt'))
 for obj in FRESH:m.relative(obj);allow.update({'build/'+obj+'.d','build/preflight/'+obj+'.d'})
 need(set(metadata)=={'format','terminalSha256','buildControllerSha256','parentPlanSha256','files'} and metadata['format']=='nano-capella-startup-deadline-public-build-bindings-v1' and metadata['terminalSha256']==a.build_terminal_sha and metadata['buildControllerSha256']==BUILD and metadata['parentPlanSha256']==a.parent_plan_sha,'external completed build metadata seal')
 rows=metadata['files'];need(isinstance(rows,list) and len(rows)==len(allow) and {r['path'] for r in rows}==allow and all(set(r)=={'path','sha256','bytes'} and re.fullmatch('[0-9a-f]{64}',r['sha256']) and type(r['bytes']) is int and r['bytes']>0 for r in rows),'exact public metadata allowlist')
 out=canonical(a.output);need(not out.exists() and out.parent.is_dir() and not any(overlap(out,q) for q in (root,selfp.parent,guardp.parent,planp.parent,mp.parent)),'fresh separated output');need(os.geteuid()==p['uid'] and os.getegid()==p['gid'] and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'public owner and clean environment');expected={};ctimes=[]
 def pin(path,h=None,n=None,old=None):
  path=canonical(path);s=m.observed(path,arch);need((h is None or s['sha256']==h) and (n is None or s['st_size']==n),'public byte pin')
  if old is not None:
   need(s['sha256']==old['sha256'] and all(s[k]==old[k] for k in m.material(arch)),'build material admission')
   if s['st_ctime_ns']!=old['st_ctime_ns']:ctimes.append({'scope':'build-admission','path':str(path),'old':old['st_ctime_ns'],'new':s['st_ctime_ns'],'materialAndBytesEqual':True,'causeMeasured':False})
  expected[str(path)]=s;return s
 for path,b in ((selfp,selfraw),(guardp,gb),(planp,pr),(terminalp,tr),(mp,mr)):pin(path,sha(b),len(b))
 bound={}
 for r in rows:
  n=m.relative(r['path']);b,s=m.observed(root/n,arch,16*1024*1024,True);need(s['sha256']==r['sha256'] and s['st_size']==r['bytes'],'same authenticated metadata buffer');expected[str(root/n)]=s;bound[n]=b
 source_map=json.loads(bound['receipts/source-before.json']);need(len(source_map)==1319,'entire actual source map')
 for n,s in source_map.items():m.relative(n);pin(source/n,s['sha256'],s['st_size'],s)
 for n,s in {**t['coreObjects'],**t['consumerObjects'],**t['programs'],'libnano_core.a':t['archive']}.items():m.relative(n);pin(build/n,s['sha256'],s['st_size'],s)
 for table in ('libraries','generated'):
  for n,s in p[table].items():m.relative(n);pin(build/n,s['sha256'],s['bytes'])
 expected_source={'src/consensus_capella_runtime.cpp':('8e6cb49a36978961a9413f2d0f40f67f4ae0d05287530d691ee25dc4f834ae63',28424),'src/consensus_capella_startup_budget.hpp':('a585e6a9ec051761772795155aec063eaf566a5036c86cab638899e700f2f228',3799),'tests/test_capella_startup_deadline.cpp':('bf9cdddbafe1a704578605cf76faedf6320ddde1d515f4657c9ffa1d70dce117',19745),'include/nano/consensus_capella_runtime.hpp':('0692ae401d0f5a2ccc66ffba3fe8531644a20048a286536145ce9dd741cfc7f0',1408),'tests/test_capella_bootstrap_archive_cas.cpp':('b3d5b0c1328ad563852df097e94f82f948eeb3bde022087b6ac101601b6bcb37',61708)}
 need(all(source_map[n]['sha256']==h and source_map[n]['st_size']==size for n,(h,size) in expected_source.items()),'literal V2/source reconciliation/public ABI pins')
 cc=json.loads(bound['build/compile_commands.json'])
 for target,src in SOURCES.items():
  matches=[r for r in cc if r['file']=='/work/'+src and m.unit(r)[0].startswith('CMakeFiles/'+target+'.dir/')];need(len(matches)==1,'exact two fresh compiler argv');obj,argv=m.unit(matches[0]);need(obj in FRESH and obj in {**t['coreObjects'],**t['consumerObjects']} and m.dep(bound['build/'+obj+'.d'])==m.dep(bound['build/preflight/'+obj+'.d']),'actual fresh MD versus genuine probe')
 for target in PROGRAMS:
  link=shlex.split(bound['build/CMakeFiles/'+target+'.dir/link.txt'].decode());obj=next(n for n in t['consumerObjects'] if n.startswith('CMakeFiles/'+target+'.dir/'));need(link[0]=='/usr/bin/clang++' and link.count('-o')==1 and obj in link and link[link.index('-o')+1]==target and link.count('libnano_core.a')==1,'actual three linked programs')
 flags=bound['build/CMakeFiles/nano-capella-startup-deadline-tests.dir/flags.make'].decode();startup_flags=bound['build/CMakeFiles/nano-capella-bootstrap-startup-tests.dir/flags.make'].decode();need('-DNANO_TEST_SOURCE=\\\"/work\\\"' in flags and '-DNANO_TEST_NANO_BINARY=\\\"/outputs/nano\\\"' in startup_flags,'actual fixture and nano generator paths')
 proof=json.loads(bound['receipts/archive-142-raw-member-proof.json']);members=proof['members'];need(proof['archiveBeforeProof']['sha256']==t['archive']['sha256'] and proof['actualArBytes'] is True and proof['runtimeOneFresh141Reused'] is True and len(members)==142 and len({r['member'] for r in members})==142 and {r['object'] for r in members}==set(t['coreObjects']),'externally sealed actual142 raw members')
 for r in members:need(r['sha256']==t['coreObjects'][r['object']]['sha256'] and r['bytes']==t['coreObjects'][r['object']]['st_size'],'all actual raw member/current object bytes')
 sb=bound['receipts/sdk-before.log'];need(sb==bound['receipts/sdk-after.log'],'build SDK before/after bytes');binding=json.loads(bound['receipts/sdk-binding.json']);need(binding['equal'] is True and sha(sb)==binding['beforeSha256']==binding['afterSha256'],'actual SDK seal');sdkrows=[l.split('  ',1) for l in sb.decode().splitlines()];sdk={n:h for h,n in sdkrows};need(len(sdk)==len(sdkrows)==binding['paths'] and all(re.fullmatch('[0-9a-f]{64}',h) and n.startswith(('/usr/','/lib/')) and '\n' not in n for n,h in sdk.items()) and all(sdk.get(n)==h for n,h in p['sdkHashes'].items()),'SDK complete current/historical bindings')
 os.mkdir(out,0o700);m.save(out/'inputs-before.json',expected);ctest=out/'ctest';ctest.mkdir(mode=0o700)
 for n in GRAPHS:
  dst=ctest/pathlib.PurePosixPath(n).relative_to('build');dst.parent.mkdir(parents=True,exist_ok=True);fd=os.open(dst,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
  with os.fdopen(fd,'wb') as f:f.write(bound[n]);f.flush();os.fsync(f.fileno())
  pin(dst,sha(bound[n]),len(bound[n]))
 for n in ('third_party/ckzg','third_party/mcl'):need((build/n).is_dir(),'actual optional CTest directory');(ctest/n).mkdir(parents=True,exist_ok=True)
 sdk_script=out/'sdk.sh';sdk_script.write_text('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(sdk)));pin(sdk_script)
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};started=time.monotonic();failure=None;testexit=None;timedout=False;dispatch=False;absence=[];attempts=[];count=0;count_evidence='not-dispatched';ctest_elapsed=None
 def container(label,args,seconds):
  need(time.monotonic()-started<180,'finite180s active test bound');attempts.append(label);token=uuid.uuid4().hex;name='nano-startup-deadline-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name];tmpfs=f'uid={p["uid"]},gid={p["gid"]},mode=700'
  argv=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.startup.deadline.tests='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','1g','--pids-limit','256','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m,'+tmpfs,'--env','TMPDIR=/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs,readonly','--mount',f'type=bind,src={out},dst=/proof','--workdir','/tmp','--entrypoint',args[0],p['image']]+args[1:];m.save(out/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds});start=time.monotonic()
  try:
   with (out/(label+'.log')).open('xb',buffering=0) as log:r=subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,180-(time.monotonic()-started))))
   m.save(out/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-start});return r.returncode,held(out/(label+'.log'))
  except subprocess.TimeoutExpired:m.save(out/(label+'.exit.json'),{'exitCode':None,'timedOut':True,'elapsedSeconds':time.monotonic()-start});raise
  finally:
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if r.returncode==0:need(json.loads(r.stdout).get('nano.startup.deadline.tests')==token,'owned cleanup label');need(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:need(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'cleanup observation')
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);need(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'owned container remains');absence.append(label)
 try:
  image=json.loads(bound['receipts/image-inspect.log']);need(image['Id']==p['image'] and image['Architecture']==arch and image['Os']=='linux','build literal image observation')
  code,current=container('sdk-before',['/bin/sh','/proof/sdk.sh'],20);need(code==0 and {n:h for h,n in (l.split('  ',1) for l in current.decode().splitlines())}==sdk,'actual pretest SDK')
  pattern='^('+'|'.join(NAMES)+')$';code,discovery=container('discovery',['/usr/bin/ctest','--test-dir','/proof/ctest','--show-only=json-v1','-R',pattern],15);need(code==0,'real CTest selector discovery');graph=json.loads(discovery);tests={r['name']:r for r in graph['tests']};need(len(graph['tests'])==2 and set(tests)==set(NAMES),'exact two disjoint selected CTests')
  for name,target in zip(NAMES,['nano-capella-bootstrap-startup-tests','nano-capella-startup-deadline-tests']):
   test=tests[name];props={r['name']:r['value'] for r in test['properties']};need(test['command']==['/outputs/'+target,'--self-test'] and props['TIMEOUT']==70 and 'qualification_consensus_recovery' in props['LABELS'],'actual selectors/two registered70s limits')
  m.save(out/'selected-ctest-graph.json',graph);dispatch=True;count=None;count_evidence='unknown-after-dispatch';start=time.monotonic();testexit,log=container('ctest',['/usr/bin/ctest','--test-dir','/proof/ctest','--verbose','--output-on-failure','--output-junit','/proof/junit.xml','-R',pattern],150);ctest_elapsed=time.monotonic()-start;scenarios=[]
  for line in log.decode().splitlines():
   match=re.match(r'^\d+: (\{.*\})$',line)
   if match:
    row=json.loads(match.group(1))
    if 'scenario' in row:scenarios.append(row)
  m.save(out/'scenario-rows.json',scenarios);need(testexit==0,'new startup/deadline CTest failure');cases=list(ET.fromstring(held(out/'junit.xml')).iter('testcase'));need(len(cases)==2 and {x.attrib['name'] for x in cases}==set(NAMES) and all(float(x.attrib['time'])>0 and x.attrib.get('status')=='run' and all(x.find(k) is None for k in ('failure','error','skipped')) for x in cases),'two actual no-skip JUnit records')
  startup=['pinned-proof-inputs-and-refusals','post-validation-artifact-rebinding-refused','strict-existing-owner-never-initializes','explicit-two-pin-runtime-startup-before-clock','fresh-typed-bootstrap-archives-before-clock','main-cli-typed-recovery-refuses-absent-primary','bootstrap-startup-self-test'];deadline=['startup-deadline-boundaries-and-compatibility','startup-deadline-owned-inputs-and-error-types','startup-deadline-real-runtime-expiration','startup-deadline-durable-prefix-reopen','startup-deadline-handoff-and-retained-callback-lifetime','capella-startup-deadline-self-test'];by={r['scenario']:r for r in scenarios};need(len(scenarios)==13 and len(by)==13 and set(by)==set(startup+deadline) and all(r['result']=='PASS' and r['privateInputsRead'] is False and r['productionQualified'] is False for r in scenarios),'actual five+summary and six+summary rows only')
  for name in deadline:need(by[name]['newSignaturesCreated']==0 and by[name]['runtimeUTCQualified'] is False and by[name]['runtimeDutiesResumed'] is False,'deadline scope zero new signatures/no actual UTC or duties')
  for name in startup[:-1]:need(by[name]['runtimeDutiesResumed'] is False,'startup scoped cases')
  checks={deadline[0]:{'exactBoundaryRefused':True,'invalidBeforeProfileRead':True,'legacyNoSteadyRead':True},deadline[1]:{'copiedBudgetAndConfiguration':True,'callerCancellationTypePreserved':True,'invalidProfileStillRefused':True},deadline[2]:{'realSteadyUsed':True,'deadlineTypeObserved':True,'noRuntimeHandleReturned':True,'noPublicationObserved':True},deadline[3]:{'actualClockPublications':1,'currentPrimaryAndWitnessRetained':True,'oldPrimaryPinRefused':True,'rawSnapshotPreserved':True,'privateJournalFloorsQualified':False},deadline[4]:{'retainedCheckSafeAfterScope':True,'deadlineInactiveBeforeWorkers':True,'realTimePastStartupBudget':True,'legacyAndBudgetedRuntimeReturnsObserved':2,'coldReadAfterHandoff':True}}
  for name,values in checks.items():need(all(by[name][k]==v for k,v in values.items()),'actual deadline assertion group '+name)
  need(type(by[deadline[2]]['callerWaitObserved']) is bool,'real wait observation may be false if expiration precedes caller');final=by[deadline[-1]];need(final['groups']==5 and final['completedTestRuntimeStarts']==2 and type(final['elapsedMs']) is int and 0<=final['elapsedMs']<60000 and type(final['maxRssKiB']) is int and 0<final['maxRssKiB']<=131072 and final['networkUsed'] is False and final['powerLossQualified'] is False,'actual deadline resource/groups/returns scope');sf=by[startup[-1]];need(sf['newSignaturesCreated']==0 and all(sf[k] is False for k in ('hostClockRead','clockRebased','workersStarted','networkUsed','privateInputsRead','productionQualified')),'startup zero-signature/no clock/duties scope')
 except subprocess.TimeoutExpired as e:timedout=True;failure='TimeoutExpired: '+str(e)
 except BaseException as e:failure=type(e).__name__+': '+str(e)
 finally:
  if dispatch and (out/'junit.xml').is_file():
   try:
    cases=list(ET.fromstring(held(out/'junit.xml')).iter('testcase'));need(1<=len(cases)<=2 and len({r.attrib['name'] for r in cases})==len(cases) and all(r.attrib['name'] in NAMES and r.attrib.get('status') in ('run','fail','notrun','disabled') for r in cases),'observed selected JUnit');count=sum(r.attrib['status'] in ('run','fail') and r.find('skipped') is None for r in cases);count_evidence='actual-ctest-junit-records'
   except BaseException:count=None;count_evidence='unknown-incomplete-junit'
  try:
   code,current=container('sdk-after',['/bin/sh','/proof/sdk.sh'],20);need(code==0 and {n:h for h,n in (l.split('  ',1) for l in current.decode().splitlines())}==sdk,'posttest SDK equality');after={}
   for path,s in expected.items():
    z=m.observed(path,arch);need(z['sha256']==s['sha256'] and all(z[k]==s[k] for k in m.material(arch)),'all public input readback');after[path]=z
    if z['st_ctime_ns']!=s['st_ctime_ns']:ctimes.append({'scope':'test-readback','path':path,'old':s['st_ctime_ns'],'new':z['st_ctime_ns'],'materialAndBytesEqual':True,'causeMeasured':False})
   m.save(out/'inputs-after.json',after);m.save(out/'ctime-diagnostics.json',ctimes)
  except BaseException as e:failure=(failure or '')+'; readback '+str(e)
  m.save(out/'terminal.json',{'format':'nano-capella-startup-deadline-two-ctest-terminal-v1','passedWithinOwnedStartupDeadlineTestScope':failure is None,'arch':arch,'controllerSha256':a.controller_sha,'buildControllerSha256':BUILD,'parentPlanSha256':a.parent_plan_sha,'buildTerminalSha256':a.build_terminal_sha,'metadataManifestSha256':a.metadata_manifest_sha,'testsRequested':NAMES,'dispatchAttempted':dispatch,'testsExecuted':count,'executionCountEvidence':count_evidence,'testsPassed':2 if failure is None else None,'ctestExitCode':testexit,'ctestElapsedSeconds':ctest_elapsed,'timedOut':timedout,'elapsedSeconds':time.monotonic()-started,'failure':failure,'inputReadbackPassed':(out/'inputs-after.json').is_file(),'containersObservedAbsent':absence,'allOwnedContainersObservedAbsent':attempts==absence,'semanticRowsExpected':13,'newSignaturesCreated':0,'syntheticDeadlineRuntimeReturnsExpected':2,'privateJournalFloorsQualified':False,'physicalPowerLossQualified':False,'privateInputsRead':False,'runtimeDutiesResumed':False,'currentUtcQualified':False,'finalityProgressionQualified':False,'productionQualified':False})
 return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
