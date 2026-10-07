from pathlib import Path
import subprocess,json,hashlib,sys
out=Path('/tmp/nano-v37-offline-recovery-copy-20261007');logs=out/'recovery-run';logs.mkdir(mode=0o700)
docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';source='nano-restored-three-node-vps-20261005-data';candidate='/build/publication-cancel-boundary-amd64-20261007'
def output(args):return subprocess.check_output(args,text=True)
def stopped():assert not output(docker+['ps','-q','--filter','volume='+source]).strip()
stopped();assert json.loads((out/'copy-proof.json').read_text())['independentCopyVerified']
source_base=docker+['run','--rm','--network','none','--read-only','-v',source+':/source:ro','--entrypoint','sh',image,'-c','find /source -type f -exec sha256sum {} + | sort']
source_before=output(source_base);assert source_before==(out/'source-before.sha256').read_text()
base=docker+['run','--rm','--cpus','1','--memory','4g','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v','nano-clean-rpc-build-20261002:/build:ro','-v','/tmp/nano-restored-three-node-vps-muxgxadj:/config:ro','-v',str(out)+':/recovery']
def run(name,program,args,expected=0):
 argv=base+['--entrypoint',program,image]+args;(logs/(name+'-command.json')).write_text(json.dumps(argv)+'\n')
 with (logs/(name+'.json')).open('w') as stdout,(logs/(name+'-error.log')).open('w') as stderr:p=subprocess.run(argv,stdout=stdout,stderr=stderr)
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode,'expectedExit':expected})+'\n');print(name,p.returncode,flush=True);assert p.returncode==expected,(name,p.returncode)
# Explicit mode must be supplied before the helper even reads a marker or DB.
run('mode-guard',candidate+'/recover-synthetic-copy',[],1)
assert 'requires explicit mode' in (logs/'mode-guard-error.log').read_text()
copy_before=(out/'copy-before.sha256').read_text();
def protected(text):return '\n'.join(line for line in text.splitlines() if not line.split('  ',1)[1].startswith(('/recovery/data/chain/','/recovery/data/witness/')))
run('recovery',candidate+'/recover-synthetic-copy',['--recover-synthetic-copy'])
recovered=json.loads((logs/'recovery.json').read_text());assert recovered['afterPrimary']==recovered['witnessNext'];assert recovered['beforePrimary']!=recovered['afterPrimary']
# Idempotent reopen must preserve the same logical checkpoint, not sign again.
run('idempotent-reopen',candidate+'/recover-synthetic-copy',['--recover-synthetic-copy']);again=json.loads((logs/'idempotent-reopen.json').read_text());assert again['beforePrimary']==again['afterPrimary']==recovered['afterPrimary']
# Full canonical replay and independent signer/witness checks remain read-only.
run('capture',candidate+'/capture',['--capture','/config/genesis.json','/recovery/config.json','/recovery/data/chain','/recovery/request.json'])
capsule=json.loads((logs/'capture.json').read_text());assert capsule['restart']==recovered['afterPrimary'];assert capsule['primaryWitnessAndSignersVerified'] and capsule['canonicalReplay']['canonicalHistoryVerified'];assert len(capsule['validatorKeys'])==33
copy_after=output(base+['--entrypoint','sh',image,'-c','find /recovery/data -type f -exec sha256sum {} + | sort']);(out/'copy-after.sha256').write_text(copy_after);assert protected(copy_before)==protected(copy_after),'Signer/peer files changed during offline recovery'
assert source_before==output(source_base),'Original source changed';stopped()
proof={'result':'PASSED_OFFLINE_COPY_ONLY','sourceOriginalHashesUnchanged':True,'sourceFiles':len(source_before.splitlines()),'allSignerAndPeerFilesUnchanged':True,'primaryWitnessIntentRecovered':True,'idempotentReopenCheckpointUnchanged':True,'current33SignerWitnessPairsVerified':True,'canonicalReplayVerified':True,'externalPreV37MinimumsVerified':True,'terminalV37ExternalPinsAvailable':False,'runtimeDutiesResumed':False,'networkQualified':False,'productionQualified':False,'captureSha256':hashlib.sha256((logs/'capture.json').read_bytes()).hexdigest()}
(out/'recovery-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof),flush=True)
