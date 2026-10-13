#!/usr/bin/env python3
"""One changed test translation unit, old coherent build mounted read-only."""
import pathlib, hashlib, os, json, subprocess, stat, shlex, sys, time, re
import xml.etree.ElementTree as ET
REPO=pathlib.Path('/home/ubuntu/blockchain-nano-git')
OLD=pathlib.Path('/home/ubuntu/nanobees-qualification/duty-availability-coherent-amd64-20261013')
ROOT=pathlib.Path('/home/ubuntu/nanobees-qualification/duty-availability-portability-amd64-20261013')
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
HEAD='d7cc1bdb63f6d5035adc6f4b2420823f135f1bf1'
OLD_HEAD='c672a30dbdc7b43ab04186db0e54be78510e6789'
SOURCE='tests/test_capella_duty_availability.cpp'
NEW_SHA='44febdb90047ea8a7de5e78002d73ed96b82cc67259dd356e98c7ba096f6647d'
ENV={'PATH':'/usr/bin:/bin','LANG':'C.UTF-8'}
sha=lambda b:hashlib.sha256(b).hexdigest()
def read(p):
 assert p.is_file() and not p.is_symlink()
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);parts=[]
  while True:
   data=os.read(fd,1048576)
   if not data:break
   parts.append(data)
  z=os.fstat(fd)
 finally:os.close(fd)
 assert (a.st_dev,a.st_ino,a.st_size,a.st_mtime_ns,a.st_ctime_ns)==(z.st_dev,z.st_ino,z.st_size,z.st_mtime_ns,z.st_ctime_ns)
 return b''.join(parts)
def file_sha(p):return sha(read(p))
def host(a):return subprocess.check_output(a,env=ENV,text=True,timeout=30)
def write(name,j):
 p=ROOT/name;p.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
 data=(json.dumps(j,indent=2)+'\n').encode();fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  offset=0
  while offset<len(data):offset+=os.write(fd,data[offset:])
  os.fsync(fd)
 finally:os.close(fd)
def capture(paths):
 rows={}
 for name in sorted(paths):
  p=OLD/'build'/name;s=p.lstat();assert stat.S_ISREG(s.st_mode) and not p.is_symlink()
  rows[name]={'sha256':file_sha(p),'bytes':s.st_size,'device':s.st_dev,'inode':s.st_ino,'owner':s.st_uid,'mode':stat.S_IMODE(s.st_mode),'mtimeNs':s.st_mtime_ns,'ctimeNs':s.st_ctime_ns}
 return rows
def source_inventory():
 before=json.loads(read(OLD/'receipts/source-before.json'));now={}
 for name in before:now[name]=file_sha(REPO/name)
 assert now[SOURCE]==NEW_SHA
 assert all(now[n]==v for n,v in before.items() if n!=SOURCE)
 return now
def run(name,args,timeout):
 command=['/usr/bin/sudo','-n','/usr/bin/docker','run','--rm','--pull=never','--platform','linux/amd64','--name','nano-duty-port-'+name+'-amd64-20261013','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','1000:1000','--cpus','2','--memory','2g','--pids-limit','256','--env','TMPDIR=/port/tmp',
  '--mount',f'type=bind,source={REPO},target=/work,readonly','--mount',f'type=bind,source={OLD/"build"},target=/outputs,readonly','--mount',f'type=bind,source={ROOT},target=/port','--workdir','/outputs','--entrypoint',args[0],IMAGE]+args[1:]
 write(name+'.command.json',{'args':command,'timeoutSeconds':timeout,'oldCoherentBuildMountedReadOnly':True})
 print(json.dumps({'phase':name,'status':'started'}),flush=True);start=time.monotonic()
 log=ROOT/(name+'.log');fd=os.open(log,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as stream:
  try:code=subprocess.run(command,env=ENV,stdout=stream,stderr=subprocess.STDOUT,timeout=timeout).returncode
  except subprocess.TimeoutExpired:
   code=124
   subprocess.run(['/usr/bin/sudo','-n','/usr/bin/docker','kill','nano-duty-port-'+name+'-amd64-20261013'],env=ENV,capture_output=True,timeout=20)
 write(name+'.exit.json',{'exitCode':code,'elapsedSeconds':time.monotonic()-start,'logSha256':file_sha(log)})
 print(json.dumps({'phase':name,'exitCode':code}),flush=True)
 assert code==0,name

assert len(sys.argv)==2 and sys.argv[1].startswith('--expected-driver-sha=')
driver_sha=sys.argv[1].split('=',1)[1];assert file_sha(pathlib.Path(__file__))==driver_sha
assert os.getuid()==1000 and sys.flags.optimize==0
assert host(['/usr/bin/git','-C',str(REPO),'rev-parse','HEAD']).strip()==HEAD and host(['/usr/bin/git','-C',str(REPO),'status','--porcelain'])==''
assert host(['/usr/bin/git','-C',str(REPO),'diff',OLD_HEAD,HEAD,'--','src','include','cmake','third_party','CMakeLists.txt'])==''
assert host(['/usr/bin/git','-C',str(REPO),'diff','--numstat',OLD_HEAD,HEAD,'--',SOURCE]).strip()=='1\t1\t'+SOURCE
assert file_sha(REPO/SOURCE)==NEW_SHA and file_sha(REPO/'CMakeLists.txt')=='28fe657bbf1a54ad5663f64024d95f90eeb8a5e5fdd25943f0a684042e66f0ec'
assert json.loads(read(OLD/'receipts/terminal-receipt.json'))['passedWithinRequestedScope'] is True
snapshot=OLD/'original-generated-build-evidence'
commands=json.loads(read(snapshot/'compile_commands.json'))
row=next(x for x in commands if 'nano-capella-duty-availability-tests.dir/' in x['command'])
old_object=row['output'];assert old_object=='CMakeFiles/nano-capella-duty-availability-tests.dir/tests/test_capella_duty_availability.cpp.o'
compile_args=shlex.split(row['command']);assert compile_args.count('-o')==1 and compile_args[-2:]==['-c','/work/'+SOURCE]
compile_args[compile_args.index('-o')+1]='/port/test.o'
compile_args+=['-MD','-MF','/port/test.o.d']
link_args=shlex.split(read(snapshot/'CMakeFiles/nano-capella-duty-availability-tests.dir/link.txt').decode())
assert link_args.count(old_object)==1 and link_args.count('-o')==1
link_args[link_args.index(old_object)]='/port/test.o';link_args[link_args.index('-o')+1]='/port/nano-capella-duty-availability-tests'
program_names=set(json.loads(read(OLD/'receipts/programs-before-tests.json')))
objects={x['path'] for x in json.loads(read(OLD/'receipts/core-object-audit.json'))['objects']};assert len(objects)==139
protect=program_names|objects|{old_object}
for item in commands:
 output=item.get('output')
 if output and (OLD/'build'/output).is_file():protect.add(output)
for item in link_args:
 if not item.startswith(('-','/')) and (OLD/'build'/item).is_file():protect.add(item)
before=capture(protect);src_before=source_inventory()
assert not ROOT.exists() and not ROOT.is_symlink();os.mkdir(ROOT,0o700)
assert list(ROOT.iterdir())==[]
write('fresh-output-proof.json',{'entries':[],'createdAtomically':True,'driverSha256':driver_sha,'head':HEAD,'oldBuildAndOutputsRemainReadOnly':True})
(ROOT/'tmp').mkdir(mode=0o700)
write('protected-build-before.json',before);write('source-before.json',src_before)
write('scope.json',{'oldHead':OLD_HEAD,'newHead':HEAD,'onlyOneTestSourceLineChanged':True,'oldSourceSha256':'8690cc722e6642d14be1bc4b021eef9924be47e2b7e17d070a04908aa256ded2','newSourceSha256':NEW_SHA,'canonicalProductionDiffEmpty':True,'protectedCoreObjects':139,'archiveUsed':before['libnano_core.a']['sha256'],'oldTestObjectSha256':before[old_object]['sha256'],'oldTestProgramSha256':before['nano-capella-duty-availability-tests']['sha256'],'oldFlagsAndLinkCommandsRetained':True,'compilingOneChangedTranslationUnitOnly':True,'cmakeOrCoreBuildRequested':False,'noCampaignOrPrivateRecoveryData':True})
failure=None;reused=False;tested=False
try:
 run('compile',compile_args,300)
 run('link',link_args,90)
 new_object=file_sha(ROOT/'test.o');new_program=file_sha(ROOT/'nano-capella-duty-availability-tests')
 reused=new_object==before[old_object]['sha256'] and new_program==before['nano-capella-duty-availability-tests']['sha256']
 write('byte-comparison-before-execution-decision.json',{'oldObjectSha256':before[old_object]['sha256'],'newObjectSha256':new_object,'oldProgramSha256':before['nano-capella-duty-availability-tests']['sha256'],'newProgramSha256':new_program,'objectAndProgramByteIdentical':reused,'decision':'reuse original three-scenario executed proof, no rerun' if reused else 'execute only changed new aggregate once; no legacy regression repeated'})
 assert capture(protect)==before,'protected coherent build bytes/inodes/ctime/mtime drifted'
 if not reused:
  run('new-aggregate',['/port/nano-capella-duty-availability-tests','--self-test'],130)
  results=[json.loads(x) for x in read(ROOT/'new-aggregate.log').decode().splitlines() if x.startswith('{')]
  assert len(results)==3 and all(x['result']=='PASS' and x['maxRssKiB']<=131072 and x['elapsedMs']<60000 for x in results)
  write('new-aggregate-results.json',{'results':results,'signedCampaignDutiesQualified':False});tested=True
except Exception as error:failure=str(error)
after=capture(protect);src_after=source_inventory();write('protected-build-after.json',after);write('source-after.json',src_after)
same=after==before and src_after==src_before and host(['/usr/bin/git','-C',str(REPO),'rev-parse','HEAD']).strip()==HEAD and host(['/usr/bin/git','-C',str(REPO),'status','--porcelain'])==''
write('terminal-receipt.json',{'format':'nano-amd64-one-line-test-portability-qualification-v1','passedWithinScope':failure is None and same and (reused or tested),'failure':failure,'protected139CoreObjectsAndParentsStatShaUnchanged':after==before,'sourceInventoryUnchanged':src_before==src_after,'oldObjectAndTestBinaryPreserved':True,'testObjectAndProgramByteIdentical':reused,'originalThreeScenarioProofReusedWithoutRerun':reused,'newChangedAggregateExecutedOnce':tested,'oldRegressionsRepeated':False,'coherentCoreOrMainRebuilt':False,'privateVpsRecoveryProofExported':False,'actualCampaignRuntimeOrDutiesStarted':False,'productionQualified':False})
print(json.dumps({'terminal':failure or 'success','byteIdentical':reused,'newAggregateExecuted':tested,'protectedBuildUnchanged':after==before}),flush=True)
if failure or not same or not(reused or tested):raise SystemExit(failure or 'protected source/build changed')
