from pathlib import Path
import json,subprocess,hashlib,shutil
old=Path('/private/tmp/nano-incoming-alternation-native-20261006');out=Path('/private/tmp/nano-vote-cache-integrated-native-20261006');out.mkdir(exist_ok=False);lib=out/'libnano_core.a';shutil.copy2(old/'libnano_core.a',lib)
def run(stage,args,cwd=None,expected=0):
 (out/(stage+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(stage+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(stage+'-exit.txt')).write_text(str(r.returncode));assert r.returncode==expected,stage
args=json.loads((old/'candidate-core-compile-command.json').read_text());args=[str(out/'consensus_capella_reactor.cpp.o') if x==str(old/'consensus_capella_reactor.cpp.o') else x for x in args];run('core-compile',args)
run('archive',['/usr/bin/ar','r',str(lib),str(out/'consensus_capella_reactor.cpp.o')]);run('index',['/usr/bin/ranlib',str(lib)])
args=json.loads((old/'test-compile-command.json').read_text());args=[str(out/'test.o') if x==str(old/'test.o') else x for x in args];run('test-compile',args)
for variant in ['baseline','candidate']:
 args=json.loads((old/'candidate-link-command.json').read_text());args=[str(out/variant) if x==str(old/'candidate') else str(out/'test.o') if x==str(old/'test.o') else str(lib) if x==str(old/'libnano_core.a') and variant=='candidate' else x for x in args];run(variant+'-link',args,'/tmp/nano-pruning-wan-native-build-20261004')
run('baseline-run',[str(out/'baseline'),'--vote-target-alternation'],expected=1);assert 'Alternating valid votes erased target continuation' in (out/'baseline-run.log').read_text()
for mode in ['vote-target-alternation','incoming-alternation','prepared-block','wire']:
 run('candidate-'+mode,[str(out/'candidate'),'--'+mode]);print((out/('candidate-'+mode+'.log')).read_text()[-1400:])
(out/'programs.sha256').write_text('\n'.join(hashlib.sha256(p.read_bytes()).hexdigest()+' '+p.name for p in [lib,out/'baseline',out/'candidate'])+'\n')
