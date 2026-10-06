from pathlib import Path
import subprocess,shlex,json,time,sys
arch=sys.argv[1];root=Path(__file__).parent;probe=root/(arch+'-host-build');old='/build/coherent-e33ad89e-'+arch;out='/build/durable-canonical-memo-'+arch+'-20261007'
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';source='/private/tmp/nano-coherent-e33ad89e-20261006/source'
else:docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';source='/tmp/nano-coherent-e33ad89e-20261006/source'
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',volume+':/build','-v',source+':/work:ro','-w',old]
cases={
 'writer-adversarial':['NANO_TEST_REFERENCE_PREFLIGHT=1','NANO_TEST_REFERENCE_VALIDATION_MEMO=1','NANO_TEST_COMPACT_REFERENCE_RESTORE=1','NANO_TEST_COMPACT_REFERENCE_WRITER=1','NANO_TEST_COLD_RECORD_CACHE=1','NANO_TEST_FRAGMENT_CHECKPOINT=1'],
 'cold-faults':['NANO_TEST_FRAGMENT_CHECKPOINT=1','NANO_TEST_COLD_REACTOR=1','NANO_TEST_RUNTIME_REFERENCE_MEMO=1','NANO_TEST_COLD_MEMO_SYNC=1','NANO_TEST_COLD_MEMO_SIGKILL=1','NANO_TEST_DUPLICATE_GUARDS=1']}
for name,variables in cases.items():
 cmd=base+sum([['-e',v] for v in variables],[])+['--entrypoint',out+'/test',image,'--prepared-block'];(probe/(name+'-command.json')).write_text(json.dumps(cmd)+'\n');start=time.monotonic()
 with (probe/(name+'.log')).open('w') as log:code=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT).returncode
 receipt={'exitCode':code,'elapsedSeconds':time.monotonic()-start};(probe/(name+'-exit.json')).write_text(json.dumps(receipt)+'\n');print(name,receipt,flush=True);assert code==0
