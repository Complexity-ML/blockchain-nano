import pathlib,subprocess,hashlib,json,os,tarfile,stat,time,xml.etree.ElementTree as ET
P=pathlib.Path(__file__).resolve().parent;os.umask(0o077)
FPR='FDE5A1A044FA13D2F7ADA019A61A13569BA28146';NAME='geth-linux-arm64-1.16.0-57f8971b.tar.gz'
assert not (P/'admission-public.json').exists() and not (P/'geth').exists()
fields=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def record(p):
 with p.open('rb') as f:
  s=os.fstat(f.fileno());b=f.read();z=os.fstat(f.fileno());m={k:getattr(s,k) for k in fields}
  assert m=={k:getattr(z,k) for k in fields}=={k:getattr(p.lstat(),k) for k in fields} and stat.S_ISREG(s.st_mode) and s.st_nlink==1
  return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'material':m}
held={n:record(P/n) for n in ['constants.ts','release-ref.json','inventory.xml','linux-builder.asc',NAME,NAME+'.asc','gnupg/pubring.kbx','import.txt','key-inspection.txt']}
assert 'FDE5 A1A0 44FA 13D2 F7AD  A019 A61A 1356 9BA2 8146' in (P/'constants.ts').read_text()
assert json.loads((P/'release-ref.json').read_bytes())['object']['sha']=='57f8971b89b3ce3d31ec09d6a1e29151c90e7d19'
names=[x.text for x in ET.fromstring((P/'inventory.xml').read_bytes()).findall('./Blobs/Blob/Name')];assert NAME in names and NAME+'.asc' in names
rows=(P/'key-inspection.txt').read_text().splitlines();pubs=[rows[i+1].split(':')[9] for i,x in enumerate(rows) if x.startswith('pub:')];assert pubs==[FPR]
with (P/'signature-verification.txt').open('xb') as f:
 x=subprocess.run(['/opt/homebrew/bin/gpgv','--homedir',str(P/'gnupg'),'--keyring',str(P/'gnupg/pubring.kbx'),'--status-fd','1',str(P/(NAME+'.asc')),str(P/NAME)],stdout=f,stderr=subprocess.STDOUT,timeout=30)
assert x.returncode==0
statuses=(P/'signature-verification.txt').read_text();valid=[x for x in statuses.splitlines() if x.startswith('[GNUPG:] VALIDSIG ')]
assert len(valid)==1 and valid[0].split()[-1]==FPR and valid[0].split()[2]==FPR
assert not any('[GNUPG:] '+x in statuses for x in ['BADSIG','ERRSIG','REVKEYSIG','EXPSIG','EXPKEYSIG','FAILURE','NODATA'])
assert all(record(P/n)==v for n,v in held.items())
with tarfile.open(P/NAME,'r:gz') as archive:
 members=archive.getmembers();c=[m for m in members if m.name.endswith('/geth')]
 assert len(c)==1 and c[0].isfile() and not c[0].issym() and not c[0].islnk() and len(members)<12 and c[0].size<150000000
 b=archive.extractfile(c[0]).read();assert len(b)==c[0].size and b[:6]==b'\x7fELF\x02\x01' and int.from_bytes(b[18:20],'little')==183
 with (P/'geth').open('xb') as f:f.write(b)
 (P/'geth').chmod(0o500)
 (P/'archive-members.json').write_text(json.dumps([{'name':m.name,'bytes':m.size,'type':str(m.type)} for m in members])+'\n')
time.sleep(2)
r={'schema':'nano.geth.official-acquisition.v1','version':'1.16.0','releaseCommit':'57f8971b89b3ce3d31ec09d6a1e29151c90e7d19','linuxBuilderPrimaryFingerprint':FPR,'detachedSignatureVerified':True,'archive':record(P/NAME),'binary':record(P/'geth'),'signature':record(P/(NAME+'.asc')),'key':record(P/'linux-builder.asc'),'sourcePins':held,'sha256Provenance':'Computed locally after authenticated acquisition; not claimed as a previously published SHA-256','executed':False,'imageBuilds':0,'newConsensusKeys':0,'acquisitionToolFailurePreserved':'acquire.py: gpg import returned2 after importing public key, agent socket path too long; gpgv detached verification uses that unchanged public keyring, no agent and no redownload'}
(P/'admission-public.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:r[k] for k in ['version','detachedSignatureVerified','archive','binary','executed']}))
