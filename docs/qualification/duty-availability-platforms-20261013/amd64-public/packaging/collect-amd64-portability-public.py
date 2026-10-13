#!/usr/bin/env python3
"""Retrieve allowlisted PUBLIC compiler/test receipts only; no private recovery files."""
import io, json, hashlib, pathlib, subprocess, tarfile, os
BASE=pathlib.Path(__file__).parent
DEST=BASE/'duty-availability-portability-amd64-20261013'
NAMES=['fresh-output-proof.json','protected-build-before.json','protected-build-after.json',
       'source-before.json','source-after.json','scope.json',
       'compile.command.json','compile.log','compile.exit.json',
       'link.command.json','link.log','link.exit.json',
       'byte-comparison-before-execution-decision.json','terminal-receipt.json','test.o.d']
REMOTE=r'''
import pathlib,tarfile,io,sys,json,hashlib,stat,os
root=pathlib.Path('/home/ubuntu/nanobees-qualification/duty-availability-portability-amd64-20261013')
names=json.loads(sys.argv[1]); entries=[]; files={}
assert len(names)==15 and all('/' not in x and x.endswith(('.json','.log','.d')) for x in names)
for name in names:
 p=root/name; s=p.lstat(); assert stat.S_ISREG(s.st_mode) and not p.is_symlink() and s.st_size<5000000
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd); data=b''
  while True:
   part=os.read(fd,1048576)
   if not part:break
   data+=part
  z=os.fstat(fd)
 finally:os.close(fd)
 assert (a.st_dev,a.st_ino,a.st_size,a.st_mtime_ns,a.st_ctime_ns)==(z.st_dev,z.st_ino,z.st_size,z.st_mtime_ns,z.st_ctime_ns)
 files[name]=data;entries.append({'path':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
terminal=json.loads(files['terminal-receipt.json']);assert terminal['passedWithinScope'] is True
assert terminal['testObjectAndProgramByteIdentical'] and not terminal['newChangedAggregateExecutedOnce']
manifest={'format':'nano-public-portability-compiler-receipts-transfer-v1','files':entries,'noBinaryObjectArchiveDatabaseOrPrivateRecoveryPayload':True}
files['remote-public-transfer-manifest.json']=(json.dumps(manifest,indent=2)+'\n').encode()
buf=io.BytesIO()
with tarfile.open(fileobj=buf,mode='w') as tf:
 for name,data in files.items():
  info=tarfile.TarInfo(name);info.size=len(data);info.mode=0o600;tf.addfile(info,io.BytesIO(data))
sys.stdout.buffer.write(buf.getvalue())
'''
cmd=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=15','ubuntu@145.239.199.166',
     '/usr/bin/python3','-I','-c',__import__('shlex').quote(REMOTE),__import__('shlex').quote(json.dumps(NAMES))]
r=subprocess.run(cmd,capture_output=True,timeout=60)
assert r.returncode==0, r.stderr.decode()
assert len(r.stdout)<20000000
with tarfile.open(fileobj=io.BytesIO(r.stdout),mode='r:') as tf:
 members=tf.getmembers();assert {m.name for m in members}==set(NAMES)|{'remote-public-transfer-manifest.json'}
 assert all(m.isfile() and '/' not in m.name for m in members)
 payloads={m.name:tf.extractfile(m).read() for m in members}
manifest=json.loads(payloads['remote-public-transfer-manifest.json'])
for row in manifest['files']:
 data=payloads[row['path']];assert row['bytes']==len(data) and row['sha256']==hashlib.sha256(data).hexdigest()
for name,data in payloads.items():
 target=DEST/name;assert not target.exists();fd=os.open(target,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as stream:stream.write(data)
summary={'format':'nano-public-portability-transfer-local-verification-v1','payloadCount':len(manifest['files']),
         'payloadBytes':sum(r['bytes'] for r in manifest['files']),
         'manifestSha256':hashlib.sha256(payloads['remote-public-transfer-manifest.json']).hexdigest(),
         'allHashesVerified':True,'privateVpsRecoveryProofExported':False}
(DEST/'public-transfer-validation.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary))
