from pathlib import Path
import subprocess,shlex,json,sys
arch=sys.argv[1];root=Path(__file__).parent
if arch=='native':
 old='/private/tmp/nano-coherent-e33ad89e-20261006/native-build';out=str(root/'native-build');obj='/private/tmp/nano-capture-resource-budget-20261006/native/capture.o';probe=root/'native-build';base=[]
 def read(path):return Path(path).read_text()
 def run(args):return subprocess.run(args,cwd=old,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
else:
 old='/build/coherent-e33ad89e-'+arch;out='/build/durable-canonical-memo-'+arch+'-20261007';obj='/build/private-capture-budget-'+arch+'-20261006/capture.o';probe=root/(arch+'-host-build')
 if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002'
 else:docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002'
 source=('/private/tmp' if arch=='arm64' else '/tmp')+'/nano-coherent-e33ad89e-20261006/source'
 base=docker+['run','--rm','--network','none','--read-only','--tmpfs','/tmp','-v',volume+':/build','-v',source+':/work:ro','-w',old]
 def read(path):return subprocess.check_output(base+['--entrypoint','cat',image,path],text=True)
 def run(args):return subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
args=shlex.split(read(old+'/CMakeFiles/nano-capella-paused-capture.dir/link.txt'));args=[out+'/libnano_core.a' if a=='libnano_core.a' else obj if a.endswith('/tests/capella/paused_capture.cpp.o') else a for a in args];args[args.index('-o')+1]=out+'/capture';(probe/'link-capture-command.json').write_text(json.dumps(args)+'\n');r=run(args);(probe/'link-capture.log').write_text(r.stdout);(probe/'link-capture-exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0;print('capture relinked',arch)
