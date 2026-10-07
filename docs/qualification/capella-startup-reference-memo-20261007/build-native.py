from pathlib import Path
import json,subprocess,shlex,shutil,hashlib,os
root=Path(__file__).parent;probe=root/'native-build';probe.mkdir();source=probe/'source';repo=Path('/Users/boris/Dev/blockchain-nano');shutil.copytree(root/'source',source);(source/'network').symlink_to(repo/'network');(source/'tests/support').symlink_to(repo/'tests/support')
old=Path('/private/tmp/nano-coherent-e33ad89e-20261006/native-build');base=Path('/private/tmp/nano-published-block-inventory-draft-20261007/native-build')
m=json.loads((root/'manifest.json').read_text())
for rel,h in m['files'].items():assert hashlib.sha256((root/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256'];assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256']
runtime='src/consensus_capella_runtime.cpp';pinned=json.loads((repo/'docs/qualification/capella-startup-stage-diagnostic-20261007/manifest.json').read_text())['sourceFiles'][runtime];assert hashlib.sha256((repo/runtime).read_bytes()).hexdigest()==pinned;shutil.copy2(repo/runtime,source/runtime);(probe/'runtime-source-hash.json').write_text(json.dumps({runtime:pinned})+'\n')
def hashes():return {f:hashlib.sha256((base/f).read_bytes()).hexdigest() for f in ['nano','test','capture','libnano_core.a']}
before=hashes();(probe/'parent-before.json').write_text(json.dumps(before,indent=2)+'\n');shutil.copy2(base/'libnano_core.a',probe/'libnano_core.a')
def run(name,args,env=None):
 (probe/(name+'-command.json')).write_text(json.dumps({'args':args,'env':env or {}})+'\n')
 with (probe/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd=old,env={**os.environ,**(env or {})},stdout=f,stderr=subprocess.STDOUT)
 (probe/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,(name,r.returncode);print(name+' exit0',flush=True)
def flags(target):
 lines=(old/'CMakeFiles'/target/'flags.make').read_text().splitlines();return sum([shlex.split(next(l.split(' = ',1)[1] for l in lines if l.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
for unit in ['consensus_capella_reactor','consensus_capella_runtime']:
 run('compile-'+unit,['/usr/bin/clang++']+flags('nano_core.dir')+['-I'+str(repo/'src'),'-c',str(source/('src/'+unit+'.cpp')),'-o',str(probe/(unit+'.cpp.o'))])
 run('replace-'+unit,['/usr/bin/ar','r',str(probe/'libnano_core.a'),str(probe/(unit+'.cpp.o'))])
run('compile-fixture',['/usr/bin/clang++']+flags('nano-capella-transition-tests.dir')+['-I'+str(repo/'tests'),'-c',str(source/'tests/test_capella_transition.cpp'),'-o',str(probe/'fixture.o')])
for target,name in [('nano-capella-transition-tests','test'),('nano','nano'),('nano-capella-paused-capture','capture')]:
 args=shlex.split((old/'CMakeFiles'/(target+'.dir')/'link.txt').read_text());args=[str(probe/'libnano_core.a') if a=='libnano_core.a' else str(probe/'fixture.o') if a.endswith('/tests/test_capella_transition.cpp.o') else '/private/tmp/nano-capture-resource-budget-20261006/native/capture.o' if a.endswith('/tests/capella/paused_capture.cpp.o') else a for a in args];args[args.index('-o')+1]=str(probe/name);run('link-'+name,args)
run('startup-recovery-and-migration',[str(probe/'test'),'--prepared-block'],{'NANO_TEST_STARTUP_REFERENCE_MEMO':'1','NANO_TEST_COLD_REACTOR':'1','NANO_TEST_FRAGMENT_CHECKPOINT':'1'})
after=hashes();assert before==after;(probe/'parent-after.json').write_text(json.dumps(after,indent=2)+'\n');(probe/'programs.json').write_text(json.dumps({f:hashlib.sha256((probe/f).read_bytes()).hexdigest() for f in ['nano','test','capture','libnano_core.a']},indent=2)+'\n');print('Native startup checks passed; parent programs unchanged; no WAN/release qualification',flush=True)
