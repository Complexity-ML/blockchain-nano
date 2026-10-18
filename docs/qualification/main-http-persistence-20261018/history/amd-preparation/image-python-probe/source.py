
import subprocess,json,uuid,re,tempfile
image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';name='nano-python-consumer-probe-'+uuid.uuid4().hex;label='nano.python.consumer.probe';client=tempfile.TemporaryDirectory(prefix='nano-python-probe-');prefix=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',client.name];cid=None;clean=False;result=None
try:
 r=subprocess.run(prefix+['create','--pull=never','--name',name,'--label',label+'='+name,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','16','--memory','128m','--entrypoint','/bin/sh',image,'-c','test -x /usr/bin/python3 && /usr/bin/python3 --version'],capture_output=True,timeout=20);assert r.returncode==0 and re.fullmatch(rb'[0-9a-f]{64}\n',r.stdout);cid=r.stdout.decode().strip()
 r=subprocess.run(prefix+['start','--attach',cid],capture_output=True,timeout=20)
 x=json.loads(subprocess.check_output(prefix+['inspect',cid]))[0];assert x['Id']==cid and x['Name']=='/'+name and x['Image']==image and x['Config']['Labels'][label]==name and not x['State']['Running']
 result={'format':'nano-amd-sdk-python-consumer-probe-v1','consumerAvailable':r.returncode==0,'actualExitCode':r.returncode,'containerExitCode':x['State']['ExitCode'],'stdout':r.stdout.decode(),'stderr':r.stderr.decode(),'image':image,'nanoProcessesStarted':0,'httpRequests':0,'newImagesBuilt':0,'productionQualified':False}
finally:
 if cid:
  x=json.loads(subprocess.check_output(prefix+['inspect',cid]))[0];assert x['Name']=='/'+name and x['Image']==image and x['Config']['Labels'][label]==name
  subprocess.run(prefix+['rm','-f',cid],check=True,capture_output=True,timeout=20)
  q=subprocess.run(prefix+['inspect',cid],capture_output=True,timeout=20);assert q.returncode==1;clean=True
 client.cleanup()
assert result is not None;result['ownedContainerAbsent']=clean;print(json.dumps(result))
