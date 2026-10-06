from pathlib import Path
import subprocess,json,shlex,tarfile,hashlib
root=Path('/private/tmp/nano-head-fallback-20261006');remote='/tmp/nano-head-fallback-20261006';target='ubuntu@145.239.199.166'
def ssh(args):return subprocess.run(['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8',target,shlex.join(args)],check=True)
pack="cd /build/head-fallback-p2p-ARCH-20261006 && test \"$(cat controller-exit.txt)\" = 0 && test \"$(cat usage-exit.txt)\" = 1 && tar -czf /output/ARCH-runtime-proof.tar.gz -- *.txt *.log *.sha256"
for arch,image,volume,on_remote in [('arm64','sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','nano-clean-arm64-build-20261002',False),('amd64','sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','nano-clean-rpc-build-20261002',True)]:
 args=['docker','run','--rm','--network','none','-v',volume+':/build:ro','-v',(remote if on_remote else str(root))+':/output','--entrypoint','/bin/bash',image,'-c',pack.replace('ARCH',arch)]
 if on_remote:ssh(['sudo','-n']+args);subprocess.run(['/usr/bin/scp','-4',target+':'+remote+'/'+arch+'-runtime-proof.tar.gz',str(root)],check=True)
 else:subprocess.run(args,check=True)
 dest=root/(arch+'-runtime-proof');dest.mkdir(exist_ok=False)
 with tarfile.open(root/(arch+'-runtime-proof.tar.gz')) as t:
  assert all('/' not in m.name and m.isfile() for m in t.getmembers());t.extractall(dest)
code="from pathlib import Path;import tarfile,json,hashlib;p=Path("+repr(remote)+");assert (p/'amd-controller-exit.txt').read_text().strip()=='0';m=json.loads((p/'source-manifest.json').read_text());assert all(hashlib.sha256((p/'network'/f).read_bytes()).hexdigest()==h for f,h in m.items());t=tarfile.open(p/'amd-test-proof.tar.gz','w:gz');[(t.add(f,arcname=f.name)) for f in p.iterdir() if f.is_file() and f.name.startswith('amd-') and f.suffix in ['.txt','.log','.json']];t.close()"
ssh(['python3','-c',code]);subprocess.run(['/usr/bin/scp','-4',target+':'+remote+'/amd-test-proof.tar.gz',str(root)],check=True);dest=root/'amd-test-proof';dest.mkdir(exist_ok=False)
with tarfile.open(root/'amd-test-proof.tar.gz') as t:
 assert all('/' not in m.name and m.isfile() for m in t.getmembers());t.extractall(dest)
receipts={'armTests':{'session':23729,'exitCode':0},'amdTests':{'session':92999,'exitCode':0},'native':{'session':98098,'exitCode':0},'realReplay':{'session':52217,'exitCode':0},'armRuntime':{'session':80195,'exitCode':0},'amdRuntime':{'session':79241,'exitCode':0}}
(root/'terminal-exits.json').write_text(json.dumps(receipts,indent=2)+'\n');(root/'active-handles.json').write_text(json.dumps(receipts,indent=2)+'\n');print('Collected all terminal proofs; no binary, database or signing material transferred')
