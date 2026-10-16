#!/usr/bin/env python3
"""ROOT-only readback; keep all original rows and SDK records on the VPS."""
import hashlib,json,os,pathlib,stat,subprocess
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
BASE=pathlib.Path('/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a')
OUT=BASE/'lifecycle-consumers-06ff955ccf334de0b79f689c7acd01f5'
REV='a3a894b492f8411b09e26fc6e48fb36e43099b39'

def need(v,s):
 if not v:raise ValueError(s)

def read(p):
 p=pathlib.Path(p);need(p.resolve(strict=True)==p,'physical original path')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and 0<=a.st_size<=268435456,'bounded regular FD')
  chunks=[];size=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   size+=len(b);need(size<=a.st_size,'original size bound');chunks.append(b)
  r={k:getattr(a,k)for k in F};need(r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}and size==a.st_size,'FD/path original nine fields')
  b=b''.join(chunks);return dict(r,sha256=hashlib.sha256(b).hexdigest()),b
 finally:os.close(fd)

def known(p,sha,size):
 r,b=read(p);need((r['sha256'],len(b))==(sha,size),'retained raw descriptor bytes');return json.loads(b)

def equal(p,row):
 seen,b=read(p);need(seen==row,'saved original nine fields plus SHA changed');return b

def main():
 need(os.getuid()==os.getgid()==1000,'Ubuntu owner')
 terminal=known(OUT/'terminal.json','f384cdcdd3ab22edb3ef3abca0ccbe639e882e3c068cc5d167eb78a5a58fcdd5',6620)
 need(terminal['passed']is True and terminal['ownedContainerAbsent']is True and len(terminal['events'])==39 and all(x['passed']is True and x['exitCode']==0 for x in terminal['events']),'actual terminal success')
 ad=known(OUT/'new-software-admission.private.json','6a2b60444dd4a09cf5343c1e4e0c45bb3082cd6deda7ff113db57f4e501eea2a',337171)
 rows=ad['usedOriginalMaterials'];need(isinstance(rows,dict)and 100<len(rows)<1000,'selected original authority count')
 for p,row in rows.items():equal(p,row)
 products=ad['newToolProducts'];need(set(products)=={'deposit_audit.cpp.o','deposit_audit.cpp.o.d','nano-capella-deposit-audit'},'three actual new products')
 for leaf,ref in products.items():
  p=OUT/leaf;need(ref['path']==str(p),'new owned product exact locator');equal(p,ref['material']);need(ref['material']['st_uid']==ref['material']['st_gid']==1000 and ref['material']['st_nlink']==1,'new product owner/links')
 tool=products['nano-capella-deposit-audit'];need((tool['sha256'],tool['bytes'])==('45090eaee0ed9738ef6d2deb2dda8c843e36de4dafd5ee218f1b814693b0e442',6589712),'new Tool identity')
 env={'PATH':'/usr/bin:/bin','HOME':'/home/ubuntu','LANG':'C','LC_ALL':'C','GIT_OPTIONAL_LOCKS':'0','GIT_TERMINAL_PROMPT':'0'}
 def command(args):
  r=subprocess.run(args,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
  need(r.returncode==0 and not r.stderr,'normal read-only command success');return r.stdout
 repo='/home/ubuntu/blockchain-nano-git'
 head=command(['/usr/bin/git','--no-optional-locks','-C',repo,'rev-parse','HEAD']).decode().strip();need(head==REV,'current source Git HEAD')
 paths=['src','include','network','third_party','CMakeLists.txt','tests/multihost/lifecycle_submit.cpp','tests/support/fresh_managed_submission.hpp','tests/support/fresh_managed_submission_self_test.hpp','tests/support/fresh_lifecycle_terminal.hpp']
 changed=command(['/usr/bin/git','--no-optional-locks','-C',repo,'diff','--name-only','712b8357d05f09cc28df6e04920c9955e59b7b69',REV,'--',*paths]).decode().splitlines()
 need(changed==['tests/support/fresh_lifecycle_terminal.hpp'],'runtime/Core/Submit source changes outside approved Tool delta')
 status=command(['/usr/bin/git','--no-optional-locks','-C',repo,'status','--porcelain','--untracked-files=normal']);need(not status,'VPS source checkout clean')
 name='nano-amd-lifecycle-consumers-06ff955ccf334de0b79f689c7acd01f5'
 present=command(['/usr/bin/sudo','-n','/usr/bin/docker','-H','unix:///var/run/docker.sock','ps','-a','--filter','name=^/'+name+'$','--format','{{.ID}}']);need(not present.strip(),'only owned test container absent')
 for p,row in rows.items():equal(p,row)
 scopes={k:len(v)for k,v in ad['selectedDependencies'].items()}
 result={'format':'nano-amd-lifecycle-consumers-root-postqualification-v1','passed':True,'effectiveSourceRevision':head,'terminal':{'sha256':'f384cdcdd3ab22edb3ef3abca0ccbe639e882e3c068cc5d167eb78a5a58fcdd5','bytes':6620},'admission':{'sha256':'6a2b60444dd4a09cf5343c1e4e0c45bb3082cd6deda7ff113db57f4e501eea2a','bytes':337171},'originalUsedRowsUnchanged':len(rows),'newToolProductsUnchanged':3,'selectedDependencyScopes':scopes,'newSameContainerSdkRowsRecorded':len(ad['newGuestSdkNamespace']),'sdkRowsRereadAfterContainerRemoval':0,'onlySourceDeltaInRuntimeAndConsumers':changed,'ownedContainerAbsent':True,'sourceCheckoutClean':True,'newCompilerInvocations':0,'newCliSelectors':0,'historicalSdkAuthorityRecovered':False,'oldAuthoritiesRefreshed':False,'rawNineFieldsExported':False,'sdkBytesReadByThisPostqualification':False,'operationalSignatures':0,'newCampaignDomainsConsumed':0,'runtimeStarted':False,'productionQualified':False}
 print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':main()
