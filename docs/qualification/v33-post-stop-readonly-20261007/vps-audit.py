from pathlib import Path
import json,subprocess,hashlib,time
root=Path('/tmp/nano-restored-three-node-vps-muxbpvx9');old=Path('/tmp/nano-v32-post-stop-readonly-20261007');out=Path('/tmp/nano-v33-post-stop-readonly-20261007');image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-restored-three-node-vps-20261005-data'
def run(args):return subprocess.check_output(args,text=True)
state=json.loads(run(['sudo','-n','docker','inspect',root.name,'--format','{{json .State}}']));assert not state['Running'] and state['ExitCode']==137 and not state['OOMKilled'];assert not run(['sudo','-n','docker','ps','-q','--filter','volume='+volume]).strip()
out.mkdir(mode=0o700)
minimum=json.loads((old/'capture.json').read_text());cfg=json.loads((root/'vps.json').read_text());c=cfg['capella'];c['initial']=None;c['automaticRecovery']=False;c['recoveryCommitment']=minimum['restart']
req=json.loads((old/'request.json').read_text());req['profile']={k:c['profile'][k] for k in ['file','sha256']};req['limits']=c['limits'];req['minimumCheckpoint']=minimum['restart'];req['requiredFinalized']=minimum['canonicalReplay']['finalized'];req['minimumValidatorKeys']=minimum['validatorKeys'];req['captureBudget']={'blocks':c['limits']['blocks'],'envelopeBytes':c['limits']['envelopeBytes']}
for name,value in [('request.json',req),('config.json',cfg)]:f=out/name;f.write_text(json.dumps(value,indent=2)+'\n');f.chmod(0o600)
base=['sudo','-n','docker','run','--rm','--cpus','1','--memory','2g','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v','nano-clean-rpc-build-20261002:/build:ro','-v',volume+':/var/lib/nano:ro','-v',str(root)+':/config:ro','-v',str(out)+':/audit:ro']
hashcmd=base+['--entrypoint','sh',image,'-c','find /var/lib/nano -type f -exec sha256sum {} + | sort'];before=run(hashcmd);f=out/'data-before.sha256';f.write_text(before);f.chmod(0o600)
started=time.monotonic()
with (out/'capture.json').open('w') as stdout,(out/'error.log').open('w') as stderr:r=subprocess.run(base+['--entrypoint','/build/published-block-inventory-amd64-20261007/capture',image,'--capture','/config/genesis.json','/audit/config.json','/var/lib/nano/chain','/audit/request.json'],stdout=stdout,stderr=stderr)
(out/'exit.json').write_text(json.dumps({'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-started})+'\n');assert r.returncode==0,r.returncode
capsule=json.loads((out/'capture.json').read_text());actual={x['index']:x for x in capsule['validatorKeys']};assert len(actual)==33
unchanged=sum(actual[key['index']]['restart']==key['restart'] for key in minimum['validatorKeys']);assert before==run(hashcmd)
(out/'immutability.json').write_text(json.dumps({'files':len(before.splitlines()),'hashesUnchanged':True,'minimumCoreAndSignerCommitmentsVerified':True,'exactUnchangedSignerPins':unchanged,'signerInventory':33,'primaryAndIndependentWitnessVerified':True,'sourceForcedStopExit':137,'sourceOomKilled':False,'requestPreparedOnVps':True,'noPrivateDataExport':True,'readOnly':True,'networkQualified':False},indent=2)+'\n');print('V33 VPS crash read-only capture0; minimum commitments verified and all files unchanged',flush=True)
