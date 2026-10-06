from pathlib import Path
import subprocess,json,shlex,sys,tarfile
arch=sys.argv[1];r=Path('/private/tmp/nano-catalog-selection-docker-20261006');terminal=json.loads((r/(arch+'-terminal-exit.json')).read_text());assert terminal['exitCode']==0,'Original Docker qualifier not successful'
vol='nano-clean-arm64-build-20261002' if arch=='arm64' else 'nano-clean-rpc-build-20261002';image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb' if arch=='arm64' else 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
base=['docker'] if arch=='arm64' else ['sudo','-n','docker'];base+=['run','--rm','--network','none','--read-only','-v',vol+':/build:ro',image,'bash','-c']
folder='/build/catalog-selection-'+arch+'-20261006';script='cd '+folder+" && find . -type f \\( -name '*.log' -o -name '*.txt' -o -name '*.json' -o -name '*.sha256' -o -name '*.d' -o -name '*.exit' \\) -print0 | tar --null -T - -czf -"
cmd=base+[script]
if arch=='amd64':cmd=['/usr/bin/ssh','-4','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(cmd)]
with (r/(arch+'-metadata.tar.gz')).open('wb') as f:subprocess.run(cmd,stdout=f,check=True)
with tarfile.open(r/(arch+'-metadata.tar.gz')) as t:
 for m in t.getmembers():assert m.isfile() and '..' not in Path(m.name).parts and not Path(m.name).is_absolute()
 t.extractall(r/(arch+'-proof'),filter='data')
p=r/(arch+'-proof');assert (p/'sdk-controller-exit.txt').read_text().strip()=='0';assert (p/'regression-controller-exit.txt').read_text().strip()=='0';print(arch,'original qualification terminal success, metadata collected')
