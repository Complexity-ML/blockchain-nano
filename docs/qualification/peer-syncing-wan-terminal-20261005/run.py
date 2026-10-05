import pathlib,subprocess,json,hashlib,os,sys,time
root=pathlib.Path(__file__).parent;repo=pathlib.Path('/Users/boris/Dev/blockchain-nano');output='/tmp/nano-peer-syncing-wan-20261005'
assert not pathlib.Path(output).exists()
def save(n,v):(root/n).write_text(json.dumps(v,indent=2)+'\n')
try:
 proof=json.loads(pathlib.Path('/tmp/nano-capella-sync-native-build-20261005/programs.json').read_text())
 for p,h in proof.items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==h
 env={**os.environ,'PATH':'/tmp/nano-wan-dual-family-transport-control-20261004/bin:'+os.environ['PATH'],'NANO_TEST_PEER_SYNCING':'1','NANO_TEST_REMOTE_SOURCE_REPOSITORY':'/home/ubuntu/nano-peer-syncing-wan-source-20261005','NANO_TEST_CONSENSUS_PROFILE':'capella','NANO_TEST_CANDIDATE_MANIFEST':str(repo/'docs/qualification/capella-sync-candidate-pair-20261005/selection.json'),'NANO_TEST_NATIVE_BUILD_DIRECTORY':'/tmp/nano-capella-sync-native-build-20261005','NANO_TEST_NATIVE_NETWORK_DIRECTORY':'/tmp/nano-coherent-native-build-20261004/network','NANO_TEST_BACKUP_RESTORE':'1','NANO_TEST_HISTORICAL_STATE_PRUNING':'1','NANO_DOCKER_NOFILE':'65536'}
 for key in ['NANO_TEST_BUILD_VOLUME','NANO_TEST_BUILD_SUBDIRECTORY','NANO_TEST_BUILD_SUBDIRECTORY_ARM64','NANO_TEST_BUILD_SUBDIRECTORY_AMD64','NANO_TEST_NETWORK_WITH_CORE']:env.pop(key,None)
 args=['node','tests/multihost/run.mjs','ubuntu@145.239.199.166','2cd9ab9d55a0c179c2d413441c787fae482bca26',output]
 save('preflight.json',{'nativeProgramsRehashed':proof,'harnessRevision':'3a3f3fb2','candidateRuntimeRevision':'2cd9ab9d','nodeCount':3,'physicalHostCount':2,'syntheticValidators':64,'publicDeployment':False,'sourceDataReuse':False,'newFixtures':True,'transport':'reviewed pinned same authorized VPS IPv6, SSH loopback','productionQualified':False})
 save('state.json',{'phase':'running','pid':os.getpid(),'output':output,'startedAt':time.time(),'productionQualified':False})
 with (root/'driver.log').open('w') as log:p=subprocess.run(args,cwd=repo,env=env,stdout=log,stderr=subprocess.STDOUT)
 save('state.json',{'phase':'terminal','exit':p.returncode,'output':output,'productionQualified':False});(root/'controller-exit.txt').write_text(str(p.returncode)+'\n');sys.exit(p.returncode)
except Exception as e:
 save('failure.json',{'error':str(e)});save('state.json',{'phase':'terminal','exit':1,'productionQualified':False});sys.exit(1)
