#!/usr/bin/env python3
"""Reconcile retained software identities; never replace a failed original fence."""
from pathlib import Path
import json,hashlib,os,stat,time
E=Path(__file__).resolve().parent;A=E/'current-arm-runtime-relink-root-20261016-01';O=E/'current-arm-runtime-relink-reconciliation-20261016-01';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
os.umask(0o077)
def material(p):
 p=Path(p);assert p.resolve()==p;fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);assert stat.S_ISREG(s.st_mode)and s.st_size<=536870912;r={k:getattr(s,k)for k in F};parts=[]
  while True:
   b=os.read(fd,1048576)
   if not b:break
   parts.append(b)
  b=b''.join(parts);assert len(b)==s.st_size and r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F};return dict(r,sha256=hashlib.sha256(b).hexdigest()),b
 finally:os.close(fd)
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def put(leaf,b):
 with(O/leaf).open('xb')as f:f.write(b);f.flush();os.fsync(f.fileno())
def save(leaf,q):put(leaf,(json.dumps(q,sort_keys=True)+'\n').encode())
def main():
 before=json.loads((A/'selected-host-inputs-before-worker.private.json').read_bytes());products=json.loads((A/'runtime-bundle-first-admission.private.json').read_bytes())['observations'][-1]['table'];known=dict(before)
 for p,row in products.items():assert p not in known or known[p]==row;known[p]=row
 for p,row in known.items():assert material(p)[0]==row
 raw=material(A/'outputs/result-public.json')[1];result=json.loads(raw);assert result==json.loads((A/'runtime-relink-worker-stdout.private.log').read_bytes());assert result['passed']is True and result['linkCommands']==6 and result['smokeCommands']==7 and result['compiledTranslationUnits']==0
 br=material(A/'outputs/software-admission.private.json')[1];assert pin(br)=={'sha256':result['softwareAdmissionSha256'],'bytes':result['softwareAdmissionBytes']}
 assert json.loads((A/'old-guest-before-stdout.private.log').read_bytes())==json.loads((A/'old-guest-after-stdout.private.log').read_bytes())=={'passed':True,'retainedRows':9}
 cleanup=json.loads((A/'cleanup-public.json').read_bytes());assert cleanup['passed']and cleanup['ownedContainersAbsent']and not cleanup['firstFailurePreserved']
 original=json.loads((A/'selected-host-final-fence-failure-public.json').read_bytes());assert original['passed']is False
 if not O.exists():
  O.mkdir(mode=0o700);put('worker-result-public.json',raw);put('worker-binding.private.json',br)
  for leaf in ['worker-result-public.json','worker-binding.private.json']:os.chmod(O/leaf,0o400)
 else:
  assert not (O/'known-byte-metadata-copies-first-admission.private.json').exists()
  assert material(O/'worker-result-public.json')[1]==raw and material(O/'worker-binding.private.json')[1]==br
 observations=[];start=time.monotonic();previous=None;count=0;since=start
 while True:
  table={leaf:material(O/leaf)[0]for leaf in ['worker-result-public.json','worker-binding.private.json']};now=time.monotonic();observations.append({'elapsedMs':int((now-start)*1000),'files':table})
  if table==previous:count+=1
  else:count=1;since=now
  assert now-start<35
  if count>=3 and now-since>=2:break
  previous=table;time.sleep(1.05)
 save('known-byte-metadata-copies-first-admission.private.json',{'observations':observations,'originalMetadataNineRecovered':False,'oldAuthoritiesRefreshed':False})
 for p,row in known.items():assert material(p)[0]==row
 save('selected-originals-rechecked.private.json',known)
 result.update(rootReconciliationPassed=True,rootRetainedHostInputsRechecked=len(before),rootRetainedSoftwareHostFilesRechecked=len(products),rootOriginalHostFinalFencePassed=False,rootOriginalHostFinalFenceFailurePreserved=True,rootOriginalHostFinalFenceFailureCauseExplained=False,rootOriginalResultAndBindingMetadataNineRecovered=False,rootNewKnownByteMetadataCopiesAdmitted=True,rootOldVolumeGuestRowsRetained=9,rootOldVolumeGuestBeforeAfterEqual=True,rootOwnedContainerAbsent=True,rootImageIdentityPreviouslyVerified=True,rootReconciliationCppCommands=0,rootReconciliationDockerCommands=0,rootReconciliationProgramRuns=0,rootFullHostQualification=False,oldAuthoritiesRefreshed=False,productionQualified=False)
 save('result-public.json',result);save('cleanup-public.json',cleanup);save('selected-inputs-final-fence-public.json',{'passed':True,'firstFailurePreserved':False,'scope':'only retained before-worker host rows and seven software rows; two new known-byte metadata copies','originalHostFinalFencePassed':False,'unexplainedOriginalHostFinalFenceFailureRemainsOpen':True,'oldAuthoritiesRefreshed':False})
 save('original-host-final-fence-failure-public.json',original);print(json.dumps({'passed':True,'retainedInputRowsMatched':len(before),'retainedSoftwareRowsMatched':len(products),'cppReplayed':False,'originalFinalFenceFailedAndUnexplained':True,'fullHostQualification':False,'productionQualified':False}),flush=True)
if __name__=='__main__':main()
