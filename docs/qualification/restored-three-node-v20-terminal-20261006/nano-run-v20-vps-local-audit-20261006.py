from pathlib import Path
import json,subprocess,shlex,tarfile,hashlib
root=Path('/private/tmp/nano-restored-three-node-v20-startup-diagnostic-20261006');terminal=json.loads((root/'campaign/terminal-exit.json').read_text());assert terminal['originalSession']==34949 and terminal['exitCode'] in [0,1]
result=json.loads((root/'campaign/result.json').read_text());assert result['stopReceipts']==3
resources=json.loads((root/'campaign/resources.json').read_text());a=json.loads((root/'campaign/artifacts.json').read_text())['nodes']['vps-docker'];receipt=json.loads((root/'campaign/2-external-stop-receipt.json').read_text())
script=Path('/private/tmp/nano-v18-vps-local-audit-20261006.py');source=script.read_bytes();remote=resources['remoteRoot'];ssh=['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166'];out=Path('/private/tmp/nano-v20-post-stop-vps-audit-20261006');out.mkdir(exist_ok=False)
subprocess.run(ssh+['cat > '+shlex.quote(remote+'/post-stop-public-audit.py')],input=source,check=True)
cmd=['sudo','-n','python3',remote+'/post-stop-public-audit.py',remote,resources['remoteContainer'],resources['vpsVolume'],a['image'],a['buildVolume']]
with (out/'controller.log').open('w') as f:p=subprocess.run(ssh+[shlex.join(cmd)],stdout=f,stderr=subprocess.STDOUT)
(out/'controller-exit.json').write_text(json.dumps({'exitCode':p.returncode}));assert p.returncode==0
names=['capture.json','error.log','exit.json','immutability.json','data-before.sha256','source-stopped-state.json','program-hash.json','request-config-hashes.json']
with (out/'public-proof.tar.gz').open('wb') as f:subprocess.run(ssh+[shlex.join(['sudo','-n','tar','-czf','-','-C',remote+'/local-readonly-audit',*names])],stdout=f,check=True)
with tarfile.open(out/'public-proof.tar.gz') as t:
 for member in t.getmembers():assert member.isfile() and member.name in names
 t.extractall(out,filter='data')
d=json.loads((out/'capture.json').read_text());assert d['restart']==receipt['restart'];assert {x['index']:x['restart'] for x in d['validatorKeys']}=={x['index']:x['restart'] for x in receipt['validatorKeys']}
(out/'audit-provenance.json').write_text(json.dumps({'publicScriptSHA256':hashlib.sha256(source).hexdigest(),'configurationAndRequestDerivedOnVps':True,'publicProofOnlyCollected':True,'exactIndependentControllerReceiptCompared':True,'databaseOrKeysTransferred':False},indent=2)+'\n');print('VPS local audit verified; exact independently retained stop commitments match; public proof collected')
