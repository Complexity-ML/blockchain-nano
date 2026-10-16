#!/usr/bin/env python3
"""ROOT-only selected ARM input audit. No Docker, SDK bytes or project execution."""
import hashlib,json,os,pathlib,shlex,stat,subprocess,time
E=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
R=pathlib.Path('/Users/boris/Dev/blockchain-nano')
D=E/'historical-beacon-rpc-20261015';S=D/'source';B=D/'linux-arm-build'
OUT=E/'fresh-arm-bootstrap-import-current-core-root-20261016'
TARGET='nano-capella-isolated-bootstrap-import';REV='944aa77b6d92e3ac609938f6d5a024acd88cc118'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
HELD={}

def need(v,m):
 if not v:raise ValueError(m)

def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}

def material(p):
 p=pathlib.Path(p);need(p.is_absolute()and p.resolve(strict=True)==p,'physical selected path')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<=a.st_size<=268435456,'bounded selected regular FD')
  chunks=[];n=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   n+=len(b);need(n<=a.st_size,'FD bound');chunks.append(b)
  row={k:getattr(a,k)for k in F};need(n==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'FD/path nine unchanged')
  b=b''.join(chunks);return dict(row,sha256=hashlib.sha256(b).hexdigest()),b
 finally:os.close(fd)

def record(p,row):
 seen,b=material(p);need(seen==row and(str(p)not in HELD or HELD[str(p)]==row),'original selected authority changed')
 HELD[str(p)]=row;return b

def known(p,sha,size):
 _,b=material(p);need(pin(b)=={'sha256':sha,'bytes':size},'known raw descriptor differs');return json.loads(b)

def write(name,x):
 b=(json.dumps(x,sort_keys=True,separators=(',',':'))+'\n').encode();p=OUT/name
 if p.exists():
  need(name in ('selected-original-closure.private.json','selected-current-source.private.json','selected-current-core.private.json')and material(p)[1]==b,'only exact unadmitted V1 descriptors may be reused')
  return dict(path=str(p),**pin(b))
 fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
 try:
  v=memoryview(b)
  while v:n=os.write(fd,v);need(n>0,'descriptor write');v=v[n:]
  os.fsync(fd)
 finally:os.close(fd)
 need(material(p)[1]==b,'descriptor readback');return dict(path=str(p),**pin(b))

def main():
 need(subprocess.check_output(['git','-C',str(R),'rev-parse','HEAD']).decode().strip()==REV,'current main revision')
 need(not subprocess.check_output(['git','-C',str(R),'status','--porcelain','--untracked-files=all']),'current main clean')
 oldinput=known(D/'controls/arm-historical-test-inputs.json','ea754f3bc60c4aad479145a90da7893b6f350f387667d71ee92e9c0e2f8b4022',1358)
 pref=oldinput['protectedInputs'];need(pref['path']==str(D/'controls/arm-historical-test-protected-inputs.json')and(pref['sha256'],pref['bytes'])==('24460bbd5eb05836ffe1835737f2cf36852075f81a28ba6a947fcd8500108ae7',6055965),'original input records protected raw commitment')
 protected=known(pathlib.Path(pref['path']),pref['sha256'],pref['bytes']);need(len(protected)==12373,'original protected row count')
 rotation=known(E/'fresh-rotation-thin-build-root-20261015/arm-01/admitted.private.json','6219c22300c8e68bcd7842603a50db81eca1619bf2bc8c960c2b1ee647e9fbe2',59003)
 def original(p):
  need(str(p)in protected,'selected original row absent');return record(p,protected[str(p)])
 db=B/'compile_commands.json';dbraw=original(db);need(rotation['hostFiles'][str(db)]==protected[str(db)],'compile DB two retained authorities disagree')
 candidates=[z for z in json.loads(dbraw)if z['file']=='/work/tests/capella/isolated_bootstrap_import.cpp'];need(len(candidates)==1,'unique actual importer recipe');recipe=candidates[0]
 need(recipe['directory']=='/outputs','actual compile cwd');argv=shlex.split(recipe['command']);need(argv[0]=='/usr/bin/clang++'and argv.count('-o')==argv.count('-c')==1,'canonical driver/object/source recipe')
 obj=argv[argv.index('-o')+1];obj=os.path.normpath(obj if obj.startswith('/')else'/outputs/'+obj)
 def host(guest):
  need(guest==os.path.normpath(guest),'normalized guest')
  if guest.startswith('/work/'):return S/pathlib.PurePosixPath(guest).relative_to('/work')
  need(guest.startswith('/outputs/'),'host source/build only');return B/pathlib.PurePosixPath(guest).relative_to('/outputs')
 object_raw=original(host(obj));mdpath=host(obj+'.d');md=original(mdpath)
 deps=sorted({os.path.normpath(p if p.startswith('/')else'/outputs/'+p)for p in shlex.split(md.decode().replace('\\\n',' ').split(':',1)[1])})
 need(recipe['file']in deps and '/work/tests/support/fresh_lifecycle_terminal.hpp'not in deps,'actual importer dependency source/header scope')
 project={};generated={};image=[];source_hashes={}
 for guest in deps:
  if guest.startswith(('/work/','/outputs/')):
   p=host(guest);raw=original(p);row={'path':str(p),'material':protected[str(p)]}
   if guest.startswith('/work/'):
    project[guest]=row;source_hashes[str(p.relative_to(S))]=pin(raw)
   else:generated[guest]=row
  else:need(guest.startswith(('/usr/','/bin/','/lib/','/lib64/','/src/.deps/')),'bounded image MD namespace');image.append(guest)
 linkpath=B/f'CMakeFiles/{TARGET}.dir/link.txt';linkraw=original(linkpath);link=shlex.split(linkraw.decode());need(pin(linkraw)=={'sha256':'165ee34babd5461fdb8d6bddd76593b54ad39e1ad786511ef48f0ef0984343ea','bytes':597},'actual known importer link bytes')
 need(link[0]==argv[0]and link.count('-o')==1,'actual link driver/output')
 actual_inputs=[]
 for word in link:
  if word.endswith(('.o','.a')):
   guest=os.path.normpath(word if word.startswith('/')else'/outputs/'+word);original(host(guest));actual_inputs.append(guest)
 need(actual_inputs==[obj,'/outputs/third_party/mcl/bint64.o','/outputs/third_party/mcl/base64.o','/outputs/libnano_core.a','/outputs/third_party/ckzg/libnano_ckzg.a','/outputs/third_party/ckzg/libnano_blst.a','/outputs/third_party/secp256k1/lib/libsecp256k1.a','/outputs/lib/libmcl.a'],'actual original link order')
 executable=original(B/TARGET);need(pin(executable)=={'sha256':'3b635993dccc50458343622a148345692b0d88b0a066a68c78381cc659247095','bytes':4765512},'original importer unchanged')
 retained=known(E/'fresh-linux-lifecycle-tool-u64-root-20261016/arm-02/new-software-admission.private.json','a4772c9bc694d8f43c08b52f92b3e7982b089563e89c5c4507fa01acc7032827',298848)
 core=E/'beacon-validator-read-implementation-20261015/arm-03/libnano_core.a';coreraw=record(core,retained['hostFiles'][str(core)])
 need(pin(coreraw)=={'sha256':'9ccadf8f6479af8610a823edd3a7b4de5752cf3bde5d6251532b734c3c0a6974','bytes':41048912},'current Core qualified identity')
 names=sorted(source_hashes);queries=''.join(REV+':'+name+'\n'for name in names).encode();raw=subprocess.check_output(['git','-C',str(R),'cat-file','--batch'],input=queries);at=0
 for name in names:
  end=raw.index(b'\n',at);oid,kind,size=raw[at:end].split();need(kind==b'blob','current Git blob kind');size=int(size);body=raw[end+1:end+1+size];need(len(body)==size and raw[end+1+size:end+2+size]==b'\n','current Git blob bounds');at=end+2+size
  need(hashlib.sha1(b'blob '+str(size).encode()+b'\0'+body).hexdigest().encode()==oid and pin(body)==source_hashes[name],'selected original source differs from current Git')
 need(at==len(raw),'Git response exact')
 for p,row in list(HELD.items()):record(pathlib.Path(p),row)
 common={'oldAuthoritiesRefreshed':False,'historicalInputRawDigestRecoveredOffsite':False,'historicalProtectedTableFullNineRecovered':False,'originalProtectedDigestRecordedInInput':True,'sdkBytesRead':False,'productionQualified':False}
 closure=write('selected-original-closure.private.json',{'format':'nano-arm-import-original-closure-v1','sourceRoot':str(S),'buildRoot':str(B),'compileDatabasePath':str(db),'compileRecipe':recipe,'linkRecipePath':str(linkpath),'linkArgv':link,'objectGuestPath':obj,'originalMdPath':str(mdpath),'originalExecutablePath':str(B/TARGET),'guestDependencies':deps,'projectDependencies':project,'generatedDependencies':generated,'imageDependencies':image,'linkInputsGuestOrder':actual_inputs,'files':{p:row for p,row in HELD.items()if pathlib.Path(p).is_relative_to(D)},'originalInput':{'path':str(D/'controls/arm-historical-test-inputs.json'),'sha256':'ea754f3bc60c4aad479145a90da7893b6f350f387667d71ee92e9c0e2f8b4022','bytes':1358},'protectedTable':pref,'compileDbNineAlsoRetainedInRotationAdmission':True,'consumerLinkOriginalNineRetained':True,**common})
 current=write('selected-current-source.private.json',{'format':'nano-arm-import-current-source-v1','repositoryRoot':str(R),'revision':REV,'projectFiles':source_hashes,'projectDependencyCount':len(project),'allSelectedSourceBytesMatchCurrentGit':True,'changedTerminalHeaderReached':False,**common})
 core_ref=write('selected-current-core.private.json',{'format':'nano-arm-import-current-core-v1','corePath':str(core),'coreMaterial':retained['hostFiles'][str(core)],'retainedAdmission':{'path':str(E/'fresh-linux-lifecycle-tool-u64-root-20261016/arm-02/new-software-admission.private.json'),'sha256':'a4772c9bc694d8f43c08b52f92b3e7982b089563e89c5c4507fa01acc7032827','bytes':298848},'retainedCoreFullNineUnchanged':True,'originalFirstProducerCoreNineRecovered':False,**common})
 refs={'originalClosure':closure,'currentSourceAdmission':current,'currentCoreAdmission':core_ref};previous=None;since=time.monotonic();samples=0;deadline=since+20
 while True:
  observed={}
  for k,v in refs.items():
   row,body=material(pathlib.Path(v['path']));need(pin(body)=={x:v[x]for x in ('sha256','bytes')},'new known descriptor bytes changed');observed[k]=row
  now=time.monotonic()
  if observed==previous:samples+=1
  else:previous=observed;samples=1;since=now
  if samples>=3 and now-since>=2:break
  need(now<deadline,'new descriptor quiescence deadline');time.sleep(.8)
 write('selected-descriptor-admission.private.json',{'format':'nano-arm-import-selected-descriptor-admission-v1','refs':refs,'materials':previous,'samples':samples,'stableSpanMs':int((time.monotonic()-since)*1000),'operationalAuthority':False})
 result={'format':'nano-arm-import-input-audit-public-v1','passed':True,'sourceRevision':REV,'originalSelectedRowsUnchanged':len(HELD)-1,'currentCoreRetainedNineShaUnchanged':True,'dependencyCount':len(deps),'projectDependencyCount':len(project),'generatedDependencyCount':len(generated),'imageDependencyNamesCount':len(image),'compileDatabaseOriginalNineRetained':True,'consumerLinkOriginalNineRetained':True,'actualStaticArchives':5,'actualStandaloneAssemblyObjects':2,'newSelectedDescriptors':{k:{a:v[a]for a in('sha256','bytes')}for k,v in refs.items()},'initialNewDescriptorStabilityRefusalPreserved':True,'unadmittedV1DescriptorBytesReused':True,'newDescriptorSamples':samples,'newDescriptorStableSpanMs':int((time.monotonic()-since)*1000),'rawNineFieldsExported':False,'compilerInvocations':0,'dockerCommands':0,'newCliSelectors':0,'runtimeStarted':False,**common}
 write('input-audit-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':main()
