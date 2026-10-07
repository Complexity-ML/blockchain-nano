from pathlib import Path
import json,subprocess,hashlib,sys
root=Path('/tmp/nano-restored-three-node-vps-muxgxadj');prior=Path('/tmp/nano-restored-three-node-vps-muxfgr38/local-readonly-audit');out=Path('/tmp/nano-v37-runtime-refusal-readonly-20261007');out.mkdir(mode=0o700)
image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-restored-three-node-vps-20261005-data';docker=['sudo','-n','docker']
def run(args):return subprocess.check_output(args,text=True)
state=json.loads(run(docker+['inspect','nano-restored-three-node-vps-muxgxadj','--format','{{json .State}}']));assert not state['Running'] and state['ExitCode']==1 and not state['OOMKilled'];assert not run(docker+['ps','-q','--filter','volume='+volume]).strip()
minimum=json.loads((prior/'capture.json').read_text());assert minimum['readOnlyStores'] and minimum['primaryWitnessAndSignersVerified']
cfg=json.loads((root/'vps.json').read_text());c=cfg['capella'];c['initial']=None;c['automaticRecovery']=False;c['recoveryCommitment']=minimum['restart']
req=json.loads((prior/'request.json').read_text());req['profile']={k:c['profile'][k] for k in ['file','sha256']};req['limits']=c['limits'];req['minimumCheckpoint']=minimum['restart'];req['minimumValidatorKeys']=minimum['validatorKeys'];req['captureBudget']={'blocks':c['limits']['blocks'],'envelopeBytes':c['limits']['envelopeBytes']}
for name,value in [('request.json',req),('config.json',cfg),('state.json',state)]:f=out/name;f.write_text(json.dumps(value,indent=2)+'\n');f.chmod(0o600)
base=docker+['run','--rm','--cpus','1','--memory','2g','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v','nano-clean-rpc-build-20261002:/build:ro','-v',volume+':/var/lib/nano:ro','-v',str(root)+':/config:ro','-v',str(out)+':/audit:ro']
hashcmd=base+['--entrypoint','sh',image,'-c','find /var/lib/nano -type f -exec sha256sum {} + | sort'];before=run(hashcmd);f=out/'data-before.sha256';f.write_text(before);f.chmod(0o600)
args=base+['--entrypoint','/build/published-signed-inventory-amd64-20261007/capture',image,'--capture','/config/genesis.json','/audit/config.json','/var/lib/nano/chain','/audit/request.json']
(out/'capture-command.json').write_text(json.dumps(args)+'\n')
with (out/'capture.json').open('w') as stdout,(out/'error.log').open('w') as stderr:r=subprocess.run(args,stdout=stdout,stderr=stderr)
assert before==run(hashcmd),'Read-only source files changed'
result={'captureExitCode':r.returncode,'files':len(before.splitlines()),'hashesUnchanged':True,'preV37MinimumPins':len(minimum['validatorKeys']),'terminalStopReceiptAvailable':False,'exactTerminalPinsVerified':False,'runtimeFailureExitCode':1,'startupRefusal':False,'runtimeHealthInferred':False,'networkQualified':False,'productionQualified':False,'error':(out/'error.log').read_text()}
if r.returncode==0:
 capsule=json.loads((out/'capture.json').read_text());assert capsule['primaryWitnessAndSignersVerified'] and capsule['canonicalReplay']['canonicalHistoryVerified'];result['currentPrimaryWitnessAndSignerPairsVerified']=True
(out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True);sys.exit(r.returncode)
