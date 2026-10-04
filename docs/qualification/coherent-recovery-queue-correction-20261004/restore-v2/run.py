#!/usr/bin/env python3
"""Wait for existing complete gates, then restore a fresh private candidate node."""
import os,sys,json,subprocess,shlex,time,hashlib,xml.etree.ElementTree as ET
from pathlib import Path
REV='efbeb9bca3d42e41edb9cb81afba98c0b953a90f'
TARGET='ubuntu@145.239.199.166'
SOURCE=Path('/tmp/nano-production-coherent-20261004')
REMOTE='/home/ubuntu/nano-production-coherent-20261004'
CONTROL=Path(__file__).resolve().parent
REPORT={'arm64':'/tmp/nano-coherent-full-arm64-reports-20261004','amd64':'/home/ubuntu/nano-coherent-full-amd64-reports-20261004'}
FULLCONTROL={'arm64':'/tmp/nano-coherent-full-arm64-control-20261004','amd64':'/home/ubuntu/nano-coherent-full-amd64-control-20261004'}
IMAGES={'arm64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','amd64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'}
VOLUMES={'arm64':'nano-clean-arm64-build-20261002','amd64':'nano-clean-rpc-build-20261002'}
ENV={**os.environ,'PATH':'/tmp/nano-wan-dual-family-transport-control-20261004/bin:'+os.environ['PATH']}
OUTPUT='/tmp/nano-coherent-wan-restore-efbeb9bc-20261004'
def run(args,arch='arm64',timeout=45):
 if arch=='amd64':args=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=10',TARGET,shlex.join(args)]
 r=subprocess.run(args,env=ENV,text=True,capture_output=True,timeout=timeout)
 if r.returncode:raise RuntimeError((args,r.returncode,r.stderr[:1000]))
 return r.stdout.strip()
def read(path,arch):return run(['cat',path],arch)
def save(name,data):(CONTROL/name).write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
def docker(args,arch):return run((['docker'] if arch=='arm64' else ['sudo','-n','docker'])+args,arch)
def observe(arch):
 script='import pathlib,json,sys;p=pathlib.Path(sys.argv[1]);f=p/"controller-exit.txt";print(json.dumps({"terminal":f.is_file(),"exit":int(f.read_text()) if f.is_file() else None}))'
 row=json.loads(run(['python3','-c',script,FULLCONTROL[arch]],arch))
 row['liveContainers']=docker(['ps','--filter','name=^/nano-full-coherent-efbeb9bc-'+arch+'-','--format','{{.Names}}'],arch).splitlines()
 return row
try:
 save('state.json',{'phase':'waiting','revision':REV,'wanStarted':False})
 start=time.monotonic();absent={a:0 for a in REPORT}
 while True:
  states={a:observe(a) for a in REPORT}
  save('state.json',{'phase':'waiting','revision':REV,'wanStarted':False,'observedAt':time.time(),'campaigns':states})
  for a,s in states.items():
   if s['terminal'] and s['exit']!=0:raise RuntimeError(f'{a} full campaign failed; no restore launched')
   absent[a]=absent[a]+1 if not s['terminal'] and not s['liveContainers'] else 0
   if absent[a]>=6:raise RuntimeError(f'{a} no live campaign and no terminal evidence; no restart')
  if all(s['terminal'] for s in states.values()):break
  if time.monotonic()-start>14400:raise RuntimeError('Wait expired; existing campaigns remain untouched')
  time.sleep(20)
 save('state.json',{'phase':'preflight','revision':REV,'wanStarted':False})
 proofs={}
 for arch in REPORT:
  reports=REPORT[arch]
  for n in ['ctest-exit.txt','audit-exit.txt','post-test-manifest-exit.txt']:assert read(reports+'/'+n,arch)=='0',n
  for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json')]:assert read(reports+'/'+a,arch)==read(reports+'/'+b,arch)
  cases=list(ET.fromstring(read(reports+'/results.xml',arch)).iter('testcase'))
  assert len(cases)==427 and all(not any(c.find(k) is not None for k in ['failure','error','skipped']) for c in cases)
  assert run(['git','-C',str(SOURCE) if arch=='arm64' else REMOTE,'rev-parse','HEAD'],arch)==REV
  assert not run(['git','-C',str(SOURCE) if arch=='arm64' else REMOTE,'status','--porcelain'],arch)
  docker(['volume','inspect',VOLUMES[arch]],arch)
  assert docker(['image','inspect',IMAGES[arch],'--format','{{.Id}}'],arch)==IMAGES[arch]
  manifest=json.loads(read(reports+'/executables-after-tests.json',arch))
  directory='/build/coherent-efbeb9bc-'+arch
  args=['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build,readonly',IMAGES[arch],'sha256sum',directory+'/nano',directory+'/network/nano-p2p']
  hashes=docker(args,arch)
  for line in hashes.splitlines():digest,path=line.split();assert manifest[path]==digest,path
  # Use the directory's actual owner, retaining dropped capabilities.
  owner=docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build,readonly',IMAGES[arch],'stat','-c','%u:%g',directory],arch)
  assert len(owner.split(':'))==2 and all(p.isdigit() for p in owner.split(':'))
  # Source marker is metadata; existing executables remain unchanged.
  docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user',owner,'--mount','type=volume,src='+VOLUMES[arch]+',dst=/build',IMAGES[arch],'sh','-c','set -eu; f="$1/production-source-revision.txt"; if test -e "$f"; then test "$(cat "$f")" = "$2"; else printf "%s\\n" "$2" > "$f"; fi','sh',directory,REV],arch)
  proofs[arch]={'cases':len(cases),'hashes':hashes,'reports':reports,'image':IMAGES[arch]}
 native=Path('/tmp/nano-coherent-native-reports-20261004')
 assert (native/'controller-exit.txt').read_text().strip()=='0'
 assert json.loads((native/'result.json').read_text())['revision']==REV
 inputs=json.loads((native/'inputs-after-tests.json').read_text())
 assert inputs==json.loads((native/'inputs-before-tests.json').read_text())
 for path,digest in inputs.items():assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest,path
 assert json.loads(Path('/tmp/nano-coherent-sst-cross-20261004/result.json').read_text())['physicalCrossReadersPassed']
 save('preflight.json',{'revision':REV,'docker':proofs,'nativeInputsRehashed':len(inputs),'physicalSSTCrossPassed':True,'productionQualified':False})
 assert not Path(OUTPUT).exists()
 env={**ENV,'NANO_TEST_CONSENSUS_PROFILE':'capella','NANO_TEST_BUILD_VOLUME':'1','NANO_TEST_NETWORK_WITH_CORE':'1','NANO_TEST_BUILD_SUBDIRECTORY_ARM64':'coherent-efbeb9bc-arm64','NANO_TEST_BUILD_SUBDIRECTORY_AMD64':'coherent-efbeb9bc-amd64','NANO_TEST_BUILDER_IMAGE_ARM64':IMAGES['arm64'],'NANO_TEST_BUILDER_IMAGE_AMD64':IMAGES['amd64'],'NANO_TEST_BUILD_VOLUME_ARM64':VOLUMES['arm64'],'NANO_TEST_BUILD_VOLUME_AMD64':VOLUMES['amd64'],'NANO_TEST_NATIVE_BUILD_DIRECTORY':'/tmp/nano-coherent-native-build-20261004','NANO_TEST_NATIVE_NETWORK_DIRECTORY':'/tmp/nano-coherent-native-build-20261004/network','NANO_TEST_REMOTE_SOURCE_REPOSITORY':REMOTE,'NANO_TEST_BACKUP_RESTORE':'1','NANO_DOCKER_NOFILE':'65536'}
 save('state.json',{'phase':'restoring','revision':REV,'wanStarted':True,'output':OUTPUT})
 with (CONTROL/'restore-driver.log').open('w') as log:
  code=subprocess.run(['node','tests/multihost/run.mjs',TARGET,REV,OUTPUT],cwd=SOURCE,env=env,stdout=log,stderr=subprocess.STDOUT).returncode
 save('state.json',{'phase':'terminal','exit':code,'revision':REV,'wanStarted':True,'output':OUTPUT,'productionQualified':False})
 sys.exit(code)
except Exception as e:
 save('failure.json',{'error':str(e),'productionQualified':False})
 state=json.loads((CONTROL/'state.json').read_text());save('state.json',{**state,'phase':'terminal','exit':1});raise
