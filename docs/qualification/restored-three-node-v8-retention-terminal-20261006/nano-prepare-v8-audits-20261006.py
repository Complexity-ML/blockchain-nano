from pathlib import Path
for old,new in [('nano-v7-terminal-verification-20261006.py','nano-v8-retention-terminal-verification-20261006.py'),('nano-v7-post-stop-native-diagnostic-20261006.py','nano-v8-retention-post-stop-native-diagnostic-20261006.py'),('nano-post-v7-vps-audit-20261006.py','nano-post-v8-retention-vps-audit-20261006.py')]:
 s=Path('/private/tmp',old).read_text().replace('nano-restored-three-node-v7-20261006','nano-restored-three-node-v8-retention-20261006').replace('restored-three-node-v7-prepare-20261006','restored-three-node-v8-retention-prepare-20261006').replace('nano-v7-post-stop-native-diagnostic-20261006','nano-v8-retention-post-stop-native-diagnostic-20261006').replace('nano-post-v7-vps-audit-20261006','nano-post-v8-retention-vps-audit-20261006')
 if 'post-stop-native' in old:
  a="config['capella']['recoveryCommitment']=receipt['restart'];pins="
  b="config['capella']['recoveryCommitment']=receipt['restart'];config['capella']['limits']=copy.deepcopy(read(root/'campaign/plan.json')['configs'][n]['capella']['limits']);assert config['capella']['limits']['blocks']==source['retentionExperiment']['blocks'];pins="
  assert a in s;s=s.replace(a,b)
  s=s.replace('/private/tmp/nano-deposit-deadline-20261005/nano-capella-paused-capture','/private/tmp/nano-atomic-proposal-native-20261006/nano-capella-paused-capture')
  a="need=root/'campaign/result.json';"
  b="assert hashlib.sha256(Path('/private/tmp/nano-atomic-proposal-native-20261006/nano-capella-paused-capture').read_bytes()).hexdigest()=='2b4672397a5c46d70a2baeb19416963c9176675d7cbb5d00b73ed96640e5880f'\n"+a
  s=s.replace(a,b)
 if 'vps-audit' in old:
  a="c=config['capella'];c['recoveryCommitment']=receipt['restart'];"
  b="c=config['capella'];c['limits']=copy.deepcopy(json.loads((root/'campaign/plan.json').read_text())['configs'][2]['capella']['limits']);assert c['limits']['blocks']==source['retentionExperiment']['blocks'];c['recoveryCommitment']=receipt['restart'];"
  assert a in s;s=s.replace(a,b)
  s=s.replace('/build/deposit-deadline-amd64-b5ee00ac-20261005/nano-capella-paused-capture','/build/atomic-runtime-amd64-d71f574a-20261006/nano-capella-paused-capture')
  a="before=ssh(hashcmd);"
  b="program=ssh(base+['--entrypoint','sha256sum',a['image'],'/build/atomic-runtime-amd64-d71f574a-20261006/nano-capella-paused-capture']);assert program.strip().split()[0]=='ef0731bd68d25d66e9073db01e835e6c576983d625b1de4a6dd98e331241b2fd';save(out/'program-hash.json',{'sha256':program.strip().split()[0]})\n"+a
  s=s.replace(a,b)
 Path('/private/tmp',new).write_text(s)
 compile(s,new,'exec')
print('Three audit controls prepared; no read-only DB audit attempted while source writers active')
