"""SOURCE_ONLY: strictly import the committed Native04 public tree, then one RO AMD check.
Root supplies original transported FD9 rows. No produce, build, signing or repair.
"""
import hashlib, io, json, os, pathlib, re, signal, stat, subprocess, sys, tarfile, time, types, uuid
V = pathlib.Path('/home/ubuntu')
IN = V/'nano-physical-native-import-public-root-20261018-01'
OUT = V/'physical-native-import-amd-root-20261018-01'
BASE = V/'physical-cohort-amd-root-20261018-02'
ELF = BASE/'outputs/physical-cohort-portability'
AUDIT = BASE/'root-audit-public.json'
OLD_CONTROL = V/'physical-cohort-amd-control-root-20261018-02/control.private.json'
ELF_PIN = {'sha256':'824bd6a9ac7ce93fbfecbeeca23fcc71322d8e4f4979d16b96e5c815ab4a753a','bytes':6408336}
AUDIT_PIN = {'sha256':'61763fd153906581389d0123c606bedad41efbf0f3fd573171f3eea2e5d877ca','bytes':3392}
CONTROL_PIN = {'sha256':'689c5951523ed44f281023b163d0c0654006f49d188b09e458066084f3ca05ba','bytes':185358}
COMMON_PIN = {'sha256':'09422e11287ce5f0357e3b3e9e5219d6c492ac2a3d3815aa6bbb6aa2ade7943c','bytes':10200}
IMAGE = 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
PREFIX = ['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config','/home/ubuntu/capella-sync-adverse-amd-docker-client-root-20261018-01']
PINS = {
 'archive':('cohort.tar',{'sha256':'7c6f556ab3514bc2115db8fb095dc9b1f5feaf1f0beaad955b81e286aebebf96','bytes':5283840}),
 'manifest':('cohort-manifest-public.json',{'sha256':'b8dfb4a9b2144c037f9f85a2137d0c4047bd0f344a739290b22c9242369246e6','bytes':12445}),
 'inventory':('export-inventory-public.json',{'sha256':'c931751ec594069eca6ce70452c8670bb3ca07bcd4e31fade85e402ddea9a5bf','bytes':18838}),
 'commitment':('export-external-commitment-public.json',{'sha256':'3144122f7872df305c47ee538a3db7f6a3d5f7c21274b163c07ab7d80137a203','bytes':1157}),
 'exportResult':('result-public.json',{'sha256':'7d55ffa8313961489970a02a27b41a4ebefa7a6a1774c1ba4bc0d7ee98385496','bytes':6406})}
MF_PIN = PINS['manifest'][1]
PROOF_PINS = {
 'producerTerminal':{'sha256':'dd6e7880efb4a8a4a3649f1c905e7e2c7ad5033db8b568fcefed76a05cd88ec8','bytes':3307},
 'checkerTerminal':{'sha256':'c71f5d186272c3d1de8df97348bc017a07cd912c58c991ec8a041b3808b0774f','bytes':3300},
 'nativeHostResult':{'sha256':'a6117e885646e809be061ba98fbcc08fea3ff1728b128866a4056f239d0b1cdc','bytes':30408}}
F = ('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
LIMITS = {'maximumBytes':134217728,'maximumEntries':4096,'importSeconds':180,'checkSeconds':180,'operationSeconds':420,'newCopyAdmissionSeconds':30,'logBytes':1048576}
def need(ok, why):
 if not ok: raise ValueError(why)
def pin(raw): return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def encode(value): return (json.dumps(value,sort_keys=True,indent=2,allow_nan=False)+'\n').encode()
def decoded(raw):
 def pairs(values):
  result={}
  for k,v in values: need(k not in result,'duplicate JSON key'); result[k]=v
  return result
 return json.loads(raw,object_pairs_hook=pairs,parse_constant=lambda _:(_ for _ in()).throw(ValueError('nonfinite JSON')))
def rowpin(row): return {'sha256':row['sha256'],'bytes':row['st_size']}
def rowshape(row): return type(row)is dict and set(row)==set(F)|{'sha256'} and all(type(row[k])is int and row[k]>=0 for k in F) and type(row['sha256'])is str and re.fullmatch('[0-9a-f]{64}',row['sha256'])
def material(path):
 p=pathlib.Path(path); need(p.is_absolute() and p.resolve(strict=True)==p,'canonical explicit file')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK|os.O_CLOEXEC)
 try:
  a=os.fstat(fd); row={k:getattr(a,k)for k in F}; need(stat.S_ISREG(a.st_mode) and a.st_nlink==1 and 0<=a.st_size<=134217728,'bounded single-link file')
  chunks=[]; count=0
  while chunk:=os.read(fd,65536): count+=len(chunk); need(count<=a.st_size,'file grew'); chunks.append(chunk)
  raw=b''.join(chunks); need(len(raw)==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'strict FD9/path during read')
  return dict(row,sha256=pin(raw)['sha256']),raw
 finally: os.close(fd)
def syncdir(path):
 fd=os.open(path,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 try: os.fsync(fd)
 finally: os.close(fd)
def exact(row,fixed,variable=()):
 need(type(row)is dict and set(row)==set(fixed)|set(variable),'closed typed JSON record')
 for k,v in fixed.items(): need(type(row[k])is type(v) and row[k]==v,'exact typed field '+k)
def integer(value,lo,hi): need(type(value)is int and lo<=value<=hi,'bounded exact integer')
def token(row):
 exact(row,{},('sequence','digest')); need(type(row['sequence'])is str and re.fullmatch(r'0x[1-9a-f][0-9a-f]*',row['sequence']) and int(row['sequence'],16)<=2**64-1,'canonical positive sequence'); need(type(row['digest'])is str and re.fullmatch(r'0x[0-9a-f]{64}',row['digest']),'canonical digest')
def cohort_schema(rows,manifest):
 need(type(rows)is list and len(rows)==3,'exact three C++ records')
 for row,name,archives in zip(rows[:2],('alpha','beta'),(2,3)):
  fixed={'scenario':'physical-authenticated-cohort','mode':'check','result':'PASS','name':name,'archivesAuthenticated':archives,'journalProposalBoundToArchivedBlock':True,'pendingBlsChanges':1,'coldClosureAuthenticated':True,'signedAnchorAuthenticatedOnBothStores':True,'executionCoupled':True,'allFourStoresReadOnly':True,'closedPhysicalInventoryUnchanged':True,'independentReadonlyPasses':2}
  exact(row,fixed,('primaryToken','witnessToken','headRoot')); token(row['primaryToken']); token(row['witnessToken']); need(type(row['headRoot'])is str and re.fullmatch(r'0x[0-9a-f]{64}',row['headRoot']),'canonical head root')
  committed=manifest['cohorts'][0 if name=='alpha'else 1]; need(committed['name']==name and row['primaryToken']==committed['primary']['token']and row['witnessToken']==committed['witness']['token']and row['headRoot']==committed['headRoot'],'Core observations equal independently committed original manifest')
 fixed={'scenario':'physical-authenticated-cohort-summary','mode':'check','result':'PASS','cohorts':2,'databases':8,'archivesAuthenticated':5,'independentReadonlyPasses':2,'newTestOnlySignaturesCreated':0,'newSignaturesInCheck':0,'maximumRssKiB':131072,'maximumWallSeconds':180,'noMissingStoreCreation':True,'automaticRecoveryUsed':False,'runtimeDutiesAuthorized':False,'networkTransport':False,'productionQualified':False,'finalityAdvanceQualified':False,'manifestSha256':MF_PIN['sha256']}
 exact(rows[2],fixed,('actualPeakRssKiB',)); integer(rows[2]['actualPeakRssKiB'],1,131072)
def portable(rows,manifest):
 children={n:[]for n,r in rows.items()if r['kind']=='directory'}
 for n in rows:
  if n: need(n.rpartition('/')[0]in children,'explicit closed parent'); children[n.rpartition('/')[0]].append(n)
 for values in children.values(): values.sort(key=lambda n:n.rsplit('/',1)[-1])
 need(set(children[''])=={'alpha','beta','profile.json','manifest.json'},'exact four top-level names')
 result={}
 for cohort in manifest['cohorts']:
  for role in ('primary','witness','signer','signer-witness'):
   root=cohort['name']+'/'+role; need(root in children,'complete role'); values=[['','directory']]; size=0
   def visit(parent,relative,depth):
    nonlocal size
    need(depth<=128,'portable depth')
    for child in children[parent]:
     rel=(relative+'/'if relative else'')+child.rsplit('/',1)[-1]; r=rows[child]
     if r['kind']=='directory': values.append([rel,'directory']); visit(child,rel,depth+1)
     else: size+=r['st_size']; values.append([rel,'file',r['st_size'],r['sha256']])
   visit(root,'',0); raw=json.dumps(values,ensure_ascii=False,separators=(',',':'),allow_nan=False).encode(); description={'sha256':pin(raw)['sha256'],'entries':len(values),'bytes':size}; need(description==cohort['physical'][role],'original portable role digest'); result[root]=description
 return result

def main():
 need(len(sys.argv)==6,'usage: runner.py CONTROL SHA BYTES SELF_SHA SELF_BYTES'); os.umask(0o077); sys.dont_write_bytecode=True
 need(sys.platform.startswith('linux') and os.geteuid()==os.getegid()==1000,'Linux host UID:GID1000')
 began=time.monotonic(); work_end=began+360; end=began+420; import_end=began+180; held={}; events=[]; first=None; secondary=[]; admitted=None; docker_ready=False; final_image=None; rows=None; handlers={}; owned=False; directory_fds={}; creation={}; copied_admission=None
 container={'name':'nano-physical-native-import-'+uuid.uuid4().hex,'label':'nano.physical-native-import.owner','cid':None,'attempted':False,'cleanup':None}
 def fail(exc,phase):
  nonlocal first
  item={'phase':phase,'type':type(exc).__name__,'message':str(exc)[:2000]}
  if first is None: first=item
  else: secondary.append(item)
 def deadline(closing=False): need(time.monotonic()<(end if closing else work_end),'bounded controller deadline')
 def hold(path,want):
  need(rowshape(want),'full original FD9+SHA'); got,raw=material(path); need(got==want,'original FD9+SHA changed: '+str(path)); need(str(path)not in held or held[str(path)]==want,'authority refresh forbidden'); held.setdefault(str(path),dict(want)); return raw
 def known(path,want):
  row,raw=material(path); need(rowpin(row)==want,'known independently pinned bytes'); return hold(path,row)
 def save(name,value):
  raw=encode(value); need(len(raw)<=4*1024*1024,'bounded output record'); fd=os.open(OUT/name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o400)
  with os.fdopen(fd,'wb')as stream: need(stream.write(raw)==len(raw),'complete exclusive output'); stream.flush(); os.fsync(stream.fileno())
  syncdir(OUT); return pin(raw)
 def old_fence():
  for p,r in held.items(): need(material(p)[0]==r,'held input original FD9+SHA unchanged')
 def closed(closing=False):
  deadline(closing); result=cc.snapshot(OUT/'public-cohort',(1000,1000)); need(len(result)==69 and set(result)==set(expected),'exact69 imported entries'); seen=set(); originals={(r['st_dev'],r['st_ino'])for r in held.values()}
  for name,r in result.items():
   want=expected[name]; need(r['kind']==want['type'],'exact imported entry type'); ident=(r['st_dev'],r['st_ino']); need(ident not in seen and ident not in originals,'new unique inode distinct from inputs'); seen.add(ident)
   if r['kind']=='file': need(rowpin(r)=={'sha256':want['sha256'],'bytes':want['bytes']},'exact imported member bytes')
  need(rowpin(result['manifest.json'])==MF_PIN,'unchanged original manifest'); return result,portable(result,manifest)
 def tree_fence(closing=False):
  need(admitted is not None,'tree was admitted'); got,descriptions=closed(closing); need(got==admitted and descriptions==inventory['portableRoleInventories'],'admitted tree strict FD9+SHA unchanged')
 def command(phase,args,seconds=20,allowed=(0,),closing=False):
  deadline(closing); limit=min(seconds,(end if closing else work_end)-time.monotonic()); stem=phase+'-'+str(len(events)+1); proc=None; error=None; joined=False; t=time.monotonic(); streams=[]
  def note(exc,where):
   nonlocal error
   if error is None: error=exc
   else: secondary.append({'phase':phase+'-'+where,'message':str(exc)[:2000]})
  def alive():
   need(proc.pid>1 and proc.pid!=28722,'only owned nonreserved process group')
   try: os.killpg(proc.pid,0); return True
   except ProcessLookupError: return False
  def send(sig):
   need(proc.pid>1 and proc.pid!=28722,'never signal reserved/nonowned PID')
   try: os.killpg(proc.pid,sig)
   except ProcessLookupError: pass
  try:
   streams.append((OUT/(stem+'.stdout.log')).open('xb')); streams.append((OUT/(stem+'.stderr.log')).open('xb'))
   proc=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=streams[0],stderr=streams[1],start_new_session=True,env={'HOME':str(V),'PATH':'/usr/bin:/bin:/usr/sbin:/sbin','LC_ALL':'C','LANG':'C'}); need(proc.pid>1 and proc.pid!=28722,'owned safe PID')
   while proc.poll()is None: need(time.monotonic()-t<limit,'command deadline'); need(all(os.fstat(f.fileno()).st_size<=1048576 for f in streams),'bounded live logs'); time.sleep(.05)
   proc.wait(timeout=.1); need(all(os.fstat(f.fileno()).st_size<=1048576 for f in streams),'bounded terminal logs')
  except BaseException as exc: error=exc
  finally:
   if proc is not None and proc.pid>1 and proc.pid!=28722:
    try:
     if alive(): send(signal.SIGTERM)
     try: proc.wait(timeout=3)
     except subprocess.TimeoutExpired: send(signal.SIGKILL); proc.wait(timeout=3)
     if alive(): send(signal.SIGKILL)
     until=min(end,time.monotonic()+3)
     while alive()and time.monotonic()<until: time.sleep(.05)
     joined=not alive(); need(joined,'owned process group closed')
    except BaseException as exc: note(exc,'join')
   for stream in streams:
    for action in (stream.flush,lambda s=stream:os.fsync(s.fileno()),stream.close):
     try: action()
     except BaseException as exc: note(exc,'log-close')
   event={'phase':phase,'pid':None if proc is None else proc.pid,'exitCode':None if proc is None else proc.returncode,'groupClosed':joined,'elapsedMs':round((time.monotonic()-t)*1000)}; events.append(event)
   try: save(stem+'.exit-public.json',event)
   except BaseException as exc: note(exc,'event-publication')
  if error is not None: raise error
  need(joined and event['exitCode']in allowed,'command exit refused: '+phase); return event['exitCode'],(OUT/(stem+'.stdout.log')).read_bytes(),(OUT/(stem+'.stderr.log')).read_bytes()
 def image(phase,closing=False):
  _,raw,err=command(phase,PREFIX+['image','inspect',IMAGE],15,closing=closing); x=decoded(raw); need(not err and type(x)is list and len(x)==1 and x[0]['Id']==IMAGE and x[0]['Architecture']=='amd64'and x[0]['Os']=='linux','retained exact AMD64 image'); return {'passed':True,'image':IMAGE,'architecture':'amd64'}
 def inspect(phase,target,closing=False):
  rc,raw,err=command(phase,PREFIX+['container','inspect',target],12,(0,1),closing)
  if rc==1: need(b'no such object'in err.lower()or b'no such container'in err.lower(),'exact owned container absence'); return None
  x=decoded(raw); need(not err and type(x)is list and len(x)==1,'one inspected container'); x=x[0]; need(type(x['Id'])is str and re.fullmatch('[0-9a-f]{64}',x['Id'])and x['Name']=='/'+container['name']and x['Image']==IMAGE and x['Config']['Labels'].get(container['label'])==container['name'],'owned CID/name/image/label'); return x
 def cleanup(closing):
  if not container['attempted']: container['cleanup']={'passed':True,'neverCreated':True}; return
  x=None
  if container['cid']is not None:
   try: x=inspect('cleanup-cid',container['cid'],closing)
   except BaseException as exc: secondary.append({'phase':'cleanup-cid-diagnostic','message':str(exc)[:2000]})
  if x is None: x=inspect('cleanup-name',container['name'],closing)
  if x is not None:
   cid=x['Id']
   if x['State']['Running']: command('stop',PREFIX+['stop','--time','5',cid],8,closing=closing)
   x=inspect('stopped',cid,closing); need(x is not None and x['State']['Running']is False,'owned container stopped'); command('remove',PREFIX+['rm',cid],10,closing=closing)
  need(inspect('absence',container['name'],closing)is None,'owned container absent'); container['cleanup']={'passed':True,'ownedContainerAbsent':True}
 def interrupted(sig,frame): raise KeyboardInterrupt('owned import/check signal '+str(sig))
 try:
  for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP): handlers[sig]=signal.signal(sig,interrupted)
  own=pathlib.Path(__file__).absolute(); known(own,{'sha256':sys.argv[4],'bytes':int(sys.argv[5])}); need(stat.S_IMODE(held[str(own)]['st_mode'])==0o400,'controller frozen before first use')
  cp=pathlib.Path(sys.argv[1]); config=decoded(known(cp,{'sha256':sys.argv[2],'bytes':int(sys.argv[3])}))
  exact(config,{'format':'nano-physical-native-import-amd-control-v1','ready':True,'incomingRoot':str(IN),'outputRoot':str(OUT),'limits':LIMITS},('inputs','common','originalAuthorities'))
  need(not os.path.lexists(OUT) and OUT.parent.resolve(strict=True)==OUT.parent and not cp.is_relative_to(OUT),'fresh fixed namespace and external control')
  need(type(config['inputs'])is dict and set(config['inputs'])==set(PINS),'exact five transported public artifacts'); bodies={}
  for key,(name,want)in PINS.items():
   d=config['inputs'][key]; exact(d,{'file':str(IN/name),'pin':want},('originalRow',)); need(rowshape(d['originalRow'])and rowpin(d['originalRow'])==want and stat.S_IMODE(d['originalRow']['st_mode'])==0o400,'original transported row and mode400'); bodies[key]=hold(IN/name,d['originalRow'])
  common=pathlib.Path(__file__).absolute().with_name('cohort_common.py'); d=config['common']; exact(d,{'file':str(common),'pin':COMMON_PIN},('originalRow',)); need(rowshape(d['originalRow'])and rowpin(d['originalRow'])==COMMON_PIN and stat.S_IMODE(d['originalRow']['st_mode'])==0o400,'known frozen common descriptor'); raw=hold(common,d['originalRow']); cc=types.ModuleType('native_import_common_verified_bytes'); cc.__file__=str(common); exec(compile(raw,str(common),'exec'),cc.__dict__)
  manifest=decoded(bodies['manifest']); inventory=cc.validate_tar(IN/PINS['archive'][0],bodies['manifest'],import_end,MF_PIN,(1000,1000)); need(inventory==decoded(bodies['inventory'])and pin(encode(inventory))==PINS['inventory'][1]and inventory['entryCount']==69 and len(inventory['portableRoleInventories'])==8,'same original 69 entries/eight portable inventories')
  commitment=decoded(bodies['commitment']); exact(commitment,{'format':'nano-native-cohort-export-commitment-v1','archivePin':PINS['archive'][1],'manifestPin':MF_PIN,'inventoryPin':PINS['inventory'][1],'sourceProofPins':PROOF_PINS,'outsideCohort':True,'historicalSourceFD9Qualified':False,'native04HostWholeFailedPreserved':True,'productionQualified':False},('sourceCurrentObservationPin',)); sourcepin=commitment['sourceCurrentObservationPin']; need(type(sourcepin)is dict and set(sourcepin)=={'sha256','bytes'}and type(sourcepin['sha256'])is str and re.fullmatch('[0-9a-f]{64}',sourcepin['sha256'])and type(sourcepin['bytes'])is int and 1<=sourcepin['bytes']<=4194304,'committed current observation pin')
  exported=decoded(bodies['exportResult']); exact(exported,{'format':'nano-physical-native-export-result-v1','passed':True,'sourceArchitecture':'macOS-ARM64','archivePin':PINS['archive'][1],'manifestPin':MF_PIN,'inventoryPin':PINS['inventory'][1],'commitmentPin':PINS['commitment'][1],'sourceEntries':69,'portableRoleInventories':8,'historicalSourceFD9Qualified':False,'historicalProofLogFD9Qualified':False,'sourceCurrentFD9ShaUnchanged':True,'native04HostWholeFailedPreserved':True,'oldHostResultsRequalified':False,'cppInvocations':0,'newSignatures':0,'databaseApiOpens':0,'imagesBuilt':0,'productionQualified':False},('outputAdmission','newCarriers'))
  need(type(exported['outputAdmission'])is dict and exported['outputAdmission']['admitted']is True and type(exported['newCarriers'])is dict and len(exported['newCarriers'])==5,'five export carriers independently admitted'); exported_pins={pathlib.Path(p).name:rowpin(r)for p,r in exported['newCarriers'].items()if rowshape(r)}; need(len(exported_pins)==5 and exported_pins=={PINS[k][0]:PINS[k][1]for k in ('archive','manifest','inventory','commitment')}|{'source-current-observation-public.json':sourcepin},'archive/MF/inventory/current-observation/commitment jointly bound')
  authorities=config['originalAuthorities']; need(type(authorities)is dict and set(authorities)=={'control','readerAudit'},'exact two old authority descriptors')
  originals={}
  for key,p,want in (('control',OLD_CONTROL,CONTROL_PIN),('readerAudit',AUDIT,AUDIT_PIN)):
   d=authorities[key]; exact(d,{'file':str(p),'pin':want},('originalRow',)); need(rowshape(d['originalRow'])and rowpin(d['originalRow'])==want,'independent original authority descriptor'); originals[key]=decoded(hold(p,d['originalRow']))
  old=originals['control']; audit=originals['readerAudit']; need(old['format']=='nano-physical-cohort-amd-control-v1'and old['ready']is True and old['docker']['prefix']==PREFIX and old['docker']['image']==IMAGE and old['docker']['architecture']=='amd64'and old['docker']['uid']==old['docker']['gid']==1000,'unchanged original Docker configuration')
  need(type(old['originalFiles'])is dict and len(old['originalFiles'])==10 and PREFIX[0]in old['originalFiles']and PREFIX[2]in old['originalFiles'],'exact ten original software rows')
  for p,r in old['originalFiles'].items(): hold(pathlib.Path(p),r)
  need(audit['passed']is True and audit['cleanup']['ownedContainerAbsent']is True and audit['executableFile']==str(ELF)and rowshape(audit['executableOriginalRow'])and rowpin(audit['executableOriginalRow'])==ELF_PIN,'retained reader original authority'); elf=hold(ELF,audit['executableOriginalRow']); need(elf[:6]==b'\x7fELF\x02\x01'and int.from_bytes(elf[18:20],'little')==62,'actual retained AMD64 ELF')
  old_fence(); need(time.monotonic()<import_end,'all archive/commitment/original authorities verified before first write'); OUT.mkdir(mode=0o700); owned=True; syncdir(OUT.parent)
  expected={n.removeprefix('public-cohort/'):v for n,v in inventory['entries'].items()if n!='public-cohort'}; expected['']=inventory['entries']['public-cohort']
  namespace_fd=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW); directory_fds['']=namespace_fd
  for name,want in sorted(inventory['entries'].items(),key=lambda x:(x[0].count('/'),x[0])):
   need(time.monotonic()<import_end,'bounded import')
   if want['type']!='directory': continue
   parent,_,leaf=name.rpartition('/'); os.mkdir(leaf,mode=0o700,dir_fd=directory_fds[parent]); directory_fds[name]=os.open(leaf,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW,dir_fd=directory_fds[parent]); syncdir(OUT if not parent else OUT/parent)
  with tarfile.open(fileobj=io.BytesIO(bodies['archive']),mode='r:')as archive:
   copied=set()
   for member in archive:
    need(time.monotonic()<import_end,'bounded manual copy'); name=member.name.rstrip('/')if member.isdir()else member.name; need(name in inventory['entries']and name not in copied and not member.pax_headers and member.type in (tarfile.REGTYPE,tarfile.AREGTYPE,tarfile.DIRTYPE)and not member.linkname,'same validated regular/directory member'); copied.add(name); want=inventory['entries'][name]
    need({'type':'directory'if member.isdir()else'file','mode':member.mode,'uid':member.uid,'gid':member.gid,'mtime':member.mtime,'bytes':member.size}=={k:want[k]for k in ('type','mode','uid','gid','mtime','bytes')},'same validated member metadata')
    if member.isdir(): continue
    parent,_,leaf=name.rpartition('/'); fd=os.open(leaf,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600,dir_fd=directory_fds[parent])
    file_error=None
    try:
     stream=archive.extractfile(member); need(stream is not None,'regular stream only'); h=hashlib.sha256(); count=0
     with stream:
      while chunk:=stream.read(65536):
       need(time.monotonic()<import_end,'copy deadline'); h.update(chunk); count+=len(chunk); need(count<=want['bytes'],'bounded member payload'); at=0
       while at<len(chunk): written=os.write(fd,chunk[at:]); need(written>0,'complete exclusive copy'); at+=written
     os.fsync(fd); a=os.fstat(fd); created={k:getattr(a,k)for k in F}; need(count==want['bytes']and h.hexdigest()==want['sha256'],'exact newly copied payload'); creation[name.removeprefix('public-cohort/')]=dict(created,sha256=h.hexdigest())
    except BaseException as exc: file_error=exc
    finally:
     try: os.close(fd)
     except BaseException as exc:
      if file_error is None: file_error=exc
      else: secondary.append({'phase':'copied-file-close','message':str(exc)[:2000]})
    if file_error is not None: raise file_error
    os.fsync(directory_fds[parent])
   need(copied==set(inventory['entries']),'all and only validated members copied')
  for name in sorted(directory_fds,key=lambda n:n.count('/'),reverse=True): os.fsync(directory_fds[name])
  for name,fd in list(directory_fds.items()): os.close(fd); del directory_fds[name]
  initial,_=closed(); fixed={p:{k:v for k,v in r.items()if k!='st_ctime_ns'}for p,r in initial.items()}; previous=None; since=None; streak=0; admission_began=time.monotonic()
  for sample in range(1,31):
   old_fence(); current,descriptions=closed(); now=time.monotonic(); same=current==previous
   if same: streak+=1
   else: streak=1; since=now
   observation={'rows':current,'creationRows':creation if sample==1 else None,'elapsedMs':round((now-admission_began)*1000),'stableSamples':streak,'fixedEightSha':fixed if sample==1 else None}; save('new-copy-sample-'+str(sample)+'.private.json',observation)
   need({p:{k:v for k,v in r.items()if k!='st_ctime_ns'}for p,r in current.items()}==fixed,'new imported eight fields/SHA fixed before first admission')
   for name,r in creation.items(): need(all(current[name][k]==r[k]for k in F if k!='st_ctime_ns')and current[name]['sha256']==r['sha256'],'writer-created eight fields/SHA unchanged')
   need(time.monotonic()<min(import_end,admission_began+30),'import prospective admission deadline includes publication')
   if streak>=3 and now-since>=2: admitted=current; copied_admission={'admitted':True,'samples':sample,'stableSamples':streak,'stableSpanMs':round((now-since)*1000),'ctimeOnlyBeforeFirstAdmission':True}; break
   previous=current; time.sleep(1.05)
  need(admitted is not None,'three stable samples over at least two seconds'); tree_fence(); old_fence(); save('import-external-commitment-public.json',{'manifestPin':MF_PIN,'archivePin':PINS['archive'][1],'inventoryPin':PINS['inventory'][1],'exportCommitmentPin':PINS['commitment'][1],'outsideImportedTreeAndContainerMounts':True,'importedRows':admitted,'admission':copied_admission,'historicalNativeSourceFD9Qualified':False,'native04HostWholeFailedPreserved':True,'productionQualified':False})
  docker_ready=True; image('image-before'); need(inspect('name-before',container['name'])is None,'fresh owned checker name')
  tree=OUT/'public-cohort'; args=PREFIX+['create','--pull=never','--name',container['name'],'--label',container['label']+'='+container['name'],'--network','none','--read-only','--user','1000:1000','--cpus','2','--memory','512m','--pids-limit','64','--security-opt','no-new-privileges','--cap-drop','ALL','--tmpfs','/tmp:rw,nosuid,nodev,size=64m','--mount','type=bind,src='+str(tree)+',dst=/restore,readonly','--mount','type=bind,src='+str(ELF)+',dst=/nano-checker,readonly','--workdir','/tmp','--env','HOME=/tmp','--env','TMPDIR=/tmp','--env','LC_ALL=C','--env','LANG=C','--entrypoint','/nano-checker',IMAGE,'check','/restore',MF_PIN['sha256']]
  container['attempted']=True; _,raw,err=command('create',args,25); need(not err,'empty create stderr'); cid=raw.decode().strip(); need(re.fullmatch('[0-9a-f]{64}',cid),'full owned CID'); container['cid']=cid; x=inspect('created',cid); hc=x['HostConfig']; binds=[m for m in x['Mounts']if m['Type']=='bind']
  need(len(binds)==2 and {m['Destination']:(m['Source'],m['RW'])for m in binds}=={'/restore':(str(tree),False),'/nano-checker':(str(ELF),False)}and not any(m['Type']=='volume'for m in x['Mounts'])and hc['NetworkMode']=='none'and hc['ReadonlyRootfs']is True and hc['Memory']==536870912 and hc['NanoCpus']==2000000000 and hc['PidsLimit']==64 and set(hc.get('CapDrop')or[])=={'ALL'}and not hc.get('CapAdd')and 'no-new-privileges'in(hc.get('SecurityOpt')or[])and x['Config']['User']=='1000:1000'and not hc.get('PortBindings')and not x['NetworkSettings'].get('Ports'),'exact RO two binds, no old volume, no network, dropped capabilities and limits')
  tree_fence(); old_fence(); rc,raw,err=command('check',PREFIX+['start','-a',cid],180,tuple(range(256))); x=inspect('terminal',cid); state=x['State']; save('container-terminal-public.json',{'containerId':cid,'state':state,'dockerCliExitCode':rc}); need(rc==0 and state['Running']is False and state['Status']=='exited'and type(state['ExitCode'])is int and state['ExitCode']==0 and state.get('OOMKilled')is False and not state.get('Error')and not state.get('FinishedAt','0001').startswith('0001'),'CLI0 AND independent State0, no OOM/runtime error')
  need(err==b'','empty C++ stderr'); rows=[decoded(line)for line in raw.splitlines()]; cohort_schema(rows,manifest); save('checker-records-public.json',rows); cleanup(False); tree_fence(); old_fence()
 except BaseException as exc: fail(exc,'import-or-check')
 finally:
  for sig in handlers:
   try: signal.signal(sig,signal.SIG_IGN)
   except BaseException as exc: fail(exc,'ignore-closing-signal')
  for name,fd in list(directory_fds.items()):
   try: os.close(fd)
   except BaseException as exc: fail(exc,'directory-close:'+name)
  if docker_ready:
   try: cleanup(True)
   except BaseException as exc: fail(exc,'final-cleanup')
   try: final_image=image('image-final',True)
   except BaseException as exc: fail(exc,'final-image')
  input_ok=False; tree_ok=False
  try: old_fence(); input_ok=True
  except BaseException as exc: fail(exc,'original-input-final-fence')
  if admitted is not None:
   try: tree_fence(True); tree_ok=True
   except BaseException as exc: fail(exc,'imported-tree-final-fence')
  for sig,handler in handlers.items():
   try: signal.signal(sig,handler)
   except BaseException as exc: fail(exc,'restore-handler')
  fence={'passed':input_ok and tree_ok,'originalInputsUnchanged':input_ok,'imported69FD9ShaUnchangedAfterCleanup':tree_ok,'heldFiles':len(held),'afterCleanup':container['cleanup'],'imageFinal':final_image}
  if owned:
   try: save('input-final-fence-public.json',fence)
   except BaseException as exc: fail(exc,'fence-publication')
  passed=first is None and rows is not None and tree_ok and input_ok and final_image is not None and container['cleanup']is not None and container['cleanup']['passed']is True
  result={'format':'nano-physical-native-import-amd-result-v1','passed':passed,'archivePin':PINS['archive'][1],'manifestPin':MF_PIN,'inventoryPin':PINS['inventory'][1],'exportCommitmentPin':PINS['commitment'][1],'executablePin':ELF_PIN,'newImportedEntries':0 if admitted is None else len(admitted),'portableRoleInventories':8 if admitted is not None else 0,'owner':{'uid':1000,'gid':1000},'newPhysicalIdentities':admitted is not None,'checkInvocationsAttempted':sum(e['phase']=='check'for e in events),'checkerRecords':rows,'inputFinalFence':fence,'imageFinal':final_image,'container':container,'processEvents':events,'firstFailure':first,'secondaryFailures':secondary,'historicalNativeSourceFD9Qualified':False,'historicalProofLogFD9Qualified':False,'native04HostWholeFailedPreserved':True,'oldHostResultsRequalified':False,'originalRowsRefreshed':False,'cppCompilations':0,'CoreRebuilt':False,'produceInvocations':0,'newSignaturesExpected':0,'operatorRecoveryQualified':False,'runtimeDutiesAuthorized':False,'commonFinalityQualified':False,'globalPhysicalPortabilityQualified':False,'productionQualified':False,'partialTreeRetained':not passed}
  if owned:
   try: save('result-public.json',result)
   except BaseException as exc: fail(exc,'result-publication'); passed=False
  else: print(json.dumps({'passed':False,'prestartFailure':first,'secondaryFailures':secondary}),file=sys.stderr)
 return 0 if passed else 1
if __name__=='__main__':
 try: raise SystemExit(main())
 except BaseException as exc:
  if isinstance(exc,SystemExit): raise
  print(json.dumps({'passed':False,'prestartFailure':str(exc)[:2000]}),file=sys.stderr); raise SystemExit(1)
