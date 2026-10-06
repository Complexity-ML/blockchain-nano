from pathlib import Path
import json,subprocess,hashlib,shutil
root=Path('/private/tmp/nano-restored-three-node-v28-reactor-failfast-20261006');audit=Path('/private/tmp/nano-v28-post-stop-native-replay-budget-20261006');out=Path('/private/tmp/nano-v28-readonly-graph-metrics-20261006');reader=out/'reader'
assert json.loads((root/'campaign/terminal-exit.json').read_text())['originalSession']==7884
assert json.loads((root/'campaign/result.json').read_text())['stopReceipts']==3
for node in ['mac-native','mac-docker']:
 data=root/node/'data';expected=json.loads((audit/(node+'-before.json')).read_text())
 def hashes():return {str(p.relative_to(data)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(data.rglob('*')) if p.is_file()}
 assert hashes()==expected
 req=json.loads((audit/(node+'-request.json')).read_text());req['runtimeBlockLimit']=1024;(out/(node+'-request.json')).write_text(json.dumps(req,indent=2)+'\n')
 args=[str(reader),'--capture',str(root/'config/genesis.json'),str(audit/(node+'-config.json')),str(data/'chain'),str(out/(node+'-request.json'))]
 with (out/(node+'-capture.json')).open('w') as f,(out/(node+'-error.log')).open('w') as err:p=subprocess.run(args,stdout=f,stderr=err)
 (out/(node+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode}));assert p.returncode==0,(node,p.returncode)
 assert hashes()==expected
 result=json.loads((out/(node+'-capture.json')).read_text());assert result['restart']==json.loads((audit/(node+'-capture.json')).read_text())['restart'];assert result['validatorKeys']==json.loads((audit/(node+'-capture.json')).read_text())['validatorKeys']
 (out/(node+'-metrics.json')).write_text(json.dumps({'forkGraphMetrics':result['forkGraphMetrics'],'allSourceHashesUnchanged':True,'sameAuditedRestartAndSignerPins':True,'readOnly':True,'productionQualified':False},indent=2)+'\n');print(node,json.dumps(result['forkGraphMetrics']),flush=True)
