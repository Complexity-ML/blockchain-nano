from pathlib import Path
import json,hashlib,subprocess,shutil,os
source=Path('/tmp/nano-restored-three-node-vps-muxbpvx9');audit=Path('/tmp/nano-v33-post-stop-readonly-20261007');dest=source/'local-readonly-audit';volume='nano-restored-three-node-vps-20261005-data';image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
assert not subprocess.check_output(['sudo','-n','docker','ps','-q','--filter','volume='+volume],text=True).strip()
state=json.loads(subprocess.check_output(['sudo','-n','docker','inspect',source.name,'--format','{{json .State}}'],text=True));assert not state['Running'] and state['ExitCode']==137 and not state['OOMKilled']
assert json.loads((audit/'exit.json').read_text())['exitCode']==0
cmd=['sudo','-n','docker','run','--rm','--network','none','--read-only','-v',volume+':/var/lib/nano:ro','--entrypoint','sh',image,'-c','find /var/lib/nano -type f -exec sha256sum {} + | sort'];assert subprocess.check_output(cmd,text=True)==(audit/'data-before.sha256').read_text()
dest.mkdir(mode=0o700)
for n in ['config.json','request.json','capture.json','immutability.json']:shutil.copy2(audit/n,dest/n);os.chmod(dest/n,0o600)
(dest/'request-config-hashes.json').write_text(json.dumps({n:hashlib.sha256((dest/n).read_bytes()).hexdigest() for n in ['config.json','request.json']})+'\n');os.chmod(dest/'request-config-hashes.json',0o600)
print(json.dumps({'sourceStopped137NoOom':True,'auditExit0':True,'fileHashesReverified':len((audit/'data-before.sha256').read_text().splitlines()),'localReferencesBound':True,'privateDataExported':False}))
