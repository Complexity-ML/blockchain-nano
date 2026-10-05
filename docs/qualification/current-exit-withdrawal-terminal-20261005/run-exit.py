import pathlib,subprocess,os,json,hashlib,sys,time
repo=pathlib.Path('/Users/boris/Dev/blockchain-nano')
root=pathlib.Path('/private/tmp/nano-exit-wan-current-control-20261005')
output=pathlib.Path('/private/tmp/nano-exit-wan-current-20261005')
revision='2c8fc30a3c5c0a7c78bfca931f5bcc47fe036f13'
manifest=repo/'docs/qualification/exit-artifact-preflight-20261005/artifacts.json'
rotation=pathlib.Path('/private/tmp/nano-monotonic-rotation-control-20261005')
assert json.loads((rotation/'state.json').read_text())['phase']=='terminal','Rotation is still running: no overlapping campaign'
assert not output.exists(),'Do not restart or reuse an output directory'
assert not root.exists(),'Do not overwrite campaign controller evidence'
root.mkdir()
def save(n,v):(root/n).write_text(json.dumps(v,indent=2)+'\n')
try:
 actual=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
 assert actual==revision,'Harness revision changed'
 artifacts=json.loads(manifest.read_text())
 for e in artifacts['native'].values():assert hashlib.sha256(pathlib.Path(e['path']).read_bytes()).hexdigest()==e['sha256'],'Native program changed'
 env={k:v for k,v in os.environ.items() if not k.startswith('NANO_TEST_')}
 env.update({'PATH':'/tmp/nano-wan-dual-family-transport-control-20261004/bin:'+os.environ['PATH'],'NANO_TEST_EXIT_ARTIFACT_MANIFEST':str(manifest),'NANO_TEST_WALL_CLOCK_DRIFT_MS':'250'})
 save('preflight.json',{'harnessRevision':revision,'runtimeRevision':artifacts['runtimeRevision'],'artifactManifestSha256':hashlib.sha256(manifest.read_bytes()).hexdigest(),'physicalHosts':2,'nodes':2,'syntheticValidators':64,'privateSshTransport':True,'imagesReused':True,'wallClockDriftMs':250,'testOnlyAutomaticRecovery':True,'testOnlyShortenedWithdrawalDelay':True,'newImageBuild':False,'publicDeployment':False,'productionQualified':False})
 save('state.json',{'phase':'running','pid':os.getpid(),'startedAt':time.time(),'output':str(output),'productionQualified':False})
 with (root/'driver.log').open('w') as log:
  process=subprocess.run(['node','tests/capella/exit-multihost.mjs','ubuntu@145.239.199.166',revision,str(output)],cwd=repo,env=env,stdout=log,stderr=subprocess.STDOUT)
 (root/'controller-exit.txt').write_text(str(process.returncode)+'\n')
 save('state.json',{'phase':'terminal','exit':process.returncode,'output':str(output),'productionQualified':False})
 sys.exit(process.returncode)
except Exception as e:
 save('failure.json',{'error':str(e)})
 save('state.json',{'phase':'terminal','exit':1,'output':str(output),'productionQualified':False})
 sys.exit(1)
