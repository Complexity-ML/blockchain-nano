#!/usr/bin/env python3
"""ROOT-only eight public cases. No compiler, old tests, DB or private inputs."""
import argparse,hashlib,json,os,pathlib,posixpath,re,shlex,stat,subprocess,sys,time,types,uuid
GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2'
BUILDER='374d954d1ef19d3e8f77fed6506064b9a5fbea566be6e9f0c1a57b50c24065e9'
SOURCE='d496f9e5ea94579cec22dc4f9cc43288b42d203e5704284a17cf88928784251f'
GENERATOR='e097b74eceab82e3e866193b1facb8f7c34e5cb8d916b9db0305ffc1315c67a1'
BUILDS={'arm64':('89fda17b4a3df47d0b2b72f7616cb144d141f8e31937927c3b8cda116eea5d07','f808abcf1a81ab0540845057eda6024ed0dfde8e788777f00626d0e57601fed7',501,20),'amd64':('cb96f1dc5e18a84476dae3c16889548706fe023fea2e96efa6101c5462bc2eeb','5bc6f87d8d5ff98f3050928602d6393f187580053cb9626879e973546c5fff8f',1000,1000)}
EXPECTED={'request-sha-refusal':('input_sha256_mismatch','request'),'snapshot-sha-refusal':('input_sha256_mismatch','public-input-authentication'),'head-state-binding-refusal':('current_head_domain_binding','current-snapshot-authentication'),'head-gap-refusal':('head_gap_or_next_slot_budget','current-utc-observation'),'chunk-count-refusal':('chunk_count_exceeded','empty-slot-projection'),'steady-deadline-refusal':('steady_deadline_expired',None),'fifo-type-refusal':('input_file_metadata','public-input-authentication')}
SAFETY={'resumeAuthorized':False,'clockPublished':False,'dbOpened':False,'journalsOpened':False,'keysLoaded':False,'newSignatures':0,'runtimeStarted':False,'networkStarted':False,'dutyReadinessQualified':False,'productionQualified':False}
def need(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(x):
 p=pathlib.Path(x);need(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlap(a,b):return a==b or a in b.parents or b in a.parents
def main():
 if not __debug__:raise RuntimeError('Optimized Python refused')
 ap=argparse.ArgumentParser();ap.add_argument('--arch',choices=BUILDS,required=True)
 for k in ('runner-sha','guard','guard-sha','build-controller','build-controller-sha','build-root','terminal-sha','binary-sha','parent-run','parent-plan','parent-plan-sha','source','source-sha','generator','generator-sha','output'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();arch=a.arch;th,bh,uid,gid=BUILDS[arch]
 need(all(re.fullmatch('[0-9a-f]{64}',v) for k,v in vars(a).items() if k.endswith('_sha')),'external SHA format');need((a.terminal_sha,a.binary_sha,a.guard_sha,a.build_controller_sha,a.source_sha,a.generator_sha)==(th,bh,GUARD,BUILDER,SOURCE,GENERATOR),'actual binary/build and frozen public controls')
 need((os.geteuid(),os.getegid())==(uid,gid) and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'public owner/environment')
 gp=canonical(a.guard);gb=gp.read_bytes();need(sha(gb)==GUARD,'guard same-buffer SHA');m=types.ModuleType('utc_public_guards');m.__file__=str(gp);exec(compile(gb,str(gp),'exec'),m.__dict__)
 protected={};diagnostics=[]
 def pin(path,h=None,n=None,old=None):
  path=canonical(path);z=m.observed(path,arch);need((h is None or z['sha256']==h) and (n is None or z['st_size']==n),'public byte pin')
  if old:need(z['sha256']==old['sha256'] and all(z[k]==old[k] for k in m.material(arch)),'actual historical material')
  protected[str(path)]=z;return z
 def read(path,h=None):
  b,z=m.observed(canonical(path),arch,16*1024*1024,True);need(h is None or sha(b)==h,'same held-byte SHA');pin(path,z['sha256'],z['st_size']);return b
 def same(path,z):
  x=m.observed(path,arch);need(x['sha256']==z['sha256'] and all(x[k]==z[k] for k in m.material(arch)),'protected readback')
  if x['st_ctime_ns']!=z['st_ctime_ns']:diagnostics.append({'path':str(path),'old':z['st_ctime_ns'],'new':x['st_ctime_ns'],'materialAndBytesEqual':True,'causeMeasured':False})
 selfp=canonical(__file__);read(selfp,a.runner_sha);read(gp,GUARD);read(a.source,SOURCE);gen=canonical(a.generator);read(gen,GENERATOR);builder_path=canonical(a.build_controller);bb=read(builder_path,BUILDER);bmod=types.ModuleType('utc_public_build_bindings');bmod.__file__=str(builder_path);exec(compile(bb,str(builder_path),'exec'),bmod.__dict__)
 dr=canonical(a.build_root);pr=canonical(a.parent_run);pb=pr/'build';ps=pr/'source';db=dr/'build';root=canonical(a.output);plan=canonical(a.parent_plan)
 need(dr.name=='current-utc-projection-artifacts-'+arch+'-20261014' and pr.name=='run-01' and pr.parent.name=='bootstrap-replica-composite-v2-'+arch+'-20261014','qualified public namespaces');need(not root.exists() and root.parent.is_dir() and not any(overlap(root,x) for x in (dr,pr,selfp.parent,gp.parent,builder_path.parent,gen.parent,canonical(a.source).parent,plan.parent)),'fresh separate output')
 t=json.loads(read(dr/'receipts/terminal-build.json',th));pth,pmh,ah,pu,pg=bmod.PARENTS[arch];p=json.loads(read(plan,a.parent_plan_sha));pt=json.loads(read(pr/'receipts/terminal-build.json',pth));md=json.loads(read(pr/'root-build-public-bindings.json',pmh))
 need(t['format']=='nano-current-utc-projection-one-target-build-terminal-v1' and t['status']=='BUILD_PASS_DIAGNOSTIC_NOT_EXECUTED' and t['arch']==arch and t['controllerSha256']==BUILDER and t['guardSha256']==GUARD and t['sourceSha256']==SOURCE and (t['parentTerminalSha256'],t['parentMetadataSha256'],t['parentPlanSha256'],t['archiveSha256'])==(pth,pmh,a.parent_plan_sha,ah) and t['archiveMembers']==142 and t['sourceViewFiles']==1324 and t['coreObjectsCompiled']==t['archiveCommandsExecuted']==t['cmakeCommandsExecuted']==0 and t['consumerCppCompiled']==t['linksExecuted']==1 and t['allActualMdEqualPreflight'] and t['sourceHeadersGeneratedSdkLibrariesAndParentsUnchanged'],'actual new build terminal')
 need(p['arch']==arch and p['image']==m.IMAGES[arch] and (p['uid'],p['gid'])==(uid,gid) and pt['parentPlanSha256']==a.parent_plan_sha and pt['archive']['sha256']==ah,'parent image/owner/plan/archive');need(md['terminalSha256']==pth and md['parentPlanSha256']==a.parent_plan_sha and len(md['files'])==32 and len({r['path'] for r in md['files']})==32,'original actual32 bindings')
 bound={}
 for r in md['files']:n=m.relative(r['path']);bound[n]=read(pr/n,r['sha256']);need(len(bound[n])==r['bytes'],'metadata byte length')
 source_map=json.loads(bound['receipts/source-before.json']);need(len(source_map)==1324,'full public source view')
 for n,z in source_map.items():pin(ps/m.relative(n),z['sha256'],z['st_size'],z)
 for n,z in json.loads(bound['receipts/generated-metadata-before-compilation.json']).items():pin(pb/m.relative(n),z['sha256'],z['st_size'],z)
 for table in ('libraries','generated'):
  for n,z in p[table].items():pin(pb/m.relative(n),z['sha256'],z['bytes'])
 raw=json.loads(bound['receipts/archive-142-actual-byte-proof.json']);need(raw['archiveBeforeProof']['sha256']==ah and len(raw['members'])==142 and len({r['object'] for r in raw['members']})==len({r['member'] for r in raw['members']})==142,'historical actual142 raw proof');pin(pb/'libnano_core.a',ah,pt['archive']['st_size'],pt['archive'])
 for r in raw['members']:need(pathlib.PurePosixPath(r['object']).name==r['member'],'member object identity');pin(pb/m.relative(r['object']),r['sha256'],r['bytes'])
 for table in ('programs','newDeadlineProgramsPreserved','objects','objectsReused'):
  for n,z in pt[table].items():pin(pb/m.relative(n),z['sha256'],z['st_size'],z)
 pin(pb/'libnano-bootstrap-maintenance-retention-fault.so',pt['shim']['sha256'],pt['shim']['st_size'],pt['shim']);pin(db/'project-current-utc',bh,t['binary']['st_size'],t['binary']);pin(db/'project-current-utc.o',t['object']['sha256'],t['object']['st_size'],t['object'])
 dependency=json.loads(read(dr/'receipts/dependency-binding.json'));before_md=read(db/'preflight.d',dependency['preflight']['sha256']);actual_md=read(db/'project-current-utc.o.d');need(bmod.dependencies(before_md)==bmod.dependencies(actual_md)==dependency['dependencies'] and dependency['sourceSha256']==SOURCE,'genuine probe/actual MD/source binding')
 for dep in dependency['dependencies']:
  if dep.startswith('/work/'):need(m.relative(dep[6:]) in source_map,'known consumed project header')
  elif dep.startswith('/outputs/'):need(m.relative(dep[9:]) in p['generated'],'known consumed generated header')
  elif dep.startswith('/diagnostic/'):need(dep=='/diagnostic/project-current-utc.cpp','sole diagnostic TU')
 for n,z in json.loads(read(dr/'receipts/generated-inputs-before-compilation.json')).items():pin(pb/m.relative(n),z['sha256'],z['st_size'],z)
 for n in ('derived-commands.json','compile-one-diagnostic.command.json','compile-one-diagnostic.exit.json','dependency-preflight.command.json','dependency-preflight.exit.json','link-one-diagnostic.command.json','link-one-diagnostic.exit.json','image-inspect.log','sdk-before.log','sdk-after.log','sdk-binding.json'):read(dr/'receipts'/n)
 old_sdk=read(dr/'receipts/sdk-before.log');sdk_info=json.loads(read(dr/'receipts/sdk-binding.json'));need(old_sdk==read(dr/'receipts/sdk-after.log') and sha(old_sdk)==sdk_info['beforeSha256']==sdk_info['afterSha256'] and sdk_info['equal'],'actual new-build SDK equal');sdk_rows=[s.split('  ',1) for s in old_sdk.decode().splitlines()];sdk={n:h for h,n in sdk_rows};need(len(sdk)==len(sdk_rows)==sdk_info['paths'],'unique actual SDK rows')
 fixture=ps/'tests/fixtures/capella-bootstrap-anchor-cb532';fixture_pins={'initial.json':('33aee17d626a3e541c7934c5576c025f8819d442555df6fadc90dde49bd045a7',62713),'profile.json':('e3440eba0a871a8eedf84a6e2cdccee53cc4389e8ce6b979166621c8e893890d',1331),'genesis.json':('3373c76de09192ad440625613db97c17b50fbfeddb5adb3190c8f0eb83a22005',15996),'manifest.json':('ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6',23260)}
 for n,(h,size) in fixture_pins.items():pin(fixture/n,h,size)
 initial=json.loads(read(fixture/'initial.json'));head_slot=int(initial['blocks'][0][1][0],16)
 os.mkdir(root,0o700);receipts=root/'receipts';controls=root/'controls'
 for d in (receipts,controls):d.mkdir(mode=0o700)
 m.save(receipts/'initial-empty-output.json',{'rootWasAbsentBeforeMkdir':True,'payloadAbsent':True,'oldInputsCopiedOrMutated':False});m.save(receipts/'inputs-before.json',protected)
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};started=time.monotonic();workers={};case_rows=[];dispatches=0;tool_rows=0;failure=None;payload_pins={};fifo=None
 def command(label,args,seconds):
  remaining=150-(time.monotonic()-started);need(remaining>0,'finite150s total');m.save(receipts/(label+'.command.json'),{'argv':args,'timeoutSeconds':seconds});at=time.monotonic()
  try:
   with (receipts/(label+'.log')).open('xb',buffering=0) as log:z=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,remaining))
   result={'exitCode':z.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-at}
  except subprocess.TimeoutExpired:result={'exitCode':None,'timedOut':True,'elapsedSeconds':time.monotonic()-at}
  m.save(receipts/(label+'.exit.json'),result);return result,m.observed(receipts/(label+'.log'),arch,1024*1024,True)[0]
 def container(label,args,seconds):
  token=uuid.uuid4().hex;name='nano-utc-public-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name]
  argv=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.utc.public.owner='+token,'--network','none','--read-only','--user',str(uid)+':'+str(gid),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','256m','--pids-limit','128','--tmpfs','/tmp:rw,noexec,nosuid,size=64m','--mount',f'type=bind,src={db},dst=/diagnostic-build,readonly','--mount',f'type=bind,src={root/"public-cases"},dst=/public-cases,readonly','--mount',f'type=bind,src={controls},dst=/controls,readonly','--entrypoint',args[0],p['image']]+args[1:]
  try:return command(label,argv,seconds)
  finally:
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10)
   if z.returncode==0:need(json.loads(z.stdout).get('nano.utc.public.owner')==token,'owned cleanup');need(subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10).returncode==0,'owned removal')
   else:need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'cleanup observation')
   z=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=10);need(z.returncode==1 and (b'No such object' in z.stdout or b'No such container' in z.stdout),'container remains')
 def recheck():
  for path,z in protected.items():same(path,z)
  for path,z in workers.items():same(path,z)
  for path,z in payload_pins.items():same(path,z)
  if fifo:
   z=pathlib.Path(fifo[0]).lstat();need(stat.S_ISFIFO(z.st_mode) and all(getattr(z,k)==fifo[1][k] for k in m.material(arch)),'public FIFO unchanged')
 try:
  generated,out=command('generate-public-cases',[sys.executable,'-B',str(gen),'--fixture',str(fixture),'--output',str(root/'public-cases'),'--runtime-root','/public-cases'],15);need(generated['exitCode']==0 and not generated['timedOut'],'public generator success');g=json.loads(out);payload=root/'public-cases';ib=(payload/'cases.json').read_bytes();need(sha(ib)==g['indexSha256'],'generated index SHA');index=json.loads(ib)
  need(index['format']=='nano-current-utc-public-synthetic-cases-v1' and index['toolSourceSha256']==SOURCE and index['runtimeRoot']=='/public-cases' and len(index['cases'])==8 and index['tokensAreSynthetic'] and not index['currentVpsPinsRead'] and not index['currentSnapshotAuthorityQualified'] and index['actualToolExecutions']==0 and not index['genesisOrPublishedClockChanged'] and index['newSignatures']==0 and not index['resumeAuthorized'],'public synthetic index')
  expected_names=[*EXPECTED,'real-utc-observation'];need([r['name'] for r in index['cases']]==expected_names,'exact8 new cases order')
  for r in index['cases']:
   n=m.relative(r['requestRelativePath']);need(r['argv']==['--request','/public-cases/'+n,r['suppliedRequestSha256']] and r['tokensAreSynthetic'] and r['expectedResumeAuthorized']==False and r['actualToolExecution']==False,'exact request CLI')
   if r['name'] in EXPECTED:
    reason,stage=EXPECTED[r['name']];need(r['expectedResult']=='REFUSED' and r['expectedReasons']==[reason] and r.get('expectedStage')==stage,'exact negative reason/stage')
   else:need(r['expectedResult']=='OBSERVE_ACTUAL','actual UTC is observation only')
  for n,z in index['files'].items():n=m.relative(n);path=payload/n;need(path.lstat().st_uid==uid and path.lstat().st_nlink==1 and stat.S_ISREG(path.lstat().st_mode),'public generated regular leaf');payload_pins[str(path)]=m.observed(path,arch);need(payload_pins[str(path)]['sha256']==z['sha256'] and payload_pins[str(path)]['st_size']==z['bytes'],'generated public bytes')
  payload_pins[str(payload/'cases.json')]=m.observed(payload/'cases.json',arch);s=(payload/'public-fifo').lstat();need(stat.S_ISFIFO(s.st_mode) and s.st_uid==uid and s.st_nlink==1,'one unwritten public FIFO');fifo=(str(payload/'public-fifo'),{k:getattr(s,k) for k in m.FIELDS})
  actual_names=set()
  for f in payload.rglob('*'):
   s=f.lstat();need(not stat.S_ISLNK(s.st_mode) and s.st_uid==uid,'public payload path/owner')
   if stat.S_ISDIR(s.st_mode):need(stat.S_IMODE(s.st_mode)==0o700,'public payload private directory')
   else:actual_names.add(str(f.relative_to(payload)))
  need(actual_names==set(index['files'])|{'cases.json','public-fifo'},'no extra public payload leaf')
  image,log=command('image-inspect',p['dockerArgv']+['image','inspect','--format','{{json .}}',p['image']],10);need(image['exitCode']==0,'image inspect success');ij=json.loads(log);need(ij['Id']==p['image'] and ij['Architecture']==arch and ij['Os']=='linux' and not any(v.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for v in ij.get('Config',{}).get('Env',[])),'existing pinned image without injected environment')
  worker=controls/'sdk.sh';body='#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(n)+'\n' for n in sorted(sdk));fd=os.open(worker,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
  with os.fdopen(fd,'wb') as f:f.write(body.encode());f.flush();os.fsync(f.fileno())
  workers[str(worker)]=m.observed(worker,arch);exit_sdk,actual=container('sdk-before',['/bin/sh','/controls/sdk.sh'],10);need(exit_sdk['exitCode']==0 and actual==old_sdk,'actual SDK exactly same as build');recheck()
  for row in index['cases']:
   name=row['name'];recheck();dispatches+=1;code,rawlog=container('case-'+name,['/diagnostic-build/project-current-utc',*row['argv']],80 if name=='real-utc-observation' else 5);data=None;error=None
   try:
    lines=[v for v in rawlog.decode().splitlines() if v.strip()];need(len(lines)==1,'one tool JSON row');data=json.loads(lines[0]);need(data['event']=='current-utc-projection-terminal','actual diagnostic result row');tool_rows+=1;need(not code['timedOut'] and all(data.get(k)==v for k,v in SAFETY.items()),'tool safety flags and no timeout')
    if name in EXPECTED:
     reason,stage=EXPECTED[name];need(code['exitCode']==1 and data['result']=='REFUSED' and data['reason']==reason and (stage is None or data['stage']==stage),'exact refusal reason before case qualification')
     if name=='chunk-count-refusal':need(data['chunks']==1 and data['lastFullyPreparedSlot']>head_slot,'genuine bounded progress before chunk refusal')
     if name=='steady-deadline-refusal':need(data['elapsedMs']>=1,'real steady deadline observed')
    else:
     need((code['exitCode'],data['result']) in [(0,'PROJECTED_READONLY_ONLY'),(1,'REFUSED')],'record actual UTC result without forced success')
     if data['result']=='PROJECTED_READONLY_ONLY':need(data['sourceInputsReadbackUnchanged'] and not data['extractionIndependentlyVerifiedByThisTool'] and not data['ordinaryClockRulesChanged'] and not data['startupAllowanceActivated'],'read-only projection scope')
     else:need(isinstance(data.get('reason'),str) and data['reason'],'actual refusal recorded')
   except BaseException as e:error=type(e).__name__+': '+str(e)
   case_rows.append({'name':name,'dispatchAttempted':True,'exit':code,'toolResult':data,'expectationMatched':error is None,'failure':error,'observationOnly':name=='real-utc-observation','negativeCasePass':name in EXPECTED and error is None});m.save(receipts/(name+'.result.json'),case_rows[-1]);recheck()
  exit_sdk,after=container('sdk-after',['/bin/sh','/controls/sdk.sh'],10);need(exit_sdk['exitCode']==0 and after==old_sdk,'actual SDK unchanged after cases');recheck();m.save(receipts/'inputs-after.json',protected);m.save(receipts/'ctime-diagnostics.json',diagnostics);need(dispatches==tool_rows==8 and len(case_rows)==8 and all(r['expectationMatched'] for r in case_rows),'eight observed tool exits and exact cases')
  result={'format':'nano-current-utc-eight-public-cases-terminal-v1','status':'PUBLIC_CASES_COMPLETE_OBSERVATION_RECORDED','arch':arch,'runnerSha256':a.runner_sha,'buildTerminalSha256':th,'binarySha256':bh,'sourceSha256':SOURCE,'generatorSha256':GENERATOR,'caseDispatchAttempts':dispatches,'dockerExitRowsObserved':sum(r['exit']['exitCode'] is not None for r in case_rows),'toolRowsObserved':tool_rows,'exactNegativeCasesPassed':7,'actualUtcObservation':case_rows[-1]['toolResult'],'actualUtcForcedPass':False,'allPublicInputsAndSdkUnchanged':True,'elapsedSeconds':time.monotonic()-started,'syntheticTokensOnly':True,'privateInputsRead':False,'newSignatures':0,'runtimeStarted':False,'resumeAuthorized':False,'productionQualified':False}
 except BaseException as e:
  failure=type(e).__name__+': '+str(e);result={'format':'nano-current-utc-eight-public-cases-terminal-v1','status':'PUBLIC_CASES_FAILED_NO_AUTOMATIC_RETRY','arch':arch,'failure':failure,'caseDispatchAttempts':dispatches,'dockerExitRowsObserved':sum(r['exit']['exitCode'] is not None for r in case_rows),'toolRowsObserved':tool_rows,'observedResults':len(case_rows),'exactNegativeCasesPassed':sum(r['negativeCasePass'] for r in case_rows),'actualUtcForcedPass':False,'privateInputsRead':False,'newSignatures':0,'runtimeStarted':False,'resumeAuthorized':False,'productionQualified':False}
  try:recheck();result['protectedInputsUnchangedAfterFailure']=True;m.save(receipts/'inputs-after-failure.json',protected)
  except BaseException:result['protectedInputsUnchangedAfterFailure']=False
 m.save(receipts/'results.json',case_rows);m.save(receipts/'terminal-public-cases.json',result);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
