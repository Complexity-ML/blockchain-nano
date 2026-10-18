"""Root only: prospectively admit four new public sources; no Docker or DB."""
import hashlib,json,os,pathlib,stat,sys,time
ROOT=pathlib.Path('/home/ubuntu/nano-real-journal-rollback-public-root-20261018-04')
FIELDS=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def need(x,m):
 if not x:raise RuntimeError(m)
def capture(path,want):
 need(path.resolve(strict=True)==path,'physical prospective path');fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_CLOEXEC)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in FIELDS};need(stat.S_ISREG(a.st_mode)and stat.S_IMODE(a.st_mode)==0o400 and a.st_uid==a.st_gid==1000 and a.st_nlink==1,'new public source owner/mode/type')
  h=hashlib.sha256();n=0
  while b:=os.read(fd,65536):n+=len(b);h.update(b);need(n<=65536,'bounded four new sources')
  need({'sha256':h.hexdigest(),'bytes':n}==want,'original Mac public source pin')
  need(row=={k:getattr(os.fstat(fd),k)for k in FIELDS}=={k:getattr(path.lstat(),k)for k in FIELDS},'prospective FD9 changed');return dict(row,sha256=h.hexdigest())
 finally:os.close(fd)
def main():
 need(len(sys.argv)==1 and os.geteuid()==os.getegid()==1000,'public Root admission arguments/owner');want={'amd_host_v5.py': {'sha256': 'd9c4c60005dba36a988533a2206dc922445233823f0b231a629de9f8786b3d1c', 'bytes': 25452}, 'amd_worker_v3.py': {'sha256': '023390da5232cd340259ec8868a6ef3ef146c62906864cfae97bae2885e61bf4', 'bytes': 13273}, 'beacon_journal_physical_rollback.cpp': {'sha256': '2e5ef451f13cca770cf02128bbb389eaae60379974252859829e197d14521acd', 'bytes': 28801}, 'bootstrap-readonly-replica.hpp': {'sha256': 'd3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125', 'bytes': 20319}};names={'amd_host_v5.py','amd_worker_v3.py','beacon_journal_physical_rollback.cpp','bootstrap-readonly-replica.hpp'};need(set(want)==names and ROOT.resolve(strict=True)==ROOT,'exact four public source names')
 before={str(ROOT/n):capture(ROOT/n,want[n])for n in sorted(names)}
 for _ in range(2):
  time.sleep(1.05);need(before=={str(ROOT/n):capture(ROOT/n,want[n])for n in sorted(names)},'three stable prospective samples')
 value={'format':'nano-real-journal-rollback-amd-source-admission-v1','ready':True,'files':before};raw=(json.dumps(value,sort_keys=True,indent=2)+'\n').encode();out=ROOT/'source-admission.private.json';fd=os.open(out,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
 with os.fdopen(fd,'wb')as f:f.write(raw);f.flush();os.fsync(f.fileno())
 d=os.open(ROOT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 try:os.fsync(d)
 finally:os.close(d)
 print(json.dumps({'format':value['format'],'ready':True,'admissionPath':str(out),'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'newPublicSourceFiles':4,'threeStableSamples':True,'historicalAuthorityRefreshed':False}))
if __name__=='__main__':main()
