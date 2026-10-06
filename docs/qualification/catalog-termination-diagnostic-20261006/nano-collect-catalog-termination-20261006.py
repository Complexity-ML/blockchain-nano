from pathlib import Path
import subprocess,json,hashlib,shlex
root=Path('/private/tmp/nano-catalog-termination-integrated-20261006');remote='/tmp/nano-catalog-termination-integrated-20261006';ssh=['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166']
for arch,image,volume in [('arm64','sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','nano-clean-arm64-build-20261002'),('amd64','sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','nano-clean-rpc-build-20261002')]:
 path='/build/catalog-termination-cmake-targets-'+arch+'-20261006';cmd=['docker','run','--rm','--network','none','-v',volume+':/build:ro',image,'sh','-c','sha256sum '+path+'/nano-p2p '+path+'/nano-capella-catalog-termination-tests '+path+'/nano-capella-catalog-relay-tests '+path+'/nano-capella-head-catalog-tests '+path+'/nano-capella-head-history-fairness-tests']
 if arch=='amd64':r=subprocess.run(ssh+[shlex.join(['sudo','-n']+cmd)],capture_output=True,text=True,check=True)
 else:r=subprocess.run(cmd,capture_output=True,text=True,check=True)
 (root/(arch+'-programs.sha256')).write_text(r.stdout)
 cmd=['docker','run','--rm','--network','none','-v',volume+':/build:ro',image,path+'/nano-p2p']
 if arch=='amd64':r=subprocess.run(ssh+[shlex.join(['sudo','-n']+cmd)],capture_output=True,text=True)
 else:r=subprocess.run(cmd,capture_output=True,text=True)
 (root/(arch+'-runtime-usage.log')).write_text(r.stdout+r.stderr);(root/(arch+'-runtime-usage-exit.json')).write_text(json.dumps({'exitCode':r.returncode,'expectedExitCode':1,'scope':'missing-arguments Usage only, no network qualification'})+'\n');assert r.returncode==1 and 'Usage' in r.stdout+r.stderr
for name in ['amd64-cmake-targets-controller-exit.txt','amd64-cmake-deps-path.txt','amd64-cmake-configure.log','amd64-cmake-targets-build.log','amd64-cmake-targets-ctest.log']:
 subprocess.run(['/usr/bin/scp','-4','ubuntu@145.239.199.166:'+remote+'/'+name,str(root/name)],check=True)
r=subprocess.run([str(root/'native/runtime')],capture_output=True,text=True);(root/'native/runtime-usage.log').write_text(r.stdout+r.stderr);(root/'native/runtime-usage-exit.json').write_text(json.dumps({'exitCode':r.returncode,'expectedExitCode':1,'scope':'missing-arguments Usage only'})+'\n');assert r.returncode==1 and 'Usage' in r.stdout+r.stderr
assert json.loads((root/'amd64-launch-exit.json').read_text())['exitCode']==0
for arch in ['arm64','amd64']:assert (root/(arch+'-cmake-targets-controller-exit.txt')).read_text().strip()=='0'
print('Three architectures: targeted tests, CMake SDK builds, P2P hashes and Usage verified; not live network qualification.')
