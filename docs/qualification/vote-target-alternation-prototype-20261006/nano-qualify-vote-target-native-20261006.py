from pathlib import Path
import json,subprocess,hashlib
out=Path('/private/tmp/nano-vote-target-alternation-native-20261006')
def run(stage,args,cwd=None):
 (out/(stage+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(stage+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(stage+'-exit.txt')).write_text(str(r.returncode));assert r.returncode==0,stage
args=json.loads((out/'test-compile-command.json').read_text());args=[str(out/'test-qualified.cpp') if x==str(out/'test.cpp') else str(out/'test-qualified.o') if x==str(out/'test.o') else x for x in args];run('qualified-test-compile',args)
args=json.loads((out/'candidate-link-command.json').read_text());args=[str(out/'candidate-qualified') if x==str(out/'candidate') else str(out/'test-qualified.o') if x==str(out/'test.o') else x for x in args];run('qualified-candidate-link',args,'/tmp/nano-pruning-wan-native-build-20261004')
run('qualified-vote-alternation',[str(out/'candidate-qualified'),'--incoming-alternation']);print((out/'qualified-vote-alternation.log').read_text()[-1800:])
(out/'qualified-program.sha256').write_text(hashlib.sha256((out/'candidate-qualified').read_bytes()).hexdigest()+' candidate-qualified\n')
