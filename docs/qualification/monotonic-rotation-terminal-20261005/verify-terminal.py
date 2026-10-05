import pathlib,json,hashlib,subprocess,shlex
repo=pathlib.Path('/Users/boris/Dev/blockchain-nano');root=pathlib.Path(__file__).parent;out=pathlib.Path('/tmp/nano-monotonic-rotation-20261005')
rows=[json.loads(l) for l in (out/'run.jsonl').read_text().splitlines()];h=next(r for r in rows if r['event']=='harness')
assert (root/'controller-exit.txt').read_text().strip()=='0'
assert rows[-1]['event']=='passed'
assert json.loads((root/'terminal-harness.json').read_text())==h['sha256']
programs=json.loads((root/'preflight.json').read_text())['nativeProgramsRehashed']
for p,d in programs.items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d
verified=[]
for arch,item in zip(['arm64','amd64'],h['artifacts']):
 spec=h['buildArtifacts'][arch];paths=h['buildPaths'][arch];argv=['docker','run','--rm','--network','none','--read-only','--mount',spec['mount'],'--entrypoint','sha256sum',spec['image'],paths['nano'],paths['p2p'],paths['rotationAudit']]
 if arch=='amd64':argv=['/tmp/nano-wan-dual-family-transport-control-20261004/bin/ssh','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166',shlex.join(['sudo','-n']+argv)]
 actual=subprocess.check_output(argv,text=True,timeout=30);assert actual==item['sha256']
 (root/(arch+'-terminal.sha256')).write_text(actual);verified.append(arch)
for n in ['docker-mac-state.json','vps-state.json']:
 s=json.loads((root/n).read_text());assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
audits=json.loads((out/'rotation-audits.json').read_text());assert len(audits)==3
for a in audits:
 assert a['committeeHash']!=a['initialCommitteeHash'] and a['remoteBlocks']>=3 and a['witnessReopened']
 for field in ['committeeHash','finalizedRoot','stateSha256','stateRoot','stateSlot']:assert a[field]==audits[0][field]
stop=[]
for name in ['native.log','docker-mac.log','vps.log']:
 receipts=[json.loads(l) for l in (out/name).read_text().splitlines() if l.startswith('{')]
 receipt=[r for r in receipts if r.get('event')=='capella-runtime-stopped'];assert len(receipt)==1
 r=receipt[0];assert r['clockHealthy'] and r['validator']['healthy'] and r['validator']['attestationsAdmitted']>0 and r['validator']['proposalsAdmitted']>0
 stop.append({'file':name,'finalized':r['finalized'],'proposals':r['validator']['proposalsAdmitted'],'attestations':r['validator']['attestationsAdmitted'],'clockHealthy':True})
recovery=next(r for r in rows if r['event']=='partition-recovery-bound');assert recovery['recoveryElapsedMs']<recovery['recoveryLimitMs']
report={'terminalExit':0,'nativeProgramsUnchanged':len(programs),'harnessFilesUnchanged':len(h['sha256']),'dockerArchitecturesRehashed':verified,'dockerWritersStoppedWithoutOom':True,'auditsAgreeOnReopenedFinalizedStateAndRotatedCommittee':True,'minimumRemoteAggregateBlocks':min(a['remoteBlocks'] for a in audits),'partitionRecoveryMs':recovery['recoveryElapsedMs'],'partitionRecoveryLimitMs':recovery['recoveryLimitMs'],'observations':json.loads((out/'observations/result.json').read_text()),'stoppedWriters':stop,'productionQualified':False}
(root/'terminal-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
