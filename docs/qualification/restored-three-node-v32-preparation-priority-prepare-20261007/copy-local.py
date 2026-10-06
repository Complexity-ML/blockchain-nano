from pathlib import Path
import json,hashlib,subprocess
src=Path('/private/tmp/nano-restored-three-node-v31-canonical-memo-20261007');audits=Path('/private/tmp/nano-v31-post-stop-readonly-20261007');dest=Path('/private/tmp/nano-restored-three-node-v32-preparation-priority-20261007');dest.mkdir(mode=0o700)
def hashes(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}
summary={}
for node,label in [('mac-native','native'),('mac-docker','mac-docker')]:
 p=audits/label;assert json.loads((p/'exit.json').read_text())['exitCode']==0 and json.loads((p/'immutability.json').read_text())['hashesUnchanged'];data=src/node/'data';expected=json.loads((p/'data-before.json').read_text());assert hashes(data)==expected
 out=dest/node/'data';out.parent.mkdir();subprocess.run(['/bin/cp','-cR',str(data),str(out)],check=True);(out/'peer.key').unlink(missing_ok=True);observed=hashes(out);assert observed=={k:v for k,v in expected.items() if k!='peer.key'};assert all((out/k).stat().st_ino!=(data/k).stat().st_ino for k in observed)
 f=out.parent/'data-copy-hashes.json';f.write_text(json.dumps(observed)+'\n');f.chmod(0o600);assert hashes(data)==expected;summary[node]={'files':len(observed),'hashesEqual':True,'independentInodes':True,'journalsAndWitnessesPreserved':True,'peerIdentityExcluded':True}
Path('/private/tmp/nano-restoration-v32-preparation-priority-prepare-20261007/local-copy-receipt.json').write_text(json.dumps({'copies':summary,'signersStarted':False},indent=2)+'\n');print('V32 complete Mac copies verified; no signer started')
