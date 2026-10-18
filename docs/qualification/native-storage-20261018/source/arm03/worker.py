"""SOURCE_ONLY: host-Python ARM transport; one rollback TU, retained Linux Core."""
import hashlib,json,os,pathlib,posixpath,re,shlex,stat,time
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
OBSERVE = 'set -eu\nexport LC_ALL=C\nfor p do\n r=$(/usr/bin/realpath -e -- "$p"); [ -f "$r" ] && [ ! -L "$r" ]\n printf \'%s\\0\' "$r"\n /usr/bin/stat --printf=\'%d\\0%i\\0%f\\0%h\\0%u\\0%g\\0%s\\0%y\\0%z\\0\' -- "$r"\n h=$(/usr/bin/sha256sum < "$r"); printf \'%s\\0\' "$h"\n /usr/bin/stat --printf=\'%d\\0%i\\0%f\\0%h\\0%u\\0%g\\0%s\\0%y\\0%z\\0\' -- "$r"\n [ "$r" = "$(/usr/bin/realpath -e -- "$p")" ]\ndone\n'

def run_qualification(c):
 def need(ok,why):
  if not ok:raise ValueError(why)
 fields={'hostOutput','dockerPrefix','projectPins','knownGuestPins','compilerFlags','linkPrefix','linkTail','guestUid','guestGid','timeoutSeconds','execute','verifyConsumed','beforeRun','runRoot','typedOwners'}
 need(type(c)is dict and set(c)==fields,'exact ARM rollback worker configuration')
 out=pathlib.Path(c['hostOutput']);need(out.is_absolute()and out.resolve(strict=True)==out and out.is_dir(),'fresh physical worker output')
 need(type(c['timeoutSeconds'])is int and 300<=c['timeoutSeconds']<=600,'bounded worker budget');deadline=time.monotonic()+c['timeoutSeconds']
 prefix=c['dockerPrefix'];need(type(prefix)is list and len(prefix)>=3 and prefix[-2]=='exec'and re.fullmatch('[0-9a-f]{64}',prefix[-1]),'Root owned Docker exec prefix')
 need(all(callable(c[k])for k in('execute','verifyConsumed','beforeRun')),'Root command/publication capabilities')
 project=dict(c['projectPins']);known=dict(c['knownGuestPins']);guest={};aliases={};products={};steps=[];serial=0;first=None;secondary=[];mmd=None;owner_preparations=[];typed_owners=c['typedOwners']
 need(type(typed_owners)is dict and set(typed_owners)==set(project)|set(known)and all(type(p)is str and p.startswith(('/work/','/case/','/old/','/retained/'))and type(v)is dict and set(v)=={'uid','gid'}and all(type(v[k])is int and 0<=v[k]<=65535 for k in('uid','gid'))for p,v in typed_owners.items()),'closed typed owners derived by host from original/created rows')
 counts={k:0 for k in('independentM','cppCompilations','links','loaderChecks','runs','CoreM','CoreCpp','archiveUpdates')};record=None
 def error(exc,stage):
  nonlocal first
  row={'stage':stage,'type':type(exc).__name__,'message':str(exc)}
  if first is None:first=row
  else:secondary.append(row)
 def save(name,value):
  with(out/name).open('x')as f:json.dump(value,f,sort_keys=True,indent=2,allow_nan=False);f.write('\n');f.flush();os.fsync(f.fileno())
 def raw_save(name,b):
  with(out/name).open('xb')as f:f.write(b);f.flush();os.fsync(f.fileno())
 def pinner(row):return {'sha256':row['sha256'],'bytes':row.get('bytes',row.get('st_size'))}
 def material(p,body=False,empty=False):
  need(p.is_absolute()and p.resolve(strict=True)==p,'physical worker file');fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
  try:
   a=os.fstat(fd);need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and (0<=a.st_size if empty else 0<a.st_size)and a.st_size<=134217728,'bounded single-link worker file');row={k:getattr(a,k)for k in F};h=hashlib.sha256();parts=[];size=0
   while b:=os.read(fd,65536):
    size+=len(b);need(size<=a.st_size,'file grew');h.update(b)
    if body:parts.append(b)
   need(size==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'worker full9/path changed');return dict(row,sha256=h.hexdigest()),b''.join(parts)if body else None
  finally:os.close(fd)
 def run(label,args,timeout=180,closing=False):
  limit=min(timeout,deadline+(60 if closing else 0)-time.monotonic());need(limit>0,'physical worker deadline')
  argv=prefix[:-1]+['--workdir','/outputs','--env','HOME=/tmp','--env','TMPDIR=/tmp','--env','LC_ALL=C','--env','LANG=C','--env','TZ=UTC',prefix[-1],*args]
  began=time.monotonic();rc,stdout,stderr=c['execute']('worker-'+label,argv,limit,closing)
  need(len(stdout)<=16777216 and len(stderr)<=16777216,'worker capture bound');raw_save(label+'.stdout.log',stdout);raw_save(label+'.stderr.log',stderr)
  event={'phase':label,'actualExitCode':rc,'elapsedMs':round((time.monotonic()-began)*1000)};steps.append(event);save(label+'.exit-public.json',event);need(rc==0,'physical command failed: '+label);return stdout,stderr
 def observe(paths,closing=False):
  nonlocal serial
  paths=sorted(set(paths));need(len(paths)<=20000,'guest observation count');rows={};resolved={}
  for at in range(0,len(paths),128):
   batch=paths[at:at+128];need(all(p.startswith('/')and not any(x in p for x in('\0','\n','\r'))for p in batch),'absolute guest paths');serial+=1
   raw,_=run('observe-'+str(serial),['/bin/sh','-c',OBSERVE,'selected-inputs',*batch],90,closing);need(len(raw)<=1048576 and raw.endswith(b'\0'),'guest observation bound');parts=raw[:-1].decode().split('\0');need(len(parts)==20*len(batch),'guest observation field count')
   for i,literal in enumerate(batch):
    values=parts[i*20:(i+1)*20];physical,a,h,b=values[0],values[1:10],values[10],values[11:20]
    need(physical.startswith('/')and a==b and re.fullmatch('[0-9a-f]{64}  -',h),'guest stat/hash/stat coherence');need(all(re.fullmatch('[0-9]+',a[j])for j in(0,1,3,4,5,6))and re.fullmatch('[0-9a-f]+',a[2])and stat.S_ISREG(int(a[2],16))and 0<=int(a[6])<=134217728,'guest regular GNU9')
    row={'materialGNU9':a,'sha256':h[:64],'bytes':int(a[6])};need(physical not in rows or rows[physical]==row,'repeated guest identity changed');rows[physical]=row;resolved[literal]=physical
  return rows,resolved
 def prepare_known_owners(paths,label):
  selected=sorted(set(paths));new_known=[p for p in selected if p in typed_owners and(p not in aliases or aliases[p]not in guest)]
  if not new_known:return
  start=time.monotonic();fixed=None;probes=0;previous_owner=None;owner_changes=0
  while True:
   rows,links=observe(selected);checks={};owners_ready=True
   for literal,physical in links.items():
    row=rows[physical];expected=project.get(literal)or known.get(literal);actual=pinner(row);canonical=not literal.startswith(('/work/','/case/','/old/','/retained/'))or expected is not None and literal==physical
    want=typed_owners.get(literal);observed={'uid':int(row['materialGNU9'][4]),'gid':int(row['materialGNU9'][5])};matched=want is None or observed==want;owners_ready=owners_ready and matched
    checks[literal]={'expectedPin':expected,'actualPin':actual,'canonicalProjectProviderPath':canonical,'knownBytesMatch':expected is None or actual==expected,'expectedOwner':want,'observedOwner':observed,'expectedOwnerReached':matched}
   probes+=1;save(label+'-unadmitted-owner-preparation-'+str(probes)+'.private.json',{'rows':rows,'aliases':links,'checks':checks,'unadmitted':True,'guestAuthorityCreated':False,'elapsedMs':round((time.monotonic()-start)*1000)})
   need(all(v['canonicalProjectProviderPath']and v['knownBytesMatch']for v in checks.values()),'unadmitted owner preparation known bytes/aliases')
   for literal,physical in links.items():
    need(literal not in aliases or aliases[literal]==physical,'held alias changed; no owner wait/reset');need(physical not in guest or guest[physical]==rows[physical],'held guest row changed; no owner wait/reset')
   # Only UID/GID can change during this preparation. No observation becomes a row authority.
   identity=({p:(r['materialGNU9'][:4]+r['materialGNU9'][6:],r['sha256'],r['bytes'])for p,r in rows.items()},links)
   if fixed is None:fixed=identity
   need(identity==fixed,'unadmitted dev/ino/mode/nlink/size/mtime/ctime/SHA/aliases fixed')
   observed_owners={p:(r['materialGNU9'][4],r['materialGNU9'][5])for p,r in rows.items()}
   if previous_owner is not None:owner_changes+=sum(previous_owner[p]!=v for p,v in observed_owners.items())
   previous_owner=observed_owners;now=time.monotonic();need(now<min(deadline,start+30)and probes<=30,'bounded unadmitted owner preparation including durable samples')
   if owners_ready:
    owner_preparations.append({'phase':label,'probes':probes,'expectedOwnerPaths':len(new_known),'observedOwnerChanges':owner_changes,'elapsedMs':round((now-start)*1000),'admitted':False});return
   time.sleep(1.05)
 def admit_inputs(paths,label):
  prepare_known_owners(paths,label)
  start=time.monotonic();previous=None;streak=0;since=0;byte_pins={};n=0;fixed=None
  while True:
   rows,links=observe(paths);n+=1;save(label+'-sample-'+str(n)+'.private.json',{'rows':rows,'aliases':links,'elapsedMs':round((time.monotonic()-start)*1000)})
   for literal,physical in links.items():
    row=rows[physical];need(literal not in aliases or aliases[literal]==physical,'old guest alias changed');need(physical not in guest or guest[physical]==row,'old guest row changed')
    expected=project.get(literal)or known.get(literal)
    if literal in typed_owners:need({'uid':int(row['materialGNU9'][4]),'gid':int(row['materialGNU9'][5])}==typed_owners[literal],'original host-derived owner required before first eight-field anchor')
    if literal.startswith(('/work/','/case/')):need(literal in project and literal==physical,'project dependency exact path')
    if physical.startswith(('/work/','/case/')):need(physical in project and pinner(row)==project[physical],'resolved project pin')
    if literal.startswith(('/old/','/retained/')):need(literal in known and literal==physical,'known old provider only')
    need(expected is None or pinner(row)==expected,'known dependency bytes changed');need(physical not in byte_pins or byte_pins[physical]==pinner(row),'prospective SDK bytes changed');byte_pins.setdefault(physical,pinner(row))
   identity=({p:(r['materialGNU9'][:8],r['sha256'],r['bytes'])for p,r in rows.items()},links)
   if fixed is None:fixed=identity
   need(identity==fixed,'new guest eight fields/bytes/aliases unchanged');now=time.monotonic();need(now<min(deadline,start+30),'guest first-admission deadline')
   if(rows,links)==previous:streak+=1
   else:streak=1;since=now
   if streak>=3 and now-since>=2:break
   previous=(rows,links);time.sleep(1.05)
  for p,r in rows.items():guest.setdefault(p,r)
  for p,r in links.items():aliases.setdefault(p,r)
 def admit_products(names,label,executable=False):
  need(all(str(out/name)not in products for name in names),'fresh products only');paths=['/outputs/'+n for n in names];initial,_=observe(paths);pins={p:pinner(initial[p])for p in paths}
  run(label+'-mode',['/bin/chmod','500'if executable else'400',*paths]);start=time.monotonic();previous=None;fixed=None;streak=0;since=0;n=0
  while True:
   rows,links=observe(paths);table={}
   for p in paths:
    host,_=material(out/p.removeprefix('/outputs/'));need(links[p]==p and pinner(rows[p])==pins[p]==pinner(host),'new product bytes known and stable');need(rows[p]['materialGNU9'][3]=='1'and int(rows[p]['materialGNU9'][4])==c['guestUid']and int(rows[p]['materialGNU9'][5])==c['guestGid'],'new product owner');table[p]={'hostFD9':host,'guestStatSha':rows[p]}
   n+=1;save(label+'-sample-'+str(n)+'.private.json',{'products':table,'elapsedMs':round((time.monotonic()-start)*1000)});identity={p:{k:v for k,v in r['hostFD9'].items()if k!='st_ctime_ns'}for p,r in table.items()}
   if fixed is None:fixed=identity
   need(identity==fixed,'new product eight-field identity changed');now=time.monotonic();need(now<min(deadline,start+30),'new product admission deadline')
   if table==previous:streak+=1
   else:streak=1;since=now
   if streak>=3 and now-since>=2:break
   previous=table;time.sleep(1.05)
  for p,r in table.items():products[str(out/p.removeprefix('/outputs/'))]=r
 def deps(name):
  row,body=material(out/name,True);need(row==products[str(out/name)]['hostFD9']and len(body)<=4194304,'retained dependency file');target,sep,text=body.decode().replace('\\\n',' ').partition(':');need(sep and shlex.split(target)==['/outputs/journal.o'],'one exact dependency MT')
  values=sorted(set(posixpath.normpath(posixpath.join('/outputs',x))for x in shlex.split(text)));need(values and all(x.startswith('/')for x in values),'normalized dependency paths');return values
 def json_body(raw):
  def unique(pairs):
   d={}
   for k,v in pairs:need(k not in d,'duplicate JSON key');d[k]=v
   return d
  def invalid(_):raise ValueError('nonfinite JSON')
  return json.loads(raw,object_pairs_hook=unique,parse_constant=invalid)
 def same(a,b):return json.dumps(a,sort_keys=True,allow_nan=False)==json.dumps(b,sort_keys=True,allow_nan=False)
 def parse_record(raw):
  need(len(raw)<=65536 and len(raw.splitlines())==1,'exact one C++ record');r=json_body(raw);fixed={'scenario':'beacon-journal-real-physical-rollback','result':'PASS','physicalPairs':2,'databases':4,'independentReadonlyPasses':2,'readonlyDatabaseOpens':8,'CoreAuthentications':7,'positiveAuthentications':4,'oldPairRecentPrimaryFloorRefused':True,'oldPairRecentWitnessFloorRefused':True,'oldPairBothRecentFloorsRefused':True,'expectedCoreRefusals':3,'newTestOnlySignaturesCreated':1,'signingProviderCalls':1,'signerConstructions':2,'newSignaturesInCheck':0,'signersConstructedInCheck':0,'fourClosedStoresFD9ShaUnchanged':True,'completeIndependentPhysicalCopy':True,'externalCommitmentOutsideBothTrees':True,'externalCommitmentFD9ShaUnchanged':True,'maximumStoreBytes':33554432,'maximumEntries':4096,'maximumRssKiB':131072,'maximumWallSeconds':180,'independentExternalRollbackProtectionQualified':False,'historicalDigestAncestryVerified':False,'deliveredSignatureHistoryVerified':False,'automaticRecoveryUsed':False,'operatorRecoveryQualified':False,'privateInputsRead':False,'runtimeDutiesAuthorized':False,'networkTransport':False,'commonFinalityQualified':False,'productionQualified':False};extra={'productionAdmissions','externalCommitmentSha256','externalCommitmentBytes','storeBytes','storeEntries','actualPeakRssKiB','elapsedMs'};need(type(r)is dict and set(r)==set(fixed)|extra and same({k:r[k]for k in fixed},fixed),'typed exact C++ counters/scope')
  need(type(r['externalCommitmentSha256'])is str and re.fullmatch('[0-9a-f]{64}',r['externalCommitmentSha256']),'commitment SHA')
  for k,lo,hi in[('externalCommitmentBytes',1,1048576),('storeBytes',1,33554432),('storeEntries',1,4096),('actualPeakRssKiB',1,131072),('elapsedMs',0,180000)]:need(type(r[k])is int and lo<=r[k]<=hi,'bounded actual C++ '+k)
  need(type(r['productionAdmissions'])is list and len(r['productionAdmissions'])==2,'two exact production admissions')
  admission_keys={'label','entries','payloadBytes','samples','stableSamples','elapsedMs','ctimeOnlyChanges','durabilityBeforeSamples','creatorEightAndShaUnchanged','exactLastCaptureAdopted','postAdoptionStrictFence'}
  for admission,label in zip(r['productionAdmissions'],('initial','post-sign')):
   need(type(admission)is dict and set(admission)==admission_keys and admission['label']==label,'closed admission schema/order')
   for key,lo,hi in(('entries',1,4096),('payloadBytes',0,33554432),('samples',3,30),('stableSamples',3,30),('elapsedMs',2000,29999),('ctimeOnlyChanges',0,122880)):
    need(type(admission[key])is int and lo<=admission[key]<=hi,'bounded production admission '+key)
   need(admission['stableSamples']<=admission['samples']and all(admission[k]is True for k in('durabilityBeforeSamples','creatorEightAndShaUnchanged','exactLastCaptureAdopted','postAdoptionStrictFence')),'typed production admission evidence')
  need(sum(v['entries']for v in r['productionAdmissions'])==r['storeEntries']and sum(v['payloadBytes']for v in r['productionAdmissions'])==r['storeBytes'],'production admissions sum to final stores')
  return r
 try:
  for key in('compilerFlags','linkPrefix','linkTail'):need(type(c[key])is list and c[key]and all(type(x)is str and x and '\0'not in x for x in c[key]),'explicit recipe')
  flags=c['compilerFlags'];need(flags[0]=='/usr/bin/clang++'and not any(x in flags for x in('-M','-MM','-MD','-MMD','-MF','-MT','-o','-c')),'one complete fresh unit recipe')
  source='/case/tests/qualification/beacon_journal_physical_rollback.cpp';obj='/outputs/journal.o';runroot=c['runRoot'];need(re.fullmatch('/state/journal-rollback-[0-9a-f]{32}',runroot),'fresh sibling-only run root')
  tools=[flags[0],c['linkPrefix'][0],'/usr/bin/ldd','/bin/sh','/usr/bin/realpath','/usr/bin/stat','/usr/bin/sha256sum','/usr/bin/find','/bin/chmod']
  initial_paths=[*tools,*known,source,'/case/tools/storage-maintenance/bootstrap-readonly-replica.hpp'];admit_inputs(initial_paths,'initial')
  need(not any((out/n).exists()for n in('journal.o.M','journal.o.d','journal.o','rollback')),'fresh outputs')
  counts['independentM']+=1;run('M',flags+['-M','-MF',obj+'.M','-MT',obj,source]);admit_products(['journal.o.M'],'M');names=deps('journal.o.M');need(source in names,'declared TU reached');c['verifyConsumed'](names)
  for at in range(0,len(names),512):admit_inputs(names[at:at+512],'M-input-'+str(at))
  counts['cppCompilations']+=1;run('CXX',flags+['-MD','-MF',obj+'.d','-MT',obj,'-o',obj,'-c',source],300);admit_products(['journal.o','journal.o.d'],'object');actual=deps('journal.o.d');c['verifyConsumed'](actual);need(actual==names,'literal M equals MD')
  for at in range(0,len(actual),512):admit_inputs(actual[at:at+512],'MD-input-'+str(at))
  need(sorted({aliases[p]for p in names})==sorted({aliases[p]for p in actual}),'normalized M equals MD');mmd={'normalizedMEqualsMd':True,'literalMEqualsMd':True,'dependencyCount':len({aliases[p]for p in names})}
  need(c['linkTail'].count('/retained/libnano_core.a')==1,'one retained Core link reference');admit_inputs([p for p in c['linkPrefix']+c['linkTail']if p.startswith('/')],'absolute-link-recipe')
  counts['links']+=1;run('LINK',c['linkPrefix']+[obj,'-o','/outputs/rollback']+c['linkTail']);admit_products(['rollback'],'executable',True)
  counts['loaderChecks']+=1;raw,_=run('LDD',['/usr/bin/ldd','/outputs/rollback'],30);need(b'not found'not in raw,'all loader dependencies resolved');closure=[]
  for line in raw.decode().splitlines():
   value=line.strip()
   if re.fullmatch(r'linux-vdso\.so\.[0-9]+ \(0x[0-9a-f]+\)',value):continue
   match=re.fullmatch(r'(?:[^ \t]+ => )?(/[^ \t]+) \(0x[0-9a-f]+\)',value);need(match is not None,'recognized absolute ldd provider');closure.append(match[1])
  need(closure,'nonempty loader closure');admit_inputs(closure,'loader-inputs');save('loader-closure-public.json',{'paths':closure,'canonicalPaths':{p:aliases[p]for p in closure}})
  c['beforeRun']();counts['runs']+=1;raw,err=run('RUN',['/outputs/rollback',runroot],180);need(err==b'','C++ stderr empty');record=parse_record(raw)
 except BaseException as exc:error(exc,'qualification')
 finally:
  if aliases:
   try:rows,links=observe(list(aliases),True);need(rows==guest and links==aliases,'all selected guest inputs/aliases unchanged')
   except BaseException as exc:error(exc,'guest-final-fence')
  for path,want in products.items():
   try:
    host,_=material(pathlib.Path(path));g='/outputs/'+pathlib.Path(path).name;rows,links=observe([g],True);need(host==want['hostFD9']and links[g]==g and rows[g]==want['guestStatSha'],'new product final host/guest fence')
   except BaseException as exc:error(exc,'product-final-fence:'+path)
  authority={'products':products,'guestStatSha':guest,'guestAliases':aliases,'guestFdIdentityVerified':False}
  try:save('final-observations.private.json',authority)
  except BaseException as exc:error(exc,'observation-publication')
  report={'format':'nano-real-journal-rollback-arm-worker-v1','passed':first is None and record is not None,'counts':counts,'mmd':mmd,'record':record,'freshBuildProducts':len(products),'ownerPreparationSamples':owner_preparations,'typedOwnerCount':len(typed_owners),'firstFailure':first,'secondaryFailures':secondary,'steps':steps,'CppExitSource':'owned docker exec actual exit and closed host process group','keepaliveStateExitCodeIsCppExitCode':False,'CoreRebuilt':False,'runtimeDutiesAuthorized':False,'productionQualified':False}
  try:save('result-public.json',report)
  except BaseException as exc:error(exc,'result-publication');report['passed']=False;report['firstFailure']=first
 return report,authority
