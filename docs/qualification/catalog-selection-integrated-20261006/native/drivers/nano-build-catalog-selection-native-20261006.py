from pathlib import Path
import json,subprocess,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');old=Path('/private/tmp/nano-admission-continuations-native-20261006');out=Path('/private/tmp/nano-catalog-selection-native-20261006');out.mkdir(exist_ok=True);parent=old/'libnano_core.a';core=out/'libnano_core.a';shutil.copy2(parent,core)
args=json.loads((old/'compile-consensus_capella_reactor-command.json').read_text());flags=args[:args.index('-c')]
def run(name,args,cwd=None):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,name
files=sorted((repo/'src').glob('*.cpp'));before={str(p.relative_to(repo)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files+list((repo/'include').rglob('*.hpp'))+list((repo/'network').glob('*.hpp'))};(out/'source-before.json').write_text(json.dumps(before,indent=2)+'\n');rebuilt=[]
for p in files:
 if p.name=='main.cpp':continue
 dep=out/(p.name+'.d');run('dependency-'+p.stem,flags+['-I'+str(repo/'third_party/secp256k1/include'),'-MM','-MT',p.name,'-MF',str(dep),str(p)])
 if not any(h in dep.read_text() for h in ['consensus_capella_reactor.hpp','altair_worker.hpp','altair_work.hpp','capella_consensus_service.hpp','simulation_queue.hpp','capella_inventory_sessions.hpp']):continue
 obj=out/(p.name+'.o');expected=flags+['-c',str(p),'-o',str(obj)];saved=out/('compile-'+p.stem+'-command.json');done=out/('compile-'+p.stem+'-exit.json');reuse=obj.exists() and saved.exists() and json.loads(saved.read_text())==expected and json.loads(done.read_text())['exitCode']==0
 if not reuse:run('compile-'+p.stem,expected)
 rebuilt.append(str(obj));print('reused verified object' if reuse else 'rebuilt',p.name,flush=True)
(out/'rebuilt-objects.json').write_text(json.dumps(rebuilt,indent=2)+'\n');assert rebuilt
run('archive',['/usr/bin/ar','r',str(core),*rebuilt]);run('index',['/usr/bin/ranlib',str(core)])
for name,source in [('nano',repo/'src/main.cpp'),('nano-capella-paused-capture',repo/'tests/capella/paused_capture.cpp')]:
 obj=out/(name+'.o');run(name+'-compile',flags+['-c',str(source),'-o',str(obj)])
 link=json.loads((old/(name+'-link-command.json')).read_text());link=[str(obj) if x.startswith('/private/tmp/') and x.endswith('.o') else str(core) if x==str(parent) else str(out/name) if x==str(old/name) else x for x in link]
 run(name+'-link',link,'/tmp/nano-pruning-wan-native-build-20261004')
run('nano-help',[str(out/'nano'),'--help'])
testflags=json.loads(Path('/private/tmp/nano-atomic-proposal-native-20261006/test-compile-command.json').read_text());testflags=testflags[:testflags.index('-c')];testobj=out/'test.o';run('test-compile',testflags+['-c',str(repo/'tests/test_capella_transition.cpp'),'-o',str(testobj)])
link=json.loads(Path('/private/tmp/nano-atomic-proposal-native-20261006/test-link-command.json').read_text());link=[str(testobj) if x.startswith('/private/tmp/') and x.endswith('/test.o') else str(core) if x.endswith('/libnano_core.a') else str(out/'candidate') if x.endswith('/candidate') or x.endswith('/test') else x for x in link]
# The saved template output is authoritative; replace its -o target explicitly.
link[link.index('-o')+1]=str(out/'candidate');run('test-link',link,'/tmp/nano-pruning-wan-native-build-20261004')
for mode in ['configured-runtime','automatic-recovery','http','wire']:
 run('candidate-'+mode,[str(out/'candidate'),'--'+mode]);print((out/('candidate-'+mode+'.log')).read_text()[-1000:],flush=True)
assert all(hashlib.sha256((repo/p).read_bytes()).hexdigest()==sha for p,sha in before.items())
(out/'programs.sha256').write_text('\n'.join(hashlib.sha256((out/n).read_bytes()).hexdigest()+' '+n for n in ['libnano_core.a','nano','nano-capella-paused-capture','candidate'])+'\n')
print('Native full dependency rebuild and scoped regressions passed',flush=True)
