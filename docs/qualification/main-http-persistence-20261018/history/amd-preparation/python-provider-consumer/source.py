
import subprocess,json,uuid,re,tempfile
image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';name='nano-python-consumer-probe-'+uuid.uuid4().hex;label='nano.python.consumer.probe';client=tempfile.TemporaryDirectory(prefix='nano-python-probe-');prefix=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',client.name];cid=None;clean=False;result=None

from pathlib import Path
import hashlib,os
control=Path('/home/ubuntu/main-http-python-provider-root-20261018-02/control.json');cb=control.read_bytes();assert len(cb)==241262 and hashlib.sha256(cb).hexdigest()=='6d1969aac717c7f980a6845756158d52fe5ce0436c1c13511094ac019df97100';c=json.loads(cb);root=Path(c['runtimeRoot']);F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def fence():
 for rel,w in c['files'].items():
  p=root/rel;st=p.stat();got={k:getattr(st,k) for k in F};got['sha256']=hashlib.sha256(p.read_bytes()).hexdigest();assert got==w
fence()
try:
 r=subprocess.run(prefix+['create','--pull=never','--name',name,'--label',label+'='+name,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','16','--memory','128m','--mount','type=bind,src='+str(root)+',dst=/python-runtime,readonly',*[x for k,v in dict(c['environment'],PYTHONPATH='').items() for x in ('--env',k+'='+v)],'--entrypoint','/python-runtime/bin/python3',image,'-S','-c','import argparse,hashlib,http.client,json,os,pathlib,re,signal,stat,subprocess,sys,time,uuid;print(json.dumps({"version":sys.version.split()[0],"stdlib":json.__file__,"socket":http.client.__file__}))'],capture_output=True,timeout=20);assert r.returncode==0 and re.fullmatch(rb'[0-9a-f]{64}\n',r.stdout);cid=r.stdout.decode().strip()
 r=subprocess.run(prefix+['start','--attach',cid],capture_output=True,timeout=20)
 x=json.loads(subprocess.check_output(prefix+['inspect',cid]))[0];assert x['Id']==cid and x['Name']=='/'+name and x['Image']==image and x['Config']['Labels'][label]==name and not x['State']['Running']
 result={'format':'nano-amd-readonly-python-provider-consumer-v1','consumerAvailable':r.returncode==0,'actualExitCode':r.returncode,'containerExitCode':x['State']['ExitCode'],'stdout':r.stdout.decode(),'stderr':r.stderr.decode(),'image':image,'nanoProcessesStarted':0,'httpRequests':0,'newImagesBuilt':0,'productionQualified':False}
finally:
 if cid:
  x=json.loads(subprocess.check_output(prefix+['inspect',cid]))[0];assert x['Name']=='/'+name and x['Image']==image and x['Config']['Labels'][label]==name
  subprocess.run(prefix+['rm','-f',cid],check=True,capture_output=True,timeout=20)
  q=subprocess.run(prefix+['inspect',cid],capture_output=True,timeout=20);assert q.returncode==1;clean=True
 client.cleanup()
assert result is not None;fence();result['ownedContainerAbsent']=clean;result['providerFencePassed']=True;result['providerControlSha256']='6d1969aac717c7f980a6845756158d52fe5ce0436c1c13511094ac019df97100';result['imagePythonInstalled']=False;print(json.dumps(result))
