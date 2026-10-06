from pathlib import Path
import json,hashlib
root=Path('/private/tmp/nano-restored-three-node-v8-retention-20261006/campaign');repo=Path('/Users/boris/Dev/blockchain-nano')
def read(p):return json.loads(p.read_text())
def identity(k):
 return ('index',int(k['index'],16)) if k['index'] is not None else ('publicKey',k['publicKey'])
result=read(root/'result.json');native=read(root/'native-state.json');assert native['exitCode']==0 and native['signalCode'] is None
for f in ['docker-mac.log.state.json','vps.log.state.json']:
 state=read(root/f)['State'];assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled']
source=read(repo/'docs/qualification/restored-three-node-v8-retention-prepare-20261006/input.json');plan=read(root/'plan.json');rows=[];seen=set()
for n,(name,source_key) in enumerate([('mac-native','macNativeCapsule'),('mac-docker','macCapsule'),('vps-docker','vpsCapsule')]):
 receipt=read(root/f'{n}-external-stop-receipt.json');capsule=source[source_key];old={identity(k):k['restart'] for k in capsule['validatorKeys']};owned=set(plan['ownership'][name]);pins={identity(k):k['restart'] for k in receipt['validatorKeys']};fixed={i[1] for i in pins if i[0]=='index'};public={i for i in pins if i[0]=='publicKey'};assert fixed==owned-(set([64]) if public else set());assert len(public)==(1 if 64 in owned else 0);assert len(pins)==len(owned) and set(pins).issubset(old) and not seen.intersection(owned);seen.update(owned)
 advances=0
 for index,pin in pins.items():
  before=old[index];assert int(pin['sequence'],16)>=int(before['sequence'],16)
  if pin['sequence']==before['sequence']:assert pin['digest']==before['digest']
  else:advances+=1
 assert int(receipt['restart']['sequence'],16)>=int(capsule['restart']['sequence'],16)
 rows.append({'name':name,'ownedKeys':len(owned),'ownedSignerPinsAdvanced':advances,'clockHealthy':receipt['clockHealthy'],'validatorHealthy':receipt['validator']['healthy'],'validatorFailure':receipt['validator']['failure'],'proposals':receipt['validator']['proposalsAdmitted'],'attestations':receipt['validator']['attestationsAdmitted'],'restart':receipt['restart'],'finalized':receipt['finalized']})
assert len(seen)==65
hashes=read(root/'controller-hashes.json')
for rel,sha in hashes.items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==sha,rel
report={'result':result,'processesCleanlyStopped':True,'controllerFilesUnchanged':True,'ownedSignerPinsCheckedSeparately':65,'nodes':rows,'scope':'Original stop receipts, owner-specific commitments and controller integrity; not a successful finality campaign or storage replay','productionQualified':False}
(root/'terminal-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(rows))
