from pathlib import Path
import subprocess,shlex,json,sys,hashlib
arch=sys.argv[1];assert arch in ('arm64','amd64');root=Path(__file__).parent;probe=root/(arch+'-build');probe.mkdir()
old='/build/coherent-e33ad89e-'+arch;parent='/build/published-block-inventory-'+arch+'-20261007';out='/build/startup-stage-diagnostic-'+arch+'-20261007'
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';source='/Users/boris/Dev/blockchain-nano'
else:docker=['sudo','-n','docker'];image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';source='/home/ubuntu/blockchain-nano-git'
m=json.loads((root/'manifest.json').read_text())
for rel,h in m['files'].items():assert hashlib.sha256((Path(source)/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((root/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',volume+':/build','-v',source+':/work:ro','-v',str(root/'source')+':/patch:ro','-w',old]
def tool(args):return subprocess.check_output(base+['--entrypoint',args[0],image]+args[1:],text=True)
def run(name,args,env=None):
 (probe/(name+'-command.json')).write_text(json.dumps({'args':args,'env':env or {}})+'\n')
 with (probe/(name+'.log')).open('w') as f:r=subprocess.run(base+sum([['-e',k+'='+v] for k,v in (env or {}).items()],[])+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (probe/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,(name,r.returncode);print(name+' exit0',flush=True)
parents=[parent+'/'+x for x in ['nano','test','capture','libnano_core.a']];before=tool(['sha256sum']+parents);(probe/'parent-before.sha256').write_text(before)
run('stage',['sh','-c','mkdir -p '+out+'/source && cp -R /patch/src '+out+'/source/ && ln -s /work/network '+out+'/source/network && cp '+parent+'/libnano_core.a '+out+'/libnano_core.a'])
lines=tool(['cat',old+'/CMakeFiles/nano_core.dir/flags.make']).splitlines();flags=sum([shlex.split(next(l.split(' = ',1)[1] for l in lines if l.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
run('compile-runtime',['/usr/bin/clang++']+flags+['-I/work/src','-c',out+'/source/src/consensus_capella_runtime.cpp','-o',out+'/consensus_capella_runtime.cpp.o'])
run('replace-runtime',['/usr/bin/ar','r',out+'/libnano_core.a',out+'/consensus_capella_runtime.cpp.o'])
for target,name in [('nano','nano'),('nano-capella-transition-tests','test')]:
 args=shlex.split(tool(['cat',old+'/CMakeFiles/'+target+'.dir/link.txt']));args=[out+'/libnano_core.a' if a=='libnano_core.a' else parent+'/fixture.o' if a.endswith('/tests/test_capella_transition.cpp.o') else a for a in args];args[args.index('-o')+1]=out+'/'+name;run('link-'+name,args)
run('configured-runtime-trace',[out+'/test','--configured-runtime'],{'NANO_TEST_NETWORK_TRACE':'1'})
rows=[]
for line in (probe/'configured-runtime-trace.log').read_text().splitlines():
 try:x=json.loads(line)
 except json.JSONDecodeError:continue
 if x.get('event')=='capella-startup-stage':rows.append(x)
assert rows and any(x['stage']=='runtime-ready' for x in rows)
assert all(isinstance(x['elapsedMs'],int) and x['elapsedMs']>=0 and x['previousElapsedMs']>=0 for x in rows)
allowed={'event','stage','elapsedMs','previousStage','previousElapsedMs'};assert all(set(x)==allowed for x in rows)
(probe/'trace-check.json').write_text(json.dumps({'stagesObserved':sorted(set(x['stage'] for x in rows)),'rows':len(rows),'timingNonnegative':True,'onlyAllowedFields':True,'networkQualified':False})+'\n')
after=tool(['sha256sum']+parents);assert after==before;(probe/'parent-after.sha256').write_text(after);(probe/'programs.sha256').write_text(tool(['sha256sum',out+'/nano',out+'/test',out+'/libnano_core.a']));print('Diagnostic draft compiled and configured runtime checked; parent programs unchanged; no node started',flush=True)
