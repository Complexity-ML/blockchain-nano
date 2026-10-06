from pathlib import Path
import subprocess,shlex,json,sys
arch='native';probe=Path(__file__).parent/'native-build';probe.mkdir(exist_ok=True);old='/private/tmp/nano-coherent-e33ad89e-20261006/native-build';out=str(probe)
source=Path('/private/tmp/nano-coherent-e33ad89e-20261006/source');(probe/'source/src').mkdir(parents=True,exist_ok=True);(probe/'source/tests').mkdir(exist_ok=True);None
import shutil
for f in ['src/consensus_capella_reactor.cpp','tests/test_capella_transition.cpp']:shutil.copy2(Path(__file__).parent/'patch'/f,probe/'source'/f)
def tool(args):return subprocess.check_output(args,cwd=old,text=True)
def run(name,args):
 (probe/(name+'-command.json')).write_text(json.dumps(args)+'\n')
 with (probe/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd=old,stdout=f,stderr=subprocess.STDOUT)
 (probe/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,(name,r.returncode);print(name+' exit0',flush=True)
parent=tool(['shasum','-a','256',old+'/nano',old+'/libnano_core.a']);(probe/'parent-before.sha256').write_text(parent);shutil.copy2(Path('/private/tmp/nano-runtime-catalog-preflight-20261006/native-build/libnano_core.a'),probe/'libnano_core.a')
def flags(target):
 lines=tool(['cat',old+'/CMakeFiles/'+target+'/flags.make']).splitlines();return sum([shlex.split(next(l.split(' = ',1)[1] for l in lines if l.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
run('compile-reactor-v2',['/usr/bin/clang++']+flags('nano_core.dir')+['-I'+str(source/'src'),'-c',out+'/source/src/consensus_capella_reactor.cpp','-o',out+'/consensus_capella_reactor.cpp.o'])
run('replace-runtime-object',['/usr/bin/ar','r',out+'/libnano_core.a',out+'/consensus_capella_reactor.cpp.o'])
run('compile-fixture',['/usr/bin/clang++']+flags('nano-capella-transition-tests.dir')+['-I'+str(source/'tests'),'-c',out+'/source/tests/test_capella_transition.cpp','-o',out+'/fixture.o'])
for target,name in [('nano-capella-transition-tests','test'),('nano','nano')]:
 args=shlex.split(tool(['cat',old+'/CMakeFiles/'+target+'.dir/link.txt']));args=[out+'/libnano_core.a' if a=='libnano_core.a' else a for a in args];args[args.index('-o')+1]=out+'/'+name
 if target!='nano':args=[out+'/fixture.o' if a.endswith('/tests/test_capella_transition.cpp.o') else a for a in args]
 run('link-'+name,args)
after=tool(['shasum','-a','256',old+'/nano',old+'/libnano_core.a']);assert parent==after;(probe/'parent-after.sha256').write_text(after);(probe/'programs.sha256').write_text(tool(['shasum','-a','256',out+'/nano',out+'/test',out+'/libnano_core.a']));print('Parent binaries unchanged; catalog guard qualified',flush=True)
