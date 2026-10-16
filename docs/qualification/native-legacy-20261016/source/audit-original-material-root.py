#!/usr/bin/env python3
"""Root-only read-only original-material audit; no project execution."""
from pathlib import Path
import hashlib,json,os,shlex,stat
E=Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
F=E/'fresh-native-legacy-root-20261016'
FIELDS=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
SEEN={}
def read(p):
 assert p.resolve(strict=True)==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);assert stat.S_ISREG(a.st_mode) and 0<a.st_size<67108864
  z=bytearray()
  while True:
   x=os.read(fd,1048576)
   if not x:break
   z.extend(x);assert len(z)<=a.st_size
  row={k:getattr(a,k) for k in FIELDS}
  assert len(z)==a.st_size and row=={k:getattr(os.fstat(fd),k) for k in FIELDS}=={k:getattr(p.lstat(),k) for k in FIELDS}
  row['sha256']=hashlib.sha256(z).hexdigest()
  return bytes(z),row
 finally:os.close(fd)
def fixed(p,sha,size):
 z,_=read(p);assert len(z)==size and hashlib.sha256(z).hexdigest()==sha
 return json.loads(z)
def publish(name,value):
 z=(json.dumps(value,sort_keys=True,indent=2)+'\n').encode();p=F/name
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o400)
 with os.fdopen(fd,'wb') as out:out.write(z);out.flush();os.fsync(out.fileno())
 return {'sha256':hashlib.sha256(z).hexdigest(),'bytes':len(z)}
def main():
 c=fixed(E/'fresh-rotation-thin-build-root-20261015/native-02/inputs.private.json','3ec9e61e1820d9b838554bb85b95573467de00cd1bbc5712f2cf4a8f852a5e40',1934711)
 m=fixed(E/'historical-beacon-rpc-20261015/controls/native-historical-test-protected-inputs.json','25c5441c78a55f88f4d4c4255d498f7ceb174ff075833164f8be7e1e2679a633',6869424)
 B=Path(c['buildRoot']);S=Path(c['sourceRoot'])
 def old(p):
  z,row=read(p);assert row==m[str(p)];SEEN[str(p)]=row;return z
 cc=json.loads(old(B/'compile_commands.json'));components={}
 for role,src in [('nano-capella-transition-tests','tests/test_capella_transition.cpp'),('nano-capella-isolated-bootstrap-import','tests/capella/isolated_bootstrap_import.cpp'),('nano-test-sync-fault','tests/support/sync_fault_macos.cpp')]:
  r=[x for x in cc if x['file']==str(S/src)];assert len(r)==1
  args=r[0].get('arguments') or shlex.split(r[0]['command'])
  obj=Path(args[args.index('-o')+1]);obj=obj if obj.is_absolute() else B/obj
  paths={'source':S/src,'object':obj,'actualMd':Path(str(obj)+'.d'),'link':B/'CMakeFiles'/(role+'.dir')/'link.txt','software':B/('libnano-test-sync-fault.dylib' if role=='nano-test-sync-fault' else role)}
  components[role]={}
  for kind,p in paths.items():
   z=old(p);components[role][kind]={'sha256':hashlib.sha256(z).hexdigest(),'bytes':len(z)}
 for p,row in SEEN.items():assert read(Path(p))[1]==row
 result={'format':'nano-native-legacy-original-material-preflight-v1','passed':True,'sourceBaseRevisionTarget':'7c004a00c25a5d7a7e48e115e2eeb0c0368d14fa','canonicalCompileRows':len(cc),'originalMaterialRowsVerified':len(SEEN),'components':components,'oldRowsNineShaUnchanged':True,'supportHistoricalProducerQualificationRecovered':False,'effectiveCurrentSourceClosureQualified':False,'compilerInvocations':0,'linkerInvocations':0,'projectProgramsExecuted':0,'runtimeStarted':False,'productionQualified':False}
 pin=publish('original-material-preflight-public.json',result)
 print(json.dumps(dict(pin,passed=True,oldNineShaRows=len(SEEN),effectiveClosureQualified=False)))
if __name__=='__main__':main()
