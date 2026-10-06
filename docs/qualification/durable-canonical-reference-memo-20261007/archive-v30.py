from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');src=Path('/private/tmp/nano-restored-three-node-v30-cold-20261006/campaign');out=repo/'docs/qualification/restored-three-node-v30-terminal-20261007';out.mkdir(parents=True,exist_ok=True)
hashes=json.loads((src/'controller-hashes.json').read_text());assert all(hashlib.sha256((repo/k).read_bytes()).hexdigest()==v for k,v in hashes.items())
result=json.loads((src/'result.json').read_text());assert result['result']=='FAILED' and result['stopReceipts']==3 and result['commonFinality'] is None
rows=[]
for i in range(3):
 receipt=json.loads((src/f'{i}-external-stop-receipt.json').read_text());v=receipt['validator'];assert receipt['clockHealthy'] and v['healthy'] and v['proposalsAdmitted']>0 and v['attestationsAdmitted']>0 and v['localKeys']==[16,16,33][i]
 rows.append({'name':['mac-native','mac-docker','vps-docker'][i],'clockHealthy':receipt['clockHealthy'],'finalized':receipt['finalized'],'validator':{k:v[k] for k in ['healthy','localKeys','proposalsAdmitted','attestationsAdmitted','syncMessagesAdmitted','timing']}})
 shutil.copy2(src/f'{i}-external-stop-receipt.json',out/f'{i}-external-stop-receipt.json')
for n in ['result.json','controller-hashes.json','artifacts.json','distinct-finality-progress.json','native-state.json','docker-mac.log.state.json','vps.log.state.json','events.jsonl']:
 shutil.copy2(src/n,out/n)
for n in ['docker-mac.log.state.json','vps.log.state.json']:
 s=json.loads((src/n).read_text())['State'];assert s['ExitCode']==0 and not s['Running'] and not s['OOMKilled']
s=json.loads((src/'native-state.json').read_text());assert s['exitCode']==0 and s['signalCode'] is None
(out/'summary.json').write_text(json.dumps({'originalSession':82206,'exitCode':1,'controllerUnchanged':True,'newCommonFinalities':0,'duties':rows,'rawLogs':{str(src/n):hashlib.sha256((src/n).read_bytes()).hexdigest() for n in ['native.log','docker-mac.log','vps.log']},'freshPostStopIndependentAuditRequired':True},indent=2)+'\n')
(out/'README.md').write_text('''# V30 terminal — 7 October 2026

Original controller session82206 exited1 at the prescribed900-second finality deadline. All three readiness checks had passed at22:01:56UTC. No restart, replacement binary, timeout extension or controller change occurred. All controller hashes still match. The run restored the prior inventories with explicit source budgets1k/1k/4k and target4k; the earlier preparation proof remains separate.

All three owners actually resumed duties: native16 keys/1 proposal/8 attestations; Docker Mac16/2/8; Docker VPS33/3/7. Stop receipts report healthy clocks and validators. Native exit0, both containers exited0/noOOM. This is positive bounded evidence for duty participation on the restored data, not successful network recovery.

The finalized checkpoint stayed at epoch1284/root0xccf88d18b2b98689ce47814d966ab42d10898eddd83a31bb2e3873ac5b2a2c48. Zero of the required three distinct advancing common checkpoints were observed. Different advancing heads do not satisfy this gate. Max validator poll was17392/24432/67874ms, compared with the configured short-slot workload; no claim of a sole cause follows from these counters.

The native stack sample and a separate regression identify a full cold decode in final durable canonical serialization despite the existing preflight memo. Its correction is qualified separately, never injected into V30. Further catch-up/transport/signed-operation costs may remain. Historical signed-vote contexts are not rewritten or erased to obtain convergence.

Public receipts, original events, states and artifact identities are retained here; raw local logs are preserved at the paths/hashes in summary.json. Private databases, keys, witnesses and full private requests remain on their host. New post-stop independent capture/pin audits are required before any further writer because this run signed and published new data. Neither old V28 pins nor pre-V30 captures can be silently reused.

All remaining production, EVM-profile, bridge/reserve/redemption, Doppler and independent-security-review requirements remain open. No launch, funds or public deployment.
''')
(out/'manifest.json').write_text(json.dumps({str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.is_file() and p.name!='manifest.json'},indent=2)+'\n');print('V30 terminal independently checked, duties positive, finality FAILED')
