from pathlib import Path
import subprocess,json,hashlib,shutil,os
volume='nano-restored-three-node-vps-20261005-data';source=Path('/var/lib/docker/volumes')/volume/'_data';old=Path('/tmp/nano-restored-three-node-vps-muxfgr38');audit=Path('/tmp/nano-v36-post-stop-readonly-20261007');backup=Path('/tmp/nano-restored-three-node-vps-v37-source-backup-20261007')
state=json.loads(subprocess.check_output(['docker','inspect',old.name,'--format','{{json .State}}'],text=True));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled'];assert not subprocess.check_output(['docker','ps','-q','--filter','volume='+volume],text=True).strip()
assert json.loads((audit/'exit.json').read_text())['exitCode']==0
m=json.loads((audit/'immutability.json').read_text());assert m['hashesUnchanged'] and m['exactStopCommitmentsVerified'] and m['primaryAndIndependentWitnessVerified']
def files(p):return {str(x.relative_to(p)):hashlib.sha256(x.read_bytes()).hexdigest() for x in sorted(p.rglob('*')) if x.is_file()}
expected={l.split('  ',1)[1].removeprefix('/var/lib/nano/'):l.split('  ',1)[0] for l in (audit/'data-before.sha256').read_text().splitlines()};assert files(source)==expected
assert not backup.exists();shutil.copytree(source,backup);assert files(backup)==expected and files(source)==expected;assert all((source/f).stat().st_ino!=(backup/f).stat().st_ino for f in expected)
dest=old/'local-readonly-audit';assert not dest.exists();dest.mkdir(mode=0o700)
for n in ['config.json','request.json','capture.json','immutability.json']:shutil.copyfile(audit/n,dest/n);os.chmod(dest/n,0o600)
(dest/'request-config-hashes.json').write_text(json.dumps({n:hashlib.sha256((dest/n).read_bytes()).hexdigest() for n in ['config.json','request.json','capture.json']})+'\n');os.chmod(dest/'request-config-hashes.json',0o600)
for p in dest.iterdir():os.chown(p,1000,1000)
os.chown(dest,1000,1000)
print(json.dumps({'backupLocation':str(backup),'sourceFiles':len(expected),'allHashesEqual':True,'sourceUnchanged':True,'independentFileInodes':True,'localAuditBound':True,'sourceStoppedClean0':True,'signersStarted':False}))
