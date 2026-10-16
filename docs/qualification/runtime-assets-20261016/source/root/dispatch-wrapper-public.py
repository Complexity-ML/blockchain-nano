
import pathlib,json,os,hashlib,sys,subprocess
E=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012');O=E/'fresh-arm-bootstrap-import-current-core-root-20261016';P=E/'fresh-arm-bootstrap-import-current-core-source-controller-20261016/relink-arm-import-root.py';I=O/'inputs-arm-01.private.json';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
ad=json.loads((O/'controller-first-admission.private.json').read_bytes());assert ad['samples']>=3 and ad['stableSpanMs']>=2000
for key,p,want,size in [('controller',P,'8d77b8998f9a78d078dbf8fe91bacea21eb9b976fc20ed9bfb26fcc8366d05e4',36960),('input',I,'69c7416aa85495aca6fda47aca3ab8fdc57a52497d7830f55c7c48f45e251cc9',2075)]:
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);parts=[]
  while True:
   b=os.read(fd,1048576)
   if not b:break
   parts.append(b)
  raw=b''.join(parts);row={k:getattr(a,k)for k in F};row['sha256']=hashlib.sha256(raw).hexdigest()
  assert row==ad['materials'][key] and row['sha256']==want and len(raw)==size
  assert {k:getattr(a,k)for k in F}=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  if key=='controller':code=raw
 finally:os.close(fd)
assert subprocess.check_output(['git','-C','/Users/boris/Dev/blockchain-nano','rev-parse','HEAD']).decode().strip()=='944aa77b6d92e3ac609938f6d5a024acd88cc118'
sys.argv=[str(P),'--inputs',str(I),'--sha256','69c7416aa85495aca6fda47aca3ab8fdc57a52497d7830f55c7c48f45e251cc9','--bytes','2075']
exec(compile(code,str(P),'exec'),{'__name__':'__main__','__file__':str(P)})
