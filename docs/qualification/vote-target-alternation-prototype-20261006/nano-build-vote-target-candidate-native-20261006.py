from pathlib import Path
import json,subprocess,hashlib,shutil
old=Path('/private/tmp/nano-incoming-alternation-native-20261006');out=Path('/private/tmp/nano-vote-target-alternation-native-20261006');lib=out/'libnano_core.a';shutil.copy2(old/'libnano_core.a',lib)
def run(stage,args,cwd=None):
 (out/(stage+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(stage+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(stage+'-exit.txt')).write_text(str(r.returncode));assert r.returncode==0,stage
args=json.loads((old/'candidate-core-compile-command.json').read_text());args.insert(1,'-I/Users/boris/Dev/blockchain-nano/src');args=[str(out/'candidate-reactor.cpp') if x.endswith('/src/consensus_capella_reactor.cpp') else str(out/'consensus_capella_reactor.cpp.o') if x==str(old/'consensus_capella_reactor.cpp.o') else x for x in args];run('candidate-core-compile',args)
run('candidate-archive',['/usr/bin/ar','r',str(lib),str(out/'consensus_capella_reactor.cpp.o')]);run('candidate-index',['/usr/bin/ranlib',str(lib)])
args=json.loads((out/'baseline-link-command.json').read_text());args=[str(out/'candidate') if x==str(out/'baseline') else str(lib) if x==str(old/'libnano_core.a') else x for x in args];run('candidate-link',args,'/tmp/nano-pruning-wan-native-build-20261004')
run('candidate-vote-alternation',[str(out/'candidate'),'--incoming-alternation']);print((out/'candidate-vote-alternation.log').read_text()[-1800:])
(out/'programs.sha256').write_text('\n'.join(hashlib.sha256(p.read_bytes()).hexdigest()+' '+p.name for p in [lib,out/'baseline',out/'candidate'])+'\n')
