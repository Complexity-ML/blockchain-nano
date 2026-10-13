#!/usr/bin/env python3
"""Read-only byte-bound review of the final AMD public allowlist."""
from pathlib import Path
import json,hashlib,stat,os
BASE=Path(__file__).parent
ROOT=BASE/'duty-availability-amd64-final-public-evidence-20261013'
OUT=BASE/'amd64-public-private-exclusion-review-d7-20261013'
EXPECTED='e9dec5b886069cadc19bc5ab9c11db78f104ec96f4d764caef0d318a7ac3323b'
FORBIDDEN={'source-capture.public.json','destination-capture.public.json',
           'physical-copy-public-receipt.json','terminal-public-receipt.json'}
RAW_FIELDS={'snapshot','witnessSnapshot','fullFileInodeInventories','rawstate','seed','privateKey'}
sha=lambda b:hashlib.sha256(b).hexdigest()
def read(p):
 s=p.lstat();assert stat.S_ISREG(s.st_mode) and not p.is_symlink()
 b=p.read_bytes();z=p.lstat();assert (s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)==(z.st_dev,z.st_ino,z.st_size,z.st_mtime_ns,z.st_ctime_ns)
 return b
manifest_bytes=read(ROOT/'manifest.json');assert sha(manifest_bytes)==EXPECTED
j=json.loads(manifest_bytes);seen=set();found=[];bad_fields=[]
def inspect(value,path):
 if isinstance(value,dict):
  for k,v in value.items():
   if k in RAW_FIELDS:bad_fields.append({'path':path,'field':k})
   inspect(v,path)
 elif isinstance(value,list):
  for v in value:inspect(v,path)
for row in j['files']:
 n=row['path'];parts=Path(n).parts
 assert not Path(n).is_absolute() and '..' not in parts and n not in seen
 seen.add(n);p=ROOT/n;assert p.resolve().is_relative_to(ROOT.resolve())
 b=read(p);assert len(b)==row['bytes'] and sha(b)==row['sha256']
 if p.name in FORBIDDEN or Path(row.get('sourcePath','')).name in FORBIDDEN:found.append(n)
 assert p.suffix not in {'.o','.a','.so','.dylib','.tar','.gz','.ssz','.env'}
 if p.suffix=='.json':inspect(json.loads(b),n)
actual={str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file()}
assert actual==seen|{'manifest.json'}
assert not any(p.is_symlink() for p in ROOT.rglob('*'))
assert len(seen)==j['payloadCount']==237 and sum(row['bytes'] for row in j['files'])==j['payloadBytes']==5768507
assert not found and not bad_fields
assert read(ROOT/'manifest.json')==manifest_bytes
assert not OUT.exists();OUT.mkdir(mode=0o700)
receipt={'format':'nano-public-amd64-private-egress-exclusion-review-v1','passed':True,
 'packagePath':str(ROOT),'packageManifestSha256':EXPECTED,'payloadCount':237,'payloadBytes':5768507,
 'allPayloadShaAndBytesVerified':True,'fullDiskFileSetExactlyManifestAndPayloads':True,'noSymlinks':True,
 'pendingFourFileBasenames':sorted(FORBIDDEN),'matchingPayloadOrSourceBasenames':found,
 'rawCapsuleOrSecretJsonFieldMatches':bad_fields,
 'compiledBinaryObjectArchiveDatabaseAndEnvironmentPayloadsIncluded':False,
 'existingPackageManifestUnmodified':True,'privateVpsFilesAccessed':False,'sshUsed':False,
 'scope':'Local public package only; does not inspect or export the four private VPS files or establish their contents.'}
for name,b in [('terminal-review.json',(json.dumps(receipt,indent=2)+'\n').encode()),('reviewer-source.py',Path(__file__).read_bytes())]:
 fd=os.open(OUT/name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b)
rows=[{'path':p.name,'sha256':sha(read(p)),'bytes':p.stat().st_size} for p in sorted(OUT.iterdir())]
manifest=(json.dumps({'format':'nano-public-package-exclusion-review-evidence-v1','files':rows},indent=2)+'\n').encode()
(OUT/'manifest.json').write_bytes(manifest)
print(json.dumps({'path':str(OUT),'manifestSha256':sha(manifest),'receiptSha256':sha((OUT/'terminal-review.json').read_bytes()),'payloadCountVerified':len(seen),'pendingFourFilesIncluded':False}))
