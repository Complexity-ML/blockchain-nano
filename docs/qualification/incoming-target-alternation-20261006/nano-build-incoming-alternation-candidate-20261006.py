from pathlib import Path
import shutil,json,subprocess,hashlib
old=Path('/private/tmp/nano-export-contention-native-20261006');out=Path('/private/tmp/nano-incoming-alternation-native-20261006');lib=out/'libnano_core.a';shutil.copy2(old/'libnano_core.a',lib)
def run(stage,args,cwd=None):
 (out/(stage+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(stage+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(stage+'-exit.txt')).write_text(str(r.returncode));assert r.returncode==0,stage
args=json.loads((old/'candidate-core-compile-command.json').read_text());args=[x.replace(str(old/'consensus_capella_reactor.cpp.o'),str(out/'consensus_capella_reactor.cpp.o')) for x in args];run('candidate-core-compile',args)
run('candidate-archive',['/usr/bin/ar','r',str(lib),str(out/'consensus_capella_reactor.cpp.o')]);run('candidate-index',['/usr/bin/ranlib',str(lib)])
args=json.loads((out/'baseline-link-command.json').read_text());args=[str(out/'candidate') if x==str(out/'baseline') else str(lib) if x==str(old/'libnano_core.a') else x for x in args];run('candidate-link',args,'/tmp/nano-pruning-wan-native-build-20261004')
for mode in ['incoming-alternation','prepared-block','wire']:
 run('candidate-'+mode,[str(out/'candidate'),'--'+mode]);print((out/('candidate-'+mode+'.log')).read_text()[-1300:])
(out/'programs.sha256').write_text('\n'.join(hashlib.sha256(p.read_bytes()).hexdigest()+' '+p.name for p in [lib,out/'baseline',out/'candidate'])+'\n')
