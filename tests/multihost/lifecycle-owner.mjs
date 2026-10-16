// Owner-local files only. Python runs on the VPS host, never inside an image.
// No stdout mode returns an action, Job/session, capture, environment or DB bytes.
import {linuxRotationCopy} from './rotation-owned-copy.mjs';
const need=(v,m)=>{if(!v)throw Error('Lifecycle owner: '+m);};
export const lifecycleLinuxStatx=String.raw`use Config qw(%Config);use Math::BigInt ();
sub lifecycle_statx_abi{my($os,$arch,$iv,$ptr,$order)=@_;need(defined($os)&&defined($arch)&&defined($iv)&&defined($ptr)&&defined($order)&&$os eq 'linux'&&$iv eq '8'&&$ptr eq '8'&&$order eq '12345678');return 332 if $arch=~m{\Ax86_64-linux(?:-|\z)};return 291 if $arch=~m{\Aaarch64-linux(?:-|\z)};need(0);}
my $lifecycle_statx_number=lifecycle_statx_abi(@Config{qw(osname archname ivsize ptrsize byteorder)});
sub lifecycle_statx_text{return pack('a*',"$_[0]");}
sub lifecycle_statx_ns{my($raw,$offset)=@_;my $sec=unpack('q<',substr($raw,$offset,8));my $ns=unpack('V',substr($raw,$offset+8,4));need($ns<1000000000);return Math::BigInt->new(lifecycle_statx_text($sec))->bmul('1000000000')->badd(lifecycle_statx_text($ns))->bstr;}
sub lifecycle_statx_decode{my($raw)=@_;need(defined($raw)&&!ref($raw)&&length($raw)==256);my $mask=unpack('V',substr($raw,0,4));need(($mask&0x3df)==0x3df);my($links,$owner,$group)=unpack('V3',substr($raw,16,12));my $mode=unpack('v',substr($raw,28,2));my $ino=unpack('Q<',substr($raw,32,8));my $size=unpack('Q<',substr($raw,40,8));my($major,$minor)=unpack('V2',substr($raw,136,8));my $ma=Math::BigInt->new(lifecycle_statx_text($major));my $mi=Math::BigInt->new(lifecycle_statx_text($minor));my $dev=$ma->copy->band('4095')->blsft(8)->bior($mi->copy->band('255'))->bior($mi->copy->band('4294967040')->blsft(12))->bior($ma->copy->band('4294963200')->blsft(32));return [map{lifecycle_statx_text($_)}($dev->bstr,$ino,$mode,$links,$owner,$group,$size,lifecycle_statx_ns($raw,112),lifecycle_statx_ns($raw,96))];}
sub stat9{my($object,$follow)=@_;tick();need(defined($follow)&&($follow==0||$follow==1));my($dirfd,$name,$flags);if($follow){need(defined($object)&&!ref($object)&&lifecycle_statx_text($object)=~m{\A(?:0|[1-9][0-9]*)\z}&&$object<=2147483647);$dirfd=0+$object;$name='';$flags=0x1000;}else{need(defined($object)&&!ref($object)&&$object=~m{\A/}&&index($object,"\0")<0);$dirfd=-100;$name=$object;$flags=0x100|0x800;}my $buffer="\0"x256;my $rc=syscall($lifecycle_statx_number,$dirfd,$name,$flags,0x7ff,$buffer);tick();need($rc==0);return lifecycle_statx_decode($buffer);}
`;
export function lifecycleWholeCopyProgram(namespace,owner,resources){
 need(/^isolated-capella-[0-9a-f]{64}$/.test(namespace)&&['mac-native','mac-docker','vps-docker'].includes(owner),'exact owner namespace');
 const files=BigInt(resources.maximumInventoryFiles),bytes=BigInt(resources.maximumInventoryBytes);need(files>0n&&files<=BigInt(Number.MAX_SAFE_INTEGER)&&bytes>0n&&bytes<=BigInt(Number.MAX_SAFE_INTEGER),'declared safe inventory bounds');
 let p=linuxRotationCopy;
 const replace=(a,b)=>{need(p.split(a).length===2,'fixed copy derivation anchor');p=p.replace(a,b);};
 const legacyStat=String.raw`sub stat9{my($p,$follow)=@_;tick();open(my $pipe,'-|','stat',$follow?'-Lc':'-c','%d|%i|%f|%h|%u|%g|%s|%y|%z','--',$p)or die "stat refused\n";local $/;my $s=<$pipe>;need(close($pipe)&&defined($s)&&length($s)<1024);chomp $s;my @a=split(/\|/,$s,-1);need(@a==9);return \@a;}`;
 replace(legacyStat,lifecycleLinuxStatx);
 replace("sub fdpath{return '/proc/'.$$.'/fd/'.fileno($_[0]);}","sub fdpath{my $fd=fileno($_[0]);need(defined($fd));return $fd;}");
 replace("my @targets=($out.'/data/chain',$out.'/witness/chain-witness');","my @targets=($out.'/data/"+namespace+"/"+owner+"',$out.'/witness/"+namespace+"/"+owner+"');");
 replace("mkdirprivate($out.($i?'/witness':'/data'));","mkdirprivate($out.($i?'/witness':'/data'));mkdirprivate($out.($i?'/witness':'/data').'/"+namespace+"');");
 replace("for my $p($out.'/data',$out.'/witness',$out){","for my $p($out.'/data/"+namespace+"',$out.'/witness/"+namespace+"',$out.'/data',$out.'/witness',$out){");
 replace("format=>'nano-fresh-rotation-owned-copy-private-proof-v1',stability=>","format=>'nano-fresh-lifecycle-owner-copy-private-proof-v2',materialEncoding=>'linux-statx-nine-decimal-ns-v1',stability=>");
 replace("need($proof->{format} eq 'nano-fresh-rotation-owned-copy-private-proof-v1');","need($proof->{format} eq 'nano-fresh-lifecycle-owner-copy-private-proof-v2'&&$proof->{materialEncoding} eq 'linux-statx-nine-decimal-ns-v1');");
 // Full owner roots include core, every signer/witness journal and peer.key.
 // Source setters remain absent; all original nine fields+SHA stay exact.
 p=p.replaceAll('<=20000','<='+files.toString()).replaceAll('<=536870912','<='+bytes.toString());
 return p;
}
export const lifecycleOwnerProjection=String.raw`import os,sys,json,hashlib,stat,pathlib,re,time,subprocess
root=pathlib.Path(sys.argv[1]);x=json.loads(sys.argv[2]);mode=x['mode'];fields=('st_dev','st_ino','st_mode','st_uid','st_gid','st_nlink','st_size','st_mtime_ns','st_ctime_ns')
def need(v):
 if not v:raise ValueError('owner-local lifecycle refusal')
def canon(v):return json.dumps(v,sort_keys=True,separators=(',',':')).encode()
def sha(b):return hashlib.sha256(b).hexdigest()
def path(p):
 p=pathlib.Path(p);need(p.is_absolute() and p==p.resolve() and (p==root or p.is_relative_to(root)));return p
def mat(s):return [str(getattr(s,k)) for k in fields]
def parent(p):
 p=path(p);s=os.lstat(p);need(stat.S_ISDIR(s.st_mode) and s.st_uid==os.geteuid() and stat.S_IMODE(s.st_mode)==0o700);return p
def read(p,pin=None,private=True):
 p=path(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);need(stat.S_ISREG(a.st_mode) and a.st_nlink==1 and a.st_uid==os.geteuid() and (stat.S_IMODE(a.st_mode)==0o600 if private else stat.S_IMODE(a.st_mode) in (0o600,0o644)) and 0<a.st_size<=16777216);b=b''
  while len(b)<a.st_size:
   z=os.read(fd,min(65536,a.st_size-len(b)));need(bool(z));b+=z
  need(not os.read(fd,1) and mat(a)==mat(os.fstat(fd))==mat(os.lstat(p)))
  if pin:need(sha(b)==pin['sha256'] and ('bytes' not in pin or len(b)==pin['bytes']) and ('material' not in pin or mat(a)==pin['material']) and ('materialSha256' not in pin or sha(canon(mat(a)))==pin['materialSha256']))
  return b,{'path':str(p),'sha256':sha(b),'bytes':len(b),'material':mat(a)}
 finally:os.close(fd)
def write(p,b):
 p=path(p);parent(p.parent);known={'path':str(p),'sha256':sha(b),'bytes':len(b)};fd=os.open(p,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  view=memoryview(b)
  while view:n=os.write(fd,view);need(n>0);view=view[n:]
  os.fsync(fd)
 finally:os.close(fd)
 d=os.open(p.parent,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW);os.fsync(d);os.close(d);read(p,known);return known
def fresh(p):p=path(p);parent(p.parent);os.mkdir(p,0o700);return parent(p)
def stable(rows):
 deadline=time.monotonic()+30;previous=None;count=0;since=0;probes=0;last=None
 while time.monotonic()<deadline:
  last=[read(r['path'],r)[1] for r in rows];now=time.monotonic();probes+=1
  if last==previous:count+=1
  else:previous=last;count=1;since=now
  if count>=3 and now-since>=2:return last
  need(deadline-now>1);time.sleep(1)
 raise ValueError('new owner-local admission deadline')
def artifact(p,pin=None):b,r=read(p,pin);return {k:r[k] for k in ('path','sha256','bytes')}
def run(argv,prefix,seconds):
 need(type(argv) is list and all(type(a) is str for a in argv) and re.fullmatch('[a-z0-9-]+',prefix) and 1<=seconds<=615);out=root/(prefix+'-stdout.private.json');err=root/(prefix+'-stderr.private.log');fs=[]
 try:
  for p in (out,err):fs.append(os.fdopen(os.open(path(p),os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600),'wb',buffering=0))
  result=subprocess.run(argv,stdout=fs[0],stderr=fs[1],timeout=seconds,check=False)
 finally:
  for f in fs:os.fsync(f.fileno());f.close()
 write(root/(prefix+'-exit.json'),canon({'exitCode':result.returncode}));need(result.returncode==0);raw,_=read(out);return json.loads(raw),raw
try:
 need(root==root.resolve() and os.geteuid()==1000);parent(root)
 if mode=='stage':
  # Source pins are external known buffers. No read computes their authority.
  fresh(x['directory']);rows=[]
  for z in x['files']:
   p=path(z['path']);need(p.parent==path(x['directory']) and re.fullmatch('[a-z0-9.-]+',p.name));b=read(z['source'],z,False)[0] if 'source' in z else bytes.fromhex(z['hex']);need(len(b)==z['bytes'] and sha(b)==z['sha256']);rows.append(write(p,b))
  stop,stoprow=read(x['stopSource']['path'],x['stopSource']['pin']);rows.append(write(path(x['directory'])/'stop.json',stop));admitted=stable(rows);pin=write(path(x['directory'])/'admitted-inputs.private.json',canon(admitted));ip=stable([pin])[0];r={'staged':True,'inputFiles':len(rows),'admissionSha256':pin['sha256'],'admissionBytes':pin['bytes'],'admissionMaterialSha256':sha(canon(ip['material'])),'rawInputsExported':False,'stopSha256':sha(stop),'stopBytes':len(stop)}
 elif mode=='original-source-fence':
  r,raw=run(x['argv'],x['prefix'],190);need(r['format']=='nano-fresh-rotation-owned-copy-v1' and r['passed'] is True and r['sourceNineFieldAndShaUnchanged'] is True and r['sourceNamespaceUnchanged'] is True and r['rawInventoryExported'] is r['productionQualified'] is False and re.fullmatch('[0-9a-f]{64}',r['copyProofSha256']))
  for key in ('copiesMatchSourceBytes','independentInodes','privateDirectoryModes','privateFileModes','copiesUnchangedAfterAudit'):need(r[key] is False)
  r={'format':'nano-fresh-lifecycle-original-source-fence-v1','passed':True,'sourceNineFieldAndShaUnchanged':True,'sourceNamespaceUnchanged':True,'copyProofSha256':r['copyProofSha256'],'resumedCopiesExpectedToChange':True,'resumedCopiesRechecked':False,'rawInventoryExported':False,'productionQualified':False}
 elif mode=='owner-copy':
  r,raw=run(x['argv'],x['prefix'],190);keys=('format','passed','databaseRoles','sourceFiles','sourceDirectories','payloadBytes','sourceNineFieldAndShaUnchanged','sourceNamespaceUnchanged','copiesMatchSourceBytes','independentInodes','privateDirectoryModes','privateFileModes','copiesUnchangedAfterAudit','copyProofSha256','stabilityProbeCount','stabilityElapsedMs','stabilitySpanMs','rawInventoryExported','productionQualified')
  need(type(r) is dict and set(r)==set(keys) and r['format']=='nano-fresh-rotation-owned-copy-v1' and r['passed'] is True and r['databaseRoles']==2 and r['copiesUnchangedAfterAudit'] is x['afterAudit'] and r['rawInventoryExported'] is r['productionQualified'] is False)
  limits=x['resources'];maximum_files=int(limits['maximumInventoryFiles'],16);maximum_bytes=int(limits['maximumInventoryBytes'],16)
  for key,maximum in (('sourceFiles',2*maximum_files),('sourceDirectories',2*maximum_files),('payloadBytes',2*maximum_bytes)):need(type(r[key]) is int and 0<=r[key]<=maximum)
  for key in ('sourceNineFieldAndShaUnchanged','sourceNamespaceUnchanged','copiesMatchSourceBytes','independentInodes','privateDirectoryModes','privateFileModes'):need(r[key] is True)
  need(type(r['stabilityProbeCount']) is int and r['stabilityProbeCount']>=3 and type(r['stabilitySpanMs']) is int and type(r['stabilityElapsedMs']) is int and 2000<=r['stabilitySpanMs']<=r['stabilityElapsedMs']<=30000 and re.fullmatch('[0-9a-f]{64}',r['copyProofSha256']))
  r={k:v for k,v in r.items() if k not in ('format','databaseRoles')};r['format']='nano-fresh-lifecycle-owner-copy-v1';r['roleRoots']=2
 elif mode=='capture-pin':
  b,row=read(x['capturePath'],{'sha256':x['captureSha256']});c=json.loads(b);need(c['passed'] is True and c['externalWitnessTokensMatched'] is True and c['canonicalStoreSnapshotVerified'] is True and c['primaryAndWitnessBlobClosuresVerified'] is True);stop,sr=read(x['stopPath'],{'sha256':c['externalStopReceiptSha256']});r={'captureBytes':row['bytes'],'stopSha256':sr['sha256'],'stopBytes':sr['bytes'],'headSlot':c['headSlot'],'finalized':c['finalized']}
 elif mode=='tool':
  rows=json.loads(read(x['controlAdmission']['path'],x['controlAdmission'])[0]);[read(z['path'],z) for z in rows]
  try:r,raw=run(x['argv'],x['prefix'],x['seconds'])
  finally:[read(z['path'],z) for z in rows]
  need(r['passed'] is True and r['owner']=='vps-docker' and r['productionQualified'] is False)
  if r.get('stage')=='terminal':
   need(r['status']=='PASS_SCOPED_FRESH_LIFECYCLE');r={k:v for k,v in r.items() if k!='identities'};r['rawCapsuleExported']=False
  else:need(r.get('status') in ('OBSERVED_SCOPED_SEALED_HEAD','PREPARED'))
 elif mode=='plan':
  action,ar=read(x['action']['path'],x['action']);d=json.loads(action);need(action==canon(d) and d['campaignId']==x['campaignId'] and d['owner']=='vps-docker' and d['kind']==x['kind']);parent(path(x['directory']));plan={'format':'nano-fresh-lifecycle-submission-plan-v1','campaignId':x['campaignId'],'owner':'vps-docker','manifest':x['manifest'],'action':x['guestAction'],'realP2p':x['realP2p'],'resultFile':x['resultFile'],'maximumWallSeconds':x['maximumWallSeconds']};b=canon(plan);known=write(x['planFile'],b);rows=stable([known,x['action']]);admission=write(path(x['directory'])/'submission-inputs.private.json',canon(rows));ip=stable([admission])[0];r={'planSha256':known['sha256'],'planBytes':known['bytes'],'admissionSha256':admission['sha256'],'admissionBytes':admission['bytes'],'admissionMaterialSha256':sha(canon(ip['material'])),'actionSha256':ar['sha256'],'actionBytes':ar['bytes'],'knownBytesPinnedBeforeWrite':True,'firstAdmissionStable':True,'rawInputsExported':False}
 elif mode=='accepted-log':
  # Logs are not required to be a single JSON document.
  rows=json.loads(read(x['admission']['path'],x['admission'])[0]);[read(z['path'],z) for z in rows];out=root/(x['prefix']+'-managed-log.private.jsonl');err=root/(x['prefix']+'-managed-log.private.err');fs=[]
  try:
   for p in (out,err):fs.append(os.fdopen(os.open(path(p),os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600),'wb',buffering=0))
   z=subprocess.run(x['logsArgv'],stdout=fs[0],stderr=fs[1],timeout=30,check=False)
  finally:
   for f in fs:os.fsync(f.fileno());f.close()
  need(z.returncode==0);raw,_=read(out);events=[]
  for line in raw.decode().splitlines():
   try:v=json.loads(line)
   except ValueError:continue
   if v.get('event')=='fresh-lifecycle-child-submission-accepted':events.append(v)
  need(len(events)<=1)
  if not events:print(json.dumps({'format':'nano-fresh-lifecycle-owner-public-v1','passed':True,'result':{'pending':True}}));sys.exit(0)
  e=events[0];b,receipt=read(x['resultPath'],{'sha256':e['receiptSha256'],'bytes':e['receiptBytes']});j=json.loads(b)
  need(set(j)=={'format','campaignId','owner','kind','actionSha256','session','sequence','job'} and j['format']=='nano-fresh-lifecycle-accepted-job-v1' and j['campaignId']==x['campaignId'] and j['owner']=='vps-docker' and j['kind']==x['kind'] and j['actionSha256']==x['actionSha256'] and re.fullmatch('0x[0-9a-f]{64}',j['session']) and re.fullmatch('0x[1-9a-f][0-9a-f]*',j['sequence']));job=j['job'];need(set(job)=={'sequence','payloadDigest','cancelRequested','state','decision'} and job['sequence']==j['sequence'] and job['payloadDigest']==x['payloadDigest'] and job['cancelRequested'] is False and job['state']=='completed' and job['decision']=='accepted');[read(z['path'],z) for z in rows]
  admission=write(path(x['resultPath']).parent/'accepted-admission.private.json',canon([receipt]));ip=stable([admission])[0];r={'format':'nano-fresh-lifecycle-accepted-public-v1','accepted':True,'campaignId':j['campaignId'],'owner':j['owner'],'kind':j['kind'],'actionSha256':j['actionSha256'],'receiptSha256':receipt['sha256'],'receiptBytes':receipt['bytes'],'rawJobExported':False,'sessionExported':False,'productionQualified':False};r={'acceptedProjection':r,'admission':{'path':admission['path'],'sha256':admission['sha256'],'bytes':admission['bytes'],'materialSha256':sha(canon(ip['material']))}}
 elif mode=='fence-index':
  rows=json.loads(read(x['admission']['path'],x['admission'])[0]);[read(z['path'],z) for z in rows];r={'strictNineAndShaUnchanged':True,'rawInputsExported':False}
 elif mode=='write-known':
  b=bytes.fromhex(x['hex']);need(sha(b)==x['sha256'] and len(b)==x['bytes']);p=write(x['path'],b);admitted=stable([p])[0];r={'written':True,'materialSha256':sha(canon(admitted['material'])),'sha256':p['sha256'],'bytes':p['bytes'],'knownBytesPinnedBeforeWrite':True}
 elif mode=='fence-known':
  [read(z['path'],z) for z in x['files']];r={'strictNineAndShaUnchanged':True,'rawInputsExported':False}
 elif mode=='read-pin':
  p=artifact(x['path'],x['pin']);r={'sha256':p['sha256'],'bytes':p['bytes'],'rawInputsExported':False}
 else:raise ValueError('unknown lifecycle mode')
 print(json.dumps({'format':'nano-fresh-lifecycle-owner-public-v1','passed':True,'result':r},separators=(',',':')))
except BaseException:
 # Full error and stdout stay in situ. Never print argv, raw inputs or session.
 err=os.open(root/'lifecycle-errors.private.log',os.O_WRONLY|os.O_CREAT|os.O_APPEND|os.O_NOFOLLOW,0o600)
 with os.fdopen(err,'a') as f:
  import traceback;traceback.print_exc(file=f);f.flush();os.fsync(f.fileno())
 print('{"format":"nano-fresh-lifecycle-owner-public-v1","passed":false}')
`;
