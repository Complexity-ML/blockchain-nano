#!/usr/bin/env python3
"""Wait for the actual ARM64 dependency build, then qualify the same candidate."""
import hashlib,json,os,pathlib,shutil,subprocess,time
SOURCE=pathlib.Path('/tmp/nano-rpc-headroom-margin-candidate-20261002')
REV='d1597d9a6987d0839ed2f0c7ded268aead35d071'
DEP=pathlib.Path('/tmp/nano-rpc-clean-arm64-build-20261002')
DEP_PID=83863
TAG='nano-build-dependencies:clean-arm64-20261002'
VOLUME='nano-clean-arm64-build-20261002'
SUBDIR='rpc-full-d1597d9a-arm64'
REPORTS='/tmp/nano-rpc-full-arm64-reports-20261002'
CONTROL=pathlib.Path('/tmp/nano-rpc-full-arm64-control-20261002')
D=['docker']
def out(args):return subprocess.check_output(args,text=True).strip()
def check():
 assert out(['git','-C',str(SOURCE),'rev-parse','HEAD'])==REV
 assert not out(['git','-C',str(SOURCE),'status','--porcelain'])
def manifest():
 files=subprocess.check_output(['git','-C',str(SOURCE),'ls-files','-z']).split(b'\0')
 return {os.fsdecode(f):hashlib.sha256((SOURCE/os.fsdecode(f)).read_bytes()).hexdigest() for f in files if f}
check();CONTROL.mkdir(exist_ok=False)
deadline=time.monotonic()+10800
print('Waiting on actual dependency build PID',DEP_PID,flush=True)
while True:
 result=subprocess.run(['ps','-p',str(DEP_PID),'-o','stat=,command='],capture_output=True,text=True)
 if result.returncode!=0:break
 status,command=result.stdout.strip().split(None,1)
 if status.startswith('Z'):break
 assert 'docker build' in command and TAG in command and '--target dependencies' in command, 'dependency PID reused'
 assert time.monotonic()<deadline, 'dependency wait exceeded'
 time.sleep(10)
check();before=manifest()
assert before==json.loads((DEP/'sources-before.json').read_text()), 'sources changed during dependency build'
(CONTROL/'sources-after-dependencies.json').write_text(json.dumps(before,sort_keys=True))
assert out(D+['image','inspect',TAG,'--format','{{.Architecture}}'])=='arm64'
image=out(D+['image','inspect',TAG,'--format','{{.Id}}'])
assert image.startswith('sha256:') and len(image)==71
(CONTROL/'builder-image.txt').write_text(image+'\n')
assert VOLUME not in out(D+['volume','ls','-q']).splitlines()
assert not out(D+['ps','-q'])
assert not pathlib.Path(REPORTS).exists()
assert shutil.disk_usage('/tmp').free>8*1024**3
subprocess.run(D+['volume','create',VOLUME],check=True)
env=os.environ.copy();env.update(NANO_BUILDER_IMAGE=image,NANO_BUILD_VOLUME=VOLUME,NANO_BUILD_SUBDIR=SUBDIR)
env.pop('NANO_RPC_TIMING_DIR',None)
command=['bash','scripts/ci/full-docker.sh',REPORTS]
(CONTROL/'command.json').write_text(json.dumps({'command':command,'source':str(SOURCE),'revision':REV,'image':image,'volume':VOLUME,'subdirectory':SUBDIR},indent=2)+'\n')
print('Dependency image frozen; starting fresh coherent ARM64 full regression.',flush=True)
with (CONTROL/'controller.log').open('wb') as log:
 try:code=subprocess.run(command,cwd=SOURCE,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=14400).returncode
 except subprocess.TimeoutExpired:
  for suffix in ['prepare','test']:
   name='nano-full-'+SUBDIR+'-'+suffix
   if out(D+['ps','-q','--filter','name=^/'+name+'$']):subprocess.run(D+['stop','-t','5',name],stdout=log,stderr=subprocess.STDOUT,timeout=30)
  code=124
(CONTROL/'controller-exit.txt').write_text(str(code)+'\n')
print('Full regression controller terminal:',code,flush=True)
