from pathlib import Path
import subprocess,tarfile,shlex,json
root=Path('/private/tmp/nano-history-first-20261006');remote='/tmp/nano-history-first-20261006';target='ubuntu@145.239.199.166'
archive=root/'source-only.tar.gz'
with tarfile.open(archive,'w:gz') as t:
 for name in ['network','source-manifest.json','tests-amd.sh','runtime.sh']:t.add(root/name,arcname=name)
def ssh(args):return subprocess.run(['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8',target,shlex.join(args)],check=True)
ssh(['mkdir',remote]);subprocess.run(['/usr/bin/scp','-4',str(archive),target+':'+remote+'/source-only.tar.gz'],check=True);ssh(['tar','-xzf',remote+'/source-only.tar.gz','-C',remote])
cmd=['sudo','-n','docker','run','--rm','--network','none','--memory','4g','--cpus','2','-v','/home/ubuntu/blockchain-nano-git:/work:ro','-v',remote+':/output','--entrypoint','/bin/bash','sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','/output/tests-amd.sh']
(root/'amd-command.json').write_text(json.dumps(cmd,indent=2)+'\n')
r=subprocess.run(['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8',target,shlex.join(cmd)]);(root/'amd-controller-exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0
runtime=cmd.copy();runtime[runtime.index('--entrypoint'):runtime.index('--entrypoint')]=['-v','nano-clean-rpc-build-20261002:/build'];runtime[-1]='/output/runtime.sh';runtime.append('amd64');(root/'amd-runtime-command.json').write_text(json.dumps(runtime,indent=2)+'\n')
r=subprocess.run(['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8',target,shlex.join(runtime)]);(root/'amd-runtime-controller-exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');raise SystemExit(r.returncode)
