
import pathlib,hashlib,json,os,stat,sys,time
base=pathlib.Path('/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a/lifecycle-consumers-control-06ff955ccf334de0b79f689c7acd01f5')
p=base/'setup-admission.private.json'
b=p.read_bytes();assert hashlib.sha256(b).hexdigest()=='9d6c29222adef3ba59fb6fd88595f1a1edd993dbd0be05181b42390bd6e7b256' and len(b)==1872
ad=json.loads(b);F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');started=time.monotonic()
for i in range(4):
 for key in ('controller','input','headerAfter','docker','sudo','git'):
  r=ad['materials'][key];q=pathlib.Path(r['path']);assert q.resolve(strict=True)==q
  fd=os.open(q,os.O_RDONLY|os.O_NOFOLLOW)
  try:
   a=os.fstat(fd);parts=[]
   while True:
    chunk=os.read(fd,1048576)
    if not chunk:break
    parts.append(chunk)
   raw=b''.join(parts);row=[str(getattr(a,k))for k in F]
   assert stat.S_ISREG(a.st_mode) and row==r['material']==[str(getattr(os.fstat(fd),k))for k in F]==[str(getattr(q.lstat(),k))for k in F]
   assert len(raw)==r['bytes'] and hashlib.sha256(raw).hexdigest()==r['sha256']
   if key=='controller':code=raw;script=str(q)
  finally:os.close(fd)
 if i<3:time.sleep(.75)
assert time.monotonic()-started>=2
sys.argv=[script,'--inputs',str(base/'inputs.private.json'),'--sha256','89f89adfa56a640675717a7fc820bdbb2e09ba1fde56b44a3f4f7ddb4427ba63','--bytes','2894']
exec(compile(code,script,'exec'),{'__name__':'__main__','__file__':script})
