from pathlib import Path
import json,subprocess,hashlib
old=Path('/private/tmp/nano-atomic-proposal-native-20261006');out=Path('/private/tmp/nano-incoming-alternation-native-20261006');core=out/'libnano_core.a'
for kind in ['nano','nano-capella-paused-capture']:
 args=json.loads((old/(kind+'-link-command.json')).read_text());args=[str(out/kind) if x==str(old/kind) else str(core) if x==str(old/'libnano_core.a') else x for x in args]
 (out/(kind+'-link-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(kind+'-link.log')).open('w') as f:r=subprocess.run(args,cwd='/tmp/nano-pruning-wan-native-build-20261004',stdout=f,stderr=subprocess.STDOUT)
 (out/(kind+'-link-exit.txt')).write_text(str(r.returncode));assert r.returncode==0
with (out/'nano-help.log').open('w') as f:r=subprocess.run([str(out/'nano'),'--help'],stdout=f,stderr=subprocess.STDOUT)
(out/'nano-help-exit.txt').write_text(str(r.returncode));assert r.returncode==0
(out/'final-programs.sha256').write_text('\n'.join(hashlib.sha256(p.read_bytes()).hexdigest()+' '+p.name for p in [core,out/'candidate',out/'nano',out/'nano-capella-paused-capture'])+'\n')
