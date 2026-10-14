#!/usr/bin/env python3
"""ROOT only: one newly built maintenance CTest, never inherited startup/pending tests."""
import argparse,collections,hashlib,json,os,pathlib,posixpath,re,shlex,stat,subprocess,sys,time,types,uuid,xml.etree.ElementTree as ET
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
BUILD='ce27006e9c62118a4d4b82362a6b9e11a0ec103e4290627446143ea2432b80f5'
NAME='storage_capella_bootstrap_archive_owned_copies'
SOURCES={'nano-bootstrap-archive-maintenance':'tools/storage-maintenance/bootstrap-archive-owned-copies.cpp','nano-bootstrap-archive-maintenance-tests':'tests/test_bootstrap_archive_maintenance.cpp','nano-bootstrap-maintenance-retention-fault':'tests/support/bootstrap_maintenance_retention_fault.cpp'}
PROGRAMS={'nano-bootstrap-archive-maintenance','nano-bootstrap-archive-maintenance-tests','libnano-bootstrap-maintenance-retention-fault.so'}
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
 terminalp=canonical(a.build_terminal);tr,t=sealed(terminalp,a.build_terminal_sha);need(t['format']=='nano-bootstrap-maintenance-one-consumer-build-terminal-v1' and t['status']=='BUILD_PASS_MAINTENANCE_TEST_NOT_EXECUTED' and t['arch']==arch and a.expected_build_controller_sha==t['controllerSha256']==BUILD and t['guardSha256']==GUARD and t['parentPlanSha256']==a.parent_plan_sha,'actual maintenance build authority')
 need(t['sourceFiles']==1320 and t['coreObjectsCompiled']==t['coreObjectsCopied']==t['archiveConstructionCommandsExecuted']==t['testsExecuted']==0 and t['parentCoreObjectsReobserved']==t['archiveVerificationCommandsExecuted']==0 and t['archiveMembers']==142 and t['newObjectsCompiled']==t['newLinks']==1 and t['oldObjectsByteCopied']==t['oldProgramsByteCopied']==2 and t['rawMemberProofReusedOnlyForByteIdenticalArchive'] is True and t['sourceChanged']==['tests/test_bootstrap_archive_maintenance.cpp'] and t['sourceSha256']=='e5f664121c238c1db2e94283fa96d9959ecabcd566028dcdd5deb0df0f9ce1ee' and t['harnessProposalSha256']=='dae95f972b019595e295e8a14c7afeca8773c84b232234cd8aee7773c76c5da0' and len(t['objects'])==3 and set(t['programs'])==PROGRAMS,'zero core/one harness CPP/link plus two unchanged copied objects/programs')
 need(all(t[k] is True for k in ('archiveByteCopiedFromQualifiedParent','oldCompilerCommandsUnchanged','allActualMdEqualPreflight','sourceSdkLibrariesGeneratedAndParentsUnchanged')) and all(t[k] is False for k in ('imageBuiltOrPulled','privateInputsRead','runtimeStarted','productionQualified')),'actual scoped build guards')
 root=canonical(a.build_root);need(root.name=='run-01' and terminalp==root/'receipts/terminal-build.json','exact completed run-01 build root');build=canonical(root/'build');source=canonical(root/'source');mp=canonical(a.metadata_manifest);mr,metadata=sealed(mp,a.metadata_manifest_sha)
 allow={'receipts/source-before.json','receipts/sdk-binding.json','receipts/sdk-before.log','receipts/sdk-after.log','receipts/archive-142-actual-byte-proof.json','receipts/image-inspect.log','build/compile_commands.json',*GRAPHS}
 allow.update('build/CMakeFiles/'+target+'.dir/'+leaf for target in SOURCES for leaf in ('flags.make','link.txt'))
 for obj in t['objects']:m.relative(obj);allow.update({'build/'+obj+'.d','build/preflight/'+obj+'.d'})
 need(set(metadata)=={'format','terminalSha256','buildControllerSha256','parentPlanSha256','files'} and metadata['format']=='nano-bootstrap-maintenance-public-build-bindings-v1' and metadata['terminalSha256']==a.build_terminal_sha and metadata['buildControllerSha256']==BUILD and metadata['parentPlanSha256']==a.parent_plan_sha,'external completed build metadata seal')
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
 source_map=json.loads(bound['receipts/source-before.json']);need(len(source_map)==1320,'entire actual source map')
 for n,s in source_map.items():m.relative(n);pin(source/n,s['sha256'],s['st_size'],s)
 for n,s in {**t['objects'],**t['programs'],'libnano_core.a':t['archive']}.items():m.relative(n);pin(build/n,s['sha256'],s['st_size'],s)
 for table in ('libraries','generated'):
  for n,s in p[table].items():m.relative(n);pin(build/n,s['sha256'],s['bytes'])
 cc=json.loads(bound['build/compile_commands.json'])
 for target,src in SOURCES.items():
  matches=[r for r in cc if r['file']=='/work/'+src and m.unit(r)[0].startswith('CMakeFiles/'+target+'.dir/')];need(len(matches)==1,'exact three compiled target argv');obj,argv=m.unit(matches[0]);need(obj in t['objects'] and m.dep(bound['build/'+obj+'.d'])==m.dep(bound['build/preflight/'+obj+'.d']),'actual MD versus genuine dependency probe')
  link=shlex.split(bound['build/CMakeFiles/'+target+'.dir/link.txt'].decode());need(link[0]=='/usr/bin/clang++' and link.count('-o')==1 and obj in link and link[link.index('-o')+1] in PROGRAMS and link.count('libnano_core.a')==(0 if target=='nano-bootstrap-maintenance-retention-fault' else 1),'actual three link graph')
 flags=bound['build/CMakeFiles/nano-bootstrap-archive-maintenance-tests.dir/flags.make'].decode();need('-DNANO_TEST_SOURCE=\\\"/work\\\"' in flags and '-DNANO_TEST_TOOL=\\\"/outputs/nano-bootstrap-archive-maintenance\\\"' in flags and '-DNANO_TEST_FSYNC_SHIM=\\\"/outputs/libnano-bootstrap-maintenance-retention-fault.so\\\"' in flags,'actual tool and Linux fault shim paths')
 proof=json.loads(bound['receipts/archive-142-actual-byte-proof.json']);members=proof['members'];need(proof['archiveBeforeProof']['sha256']==t['archive']['sha256'] and proof['archiveCreatedOrModified'] is False and proof['parentProofByteCopiedArchiveOnly'] is True and len(members)==142 and len({r['member'] for r in members})==len({r['object'] for r in members})==142 and all(re.fullmatch('[0-9a-f]{64}',r['sha256']) and type(r['bytes']) is int and r['bytes']>0 for r in members),'externally sealed actual142 raw member proof')
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
  need(time.monotonic()-started<400,'finite400s outer bound');attempts.append(label);token=uuid.uuid4().hex;name='nano-maint-one-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name];tmpfs=f'uid={p["uid"]},gid={p["gid"]},mode=700'
  argv=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.maintenance.one='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','1g','--pids-limit','256','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m,'+tmpfs,'--env','TMPDIR=/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs,readonly','--mount',f'type=bind,src={out},dst=/proof','--workdir','/tmp','--entrypoint',args[0],p['image']]+args[1:];m.save(out/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds});start=time.monotonic()
  try:
   with (out/(label+'.log')).open('xb',buffering=0) as log:r=subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,400-(time.monotonic()-started))))
   m.save(out/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-start});return r.returncode,held(out/(label+'.log'))
  except subprocess.TimeoutExpired:m.save(out/(label+'.exit.json'),{'exitCode':None,'timedOut':True,'elapsedSeconds':time.monotonic()-start});raise
  finally:
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if r.returncode==0:need(json.loads(r.stdout).get('nano.maintenance.one')==token,'owned cleanup label');need(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:need(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'cleanup observation')
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);need(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'owned container remains');absence.append(label)
 try:
  image=json.loads(bound['receipts/image-inspect.log']);need(image['Id']==p['image'] and image['Architecture']==arch and image['Os']=='linux','build literal image observation')
  code,current=container('sdk-before',['/bin/sh','/proof/sdk.sh'],45);need(code==0 and {n:h for h,n in (l.split('  ',1) for l in current.decode().splitlines())}==sdk,'actual pretest SDK')
  pattern='^'+NAME+'$';code,discovery=container('discovery',['/usr/bin/ctest','--test-dir','/proof/ctest','--show-only=json-v1','-R',pattern],20);need(code==0,'real discovery');graph=json.loads(discovery);need(len(graph['tests'])==1,'only newly selected maintenance test');test=graph['tests'][0];props={r['name']:r['value'] for r in test['properties']};need(test['name']==NAME and test['command']==['/outputs/nano-bootstrap-archive-maintenance-tests','--self-test'] and props['TIMEOUT']==130 and 'qualification_consensus_bootstrap_maintenance' in props['LABELS'],'registered command timeout label');m.save(out/'selected-ctest-graph.json',graph)
  dispatch=True;count=None;count_evidence='unknown-after-dispatch';start=time.monotonic();testexit,log=container('ctest',['/usr/bin/ctest','--test-dir','/proof/ctest','--verbose','--output-on-failure','--output-junit','/proof/junit.xml','-R',pattern],155);ctest_elapsed=time.monotonic()-start;scenarios=[]
  for line in log.decode().splitlines():
   match=re.match(r'^\d+: (\{.*\})$',line)
   if match:
    row=json.loads(match.group(1))
    if 'scenario' in row:scenarios.append(row)
  m.save(out/'scenario-rows.json',scenarios);need(testexit==0,'new maintenance CTest failure');cases=list(ET.fromstring(held(out/'junit.xml')).iter('testcase'));need(len(cases)==1 and cases[0].attrib['name']==NAME and float(cases[0].attrib['time'])>0 and all(cases[0].find(k) is None for k in ('failure','error','skipped')) and cases[0].attrib.get('status')=='run','one actual no-skip JUnit')
  expected_counts={'maintenance-existing-owner-refuses-initialization':1,'maintenance-authenticated-stop-and-actual-exclusive-lock':1,'maintenance-provenance-alias-hardlink-and-budget':1,'maintenance-retained-pins-and-protected-output-refusals':1,'maintenance-retention-file-and-directory-fsync-refusals':1,'maintenance-real-apply-cold-readback-and-completed-recovery':3,'maintenance-stale-original-apply-refused':3,'maintenance-existing-archive-no-work-apply':1,'bootstrap-archive-maintenance-wrapper-self-test':1};need(len(scenarios)==13 and collections.Counter(r['scenario'] for r in scenarios)==expected_counts and all(r['result']=='PASS' and r['newSignaturesCreated']==0 and r['privateInputsRead'] is False and r['productionQualified'] is False for r in scenarios),'exact thirteen semantic rows zero new signatures')
  by={r['scenario']:r for r in scenarios};checks={'maintenance-existing-owner-refuses-initialization':{'missingPrimaryBindings':4,'missingWitnessIdentity':True,'missingDatabaseOrCurrentNeverCreated':True,'canonicalStateKeyDerivedFromTip':True,'missingCanonicalStateRefused':True,'historicalStateAbsentCurrentStatePresentAccepted':True},'maintenance-authenticated-stop-and-actual-exclusive-lock':{'falseStopReceiptRefused':True,'freshExecLockAcquisitionObserved':True,'sourcesNeverOpenedRw':True},'maintenance-provenance-alias-hardlink-and-budget':{'cases':8,'sourceHashAliasSymlinkHardlinkScanBudgetForeignGenesisOverlapAndAbsenceRefused':True,'allProtocolRowsAndOriginalSourcesUnchanged':True},'maintenance-retained-pins-and-protected-output-refusals':{'cases':7,'missingSwappedWrongHashWrongNextNonPrivateAndExistingOutputRefused':True,'noSupplementOnRefusal':True},'maintenance-retention-file-and-directory-fsync-refusals':{'actualFsyncFaultCases':3,'twoFilesAndPostWitnessDirectoryBoundaries':True,'allBeforeProtocolPublication':True},'maintenance-existing-archive-no-work-apply':{'twoTokensUnchanged':True,'ordinaryCompleteInitialIntentSupported':True}}
  for name,values in checks.items():need(all(by[name][k]==v for k,v in values.items()),'actual wrapper safety assertions '+name)
  for name in ('maintenance-real-apply-cold-readback-and-completed-recovery','maintenance-stale-original-apply-refused'):need({r['actualDurableFormat'] for r in scenarios if r['scenario']==name}=={'hot','cold','fragmented'},'each actual durable format exactly once')
  for r in scenarios:
   if r['scenario']=='maintenance-real-apply-cold-readback-and-completed-recovery':need(all(r[k] is True for k in ('archiveAndIndexRetained','rawSnapshotAndAllUnrelatedNamespacesExact','bothNextReretainedBeforeIo','coldTypedReopen','ordinaryRecoveryRefused')) and r['completedRecoveryRepetitions']==2 and r['interruptedRecoveryQualified'] is False,'actual apply and completed recovery')
  final=by['bootstrap-archive-maintenance-wrapper-self-test'];need(type(final['elapsedMs']) is int and 0<=final['elapsedMs']<120000 and type(final['maxRssKiB']) is int and 0<final['maxRssKiB']<=131072 and final['retentionFsyncFaultCases']==3 and final['actualCliUsed'] is True and final['alreadySignedPublicFixtureOnly'] is True and all(final[k] is False for k in ('wholeNodeBackupQualified','interruptedWalRecoveryQualified','physicalPowerLossQualified','clockReadOrReset','dutiesResumed','networkUsed')),'bounded public fixture maintenance scope')
 except subprocess.TimeoutExpired as e:timedout=True;failure='TimeoutExpired: '+str(e)
 except BaseException as e:failure=type(e).__name__+': '+str(e)
 finally:
  if dispatch and (out/'junit.xml').is_file():
   try:
    cases=list(ET.fromstring(held(out/'junit.xml')).iter('testcase'));need(len(cases)==1 and cases[0].attrib['name']==NAME and cases[0].attrib.get('status') in ('run','fail','notrun','disabled'),'observed exact JUnit');count=int(cases[0].attrib['status'] in ('run','fail') and cases[0].find('skipped') is None);count_evidence='actual-ctest-junit-record'
   except BaseException:count=None;count_evidence='unknown-incomplete-junit'
  try:
   code,current=container('sdk-after',['/bin/sh','/proof/sdk.sh'],45);need(code==0 and {n:h for h,n in (l.split('  ',1) for l in current.decode().splitlines())}==sdk,'posttest SDK equality');after={}
   for path,s in expected.items():
    z=m.observed(path,arch);need(z['sha256']==s['sha256'] and all(z[k]==s[k] for k in m.material(arch)),'all public input readback');after[path]=z
    if z['st_ctime_ns']!=s['st_ctime_ns']:ctimes.append({'scope':'test-readback','path':path,'old':s['st_ctime_ns'],'new':z['st_ctime_ns'],'materialAndBytesEqual':True,'causeMeasured':False})
   m.save(out/'inputs-after.json',after);m.save(out/'ctime-diagnostics.json',ctimes)
  except BaseException as e:failure=(failure or '')+'; readback '+str(e)
  m.save(out/'terminal.json',{'format':'nano-bootstrap-maintenance-one-ctest-terminal-v1','passedWithinOwnedMaintenanceTestScope':failure is None,'arch':arch,'controllerSha256':a.controller_sha,'buildControllerSha256':BUILD,'parentPlanSha256':a.parent_plan_sha,'buildTerminalSha256':a.build_terminal_sha,'metadataManifestSha256':a.metadata_manifest_sha,'testsRequested':[NAME],'dispatchAttempted':dispatch,'testsExecuted':count,'executionCountEvidence':count_evidence,'testsPassed':1 if failure is None else None,'ctestExitCode':testexit,'ctestElapsedSeconds':ctest_elapsed,'timedOut':timedout,'elapsedSeconds':time.monotonic()-started,'failure':failure,'inputReadbackPassed':(out/'inputs-after.json').is_file(),'containersObservedAbsent':absence,'allOwnedContainersObservedAbsent':attempts==absence,'semanticRowsExpected':13,'newSignaturesCreated':0,'interruptedWalRecoveryQualified':False,'wholeNodeBackupQualified':False,'physicalPowerLossQualified':False,'privateInputsRead':False,'runtimeDutiesResumed':False,'currentUtcQualified':False,'finalityProgressionQualified':False,'productionQualified':False})
 return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
