#!/usr/bin/env python3
"""Root-only selected known-byte copies and a read-only Linux FD probe; no project execution."""
import argparse, hashlib, json, os, pathlib, re, resource, signal, stat, subprocess, sys, time, uuid

F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
A=pathlib.Path('/home/ubuntu/nano-current-helper-amd-dbf51ad9cfd14c82a0aaaae635a7e14e/current-amd-helpers-dbf51ad9cfd14c82a0aaaae635a7e14e')
OLD=pathlib.Path('/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a')
P2P=OLD/'network/nano-p2p'
NANO=pathlib.Path('/home/ubuntu/nano-complete-nano-amd-38d46c8fa8684530a2669bd8c8922895/attempt01/outputs/nano')
ROWS={
 'bootstrapAudit':('nano-capella-isolated-bootstrap-audit',A/'outputs/nano-capella-isolated-bootstrap-audit','a4a4d36eae7fde6d0104c30c98fcfe0ee647bd3c504385ad12474c5fc9369b7c',2513488),
 'stoppedCapture':('nano-capella-isolated-stopped-capture',A/'outputs/nano-capella-isolated-stopped-capture','5ee6fcf0615ddc66fac613fb1acef89a37271e36e50ab03149ad87571c86d5c5',6097064),
 'lifecycleTool':('nano-capella-deposit-audit',A/'outputs/nano-capella-deposit-audit','45090eaee0ed9738ef6d2deb2dda8c843e36de4dafd5ee218f1b814693b0e442',6589712),
 'lifecycleSubmit':('nano-capella-lifecycle-submit',A/'outputs/nano-capella-lifecycle-submit','ba626c763b6e98c3b69416eb249c6c66fae2ea74c6863f4b8b6abb26d38fa2e3',482584),
 'nano':('nano',NANO,'d0b9d264c79d5a0bb0e163c2ef8935bf41f84e9353bf17bd2af7c5b8b7cb751f',13919368),
 'core':('libnano_core.a',A/'baseline/libnano_core.a','e49483a58f71b9b66f83a88b7c5631f6657d77fcae3c85bc57424fbe6bcbed92',40819792),
 'p2p':('nano-p2p',P2P,'b87645b74fb4b3139d4d9d9f43443e2afaff6b773375941a7fb624f113d88795',18964752)}
P2P_NINE=dict(zip(F,(2049,894370,33261,1,1000,1000,18964752,1792105628315685200,1792105628315685200)))
ENV={'PATH':'/usr/bin:/bin','HOME':'/home/ubuntu','LANG':'C','LC_ALL':'C'}
START=time.monotonic();END=START+100;WORK_END=END-15
OUT=None;CREATED=False;ACTIVE=None;CID=None;ATTEMPTED=False;STAGE='arguments';TOOLS={};SOURCES={};INPUT=None;COUNTER=0;CLEAN_END=None
os.umask(0o077)
def need(ok):
 if not ok:raise RuntimeError('selected runtime bundle refused')
def interrupted(*_):raise InterruptedError('selected runtime bundle interrupted')
for sig in (signal.SIGTERM,signal.SIGINT,signal.SIGHUP):signal.signal(sig,interrupted)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def exact_row(row):
 need(isinstance(row,dict) and set(row)==set(F)|{'sha256'} and all(type(row[k])is int and row[k]>=0 for k in F) and re.fullmatch('[0-9a-f]{64}',row['sha256']))
def material(path,limit=67108864,copy_to=None):
 p=pathlib.Path(path);need(p.is_absolute() and p.resolve(strict=True)==p and time.monotonic()<END)
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK);parts=[];h=hashlib.sha256();total=0
 try:
  s=os.fstat(fd);need(stat.S_ISREG(s.st_mode) and s.st_nlink==1 and 0<=s.st_size<=limit);row={k:getattr(s,k)for k in F}
  while True:
   need(time.monotonic()<END);raw=os.read(fd,1048576)
   if not raw:break
   total+=len(raw);need(total<=s.st_size and total<=limit);h.update(raw)
   if copy_to is None:parts.append(raw)
   else:copy_to.write(raw)
  need(total==s.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F})
  return dict(row,sha256=h.hexdigest()),b''.join(parts)
 finally:os.close(fd)
def fence(rows):
 for path,row in rows.items():need(material(path)[0]==row)
def save(leaf,obj):
 need(CREATED and re.fullmatch('[A-Za-z0-9_.-]+',leaf) and leaf not in ('.','..'))
 raw=(json.dumps(obj,sort_keys=True)+'\n').encode();p=OUT/leaf
 with p.open('xb')as f:f.write(raw);f.flush();os.fsync(f.fileno())
 fd=os.open(OUT,os.O_RDONLY|os.O_DIRECTORY)
 try:os.fsync(fd)
 finally:os.close(fd)
 return pin(raw)
def group_alive(pid):
 try:os.killpg(pid,0);return True
 except ProcessLookupError:return False
def stop(q):
 for sig,span in ((signal.SIGTERM,.5),(signal.SIGKILL,.5)):
  if group_alive(q.pid):
   try:os.killpg(q.pid,sig)
   except ProcessLookupError:pass
  until=min(END,time.monotonic()+span)
  while time.monotonic()<until and(q.poll()is None or group_alive(q.pid)):time.sleep(.01)
 need(q.poll()is not None and not group_alive(q.pid))
def run(name,args,seconds=10,cleanup=False):
 global ACTIVE,COUNTER
 fence(TOOLS);deadline=CLEAN_END if cleanup else WORK_END;need(time.monotonic()<deadline);COUNTER+=1;label=f'{COUNTER:02d}-{name}'
 save(label+'-argv.private.json',args)
 def cap():resource.setrlimit(resource.RLIMIT_FSIZE,(1048576,1048576))
 with(OUT/(label+'-stdout.private.log')).open('xb')as stdout,(OUT/(label+'-stderr.private.log')).open('xb')as stderr:
  q=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr,env=ENV,cwd=OUT,start_new_session=True,preexec_fn=cap);ACTIVE=q;until=min(deadline,time.monotonic()+seconds)
  try:
   while q.poll()is None:need(time.monotonic()<until);time.sleep(.01)
   need(not group_alive(q.pid));stdout.flush();stderr.flush();os.fsync(stdout.fileno());os.fsync(stderr.fileno())
  except BaseException:
   try:stop(q)
   except BaseException:pass
   raise
  finally:
   if q.poll()is not None and not group_alive(q.pid):ACTIVE=None
 need(q.returncode==0);fence(TOOLS);raw=material(OUT/(label+'-stdout.private.log'),1048576)[1];err=material(OUT/(label+'-stderr.private.log'),1048576)[1];need(not err)
 save(label+'-exit.json',{'exitCode':q.returncode});return raw
def stable(pins):
 start=time.monotonic();since=start;previous=None;count=0;observations=[]
 while True:
  need(time.monotonic()<WORK_END);table={str(p):material(p)[0]for p in pins}
  for p,known in pins.items():
   row=table[str(p)];need(row['st_uid']==row['st_gid']==1000 and {'sha256':row['sha256'],'bytes':row['st_size']}==known and not(row['st_mode']&0o222))
  now=time.monotonic();observations.append({'elapsedMs':int((now-start)*1000),'files':table})
  if table==previous:count+=1
  else:previous=table;count=1;since=now
  need(now-start<20)
  if count>=3 and now-since>=2:break
  time.sleep(1.05)
 save('new-bundle-first-observations.private.json',observations);return table

READER=r'''use strict;use warnings;use Fcntl qw(:DEFAULT :mode);use Cwd qw(abs_path);use JSON::PP ();use Digest::SHA ();
sub need{die "selected FD reader refused\n" unless $_[0];}
sub same{my($a,$b)=@_;return 0 unless @$a==9&&@$b==9;for(0..8){return 0 unless defined($a->[$_])&&defined($b->[$_])&&!ref($a->[$_])&&!ref($b->[$_])&&$a->[$_] eq $b->[$_];}return 1;}
sub row{my($fh,$p)=@_;open(my $s,'-|','/usr/bin/stat','-Lc','%d|%i|%f|%h|%u|%g|%s|%y|%z','--','/proc/'.$$.'/fd/'.fileno($fh),$p)or die;local $/;my $raw=<$s>;close($s)or die;need(defined($raw)&&length($raw)<4096&&($raw=~s/\n\z//)==1&&$raw!~/\r/);my @l=split(/\n/,$raw,-1);need(@l==2);my @a=split(/\|/,$l[0],-1);my @b=split(/\|/,$l[1],-1);need(same(\@a,\@b));return \@a;}
my $j=JSON::PP->new->canonical;my $expected=$j->decode($ARGV[0]);my %rows;
for my $role(sort keys %$expected){my $z=$expected->{$role};my $p=$z->{path};need(abs_path($p) eq $p&&$p=~m{\A/(?:outputs|network-build)/[A-Za-z0-9_.-]+\z});sysopen(my $f,$p,O_RDONLY|O_NOFOLLOW|O_NONBLOCK)or die;my @s=stat($f);need(@s&&S_ISREG($s[2])&&$s[3]==1&&$s[4]==1000&&$s[5]==1000&&($s[2]&07777)==$z->{mode}&&$s[7]==$z->{bytes});my $before=row($f,$p);my $h=Digest::SHA->new(256);my $n=0;while(1){my $got=sysread($f,my $b,65536);need(defined($got));last unless $got;$n+=$got;need($n<=$z->{bytes});$h->add($b);}my $sha=$h->hexdigest;need($n==$z->{bytes}&&$sha eq $z->{sha256}&&same($before,row($f,$p))&&abs_path($p) eq $p);close($f)or die;$rows{$role}={material=>$before,sha256=>$sha,bytes=>$n};}
print $j->encode({passed=>JSON::PP::true,files=>\%rows})."\n";'''

def main():
 global OUT,CREATED,TOOLS,SOURCES,INPUT,CID,ATTEMPTED,STAGE,CLEAN_END
 parser=argparse.ArgumentParser();parser.add_argument('--input',required=True);parser.add_argument('--input-sha256',required=True);parser.add_argument('--input-bytes',required=True,type=int);parser.add_argument('--output-root',required=True);parser.add_argument('--p2p-path',required=True);a=parser.parse_args()
 need(sys.platform.startswith('linux')and os.getuid()==os.getgid()==1000 and pathlib.Path(a.p2p_path)==P2P)
 ip=pathlib.Path(a.input);ir,raw=material(ip,65536);need(pin(raw)=={'sha256':a.input_sha256,'bytes':a.input_bytes});INPUT={str(ip):ir};control=json.loads(raw)
 need(set(control)=={'format','sources','tools'}and control['format']=='nano-current-amd-final-runtime-input-v1'and set(control['sources'])==set(ROWS)and set(control['tools'])=={'docker','sudo'})
 for key,p in (('docker','/usr/bin/docker'),('sudo','/usr/bin/sudo')):
  row=control['tools'][key];need(set(row)=={'path','material'}and row['path']==p);exact_row(row['material']);need(material(p)[0]==row['material']);TOOLS[p]=row['material']
 for role,(leaf,path,sha,size)in ROWS.items():
  z=control['sources'][role];need(set(z)=={'path','sha256','bytes','material'}and z['path']==str(path)and z['sha256']==sha and z['bytes']==size);exact_row(z['material']);row=material(path)[0]
  need(row==z['material']and row['sha256']==sha and row['st_size']==size and row['st_uid']==row['st_gid']==1000 and not(row['st_mode']&0o022))
  if role=='p2p':need({k:row[k]for k in F}==P2P_NINE)
  SOURCES[str(path)]=row
 candidate=pathlib.Path(a.output_root);need(candidate.is_absolute()and candidate.parent.resolve(strict=True)==candidate.parent and candidate.parent.stat().st_uid==1000 and re.fullmatch('current-amd-final-runtime-[a-z0-9-]{1,100}',candidate.name)and not candidate.exists()and not candidate.is_symlink())
 for protected in [OLD,A,NANO.parent,pathlib.Path('/home/ubuntu/blockchain-nano-git')]:need(not candidate.is_relative_to(protected)and not protected.is_relative_to(candidate))
 candidate.mkdir(mode=0o700);OUT=candidate;CREATED=True
 for leaf in ('core-build','network-build','docker-client'):(OUT/leaf).mkdir(mode=0o700)
 STAGE='copy';pins={};guest={}
 for role,(leaf,path,sha,size)in ROWS.items():
  target=OUT/('network-build'if role=='p2p'else'core-build')/leaf;mode=0o400 if role=='core'else 0o500
  with target.open('xb')as f:row,_=material(path,copy_to=f);need(row==SOURCES[str(path)]);f.flush();os.fsync(f.fileno())
  os.chmod(target,mode);pins[target]={'sha256':sha,'bytes':size};guest[role]={'path':('/network-build/'if role=='p2p'else'/outputs/')+leaf,'sha256':sha,'bytes':size,'mode':mode}
 for leaf in ('core-build','network-build'):os.chmod(OUT/leaf,0o555)
 table=stable(pins);fence(SOURCES);fence(INPUT);fence(TOOLS)
 docker=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',str(OUT/'docker-client')];name='nano-amd-final-fd-'+uuid.uuid4().hex;labelkey='nano.amd.final.fd';label=labelkey+'='+name;first_error=None;probe=None;cleanup=False
 def dk(step,args,seconds=10,clean=False):return run(step,docker+args,seconds,clean)
 try:
  STAGE='image';im=json.loads(dk('image',['image','inspect',IMAGE,'--format','{{json .}}']));need(im['Id']==IMAGE and im['Architecture']=='amd64'and im['Os']=='linux')
  need(not dk('absence',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}']).strip())
  args=['create','--name',name,'--label',label,'--pull=never','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','1','--memory','256m','--pids-limit','32','--user','1000:1000','--mount','type=bind,src='+str(OUT/'core-build')+',dst=/outputs,readonly','--mount','type=bind,src='+str(OUT/'network-build')+',dst=/network-build,readonly','--entrypoint','/usr/bin/perl',IMAGE,'-e',READER,json.dumps(guest,sort_keys=True)]
  ATTEMPTED=True;CID=dk('create',args).decode().strip();need(re.fullmatch('[0-9a-f]{64}',CID));STAGE='fd-reader'
  before=json.loads(dk('inspect-before',['inspect',CID,'--format','{{json .}}']));need(before['Id']==CID and before['Name']=='/'+name and before['Config']['Labels'].get(labelkey)==name and before['Image']==IMAGE)
  probe=json.loads(dk('read',['start','-a',CID],35));need(probe['passed']is True and set(probe['files'])==set(ROWS))
  after=json.loads(dk('inspect-after',['inspect',CID,'--format','{{json .}}']));need(after['State']['ExitCode']==0 and not after['State']['Running'])
  for role,z in probe['files'].items():need(z['sha256']==guest[role]['sha256']and z['bytes']==guest[role]['bytes'])
 except BaseException as error:first_error=error;raise
 finally:
  try:
   for sig in (signal.SIGTERM,signal.SIGINT,signal.SIGHUP):signal.signal(sig,signal.SIG_IGN)
   CLEAN_END=min(END,time.monotonic()+15)
   if ATTEMPTED:
    if not CID or re.fullmatch('[0-9a-f]{64}',CID)is None:CID=dk('cleanup-discover',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],clean=True).decode().strip()or None
    if CID:
     need(re.fullmatch('[0-9a-f]{64}',CID));item=json.loads(dk('cleanup-inspect',['inspect',CID,'--format','{{json .}}'],clean=True));need(item['Id']==CID and item['Name']=='/'+name and item['Config']['Labels'].get(labelkey)==name and item['Config']['Image']==IMAGE and item['Image']==IMAGE);dk('cleanup-remove',['rm','-f',CID],clean=True)
    need(not dk('cleanup-absence',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],clean=True).strip())
   cleanup=True;save('cleanup-public.json',{'ownedContainerAbsent':True,'creationAttempted':ATTEMPTED})
  except BaseException as error:
   save('cleanup-failure-public.json',{'passed':False,'firstFailurePreserved':first_error is not None,'errorType':type(error).__name__})
   if first_error is None:raise
 need(cleanup);STAGE='final-fences';fence(SOURCES);fence(INPUT);fence(TOOLS);fence(table)
 binding=save('runtime-bundle-admission.private.json',{'format':'nano-current-amd-final-runtime-binding-v1','files':table,'sourceFiles':SOURCES,'input':INPUT,'tools':TOOLS,'linuxReadOnlyProbe':probe,'coreBuildRoot':str(OUT/'core-build'),'networkBuildRoot':str(OUT/'network-build'),'originalSourceFullNineAndShaUnchanged':True,'oldAuthoritiesRefreshed':False})
 result={'format':'nano-current-amd-final-runtime-bundle-v1','passed':True,'programs':6,'coreArchives':1,'readOnlyFdFiles':7,'sourceFullNineAndShaUnchangedSinceRootInput':True,'nanoSourceHostNineRecoveredHistorically':False,'nanoSourceHostNineAdmittedProspectivelyByRoot':True,'newStableSamplesAtLeast':3,'newStableSpanAtLeastTwoSeconds':True,'bundleAdmission':binding,'artifacts':{r:{'sha256':z[2],'bytes':z[3]}for r,z in ROWS.items()},'ownedContainerAbsent':True,'compilersRun':0,'linksRun':0,'projectProgramsRun':0,'oldSdkMapsRead':False,'historicalAuthorityRefreshed':False,'dutiesResumed':False,'runtimeStarted':False,'commonFinalityQualified':False,'productionQualified':False}
 save('result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)
if __name__=='__main__':
 failed=False
 try:main()
 except BaseException:
  failed=True
  if CREATED:save('failure-public.json',{'passed':False,'stage':STAGE,'firstFailurePreserved':True,'rawErrorDetailsExported':False,'productionQualified':False})
  raise
 finally:
  for sig in (signal.SIGTERM,signal.SIGINT,signal.SIGHUP):signal.signal(sig,signal.SIG_IGN)
  try:
   if ACTIVE is not None:stop(ACTIVE)
   fence(SOURCES);fence(INPUT or{});fence(TOOLS)
   if CREATED:save('sources-final-fence-public.json',{'passed':True,'firstFailurePreserved':failed,'oldAuthoritiesRefreshed':False})
  except BaseException as error:
   if CREATED:save('sources-final-fence-failure-public.json',{'passed':False,'firstFailurePreserved':failed,'errorType':type(error).__name__})
   if not failed:raise
