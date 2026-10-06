from pathlib import Path
import subprocess,shlex,json,time,sys
arch=sys.argv[1];root=Path(__file__).parent;probe=root/(arch+'-host-build');old='/build/coherent-e33ad89e-'+arch;out='/build/durable-canonical-memo-'+arch+'-20261007'
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';source='/private/tmp/nano-coherent-e33ad89e-20261006/source'
else:docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';source='/tmp/nano-coherent-e33ad89e-20261006/source'
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',volume+':/build','-v',source+':/work:ro','-w',old]
args=shlex.split(subprocess.check_output(base+['--entrypoint','cat',image,old+'/CMakeFiles/nano-capella-transition-tests.dir/link.txt'],text=True));args=['/build/catalog-preflight-'+arch+'-20261006/libnano_core.a' if a=='libnano_core.a' else out+'/fixture.o' if a.endswith('/tests/test_capella_transition.cpp.o') else a for a in args];args[args.index('-o')+1]=out+'/baseline-test'
with (probe/'link-baseline.log').open('w') as log:subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=log,stderr=subprocess.STDOUT,check=True)
for name,binary,expected in [('baseline',out+'/baseline-test',1),('fixed',out+'/test',0)]:
 start=time.monotonic();cmd=base+['-e','NANO_TEST_COLD_RUNTIME=1','-e','NANO_TEST_RUNTIME_REFERENCE_MEMO=1','-e','NANO_TEST_DURABLE_CANONICAL_MEMO=1','--entrypoint',binary,image,'--configured-runtime']
 (probe/(name+'-runtime-command.json')).write_text(json.dumps(cmd)+'\n')
 with (probe/(name+'-runtime.log')).open('w') as log:code=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT).returncode
 receipt={'exitCode':code,'elapsedSeconds':time.monotonic()-start,'expectedExitCode':expected};(probe/(name+'-runtime-exit.json')).write_text(json.dumps(receipt)+'\n');print(name,receipt,flush=True);text=(probe/(name+'-runtime.log')).read_text()
 if name=='baseline':assert code!=0 and 'durable wrapper canonical check revalidated immutable cold rows' in text
 else:assert code==0 and 'durable-wrapper-canonical-memo' in text
