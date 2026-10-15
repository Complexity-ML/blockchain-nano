#!/usr/bin/env python3
"""Read only this completed PUBLIC build; return artifact digests, never SDK rows."""
import hashlib,json,os,pathlib,shlex,stat,subprocess
HERE=pathlib.Path(__file__).resolve().parent
EXPECTED='a28d33ebaf38180f6dce1cd787080ac3d9a3fa26e533ac0cd6dd279cfd7cad0a'
p=HERE/'canonical-invocation-06'/'public-result.json'
raw=p.read_bytes();assert len(raw)==3608 and hashlib.sha256(raw).hexdigest()==EXPECTED
receipt=json.loads(raw);assert receipt['remoteExitCode']==0 and receipt['result']['passed']is True
assert receipt['nonce']=='831803dd-7b2b-42d5-ab50-69533118e10e'
OUT=HERE/'public-identities-01';OUT.mkdir(mode=0o700,exist_ok=False)
source=r'''
import hashlib,json,os,pathlib,re,stat,subprocess
R=pathlib.Path('/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a')
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
EXPECTED_TERMINAL=__EXPECTED_TERMINAL__
def read(p,maximum,saved=None):
 assert p.is_absolute() and p.resolve()==p
 f=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(f);row=[str(getattr(a,k))for k in F]
  assert stat.S_ISREG(a.st_mode)and a.st_nlink==1 and a.st_uid==1000 and a.st_gid==1000 and 0<a.st_size<=maximum
  if saved is not None:assert row==saved['material'] and str(p)==saved['path'] and a.st_size==saved['bytes']
  b=bytearray();h=hashlib.sha256()
  while len(b)<a.st_size:
   c=os.read(f,min(1048576,a.st_size-len(b)));assert c;b.extend(c);h.update(c)
  assert not os.read(f,1) and row==[str(getattr(os.fstat(f),k))for k in F]==[str(getattr(p.lstat(),k))for k in F]
  if saved is not None:assert h.hexdigest()==saved['sha256']
  return bytes(b),{'bytes':len(b),'sha256':h.hexdigest()},row
 finally:os.close(f)
terminal,terminal_pin,terminal_row=read(R/'resume06-public-result.json',65536)
assert json.loads(terminal)==EXPECTED_TERMINAL
admission,admission_pin,admission_row=read(R/'control06/new-artifacts-first-admission.private.json',65536)
d=json.loads(admission)
assert set(d)=={'files','samples','stableSpanMs','oldAuthorityReadOrRefreshed'} and d['samples']>=3 and d['stableSpanMs']>=2000 and d['oldAuthorityReadOrRefreshed']is False
names=['nano','nano-capella-isolated-bootstrap-audit','nano-capella-isolated-bootstrap-import','nano-capella-isolated-stopped-capture','nano-capella-deposit-audit','nano-capella-lifecycle-submit','nano-capella-validator-read-tests']
expected={str(R/'core'/n):n for n in names};expected[str(R/'core/libnano_core.a')]='libnano_core.a';expected[str(R/'network/nano-p2p')]='nano-p2p'
assert set(d['files'])==set(expected)
artifacts=[]
for path_,name in sorted(expected.items(),key=lambda x:x[1]):
 saved=d['files'][path_];assert set(saved)=={'path','material','sha256','bytes'} and len(saved['material'])==9 and all(isinstance(v,str)and v.isdigit()for v in saved['material']) and re.fullmatch('[0-9a-f]{64}',saved['sha256'])
 b,pin,_=read(pathlib.Path(path_),1073741824,saved)
 if name=='libnano_core.a':assert b.startswith(b'!<arch>\n');kind='ar'
 else:assert b[:6]==b'\x7fELF\x02\x01'and int.from_bytes(b[18:20],'little')==62;kind='ELF-amd64'
 artifacts.append({'name':name,'format':kind,**pin})
b,pin,row=read(R/'control06/new-artifacts-first-admission.private.json',65536);assert pin==admission_pin and row==admission_row and b==admission
b,pin,row=read(R/'resume06-public-result.json',65536);assert pin==terminal_pin and row==terminal_row and b==terminal
q=subprocess.run(['sudo','-n','docker','ps','-a','--filter','name=^/nano-canonical-amd-831803dd-7b2b-42d5-ab50-69533118e10e$','--format','{{.ID}}'],capture_output=True,timeout=10)
assert q.returncode==0 and not q.stdout.strip()
print(json.dumps({'format':'nano-new-public-amd-artifact-identities-v1','passed':True,'sourceRevision':'712b8357d05f09cc28df6e04920c9955e59b7b69','artifacts':artifacts,'producerArtifactSavedNineAndShaUnchanged':True,'producerStableSamples':d['samples'],'producerStableSpanMs':d['stableSpanMs'],'admissionDescriptorPostQualificationDigest':admission_pin,'admissionDescriptorDigestPreviouslyRetainedOffsite':False,'terminalPostQualificationDigest':terminal_pin,'producerRootReceiptSha256':'a28d33ebaf38180f6dce1cd787080ac3d9a3fa26e533ac0cd6dd279cfd7cad0a','ownedContainerAbsent':True,'sdkHashesExported':False,'rawNineFieldsExported':False,'oldPrivateInputsRead':False,'runtimeStarted':False,'productionQualified':False},sort_keys=True))
'''.replace('__EXPECTED_TERMINAL__',repr(receipt['result']))
argv=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=8','-o','StrictHostKeyChecking=yes','-o','ForwardAgent=no','ubuntu@145.239.199.166','python3 -c '+shlex.quote(source)]
r=subprocess.run(argv,stdin=subprocess.DEVNULL,capture_output=True,timeout=75)
for name,b in [('stdout.private',r.stdout),('stderr.private',r.stderr)]:
 f=os.open(OUT/name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(f,'wb')as w:w.write(b);w.flush();os.fsync(w.fileno())
assert r.returncode==0 and len(r.stdout)<=16384 and not r.stderr
d=json.loads(r.stdout);assert d['format']=='nano-new-public-amd-artifact-identities-v1' and d['passed']is True and d['sdkHashesExported']is False and d['rawNineFieldsExported']is False and len(d['artifacts'])==9
b=(json.dumps(d,sort_keys=True)+'\n').encode();f=os.open(OUT/'public-result.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
with os.fdopen(f,'wb')as w:w.write(b);w.flush();os.fsync(w.fileno())
print(json.dumps({'passed':True,'artifacts':9,'savedNineAndShaUnchanged':True,'sdkHashesExported':False,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'runtimeStarted':False,'productionQualified':False},sort_keys=True))
