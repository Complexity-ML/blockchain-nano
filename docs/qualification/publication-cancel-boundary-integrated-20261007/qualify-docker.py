from pathlib import Path
import subprocess,json,sys,hashlib,shlex
r=Path(__file__).parent;arch=sys.argv[1];assert arch in ('arm64','amd64')
repo=Path('/Users/boris/Dev/blockchain-nano' if arch=='arm64' else '/home/ubuntu/blockchain-nano-git')
docker=['/Users/boris/.docker/bin/docker'] if arch=='arm64' else ['sudo','-n','docker']
image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb' if arch=='arm64' else 'fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
volume='nano-clean-arm64-build-20261002' if arch=='arm64' else 'nano-clean-rpc-build-20261002'
container='nano-restored-three-node-'+('mac' if arch=='arm64' else 'vps')+'-muxgxadj'
s=json.loads(subprocess.check_output(docker+['inspect',container,'--format','{{json .State}}'],text=True));assert not s['Running'] and not s['OOMKilled'] and s['ExitCode']==(0 if arch=='arm64' else 1)
if arch=='amd64':assert not subprocess.check_output(docker+['ps','-q','--filter','volume=nano-restored-three-node-vps-20261005-data'],text=True).strip()
old='/build/coherent-e33ad89e-'+arch;parent='/build/published-signed-inventory-'+arch+'-20261007';out='/build/publication-cancel-boundary-'+arch+'-20261007';logs=r/(arch+'-build');logs.mkdir()
assert hashlib.sha256((repo/'src/consensus_capella_reactor.cpp').read_bytes()).hexdigest()=='d3b6bf67c533024a657a4d0e2b0d4160ea6fe0d5df460e994064ca63310ccbc5'
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',volume+':/build','-v',str(repo)+':/work:ro','-v',str(r)+':/patch:ro','-w',old]
def tool(args):return subprocess.check_output(base+['--entrypoint',args[0],image]+args[1:],text=True)
def run(name,args,env=None,expected=0):
 (logs/(name+'-command.json')).write_text(json.dumps({'args':args,'env':env or {},'expectedExit':expected,'cwd':old})+'\n')
 with (logs/(name+'.log')).open('w') as f:x=subprocess.run(base+sum([['-e',k+'='+v] for k,v in (env or {}).items()],[])+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':x.returncode})+'\n');print(name+' exit'+str(x.returncode),flush=True);assert x.returncode==expected,(name,x.returncode)
def flags(target):
 rows=tool(['cat',old+'/CMakeFiles/'+target+'.dir/flags.make']).splitlines();return sum([shlex.split(next(x.split(' = ',1)[1] for x in rows if x.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
parents=[parent+'/'+n for n in ['nano','test','capture','libnano_core.a']];before=tool(['sha256sum']+parents);(logs/'parent-before.sha256').write_text(before)
qualified=repo/'docs/qualification/capella-published-signed-inventory-20261007'/(arch+'-build/programs.sha256');assert before==qualified.read_text()
run('stage',['sh','-c','test ! -e '+out+' && mkdir '+out+' && cp '+parent+'/libnano_core.a '+out+'/libnano_core.a'])
def link(name,target,obj,library):
 args=shlex.split(tool(['cat',old+'/CMakeFiles/'+target+'.dir/link.txt']));positions=[i for i,v in enumerate(args) if v.endswith('.cpp.o')];assert len(positions)==1;args[positions[0]]=obj;args=[library if x=='libnano_core.a' else x for x in args];args[args.index('-o')+1]=out+'/'+name;run('link-'+name,args)
run('compile-fixture',['/usr/bin/clang++',*flags('nano-capella-transition-tests'),'-I/work/tests','-c','/patch/test_capella_transition.cpp','-o',out+'/test.o'])
link('parent-test','nano-capella-transition-tests',out+'/test.o',parent+'/libnano_core.a')
env={'NANO_TEST_FRAGMENT_CHECKPOINT':'1','NANO_TEST_COLD_REACTOR':'1','NANO_TEST_PUBLICATION_CANCEL_BOUNDARY':'1'}
run('parent-counterexample',[out+'/parent-test','--prepared-block'],env,1);assert 'cancellation callback after durable witness intent' in (logs/'parent-counterexample.log').read_text()
obj=out+'/consensus_capella_reactor.cpp.o';run('compile-reactor',['/usr/bin/clang++',*flags('nano_core'),'-I/work/src','-MD','-MF',out+'/reactor.d','-c','/patch/consensus_capella_reactor.cpp','-o',obj])
members=tool(['/usr/bin/ar','t',out+'/libnano_core.a']).splitlines();assert members.count('consensus_capella_reactor.cpp.o')==1
run('replace-reactor',['/usr/bin/ar','r',out+'/libnano_core.a',obj]);assert tool(['/usr/bin/ar','t',out+'/libnano_core.a']).splitlines()==members
link('test','nano-capella-transition-tests',out+'/test.o',out+'/libnano_core.a')
run('draft-boundary-and-restart',[out+'/test','--prepared-block'],env)
run('cold-storage-fail-closed-guards',[out+'/test','--prepared-block'],{**env,'NANO_TEST_DUPLICATE_GUARDS':'1'})
for name,target in [('nano','nano'),('capture','nano-capella-paused-capture')]:link(name,target,parent+'/'+name+'.o',out+'/libnano_core.a')
after=tool(['sha256sum']+parents);assert before==after;(logs/'parent-after.sha256').write_text(after)
(logs/'programs.sha256').write_text(tool(['sha256sum']+[out+'/'+n for n in ['nano','test','capture','libnano_core.a']]))
(logs/'binding.json').write_text(json.dumps({'changedProductionSource':'src/consensus_capella_reactor.cpp','headerOrAbiChanged':False,'parentUnchanged':True,'parentCounterexampleExit':1,'draftExit':0,'storageFailClosedGuards':True,'networkQualified':False,'cleanReleaseQualified':False},indent=2)+'\n');print('Scoped Docker qualification complete',flush=True)
