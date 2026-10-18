"""Root prepares only a fresh, externally pinned control for the public export."""
import hashlib,json,os,pathlib,stat,sys,time
E=pathlib.Path(__file__).resolve().parent
D=E/'physical-native-export-source-proposal-20261018-01'
N=E/'native-c7-cmake-root-20261018-01/native-successor-04'
O=E/'physical-native-export-control-root-20261018-01'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
EXPECTED={
 'common':(D/'cohort_common.py','09422e11287ce5f0357e3b3e9e5219d6c492ac2a3d3815aa6bbb6aa2ade7943c',10200),
 'producerTerminal':(N/'cohort-produce.terminal.json','dd6e7880efb4a8a4a3649f1c905e7e2c7ad5033db8b568fcefed76a05cd88ec8',3307),
 'checkerTerminal':(N/'cohort-check.terminal.json','c71f5d186272c3d1de8df97348bc017a07cd912c58c991ec8a041b3808b0774f',3300),
 'nativeHostResult':(N/'result-public.json','a6117e885646e809be061ba98fbcc08fea3ff1728b128866a4056f239d0b1cdc',30408)}
def need(ok,why):
 if not ok:raise RuntimeError(why)
def capture(path,sha,size):
 need(path.is_absolute()and path.resolve(strict=True)==path,'physical input');fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_CLOEXEC)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and stat.S_IMODE(a.st_mode)==0o400 and a.st_nlink==1 and a.st_size==size,'frozen known single-link input');raw=b''
  while b:=os.read(fd,65536):raw+=b;need(len(raw)<=size,'bounded input')
  need(len(raw)==size and hashlib.sha256(raw).hexdigest()==sha and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(path.lstat(),k)for k in F},'exact known bytes/stable FD9/path');return row
 finally:os.close(fd)
def main():
 os.umask(0o077);need(len(sys.argv)==3,'usage prepare-control.py RUNNER_SHA RUNNER_BYTES');runner=(D/'runner.py',sys.argv[1],int(sys.argv[2]));need(len(runner[1])==64 and 0<runner[2]<=65536,'explicit bounded runner pin')
 need(not os.path.lexists(O)and not os.path.lexists(E/'physical-native-export-root-20261018-01'),'unused control/export namespaces');all_inputs=dict(EXPECTED,runner=runner);initial={k:capture(*v)for k,v in all_inputs.items()}
 for _ in range(2):time.sleep(1.05);need({k:capture(*v)for k,v in all_inputs.items()}==initial,'three unchanged current external proof descriptors; no history claim')
 def descriptor(key):
  p,sha,size=all_inputs[key];return {'path':str(p),'pin':{'sha256':sha,'bytes':size},'fd9':initial[key]}
 config={'format':'nano-physical-native-export-control-v1','ready':True,'sourceTree':str(N/'cohort-stores'),'outputRoot':str(E/'physical-native-export-root-20261018-01'),'common':descriptor('common'),'proofs':{k:descriptor(k)for k in EXPECTED if k!='common'},'manifestPin':{'sha256':'b8dfb4a9b2144c037f9f85a2137d0c4047bd0f344a739290b22c9242369246e6','bytes':12445},'historicalSourceFD9Qualified':False,'preserveOriginalHostWholeFailure':True}
 raw=(json.dumps(config,sort_keys=True,indent=2,allow_nan=False)+'\n').encode();O.mkdir(mode=0o700);path=O/'control.private.json';fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
 with os.fdopen(fd,'wb')as f:f.write(raw);f.flush();os.fsync(f.fileno())
 fd=os.open(O,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 try:os.fsync(fd)
 finally:os.close(fd)
 need({k:capture(*v)for k,v in all_inputs.items()}==initial,'inputs unchanged through creation')
 print(json.dumps({'passed':True,'control':str(path),'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'proofDescriptorsCurrentForThisOperationOnly':True,'historicalSourceFD9Qualified':False,'exportInvoked':False,'productionQualified':False}))
if __name__=='__main__':main()
