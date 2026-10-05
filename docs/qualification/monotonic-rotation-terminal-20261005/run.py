import pathlib,subprocess,os,json,hashlib,sys,time
root=pathlib.Path(__file__).parent;repo=pathlib.Path('/Users/boris/Dev/blockchain-nano');output='/tmp/nano-monotonic-rotation-20261005'
assert not pathlib.Path(output).exists()
def save(n,v):(root/n).write_text(json.dumps(v,indent=2)+'\n')
try:
 programs=json.loads(pathlib.Path('/private/tmp/nano-monotonic-runtime-native-build-20261005/programs.json').read_text())
 for p in ['/tmp/nano-monotonic-runtime-native-build-20261005/nano-capella-rotation-audit','/tmp/nano-coherent-native-build-20261004/network/nano-p2p']:programs[p]=hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
 for p,h in programs.items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==h
 env={k:v for k,v in os.environ.items() if not k.startswith('NANO_TEST_')}
 env.update({'PATH':'/tmp/nano-wan-dual-family-transport-control-20261004/bin:'+os.environ['PATH'],'NANO_TEST_PEER_SYNCING':'1','NANO_TEST_REMOTE_SOURCE_REPOSITORY':'/home/ubuntu/nano-monotonic-pruned-load-v3-source-20261005','NANO_TEST_CONSENSUS_PROFILE':'capella','NANO_TEST_BUILD_VOLUME':'1','NANO_TEST_NETWORK_WITH_CORE':'1','NANO_TEST_BUILD_SUBDIRECTORY_ARM64':'monotonic-clock-arm64-20261005','NANO_TEST_BUILD_SUBDIRECTORY_AMD64':'monotonic-clock-amd64-20261005','NANO_TEST_BUILDER_IMAGE_ARM64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','NANO_TEST_BUILDER_IMAGE_AMD64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','NANO_TEST_BUILD_VOLUME_ARM64':'nano-clean-arm64-build-20261002','NANO_TEST_BUILD_VOLUME_AMD64':'nano-clean-rpc-build-20261002','NANO_TEST_NATIVE_BUILD_DIRECTORY':'/tmp/nano-monotonic-runtime-native-build-20261005','NANO_TEST_NATIVE_NETWORK_DIRECTORY':'/tmp/nano-coherent-native-build-20261004/network','NANO_TEST_WALL_CLOCK_DRIFT_MS':'250','NANO_TEST_WAN_PARTITION':'1','NANO_TEST_WAN_ROTATION':'1','NANO_DOCKER_NOFILE':'65536'})
 save('preflight.json',{'harnessRevision':'0950c18e','runtimeRevision':'ab10b8ae','nativeProgramsRehashed':programs,'existingBuilderImagesAndVolumes':True,'newImageBuild':False,'rotationAndPartition':True,'backupRestore':False,'historicalStatePruning':False,'boundedLoad':False,'wallClockDriftMs':250,'physicalHosts':2,'nodes':3,'syntheticValidators':64,'publicDeployment':False,'productionQualified':False})
 save('state.json',{'phase':'running','pid':os.getpid(),'startedAt':time.time(),'output':output,'productionQualified':False})
 with (root/'driver.log').open('w') as log:p=subprocess.run(['node','tests/multihost/run.mjs','ubuntu@145.239.199.166','ab10b8ae5dd50c7edf6f919ed440aa51a46e5e0e',output],cwd=repo,env=env,stdout=log,stderr=subprocess.STDOUT)
 (root/'controller-exit.txt').write_text(str(p.returncode)+'\n');save('state.json',{'phase':'terminal','exit':p.returncode,'output':output,'productionQualified':False});sys.exit(p.returncode)
except Exception as e:
 save('failure.json',{'error':str(e)});save('state.json',{'phase':'terminal','exit':1,'productionQualified':False});sys.exit(1)
