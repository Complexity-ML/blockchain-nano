#!/usr/bin/env python3
"""Synthetic SST6 cross-reader gate; never opens existing chain data."""
import os,json,pathlib,subprocess,shlex,hashlib,sys
REV='efbeb9bca3d42e41edb9cb81afba98c0b953a90f'
TARGET='ubuntu@145.239.199.166'
ENV={**os.environ,'PATH':'/tmp/nano-wan-dual-family-transport-control-20261004/bin:'+os.environ['PATH']}
ROOTS={'arm64':'/tmp/nano-coherent-sst-cross-20261004','amd64':'/home/ubuntu/nano-coherent-sst-cross-20261004'}
SOURCES={'arm64':'/tmp/nano-production-coherent-20261004','amd64':'/home/ubuntu/nano-production-coherent-20261004'}
REPORTS={'arm64':'/tmp/nano-coherent-full-arm64-reports-20261004','amd64':'/home/ubuntu/nano-coherent-full-amd64-reports-20261004'}
IMAGES={'arm64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','amd64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'}
VOLUMES={'arm64':'nano-clean-arm64-build-20261002','amd64':'nano-clean-rpc-build-20261002'}
def require(ok,msg):
 if not ok:raise RuntimeError(msg)
def call(args,arch='arm64',timeout=60):
 if arch=='amd64':args=['ssh','-o','ConnectTimeout=10',TARGET,shlex.join(args)]
 r=subprocess.run(args,env=ENV,text=True,capture_output=True,timeout=timeout)
 require(r.returncode==0,'command exit '+str(r.returncode)+': '+r.stderr[:600]);return r.stdout.strip()
def hostpy(script,args,arch):return call(['python3','-B','-c',script,*args],arch)
def tree(path,arch):
 s='import pathlib,json,hashlib,sys;p=pathlib.Path(sys.argv[1]);fs=list(p.rglob("*"));assert p.is_dir() and not any(f.is_symlink() for f in fs);print(json.dumps({str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in fs if f.is_file()},sort_keys=True))'
 return json.loads(hostpy(s,[path],arch))
def docker(args,arch):return call((['docker'] if arch=='arm64' else ['sudo','-n','docker'])+args,arch)
def helper(mode,store,arch):
 args=['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user',call(['id','-u'],arch)+':'+call(['id','-g'],arch),'--cpus','1','--memory','512m','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=32m','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build,readonly','--mount','type=bind,src='+ROOTS[arch]+',dst=/proof'+(',readonly' if mode=='verify' else ''),IMAGES[arch],'/build/coherent-efbeb9bc-'+arch+'/nano-storage-portability',mode,'/proof/'+store]
 result=json.loads(docker(args,arch))
 require(result['sequence']==1 and result['payload']=='portable-checkpoint-v1' and result['readOnlyWriteRefused'],'wrong checkpoint or writable read-only API')
 require(result['digest']=='0x8f301145ac2dcc4e0aa9b4716680da4abeabaa674c31b0cbe83b9f5fa4ed8cd2','independent known fixture commitment mismatch')
 return result
pins={}
for arch in ('arm64','amd64'):
 require(call(['git','-C',SOURCES[arch],'rev-parse','HEAD'],arch)==REV,'wrong source revision')
 require(not call(['git','-C',SOURCES[arch],'status','--porcelain'],arch),'dirty source')
 gate='import pathlib,json,sys;p=pathlib.Path(sys.argv[1]);assert (p/"inventory.json").is_file();a=json.loads((p/"source-manifest.json").read_text());b=json.loads((p/"source-manifest-after-build.json").read_text());assert a==b;print(len(a))'
 count=hostpy(gate,[REPORTS[arch]],arch)
 require(json.loads(docker(['image','inspect',IMAGES[arch]],arch))[0]['Id']==IMAGES[arch],'wrong image')
 path='/build/coherent-efbeb9bc-'+arch+'/nano-storage-portability'
 digest=docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build,readonly',IMAGES[arch],'sha256sum',path],arch).split()[0]
 require(len(digest)==64,'missing helper digest');pins[arch]=dict(helperSha256=digest,sourceFiles=int(count),image=IMAGES[arch])
for arch in ('arm64','amd64'):hostpy('import pathlib,sys;pathlib.Path(sys.argv[1]).mkdir(exist_ok=False)',[ROOTS[arch]],arch)
root=pathlib.Path(ROOTS['arm64'])
def save(n,x):(root/n).write_text(json.dumps(x,indent=2,sort_keys=True)+'\n')
save('pins-before.json',pins)
try:
 results={};before={}
 for arch in ('arm64','amd64'):
  results[arch+'-create']=helper('create','written',arch)
  before[arch]=tree(ROOTS[arch]+'/written',arch)
  require(any(p.endswith('.sst') for p in before[arch]),'no physical SST')
  # Independently read actual recorded writer settings, not only helper assertion.
  options=hostpy('import pathlib,sys;print("\\n".join(p.read_text() for p in pathlib.Path(sys.argv[1]).glob("OPTIONS-*")))',[ROOTS[arch]+'/written'],arch)
  require('format_version=6' in options and 'format_version=7' not in options,'recorded format is not6')
  (root/(arch+'-writer-options.txt')).write_text(options)
 call(['scp','-r',ROOTS['arm64']+'/written',TARGET+':'+ROOTS['amd64']+'/arm64-copy'])
 call(['scp','-r',TARGET+':'+ROOTS['amd64']+'/written',ROOTS['arm64']+'/amd64-copy'])
 transferred={'arm64-to-amd64':tree(ROOTS['amd64']+'/arm64-copy','amd64'),'amd64-to-arm64':tree(ROOTS['arm64']+'/amd64-copy','arm64')}
 require(transferred['arm64-to-amd64']==before['arm64'] and transferred['amd64-to-arm64']==before['amd64'],'transfer changed files')
 results['arm64-to-amd64']=helper('verify','arm64-copy','amd64')
 results['amd64-to-arm64']=helper('verify','amd64-copy','arm64')
 after={'arm64-to-amd64':tree(ROOTS['amd64']+'/arm64-copy','amd64'),'amd64-to-arm64':tree(ROOTS['arm64']+'/amd64-copy','arm64')}
 require(after==transferred,'read-only cross-reader changed files')
 for arch in ('arm64','amd64'):
  require(tree(ROOTS[arch]+'/written',arch)==before[arch],'original store changed')
  require(call(['git','-C',SOURCES[arch],'rev-parse','HEAD'],arch)==REV and not call(['git','-C',SOURCES[arch],'status','--porcelain'],arch),'source changed')
  digest=docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount','type=volume,src='+VOLUMES[arch]+',dst=/build,readonly',IMAGES[arch],'sha256sum','/build/coherent-efbeb9bc-'+arch+'/nano-storage-portability'],arch).split()[0]
  require(digest==pins[arch]['helperSha256'],'helper changed')
 save('results.json',results);save('files-before.json',before);save('cross-files-before.json',transferred);save('cross-files-after.json',after)
 save('result.json',dict(exit=0,revision=REV,physicalCrossReadersPassed=True,productionQualified=False,fullRegressionResultInherited=False,nativeReaderTested=False,overlapsFullRegression=True))
 print('Physical SST6 cross-reader gate passed on coherent ARM64/AMD64 programs')
except Exception as e:
 save('failure.json',dict(error=str(e),productionQualified=False));raise
