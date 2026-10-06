from pathlib import Path
import subprocess,json,shlex,sys,tarfile
arch=sys.argv[1];r=Path('/private/tmp/nano-delivery-diagnostic-docker-20261006');assert json.loads((r/(arch+'-terminal-exit.json')).read_text())['exitCode']==0
ssh=['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166']
if arch=='amd64':
 with (r/'amd64-sdk-proof.tar.gz').open('wb') as f:subprocess.run(ssh+['tar -czf - -C /tmp/nano-delivery-diagnostic-docker-20261006 amd64-proof amd64-source-before.log amd64-source-after.log'],stdout=f,check=True)
 with tarfile.open(r/'amd64-sdk-proof.tar.gz') as t:
  for m in t.getmembers():assert '..' not in Path(m.name).parts and not Path(m.name).is_absolute() and (m.isfile() or m.isdir())
  t.extractall(r,filter='data')
vol='nano-clean-arm64-build-20261002' if arch=='arm64' else 'nano-clean-rpc-build-20261002';image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb' if arch=='arm64' else 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
cmd=(['docker'] if arch=='arm64' else ['sudo','-n','docker'])+['run','--rm','--network','none','--read-only','-v',vol+':/build:ro',image,'bash','-c','cd /build/delivery-diagnostic-'+arch+r'-20261006 && find . -type f \( -name "*.log" -o -name "*.txt" -o -name "*.json" -o -name "*.sha256" \) -print0 | tar --null -T - -czf -']
actual=ssh+[shlex.join(cmd)] if arch=='amd64' else cmd
with (r/(arch+'-wire-proof.tar.gz')).open('wb') as f:subprocess.run(actual,stdout=f,check=True)
with tarfile.open(r/(arch+'-wire-proof.tar.gz')) as t:
 for m in t.getmembers():assert m.isfile() and '..' not in Path(m.name).parts and not Path(m.name).is_absolute()
 t.extractall(r/(arch+'-wire-proof'),filter='data')
assert (r/(arch+'-proof/sdk-controller-exit.txt')).read_text().strip()=='0'
assert (r/(arch+'-wire-proof/regression-controller-exit.txt')).read_text().strip()=='0'
assert (r/(arch+'-wire-proof/test-wire-exit.txt')).read_text().strip()=='0'
print(arch,'terminal SDK and actual wire proof collected')
