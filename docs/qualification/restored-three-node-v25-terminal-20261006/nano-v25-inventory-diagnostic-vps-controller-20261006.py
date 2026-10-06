from pathlib import Path
import subprocess,json,shlex,tarfile,hashlib
base=Path('/private/tmp');campaign=base/'nano-restored-three-node-v25-timeout-preparation-20261006/campaign';audit=base/'nano-v25-post-stop-vps-audit-20261006';driver=base/'nano-v25-inventory-diagnostic-vps-driver-20261006';out=base/'nano-v25-inventory-diagnostic-vps-proof-20261006'
def read(p):return json.loads(p.read_text())
assert read(campaign/'terminal-exit.json')['originalSession']==29725 and read(campaign/'result.json')['stopReceipts']==3;assert read(audit/'controller-exit.json')['exitCode']==0 and read(audit/'immutability.json')['hashesUnchanged']
r=read(campaign/'resources.json');a=read(campaign/'artifacts.json')['nodes']['vps-docker'];remote=r['remoteRoot']+'/inventory-public-driver';ssh=['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166'];out.mkdir(exist_ok=False)
subprocess.run(ssh+[shlex.join(['mkdir','-p',remote+'/source/tests/capella'])],check=True)
cpp=base/'nano-v25-inventory-diagnostic-source-20261006/tests/capella/paused_capture.cpp'
for source,dest in [(cpp,'source/tests/capella/paused_capture.cpp'),(driver/'build.sh','build.sh'),(driver/'run.py','run.py')]:
 with source.open('rb') as f:subprocess.run(ssh+['cat > '+shlex.quote(remote+'/'+dest)],stdin=f,check=True)
cmd=['sudo','-n','python3',remote+'/run.py',r['remoteRoot'],r['remoteContainer'],r['vpsVolume'],a['image'],a['buildVolume'],remote]
with (out/'controller.log').open('w') as f:p=subprocess.run(ssh+[shlex.join(cmd)],stdout=f,stderr=subprocess.STDOUT)
(out/'controller-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0
names=['build-exit.txt','compile-command.txt','compile.log','link-command.txt','link.log','programs.sha256','build-command.json','capture.json','error.log','exit.json','immutability.json','source.json']
with (out/'public-proof.tar.gz').open('wb') as f:subprocess.run(ssh+[shlex.join(['sudo','-n','tar','-czf','-','-C',remote+'/proof',*names])],stdout=f,check=True)
with tarfile.open(out/'public-proof.tar.gz') as t:
 for m in t.getmembers():assert m.isfile() and m.name in names
 t.extractall(out,filter='data')
assert read(out/'source.json')['sha256']==hashlib.sha256(cpp.read_bytes()).hexdigest();assert read(out/'capture.json')['restart']==read(audit/'capture.json')['restart'];assert read(out/'immutability.json')['hashesUnchanged'];print('VPS inventory metadata verified, exact previously audited stop commitment matched; no database/config/key export')
