#!/usr/bin/env python3
"""Root-only V4 tiny repair: one compile/link/self-test; no core, archive, CMake or old tests."""
import argparse,hashlib,json,os,pathlib,posixpath,re,shlex,shutil,stat,subprocess,sys,time,uuid
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
FIELDS={'dev':'st_dev','ino':'st_ino','uid':'st_uid','gid':'st_gid','mode':'st_mode','bytes':'st_size','mtimeNs':'st_mtime_ns','ctimeNs':'st_ctime_ns'}
def req(ok,why):
 if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def path(p):
 p=pathlib.Path(p);req(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def read(p,maxbytes=16*1024*1024):
 p=path(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);req(stat.S_ISREG(a.st_mode) and a.st_size<=maxbytes,'bounded regular input');b=b''
  while True:
   c=os.read(fd,1024*1024)
   if not c:break
   b+=c;req(len(b)<=maxbytes,'read bound')
  z=os.fstat(fd);req(all(getattr(a,k)==getattr(z,k)==getattr(p.lstat(),k) for k in FIELDS.values()),'input changed');return b
 finally:os.close(fd)
def pin(p,h):
 req(re.fullmatch('[0-9a-f]{64}',h) is not None,'external SHA');b=read(p);req(sha(b)==h,'public pin mismatch');return b
def hs(p):
 p=path(p);a=p.lstat();req(stat.S_ISREG(a.st_mode),'regular public artifact');h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 z=p.lstat();req(all(getattr(a,k)==getattr(z,k) for k in FIELDS.values()),'artifact changed');return {'sha256':h.hexdigest(),**{d:getattr(z,k) for d,k in FIELDS.items()}}
def save(p,v):
 b=(json.dumps(v,indent=2)+'\n').encode();fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
def relative(r):req(isinstance(r,str) and r and pathlib.PurePosixPath(r).as_posix()==r and not r.startswith('/') and '..' not in pathlib.PurePosixPath(r).parts and '\n' not in r and '\0' not in r,'relative public path');return r
def dep(b):
 d=sorted(set(shlex.split(b.decode().replace('\\\n',' ').split(':',1)[1])));req(d and all(p.startswith(('/work/','/outputs/','/usr/','/lib/')) for p in d),'dependency namespace');return d

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for k in ('controller-sha','plan','plan-sha'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();selfp=path(__file__);pin(selfp,a.controller_sha);planp=path(a.plan);i=json.loads(pin(planp,a.plan_sha));req(i['format']=='nano-target-source-auth-test-v4-one-consumer-plan-v1' and i['status']=='READY_ROOT_DISPATCH_ONLY' and i['image']==IMAGE,'specific V4 plan')
 req(os.geteuid()==1000 and os.getegid()==1000 and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'uid1000 and public environment')
 q=pathlib.Path('/home/ubuntu/nanobees-qualification');parent=path(i['parentRoot']);root=path(i['outputRoot']);req(parent==q/'target-source-auth-amd64-20261013/build-pass-01' and root==q/'target-source-auth-amd64-test-v4-20261013/run-01' and not root.exists(),'exact new namespace')
 v4=json.loads(pin(i['v4Manifest']['path'],i['v4Manifest']['sha256']));req(i['v4Manifest']['sha256']=='276674201c356da8d9d15a308c544bc86dcedbec609731d975ee53afb5b45dd7' and i['v4Source']['sha256']=='17e32c0f79c784e16d8561fa7812d1c75eff438f95842d1c3fb0303bd3c77525','V4 literal pins');newcpp=pin(i['v4Source']['path'],i['v4Source']['sha256']);req(len(newcpp)==i['v4Source']['bytes'],'new CPP bytes')
 t=json.loads(pin(i['parentBuildTerminal']['path'],i['parentBuildTerminal']['sha256']));req(i['parentBuildTerminal']['sha256']=='7e5388bafffcca1e9cc8f4e5fc417ec3f4f47dd717617fa28e1c379107453709' and t['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and t['archive']['sha256']==i['coreArchiveSha256']=='78d2852a9e423daa2767d0432f90b8aebc45351b5f94f6e307e97b33b1f16aab','actual unchanged140 parent')
 req(i['target']=='nano-capella-target-source-authentication-tests' and i['source']=='tests/test_capella_target_source_authentication.cpp' and i['sourceCount']==1304,'single fixed consumer');req(next(r for r in v4['sourceFiles'] if r['path']=='CMakeLists.txt')['sha256']==i['parentSource']['CMakeLists.txt']['sha256'],'CMake unchanged')
 ps=parent/'source';pb=parent/'build';protected={str(ps/relative(p)):r for p,r in i['parentSource'].items()};protected.update({str(pb/relative(p)):r for p,r in i['parentArtifacts'].items()});req(len(i['parentSource'])==1304 and len(i['parentArtifacts'])==170,'actual public parent scope');req({p:hs(p) for p in protected}==protected,'parent source/artifacts differ from actual prior run')
 meta=pb/'CMakeFiles'/(i['target']+'.dir');flags=read(meta/'flags.make');linktext=read(meta/'link.txt');req(sha(flags)==i['parentMetadata']['flagsSha256'] and sha(linktext)==i['parentMetadata']['linkSha256'],'original consumer flags/link');protected.update({str(p):hs(p) for p in (meta/'flags.make',meta/'link.txt')})
 if not root.parent.exists():os.mkdir(root.parent,0o700)
 req(root.parent.lstat().st_uid==1000 and stat.S_IMODE(root.parent.lstat().st_mode)==0o700,'owned700 parent');os.mkdir(root,0o700);source=root/'source';build=root/'build';receipts=root/'receipts';controls=root/'controls'
 for p in (source,build,receipts,controls):p.mkdir(mode=0o700)
 (build/'tmp').mkdir(mode=0o700);save(receipts/'parents-before.json',protected);env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};began=time.monotonic();failure=None;summary=None;stats={};compile_done=link_done=run_done=0
 def copy(src,dst,expected):
  b=hs(src);req(b['sha256']==expected['sha256'] and b['bytes']==expected.get('bytes',expected.get('st_size')),'copy source pin');dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst);os.chmod(dst,stat.S_IMODE(b['mode']));req(hs(src)==b and hs(dst)['sha256']==b['sha256'],'copy stable bytes')
 def command(label,argv,seconds):
  req(time.monotonic()-began<1200,'finite1200s bound');save(receipts/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds});start=time.monotonic()
  with (receipts/(label+'.log')).open('wb',buffering=0) as out:
   try:r=subprocess.run(argv,stdout=out,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,1200-(time.monotonic()-began))))
   except subprocess.TimeoutExpired:save(receipts/(label+'.exit.json'),{'exitCode':None,'timedOut':True});raise
  save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-start});req(r.returncode==0,'actual phase failed: '+label);print(json.dumps({'phase':label}),flush=True);return read(receipts/(label+'.log'))
 def container(label,argv,seconds=60,test=False):
  token=uuid.uuid4().hex;name='nano-auth-v4-'+token[:16];dk=['/usr/bin/sudo','-n','/usr/bin/docker'];cmd=dk+['run','--rm','--pull=never','--platform','linux/amd64','--name',name,'--label','nano.auth.v4.owner='+token,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','1024m' if test else '4g','--pids-limit','256','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=512m,uid=1000,gid=1000,mode=700','--env','TMPDIR=/tmp' if test else 'TMPDIR=/outputs/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs'+(',readonly' if test else ''),'--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',argv[0],IMAGE]+argv[1:]
  try:return command(label,cmd,seconds)
  finally:
   check=dk+['container','inspect','--format','{{json .Config.Labels}}',name];r=subprocess.run(check,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20)
   if r.returncode==0:req(json.loads(r.stdout).get('nano.auth.v4.owner')==token,'owned-only cleanup');rr=subprocess.run(dk+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20);req(rr.returncode==0,'cleanup remove')
   else:req(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'ambiguous cleanup')
   r=subprocess.run(check,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20);req(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'owned container remains');save(receipts/(label+'-cleanup.json'),{'ownedUuidContainerAbsent':True})
 try:
  image=json.loads(command('image-inspect',['/usr/bin/sudo','-n','/usr/bin/docker','image','inspect','--format','{{json .}}',IMAGE],25));req(image['Id']==IMAGE and image['Architecture']=='amd64' and image['Os']=='linux' and not any(s.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for s in image.get('Config',{}).get('Env',[])),'same immutable image')
  for rel,r in i['parentSource'].items():
   if rel==i['source']:dst=source/rel;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(newcpp);os.chmod(dst,0o644)
   else:copy(ps/rel,source/rel,r)
  source_before={p:hs(source/p) for p in i['parentSource']};req(sum(p.startswith('tests/fixtures/') for p in source_before)==23 and source_before[i['source']]['sha256']==i['v4Source']['sha256'],'source1304/V4/fixtures23 exact');save(receipts/'source-before.json',source_before)
  copy(pb/'libnano_core.a',build/'libnano_core.a',i['parentArtifacts']['libnano_core.a'])
  for rel,r in i['libraries'].items():copy(pb/rel,build/rel,r)
  for rel,r in i['generated'].items():copy(pb/rel,build/rel,r)
  prefix=build/'CMakeFiles'/(i['target']+'.dir');prefix.mkdir(parents=True);(prefix/'flags.make').write_bytes(flags);(prefix/'link.txt').write_bytes(linktext);copies={p.relative_to(build).as_posix():hs(p) for p in build.rglob('*') if p.is_file()}
  cc=json.loads(read(pb/'compile_commands.json'));rows=[r for r in cc if r['file']=='/work/'+i['source']];req(len(rows)==1 and rows[0]['directory']=='/outputs','actual unique tiny compiler');argv=shlex.split(rows[0]['command']);obj=i['object'];req(argv[0]=='/usr/bin/clang++' and argv.count('-o')==argv.count('-c')==1 and argv[argv.index('-o')+1]==obj,'exact original object command');(build/obj).parent.mkdir(parents=True,exist_ok=True)
  probe=argv.copy();probe[probe.index('-c')]='-M';probe[probe.index('-o')+1]='preflight.d';probe+=['-MT',obj];container('preflight',probe,60);deps=dep(read(build/'preflight.d'));sdk={x for x in deps if x.startswith(('/usr/','/lib/'))};link=shlex.split(linktext.decode());req(link[0]=='/usr/bin/clang++' and link.count('libnano_core.a')==1 and link.count('-o')==1 and link[link.index('-o')+1]==i['target'] and [v for v in link if v.endswith('.o')]==[obj,'third_party/mcl/bint64.o','third_party/mcl/base64.o'],'same exact link operands');sdk.update(v for v in link if v.startswith(('/usr/','/lib/')));sdk.add('/usr/bin/clang++')
  for d in deps:
   if d.startswith('/work/'):r=relative(posixpath.normpath(d[6:]));req(r in source_before and hs(source/r)==source_before[r],'new project closure pin')
   elif d.startswith('/outputs/'):r=relative(d[9:]);req(r in copies and hs(build/r)==copies[r],'generated closure pin')
  worker=controls/'sdk.sh';worker.write_text('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(v)+'\n' for v in sorted(sdk)));worker_before=hs(worker);sdk_before=container('sdk-before',['/bin/sh','/controls/sdk.sh'],120);save(receipts/'copies-before.json',copies)
  container('compile',argv+['-MD','-MF',obj+'.d','-MT',obj],600);compile_done=1;req(dep(read(build/(obj+'.d')))==deps,'actual MD differs from preflight');container('link',link,120);link_done=1;stats={'object':hs(build/obj),'binary':hs(build/i['target']),'archive':hs(build/'libnano_core.a')};req(hs(build/i['target'])==stats['binary'],'new binary second full observation')
  sdk_after=container('sdk-after',['/bin/sh','/controls/sdk.sh'],120);req(sdk_before==sdk_after and hs(worker)==worker_before and {p:hs(build/p) for p in copies}==copies,'SDK/dependency/gen/metadata bytes changed');save(receipts/'sdk-binding.json',{'beforeSha256':sha(sdk_before),'afterSha256':sha(sdk_after),'equal':True,'consumedSdkPaths':len(sdk),'noHistoricalSdkInvented':True})
  output=container('self-test',['/outputs/'+i['target'],'--self-test'],70,True);run_done=1;jsonrows=[json.loads(s) for s in output.decode().splitlines() if s.startswith('{')];categories=[r for r in jsonrows if r.get('scenario','').startswith('target-source-')];summaries=[r for r in jsonrows if r.get('scenario')=='capella-target-source-authentication-summary'];req(len(categories)==12 and len({r['scenario'] for r in categories})==12 and all(r.get('result')=='PASS' and r.get('productionQualified') is False for r in categories) and len(summaries)==1,'actual12 V4 categories');summary=summaries[0];req(summary['categories']==12 and summary['result']=='PASS' and 0<=summary['elapsedMs']<60000 and 0<=summary['cpuMicroseconds']<60000000 and 0<=summary['maxRssKiB']<=131072 and 0<summary['syntheticSignatures']<=8 and all(summary[k] is False for k in ['runtimeStarted','networkQualified','productionQualified','naturalChainDeadlineProven','coldRecordFixtureQualified','performanceImprovementRequired']),'actual V4 bounded scope')
  for k in ['freshFullAuthentication','repeatedSourceMatches','preparedContextStillFullyProjected']:req(summary[k]['iterations']==8 and 0<=summary[k]['wallNanoseconds']<60000000000 and 0<=summary[k]['cpuMicroseconds']<60000000,'bounded three-operation benchmark')
  req({p:hs(source/p) for p in source_before}==source_before and hs(build/obj)==stats['object'] and hs(build/i['target'])==stats['binary'] and {p:hs(build/p) for p in copies}==copies,'actual output inputs changed')
 except BaseException as error:failure=type(error).__name__+': '+str(error)
 finally:
  try:req({p:hs(p) for p in protected}==protected,'public parents changed');pin(selfp,a.controller_sha);pin(planp,a.plan_sha);pin(i['v4Manifest']['path'],i['v4Manifest']['sha256']);pin(i['v4Source']['path'],i['v4Source']['sha256']);save(receipts/'parents-after.json',protected)
  except BaseException as error:failure=(failure or '')+'; final guard '+str(error)
  save(receipts/'terminal.json',{'format':'nano-target-source-auth-test-v4-one-consumer-terminal-v1','passedWithinOneTinyTestScope':failure is None,'controllerSha256':a.controller_sha,'planSha256':a.plan_sha,'parentTerminalSha256':i['parentBuildTerminal']['sha256'],'testManifestSha256':i['v4Manifest']['sha256'],'sourceCppSha256':i['v4Source']['sha256'],'archiveSha256':i['coreArchiveSha256'],'coreCompiled':0,'archiveCommands':0,'compileCommandsCompleted':compile_done,'linkCommandsCompleted':link_done,'selfTestRunsCompleted':run_done,'oldEightRetested':False,'naturalDeadlineQualified':False,'stats':stats,'actualSummary':summary,'failure':failure,'privateInputsRead':False,'runtimeDutiesResumed':False,'productionQualified':False})
 return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
