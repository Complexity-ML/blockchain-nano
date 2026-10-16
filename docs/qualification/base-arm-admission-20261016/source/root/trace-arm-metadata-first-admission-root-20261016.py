#!/usr/bin/env python3
"""Three metadata-only reproductions: no compiler, container, node or private state."""
from pathlib import Path
import os,stat,json,hashlib,time
E=Path(__file__).resolve().parent;A=E/'current-arm-runtime-relink-root-20261016-01';O=E/'metadata-fence-trace-root-20261016-01';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
os.umask(0o077)
def row(s):return {k:getattr(s,k)for k in F}
def read(p):
 p=Path(p);assert p.resolve()==p;before=row(p.lstat());fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);assert stat.S_ISREG(a.st_mode)and a.st_nlink==1 and a.st_uid==501 and 0<=a.st_size<=4194304;parts=[]
  while True:
   b=os.read(fd,65536)
   if not b:break
   parts.append(b)
  b=b''.join(parts);after_fd=row(os.fstat(fd));after_path=row(p.lstat());r=dict(after_fd,sha256=hashlib.sha256(b).hexdigest());return r,b,{'beforePath':before,'openedFd':row(a),'afterFd':after_fd,'afterPath':after_path,'consistentDuringRead':before==row(a)==after_fd==after_path and len(b)==a.st_size}
 finally:os.close(fd)
def save(name,value):
 with(O/name).open('x')as f:json.dump(value,f,sort_keys=True);f.write('\n');f.flush();os.fsync(f.fileno())
def main():
 assert os.getuid()==501;O.mkdir(mode=0o700);sources={};guards={}
 for leaf in ['result-public.json','software-admission.private.json']:
  p=A/'outputs'/leaf;r,b,trace=read(p);assert trace['consistentDuringRead'];sources[leaf]=b;guards[str(p)]=r
 result=json.loads(sources['result-public.json']);assert sources['result-public.json']==(A/'runtime-relink-worker-stdout.private.log').read_bytes();assert hashlib.sha256(sources['software-admission.private.json']).hexdigest()==result['softwareAdmissionSha256']and len(sources['software-admission.private.json'])==result['softwareAdmissionBytes']
 rounds=[]
 for number in range(3):
  d=O/('sample-'+str(number));d.mkdir(mode=0o700);held={};events=[];started=time.monotonic()
  for leaf,b in sources.items():
   p=d/leaf
   # Match the original extraction: close after write, with no preceding fsync
   # or stable admission. The path is new and has never been an authority.
   with p.open('xb')as f:f.write(b)
   r,actual,t=read(p);assert actual==b;held[leaf]=r;events.append({'phase':'eager-first-hold','elapsedNs':time.monotonic_ns(),'file':leaf,'row':r,'readTrace':t})
  memory_result=json.loads(sources['result-public.json']);memory_result.update(metadataTraceOnly=True)
  for sample in range(7):
   for leaf,b in sources.items():
    r,actual,t=read(d/leaf);assert actual==b;changed={k:{'first':held[leaf][k],'now':r[k]}for k in r if r[k]!=held[leaf][k]};events.append({'phase':'fence-sample','sample':sample,'elapsedNs':time.monotonic_ns(),'file':leaf,'row':r,'changed':changed,'readTrace':t})
   if sample<6:time.sleep(.4)
  rounds.append({'round':number,'spanMs':int((time.monotonic()-started)*1000),'events':events})
 for p,want in guards.items():
  r,_,t=read(p);assert r==want and t['consistentDuringRead']
 save('observations.private.json',{'sourceMetadataCurrentRows':guards,'historicalMetadataNineRecovered':False,'rounds':rounds})
 changed=[e for q in rounds for e in q['events']if e.get('changed')];inflight=[e for q in rounds for e in q['events']if not e['readTrace']['consistentDuringRead']]
 summary={'format':'nano-arm-host-metadata-first-admission-trace-v1','traceCompleted':True,'rounds':3,'metadataCopies':6,'postHoldFenceSamples':42,'changedObservations':len(changed),'withinReadInconsistencies':len(inflight),'changedAttributes':sorted({k for e in changed for k in e['changed']}),'eagerHoldDriftReproduced':bool(changed),'historicalCauseProven':False,'historicalFinalFenceFailureReclassified':False,'historicalMetadataNineRecovered':False,'originalKnownBytesAndCurrentSourceRowsUnchanged':True,'compilerCommands':0,'dockerCommands':0,'nodeCommands':0,'oldCxxTestsReplayed':False,'productionQualified':False};save('result-public.json',summary);print(json.dumps(summary,sort_keys=True),flush=True)
if __name__=='__main__':main()
