from pathlib import Path
import json,subprocess,os,shutil
base=Path('/private/tmp/nano-publication-cancel-boundary-20261007');out=base/'repair';out.mkdir(exist_ok=True);shutil.copyfile(base/'test_capella_transition.cpp',out/'test_capella_transition.cpp');shutil.copyfile(base/'reactor.o',out/'consensus_capella_reactor.cpp.o');parent=Path('/private/tmp/nano-published-signed-inventory-draft-20261007/native-build')
def run(name,args,cwd,expected=0,env=None):
 (out/(name+'-command.json')).write_text(json.dumps({'args':args,'cwd':cwd,'expectedExit':expected,'env':env or {}},indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,env={**os.environ,**(env or {})},stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode,'expectedExit':expected})+'\n');print(name,r.returncode,flush=True)
 if r.returncode!=expected:raise SystemExit('Unexpected result: '+name)
j=json.loads((parent/'compile-test-command.json').read_text());a=j['args'];a[a.index('-c')+1]=str(out/'test_capella_transition.cpp');a[a.index('-o')+1]=str(out/'test.o');run('compile-regression',a,j['cwd'])
link=json.loads((parent/'link-test-command.json').read_text());a=link['args'];a[a.index('-o')-1]=str(out/'test.o')
archive=str(parent/'programs/libnano_core.a');env={'NANO_TEST_FRAGMENT_CHECKPOINT':'1','NANO_TEST_COLD_REACTOR':'1','NANO_TEST_PUBLICATION_CANCEL_BOUNDARY':'1'}
for kind in ['parent','draft']:
 args=a.copy();args[args.index('-o')+1]=str(out/('test-'+kind))
 if kind=='draft':
  dst=out/'libnano_core.a';shutil.copyfile(archive,dst);run('replace-reactor',['/usr/bin/ar','r',str(dst),str(out/'consensus_capella_reactor.cpp.o')],link['cwd']);args[args.index(archive)]=str(dst)
 run('link-'+kind,args,link['cwd']);run('boundary-'+kind,[str(out/('test-'+kind)),'--prepared-block'],link['cwd'],1 if kind=='parent' else 0,env)
 if kind=='parent':assert 'cancellation callback after durable witness intent' in (out/'boundary-parent.log').read_text()
print('Native regression complete; no WAN or cross-architecture claim',flush=True)
