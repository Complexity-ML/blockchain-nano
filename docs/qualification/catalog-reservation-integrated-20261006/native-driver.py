from pathlib import Path
import json,subprocess,hashlib,shutil,os
base=Path('/private/tmp/nano-catalog-worker-failfast-native-20261006');source=Path('/private/tmp/nano-catalog-reservation-integrated-docker-20261006/source');out=Path('/private/tmp/nano-catalog-reservation-integrated-native-20261006');out.mkdir();cwd='/tmp/nano-pruning-wan-native-build-20261004'
def run(name,cmd,env=None):
 (out/(name+'-command.json')).write_text(json.dumps(cmd,indent=2))
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(cmd,cwd=cwd,env=env,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode}));print(name,r.returncode,flush=True);assert r.returncode==0,name
oldsource='/private/tmp/nano-catalog-worker-failfast-docker-20261006/source'
flags=json.loads((base/'compile-rpc-command.json').read_text());flags=[x.replace(oldsource,str(source)) for x in flags[:flags.index('-c')]]
objects=[];reused=[]
for f in sorted((source/'src').glob('*.cpp')):
 if f.name=='main.cpp':continue
 dep=out/(f.name+'.d');run('dependency-'+f.stem,flags+['-MM','-MT',f.name,'-MF',str(dep),str(f)])
 if 'capella_inventory_sessions.hpp' in dep.read_text():
  obj=out/(f.name+'.o');run('compile-'+f.stem,flags+['-c',str(f),'-o',str(obj)]);objects.append(str(obj))
 else:reused.append(f.name)
(out/'dependency-summary.json').write_text(json.dumps({'rebuilt':objects,'reused':reused},indent=2));assert len(objects)==2
parent_hash=hashlib.sha256((base/'libnano_core.a').read_bytes()).hexdigest();core=out/'libnano_core.a';shutil.copy2(base/'libnano_core.a',core);run('archive',['/usr/bin/ar','r',str(core),*objects]);run('index',['/usr/bin/ranlib',str(core)])
for name in ['nano','nano-capella-paused-capture']:
 obj=base/(name+'.o');cmd=json.loads((base/(name+'-link-command.json')).read_text());cmd=[str(core) if x==str(base/'libnano_core.a') else str(out/name) if x==str(base/name) else x for x in cmd];run(name+'-link',cmd)
fixture=out/'fixture-support';shutil.copytree('/Users/boris/Dev/blockchain-nano/tests/support',fixture/'tests/support');(fixture/'network').symlink_to(source/'network',target_is_directory=True)
c=json.loads((base/'test-compile-command.json').read_text());c=[x.replace(oldsource,str(source)) for x in c];c[1]='-I'+str(fixture/'tests');c[c.index('-c')+1]=str(source/'tests/test_capella_transition.cpp');c[c.index('-o')+1]=str(out/'test.o');run('test-compile',c)
l=json.loads((base/'test-link-command.json').read_text());l=[str(core) if x==str(base/'libnano_core.a') else str(out/'test.o') if x==str(base/'test.o') else x for x in l];l[l.index('-o')+1]=str(out/'candidate');run('test-link',l)
for mode in ['catalog-concurrency','configured-runtime','automatic-recovery']:run('test-'+mode,[str(out/'candidate'),'--'+mode])
env=os.environ.copy();env['NANO_TEST_CATALOG_CAPTURE_UNLOCKED']='1';run('test-capture-control',[str(out/'candidate'),'--beacon-reads'],env)
env=os.environ.copy();env['NANO_TEST_CATALOG_WORKER_FAILFAST']='1'
for mode in ['beacon-reads','queued-bls-rpc']:run('test-worker-'+mode,[str(out/'candidate'),'--'+mode],env)
env=os.environ.copy();env['NANO_TEST_CATALOG_GENERATION_RACE']='1';run('test-generation-reservation',[str(out/'candidate'),'--beacon-reads'],env)
assert '"scenario":"catalog-busy-http-control"' in (out/'test-worker-queued-bls-rpc.log').read_text()
assert '"scenario":"catalog-generation-reservation"' in (out/'test-generation-reservation.log').read_text()
assert hashlib.sha256((base/'libnano_core.a').read_bytes()).hexdigest()==parent_hash
(out/'parent-unchanged.json').write_text(json.dumps({'sha256':parent_hash,'unchanged':True}));(out/'programs.json').write_text(json.dumps({n:hashlib.sha256((out/n).read_bytes()).hexdigest() for n in ['libnano_core.a','nano','nano-capella-paused-capture','candidate']},indent=2))
print('coherent native terminal success',flush=True)
