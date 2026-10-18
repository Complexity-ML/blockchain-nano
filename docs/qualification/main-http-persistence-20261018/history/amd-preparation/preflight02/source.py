
from pathlib import Path
import hashlib,json,stat,subprocess,os
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
p=Path('/home/ubuntu/rpc-candidate-amd-root-20261018-01/outputs/final-observations.private.json');b=p.read_bytes();assert len(b)==1512552 and hashlib.sha256(b).hexdigest()=='866e3f3d0042ec8cf90b487498bfa70601e9f9af826379d6dfe5eb03239e43c9'
a=json.loads(b);exe=Path('/home/ubuntu/rpc-candidate-amd-root-20261018-01/outputs/nano');old=a['products'][str(exe)]['hostFD9'];s=exe.stat();data=exe.read_bytes();got={k:getattr(s,k) for k in F};got['sha256']=hashlib.sha256(data).hexdigest();assert got==old and stat.S_ISREG(s.st_mode) and s.st_nlink==1
cp=Path('/home/ubuntu/rpc-candidate-amd-control-root-20261018-01/control.private.json');b=cp.read_bytes();assert len(b)==1178681 and hashlib.sha256(b).hexdigest()=='ac2eeedf8406ae807043bb91ec56d135f8b9829b89e54665cd2d3c0e84e2d3d1';c=json.loads(b)
for path in ('/usr/bin/git','/usr/bin/docker','/usr/bin/sudo'):
 q=Path(path);st=q.stat();rr={k:getattr(st,k) for k in F};rr['sha256']=hashlib.sha256(q.read_bytes()).hexdigest();assert rr==c['originalFiles'][path]
import tempfile
client=tempfile.TemporaryDirectory(prefix='nano-main-http-preflight-',dir='/home/ubuntu')
prefix=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',client.name]

image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';r=subprocess.run(prefix+['image','inspect',image],capture_output=True,timeout=20);assert r.returncode==0 and not r.stderr;i=json.loads(r.stdout)[0];assert i['Id']==image and i['Architecture']=='amd64' and i['Os']=='linux'
R='/home/ubuntu/blockchain-nano-git';head=subprocess.check_output(['/usr/bin/git','-C',R,'rev-parse','HEAD']).decode().strip();assert head=='bc99b0ec2cf8bc6bcb21a26ec980ef2c4e8573a8' and not subprocess.check_output(['/usr/bin/git','-C',R,'status','--porcelain'])
print(json.dumps({'format':'nano-main-http-amd-original-software-preflight-v1','passed':True,'main':head,'nano':{'path':str(exe),'sha256':old['sha256'],'bytes':old['st_size']},'originalNanoProductRowVerified':True,'oldProductAuthorityRowsRefreshed':False,'authority866OwnHistoricalFd9Claimed':False,'authority866ContentPinPreserved':True,'dockerPrefixKind':'sudo-host-socket-owned-client','image':image,'imageArchitecture':i['Architecture'],'dockerUid':1000,'dockerGid':1000,'hostUid':os.getuid(),'hostGid':os.getgid(),'newRuntimeProcessesStarted':0,'newImagesBuilt':0,'productionQualified':False}))

client.cleanup()
