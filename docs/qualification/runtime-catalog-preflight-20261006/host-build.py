from pathlib import Path
import subprocess,shlex,json,sys
arch=sys.argv[1];probe=Path(__file__).parent/(arch+'-host-build');probe.mkdir();old='/build/coherent-e33ad89e-'+arch;out='/build/catalog-preflight-'+arch+'-20261006'
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';source='/private/tmp/nano-coherent-e33ad89e-20261006/source'
else:docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';source='/tmp/nano-coherent-e33ad89e-20261006/source'
base=docker+['run','--rm','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',volume+':/build','-v',source+':/work:ro','-v',str(Path(__file__).parent/'patch')+':/patch:ro','-w',old]
def tool(args):return subprocess.check_output(base+['--entrypoint',args[0],image]+args[1:],text=True)
def run(name,args):
 (probe/(name+'-command.json')).write_text(json.dumps(args)+'\n')
 with (probe/(name+'.log')).open('w') as f:r=subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (probe/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,(name,r.returncode);print(name+' exit0',flush=True)
parent=tool(['sha256sum',old+'/nano',old+'/libnano_core.a',old+'/nano-capella-transition-tests']);(probe/'parent-before.sha256').write_text(parent)
run('stage',['sh','-c','mkdir -p '+out+'/source/src '+out+'/source/tests && ln -s /work/network '+out+'/source/network && ln -s /work/tests/support '+out+'/source/tests/support && cp /patch/src/consensus_capella_runtime.cpp '+out+'/source/src/ && cp /patch/tests/test_capella_transition.cpp '+out+'/source/tests/ && cp '+old+'/libnano_core.a '+out+'/libnano_core.a'])
def flags(target):
 lines=tool(['cat',old+'/CMakeFiles/'+target+'/flags.make']).splitlines();return sum([shlex.split(next(l.split(' = ',1)[1] for l in lines if l.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
run('compile-runtime',['/usr/bin/clang++']+flags('nano_core.dir')+['-c',out+'/source/src/consensus_capella_runtime.cpp','-o',out+'/consensus_capella_runtime.cpp.o'])
run('replace-runtime-object',['/usr/bin/ar','r',out+'/libnano_core.a',out+'/consensus_capella_runtime.cpp.o'])
run('compile-fixture',['/usr/bin/clang++']+flags('nano-capella-transition-tests.dir')+['-c',out+'/source/tests/test_capella_transition.cpp','-o',out+'/fixture.o'])
for target,name in [('nano-capella-transition-tests','test'),('nano','nano')]:
 args=shlex.split(tool(['cat',old+'/CMakeFiles/'+target+'.dir/link.txt']));args=[out+'/libnano_core.a' if a=='libnano_core.a' else a for a in args];args[args.index('-o')+1]=out+'/'+name
 if target!='nano':args=[out+'/fixture.o' if a.endswith('/tests/test_capella_transition.cpp.o') else a for a in args]
 run('link-'+name,args)
run('catalog-preflight',[out+'/test','--runtime-catalog-preflight']);after=tool(['sha256sum',old+'/nano',old+'/libnano_core.a',old+'/nano-capella-transition-tests']);assert parent==after;(probe/'parent-after.sha256').write_text(after);(probe/'programs.sha256').write_text(tool(['sha256sum',out+'/nano',out+'/test',out+'/libnano_core.a']));print('Parent binaries unchanged; catalog guard qualified',flush=True)
