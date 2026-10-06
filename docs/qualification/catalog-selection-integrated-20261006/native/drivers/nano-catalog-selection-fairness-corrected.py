from pathlib import Path
import json,subprocess,hashlib
r=Path('/private/tmp/nano-catalog-selection-native-sdk-20261006')
p=r/'fairness-cmake-definitions';p.mkdir(exist_ok=False)
cmd=json.loads((r/'capella_head_history_fairness-compile-command.json').read_text());cmd.insert(1,'-DNANO_HEAD_HISTORY_FIRST');cmd[-1]=str(p/'test.o')
(p/'compile-command.json').write_text(json.dumps(cmd,indent=2))
def run(c,n,cwd=None):
 with (p/(n+'.log')).open('w') as f: v=subprocess.run(c,stdout=f,stderr=subprocess.STDOUT,cwd=cwd)
 (p/(n+'-exit.json')).write_text(json.dumps({'exitCode':v.returncode}))
 assert v.returncode==0,(n,v.returncode)
run(cmd,'compile')
link=json.loads((r/'capella_head_history_fairness-link-command.json').read_text())
link=[str(p/'test.o') if x==str(r/'capella_head_history_fairness.o') else str(p/'test') if x==str(r/'capella_head_history_fairness') else x for x in link]
(p/'link-command.json').write_text(json.dumps(link,indent=2));run(link,'link','/private/tmp/nano-dispatch-integrated-20261006/native')
run([str(p/'test')],'run');print((p/'run.log').read_text())
(p/'provenance.json').write_text(json.dumps({'previousFailure':'missing NANO_HEAD_HISTORY_FIRST definition required by network/CMakeLists.txt:186','repositorySourceChanged':False,'sha256':hashlib.sha256((p/'test').read_bytes()).hexdigest()},indent=2))
