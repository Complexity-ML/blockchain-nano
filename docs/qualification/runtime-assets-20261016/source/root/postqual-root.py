#!/usr/bin/env python3
"""ROOT readback of the completed ARM producer; no project/SDK execution."""
import hashlib,json,os,pathlib,stat,subprocess
ROOT=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
HERE=ROOT/'fresh-arm-bootstrap-import-current-core-root-20261016';OUT=HERE/'arm-01'
REV='944aa77b6d92e3ac609938f6d5a024acd88cc118'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')

def need(v,s):
 if not v:raise ValueError(s)

def read(p):
 p=pathlib.Path(p);need(p.resolve(strict=True)==p,'physical retained file')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and 0<=a.st_size<=268435456,'bounded regular FD');chunks=[];size=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   size+=len(b);need(size<=a.st_size,'size bound');chunks.append(b)
  row={k:getattr(a,k)for k in F};need(size==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'FD/path nine fields')
  b=b''.join(chunks);return dict(row,sha256=hashlib.sha256(b).hexdigest()),b
 finally:os.close(fd)

def known(p,sha,size):
 row,b=read(p);need((row['sha256'],len(b))==(sha,size),'known descriptor bytes');return json.loads(b)

def equal(p,old):
 row,b=read(p);need(row==old,'retained full nine SHA differs');return b

def main():
 t=known(OUT/'terminal.json','e476fa44f47fe53109eb43e46bbea55e8edc2f1376aeef6e6087412566cb6247',6885)
 stdout=known(HERE/'execution-stdout-public.json','bbe72b99b19c902ee21aa5e56764f0b180f7fd587d598e043e204bd475d63a1b',7534);need(t==stdout,'two exact serializations same object')
 need(t['passed']is True and len(t['events'])==44 and all(x['passed']is True and x['exitCode']==(1 if x['phase'].endswith('-arguments-refusal')else 0)for x in t['events']),'actual 44-phase terminal including expected refusal')
 need(t['ownedContainerAbsent']is True and t['ownedVolumeAbsent']is True and t['consumerCompilations']==0 and t['consumerLinks']==t['dependencyOnlyM']==t['argumentsRefusalProbes']==1,'actual minimal producer and cleanup scope')
 ad=known(OUT/'new-software-admission.private.json','2b50813e51d52a7284bcabafa1c543463a40733f2ea8458e8bb29649d624f067',517971)
 held=ad['usedHostMaterials'];need(80<len(held)<200,'bounded used host material namespace')
 for p,row in held.items():equal(p,row)
 original=known(HERE/'selected-original-closure.private.json','566d2db8a58f544ca4574e9b08cd20b42c3f5384b1ad507d112322d78c6b39cb',107669)
 need(len(original['files'])==77 and all(p in held and held[p]==row for p,row in original['files'].items()),'77 original selected rows unchanged')
 program=ad['program'];need(program['path']==str(OUT/'nano-capella-isolated-bootstrap-import'),'new owned binary locator');new=equal(program['path'],program['material']);old=equal(original['originalExecutablePath'],held[original['originalExecutablePath']])
 need(new==old and hashlib.sha256(new).hexdigest()=='3b635993dccc50458343622a148345692b0d88b0a066a68c78381cc659247095'and len(new)==4765512,'actual bytes equal historical importer')
 need(program['material']['st_uid']==501 and program['material']['st_nlink']==1 and stat.S_IMODE(program['material']['st_mode'])==0o500,'new host binary ownership/mode')
 compile_sdk=ad['guestCompileSdkMaterials'];extra=ad['guestAdditionalRuntimeSdkMaterials'];sdk=ad['guestSdkMaterials']
 need(len(extra)==18 and set(compile_sdk).isdisjoint(extra)and sdk=={**compile_sdk,**extra},'distinct initial compilation SDK and runtime additions')
 repo='/Users/boris/Dev/blockchain-nano';need(subprocess.check_output(['git','-C',repo,'rev-parse','HEAD']).decode().strip()==REV and not subprocess.check_output(['git','-C',repo,'status','--porcelain','--untracked-files=all']),'current source revision clean')
 for p,row in held.items():equal(p,row)
 result={'format':'nano-arm-import-current-core-postqualification-v1','passed':True,'sourceRevision':REV,'originalSelectedRowsUnchanged':77,'allUsedHostRowsUnchanged':len(held),'newBinaryNineShaUnchanged':True,'newBinaryActualBytesEqualHistoricalImporter':True,'newBinary':{'sha256':hashlib.sha256(new).hexdigest(),'bytes':len(new)},'currentCoreEquivalenceQualifiedForThisImporter':True,'initialCompileSdkRowsRecorded':len(compile_sdk),'additionalRuntimeSdkRowsRecorded':len(extra),'sameContainerTotalSdkRowsRecorded':len(sdk),'sdkRowsRereadAfterContainerRemoval':0,'compilerInvocations':0,'newCliSelectors':0,'dockerCommands':0,'rawNineFieldsExported':False,'oldAuthoritiesRefreshed':False,'historicalSdkAuthorityRecovered':False,'historicalInputRawDigestRecoveredOffsite':False,'historicalProtectedTableFullNineRecovered':False,'originalFirstProducerCoreNineRecovered':False,'historicalImport65EvidenceRequalified':False,'runtimeStarted':False,'dutiesResumed':False,'commonFinalityQualified':False,'productionQualified':False}
 b=(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n').encode();p=HERE/'postqualification-public.json';fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o400)
 try:os.write(fd,b);os.fsync(fd)
 finally:os.close(fd)
 need(read(p)[1]==b,'public result readback');print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':main()
