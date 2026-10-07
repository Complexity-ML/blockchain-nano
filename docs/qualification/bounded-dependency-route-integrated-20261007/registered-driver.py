from pathlib import Path
import json,subprocess,sys,shlex,hashlib
r=Path(__file__).parent;arch=sys.argv[1];assert arch in ['native','arm64','amd64'];logs=r/('registered-'+arch);logs.mkdir()
if arch=='native':
 old=Path('/private/tmp/nano-long-dependency-profile-20261007');source=r/'registered-fixture/test_capella_dependency_route.cpp';obj=logs/'program.o';binary=logs/'program'
 for stage in ['compile','link']:
  a=json.loads((old/f'actual-{stage}-command.json').read_text());a=[str(source) if x==str(old/'profile-actual.cpp') else str(obj) if x==str(old/'profile-actual.o') else str(binary) if x==str(old/'profile-actual') else x for x in a]
  if stage=='compile':a.insert(1,'-I'+str(r/'network'))
  (logs/f'{stage}-command.json').write_text(json.dumps(a)+'\n')
  with (logs/f'{stage}.log').open('w') as f:p=subprocess.run(a,stdout=f,stderr=subprocess.STDOUT)
  (logs/f'{stage}-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,stage
 for mode in ['churn','adversarial']:
  with (logs/f'{mode}.log').open('w') as f:p=subprocess.run([str(binary),mode],stdout=f,stderr=subprocess.STDOUT)
  (logs/f'{mode}-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,mode
 print('registered native fixture exit0',flush=True);sys.exit(0)
if arch=='arm64':
 docker=['/Users/boris/.docker/bin/docker'];repo=Path('/Users/boris/Dev/blockchain-nano');image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002'
else:
 docker=['sudo','-n','docker'];repo=Path('/home/ubuntu/blockchain-nano-git');image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002'
assert (r/('docker-'+arch)/'binding.json').exists(),'Original affected driver not completed successfully'
out='/build/dependency-route-'+arch+'-20261007';work=out+'/registered-fixture';source=out+'/source/network';base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','-w',work,'-v',volume+':/build','-v',str(repo)+':/work:ro','-v',str(r)+':/patch:ro']
# First staging has a different cwd because the new directory must not exist yet.
stage=base.copy();stage[stage.index('-w')+1]=out
cmd=['sh','-c','test ! -e '+work+' && mkdir '+work+' && cp /patch/registered-fixture/test_capella_dependency_route.cpp '+source+'/ && cp /patch/registered-fixture/CMakeLists.txt '+source+'/ && ln -s /work/third_party '+out+'/source/third_party']
with (logs/'stage.log').open('w') as f:p=subprocess.run(stage+['--entrypoint',cmd[0],image]+cmd[1:],stdout=f,stderr=subprocess.STDOUT)
(logs/'stage-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0

def run(name,args,expected=0):
 (logs/(name+'-command.json')).write_text(json.dumps(args)+'\n')
 with (logs/(name+'.log')).open('w') as f:p=subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode,'expected':expected})+'\n');assert p.returncode==expected,(name,p.returncode);print(name,p.returncode,flush=True)
run('configure',['cmake','-S',source,'-B',work+'/cmake','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DCMAKE_C_COMPILER=/usr/bin/clang','-DNANO_DEPS_DIR=/src/.deps'])
run('build',['cmake','--build',work+'/cmake','--target','nano-capella-dependency-route-tests','--parallel','1'])
run('ctest',['ctest','--test-dir',work+'/cmake','-R','^capella_dependency_route_','--no-tests=error','--output-on-failure','--output-junit',work+'/results.xml'])
run('registry',['sh','-c','mkdir ci-reports && ctest --test-dir '+work+'/cmake --show-only=json-v1 > ci-reports/network-selection.json'])
run('old-registry-counterexample',['cmake','-DNANO_CI_CHECK=registry','-P','/patch/registered-fixture/check-network-parent.cmake'],1)
assert 'Network CTest count differs' in (logs/'old-registry-counterexample.log').read_text()
run('new-registry',['cmake','-DNANO_CI_CHECK=registry','-P','/patch/registered-fixture/check-network.cmake'])
for name,path in [('selection',work+'/ci-reports/network-selection.json'),('junit',work+'/results.xml'),('test-program-hash',work+'/cmake/nano-capella-dependency-route-tests')]:
 a=['sha256sum',path] if name=='test-program-hash' else ['cat',path]
 run(name,a)
(logs/'binding.json').write_text(json.dumps({'architecture':arch,'fixtureSourceSHA256':hashlib.sha256((r/'registered-fixture/test_capella_dependency_route.cpp').read_bytes()).hexdigest(),'cmakeSourceSHA256':hashlib.sha256((r/'registered-fixture/CMakeLists.txt').read_bytes()).hexdigest(),'registry49Verified':True,'twoRegisteredScenariosExecuted':True,'otherCTestExecutionsClaimed':False,'networkQualified':False},indent=2)+'\n')
