import pathlib,subprocess,json,hashlib,shlex,os,time,sys
ROOT=pathlib.Path(__file__).resolve().parent;REV='092a153ca1d203434beb14fe2102e3a092dc454c';TARGET='ubuntu@145.239.199.166';SOURCE=pathlib.Path('/tmp/nano-pruning-wan-source-20261004');REMOTE='/home/ubuntu/nano-pruning-wan-source-20261004';OUTPUT='/tmp/nano-pruned-wan-restore-092a153c-20261004';ENV={**os.environ,'PATH':'/tmp/nano-wan-dual-family-transport-control-20261004/bin:'+os.environ['PATH']};CONTROLS={'native':'/tmp/nano-pruning-wan-native-control-20261004','arm64':'/tmp/nano-pruning-wan-delivery-control-20261004','amd64':'/home/ubuntu/nano-pruning-wan-delivery-control-20261004'};IMAGES={'arm64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','amd64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'};VOLUMES={'arm64':'nano-clean-arm64-build-20261002','amd64':'nano-clean-rpc-build-20261002'}
def save(n,v):(ROOT/n).write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def call(args,host='native',timeout=45):
 if host=='amd64':args=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=10',TARGET,shlex.join(args)]
 p=subprocess.run(args,env=ENV,text=True,capture_output=True,timeout=timeout)
 if p.returncode:raise RuntimeError((args,p.returncode,p.stderr[-1200:]))
 return p.stdout.strip()
def state(host):return json.loads(call(['cat',CONTROLS[host]+'/state.json'],host))
def docker(args,arch):return call((['docker'] if arch=='arm64' else ['sudo','-n','docker'])+args,arch)
try:
 start=time.monotonic()
 while True:
  states={h:state(h) for h in CONTROLS}
  save('state.json',{'phase':'waiting-delivery','revision':REV,'wanStarted':False,'builds':states,'observedAt':time.time()})
  for h,s in states.items():
   if s['phase']=='terminal':
    if s['exit']!=0:raise RuntimeError(h+' prerequisite failed; no WAN started')
   else:
    suffix=CONTROLS[h]+'/run.py'+(' '+h if h!='native' else '')
    rows=call(['ps','-axo','pid,args'],h)
    if not any(row.strip().endswith(suffix) and ('Python -B ' in row or 'python3 -B ' in row) for row in rows.splitlines()):raise RuntimeError(h+' prerequisite handle absent; no restart')
  if all(s['phase']=='terminal' for s in states.values()):break
  if time.monotonic()-start>7200:raise RuntimeError('Observation wait expired; existing builds untouched')
  time.sleep(20)
 assert not pathlib.Path(OUTPUT).exists();proofs={}
 for arch in ['arm64','amd64']:
  assert not docker(['ps','-q'],arch),'No overlap with Docker qualification'
  directory='/build/pruning-wan-092a153c-'+arch
  result=json.loads(call(['cat',CONTROLS[arch]+'/result.json'],arch));assert result['revision']==REV and result['existingQualifiedCoreAndNetworkUnchanged']
  assert call(['cat',CONTROLS[arch]+'/baseline-before.txt'],arch)==call(['cat',CONTROLS[arch]+'/baseline-after.txt'],arch)
  assert call(['cat',CONTROLS[arch]+'/sources-before.json'],arch)==call(['cat',CONTROLS[arch]+'/sources-after.json'],arch)
  digest=docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build,readonly',IMAGES[arch],'sha256sum',directory+'/nano',directory+'/network/nano-p2p'],arch)
  assert digest==call(['cat',CONTROLS[arch]+'/programs.txt'],arch)
  assert call(['git','-C',str(SOURCE) if arch=='arm64' else REMOTE,'rev-parse','HEAD'],arch)==REV
  assert not call(['git','-C',str(SOURCE) if arch=='arm64' else REMOTE,'status','--porcelain'],arch)
  proofs[arch]={'runtimeSourceIdenticalTo':'4734f285','deliveryHashesRechecked':digest,'qualifiedCoreReused':True,'reports':CONTROLS[arch],'image':IMAGES[arch]}
 native=json.loads(pathlib.Path(CONTROLS['native']+'/result.json').read_text());assert native['revision']==REV and native['prunedRestorePassed']
 programs=json.loads(pathlib.Path(CONTROLS['native']+'/programs.json').read_text())
 for path,digest in programs.items():assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()==digest
 inputs=json.loads(pathlib.Path(CONTROLS['native']+'/sources-after.json').read_text());assert inputs==json.loads(pathlib.Path(CONTROLS['native']+'/sources-before.json').read_text())
 for path,digest in inputs.items():assert hashlib.sha256((SOURCE/path).read_bytes()).hexdigest()==digest
 save('preflight.json',{'revision':REV,'docker':proofs,'nativeSourceFilesRehashed':len(inputs),'nativeProgramsRehashed':programs,'historicalPruningRequested':True,'productionQualified':False})
 env={**ENV,'NANO_TEST_CONSENSUS_PROFILE':'capella','NANO_TEST_BUILD_VOLUME':'1','NANO_TEST_NETWORK_WITH_CORE':'1','NANO_TEST_BUILD_SUBDIRECTORY_ARM64':'pruning-wan-092a153c-arm64','NANO_TEST_BUILD_SUBDIRECTORY_AMD64':'pruning-wan-092a153c-amd64','NANO_TEST_BUILDER_IMAGE_ARM64':IMAGES['arm64'],'NANO_TEST_BUILDER_IMAGE_AMD64':IMAGES['amd64'],'NANO_TEST_BUILD_VOLUME_ARM64':VOLUMES['arm64'],'NANO_TEST_BUILD_VOLUME_AMD64':VOLUMES['amd64'],'NANO_TEST_NATIVE_BUILD_DIRECTORY':'/tmp/nano-pruning-wan-native-build-20261004','NANO_TEST_NATIVE_NETWORK_DIRECTORY':'/tmp/nano-coherent-native-build-20261004/network','NANO_TEST_REMOTE_SOURCE_REPOSITORY':REMOTE,'NANO_TEST_BACKUP_RESTORE':'1','NANO_TEST_HISTORICAL_STATE_PRUNING':'1','NANO_DOCKER_NOFILE':'65536'}
 save('state.json',{'phase':'restoring','revision':REV,'wanStarted':True,'output':OUTPUT,'historicalPruning':True,'productionQualified':False})
 with (ROOT/'restore-driver.log').open('w') as log:p=subprocess.run(['node','tests/multihost/run.mjs',TARGET,REV,OUTPUT],cwd=SOURCE,env=env,stdout=log,stderr=subprocess.STDOUT)
 save('state.json',{'phase':'terminal','exit':p.returncode,'revision':REV,'wanStarted':True,'output':OUTPUT,'productionQualified':False});sys.exit(p.returncode)
except Exception as e:
 save('failure.json',{'error':str(e),'productionQualified':False});current=json.loads((ROOT/'state.json').read_text()) if (ROOT/'state.json').exists() else {};save('state.json',{**current,'phase':'terminal','exit':1,'revision':REV});sys.exit(1)
