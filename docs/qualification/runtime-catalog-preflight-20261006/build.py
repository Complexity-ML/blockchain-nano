from pathlib import Path
import subprocess,shlex,shutil,hashlib,json,sys
arch=sys.argv[1];old=Path('/build/coherent-e33ad89e-'+arch);out=Path('/build/catalog-preflight-'+arch+'-20261006');out.mkdir();(out/'source/src').mkdir(parents=True);(out/'source/tests').mkdir();(out/'source/network').symlink_to('/work/network');(out/'source/tests/support').symlink_to('/work/tests/support')
for name in ['src/consensus_capella_runtime.cpp','tests/test_capella_transition.cpp']:shutil.copy2(Path('/patch')/name,out/'source'/name)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
before={str(p):sha(p) for p in [old/'nano',old/'libnano_core.a',old/'nano-capella-transition-tests']}
def run(name,args,cwd=old):
 (out/(name+'-command.json')).write_text(json.dumps(args)+'\n')
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.txt')).write_text(str(r.returncode)+'\n');assert r.returncode==0,(name,r.returncode);print(name+' exit0',flush=True)
def flags(target):
 lines=(old/'CMakeFiles'/target/'flags.make').read_text().splitlines();return sum([shlex.split(next(l.split(' = ',1)[1] for l in lines if l.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
run('compile-runtime',['/usr/bin/clang++']+flags('nano_core.dir')+['-c',str(out/'source/src/consensus_capella_runtime.cpp'),'-o',str(out/'consensus_capella_runtime.cpp.o')])
shutil.copy2(old/'libnano_core.a',out/'libnano_core.a');run('replace-runtime-object',['/usr/bin/ar','r',str(out/'libnano_core.a'),str(out/'consensus_capella_runtime.cpp.o')])
run('compile-fixture',['/usr/bin/clang++']+flags('nano-capella-transition-tests.dir')+['-c',str(out/'source/tests/test_capella_transition.cpp'),'-o',str(out/'fixture.o')])
for target,name in [('nano-capella-transition-tests','test'),('nano','nano')]:
 args=shlex.split((old/'CMakeFiles'/(target+'.dir')/'link.txt').read_text());args=[str(out/'libnano_core.a') if a=='libnano_core.a' else a for a in args];args[args.index('-o')+1]=str(out/name)
 if target!='nano':args=[str(out/'fixture.o') if a.endswith('/tests/test_capella_transition.cpp.o') else a for a in args]
 run('link-'+name,args)
run('catalog-preflight',[str(out/'test'),'--runtime-catalog-preflight'])
assert before=={p:sha(Path(p)) for p in before};(out/'qualification.json').write_text(json.dumps({'parentBinariesUnchanged':True,'oneCoreTranslationUnitChanged':True,'bootstrapAndMigrationRefusals':14,'programs':{n:sha(out/n) for n in ['nano','test','libnano_core.a']},'parent':before,'networkQualified':False},indent=2)+'\n')
