from pathlib import Path
import shutil,json,hashlib
root=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');proof=repo/'docs/qualification/durable-canonical-reference-memo-20261007';proof.mkdir(parents=True,exist_ok=True)
for arch,name in [('native','native-build'),('arm64','arm64-host-build'),('amd64','amd64-host-build')]:
 p=root/name
 assert json.loads((p/'baseline-runtime-exit.json').read_text())['exitCode']!=0
 assert 'durable wrapper canonical check revalidated immutable cold rows' in (p/'baseline-runtime.log').read_text()
 assert json.loads((p/'fixed-runtime-exit.json').read_text())['exitCode']==0
 rows=[json.loads(l) for l in (p/'fixed-runtime.log').read_text().splitlines() if l.startswith('{')]
 memo=next(x for x in rows if x.get('scenario')=='durable-wrapper-canonical-memo');assert memo['reusePasses']==3 and memo['fullColdRevalidations']==0 and memo['signedVotes']>0
 runtime=next(x for x in rows if x.get('scenario')=='capella-configured-runtime');assert runtime['blocks']==32 and runtime['workersStopped'] and runtime['coldFinalizedPruning']
 if arch!='native':
  for case in ['writer-adversarial','cold-faults']:assert json.loads((p/(case+'-exit.json')).read_text())['exitCode']==0
  faults=[json.loads(l) for l in (p/'cold-faults.log').read_text().splitlines() if l.startswith('{')];assert sum(x.get('scenario')=='capella-cold-memo-sync-recovery' for x in faults)==4;assert sum(x.get('scenario')=='capella-cold-memo-sigkill-recovery' for x in faults)==3
 dest=proof/arch;dest.mkdir(exist_ok=True)
 for f in p.iterdir():
  if f.is_file() and f.suffix in ['.log','.json','.sha256']:shutil.copy2(f,dest/f.name)
for f in root.iterdir():
 if f.is_file() and f.name!='continuation-state.json' and f.suffix in ['.py','.md','.txt','.json','.sha256']:shutil.copy2(f,proof/f.name)
shutil.copytree(root/'patch',proof/'patch',dirs_exist_ok=True)
(proof/'manifest.json').write_text(json.dumps({str(p.relative_to(proof)):hashlib.sha256(p.read_bytes()).hexdigest() for p in proof.rglob('*') if p.is_file() and p.name!='manifest.json'},indent=2)+'\n')
print('Proof verified and archived',proof)
