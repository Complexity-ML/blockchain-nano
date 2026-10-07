from pathlib import Path
import json,hashlib,collections,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path(__file__).parent;campaign=Path('/private/tmp/nano-restored-three-node-v34-startup-memo-20261007/campaign');original=json.loads((root/'original-exit.json').read_text())
assert original['session']==14735 and original['exitCode'] in (0,1)
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
result=read(campaign/'result.json');events=[json.loads(l) for l in (campaign/'events.jsonl').read_text().splitlines()];assert events[-1]['event']=='terminal' and events[-1]['result']==result['result'];assert original['exitCode']==(0 if result['result']=='PASSED' else 1)
controllers=read(campaign/'controller-hashes.json');assert controllers=={f:sha(repo/f) for f in controllers}
progress=read(campaign/'distinct-finality-progress.json') if (campaign/'distinct-finality-progress.json').exists() else {'distinctAdvances':[]}
summary={'originalSession':14735,'exitCode':original['exitCode'],'result':result['result'],'controllerUnchanged':True,'newCommonFinalities':len(progress['distinctAdvances']),'rawLogs':{},'duties':[],'relayCompletedTurns':{},'productionQualified':False}
for i,(node,log) in enumerate(zip(['mac-native','mac-docker','vps-docker'],['native.log','docker-mac.log','vps.log'])):
 f=campaign/log
 if f.exists():
  summary['rawLogs'][node]={'path':str(f),'sha256':sha(f),'bytes':f.stat().st_size};turns=[];failures=collections.Counter();stages=[]
  for l in f.read_text().splitlines():
   try:x=json.loads(l)
   except ValueError:continue
   if x.get('event')=='test-capella-catalog-relay':turns.append(x)
   if x.get('event')=='test-core-rpc-failure':failures[(x.get('method'),x.get('cause'))]+=1
   if x.get('event')=='capella-startup-stage':stages.append(x)
  blocks=[x for x in turns if x['kind']=='block'];deps=[x['dependencyProgress'] for x in blocks if 'dependencyProgress' in x]
  summary['relayCompletedTurns'][node]={'kinds':dict(collections.Counter(x['kind'] for x in turns)),'blockReasons':dict(collections.Counter(x.get('reason') or 'none' for x in blocks)),'dependencyExports':sum(x['exportsRequested'] for x in deps),'dependencyACKs':sum(x['payloadsAdmitted'] for x in deps),'dependencySendAttempts':sum(x['sendAttempts'] for x in deps),'selectedHeadACKs':sum(bool(x.get('selectedHeadAcknowledged')) for x in blocks),'rpcRefusals':[{'method':k[0],'cause':k[1],'count':v} for k,v in failures.items()],'startupStages':stages,'scope':'completed turns only; independent catalogue deliveries excluded from dependency counts'}
 receipt=campaign/(str(i)+'-external-stop-receipt.json')
 if receipt.exists():
  x=read(receipt);v=x['validator'];summary['duties'].append({'name':node,'clockHealthy':x['clockHealthy'],'finalized':x['finalized'],'validator':{k:v[k] for k in ['healthy','localKeys','proposalsAdmitted','attestationsAdmitted','syncMessagesAdmitted','timing'] if k in v}})
summary['stops']={f:read(campaign/f) for f in ['native-state.json','docker-mac.log.state.json','vps.log.state.json'] if (campaign/f).exists()}
if result['result']=='PASSED':
 assert summary['newCommonFinalities']>=3 and len(summary['duties'])==3
 for d in summary['duties']:assert d['clockHealthy'] and d['validator']['healthy'] and d['validator']['proposalsAdmitted']>0 and d['validator']['attestationsAdmitted']>0
 s=summary['stops']['native-state.json'];assert s['exitCode']==0 and s['signalCode'] is None
 for f in ['docker-mac.log.state.json','vps.log.state.json']:
  s=summary['stops'][f]['State'];assert s['ExitCode']==0 and not s['Running'] and not s['OOMKilled']
proof=repo/'docs/qualification/restored-three-node-v34-terminal-20261007';proof.mkdir()
for f in ['result.json','events.jsonl','controller-hashes.json','artifacts.json','distinct-finality-progress.json','native-state.json','docker-mac.log.state.json','vps.log.state.json','vps-local-preparation-proof.json','canonical-blocks.json']+[str(i)+'-external-stop-receipt.json' for i in range(3)]:
 if (campaign/f).exists():shutil.copy2(campaign/f,proof/f)
for f in ['original-exit.json','archive-terminal.py']:shutil.copy2(root/f,proof/f)
(proof/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(proof/'README.md').write_text('# V34 — restored three-node startup memo\n\nOriginal controller14735 terminal'+str(original['exitCode'])+', result **'+result['result']+'**. '+str(summary['newCommonFinalities'])+' distinct new common checkpoints. Core c4996498; relay77848f1f; checkout d515914d. Existing controller/limits/900-second window/signing guards retained. Full startup/terminal receipt metrics and complete raw log hashes in summary.json; no image or binary rebuild during campaign.\n\nDependency ACKs are not selected-head admission or finality. Any failure and prior V33/V32 failures remain preserved. A separate diagnostic bound correction is drafted but was not compiled or injected. This result does not qualify all partitions/rotations/lifecycle/sanctions/resources, production, EIP forks, bridge/native linkage, Doppler or independent security review. Fresh read-only replay audits against exact terminal commitments are required before another writer.\n')
(proof/'manifest.json').write_text(json.dumps({'controllerCheckout':'d515914d8484e837cc5f66a32f43f530c1a85a30','coreRevision':'c499649857b88a39df3d7badb97156daf96eb2c6','relayRevision':'77848f1fa52fa2c44ae622e0809a74501616e28a','productionQualified':False,'files':{str(p.relative_to(proof)):sha(p) for p in sorted(proof.rglob('*')) if p.is_file()}},indent=2)+'\n')
print(json.dumps({'result':result['result'],'newCommonFinalities':summary['newCommonFinalities'],'stopReceipts':len(summary['duties']),'proof':str(proof)}))
