from pathlib import Path
import json,hashlib,subprocess
src=Path('/private/tmp/nano-restored-three-node-v34-startup-memo-20261007');audits=Path('/private/tmp/nano-v34-post-stop-readonly-20261007');dest=Path('/private/tmp/nano-restored-three-node-v35-trace-bound-20261007');dest.mkdir(mode=0o700)
def hashes(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}
state=json.loads((src/'campaign/native-state.json').read_text());assert state['exitCode']==0 and state['signalCode'] is None
state=json.loads(subprocess.check_output(['/Users/boris/.docker/bin/docker','inspect','nano-restored-three-node-mac-muxcwsxz','--format','{{json .State}}'],text=True));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled']
assert not any('/nano run ' in l for l in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines())
summary={}
for node,label in [('mac-native','native'),('mac-docker','mac-docker')]:
 p=audits/label;m=json.loads((p/'immutability.json').read_text());assert json.loads((p/'exit.json').read_text())['exitCode']==0 and m['hashesUnchanged'] and m['exactStopCommitmentsVerified'];data=src/node/'data';expected=json.loads((p/'data-before.json').read_text());assert hashes(data)==expected
 out=dest/node/'data';out.parent.mkdir();subprocess.run(['/bin/cp','-cR',str(data),str(out)],check=True);(out/'peer.key').unlink(missing_ok=True);observed=hashes(out);assert observed=={k:v for k,v in expected.items() if k!='peer.key'};assert all((out/k).stat().st_ino!=(data/k).stat().st_ino for k in observed)
 f=out.parent/'data-copy-hashes.json';f.write_text(json.dumps(observed)+'\n');f.chmod(0o600);assert hashes(data)==expected;summary[node]={'files':len(observed),'hashesEqual':True,'independentInodes':True,'journalsAndWitnessesPreserved':True,'peerIdentityExcluded':True}
Path(__file__).with_name('local-copy-receipt.json').write_text(json.dumps({'copies':summary,'signersStarted':False},indent=2)+'\n');print(json.dumps(summary))
