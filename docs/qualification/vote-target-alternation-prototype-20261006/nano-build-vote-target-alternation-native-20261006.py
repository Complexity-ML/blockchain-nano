from pathlib import Path
import json,subprocess,hashlib
old=Path('/private/tmp/nano-incoming-alternation-native-20261006');out=Path('/private/tmp/nano-vote-target-alternation-native-20261006')
def run(stage,args,cwd=None):
 (out/(stage+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(stage+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(stage+'-exit.txt')).write_text(str(r.returncode));return r.returncode
args=json.loads((old/'test-compile-command.json').read_text());args.insert(1,'-I/Users/boris/Dev/blockchain-nano/tests');args=[str(out/'test.cpp') if x.endswith('/tests/test_capella_transition.cpp') else str(out/'test.o') if x==str(old/'test.o') else x for x in args];assert run('test-compile',args)==0
args=json.loads((old/'candidate-link-command.json').read_text());args=[str(out/'baseline') if x==str(old/'candidate') else str(out/'test.o') if x==str(old/'test.o') else x for x in args];assert run('baseline-link',args,'/tmp/nano-pruning-wan-native-build-20261004')==0
status=run('baseline-run',[str(out/'baseline'),'--incoming-alternation']);print('baseline exit',status);print((out/'baseline-run.log').read_text()[-2000:]);(out/'baseline-program.sha256').write_text(hashlib.sha256((out/'baseline').read_bytes()).hexdigest()+' baseline\n')
