"""Root-only creation of one new public-cohort import control on the authorized VPS."""
import hashlib,json,os,pathlib,stat,sys,time
V=pathlib.Path('/home/ubuntu')
I=V/'nano-physical-native-import-public-root-20261018-01'
O=V/'physical-native-import-amd-control-root-20261018-01'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
PINS={
 'archive':('cohort.tar','7c6f556ab3514bc2115db8fb095dc9b1f5feaf1f0beaad955b81e286aebebf96',5283840),
 'manifest':('cohort-manifest-public.json','b8dfb4a9b2144c037f9f85a2137d0c4047bd0f344a739290b22c9242369246e6',12445),
 'inventory':('export-inventory-public.json','c931751ec594069eca6ce70452c8670bb3ca07bcd4e31fade85e402ddea9a5bf',18838),
 'commitment':('export-external-commitment-public.json','3144122f7872df305c47ee538a3db7f6a3d5f7c21274b163c07ab7d80137a203',1157),
 'exportResult':('result-public.json','7d55ffa8313961489970a02a27b41a4ebefa7a6a1774c1ba4bc0d7ee98385496',6406)}
LIMITS={'maximumBytes':134217728,'maximumEntries':4096,'importSeconds':180,'checkSeconds':180,'operationSeconds':420,'newCopyAdmissionSeconds':30,'logBytes':1048576}
def need(ok,why):
 if not ok:raise RuntimeError(why)
def capture(p,sha,size,frozen=False):
 need(p.is_absolute()and p.resolve(strict=True)==p,'canonical explicit input')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK|os.O_CLOEXEC)
 try:
  s=os.fstat(fd);row={k:getattr(s,k)for k in F};need(stat.S_ISREG(s.st_mode)and s.st_nlink==1 and s.st_size==size,'known single-link input')
  need(not frozen or stat.S_IMODE(s.st_mode)==0o400,'frozen source/public transport')
  h=hashlib.sha256();n=0
  while b:=os.read(fd,65536):n+=len(b);need(n<=size,'bounded input');h.update(b)
  z=os.fstat(fd);a=p.lstat();need(n==size and h.hexdigest()==sha and row=={k:getattr(z,k)for k in F}=={k:getattr(a,k)for k in F},'known bytes and strict nine fields unchanged during read')
  return dict(row,sha256=sha)
 finally:os.close(fd)
def main():
 os.umask(0o077);need(sys.platform.startswith('linux')and os.geteuid()==os.getegid()==1000,'authorized Linux1000 host');need(len(sys.argv)==3,'usage prepare.py RUNNER_SHA RUNNER_BYTES')
 sha=sys.argv[1];size=int(sys.argv[2]);need(len(sha)==64 and all(c in '0123456789abcdef'for c in sha)and 0<size<=65536,'explicit bounded runner pin')
 need(not os.path.lexists(O)and not os.path.lexists(V/'physical-native-import-amd-root-20261018-01'),'fresh control/import namespaces')
 expected={k:(I/n,h,z,True)for k,(n,h,z)in PINS.items()}
 expected.update({
  'runner':(I/'runner.py',sha,size,True),
  'common':(I/'cohort_common.py','09422e11287ce5f0357e3b3e9e5219d6c492ac2a3d3815aa6bbb6aa2ade7943c',10200,True),
  'control':(V/'physical-cohort-amd-control-root-20261018-02/control.private.json','689c5951523ed44f281023b163d0c0654006f49d188b09e458066084f3ca05ba',185358,False),
  'readerAudit':(V/'physical-cohort-amd-root-20261018-02/root-audit-public.json','61763fd153906581389d0123c606bedad41efbf0f3fd573171f3eea2e5d877ca',3392,False)})
 initial={k:capture(*v)for k,v in expected.items()}
 for _ in range(2):time.sleep(1.05);need({k:capture(*v)for k,v in expected.items()}==initial,'three identical current control descriptors over two seconds')
 def descriptor(k):
  p,h,z,_=expected[k];return {'file':str(p),'pin':{'sha256':h,'bytes':z},'originalRow':initial[k]}
 config={'format':'nano-physical-native-import-amd-control-v1','ready':True,'incomingRoot':str(I),'outputRoot':str(V/'physical-native-import-amd-root-20261018-01'),'limits':LIMITS,'inputs':{k:descriptor(k)for k in PINS},'common':descriptor('common'),'originalAuthorities':{k:descriptor(k)for k in ('control','readerAudit')}}
 raw=(json.dumps(config,sort_keys=True,indent=2,allow_nan=False)+'\n').encode();O.mkdir(mode=0o700);p=O/'control.private.json';fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
 with os.fdopen(fd,'wb')as f:need(f.write(raw)==len(raw),'complete control');f.flush();os.fsync(f.fileno())
 fd=os.open(O,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 try:os.fsync(fd)
 finally:os.close(fd)
 need({k:capture(*v)for k,v in expected.items()}==initial,'all descriptors unchanged through control creation')
 print(json.dumps({'passed':True,'control':str(p),'pin':{'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)},'currentDescriptorsForThisOperationOnly':True,'oldSoftwareRowsMustComeFromOldAuthority':True,'cppInvoked':False,'databaseOpened':False,'productionQualified':False}))
if __name__=='__main__':main()
