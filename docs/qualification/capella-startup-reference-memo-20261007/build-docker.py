from pathlib import Path
import subprocess,shlex,json,sys,hashlib
arch=sys.argv[1];assert arch in ('arm64','amd64');root=Path(__file__).parent;probe=root/(arch+'-build');probe.mkdir()
old='/build/coherent-e33ad89e-'+arch;parent='/build/startup-stage-diagnostic-'+arch+'-20261007';out='/build/startup-checkpoint-memo-'+arch+'-20261007'
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';source='/Users/boris/Dev/blockchain-nano'
else:docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';source='/home/ubuntu/blockchain-nano-git'
m=json.loads((root/'manifest.json').read_text())
for rel,h in m['files'].items():assert hashlib.sha256((Path(source)/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((root/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',volume+':/build','-v',source+':/work:ro','-v',str(root/'source')+':/patch:ro','-w',old]
def tool(args):return subprocess.check_output(base+['--entrypoint',args[0],image]+args[1:],text=True)
def run(name,args,env=None,expected=0):
 (probe/(name+'-command.json')).write_text(json.dumps({'args':args,'env':env or {},'expectedExit':expected})+'\n')
 with (probe/(name+'.log')).open('w') as f:r=subprocess.run(base+sum([['-e',k+'='+v] for k,v in (env or {}).items()],[])+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (probe/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==expected,(name,r.returncode);print(name+' exit'+str(r.returncode),flush=True)
parents=[parent+'/'+x for x in ['nano','test','libnano_core.a']];before=tool(['sha256sum']+parents);(probe/'parent-before.sha256').write_text(before)
run('stage',['sh','-c','mkdir -p '+out+'/source && cp -R /patch/src /patch/tests '+out+'/source/ && ln -s /work/network '+out+'/source/network && ln -s /work/tests/support '+out+'/source/tests/support && cp '+parent+'/libnano_core.a '+out+'/libnano_core.a'])
def flags(target):
 lines=tool(['cat',old+'/CMakeFiles/'+target+'/flags.make']).splitlines();return sum([shlex.split(next(l.split(' = ',1)[1] for l in lines if l.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
run('compile-fixture',['/usr/bin/clang++']+flags('nano-capella-transition-tests.dir')+['-I/work/tests','-c',out+'/source/tests/test_capella_transition.cpp','-o',out+'/fixture.o'])
def link(library,name,target='nano-capella-transition-tests'):
 args=shlex.split(tool(['cat',old+'/CMakeFiles/'+target+'.dir/link.txt']));args=[library if a=='libnano_core.a' else out+'/fixture.o' if a.endswith('/tests/test_capella_transition.cpp.o') else a for a in args];args[args.index('-o')+1]=out+'/'+name;run('link-'+name,args)
env={'NANO_TEST_STARTUP_REFERENCE_MEMO':'1','NANO_TEST_COLD_REACTOR':'1','NANO_TEST_FRAGMENT_CHECKPOINT':'1'}
link(parent+'/libnano_core.a','parent-test');run('counterfactual-startup',[out+'/parent-test','--prepared-block'],env,1);assert 'witness startup failed to reuse fully validated bounded certificates' in (probe/'counterfactual-startup.log').read_text()
run('compile-reactor',['/usr/bin/clang++']+flags('nano_core.dir')+['-I/work/src','-c',out+'/source/src/consensus_capella_reactor.cpp','-o',out+'/consensus_capella_reactor.cpp.o'])
run('replace-reactor',['/usr/bin/ar','r',out+'/libnano_core.a',out+'/consensus_capella_reactor.cpp.o']);link(out+'/libnano_core.a','test')
run('startup-recovery-and-migration',[out+'/test','--prepared-block'],env)
run('reference-corruption-and-bounds',[out+'/test','--prepared-block'],{'NANO_TEST_REFERENCE_PREFLIGHT':'1','NANO_TEST_REFERENCE_VALIDATION_MEMO':'1','NANO_TEST_COMPACT_REFERENCE_RESTORE':'1','NANO_TEST_COMPACT_REFERENCE_WRITER':'1','NANO_TEST_COLD_RECORD_CACHE':'1','NANO_TEST_FRAGMENT_CHECKPOINT':'1'})
run('cold-runtime-durable-wrapper',[out+'/test','--configured-runtime'],{'NANO_TEST_DURABLE_CANONICAL_MEMO':'1','NANO_TEST_RUNTIME_REFERENCE_MEMO':'1','NANO_TEST_COLD_RUNTIME':'1'})
link(out+'/libnano_core.a','nano','nano');after=tool(['sha256sum']+parents);assert after==before;(probe/'parent-after.sha256').write_text(after);(probe/'programs.sha256').write_text(tool(['sha256sum',out+'/nano',out+'/test',out+'/libnano_core.a']));print('Scoped startup memo gates passed; parent programs unchanged; no network node started',flush=True)
