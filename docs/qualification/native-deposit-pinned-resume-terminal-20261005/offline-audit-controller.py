# Run only AFTER the exact campaign is terminal. Never restarts network nodes.
import pathlib,json,subprocess,os,hashlib,sys
out=pathlib.Path('/private/tmp/nano-deposit-resume-v2-20261005');control=pathlib.Path('/private/tmp/nano-deposit-resume-v2-control-20261005');dest=pathlib.Path('/private/tmp/nano-deposit-resume-v2-offline-audits-20261005')
assert (control/'controller-exit.txt').exists(),'Campaign not terminal; do not run offline audit'
assert (control/'controller-exit.txt').read_text().strip()=='0','Keep campaign failure; audit separately after diagnosis'
assert json.loads((out/'result.json').read_text())['result']=='PASSED'
assert not dest.exists(),'Do not overwrite audit evidence';dest.mkdir()
resources=json.loads((out/'resources.json').read_text());artifacts=json.loads((out/'artifacts.json').read_text());common=json.loads((out/'common-finality.json').read_text());events=[json.loads(l) for l in (out/'events.jsonl').read_text().splitlines()]
cutoff=common['restartCutoffSlot']
original=json.loads((out/'resume-source.json').read_text());seedconfig=json.loads((pathlib.Path(original['sourceOutput'])/'fixture/node-0.json').read_text())['capella']['initial']
assert not json.loads((out/'container-state.json').read_text())['State']['Running']
env=dict(os.environ,PATH='/tmp/nano-wan-dual-family-transport-control-20261004/bin:'+os.environ['PATH'])
def execute(cmd,stem):
 with (dest/(stem+'.stdout')).open('w') as stdout,(dest/(stem+'.stderr')).open('w') as stderr:r=subprocess.run(cmd,env=env,stdout=stdout,stderr=stderr)
 (dest/(stem+'-exit.txt')).write_text(str(r.returncode)+'\n');return r.returncode
try:
 os.kill(resources['nativePid'],0)
except ProcessLookupError:
 pass
else:
 raise RuntimeError('Original native process still exists; stopped audit prohibited')
tools=json.loads(pathlib.Path('/private/tmp/nano-deposit-audit-tools-v7-v5-20261005.json').read_text())
assert hashlib.sha256(pathlib.Path(tools['native']['path']).read_bytes()).hexdigest()==tools['native']['sha256'],'Native auditor binary changed'
import shlex
inventory=['sudo','-n','docker','run','--rm','--network','none','--read-only','--mount',artifacts['mount'],'--entrypoint','sha256sum',artifacts['vps']['image'],tools['vps']['path']]
assert execute(['ssh','-o','BatchMode=yes',resources['target'],shlex.join(inventory)],'vps-tool-hash')==0
assert (dest/'vps-tool-hash.stdout').read_text().strip()==tools['vps']['sha256']+'  '+tools['vps']['path'],'VPS auditor binary changed'
(dest/'tools.json').write_text(json.dumps(tools,indent=2)+'\n')
for n,name in enumerate(['native.log','vps.log']):
 stops=[]
 for line in (out/name).read_text().splitlines():
  try:e=json.loads(line)
  except ValueError:continue
  if e.get('event')=='capella-runtime-stopped':stops.append(e)
 stop=stops[-1];config=json.loads((out/'mac.json' if n==0 else out/'fixture/node-1.json').read_text())['capella']
 assert stop['clockHealthy'] and stop['validator']['healthy']
 req={'profile':{k:config['profile'][k] for k in ['file','sha256']},'anchor':{'file':str(out/'fixture/initial.json') if n==0 else '/config/initial.json','sha256':seedconfig['sha256']},'limits':config['limits'],'expectedCheckpoint':stop['restart'],'requiredFinalized':common['afterRestart'],'maximumBlocks':'0x400','maximumArchiveBytes':'0x20000000','validatorKeys':stop['validatorKeys'],'restartCutoffSlot':cutoff,'proposalBefore':common['proposalBefore'],'proposalAfter':common['proposalAfter']}
 p=dest/(str(n)+'-request.json');p.write_text(json.dumps(req,indent=2)+'\n');(dest/(str(n)+'-external-stop-receipt.json')).write_text(json.dumps(stop,indent=2)+'\n')
 if n==0:
  cmd=['/tmp/nano-deposit-audit-native-build-v7-20261005/nano-capella-deposit-audit','--audit',str(out/'fixture/genesis.json'),str(out/'mac.json'),str(out/'mac-data/chain'),str(p),str(out/'fixture/deposit.json')]
 else:
  assert execute(['scp','-q',str(p),resources['target']+':'+resources['remoteRoot']+'/deposit-audit-request.json'],'request-transfer')==0
  remote=['sudo','-n','docker','run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount',artifacts['mount'],'--mount','type=volume,src='+resources['volume']+',dst=/var/lib/nano,readonly','--mount','type=bind,src='+resources['remoteRoot']+'/config,dst=/config,readonly','--mount','type=bind,src='+resources['remoteRoot']+'/deposit-audit-request.json,dst=/request.json,readonly','--entrypoint','/build/deposit-audit-amd64-v5-20261005/nano-capella-deposit-audit',artifacts['vps']['image'],'--audit','/config/genesis.json','/config/node-1.json','/var/lib/nano/chain','/request.json','/config/deposit.json']
  import shlex
  cmd=['ssh','-o','BatchMode=yes',resources['target'],shlex.join(remote)]
 code=execute(cmd,['native-audit','vps-audit'][n]);assert code==0,'Offline audit failed; retain terminal output'
 audit=json.loads((dest/(['native-audit','vps-audit'][n]+'.stdout')).read_text());assert audit['canonicalRegistrationTransitions']==1 and audit['canonicalDepositInclusions']==1 and audit['canonicalSignedDepositTransactions']==1 and audit['perKeyJournalWitnessAuditQualified'] and audit['externalStopTokenAndWitnessVerified']
 assert len(audit['localSigningCommitments'])==(32 if n==0 else 33)
(dest/'result.json').write_text(json.dumps({'bothReadOnlyAuditsPassed':True,'externalStopPinsAndWitnessesVerified':True,'localSigningInventories':[32,33],'requiredFinalized':common['afterRestart'],'productionQualified':False},indent=2)+'\n')
