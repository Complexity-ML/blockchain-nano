from pathlib import Path
import json,subprocess,sys,shlex,hashlib
r=Path(__file__).parent;arch=sys.argv[1];assert arch in ['arm64','amd64'];logs=r/('docker-'+arch);logs.mkdir()
if arch=='arm64':
 docker=['/Users/boris/.docker/bin/docker'];repo=Path('/Users/boris/Dev/blockchain-nano');image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';container='nano-restored-three-node-mac-muxfgr38';audit=Path('/private/tmp/nano-v36-post-stop-readonly-20261007/mac-docker/immutability.json')
else:
 docker=['sudo','-n','docker'];repo=Path('/home/ubuntu/blockchain-nano-git');image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';container='nano-restored-three-node-vps-muxfgr38';audit=Path('/tmp/nano-v36-post-stop-readonly-20261007/immutability.json')
s=json.loads(subprocess.check_output(docker+['inspect',container,'--format','{{json .State}}'],text=True));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled'];assert json.loads(audit.read_text())['exactStopCommitmentsVerified']
binding=json.loads((r/'input-binding.json').read_text())
for n,h in binding['parentHeaders'].items():assert hashlib.sha256((repo/'network'/n).read_bytes()).hexdigest()==h
for n,h in binding['draftHeaders'].items():assert hashlib.sha256((r/'network'/n).read_bytes()).hexdigest()==h
out='/build/dependency-route-'+arch+'-20261007';old='/build/coherent-e33ad89e-'+arch+'/network';parent='/build/core-trace-bound-'+arch+'-20261007/network/nano-p2p'
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','-w',old,'-v',volume+':/build','-v',str(repo)+':/work:ro','-v',str(r)+':/patch:ro']
def read(path):return subprocess.check_output(base+['--entrypoint','cat',image,path],text=True)
def run(name,args,timeout=None):
 (logs/(name+'-command.json')).write_text(json.dumps({'args':args,'cwd':old})+'\n')
 with (logs/(name+'.log')).open('w') as f:
  try:p=subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT,timeout=timeout);code=p.returncode
  except subprocess.TimeoutExpired:code='driver-timeout'
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':code})+'\n');assert code==0,(name,code);print(name,'exit0',flush=True)
before=subprocess.check_output(base+['--entrypoint','sha256sum',image,parent],text=True)
run('stage',['sh','-c','test ! -e '+out+' && mkdir -p '+out+'/source && cp -R /work/network '+out+'/source/ && cp /patch/network/*.hpp '+out+'/source/network/ && cp /patch/route-adversarial/test.cpp '+out+'/source/network/route-adversarial.cpp && cp /patch/churn-draft/profile.cpp '+out+'/source/network/route-churn.cpp'])
rows=read(old+'/CMakeFiles/nano-p2p.dir/flags.make').splitlines();flags=[]
for key in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']:flags+=shlex.split(next(x.split(' = ',1)[1] for x in rows if x.startswith(key+' = ')))
linktemplate=shlex.split(read(old+'/CMakeFiles/nano-p2p.dir/link.txt'))
consumers=sorted(json.loads((r/'affected-source-consumers.json').read_text()));assert len(consumers)==13
macros={'test_capella_dependency_preparation.cpp':'NANO_TEST_DEPENDENCY_PREPARATION','test_capella_export_resume.cpp':'NANO_TEST_EXPORT_CACHE','test_capella_head_history_fairness.cpp':'NANO_HEAD_HISTORY_FIRST'}
for source in consumers+['route-adversarial.cpp','route-churn.cpp']:
 name=Path(source).stem;obj=out+'/'+name+'.o';binary=out+'/'+name;extra=['-D'+macros[source]] if source in macros else []
 run(name+'-compile',['/usr/bin/clang++','-I'+out+'/source/network',*flags,*extra,'-MMD','-MF',out+'/'+name+'.d','-c',out+'/source/network/'+source,'-o',obj])
 a=linktemplate.copy();pos=[i for i,x in enumerate(a) if x.endswith('.cpp.o')];assert len(pos)==1;a[pos[0]]=obj;a[a.index('-o')+1]=binary;run(name+'-link',a)
 if source=='main.cpp':continue
 modes=['normal','duplicate','unavailable','unavailable-anchor','refused','lost','cancel','slow','slow-duplicate','slow-one-step'] if source=='test_capella_dependency_preparation.cpp' else ['anchor','static','moving','reset','refused'] if source=='test_capella_deep_preparation.cpp' else ['2049'] if source=='route-churn.cpp' else [None]
 for mode in modes:run(name+'-'+(mode or 'default'),[binary]+([] if mode is None else [mode]))
after=subprocess.check_output(base+['--entrypoint','sha256sum',image,parent],text=True);assert before==after
run('programs-sha256',['sha256sum']+[out+'/'+Path(f).stem for f in consumers+['route-adversarial.cpp','route-churn.cpp']])
(logs/'binding.json').write_text(json.dumps({'parentProgram':before.strip(),'parentUnchanged':True,'sourceConsumers':consumers,'draftHeaders':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (r/'network').glob('*.hpp')},'newMainCompiledLinkedOnly':True,'networkQualified':False,'productionQualified':False},indent=2)+'\n')
print('affected relay consumers and configured scenarios qualified; WAN finality not qualified',flush=True)
