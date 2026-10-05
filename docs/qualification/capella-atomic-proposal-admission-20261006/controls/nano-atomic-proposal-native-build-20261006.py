from pathlib import Path
import json,subprocess,shutil,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano');parent=Path('/private/tmp/nano-deposit-deadline-20261005');out=Path('/private/tmp/nano-atomic-proposal-native-20261006');out.mkdir(exist_ok=False);cwd='/tmp/nano-pruning-wan-native-build-20261004'
parents={str(parent/n):hashlib.sha256((parent/n).read_bytes()).hexdigest() for n in ['libnano_core.a','nano','nano-capella-transition-tests','nano-capella-paused-capture']};(out/'parents-before.json').write_text(json.dumps(parents,indent=2)+'\n')
def run(name,cmd):
 (out/(name+'-command.json')).write_text(json.dumps(cmd,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(cmd,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.txt')).write_text(str(r.returncode)+'\n');print(name,r.returncode,flush=True)
 assert r.returncode==0,(out/(name+'.log')).read_text()[-1600:]
shutil.copyfile(parent/'libnano_core.a',out/'libnano_core.a')
objects=[]
for unit in ['consensus_capella_signing','consensus_capella_reactor','consensus_capella_validator']:
 cmd=json.loads((parent/'core-unit-compile-command.json').read_text());cmd[cmd.index('-c')+1]=str(repo/'src'/(unit+'.cpp'));cmd[cmd.index('-o')+1]=str(out/(unit+'.cpp.o'));run(unit+'-compile',cmd);objects.append(cmd[cmd.index('-o')+1])
run('archive',['/usr/bin/ar','r',str(out/'libnano_core.a')]+objects);run('archive-index',['/usr/bin/ranlib',str(out/'libnano_core.a')])
cmd=json.loads((parent/'nano-capella-transition-tests-fresh-compile-command.json').read_text());cmd[cmd.index('-o')+1]=str(out/'test.o');run('test-compile',cmd)
for name,record in [('test','nano-capella-transition-tests-candidate-link-command.json'),('nano','nano-candidate-link-command.json'),('nano-capella-paused-capture','capture-candidate-link-command.json')]:
 cmd=json.loads((parent/record).read_text());cmd=[str(out/'libnano_core.a') if x==str(parent/'libnano_core.a') else str(out/'test.o') if name=='test' and x.endswith('nano-capella-transition-tests.o') else x for x in cmd];cmd[cmd.index('-o')+1]=str(out/name);run(name+'-link',cmd)
run('atomic-proposal',[str(out/'test'),'--atomic-proposal']);run('capacity-pressure',[str(out/'test'),'--capacity-pressure']);run('nano-help',[str(out/'nano'),'--help'])
assert parents=={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in parents}
(out/'parents-unchanged.json').write_text(json.dumps(parents,indent=2)+'\n')
sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in ['src/consensus_capella_signing.cpp','src/consensus_capella_reactor.cpp','src/consensus_capella_validator.cpp','include/nano/consensus_capella_reactor.hpp','tests/test_capella_transition.cpp']};(out/'sources.json').write_text(json.dumps(sources,indent=2)+'\n')
(out/'programs.json').write_text(json.dumps({name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ['libnano_core.a','test','nano','nano-capella-paused-capture']},indent=2)+'\n')
(out/'controller-exit.txt').write_text('0\n')
