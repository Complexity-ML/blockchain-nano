#!/usr/bin/env python3
"""Root readback: only new public synthetic results/counters, never SDK/keys/DBs."""
import pathlib,json,subprocess,shlex,hashlib,base64
E=pathlib.Path(__file__).resolve().parent;L=E/'pruning-operation-amd-root-qualification-20261016-01'
FILES=['result-public.json','cleanup-public.json','originals-final-fence-public.json','failure-public.json','cleanup-failure-public.json','originals-final-fence-failure-public.json','outputs/result-public.json','outputs/worker-refusal.private.json',*[f'outputs/operation-{case}-{stream}.private.log'for case in ['startup-baseline','startup-validation','sst-reclaim']for stream in ['stdout','stderr']]]
code=r"""import json,sys,pathlib,os,stat,hashlib,base64
j=json.load(sys.stdin);r=pathlib.Path('/home/ubuntu/current-amd-pruning-operation-20261016-01');assert r.resolve()==r
allow=['result-public.json','cleanup-public.json','originals-final-fence-public.json','failure-public.json','cleanup-failure-public.json','originals-final-fence-failure-public.json','outputs/result-public.json','outputs/worker-refusal.private.json',*[f'outputs/operation-{case}-{stream}.private.log'for case in ['startup-baseline','startup-validation','sst-reclaim']for stream in ['stdout','stderr']]];assert j['files']==allow
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');out={};total=0
for leaf in allow:
 p=r/leaf
 if not p.exists():continue
 assert p.resolve()==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  st=os.fstat(fd);row={k:getattr(st,k)for k in F};assert stat.S_ISREG(st.st_mode)and st.st_nlink==1 and st.st_uid==st.st_gid==1000 and st.st_size<=1048576
  b=b''
  while True:
   z=os.read(fd,65536)
   if not z:break
   b+=z;assert len(b)<=st.st_size
  assert len(b)==st.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F};total+=len(b);assert total<=8388608
  out[leaf]={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),'data':base64.b64encode(b).decode(),'material9':row}
 finally:os.close(fd)
print(json.dumps({'files':out,'selectedPublicReadbackOnly':True,'privateArtifactsOrDatabasesExported':False}))
"""
r=subprocess.run(['/usr/bin/ssh','-o','BatchMode=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','/usr/bin/python3 -c '+shlex.quote(code)],input=json.dumps({'files':FILES}).encode(),capture_output=True,timeout=40);assert r.returncode==0,r.stderr.decode();j=json.loads(r.stdout)
assert set(j['files'])<=set(FILES)and j['selectedPublicReadbackOnly']and not j['privateArtifactsOrDatabasesExported']
known={}
for leaf,row in j['files'].items():
 b=base64.b64decode(row.pop('data'),validate=True);assert len(b)==row['bytes']and hashlib.sha256(b).hexdigest()==row['sha256'];p=L/'remote'/leaf;p.parent.mkdir(mode=0o700,parents=True,exist_ok=True)
 with p.open('xb')as f:f.write(b)
 p.chmod(0o400);known[leaf]=row
with(L/'selected-public-readback-root.json').open('x')as f:json.dump({'files':known,'selectedPublicReadbackOnly':True,'privateArtifactsOrDatabasesExported':False},f,sort_keys=True);f.write('\n')
result_path=L/'remote/result-public.json'
if result_path.exists():
 z=json.loads(result_path.read_bytes());assert z['passed']and not z['productionQualified'];q=json.loads((L/'host-stdout.private.log').read_bytes());assert q==z
 print(json.dumps({'passed':True,'owner':z['owner'],'newCoreSha256':z['newCoreSha256'],'startupExpectedRefusals':z['startupSummary']['expectedRefusals'],'sstReclaimedBytes':z['sstSummary']['sstReclaimedBytes'],'allocatedFileBytesReclaimed':z['sstSummary']['allocatedFileBytesReclaimed'],'sstElapsedMs':z['sstSummary']['elapsedMs'],'sstMaxRssKiB':z['sstSummary']['maxRssKiB'],'privateArtifactsExported':False,'reportsCollected':len(known)}))
else:print(json.dumps({'passed':False,'reportsCollected':len(known),'readbackComplete':True,'privateArtifactsExported':False}))
