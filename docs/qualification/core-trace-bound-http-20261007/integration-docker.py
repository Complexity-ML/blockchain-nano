from pathlib import Path
import subprocess,json,sys,hashlib,shlex
r=Path(__file__).parent;arch=sys.argv[1];assert arch in ('arm64','amd64');logs=r/(arch+'-integration');logs.mkdir()
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];repo=Path('/Users/boris/Dev/blockchain-nano');image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';container='nano-restored-three-node-mac-muxcwsxz'
else:docker=['sudo','-n','docker'];repo=Path('/home/ubuntu/blockchain-nano-git');image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';container='nano-restored-three-node-vps-muxcwsxz'
s=json.loads(subprocess.check_output(docker+['inspect',container,'--format','{{json .State}}'],text=True));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
m=json.loads((r/'manifest.json').read_text())
for rel,h in m['files'].items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((r/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
out='/build/core-trace-bound-'+arch+'-20261007';old='/build/coherent-e33ad89e-'+arch+'/network';parent='/build/catchup-priority-'+arch+'-20261007/network/nano-p2p'
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','-w',old,'-v',volume+':/build','-v',str(repo)+':/work:ro','-v',str(r)+':/patch:ro']
def read(path):return subprocess.check_output(base+['--entrypoint','cat',image,path],text=True)
def run(name,args):
 (logs/(name+'-command.json')).write_text(json.dumps({'args':args,'cwd':old})+'\n')
 with (logs/(name+'.log')).open('w') as f:x=subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':x.returncode})+'\n');assert x.returncode==0,(name,x.returncode);print(name+' exit0',flush=True)
before=read(parent) if False else subprocess.check_output(base+['--entrypoint','sha256sum',image,parent],text=True)
run('stage',['sh','-c','mkdir -p '+out+'/source && cp -R /work/network '+out+'/source/ && cp /patch/source/network/* '+out+'/source/network/'])
def build(name,target,source,execute):
 directory=old+'/CMakeFiles/'+target+'.dir';rows=read(directory+'/flags.make').splitlines();flags=[]
 for key in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']:flags+=shlex.split(next(x.split(' = ',1)[1] for x in rows if x.startswith(key+' = ')))
 obj=out+'/'+name+'.o';binary=out+'/'+name
 run(name+'-compile',['/usr/bin/clang++','-I'+out+'/source/network',*flags,'-c',out+'/source/network/'+source,'-o',obj])
 link=shlex.split(read(directory+'/link.txt'));pos=[i for i,v in enumerate(link) if v.endswith('.cpp.o')];assert len(pos)==1;link[pos[0]]=obj;link[link.index('-o')+1]=binary
 run(name+'-link',link)
 if execute:run(name,[binary])
build('trace-http','nano-core-keepalive-tests','test_core_trace_bound_http.cpp',True)
build('keepalive-http','nano-core-keepalive-tests','test_core_keepalive.cpp',True)
build('catalog-busy-http','nano-capella-catalog-busy-http-tests','test_capella_catalog_busy_http.cpp',True)
build('nano-p2p','nano-p2p','main.cpp',False)
after=subprocess.check_output(base+['--entrypoint','sha256sum',image,parent],text=True);assert before==after
for rel,h in m['files'].items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((r/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
(logs/'binding.json').write_text(json.dumps({'parentProgram':before.strip(),'parentUnchanged':True,'draftFiles':m['files'],'mainBuilt':True,'realHttpQualified':True,'networkQualified':False},indent=2)+'\n')
print('HTTP integration and main link passed; network finality remains unqualified',flush=True)
