
from pathlib import Path as _AuditPath
import json as _AuditJson
_terminal=_AuditJson.loads(_AuditPath('/private/tmp/nano-restored-three-node-v9-dispatch-20261006/campaign/terminal-exit.json').read_text())
assert _terminal['originalSession']==37036, 'Missing original controller terminal evidence'
import json,copy,subprocess,hashlib,time
from pathlib import Path
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-restored-three-node-v9-dispatch-20261006');out=Path('/private/tmp/nano-v9-dispatch-post-stop-native-diagnostic-20261006');out.mkdir(exist_ok=True)
source=json.loads((repo/'docs/qualification/restored-three-node-v9-dispatch-prepare-20261006/input.json').read_text())
def read(p):return json.loads(p.read_text())
def save(p,d):p.write_text(json.dumps(d,indent=2)+'\n')
def rebase(v,data):
 if isinstance(v,dict):return {k:rebase(x,data) for k,x in v.items()}
 if isinstance(v,list):return [rebase(x,data) for x in v]
 if isinstance(v,str):
  for pre,post in [('/config/',str(root/'config')+'/'),('/var/lib/nano/',str(data)+'/')]:
   if v.startswith(pre):return post+v[len(pre):]
 return v
assert hashlib.sha256(Path('/private/tmp/nano-atomic-proposal-native-20261006/nano-capella-paused-capture').read_bytes()).hexdigest()=='2b4672397a5c46d70a2baeb19416963c9176675d7cbb5d00b73ed96640e5880f'
need=root/'campaign/result.json';assert need.exists(),'Wait for original controller terminal result before readonly audit'
native_state=read(root/'campaign/native-state.json')
assert native_state['exitCode']==0 and native_state['signalCode'] is None
processes=subprocess.check_output(['ps','-axo','pid,command'],text=True)
assert not any(line.strip().split(maxsplit=1)[0]==str(native_state['pid']) for line in processes.splitlines() if line.strip()),'Original native writer still active'
resources=read(root/'campaign/resources.json')
docker_state=json.loads(subprocess.check_output(['docker','inspect',resources['localContainer'],'--format','{{json .State}}'],text=True))
assert not docker_state['Running'] and docker_state['ExitCode']==0 and not docker_state['OOMKilled'],'Original Mac Docker writer not cleanly stopped'
assert read(root/'campaign/result.json')['stopReceipts']==3,'Missing original owner stop receipt'

for name,n in [('mac-native',0),('mac-docker',1)]:
 receipt=read(root/f'campaign/{n}-external-stop-receipt.json');data=root/name/'data';capsule=source['macNativeCapsule' if n==0 else 'macCapsule'];config=rebase(copy.deepcopy(source['macNativeConfig' if n==0 else 'macConfig']),data);config['capella']['recoveryCommitment']=receipt['restart'];config['capella']['limits']=copy.deepcopy(read(root/'campaign/plan.json')['configs'][n]['capella']['limits']);assert config['capella']['limits']['blocks']==source['macConfig']['capella']['limits']['blocks'];pins=copy.deepcopy(capsule['validatorKeys']);by={k['index']:k for k in receipt['validatorKeys']}
 for i,k in enumerate(pins):
  if k['index'] in by:pins[i]=by[k['index']]
 for k in config['capella']['validator']['keys']:k['restart']=next(p['restart'] for p in pins if p['index']==k['index'])
 request={'profile':{k:config['capella']['profile'][k] for k in ['file','sha256']},'anchor':{'file':str(root/'config/initial.json'),'sha256':'0x780dd0ba2cb38f95f2c0e24c9ee15887c19f974a9838679ca99acaae923e239a'},'limits':config['capella']['limits'],'maximumBlocks':'0x400','maximumArchiveBytes':'0x20000000','requiredFinalized':source['finalized'],'minimumCheckpoint':receipt['restart'],'minimumValidatorKeys':pins,'minimumAttestationTarget':'0x4c','participationDiagnostics':True}
 save(out/f'{name}-config.json',config);save(out/f'{name}-request.json',request)
 def hashes():return {str(p.relative_to(data)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(data.rglob('*')) if p.is_file()}
 before=hashes();save(out/f'{name}-before.json',before)
 with (out/f'{name}-capture.json').open('w') as stdout,(out/f'{name}-error.log').open('w') as stderr:r=subprocess.run(['/private/tmp/nano-atomic-proposal-native-20261006/nano-capella-paused-capture','--capture',str(root/'config/genesis.json'),str(out/f'{name}-config.json'),str(data/'chain'),str(out/f'{name}-request.json')],stdout=stdout,stderr=stderr)
 save(out/f'{name}-exit.json',{'exitCode':r.returncode});assert r.returncode==0
 assert hashes()==before;save(out/f'{name}-immutability.json',{'files':len(before),'hashesUnchanged':True})
 d=read(out/f'{name}-capture.json');assert d['restart']==receipt['restart'];assert {k['index']:k['restart'] for k in d['validatorKeys']}=={k['index']:k['restart'] for k in pins};diag=d['participationDiagnostics'];print(name,json.dumps({k:diag[k] for k in ['headSlot','stateEpoch','previousTimelyTargetCountsByOwner','currentTimelyTargetCountsByOwner']}),flush=True)
