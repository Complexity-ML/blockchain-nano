from pathlib import Path
import subprocess,tarfile,shlex,json
root=Path('/private/tmp/nano-head-fallback-20261006');remote='/tmp/nano-head-fallback-20261006';target='ubuntu@145.239.199.166'
(root/'tests-amd.sh').write_text((root/'tests.sh').read_text().replace('arm-','amd-'))
archive=root/'source-only.tar.gz'
with tarfile.open(archive,'w:gz') as t:
 for name in ['network','source-manifest.json','tests-amd.sh']:t.add(root/name,arcname=name)
def ssh(args):return subprocess.run(['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8',target,shlex.join(args)],check=True)
ssh(['mkdir',remote]);subprocess.run(['/usr/bin/scp','-4',str(archive),target+':'+remote+'/source-only.tar.gz'],check=True);ssh(['tar','-xzf',remote+'/source-only.tar.gz','-C',remote])
cmd=['sudo','-n','docker','run','--rm','--network','none','--memory','4g','--cpus','2','-v','/home/ubuntu/blockchain-nano-git:/work:ro','-v',remote+':/output','--entrypoint','/bin/bash','sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','/output/tests-amd.sh']
(root/'amd-command.json').write_text(json.dumps(cmd,indent=2)+'\n')
r=subprocess.run(['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8',target,shlex.join(cmd)]);(root/'amd-controller-exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');raise SystemExit(r.returncode)
