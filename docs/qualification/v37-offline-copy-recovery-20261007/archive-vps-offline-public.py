from pathlib import Path
import shutil,json,hashlib
r=Path('/tmp/nano-publication-cancel-boundary-20261007');out=r/'offline-public';out.mkdir(mode=0o700)
for folder in ['recovery-run','recovery-run-repair']:
 src=Path('/tmp/nano-v37-offline-recovery-copy-20261007')/folder;dst=out/folder;dst.mkdir()
 for name in ['mode-guard','recovery','idempotent-reopen','capture']:
  for suffix in ['-exit.json','-error.log','-command.json']:
   p=src/(name+suffix)
   if p.exists():shutil.copyfile(p,dst/p.name)
 for name in ['recovery.json','idempotent-reopen.json']:
  if (src/name).exists():shutil.copyfile(src/name,dst/name)
assert not any(p.name=='capture.json' for p in out.rglob('*'))
(out/'manifest.json').write_text(json.dumps({'privateCapsuleExported':False,'files':{str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.is_file()}},indent=2)+'\n')
print('Only public commands, exits and core tokens archived',flush=True)
