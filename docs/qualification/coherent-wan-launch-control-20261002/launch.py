"""Restore WAN only after terminal qualification and current artifact attestation."""
import hashlib,json,os,socket,subprocess,sys,time
from pathlib import Path
C=Path(__file__).resolve().parent
REV='9e9ebed3833a24f75ce7b29b9a80dcce341dfc3a'
HARNESS_REV='dea3f827b068c0989179a5c99b0753d5ce9f7ed6'
H=Path('/tmp/nano-coherent-wan-harness-candidate-20261002')
S=Path('/tmp/nano-coherent-network-build-candidate-20261002')
N=Path('/tmp/nano-snapshot-account-portable-candidate-20261002')
NB=Path('/tmp/nano-coherent-native-network-build-v2-20261002')
NR=Path('/tmp/nano-coherent-native-network-reports-v3-20261002')
R=Path('/Users/boris/Dev/blockchain-nano')
REMOTE_SOURCE='/home/ubuntu/nano-coherent-network-build-candidate-20261002'
REMOTE_FULL='/home/ubuntu/nano-coherent-network-amd64-reports-20261002'
REMOTE_NETWORK='/home/ubuntu/nano-network-supplement-amd64-reports-20261002'
REMOTE_SIGNED='/home/ubuntu/nano-coherent-amd64-signed-audit-reports-20261002'
OUTPUT=Path('/tmp/nano-coherent-wan-restore-9e9ebed3-20261002')
ARM_REPORT=Path('/tmp/nano-coherent-network-arm64-reports-20261002')
ARM_NETWORK=Path('/tmp/nano-network-supplement-arm64-reports-20261002')
ARM_BUILD='full-network-9e9ebed3-arm64-20261002'
AMD_BUILD='full-network-9e9ebed3-amd64-20261002'
IMAGES={'arm64':'sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4','amd64':'sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c'}
TARGET='ubuntu@145.239.199.166'
ENV={**os.environ,'PATH':'/tmp/nano-coherent-wan-transport-control-20261002/bin:'+os.environ['PATH'],'PYTHONDONTWRITEBYTECODE':'1'}

def require(ok,msg):
 if not ok:raise RuntimeError(msg)
def save(path,value):path.write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
def call(args,remote=False,cwd=None,timeout=60):
 if remote:
  import shlex
  args=['ssh','-o','ConnectTimeout=10',TARGET,shlex.join(args)]
 p=subprocess.run(args,cwd=cwd,env=ENV,capture_output=True,text=True,timeout=timeout)
 require(p.returncode==0,str(args[0])+' exit '+str(p.returncode)+': '+p.stderr[:500])
 return p.stdout

def local_ready(directory,names):return all((directory/n).exists() and (directory/n).read_text().strip()=='0' for n in names)
def remote_ready(directory,names):
 # Only reads terminal files; no workload or metadata writes occur here.
 script="from pathlib import Path;import json,sys;p=Path(sys.argv[1]);print(json.dumps({n:(p/n).read_text().strip() if (p/n).exists() else None for n in sys.argv[2:]}))"
 values=json.loads(call(['python3','-c',script,directory,*names],remote=True))
 return all(v=='0' for v in values.values())
def native_inputs():
 expected=json.loads(Path('/tmp/nano-coherent-016ab243-native-test-reports-20261002/inputs-after.json').read_text())
 actual={p:hashlib.sha256((Path(p) if Path(p).is_absolute() else N/p).read_bytes()).hexdigest() for p in expected}
 require(actual==expected,'native materialized inputs drift');return actual

def program_hashes(arch,build):
 docker=['sudo','-n','docker'] if arch=='amd64' else ['docker']
 paths=['/build/'+build+'/nano','/build/'+build+'/network/nano-p2p','/build/'+build+'/network/nano-altair-probe']
 text=call([*docker,'run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src=nano-linux-sync-build,dst=/build,readonly',IMAGES[arch],'sha256sum',*paths],remote=arch=='amd64')
 return {line.split()[1]:line.split()[0] for line in text.splitlines()}

def source_manifest(root,remote=False):
 script="import json,hashlib,subprocess,sys;from pathlib import Path;p=Path(sys.argv[1]);names=subprocess.check_output(['git','-C',str(p),'ls-files','-z']).split(b'\\0');print(json.dumps({n.decode():hashlib.sha256((p/n.decode()).read_bytes()).hexdigest() for n in names if n},sort_keys=True))"
 return json.loads(call(['python3','-c',script,str(root)],remote=remote,timeout=120))

def main():
 require(__debug__,'assertion-based inherited audits must remain enabled')
 require(sys.argv[1:]==['--execute-qualified-wan'],'Use separate read-only-gates.py until all terminal evidence is independently verified; explicit qualified execution flag required')
 attempt=C/('attempt-'+str(time.time_ns()));attempt.mkdir()
 save(attempt/'scope.json',{'revision':REV,'harnessRevision':HARNESS_REV,'nativeSourceRevision':'016ab2438e9fbfef28f6526290c3de008a44b6ff','output':str(OUTPUT),'newWanNodesStarted':False})
 require(not OUTPUT.exists(),'prior WAN output exists; do not overwrite or relaunch')
 # Missing evidence is a pending gate, never a reason to restart campaigns.
 ready={'armFull':local_ready(ARM_REPORT,['ctest-exit.txt','post-test-manifest-exit.txt','audit-exit.txt']),'armNetwork':local_ready(ARM_NETWORK,['ctest-exit.txt','container-exit.txt','audit-exit.txt']),'amdFull':remote_ready(REMOTE_FULL,['ctest-exit.txt','post-test-manifest-exit.txt','audit-exit.txt']),'amdNetwork':remote_ready(REMOTE_NETWORK,['ctest-exit.txt','container-exit.txt','audit-exit.txt']),'amdSigned':remote_ready(REMOTE_SIGNED,['qualification-exit.txt','container-exit.txt'])}
 save(attempt/'terminal-gates.json',ready)
 if not all(ready.values()):
  save(attempt/'result.json',{'phase':'terminal-gates-pending-or-refused','newWanNodesStarted':False,'gates':ready});return 2
 review=R/'docs/qualification/coherent-terminal-review-controls-20261002'
 local_audit=attempt/'arm-terminal-review'
 call(['bash',str(review/'run-review.sh'),str(S),str(ARM_REPORT),str(local_audit),'/build/'+ARM_BUILD,IMAGES['arm64'],'nano-linux-sync-build',str(ARM_NETWORK)],timeout=180)
 remote_audit='/home/ubuntu/nano-wan-terminal-review-'+attempt.name
 remote_script='/home/ubuntu/blockchain-nano-git/docs/qualification/coherent-terminal-review-controls-20261002/run-review.sh'
 call(['env','PATH=/home/ubuntu/nano-coherent-network-build-control-20261002:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin','bash',remote_script,REMOTE_SOURCE,REMOTE_FULL,remote_audit,'/build/'+AMD_BUILD,IMAGES['amd64'],'nano-linux-sync-build',REMOTE_NETWORK],remote=True,timeout=180)
 remote_controls='/home/ubuntu/blockchain-nano-git/docs/qualification/coherent-amd-signed-audit-control-20261002'
 remote_signed_reviewer='/home/ubuntu/blockchain-nano-git/docs/qualification/coherent-wan-launch-control-20261002/signed-review.py'
 signed_audit=call(['python3',remote_signed_reviewer,REMOTE_SIGNED,REMOTE_FULL,'/home/ubuntu/blockchain-nano-git/docs/qualification/coherent-terminal-review-controls-20261002/source-manifest.json.gz',remote_controls],remote=True)
 (attempt/'amd-signed-independent-review.json').write_text(signed_audit)
 native_audit=call([sys.executable,str(C/'native-audit.py')],timeout=180)
 (attempt/'native-audit.stdout').write_text(native_audit)
 call([sys.executable,str(review/'network-review.py'),str(S),str(NR),str(NB)],timeout=30)
 native_before=native_inputs()
 network_expected={p:h for h,p in [line.split(None,1) for line in (NR/'programs-after.sha256').read_text().splitlines()]}
 network_actual={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in network_expected}
 require(network_actual==network_expected,'native network programs drift')
 require(call(['git','rev-parse','HEAD'],cwd=H).strip()==HARNESS_REV,'harness revision drift')
 require(not call(['git','status','--porcelain'],cwd=H),'harness worktree dirty')
 delta=set(call(['git','diff','--name-only',REV,'HEAD'],cwd=H).splitlines())
 require(delta=={'tests/multihost/build-paths.mjs','tests/multihost/build-paths.test.mjs','tests/multihost/run.mjs'},'unexpected harness/runtime delta')
 require(call(['git','diff','--name-only','016ab2438e9fbfef28f6526290c3de008a44b6ff',REV],cwd=S).splitlines()==['scripts/ci/full-docker.sh'],'native and Docker source mismatch beyond controller')
 # Preserve old native artifacts and attest real retained program bytes.
 artifacts=[]
 for arch,build in [('arm64',ARM_BUILD),('amd64',AMD_BUILD)]:
  remote=arch=='amd64';docker=['sudo','-n','docker'] if remote else ['docker']
  info=json.loads(call([*docker,'image','inspect','nano-build-dependencies:local','--format','{{json .}}'],remote))
  require(info['Id']==IMAGES[arch] and info['Architecture']==arch,'builder tag/architecture drift')
  values=program_hashes(arch,build)
  expected=json.loads(call(['cat',REMOTE_FULL+'/executables-after-tests.json'],True)) if remote else json.loads((ARM_REPORT/'executables-after-tests.json').read_text())
  require(all(expected.get(p)==h for p,h in values.items()),'retained program bytes differ from terminal qualification')
  # Metadata only, after program checks. UID matches the owned build directory.
  uid='1000:1000' if remote else str(os.getuid())+':'+str(os.getgid())
  path='/build/'+build+'/production-source-revision.txt'
  script='set -e; if test -f "$1"; then test "$(cat "$1")" = "$2"; else printf "%s\\n" "$2" > "$1"; fi'
  call([*docker,'run','--rm','--network','none','--read-only','--user',uid,'--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src=nano-linux-sync-build,dst=/build',IMAGES[arch],'sh','-c',script,'sh',path,REV],remote)
  artifacts.append({'architecture':arch,'image':IMAGES[arch],'build':build,'programs':values})
 for remote in [False,True]:
  docker=['sudo','-n','docker'] if remote else ['docker']
  require(not call([*docker,'ps','--filter','label=nano.qualification=multihost','--format','{{.Names}}'],remote).strip(),'existing WAN node; never replace it')
 with socket.socket() as probe:
  probe.settimeout(1);require(probe.connect_ex(('127.0.0.1',18545))!=0,'native RPC already occupied')
 sources_before={'harness':source_manifest(H),'dockerMac':source_manifest(S),'nativeCore':source_manifest(N),'dockerVps':source_manifest(REMOTE_SOURCE,True)}
 save(attempt/'source-manifests-before.json',sources_before)
 save(attempt/'artifact-attestation.json',{'revision':REV,'harnessRevision':HARNESS_REV,'artifacts':artifacts,'nativeInputs':native_before,'nativeNetworkPrograms':network_actual,'wanNodesStarted':False,'transport':'pinned IPv6 with original IPv4 HostKeyAlias'})
 env={**ENV,'NANO_TEST_CONSENSUS_PROFILE':'capella','NANO_TEST_BUILD_VOLUME':'1','NANO_TEST_NETWORK_WITH_CORE':'1','NANO_TEST_BACKUP_RESTORE':'1','NANO_TEST_BUILD_SUBDIRECTORY_ARM64':ARM_BUILD,'NANO_TEST_BUILD_SUBDIRECTORY_AMD64':AMD_BUILD,'NANO_TEST_NATIVE_BUILD_DIRECTORY':str(N/'build-release'),'NANO_TEST_NATIVE_NETWORK_DIRECTORY':str(NB),'NANO_TEST_REMOTE_SOURCE_REPOSITORY':REMOTE_SOURCE,'NANO_DOCKER_NOFILE':'65536'}
 save(attempt/'driver-environment.json',{k:v for k,v in env.items() if k.startswith('NANO_')})
 with (attempt/'driver.log').open('w') as log:
  code=subprocess.run(['/opt/homebrew/bin/node','tests/multihost/run.mjs',TARGET,REV,str(OUTPUT)],cwd=H,env=env,stdout=log,stderr=subprocess.STDOUT).returncode
 sources_after={'harness':source_manifest(H),'dockerMac':source_manifest(S),'nativeCore':source_manifest(N),'dockerVps':source_manifest(REMOTE_SOURCE,True)}
 save(attempt/'source-manifests-after.json',sources_after)
 require(sources_after==sources_before,'tracked source drift during WAN')
 require(native_inputs()==native_before,'native input drift after WAN')
 require({p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in network_actual}==network_actual,'native network drift after WAN')
 require(all(program_hashes(a['architecture'],a['build'])==a['programs'] for a in artifacts),'Docker artifact drift after WAN')
 save(attempt/'result.json',{'phase':'terminal','wanDriverExit':code,'wanHarnessInvoked':True,'runtimeArtifactHashesUnchanged':True,'productionQualified':False})
 return code

if __name__=='__main__':
 try:code=main()
 except Exception as error:
  print(str(error),file=sys.stderr);code=1
 raise SystemExit(code)
