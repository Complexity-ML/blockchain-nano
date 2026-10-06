from pathlib import Path
import json,subprocess,tarfile
root=Path('/private/tmp/nano-timeout-closed-guard-docker-20261006');assert json.loads((root/'amd64-terminal-exit.json').read_text())['exitCode']==0
ssh=['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166']
with (root/'amd64-proof.tar.gz').open('wb') as f:subprocess.run(ssh+['tar -czf - -C /tmp/nano-timeout-closed-guard-docker-20261006 amd64-proof amd64-source-before.log amd64-source-after.log'],stdout=f,check=True)
with tarfile.open(root/'amd64-proof.tar.gz') as t:
 for m in t.getmembers():assert '..' not in Path(m.name).parts and not Path(m.name).is_absolute() and (m.isfile() or m.isdir())
 t.extractall(root,filter='data')
print('AMD64 guard terminal proof collected')
