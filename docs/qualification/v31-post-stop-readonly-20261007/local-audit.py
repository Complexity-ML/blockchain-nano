from pathlib import Path
import json,hashlib,subprocess
root=Path('/private/tmp/nano-restored-three-node-v31-canonical-memo-20261007');oldroot=Path('/private/tmp/nano-restored-three-node-v30-cold-20261006');old=Path('/private/tmp/nano-v30-post-stop-readonly-20261007');out=Path(__file__).parent;docker='/Users/boris/.docker/bin/docker';image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
def hashes(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}
def remap(value):
 if isinstance(value,str):return value.replace(str(oldroot),str(root))
 if isinstance(value,list):return [remap(x) for x in value]
 if isinstance(value,dict):return {k:remap(v) for k,v in value.items()}
 return value
for i,(node,label) in enumerate([('mac-native','native'),('mac-docker','mac-docker')]):
 p=out/label;p.mkdir(mode=0o700);data=root/node/'data'
 if i==0:
  s=json.loads((root/'campaign/native-state.json').read_text());assert s['exitCode']==0 and s['signalCode'] is None
 else:
  s=json.loads(subprocess.check_output([docker,'inspect','nano-restored-three-node-mac-mux94cau','--format','{{json .State}}'],text=True));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
 receipt=json.loads((root/f'campaign/{i}-external-stop-receipt.json').read_text());active={x['index']:x for x in receipt['validatorKeys']};prior=json.loads((old/label/'capture.json').read_text());inactive={x['index']:x for x in prior['validatorKeys'] if x['index'] not in active}
 cfg=remap(json.loads((old/label/'config.json').read_text()));current=json.loads((root/f'config/{node}.json').read_text());c=cfg['capella'];c['limits']=current['capella']['limits'];c['initial']=None;c['automaticRecovery']=False;c['recoveryCommitment']=receipt['restart'];c['profile']=current['capella']['profile'];c['profile']['file']=str(root/'config/profile.json')
 req=remap(json.loads((old/label/'request.json').read_text()));req['profile']={k:c['profile'][k] for k in ['file','sha256']};req['limits']=c['limits'];req['minimumCheckpoint']=receipt['restart'];req['requiredFinalized']=receipt['finalized'];req['minimumValidatorKeys']=[active.get(x['index'],x) for x in req['minimumValidatorKeys']];req['captureBudget']={'blocks':c['limits']['blocks'],'envelopeBytes':c['limits']['envelopeBytes']}
 for name,value in [('request.json',req),('config.json',cfg)]:f=p/name;f.write_text(json.dumps(value,indent=2)+'\n');f.chmod(0o600)
 before=hashes(data);f=p/'data-before.json';f.write_text(json.dumps(before)+'\n');f.chmod(0o600)
 if i==0:args=['/private/tmp/nano-durable-canonical-memo-20261007/native-build/capture','--capture',str(root/'config/genesis.json'),str(p/'config.json'),str(data/'chain'),str(p/'request.json')]
 else:
  args=[docker,'run','--rm','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v','nano-clean-arm64-build-20261002:/build:ro']
  for path in [root,old,out]:args+=['-v',str(path)+':'+str(path)+':ro']
  args+=['--entrypoint','/build/durable-canonical-memo-arm64-20261007/capture',image,'--capture',str(root/'config/genesis.json'),str(p/'config.json'),str(data/'chain'),str(p/'request.json')]
 with (p/'capture.json').open('w') as stdout,(p/'error.log').open('w') as stderr:r=subprocess.run(args,stdout=stdout,stderr=stderr)
 (p/'exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,(node,r.returncode)
 capsule=json.loads((p/'capture.json').read_text());assert capsule['restart']==receipt['restart'];observed={x['index']:x for x in capsule['validatorKeys']}
 for index,key in active.items():assert observed[index]['restart']==key['restart']
 for index,key in inactive.items():assert observed[index]['restart']==key['restart']
 assert hashes(data)==before
 (p/'immutability.json').write_text(json.dumps({'files':len(before),'hashesUnchanged':True,'exactStopCommitmentsVerified':True,'activePinsVerified':len(active),'inactiveHistoricalPinsUnchanged':len(inactive),'primaryAndIndependentWitnessVerified':True,'readOnly':True,'networkQualified':False},indent=2)+'\n');print(node,'read-only capture0, active/inactive pins separately exact, files unchanged',flush=True)
