from pathlib import Path
import json,subprocess,hashlib
old=Path('/private/tmp/nano-export-contention-native-20261006');out=Path('/private/tmp/nano-incoming-alternation-native-20261006');out.mkdir(exist_ok=False)
def run(stage,args,cwd=None):
 (out/(stage+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(stage+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(stage+'-exit.txt')).write_text(str(r.returncode));return r.returncode
args=json.loads((old/'candidate-test-final-command.json').read_text());args=[x.replace(str(old/'test-final.o'),str(out/'test.o')) for x in args];assert run('test-compile',args)==0
args=json.loads((old/'candidate-final-link-command.json').read_text());args=[x.replace(str(old/'test-final.o'),str(out/'test.o')).replace(str(old/'candidate-final'),str(out/'baseline')) for x in args];assert run('baseline-link',args,'/tmp/nano-pruning-wan-native-build-20261004')==0
status=run('baseline-run',[str(out/'baseline'),'--incoming-alternation']);print('baseline exit',status);print((out/'baseline-run.log').read_text()[-1800:]);assert status==1 and 'Alternating valid blocks erased incoming target continuation' in (out/'baseline-run.log').read_text();(out/'baseline-program.sha256').write_text(hashlib.sha256((out/'baseline').read_bytes()).hexdigest()+' baseline\n')
