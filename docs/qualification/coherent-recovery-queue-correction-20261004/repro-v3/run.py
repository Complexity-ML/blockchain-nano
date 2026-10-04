#!/usr/bin/env python3
"""Independent delivery rebuild after current qualification/recovery, never image rebuild."""
import os,json,pathlib,subprocess,shlex,time,sys
REV='efbeb9bca3d42e41edb9cb81afba98c0b953a90f'
TARGET='ubuntu@145.239.199.166'
ROOT=pathlib.Path(__file__).resolve().parent
ENV={**os.environ,'PATH':'/tmp/nano-wan-dual-family-transport-control-20261004/bin:'+os.environ['PATH']}
IMAGES={'arm64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','amd64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'}
VOLUMES={'arm64':'nano-clean-arm64-build-20261002','amd64':'nano-clean-rpc-build-20261002'}
SOURCES={'arm64':'/tmp/nano-production-coherent-20261004','amd64':'/home/ubuntu/nano-production-coherent-20261004'}
REPORTS={'arm64':'/tmp/nano-coherent-full-arm64-reports-20261004','amd64':'/home/ubuntu/nano-coherent-full-amd64-reports-20261004'}
def save(n,v):(ROOT/n).write_text(json.dumps(v,sort_keys=True,indent=2)+'\n')
def call(args,arch='arm64',timeout=45,log=None):
 if arch=='amd64':args=['ssh','-o','ConnectTimeout=10',TARGET,shlex.join(args)]
 if log is not None:
  with log.open('w') as stream:
   r=subprocess.run(args,env=ENV,text=True,stdout=stream,stderr=subprocess.STDOUT,timeout=timeout)
  if r.returncode:raise RuntimeError((args,r.returncode,'See '+str(log)))
  return log.read_text().strip()
 r=subprocess.run(args,env=ENV,text=True,capture_output=True,timeout=timeout)
 if r.returncode:raise RuntimeError((args,r.returncode,r.stderr[-1500:]))
 return r.stdout.strip()
def docker(args,arch,timeout=45,log=None):return call((['docker'] if arch=='arm64' else ['sudo','-n','docker'])+args,arch,timeout,log)
def source_snapshot(arch):
 code="import subprocess,hashlib,json,pathlib; p=pathlib.Path(__import__('sys').argv[1]); files=subprocess.check_output(['git','-C',str(p),'ls-files','-z']).split(b'\\0'); print(json.dumps({f.decode():hashlib.sha256((p/f.decode()).read_bytes()).hexdigest() for f in files if f},sort_keys=True))"
 return json.loads(call(['python3','-c',code,SOURCES[arch]],arch))
def original_hashes(arch):
 base='/build/coherent-efbeb9bc-'+arch
 output=docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build,readonly',IMAGES[arch],'sha256sum',base+'/nano',base+'/network/nano-p2p'],arch)
 return {row.split()[1]:row.split()[0] for row in output.splitlines()}

def wait():
 start=time.monotonic()
 while True:
  state=json.loads(pathlib.Path('/tmp/nano-coherent-restore-queue-v2-20261004/state.json').read_text())
  failure=pathlib.Path('/tmp/nano-coherent-restore-queue-v2-20261004/failure.json')
  if failure.exists():raise RuntimeError('Restore queue failed; do not rebuild')
  if state['phase']=='terminal':
   if state['exit']!=0:raise RuntimeError('Restore campaign failed; do not rebuild')
   break
  # Verify an actual controller handle, not a marker alone.
  rows=call(['ps','-axo','pid,args'])
  if not any('/tmp/nano-coherent-restore-queue-v2-20261004/run.py' in row for row in rows.splitlines()):raise RuntimeError('No live restore controller')
  save('state.json',{'phase':'waiting','restorePhase':state['phase'],'observedAt':time.time(),'rebuildStarted':False})
  if time.monotonic()-start>18000:raise RuntimeError('Wait limit, existing jobs untouched')
  time.sleep(20)
BUILD_SCRIPT=r'''set -euo pipefail
build=/build/repro-efbeb9bc-ARCH
export TMPDIR=$build/scratch
mkdir -p "$TMPDIR"
cmake -S /work/network -B "$build/network" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DNANO_DEPS_DIR=/src/.deps
cmake --build "$build/network" --parallel 2 --target nano-p2p
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DNANO_NETWORK_EXECUTABLE="$build/network/nano-p2p"
cmake --build "$build" --parallel 2 --target nano
sha256sum "$build/nano" "$build/network/nano-p2p"
'''
try:
 wait();results={}
 for arch in IMAGES:
  assert call(['git','-C',SOURCES[arch],'rev-parse','HEAD'],arch)==REV
  assert not call(['git','-C',SOURCES[arch],'status','--porcelain'],arch)
  assert not docker(['ps','-q'],arch),'Do not overlap any running Docker job'
  for name in ['ctest-exit.txt','audit-exit.txt','post-test-manifest-exit.txt']:assert call(['cat',REPORTS[arch]+'/'+name],arch)=='0'
  old=json.loads(call(['cat',REPORTS[arch]+'/executables-after-tests.json'],arch))
  expected_sources=json.loads(call(['cat',REPORTS[arch]+'/source-manifest-after-tests.json'],arch))
  before=source_snapshot(arch); assert before==expected_sources,'Actual source differs from qualification'
  originals=original_hashes(arch); assert all(old[p]==h for p,h in originals.items()),'Original delivery binary changed'
  save(arch+'-sources-before.json',before); save(arch+'-originals-before.json',originals)
  docker(['volume','inspect',VOLUMES[arch]],arch)
  assert docker(['image','inspect',IMAGES[arch],'--format','{{.Id}}'],arch)==IMAGES[arch]
  subdir='repro-efbeb9bc-'+arch
  uid=call(['id','-u'],arch);gid=call(['id','-g'],arch)
  docker(['run','--rm','--network','none','--read-only','--user','0:0','--cap-drop','ALL','--cap-add','CHOWN','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build',IMAGES[arch],'sh','-c','set -eu; mkdir "$1"; chown "$2:$3" "$1"','sh','/build/'+subdir,uid,gid],arch)
  command=['run','--rm','--name','nano-repro-efbeb9bc-'+arch,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user',uid+':'+gid,'--cpus','2','--memory','3g','--ulimit','nofile=65536:65536','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=128m','--mount','type=bind,src='+SOURCES[arch]+',dst=/work,readonly','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build','--workdir','/work','--env','NANO_DEPS_DIR=/src/.deps',IMAGES[arch],'bash','-c',BUILD_SCRIPT.replace('ARCH',arch)]
  save('state.json',{'phase':'rebuilding','arch':arch,'image':IMAGES[arch],'revision':REV,'imageBuild':False})
  try:
   output=docker(command,arch,timeout=7200,log=ROOT/(arch+'-build.log'))
  except BaseException:
   # Stop only this controller's named rebuild, never existing qualification jobs.
   if docker(['ps','-q','--filter','name=^/nano-repro-efbeb9bc-'+arch+'$'],arch):
    docker(['stop','-t','5','nano-repro-efbeb9bc-'+arch],arch,timeout=30)
   raise
  after=source_snapshot(arch); assert after==before,'Source changed during rebuild'
  originals_after=original_hashes(arch); assert originals_after==originals,'Original delivery binary changed during rebuild'
  save(arch+'-sources-after.json',after); save(arch+'-originals-after.json',originals_after)
  actual={}
  for row in output.splitlines():
   parts=row.split()
   if len(parts)==2 and len(parts[0])==64 and parts[1].startswith('/build/'+subdir+'/'):actual[parts[1]]=parts[0]
  assert len(actual)==2
  comparisons={name:{'original':old['/build/coherent-efbeb9bc-'+arch+'/'+name],'rebuilt':actual['/build/'+subdir+'/'+name]} for name in ['nano','network/nano-p2p']}
  results[arch]={'revision':REV,'image':IMAGES[arch],'comparisons':comparisons,'bitIdentical':all(v['original']==v['rebuilt'] for v in comparisons.values()),'scope':'delivery executables only; same pinned dependency image and cache; not dependency-source reproducibility'}
  save('result.json',{'architectures':results,'complete':False,'productionQualified':False})
  if not results[arch]['bitIdentical']:raise RuntimeError(arch+' delivery rebuild differs; preserve diagnostics')
 save('result.json',{'architectures':results,'complete':True,'productionQualified':False});save('state.json',{'phase':'terminal','exit':0})
except Exception as e:
 save('failure.json',{'error':str(e),'productionQualified':False});save('state.json',{'phase':'terminal','exit':1});sys.exit(1)
