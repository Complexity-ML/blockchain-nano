from pathlib import Path
import subprocess,shlex,json,sys
arch=sys.argv[1];assert arch in ('arm64','amd64');root=Path(__file__).parent;probe=root/(arch+'-runtime-link');probe.mkdir();old='/build/coherent-e33ad89e-'+arch;out='/build/published-block-inventory-'+arch+'-20261007';parent='/build/durable-canonical-memo-'+arch+'-20261007'
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';source='/Users/boris/Dev/blockchain-nano'
else:docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';source='/home/ubuntu/blockchain-nano-git'
for gate in ['catalog-concurrency-batch','stable-inventory-and-session-bounds','clock-failure-diagnostic','private-http-catalog-control']:assert json.loads((root/(arch+'-build')/(gate+'-exit.json')).read_text())['exitCode']==0
base=docker+['run','--rm','--network','none','--read-only','--tmpfs','/tmp','-v',volume+':/build','-v',source+':/work:ro','-w',old]
def read(path):return subprocess.check_output(base+['--entrypoint','cat',image,path],text=True)
def sha(paths):return subprocess.check_output(base+['--entrypoint','sha256sum',image]+paths,text=True)
before=sha([parent+'/nano',parent+'/capture']);(probe/'parent-before.sha256').write_text(before)
for target,name in [('nano','nano'),('nano-capella-paused-capture','capture')]:
 args=shlex.split(read(old+'/CMakeFiles/'+target+'.dir/link.txt'));args=[out+'/libnano_core.a' if a=='libnano_core.a' else '/build/private-capture-budget-'+arch+'-20261006/capture.o' if a.endswith('/tests/capella/paused_capture.cpp.o') else a for a in args];args[args.index('-o')+1]=out+'/'+name
 (probe/('link-'+name+'-command.json')).write_text(json.dumps(args)+'\n')
 with (probe/('link-'+name+'.log')).open('w') as f:r=subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (probe/('link-'+name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0;print('link '+name+' exit0',flush=True)
after=sha([parent+'/nano',parent+'/capture']);assert before==after;(probe/'parent-after.sha256').write_text(after);(probe/'programs.sha256').write_text(sha([out+'/nano',out+'/capture',out+'/test',out+'/libnano_core.a']));print('New runtime linked; parent programs unchanged; no node started.',flush=True)
