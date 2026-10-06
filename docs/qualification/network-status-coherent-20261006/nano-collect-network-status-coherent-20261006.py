from pathlib import Path
import subprocess,json,shlex,sys,tarfile
arch=sys.argv[1];r=Path('/private/tmp/nano-network-status-coherent-20261006');assert json.loads((r/(arch+'-terminal-exit.json')).read_text())['exitCode']==0
ssh=['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166'];vol='nano-clean-arm64-build-20261002' if arch=='arm64' else 'nano-clean-rpc-build-20261002';image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb' if arch=='arm64' else 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
folder='/build/network-status-coherent-'+arch+'-20261006'
script='cd '+folder+r' && find . -type f \( -name "*.log" -o -name "*.txt" -o -name "*.json" -o -name "*.sha256" -o -name "*.d" -o -name "*.exit" \) -print0 | tar --null -T - -czf -'
cmd=(['docker'] if arch=='arm64' else ['sudo','-n','docker'])+['run','--rm','--network','none','--read-only','-v',vol+':/build:ro',image,'bash','-c',script];actual=ssh+[shlex.join(cmd)] if arch=='amd64' else cmd
with (r/(arch+'-proof.tar.gz')).open('wb') as f:subprocess.run(actual,stdout=f,check=True)
out=r/(arch+'-proof');out.mkdir(exist_ok=False)
with tarfile.open(r/(arch+'-proof.tar.gz')) as t:
 for m in t.getmembers():assert m.isfile() and '..' not in Path(m.name).parts and not Path(m.name).is_absolute()
 t.extractall(out,filter='data')
for name in ['controller-exit.txt','runtime-controller-exit.txt','regression-controller-exit.txt','test-network-status-identity-exit.txt','test-network-status-sync-exit.txt']:assert (out/name).read_text().strip()=='0'
if arch=='amd64':
 for phase in ['before','after']:
  with (r/(arch+'-source-'+phase+'.log')).open('wb') as f:subprocess.run(ssh+['cat /tmp/nano-network-status-coherent-20261006/'+arch+'-source-'+phase+'.log'],stdout=f,check=True)
for field in ['identity','sync']:
 rows=[json.loads(line) for line in (out/('test-network-status-'+field+'.log')).read_text().splitlines() if line.startswith('{')];probe=[x for x in rows if x.get('scenario')=='network-status-during-held-admission'];assert probe and all(x['field']==field and x['completedWhileAdmissionHeld'] and x['cancelledWriterUnpublished'] and x['sameStatus'] for x in probe)
print(arch,'coherent programs, dependency scans and two held-admission status regressions verified')
