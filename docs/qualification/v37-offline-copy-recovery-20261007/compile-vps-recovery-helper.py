from pathlib import Path
import json,subprocess,shlex
r=Path(__file__).parent;docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';old='/build/coherent-e33ad89e-amd64';candidate='/build/publication-cancel-boundary-amd64-20261007';logs=r/'recovery-helper-build';logs.mkdir();assert json.loads((r/'amd64-build/binding.json').read_text())['draftExit']==0
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v','nano-clean-rpc-build-20261002:/build','-v','/home/ubuntu/blockchain-nano-git:/work:ro','-v',str(r)+':/patch:ro','-w',old]
def read(args):return subprocess.check_output(base+['--entrypoint',args[0],image]+args[1:],text=True)
def run(name,args):
 (logs/(name+'-command.json')).write_text(json.dumps({'args':args,'cwd':old})+'\n')
 with (logs/(name+'.log')).open('w') as f:p=subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode})+'\n');print(name,p.returncode,flush=True);assert p.returncode==0
rows=read(['cat',old+'/CMakeFiles/nano-capella-paused-capture.dir/flags.make']).splitlines();flags=sum([shlex.split(next(x.split(' = ',1)[1] for x in rows if x.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
obj=candidate+'/recover-synthetic-copy.o';run('compile',['/usr/bin/clang++',*flags,'-I/work/tests/capella','-c','/patch/recover-synthetic-copy.cpp','-o',obj])
a=shlex.split(read(['cat',old+'/CMakeFiles/nano-capella-paused-capture.dir/link.txt']));positions=[i for i,v in enumerate(a) if v.endswith('.cpp.o')];assert len(positions)==1;a[positions[0]]=obj;a=[candidate+'/libnano_core.a' if x=='libnano_core.a' else x for x in a];a[a.index('-o')+1]=candidate+'/recover-synthetic-copy';run('link',a)
(logs/'binary.sha256').write_text(read(['sha256sum',candidate+'/recover-synthetic-copy']))
