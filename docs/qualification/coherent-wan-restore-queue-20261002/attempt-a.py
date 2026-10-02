import hashlib,json,os,subprocess,time
from pathlib import Path
control=Path('/tmp/nano-coherent-wan-restore-control-20261002')
state=control/'state.json';proof=control/'preflight.json'
source=Path('/tmp/nano-coherent-wan-paths-20261002')
revision='2d2764ded1f1d7f065ef163d8dcc0fc216268bc1'
target='ubuntu@145.239.199.166';remote_source='/home/ubuntu/nano-production-hex-candidate-20261002'
output=Path('/tmp/nano-coherent-wan-restore-2d2764de-20261002')
arm_image='sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4'
amd_image='sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c'
arm_build='coherent-2d2764de-arm64-20261002';amd_build='coherent-2d2764de-amd64-20261002'
def save(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
def command(args,remote=False):
 if remote:
  import shlex
  args=['ssh','-o','BatchMode=yes',target,shlex.join(args)]
 return subprocess.run(args,capture_output=True,text=True,timeout=45)
def get(args,remote=False):
 r=command(args,remote)
 if r.returncode:raise RuntimeError((args,r.returncode,r.stderr))
 return r.stdout
def read_remote(path):return get(['cat',path],True)
def active(name,remote=False):
 r=command((['sudo','-n'] if remote else [])+['docker','inspect',name,'--format','{{.State.Running}}'],remote)
 if r.returncode==0:return r.stdout.strip()=='true'
 # Only a confirmed missing container is terminal; SSH failures must not start WAN.
 if 'No such object' in r.stderr or 'No such container' in r.stderr:return False
 raise RuntimeError(('Cannot observe job',name,r.stderr))
save(state,{'phase':'waiting','runtimeRevision':revision,'controllerRevision':'554ebc3a','newWanRun':False})
while True:
 jobs={'armFull':active('nano-full-coherent-2d2764de-arm64-20261002-test'),'amdSelected':active('nano-coherent-2d2764de-amd64-test',True),'amdOffline':active('nano-coherent-2d2764de-amd64-offline',True)}
 save(state,{'phase':'waiting','runtimeRevision':revision,'liveJobs':jobs,'observedAt':time.time(),'newWanRun':False})
 if not any(jobs.values()):
  # The audit's queued controller may still be launching after parent shutdown.
  r=command(['ps','-p','510599','-o','stat=,args='],True)
  if r.returncode==1 and not r.stdout.strip():break
  if r.returncode not in [0,1]:raise RuntimeError(('Cannot observe queued audit',r.stderr))
  if r.returncode==0 and ('/tmp/nano-coherent-2d2764de-amd64-offline-control/run.sh' not in r.stdout or r.stdout.strip().startswith('Z')):break
 time.sleep(20)
# These read terminal evidence once; never relaunch or extend a failed campaign.
arm=Path('/tmp/nano-full-2d2764de-arm64-reports-20261002')
for name in ['ctest-exit.txt','audit-exit.txt','post-test-manifest-exit.txt']:assert (arm/name).read_text().strip()=='0',name
for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json')]:assert (arm/a).read_bytes()==(arm/b).read_bytes()
import xml.etree.ElementTree as E
cases=list(E.parse(arm/'results.xml').getroot().iter('testcase'));assert len(cases)==425 and all(c.find('failure') is None and c.find('error') is None and c.find('skipped') is None for c in cases)
amd='/home/ubuntu/nano-coherent-2d2764de-amd64-test-reports-20261002'
for name in ['ctest-exit.txt','container-exit.txt','input-compare-exit.txt']:assert read_remote(amd+'/'+name).strip()=='0',name
assert json.loads(read_remote(amd+'/result-audit.json'))=={'cases':15,'skipped':0,'failed':0,'timeoutsPreserved':True}
for a,b in [('source-manifest-before.json','source-manifest-after.json'),('programs-before.sha256','programs-after.sha256'),('inputs-before.sha256','inputs-after.sha256')]:assert read_remote(amd+'/'+a)==read_remote(amd+'/'+b)
offline='/home/ubuntu/nano-coherent-2d2764de-amd64-offline-reports-20261002'
assert read_remote(offline+'/qualification-exit.txt').strip()=='0'
assert json.loads(read_remote(offline+'/result-audit.json'))['passed']==4
parent=Path('/tmp/nano-coherent-wan-native-test-reports-20261002')
assert (parent/'ctest-exit.txt').read_text().strip()=='0'
assert json.loads((parent/'result-audit.json').read_text())['failed']==0
native_inputs=json.loads((parent/'inputs-before.json').read_text())
for name,digest in native_inputs.items():assert hashlib.sha256((Path(name) if Path(name).is_absolute() else source/name).read_bytes()).hexdigest()==digest,name
assert get(['git','-C',str(source),'rev-parse','--short=8','HEAD']).strip()=='554ebc3a'
assert not get(['git','-C',str(source),'status','--porcelain'])
get(['git','-C',str(source),'diff','--exit-code',revision,'HEAD','--','src','include','network','third_party'])
get(['git','-C',remote_source,'diff','--exit-code',revision,'HEAD','--','src','include','network','third_party'],True)
artifacts=[]
for remote,image,build in [(False,arm_image,arm_build),(True,amd_image,amd_build)]:
 docker=(['sudo','-n'] if remote else [])+['docker']
 info=json.loads(get(docker+['image','inspect','nano-build-dependencies:local','--format','{{json .}}'],remote))
 assert info['Id']==image and info['Architecture']==('amd64' if remote else 'arm64')
 # A provenance file is written only after terminal source/binary checks above.
 script='set -e; f=/build/'+build+'/production-source-revision.txt; if test -f "$f"; then test "$(cat "$f")" = '+revision+'; else printf "%s\\n" '+revision+' > "$f"; fi; sha256sum /build/'+build+'/nano /build/network/nano-p2p'
 hashes=get(docker+['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src=nano-linux-sync-build,dst=/build',image,'bash','-c',script],remote)
 # Match the reviewed terminal nano hash; P2P is recorded separately.
 if remote:expected=read_remote(amd+'/programs-after.sha256')
 else:expected=(Path('/tmp/nano-coherent-2d2764de-arm64-test-reports-20261002')/'programs-after.sha256').read_text()
 line=next(line for line in hashes.splitlines() if line.endswith('/nano'));assert line in expected
 artifacts.append({'host':'amd64' if remote else 'arm64','image':image,'hashes':hashes})
network=source/'build-network'
if not network.exists():network.symlink_to('/Users/boris/Dev/blockchain-nano/build-network',target_is_directory=True)
assert not get(['git','-C',str(source),'status','--porcelain'])
save(proof,{'runtimeRevision':revision,'nativeSourceRevision':'554ebc3a','nativeNano':native_inputs['build-release/nano'],'dockerArtifacts':artifacts,'armFull425Gate':True,'amdSelected15Gate':True,'amdOffline4Gate':True,'native8Gate':True,'allSourceAndBinaryChecks':True,'fullAmdRegressionQualified':False})
env=os.environ.copy();env.update({'NANO_TEST_CONSENSUS_PROFILE':'capella','NANO_TEST_BUILD_VOLUME':'1','NANO_TEST_BUILD_SUBDIRECTORY_ARM64':arm_build,'NANO_TEST_BUILD_SUBDIRECTORY_AMD64':amd_build,'NANO_TEST_REMOTE_SOURCE_REPOSITORY':remote_source,'NANO_TEST_BACKUP_RESTORE':'1','NANO_DOCKER_NOFILE':'65536'})
save(state,{'phase':'running-wan','startedAt':time.time(),'output':str(output),'runtimeRevision':revision})
# run.mjs owns bounded recovery/observation and finally stops just its own nodes.
with (control/'wan-driver.log').open('w') as log:
 r=subprocess.run(['/opt/homebrew/bin/node','tests/multihost/run.mjs',target,revision,str(output)],cwd=source,env=env,stdout=log,stderr=subprocess.STDOUT)
save(state,{'phase':'terminal','exit':r.returncode,'output':str(output),'runtimeRevision':revision,'newWanRun':True})
if r.returncode:raise SystemExit(r.returncode)
