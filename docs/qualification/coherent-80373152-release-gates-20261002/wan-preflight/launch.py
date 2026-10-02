import hashlib,json,os,subprocess,sys,time,shlex
from pathlib import Path
control=Path('/tmp/nano-coherent-80373152-wan-preflight-20261002')
source=Path('/tmp/nano-snapshot-ssz-view-candidate-20261002')
revision='803731526c33111ea128bf6fddf5d7323fb5b220'
target='ubuntu@145.239.199.166';remote_source='/home/ubuntu/nano-snapshot-ssz-view-candidate-20261002'
output=Path('/tmp/nano-coherent-wan-restore-80373152-20261002')
state=control/'launch-state.json'
def save(path,obj):path.write_text(json.dumps(obj,indent=2,sort_keys=True)+'\n')
def get(args,remote=False):
 if remote:args=['ssh','-o','BatchMode=yes',target,shlex.join(args)]
 r=subprocess.run(args,capture_output=True,text=True,timeout=45)
 if r.returncode:raise RuntimeError((args[0],r.returncode,r.stderr[:400]))
 return r.stdout
# Always re-execute the evidence gate. Never trust a saved ready flag.
r=subprocess.run([sys.executable,str(control/'check.py')],capture_output=True,text=True,timeout=120)
(control/'launch-preflight.stdout').write_text(r.stdout);(control/'launch-preflight.stderr').write_text(r.stderr)
if r.returncode:
 save(state,{'phase':'preflight-refused','exit':r.returncode,'newWanNodesStarted':False,'runtimeRevision':revision})
 raise SystemExit(r.returncode)
proof=json.loads(r.stdout);assert proof['readyForBinaryAttestation'] and proof['runtimeRevision']==revision
assert not output.exists(),'Preserve previous run; never reuse its report directory'
artifacts=[]
for remote,image,build in [
 (False,'sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4','coherent-80373152-arm64-20261002'),
 (True,'sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c','full-coherent-80373152-amd64-20261002')]:
 docker=(['sudo','-n'] if remote else [])+['docker']
 info=json.loads(get(docker+['image','inspect','nano-build-dependencies:local','--format','{{json .}}'],remote))
 assert info['Id']==image and info['Architecture']==('amd64' if remote else 'arm64')
 args=docker+['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src=nano-linux-sync-build,dst=/build,readonly',image,'sha256sum','/build/'+build+'/nano','/build/network/nano-p2p']
 hashes=get(args,remote);values={line.split()[1]:line.split()[0] for line in hashes.splitlines()}
 if remote:
  expected=json.loads(get(['cat','/home/ubuntu/nano-full-coherent-80373152-amd-reports-20261002/executables-after-tests.json'],True))
  for path,digest in values.items():assert expected[path]==digest,path
 else:
  expected={line.split()[1]:line.split()[0] for line in Path('/tmp/nano-coherent-80373152-arm64-test-reports-20261002/programs-after.sha256').read_text().splitlines()}
  assert expected['/build/'+build+'/nano']==values['/build/'+build+'/nano']
  prior=json.loads(Path('/tmp/nano-full-2d2764de-arm64-reports-20261002/executables-after-tests.json').read_text())
  assert prior['/build/network/nano-p2p']==values['/build/network/nano-p2p']
 # Only provenance metadata is added after qualified programs match.
 script='set -e; f=/build/'+build+'/production-source-revision.txt; if test -f "$f"; then test "$(cat "$f")" = '+revision+'; else printf "%s\\n" '+revision+' > "$f"; fi'
 get(docker+['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src=nano-linux-sync-build,dst=/build',image,'bash','-c',script],remote)
 artifacts.append({'architecture':info['Architecture'],'image':image,'build':build,'programs':values})
network=source/'build-network';canonical=Path('/Users/boris/Dev/blockchain-nano/build-network')
if not network.exists():network.symlink_to(canonical,target_is_directory=True)
assert network.resolve()==canonical.resolve()
assert not get(['git','-C',str(source),'status','--porcelain'])
save(control/'launch-attestation.json',{'runtimeRevision':revision,'dockerArtifacts':artifacts,'preflight':proof,'newWanNodesStarted':False})
# Preserve artifact identity around the actual WAN invocation, including
# native materialized link/runtime inputs. Database state is intentionally
# excluded: blocks and signer journals must advance during this scenario.
def source_manifest():
 names=subprocess.check_output(['git','ls-files','-z'],cwd=source).split(b'\0')
 return {n.decode():hashlib.sha256((source/n.decode()).read_bytes()).hexdigest() for n in names if n}
def native_manifest():
 expected=json.loads(Path('/tmp/nano-coherent-80373152-native-test-reports-20261002/inputs-after.json').read_text())
 actual={name:hashlib.sha256((Path(name) if Path(name).is_absolute() else source/name).read_bytes()).hexdigest() for name in expected}
 assert actual==expected,'Native qualified inputs changed before WAN'
 return actual
def remote_manifest():
 script="import json,hashlib,subprocess,sys;from pathlib import Path;p=Path(sys.argv[1]);names=subprocess.check_output(['git','ls-files','-z'],cwd=p).split(b'\\0');print(json.dumps({n.decode():hashlib.sha256((p/n.decode()).read_bytes()).hexdigest() for n in names if n},sort_keys=True))"
 return json.loads(get(['python3','-c',script,remote_source],True))
source_before=source_manifest();native_before=native_manifest();remote_before=remote_manifest()
assert remote_before==source_before,'Qualified source trees differ before WAN'
save(control/'wan-remote-source-before.json',remote_before)
save(control/'wan-source-before.json',source_before);save(control/'wan-native-inputs-before.json',native_before)

env=os.environ.copy();env.update({'NANO_TEST_CONSENSUS_PROFILE':'capella','NANO_TEST_BUILD_VOLUME':'1','NANO_TEST_BUILD_SUBDIRECTORY_ARM64':artifacts[0]['build'],'NANO_TEST_BUILD_SUBDIRECTORY_AMD64':artifacts[1]['build'],'NANO_TEST_REMOTE_SOURCE_REPOSITORY':remote_source,'NANO_TEST_BACKUP_RESTORE':'1','NANO_DOCKER_NOFILE':'65536'})
save(state,{'phase':'running-wan','startedAt':time.time(),'output':str(output),'runtimeRevision':revision})
with (control/'wan-driver.log').open('w') as log:
 r=subprocess.run(['/opt/homebrew/bin/node','tests/multihost/run.mjs',target,revision,str(output)],cwd=source,env=env,stdout=log,stderr=subprocess.STDOUT)
try:
 source_after=source_manifest();save(control/'wan-source-after.json',source_after)
 assert source_after==source_before,'Source changed during WAN'
 remote_after=remote_manifest();save(control/'wan-remote-source-after.json',remote_after)
 assert remote_after==remote_before==source_after,'Remote source changed during WAN'
 assert not get(['git','-C',remote_source,'status','--porcelain'],True)
 assert not get(['git','-C',str(source),'status','--porcelain'])
 native_after=native_manifest();save(control/'wan-native-inputs-after.json',native_after)
 assert native_after==native_before
 for artifact in artifacts:
  remote=artifact['architecture']=='amd64';docker=(['sudo','-n'] if remote else [])+['docker']
  args=docker+['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src=nano-linux-sync-build,dst=/build,readonly',artifact['image'],'sha256sum','/build/'+artifact['build']+'/nano','/build/network/nano-p2p']
  values={line.split()[1]:line.split()[0] for line in get(args,remote).splitlines()}
  assert values==artifact['programs'],'Docker programs changed during WAN'
 save(control/'wan-artifact-integrity.json',{'sourceAndProgramsUnchanged':True,'nativeInputs':len(native_after),'dockerOwners':2,'sourceOwners':2,'runtimeRevision':revision,'wanExit':r.returncode})
except Exception as error:
 save(state,{'phase':'artifact-integrity-failed','reason':str(error),'wanExit':r.returncode,'output':str(output),'runtimeRevision':revision,'wanHarnessInvoked':True,'nodesStartedProvenByThisController':False})
 raise
save(state,{'phase':'terminal','exit':r.returncode,'output':str(output),'runtimeRevision':revision,'wanHarnessInvoked':True,'nodesStartedProvenByThisController':False})
raise SystemExit(r.returncode)
