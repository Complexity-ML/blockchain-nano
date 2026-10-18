"""SOURCE_ONLY: Root's read-only post-terminal ARM03 audit and closed public readback."""
import hashlib,json,os,pathlib,posixpath,re,shlex,signal,stat,struct,subprocess,sys,time
E=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
D=E/'real-journal-rollback-arm-source-proposal-20261018-03';O=E/'real-journal-rollback-arm-root-20261018-03';P=O/'outputs';A=E/'journal-arm03-passive-root-20261018-01'
OLD=E/'physical-cohort-arm-control-root-20261018-05/control.private.json';OLD_PIN={'sha256':'3a1697c45c0fde5257963392715268013767a8ff53cba9ce68b3f6e5d0f97a3a','bytes':186873}
SOURCE=E/'physical-cohort-arm-root-20261018-05/fresh-source-projections-sample-1.private.json';SOURCE_PIN={'sha256':'0b4b3c84dc4ed99d297278e6ceda22422fa21d3941e08fed88332edf51219f8e','bytes':623344}
INVENTORY=E/'physical-cohort-volume-export-root-20261018-01/export-inventory-public.json';INVENTORY_PIN={'sha256':'19d3c604fb202602bc9d5a33e92aa89bb059034f229b419fd505973a88e38bae','bytes':18623}
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
# Only these PUBLIC source files may be read back. No private carrier, binary, DB or directory recursion.
PUBLIC=[(O/'result-public.json','host-result-public.json',262144),(O/'host-final-fence-public.json','host-final-fence-public.json',65536),(P/'result-public.json','worker-result-public.json',131072),(P/'RUN.stdout.log','RUN.stdout.log',65536),(P/'RUN.stderr.log','RUN.stderr.log',65536),*[(P/(phase+'.exit-public.json'),phase+'.exit-public.json',16384)for phase in('M','CXX','LINK','LDD','RUN')],(P/'loader-closure-public.json','loader-closure-public.json',65536),*[(O/(phase+'-public.json'),phase+'-public.json',262144)for phase in('fixture-before-RUN','fixture-after-RUN','fixture-after-cleanup')],(O/'source-before-public.json','source-before-public.json',65536),(O/'source-after-cleanup-public.json','source-after-cleanup-public.json',65536),(E/'real-journal-rollback-arm-control-root-20261018-03/preparation-result-public.json','preparation-result-public.json',131072)]
SOURCE_PINS={D/'host.py':{'sha256':'c9f0ab2c6295905379d06354b9f224cb09c48a09c86264611657499de55eb0f1','bytes':33108},D/'worker.py':{'sha256':'e88d6f69acb7a0a3bb688bd1c5482bd919e914a193a3398362a338c2c4ae5e6d','bytes':20025},D/'prepare-root.py':{'sha256':'d726bffd0aa5ebb49936e17e6754927eecf70f8e7766c476332032304a969acd','bytes':11060},E/'real-journal-rollback-arm-case-root-20261018-03/tests/qualification/beacon_journal_physical_rollback.cpp':{'sha256':'2e5ef451f13cca770cf02128bbb389eaae60379974252859829e197d14521acd','bytes':28801},E/'real-journal-rollback-arm-case-root-20261018-03/tools/storage-maintenance/bootstrap-readonly-replica.hpp':{'sha256':'d3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125','bytes':20319}}
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def same(a,b):return json.dumps(a,sort_keys=True,allow_nan=False)==json.dumps(b,sort_keys=True,allow_nan=False)
def decode(raw):
 def unique(items):
  d={}
  for k,v in items:need(k not in d,'duplicate JSON key');d[k]=v
  return d
 return json.loads(raw,object_pairs_hook=unique,parse_constant=lambda _:(_ for _ in()).throw(ValueError('nonfinite JSON')))
def material(path,maximum=134217728):
 p=pathlib.Path(path);need(p.is_absolute()and p.resolve(strict=True)==p,'physical exact file');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);row={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and a.st_nlink>=1 and 0<=a.st_size<=maximum,'bounded regular file');chunks=[];n=0
  while b:=os.read(fd,65536):n+=len(b);need(n<=a.st_size,'input grew');chunks.append(b)
  raw=b''.join(chunks);need(n==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'full9/hash/path coherence');return dict(row,sha256=pin(raw)['sha256']),raw
 finally:os.close(fd)
def write_new(path,raw):
 need(path.parent.resolve(strict=True)==path.parent and not os.path.lexists(path),'new physical output');fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb')as f:f.write(raw);f.flush();os.fchmod(f.fileno(),0o400);os.fsync(f.fileno());born={k:getattr(os.fstat(f.fileno()),k)for k in F}
 fd=os.open(path.parent,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 try:os.fsync(fd)
 finally:os.close(fd)
 row,b=material(path);need(b==raw and all(row[k]==born[k]for k in F[:8]),'created bytes/eight identities unchanged');return pin(raw)
def main():
 held={};first=None;secondary=[];events=[];copied=[];documents={};prefix=None;software_ready=False;image_final=None;absent=[];consumed=set();mmd=None;elf=None;body_record=None;external={};out_created=False
 def fail(exc,stage):
  nonlocal first
  item={'stage':stage,'type':type(exc).__name__,'message':str(exc)[:1000]}
  if first is None:first=item
  else:secondary.append(item)
 def hold(path,want=None,known=None,maximum=134217728):
  row,raw=material(path,maximum);need(want is None or row==want,'genuine original row changed: '+pathlib.Path(path).name);need(known is None or pin(raw)==known,'externally known bytes changed: '+pathlib.Path(path).name);key=str(path);need(key not in held or held[key]==row,'audit input refresh forbidden');held.setdefault(key,row);return raw
 def command(phase,args):
  p=None;err=None;joined=False;raw=b'';stderr=b'';began=time.monotonic()
  try:
   p=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True,env={'HOME':str(pathlib.Path.home()),'PATH':'/usr/bin:/bin','LANG':'C','LC_ALL':'C'});need(p.pid>1 and p.pid!=28722,'safe owned audit process');raw,stderr=p.communicate(timeout=15);need(len(raw)<=1048576 and len(stderr)<=65536,'bounded inspect response')
  except BaseException as exc:err=exc
  finally:
   if p is not None and p.pid>1 and p.pid!=28722:
    try:
     try:os.killpg(p.pid,signal.SIGTERM)
     except ProcessLookupError:pass
     try:p.wait(timeout=3)
     except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait(timeout=3)
     try:os.killpg(p.pid,0)
     except ProcessLookupError:joined=True
     need(joined,'owned audit group remains')
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-join','message':str(exc)})
    for stream in(p.stdout,p.stderr):
     try:stream.close()
     except BaseException as exc:
      if err is None:err=exc
      else:secondary.append({'stage':phase+'-close','message':str(exc)})
   event={'phase':phase,'exitCode':None if p is None else p.returncode,'groupClosed':joined,'elapsedMs':round((time.monotonic()-began)*1000)};events.append(event)
   if out_created:
    try:write_new(A/(phase+'.exit-public.json'),(json.dumps(event,sort_keys=True)+'\n').encode())
    except BaseException as exc:
     if err is None:err=exc
     else:secondary.append({'stage':phase+'-event','message':str(exc)})
  if err is not None:raise err
  return p.returncode,raw,stderr
 def image(phase):
  rc,raw,_=command(phase,prefix+['image','inspect',IMAGE]);x=decode(raw);need(rc==0 and type(x)is list and len(x)==1 and x[0]['Id']==IMAGE and x[0]['Architecture']=='arm64'and x[0]['Os']=='linux','stable pinned image identity');return {'passed':True,'image':IMAGE,'historicalLoaderSdkImageFD9Qualified':False}
 def dependencies(raw):
  need(len(raw)<=4194304,'bounded M/MD');target,sep,text=raw.decode().replace('\\\n',' ').partition(':');need(sep and shlex.split(target)==['/outputs/journal.o'],'exact one closed MT');names=sorted(set(posixpath.normpath(posixpath.join('/outputs',v))for v in shlex.split(text)));need(0<len(names)<=20000 and all(v.startswith('/')for v in names),'bounded absolute dependencies');return names
 try:
  need(len(sys.argv)==5,'usage passive-audit-root.py HOST_RESULT_SHA BYTES WORKER_AUTHORITY_SHA BYTES');need(all(re.fullmatch('[0-9a-f]{64}',sys.argv[i])for i in(1,3)),'external SHA arguments');external={'hostResult':{'sha256':sys.argv[1],'bytes':int(sys.argv[2])},'workerAuthority':{'sha256':sys.argv[3],'bytes':int(sys.argv[4])}};need(all(0<v['bytes']<=8388608 for v in external.values()),'external document bounds')
  need(not os.path.lexists(A)and A.parent.resolve(strict=True)==A.parent,'fresh passive audit output');old=decode(hold(OLD,known=OLD_PIN));origin=decode(hold(SOURCE,known=SOURCE_PIN));host=decode(hold(O/'result-public.json',known=external['hostResult']));authority=decode(hold(P/'final-observations.private.json',known=external['workerAuthority']));need(host['passed']is True,'actual terminal host PASS')
  originals=old['originalFiles'];need(type(originals)is dict and len(originals)==9,'exact nine original host software rows')
  for path,row in originals.items():hold(path,want=row)
  prefix=old['docker']['prefix'];need(old['docker']['image']==IMAGE and len(prefix)==5 and prefix[0]in originals,'original ARM Docker prefix');software_ready=True;A.mkdir(mode=0o700);out_created=True;image('image-before')
  source_pins={}
  for path,known in SOURCE_PINS.items():hold(path,known=known);source_pins[path.name]=known
  source_pins.update(oldControl=OLD_PIN,originalSourceAuthority=SOURCE_PIN,originalPortableInventory=INVENTORY_PIN)
  products=authority['products'];wanted={str(P/name)for name in('journal.o.M','journal.o','journal.o.d','rollback')};need(set(products)==wanted,'exact four original build products');payloads={}
  for path,record in products.items():
   need(type(record)is dict and set(record)=={'hostFD9','guestStatSha'},'separate original product host/guest rows');raw=hold(path,want=record['hostFD9']);need(pin(raw)=={'sha256':record['guestStatSha']['sha256'],'bytes':record['guestStatSha']['bytes']},'original product host/guest bytes');payloads[pathlib.Path(path).name]=raw
  raw=payloads['rollback'];need(len(raw)>=64 and raw[:7]==b'\x7fELF\x02\x01\x01'and struct.unpack_from('<H',raw,18)[0]==183,'ELF64 little-endian AArch64');elf={'sha256':pin(raw)['sha256'],'bytes':len(raw),'eMachine':183,'class':64,'littleEndian':True}
  left=dependencies(payloads['journal.o.M']);right=dependencies(payloads['journal.o.d']);need(left==right,'literal M equals MD');aliases=authority['guestAliases'];need(all(p in aliases for p in left),'all dependencies have original guest aliases');normalized=sorted({aliases[p]for p in left});worker=decode(hold(P/'result-public.json'));need(same(worker,host['worker'])and worker['passed']is True and worker['mmd']['normalizedMEqualsMd']is True and worker['mmd']['dependencyCount']==len(normalized),'terminal worker/MMD joins');mmd={'literalMEqualsMd':True,'normalizedMEqualsMd':True,'dependencyCount':len(normalized)}
  src=pathlib.Path(old['sourceRoot']['root']);need(len(old['sourceRoot']['pins'])==1047 and len(origin['rows'])==1050,'original source authority scope')
  for guest in left:
   if guest.startswith('/work/'):
    rel=guest[len('/work/'):];path=str(src/rel);need(rel in old['sourceRoot']['pins']and path in origin['rows']and aliases[guest]==guest,'consumed source maps exact old host path');hold(path,want=origin['rows'][path],known=old['sourceRoot']['pins'][rel]);consumed.add(path)
  need(len(consumed)==host['consumedOriginalSourceFiles'],'host consumed source count agrees independently')
  need(worker['counts']=={'independentM':1,'cppCompilations':1,'links':1,'loaderChecks':1,'runs':1,'CoreM':0,'CoreCpp':0,'archiveUpdates':0},'single-TU actual counts')
  for phase in('M','CXX','LINK','LDD','RUN'):
   event=decode(hold(P/(phase+'.exit-public.json')));matching=[v for v in host['processEvents']if v['phase']=='worker-'+phase];need(event['phase']==phase and type(event['actualExitCode'])is int and event['actualExitCode']==0 and len(matching)==1 and matching[0]['exitCode']==0 and matching[0]['groupClosed']is True,'five actual commands0 and groups joined')
  run=hold(P/'RUN.stdout.log');need(len(run.splitlines())==1 and hold(P/'RUN.stderr.log')==b'','one actual C++ record/empty stderr');body_record=decode(run);need(same(body_record,worker['record']),'actual stdout equals reported C++ record')
  fixed={'result':'PASS','CoreAuthentications':7,'positiveAuthentications':4,'expectedCoreRefusals':3,'readonlyDatabaseOpens':8,'newTestOnlySignaturesCreated':1,'physicalPairs':2,'databases':4,'newSignaturesInCheck':0,'productionQualified':False}
  need(same({k:body_record[k]for k in fixed},fixed)and len(body_record['productionAdmissions'])==2 and sum(v['entries']for v in body_record['productionAdmissions'])==body_record['storeEntries']and sum(v['payloadBytes']for v in body_record['productionAdmissions'])==body_record['storeBytes'],'local C++ oracle and production admission joins')
  inventory=decode(hold(INVENTORY,known=INVENTORY_PIN));snapshots=[decode(hold(O/(phase+'-public.json')))for phase in('fixture-before-RUN','fixture-after-RUN','fixture-after-cleanup')];base=snapshots[0]
  need(len(base['rows'])==69 and set(base['rows'])==set(inventory['entries'])and same(base['portableRoleInventories'],inventory['portableRoleInventories'])and all(same(v['rows'],base['rows'])and same(v['portableRoleInventories'],base['portableRoleInventories'])and v['historicalGuestFD9Qualified']is False for v in snapshots),'three actual passive69/portable8 snapshots equal, not historical guest FD9')
  for role in('main','audit'):
   actual=decode(hold(O/(role+'-created.stdout.log')));need(type(actual)is list and len(actual)==1,'one actual created container inspection');x=actual[0];cid=x['Id'];nonce=host['runRoot'].removeprefix('/state/journal-rollback-');need(re.fullmatch('[0-9a-f]{32}',nonce)and re.fullmatch('[0-9a-f]{64}',cid)and x['Image']==IMAGE and x['Name']=='/nano-journal-'+role+'-'+nonce and x['Config']['Labels'].get('nano.journal-rollback.owner')==nonce and x['Config']['Labels'].get('nano.journal-rollback.owner.role')==role,'actual owned role CID joined to terminal run');rc,_,stderr=command(role+'-absence',prefix+['container','inspect',cid]);need(rc==1 and(b'no such object'in stderr.lower()or b'no such container'in stderr.lower()),'actual role CID independently absent');absent.append(role)
  fence=decode(hold(O/'host-final-fence-public.json'));need(fence['passed']is True and fence['afterCleanupAndImageInspection']is True and fence['guest69InRunUnchanged']is True and fence['historicalAllSourceFD9Qualified']is False,'terminal final fence exact scopes')
  for phase in('source-before','source-after-cleanup'):
   observation=decode(hold(O/(phase+'-public.json')));need(observation['passed']is True and observation['files']==1047 and observation['namesSizeShaQualified']is True and observation['historicalAllSourceFD9Qualified']is False and observation['sourceRowsReadopted']is False,'global source content scope, not historical FD9')
  loader=decode(hold(P/'loader-closure-public.json'));need(set(loader)=={'paths','canonicalPaths'}and type(loader['paths'])is list and loader['paths']and set(loader['canonicalPaths'])==set(loader['paths'])and all(loader['canonicalPaths'][v]==aliases[v]and aliases[v]in authority['guestStatSha']for v in loader['paths']),'per-run loader closure evidence joined, not replayed')
  prep=decode(hold(E/'real-journal-rollback-arm-control-root-20261018-03/preparation-result-public.json'));need(prep['passed']is True and prep['createdNotAdmitted']is True and prep['recipeByteEqualOld']is True and prep['inputAuthoritiesRefreshed']is False,'preparation retained exact old recipe and no admission claim')
  total=0
  for path,leaf,maximum in PUBLIC:
   raw=hold(path,maximum=maximum);total+=len(raw);need(total<=4194304 and '.private'not in path.name,'closed public aggregate/no private carrier');documents[leaf]=pin(raw)
   if leaf.endswith('.json'):decode(raw)
   copied.append({'file':leaf,'pin':write_new(A/leaf,raw)})
  raw=(json.dumps({'format':'nano-arm03-source-pins-v1','files':source_pins},sort_keys=True,indent=2)+'\n').encode();copied.append({'file':'source-pins-public.json','pin':write_new(A/'source-pins-public.json',raw)})
 except BaseException as exc:fail(exc,'passive-audit')
 finally:
  if software_ready:
   try:image_final=image('image-final')
   except BaseException as exc:fail(exc,'image-final')
  failures=[]
  for path,row in held.items():
   try:need(material(path)[0]==row,'held original/audit input changed')
   except BaseException as exc:failures.append(pathlib.Path(path).name);fail(exc,'input-final-fence')
  report={'format':'nano-journal-arm03-passive-root-audit-v1','passed':first is None,'externalPins':external,'oldOriginalSoftwareRows':9 if software_ready else 0,'consumedOriginalSourceRows':len(consumed),'originalBuildProducts':4 if elf is not None else 0,'ELF':elf,'MMD':mmd,'actualCppCounts':None if body_record is None else{k:body_record[k]for k in('CoreAuthentications','positiveAuthentications','expectedCoreRefusals','readonlyDatabaseOpens','newTestOnlySignaturesCreated')},'ownedRoleCidsAbsent':absent,'imageFinal':image_final,'historicalAllSourceFD9Qualified':False,'historicalGuestFD9Qualified':False,'historicalAuthorityJsonFD9Claimed':False,'historicalLoaderSdkImageFD9Qualified':False,'loaderAndMmdEvidenceReplayed':False,'publicFiles':copied,'publicAllowlistEntries':18,'firstFailure':first,'secondaryFailures':secondary,'inputFinalFenceFailures':failures,'inspectEvents':events,'DbOpened':False,'CppInvoked':False,'signaturesCreated':0,'CoreRebuilt':False,'productionQualified':False}
  if out_created:
   try:write_new(A/'result-public.json',(json.dumps(report,sort_keys=True,indent=2,allow_nan=False)+'\n').encode())
   except BaseException as exc:fail(exc,'result-publication');report.update(passed=False,firstFailure=first,secondaryFailures=secondary)
 print(json.dumps(report,sort_keys=True,allow_nan=False));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
