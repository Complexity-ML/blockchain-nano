#!/usr/bin/env python3
"""Root owns one disposable SDK container; source tests only, no node service."""
import hashlib,json,os,pathlib,subprocess,sys,time,traceback
D=pathlib.Path(sys.argv[1]);owner=sys.argv[2];worker=pathlib.Path(sys.argv[3]);worker_pin=sys.argv[4]
assert owner in ['arm-baseline','arm-candidate','amd-candidate']
assert hashlib.sha256(worker.read_bytes()).hexdigest()==worker_pin
amd=owner.startswith('amd');E=D.parent
docker=['/usr/bin/sudo','-n','/usr/bin/docker'] if amd else ['/Applications/Docker.app/Contents/Resources/bin/docker','--config',str(E/'packaging-docker-client-root-20261017'),'--host','unix:///Users/boris/.docker/run/docker.sock']
image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14' if amd else 'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
name='nano-status-'+owner+'-root-20261017-02';mode='baseline' if owner.endswith('baseline') else 'candidate'
def save(p,z):
    with p.open('x')as f:json.dump(z,f,sort_keys=True,indent=2);f.write('\n')
def cmd(args,**kwargs):return subprocess.run(args,check=True,timeout=60,**kwargs)
meta=json.loads(cmd(docker+['image','inspect',image],capture_output=True).stdout)[0]
assert meta['Id']==image and meta['Architecture']==('amd64' if amd else 'arm64')
source=D/mode;output=D/'outputs';assert source.is_dir() and output.is_dir()
args=docker+['run','-d','--name',name,'--network','none','--read-only','--user','1000:1000' if amd else '501:20','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--tmpfs','/tmp:rw,mode=1777,size=128m','--env','HOME=/tmp','--env','TMPDIR=/tmp','--env','LANG=C','--env','LC_ALL=C','--mount','type=bind,src='+str(source)+',dst='+str(source)+',readonly','--mount','type=bind,src='+str(output)+',dst='+str(output),'--entrypoint','/bin/sleep',image,'1500']
save(D/(owner+'-sdk-start-argv.json'),args)
cid=None;error=None;ret=None;cleanup=None;start=time.monotonic()
try:
    candidate=cmd(args,capture_output=True).stdout.decode().strip();assert len(candidate)==64 and all(c in '0123456789abcdef'for c in candidate);cid=candidate
    save(D/(owner+'-sdk-container.json'),{'id':cid,'name':name,'image':image})
    c=json.loads((D/'controls-v2'/(owner+'.json')).read_bytes());c['dockerPrefix']=docker+['exec',cid]
    control=D/(owner+'-exec-control.json');save(control,c);control.chmod(0o400);pin=hashlib.sha256(control.read_bytes()).hexdigest()
    with (D/(owner+'-exec-worker.log')).open('xb')as log:
        p=subprocess.run([sys.executable,str(worker),str(control),pin],stdout=log,stderr=subprocess.STDOUT,timeout=1200)
    ret=p.returncode
    if ret:raise RuntimeError('host worker exit '+str(ret))
except BaseException as e:
    error=str(e);traceback.print_exc()
finally:
    if cid:
        try:
            p=subprocess.run(docker+['rm','-f',cid],capture_output=True,timeout=30)
            check=subprocess.run(docker+['container','inspect',cid],capture_output=True,timeout=30)
            cleanup={'removeExitCode':p.returncode,'absent':check.returncode!=0,'stdout':p.stdout.decode(),'stderr':p.stderr.decode()}
        except BaseException as ex:
            cleanup={'removeExitCode':None,'absent':False,'cleanupError':str(ex)}
            if error is None:error='cleanup: '+str(ex)
    z={'passed':error is None and ret==0 and cleanup is not None and cleanup['removeExitCode']==0 and cleanup['absent'],'workerExitCode':ret,'error':error,'elapsedMs':round((time.monotonic()-start)*1000),'image':image,'container':cid,'cleanup':cleanup,'imagesBuilt':0,'nodeServicesStarted':0,'privateMaterials':False}
    save(D/(owner+'-exec-envelope-result.json'),z);print(json.dumps(z,sort_keys=True),flush=True)
    p=D/(owner+'-exec-worker.log')
    if p.exists():print(p.read_text()[-6000:],flush=True)
sys.exit(0 if z['passed'] else 1)
