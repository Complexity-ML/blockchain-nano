
from pathlib import Path as _AuditPath
import json as _AuditJson
_terminal=_AuditJson.loads(_AuditPath('/private/tmp/nano-restored-three-node-v14-head-fallback-20261006/campaign/terminal-exit.json').read_text())
assert _terminal['originalSession']==40209 and _terminal.get('exitCode') in [0,1], 'Missing original controller terminal evidence'
import json,subprocess,shlex,copy,hashlib
from pathlib import Path
root=Path('/private/tmp/nano-restored-three-node-v14-head-fallback-20261006');out=Path('/private/tmp/nano-post-v14-head-fallback-vps-audit-20261006');out.mkdir(exist_ok=True)
assert (root/'campaign/result.json').exists(),'Original campaign not terminal'
repo=Path('/Users/boris/Dev/blockchain-nano');source=json.loads((repo/'docs/qualification/restored-three-node-v14-head-fallback-prepare-20261006/input.json').read_text()); resources=json.loads((root/'campaign/resources.json').read_text());receipt=json.loads((root/'campaign/2-external-stop-receipt.json').read_text());a=json.loads((root/'campaign/artifacts.json').read_text())['nodes']['vps-docker']
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
def ssh(args):return subprocess.check_output(['/usr/bin/ssh','-4','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(args)],text=True)
state=json.loads(ssh(['sudo','-n','docker','inspect',resources['remoteContainer'],'--format','{{json .State}}']));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled']
assert not ssh(['sudo','-n','docker','ps','-q','--filter','volume='+resources['vpsVolume']]).strip()
config=copy.deepcopy(source['vpsConfig']);c=config['capella'];c['limits']=copy.deepcopy(json.loads((root/'campaign/plan.json').read_text())['configs'][2]['capella']['limits']);assert c['limits']['blocks']==source['macConfig']['capella']['limits']['blocks'];c['recoveryCommitment']=receipt['restart'];c['profile']['file']='/config/profile.json';c['automaticRecovery']=False;c['initial']=None
for key in c['validator']['keys']:
 pin=next(x for x in receipt['validatorKeys'] if x['index']==key['index']);key['restart']=pin['restart']
request={'profile':{k:c['profile'][k] for k in ['file','sha256']},'anchor':{'file':'/config/initial.json','sha256':'0x780dd0ba2cb38f95f2c0e24c9ee15887c19f974a9838679ca99acaae923e239a'},'limits':c['limits'],'maximumBlocks':'0x400','maximumArchiveBytes':'0x20000000','requiredFinalized':receipt['finalized'],'minimumCheckpoint':receipt['restart'],'minimumValidatorKeys':receipt['validatorKeys'],'minimumAttestationTarget':'0x4c'}
save(out/'config.json',config);save(out/'request.json',request);save(out/'source-stopped-state.json',state)
for f in ['config.json','request.json']:subprocess.run(['/usr/bin/scp','-4',str(out/f),'ubuntu@145.239.199.166:'+resources['remoteRoot']+'/post-audit-'+f],check=True)
base=['sudo','-n','docker','run','--rm','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',a['buildVolume']+':/build:ro','-v',resources['vpsVolume']+':/var/lib/nano:ro','-v',resources['remoteRoot']+':/config:ro']
hashcmd=base+['--entrypoint','sh',a['image'],'-c','find /var/lib/nano -type f -exec sha256sum {} + | sort']
program=ssh(base+['--entrypoint','sha256sum',a['image'],'/build/parent-cache-amd64-20261006/nano-capella-paused-capture']);assert program.strip().split()[0]=='42ad40d6da1370e9b4afebcbb65a85777e7fe8147d37b6c85f3af990afc2a8d3';save(out/'program-hash.json',{'sha256':program.strip().split()[0]})
before=ssh(hashcmd);(out/'data-before.sha256').write_text(before)
cmd=base+['--entrypoint','/build/parent-cache-amd64-20261006/nano-capella-paused-capture',a['image'],'--capture','/config/genesis.json','/config/post-audit-config.json','/var/lib/nano/chain','/config/post-audit-request.json']
with (out/'capture.json').open('w') as stdout,(out/'error.log').open('w') as stderr:r=subprocess.run(['/usr/bin/ssh','-4','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(cmd)],stdout=stdout,stderr=stderr)
save(out/'exit.json',{'exitCode':r.returncode});assert r.returncode==0
assert before==ssh(hashcmd);d=json.loads((out/'capture.json').read_text());assert d['restart']==receipt['restart'];assert {x['index']:x['restart'] for x in d['validatorKeys']}=={x['index']:x['restart'] for x in receipt['validatorKeys']}
save(out/'immutability.json',{'files':len(before.splitlines()),'hashesUnchanged':True,'exactStopCommitmentsVerified':True,'macDatabaseTransferred':False,'productionQualified':False})
print('VPS readonly audit PASSED',len(before.splitlines()),'unchanged files',flush=True)
