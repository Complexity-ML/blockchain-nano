"""Root bounded passive collection of public synthetic results; never open DBs."""
import base64,hashlib,json,os,pathlib,stat,struct,subprocess
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
BASE=pathlib.Path('/home/ubuntu');held={}
def need(x,m):
 if not x:raise RuntimeError(m)
def read(p):
 p=pathlib.Path(p);need(p.is_absolute()and p.resolve(strict=True)==p,'physical explicit path');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink>=1 and 0<=a.st_size<=134217728,'bounded regular file');r={k:getattr(a,k)for k in F};parts=[];n=0;h=hashlib.sha256()
  while b:=os.read(fd,1024*1024):n+=len(b);h.update(b);parts.append(b)
  need(n==a.st_size and r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'full9 unchanged on read');row=dict(r,sha256=h.hexdigest());need(str(p)not in held or held[str(p)]==row,'held input refreshed');held[str(p)]=row;return row,b''.join(parts)
 finally:os.close(fd)
control=BASE/'physical-cohort-amd-control-root-20261018-02/control.private.json';r,b=read(control);need(r['sha256']=='689c5951523ed44f281023b163d0c0654006f49d188b09e458066084f3ca05ba'and len(b)==185358,'original AMD control pin');c=json.loads(b)
selected=[c['docker']['prefix'][0],c['docker']['prefix'][2],c['core']['file'],*[x['file']for x in c['libraries']]]
for p in selected:need(read(p)[0]==c['originalFiles'][p],'original software full9/SHA')
exports={};total=0
files=['result-public.json','host-final-fence-public.json','container-terminal-public.json','worker-start-attached.stderr.log']
for attempt in ('03','04'):
 root=BASE/('real-journal-rollback-amd-root-20261018-'+attempt)
 names=list(files)
 if attempt=='04':names+=['outputs/result-public.json','outputs/RUN.stdout.log','outputs/RUN.stderr.log','outputs/run-root/commitments.json','outputs/loader-closure-public.json',*['outputs/'+x+'.exit-public.json'for x in('M','CXX','LINK','LDD','RUN')]]
 for name in names:
  row,raw=read(root/name);need(len(raw)<=2097152,'bounded public proof');total+=len(raw);need(total<=4194304,'bounded proof bundle');exports[attempt+'/'+name]={'row':row,'base64':base64.b64encode(raw).decode()}
root=BASE/'real-journal-rollback-amd-root-20261018-04';host=json.loads(read(root/'result-public.json')[1]);need(host['passed']is True and host['worker']['passed']is True and host['inputFinalFence']['passed']is True and host['cleanup']['ownedContainerAbsent']is True,'actual host terminal PASS');authority=json.loads(read(root/'outputs/final-observations.json')[1]);products={}
for guest,want in authority['products'].items():
 need(guest in {'/out/journal.o.M','/out/journal.o','/out/journal.o.d','/out/rollback'},'four declared build products');row,raw=read(root/'outputs'/pathlib.PurePosixPath(guest).name);need(row['sha256']==want['sha256']and row['st_size']==want['st_size'],'actual host product vs guest known bytes');products[guest]={'sha256':row['sha256'],'bytes':row['st_size']}
 if guest=='/out/rollback':need(raw[:4]==b'\x7fELF'and raw[4:6]==b'\x02\x01'and struct.unpack_from('<H',raw,18)[0]==62,'real ELF64 little-endian AMD64')
def deps(raw):
 import shlex,posixpath
 target,sep,text=raw.decode().replace('\\\n',' ').partition(':');need(sep and shlex.split(target)==['/out/journal.o'],'independent M/MD target');return sorted(set(posixpath.normpath(posixpath.join('/out',x))for x in shlex.split(text)))
need(deps(read(root/'outputs/journal.o.M')[1])==deps(read(root/'outputs/journal.o.d')[1]),'independent literal normalized M/MD match')
prefix=c['docker']['prefix'];image=c['docker']['image'];r=subprocess.run(prefix+['image','inspect',image],capture_output=True,timeout=20);need(r.returncode==0,'current image inspection');imagebody=json.loads(r.stdout);need(len(imagebody)==1 and imagebody[0]['Id']==image and imagebody[0]['Architecture']=='amd64','same immutable AMD image');cid=host['cleanup']['containerId'];r=subprocess.run(prefix+['container','inspect',cid],capture_output=True,timeout=20);need(r.returncode==1 and (b'no such object'in r.stderr.lower()or b'no such container'in r.stderr.lower()),'owned container independently absent')
snapshot=dict(held)
for p,row in snapshot.items():need(read(p)[0]==row,'post-collection held input unchanged')
audit={'format':'nano-journal-rollback-amd-root-passive-audit-v1','passed':True,'originalSoftwareFilesUnchanged':9,'heldFilesUnchanged':len(held),'products':products,'actualElfArchitecture':'x86_64','MAndMdMatch':True,'mmdDependencyCount':len(deps(read(root/'outputs/journal.o.M')[1])),'imageUnchanged':True,'ownedContainerIndependentlyAbsent':True,'newCppTestInvocations':0,'newCompilations':0,'newSignatures':0,'privateKeysOrDatabasesExported':False,'publicFiles':len(exports),'publicBytes':total,'productionQualified':False}
print(json.dumps({'audit':audit,'exports':exports},sort_keys=True))
