from pathlib import Path
import json,subprocess,hashlib
base=Path('/private/tmp');out=base/'nano-dependency-preparation-native-20261006';proof=out/'wire-repair';proof.mkdir(exist_ok=False)
t=json.loads((out/'original-terminal-exit.json').read_text());assert t['originalSession']==46475 and t['exitCode'] in [0,1]
args=json.loads((out/'wire-link-command.json').read_text());old=base/'nano-network-status-coherent-native-20261006/test.o';new=out/'candidate.o';assert str(old) in args and new.exists();args=[str(new) if x==str(old) else x for x in args];args[args.index('-o')+1]=str(proof/'candidate')
(proof/'link-command.json').write_text(json.dumps(args,indent=2)+'\n')
with (proof/'link.log').open('w') as f:p=subprocess.run(args,cwd='/tmp/nano-pruning-wan-native-build-20261004',stdout=f,stderr=subprocess.STDOUT)
(proof/'link-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0
with (proof/'wire.log').open('w') as f:p=subprocess.run([str(proof/'candidate'),'--wire'],stdout=f,stderr=subprocess.STDOUT)
(proof/'wire-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0
(proof/'provenance.json').write_text(json.dumps({'oldFixtureLinkNotQualifying':True,'freshlyCompiledObject':str(new),'objectSHA256':hashlib.sha256(new.read_bytes()).hexdigest(),'coreSHA256':hashlib.sha256((base/'nano-network-status-coherent-native-20261006/libnano_core.a').read_bytes()).hexdigest(),'programSHA256':hashlib.sha256((proof/'candidate').read_bytes()).hexdigest(),'runtimeOrWireRecompiled':False,'wanFinalityQualified':False},indent=2)+'\n')
print('Native fresh fixture object correctly linked and targeted wire scenario passed; old link retained and excluded')
