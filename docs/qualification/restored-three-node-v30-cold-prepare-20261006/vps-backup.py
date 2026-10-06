from pathlib import Path
import json,hashlib,subprocess,shutil,os
volume='nano-restored-three-node-vps-20261005-data';source=Path('/var/lib/docker/volumes')/volume/'_data';root=Path('/tmp/nano-restored-three-node-vps-v30-source-backup-20261006');old=Path('/tmp/nano-restored-three-node-vps-mux7cfxf/local-readonly-audit')
assert not subprocess.check_output(['docker','ps','-q','--filter','volume='+volume],text=True).strip();state=json.loads(subprocess.check_output(['docker','inspect','nano-restored-three-node-vps-mux7cfxf','--format','{{json .State}}'],text=True));assert state['Status']=='exited' and state['ExitCode']==1 and not state['OOMKilled']
def hashes(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}
expected={line.split('  ',1)[1].removeprefix('/var/lib/nano/'):line.split('  ',1)[0] for line in (old/'data-before.sha256').read_text().splitlines()};before=hashes(source);assert before==expected
assert shutil.disk_usage('/tmp').free>2*sum(f.stat().st_size for f in source.rglob('*') if f.is_file())+2**30
root.mkdir(mode=0o700);dest=root/'data';shutil.copytree(source,dest,copy_function=shutil.copy2);assert hashes(dest)==before==hashes(source)
assert all((source/f).stat().st_ino!=(dest/f).stat().st_ino for f in before)
(root/'private-file-hashes.json').write_text(json.dumps(before,indent=2)+'\n');os.chmod(root/'private-file-hashes.json',0o600)
receipt={'sourceVolume':volume,'backupLocation':str(root),'sourceFiles':len(before),'allHashesEqual':True,'sourceUnchanged':True,'independentFileInodes':True,'fullJournalsAndWitnessesPreserved':True,'peerIdentityPreservedInBackupOnly':True,'backupKeptOnVps':True,'sourceObservedStopped':True,'signersStarted':False,'productionQualified':False}
(root/'public-backup-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
