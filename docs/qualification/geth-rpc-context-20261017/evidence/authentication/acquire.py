import pathlib, subprocess, hashlib, json, os, tarfile, stat, time, xml.etree.ElementTree as ET
P=pathlib.Path(__file__).resolve().parent
os.umask(0o077)
assert not (P/'admission-public.json').exists()
FPR='FDE5A1A044FA13D2F7ADA019A61A13569BA28146'
NAME='geth-linux-arm64-1.16.0-57f8971b.tar.gz'
sources={
 'constants.ts':'https://raw.githubusercontent.com/ethereum/go-ethereum/website/src/constants.ts',
 'release-ref.json':'https://api.github.com/repos/ethereum/go-ethereum/git/ref/tags/v1.16.0',
 'inventory.xml':'https://gethstore.blob.core.windows.net/builds?restype=container&comp=list&prefix=geth-linux-arm64-1.16.0-57f8971b',
 'linux-builder.asc':'https://keyserver.ubuntu.com/pks/lookup?op=get&search=0xA61A13569BA28146',
 NAME:'https://gethstore.blob.core.windows.net/builds/'+NAME,
 NAME+'.asc':'https://gethstore.blob.core.windows.net/builds/'+NAME+'.asc',
}
def run(args,out):
 with (P/out).open('xb') as f:
  x=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,timeout=120)
 assert x.returncode==0, (args[0],out,x.returncode)
for name,url in sources.items():
 assert not (P/name).exists()
 run(['/usr/bin/curl','--fail','--silent','--show-error','--location','--proto','=https','--proto-redir','=https','--tlsv1.2','--max-time','100','--max-filesize','150000000','--output',str(P/name),url],name+'.fetch.log')
text=(P/'constants.ts').read_text()
assert 'FDE5 A1A0 44FA 13D2 F7AD  A019 A61A 1356 9BA2 8146' in text and sources['linux-builder.asc'] in text
assert json.loads((P/'release-ref.json').read_bytes())['object']=={'sha':'57f8971b89b3ce3d31ec09d6a1e29151c90e7d19','type':'commit','url':'https://api.github.com/repos/ethereum/go-ethereum/git/commits/57f8971b89b3ce3d31ec09d6a1e29151c90e7d19'}
names=[x.text for x in ET.fromstring((P/'inventory.xml').read_bytes()).findall('./Blobs/Blob/Name')]
assert NAME in names and NAME+'.asc' in names
G=P/'gnupg';G.mkdir(mode=0o700)
gpg=['/opt/homebrew/bin/gpg','--homedir',str(G),'--batch','--no-auto-key-retrieve']
run(gpg+['--with-colons','--show-keys',str(P/'linux-builder.asc')],'key-inspection.txt')
lines=(P/'key-inspection.txt').read_text().splitlines(); pubs=[]
for i,line in enumerate(lines):
 if line.startswith('pub:'):
  assert lines[i+1].startswith('fpr:');pubs.append(lines[i+1].split(':')[9])
assert pubs==[FPR],pubs
run(gpg+['--import',str(P/'linux-builder.asc')],'import.txt')
run(gpg+['--status-fd','1','--verify',str(P/(NAME+'.asc')),str(P/NAME)],'signature-verification.txt')
statuses=(P/'signature-verification.txt').read_text()
valid=[x for x in statuses.splitlines() if x.startswith('[GNUPG:] VALIDSIG ')]
assert len(valid)==1 and valid[0].split()[-1]==FPR and valid[0].split()[2]==FPR
assert not any('[GNUPG:] '+x in statuses for x in ['BADSIG','ERRSIG','REVKEYSIG','EXPSIG','EXPKEYSIG','FAILURE','NODATA'])
with tarfile.open(P/NAME,'r:gz') as archive:
 members=archive.getmembers(); candidates=[m for m in members if m.name.endswith('/geth')]
 assert len(candidates)==1 and candidates[0].isfile() and not candidates[0].issym() and not candidates[0].islnk()
 assert len(members)<12 and candidates[0].size<150000000
 b=archive.extractfile(candidates[0]).read(); assert len(b)==candidates[0].size and b[:4]==b'\x7fELF' and b[4:6]==b'\x02\x01' and int.from_bytes(b[18:20],'little')==183
 with (P/'geth').open('xb') as f:f.write(b)
 (P/'geth').chmod(0o500)
 (P/'archive-members.json').write_text(json.dumps([{'name':m.name,'bytes':m.size,'type':str(m.type)} for m in members])+'\n')
time.sleep(2)
def record(p):
 with p.open('rb') as f:
  s=os.fstat(f.fileno());b=f.read();z=os.fstat(f.fileno());fields=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
  m={k:getattr(s,k) for k in fields};assert m=={k:getattr(z,k) for k in fields}=={k:getattr(p.lstat(),k) for k in fields} and stat.S_ISREG(s.st_mode) and s.st_nlink==1
  return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'material':m}
r={'schema':'nano.geth.official-acquisition.v1','version':'1.16.0','releaseCommit':'57f8971b89b3ce3d31ec09d6a1e29151c90e7d19','linuxBuilderPrimaryFingerprint':FPR,'detachedSignatureVerified':True,'sourceURLs':sources,'archive':record(P/NAME),'binary':record(P/'geth'),'signature':record(P/(NAME+'.asc')),'key':record(P/'linux-builder.asc'),'sha256Provenance':'Computed locally after authenticated acquisition; not claimed as a previously published SHA-256','executed':False,'imageBuilds':0,'newConsensusKeys':0}
(P/'admission-public.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps({k:r[k] for k in ('version','releaseCommit','linuxBuilderPrimaryFingerprint','detachedSignatureVerified','archive','binary','executed')}))
