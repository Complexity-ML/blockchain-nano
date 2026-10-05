import json,time,subprocess,hashlib
from pathlib import Path
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-restored-three-node-v6-20261006/campaign')
print('Waiting for the original v6 terminal result and clean Mac stop before compilation',flush=True)
for i in range(100):
 if (root/'result.json').exists():break
 time.sleep(5)
else:raise RuntimeError('Original campaign has not terminated; compilation not started')
stop=json.loads((root/'native-state.json').read_text());assert stop['exitCode']==0 and stop['signalCode'] is None
state=json.loads(subprocess.check_output(['docker','inspect','nano-restored-three-node-mac-muvt00jb','--format','{{json .State}}'],text=True));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled']
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()=='81e5ab3105fd21267e864dd11cca694faf0ad0d0'
cmd=['docker','run','--name','nano-depth-catchup-arm64-81e5ab31-20261006','--network','none','--read-only','--tmpfs','/tmp','--memory','4g','--cpus','2','--cap-drop','ALL','--security-opt','no-new-privileges','-v','nano-clean-arm64-build-20261002:/build','-v',str(repo)+':/work:ro','-v','/private/tmp/nano-depth-p2p-build-control-20261006.sh:/control.sh:ro','--entrypoint','bash','sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','/control.sh','/build/coherent-efbeb9bc-arm64/network','/build/rejection-scan-8adbabff-arm64-20261005','/build/depth-catchup-arm64-81e5ab31-20261006','arm64']
subprocess.run(cmd,check=True)
subprocess.run(['python3','/private/tmp/nano-depth-p2p-native-build-20261006.py','candidate'],check=True)
out=Path('/private/tmp/nano-depth-p2p-native-20261006/candidate')
for step in ['compile','link','run']:assert (out/(step+'-exit.txt')).read_text().strip()=='0'
print('Mac Docker and native scoped P2P builds terminal passed',flush=True)
