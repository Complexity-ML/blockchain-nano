from pathlib import Path
import json,subprocess,hashlib,os
repo=Path('/private/tmp/nano-network-status-coherent-20261006/source');out=Path('/private/tmp/nano-network-status-coherent-native-20261006');core=out/'libnano_core.a'
before=json.loads((out/'source-before.json').read_text());files=sorted((repo/'src').glob('*.cpp'))
assert all(hashlib.sha256((repo/n).read_bytes()).hexdigest()==h for n,h in before.items()),'Frozen source changed before resume'
def run(name,args,cwd=None):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,name
testflags=json.loads(Path('/private/tmp/nano-atomic-proposal-native-20261006/test-compile-command.json').read_text());testflags=testflags[:testflags.index('-c')];testflags.insert(1,'-I'+str(repo/'include'));testflags.insert(1,'-I'+str(repo/'network'));testflags.insert(1,'-I'+str(out/'fixture-support/tests'));testobj=out/'test.o';run('test-compile',testflags+['-c',str(repo/'tests/test_capella_transition.cpp'),'-o',str(testobj)])
link=json.loads(Path('/private/tmp/nano-atomic-proposal-native-20261006/test-link-command.json').read_text());link=[str(testobj) if x.startswith('/private/tmp/') and x.endswith('/test.o') else str(core) if x.endswith('/libnano_core.a') else str(out/'candidate') if x.endswith('/candidate') or x.endswith('/test') else x for x in link]
# The saved template output is authoritative; replace its -o target explicitly.
link[link.index('-o')+1]=str(out/'candidate');run('test-link',link,'/tmp/nano-pruning-wan-native-build-20261004')
for mode in ['beacon-reads','configured-runtime','automatic-recovery','http','wire']:
 run('candidate-'+mode,[str(out/'candidate'),'--'+mode]);print((out/('candidate-'+mode+'.log')).read_text()[-1000:],flush=True)
assert all(hashlib.sha256((repo/p).read_bytes()).hexdigest()==sha for p,sha in before.items())
(out/'programs.sha256').write_text('\n'.join(hashlib.sha256((out/n).read_bytes()).hexdigest()+' '+n for n in ['libnano_core.a','nano','nano-capella-paused-capture','candidate'])+'\n')
print('Native full dependency rebuild and scoped regressions passed',flush=True)

import os
for field in ['identity','sync']:
 env=os.environ.copy();env['NANO_TEST_NETWORK_STATUS_FIELD']=field
 with (out/('network-status-'+field+'.log')).open('w') as f:p=subprocess.run([str(out/'candidate'),'--beacon-reads'],stdout=f,stderr=subprocess.STDOUT,env=env)
 (out/('network-status-'+field+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,field
print('Native coherent status qualification completed',flush=True)
