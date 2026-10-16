#!/usr/bin/env python3
"""Selected retained P2P artifact audit. No program execution or SDK traversal."""
from pathlib import Path
import os,stat,hashlib,json,subprocess,shlex,time
E=Path(__file__).resolve().parent
R=Path('/Users/boris/Dev/blockchain-nano')
O=E/'unchanged-p2p-reuse-root-20261017-01'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
REV='d26e4744416d033dad5a2519cc2ab8899b838faf';BASE='bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d'
os.umask(0o077)
def material(p):
 p=Path(p);assert p.resolve()==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);assert stat.S_ISREG(s.st_mode) and s.st_size<=16777216*128
  row={k:getattr(s,k)for k in F};parts=[];total=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   total+=len(b);assert total<=s.st_size;parts.append(b)
  b=b''.join(parts);assert len(b)==s.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  return dict(row,sha256=hashlib.sha256(b).hexdigest()),b
 finally:os.close(fd)
def load(rel,sha,n):
 p=E/rel;row,b=material(p);assert row['sha256']==sha and len(b)==n
 return json.loads(b),p,row
def save(name,value):
 with(O/name).open('x')as f:json.dump(value,f,sort_keys=True);f.write('\n')
def alias(p):
 p=Path(p);s=p.lstat();return {'material':{k:getattr(s,k)for k in F},'resolved':str(p.resolve(strict=True)),'link':os.readlink(p)if stat.S_ISLNK(s.st_mode)else None}
def git(*args):return subprocess.check_output(['/usr/bin/git','--no-optional-locks','-C',str(R),*args],timeout=30)
def main():
 assert git('status','--porcelain')==b''
 changes=git('diff','--name-only',BASE,REV,'--','network','include','third_party').decode().splitlines();assert changes==[]
 trees={x:git('rev-parse',REV+':'+x).decode().strip()for x in ('network','include','third_party')}
 O.mkdir(mode=0o700);held={};aliases={};reports={}
 n,p,row=load('current-native-runtime-bindings-root-attempt03-20261016/binding-inputs.private.json','7f9083e8bcab2c72237c3012bae9d858948f46b1947bc2dd2d88ac28afe10a89',2027120);held[str(p)]=row
 asset=n['programs']['p2p'];paths=[asset['path'],*n['resolvedDirectLoads']['p2p'].values()]
 for name in paths:
  assert isinstance(name,str);p=Path(name);row,_=material(p);assert row==n['files'][str(p)];held[str(p)]=row
  for q in [p,*p.parents]:
   for table in (n['aliases'],n['prospectiveLoaderAliasGuards']):
    if str(q)in table:
     value=alias(q);assert value==table[str(q)];aliases[str(q)]=value
 reports['mac-native']={'programSha256':asset['sha256'],'programBytes':asset['bytes'],'retainedProgramFullNineAndShaMatched':True,'retainedDirectExternalProvidersVerified':len(paths)-1,'retainedAliasGuardsVerified':len(aliases),'providerScope':'previous direct-loader qualification, including its system-cache boundary','programExecuted':False,'sourceBase':BASE}
 a,p,row=load('current-arm-runtime-network-root-20261016/first-copy-admission.private.json','dccedc2df5f1ff21fef6f75501dfe2f48fe05cbb8c1818cd9f2436ba946a474d',1856);held[str(p)]=row
 p=Path(a['program']['path']);row,_=material(p);assert row==a['hostObservations'][-1]['material'];held[str(p)]=row
 reports['mac-docker']={'programSha256':row['sha256'],'programBytes':row['st_size'],'retainedProgramFullNineAndShaMatched':True,'sameImageId':a['image'],'programExecuted':False,'sourceBase':BASE,'guestNineRecheckedThisAudit':False}
 save('local-selected-inputs.private.json',{'files':held,'aliases':aliases,'oldAuthoritiesRefreshed':False})
 remote=r'''from pathlib import Path
import os,stat,json,hashlib
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def material(p):
 p=Path(p);assert p.resolve()==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);assert stat.S_ISREG(s.st_mode)and s.st_size<=67108864;row={k:getattr(s,k)for k in F};b=b''
  while True:
   z=os.read(fd,1048576)
   if not z:break
   b+=z;assert len(b)<=s.st_size
  assert len(b)==s.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F};return dict(row,sha256=hashlib.sha256(b).hexdigest()),b
 finally:os.close(fd)
p=Path('/home/ubuntu/current-amd-final-runtime-20261016-01/runtime-bundle-admission.private.json');old,raw=material(p);assert old['sha256']=='5c5bb5928a00fe4d5fafef2028c03c450aa25c61855b595fb0fe3c2c9e5226ab'and len(raw)==8589
q=json.loads(raw);files=q['files'];selected=[(path,row)for path,row in files.items()if path.endswith('/nano-p2p')];assert len(selected)==1;name,want=selected[0];actual,_=material(name);assert actual==want;assert material(p)[0]==old
print(json.dumps({'programSha256':actual['sha256'],'programBytes':actual['st_size'],'retainedProgramFullNineAndShaMatched':True,'sameImageId':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','programExecuted':False,'oldSdkMapsRead':False,'oldAuthoritiesRefreshed':False,'productionQualified':False},sort_keys=True))
'''
 q=subprocess.run(['/usr/bin/ssh','-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','/usr/bin/python3 -c '+shlex.quote(remote)],capture_output=True,timeout=45)
 save('remote-audit-transport.json',{'exitCode':q.returncode,'stdoutSha256':hashlib.sha256(q.stdout).hexdigest(),'stdoutBytes':len(q.stdout),'stderrSha256':hashlib.sha256(q.stderr).hexdigest(),'stderrBytes':len(q.stderr)})
 (O/'remote-stderr.private.log').write_bytes(q.stderr);assert q.returncode==0 and not q.stderr;reports['vps-docker']=json.loads(q.stdout);reports['vps-docker']['sourceBase']=BASE
 for p,row in held.items():assert material(p)[0]==row
 for p,row in aliases.items():assert alias(p)==row
 save('result-public.json',{'format':'nano-unchanged-p2p-selected-reuse-audit-v1','passed':True,'currentCppSourceRevision':REV,'historicalSourceBase':BASE,'unchangedTrees':trees,'roles':reports,'p2pCoreDependency':False,'dependencyScans':0,'compilations':0,'links':0,'programRuns':0,'dockerImagesBuilt':0,'liveImageInspectPerformedInThisAudit':False,'newNetworkQualification':False,'dutiesResumed':False,'productionQualified':False,'oldAuthoritiesRefreshed':False})
 print(json.dumps({'passed':True,'roles':list(reports),'programRuns':0,'selectedInputFiles':len(held),'productionQualified':False}),flush=True)
if __name__=='__main__':main()
