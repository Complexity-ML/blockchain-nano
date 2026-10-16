#!/usr/bin/env python3
"""Root-only existing-binary SST probe; source author performed no operations."""
import argparse, hashlib, json, os, pathlib, re, signal, stat, subprocess, sys, time, uuid
os.umask(0o077)

F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
IMAGES={'mac-docker':('sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb',501,20,'arm64'),'vps-docker':('sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14',1000,1000,'amd64')}
LEAVES={'nano-owned-copy-physical-admission-tests','nano-storage-maintenance'}
ACTIVE=None;OUT=None;END=time.monotonic()+210;CLEAN=False;SEQUENCE=0
def need(ok):
 if not ok:raise RuntimeError('SST disk probe refused')
def digest(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
def read_control(path,known):
 need(path.is_absolute() and path.resolve()==path);fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  before=os.fstat(fd);row={k:getattr(before,k) for k in F};need(stat.S_ISREG(before.st_mode) and 0<before.st_size<=16384);raw=os.read(fd,before.st_size+1)
  need(digest(raw)==known and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(path.lstat(),k) for k in F});return row,raw
 finally:os.close(fd)
def save(leaf,value):
 raw=(json.dumps(value,sort_keys=True)+'\n').encode();fd=os.open(OUT/leaf,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  at=0
  while at<len(raw):n=os.write(fd,raw[at:]);need(n>0);at+=n
  os.fsync(fd)
 finally:os.close(fd)
 return digest(raw)
def stop(q):
 if q.poll() is None:
  try:os.killpg(q.pid,signal.SIGTERM)
  except ProcessLookupError:pass
  try:q.wait(timeout=1)
  except subprocess.TimeoutExpired:
   try:os.killpg(q.pid,signal.SIGKILL)
   except ProcessLookupError:pass
   q.wait(timeout=1)
def command(argv,seconds=15,data=None):
 global ACTIVE,SEQUENCE
 need(OUT is not None);remaining=END-time.monotonic();need(remaining>0);SEQUENCE+=1;label=f'{SEQUENCE:02d}'
 save(label+'-argv.private.json',argv)
 started=time.monotonic();failure=None
 with (OUT/(label+'-stdout.private.log')).open('xb') as out,(OUT/(label+'-stderr.private.log')).open('xb') as err:
  q=subprocess.Popen(argv,stdin=subprocess.PIPE if data is not None else subprocess.DEVNULL,stdout=out,stderr=err,start_new_session=True,env={'PATH':'/usr/bin:/bin','HOME':str(OUT),'LANG':'C','LC_ALL':'C'});ACTIVE=q
  try:
   q.communicate(input=data,timeout=min(seconds,remaining));need(out.tell()<=16777216 and err.tell()<=16777216)
  except BaseException as error:
   failure=error
   try:stop(q)
   except BaseException:pass
  finally:
   if q.poll() is not None:ACTIVE=None
   event={'exitCode':q.returncode,'elapsedMs':int((time.monotonic()-started)*1000)}
   if failure is not None:event['errorType']=type(failure).__name__
   try:out.flush();err.flush();os.fsync(out.fileno());os.fsync(err.fileno());save(label+'-exit.json',event)
   except BaseException as error:
    if failure is None:failure=error
 if failure is not None:raise failure
 need(q.returncode==0)
 stdout=(OUT/(label+'-stdout.private.log')).read_bytes();stderr=(OUT/(label+'-stderr.private.log')).read_bytes();need(not stderr);return stdout

READER=r'''
use strict;use warnings;use Fcntl qw(:DEFAULT :mode);use JSON::PP ();use Digest::SHA ();use Cwd qw(abs_path);
sub need {die "SST disk reader refused\n" unless $_[0]}
my $raw;{local $/;$raw=<STDIN>;}need(defined($raw)&&length($raw)<=16384);my $c=JSON::PP->new->decode($raw);
sub same {my($a,$b)=@_;return 0 unless ref($a)eq'ARRAY'&&ref($b)eq'ARRAY'&&@$a==9&&@$b==9;for(0..8){return 0 if !defined($a->[$_])||!defined($b->[$_])||ref($a->[$_])||ref($b->[$_])||$a->[$_] ne $b->[$_];}return 1;}
sub row {my($f,$p)=@_;open(my $s,'-|','/usr/bin/stat','-Lc','%d|%i|%f|%h|%u|%g|%s|%y|%z','--','/proc/'.$$.'/fd/'.fileno($f),$p)or die;my $v;{local $/;$v=<$s>;}need(close($s));need(defined($v)&&length($v)<4096&&$v=~s/\n\z//&&$v!~/\r/);my @l=split(/\n/,$v,-1);need(@l==2);my @a=split(/\|/,$l[0],-1);my @b=split(/\|/,$l[1],-1);need(same(\@a,\@b));return \@a;}
sub check {
 for my $leaf(sort keys %{$c->{programs}}){my $pin=$c->{programs}{$leaf};my $p='/outputs/'.$leaf;need(abs_path($p)eq$p);sysopen(my $f,$p,O_RDONLY|O_NOFOLLOW|O_NONBLOCK)or die;
  my @s=stat($f);need(@s&&S_ISREG($s[2])&&$s[3]==1&&$s[4]==$c->{uid}&&$s[5]==$c->{gid}&&($s[2]&07777)==0500);my $before=row($f,$p);need(same($before,$pin->{materialGNU9}));my $h=Digest::SHA->new(256);my $n=0;
  while(1){my $k=sysread($f,my $b,65536);need(defined($k));last unless $k;$n+=$k;need($n<=$pin->{bytes});$h->add($b);}my $sha=$h->hexdigest;
  need($n==$pin->{bytes}&&$sha eq$pin->{sha256}&&same($before,row($f,$p))&&abs_path($p)eq$p);need(close($f));
 }
}
sub empty {opendir(my $d,'/tmp')or die;my @n=grep{$_ ne'.'&&$_ ne'..'}readdir($d);closedir($d);need(!@n);}
need($>==$c->{uid}&&$<==$c->{uid});empty();check();my $status=system{'/outputs/nano-owned-copy-physical-admission-tests'}'/outputs/nano-owned-copy-physical-admission-tests','--pruning-sst-reclaim';check();empty();need($status==0);
'''

def main():
 global OUT,END,CLEAN
 p=argparse.ArgumentParser();p.add_argument('--control',required=True);p.add_argument('--sha256',required=True);p.add_argument('--bytes',required=True,type=int);a=p.parse_args();need(re.fullmatch('[0-9a-f]{64}',a.sha256))
 path=pathlib.Path(a.control);known={'sha256':a.sha256,'bytes':a.bytes};original,raw=read_control(path,known);c=json.loads(raw)
 need(set(c)=={'format','owner','imageId','dockerArgv','dockerEndpoint','oldOutputs','outputRoot','programs'} and c['format']=='nano-pruning-sst-disk-runner-input-v1' and c['owner'] in IMAGES)
 image,uid,gid,arch=IMAGES[c['owner']];need(c['imageId']==image and set(c['programs'])==LEAVES)
 for pin in c['programs'].values():
  need(set(pin)=={'materialGNU9','sha256','bytes'} and isinstance(pin['materialGNU9'],list) and len(pin['materialGNU9'])==9 and all(isinstance(q,str) and q and not re.search('[\x00\r\n|]',q) for q in pin['materialGNU9']) and re.fullmatch('[0-9a-f]{64}',pin['sha256']) and type(pin['bytes']) is int and 0<pin['bytes']<=67108864)
 prefix=c['dockerArgv'];need(isinstance(prefix,list) and all(isinstance(q,str) and q and not re.search('[\x00\r\n]',q) for q in prefix) and (len(prefix)==1 and pathlib.Path(prefix[0]).is_absolute() and pathlib.Path(prefix[0]).name=='docker' or prefix==['/usr/bin/sudo','-n','/usr/bin/docker']))
 need(isinstance(c['dockerEndpoint'],str) and re.fullmatch('unix:///[^\x00\r\n]+',c['dockerEndpoint']))
 candidate=pathlib.Path(c['outputRoot']);need(candidate.is_absolute() and candidate.parent.resolve()==candidate.parent and candidate.parent.is_dir() and not candidate.exists() and not candidate.is_symlink() and candidate.parent.stat().st_uid==os.getuid())
 need(not path.is_relative_to(candidate) and not pathlib.Path(__file__).resolve().is_relative_to(candidate))
 for protected in (pathlib.Path('/Users/boris/Dev/blockchain-nano'),pathlib.Path('/home/ubuntu/blockchain-nano-git')):need(not candidate.is_relative_to(protected))
 old=c['oldOutputs'];need(old.get('type') in ('volume','bind'))
 if old['type']=='volume':need(set(old)=={'type','name','labels'} and re.fullmatch('[A-Za-z0-9][A-Za-z0-9_.-]{0,159}',old['name']) and isinstance(old['labels'],dict) and old['labels'])
 else:
  oldpath=pathlib.Path(old['path']);need(c['owner']=='vps-docker' and set(old)=={'type','path'} and oldpath.is_absolute() and oldpath.resolve()==oldpath and oldpath.is_dir() and not candidate.is_relative_to(oldpath))
 candidate.mkdir(mode=0o700);OUT=candidate;(OUT/'docker-client').mkdir(mode=0o700);docker=[*prefix,'--host',c['dockerEndpoint'],'--config',str(OUT/'docker-client')]
 def dk(args,seconds=15,data=None):return command(docker+args,seconds,data)
 nonce=uuid.uuid4().hex;label='nano.pruning.sst.disk';volume='nano-pruning-sst-disk-'+nonce;containers=[];volume_attempted=False;first=None;result=None
 def inspect_volume(name):return json.loads(dk(['volume','inspect',name,'--format','{{json .}}']))
 def info(cid):return json.loads(dk(['inspect',cid,'--format','{{json .}}']))
 def own(item,name):need(item['Name']=='/'+name and item['Image']==image and item['Config']['Image']==image and item['Config']['Labels'].get(label)==nonce and re.fullmatch('[0-9a-f]{64}',item['Id']))
 def create(suffix,args):
  name=volume+'-'+suffix;need(not dk(['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}']).strip());rec={'name':name,'id':None};containers.append(rec)
  rec['id']=dk(['create','--pull=never','--name',name,'--label',label+'='+nonce,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','128',*args]).decode().strip();need(re.fullmatch('[0-9a-f]{64}',rec['id']));item=info(rec['id']);own(item,name);return rec['id']
 try:
  im=json.loads(dk(['image','inspect',image,'--format','{{json .}}']));need(im['Id']==image and im['Architecture']==arch and im['Os']=='linux')
  if old['type']=='volume':
   v=inspect_volume(old['name']);need(v['Name']==old['name'] and v['Driver']=='local' and all((v.get('Labels') or {}).get(k)==value for k,value in old['labels'].items()));old_mount='type=volume,src='+old['name']+',dst=/outputs,readonly,volume-nocopy'
  else:old_mount='type=bind,src='+old['path']+',dst=/outputs,readonly';need(',' not in old['path'])
  need(not dk(['volume','ls','--filter','name=^'+volume+'$','--format','{{.Name}}']).strip());volume_attempted=True;need(dk(['volume','create','--driver','local','--label',label+'='+nonce,volume]).decode().strip()==volume)
  v=inspect_volume(volume);need(v['Name']==volume and v['Driver']=='local' and (v.get('Labels') or {}).get(label)==nonce)
  tmp_mount='type=volume,src='+volume+',dst=/tmp,volume-nocopy'
  setup="opendir(my $d,'/tmp')or die;die if grep{$_ ne'.'&&$_ ne'..'}readdir($d);closedir($d);chmod(0700,'/tmp')==1 or die;chown("+str(uid)+','+str(gid)+",'/tmp')==1 or die;"
  init=create('init',['--user','0:0','--cap-add','CHOWN','--mount',tmp_mount,'--entrypoint','/usr/bin/perl',image,'-e',setup]);need(not dk(['start','--attach',init]))
  cid=create('test',['--user',f'{uid}:{gid}','--mount',tmp_mount,'--mount',old_mount,'--workdir','/outputs','--entrypoint','/bin/sleep',image,'240']);need(dk(['start',cid]).decode().strip()==cid);item=info(cid)
  mounts={q['Destination']:q for q in item['Mounts']};need(mounts['/tmp']['Type']=='volume' and mounts['/tmp']['Name']==volume and mounts['/tmp']['RW'] is True and mounts['/outputs']['Type']==old['type'] and mounts['/outputs']['RW'] is False and item['HostConfig']['ReadonlyRootfs'] is True)
  need(mounts['/outputs']['Name']==old['name'] if old['type']=='volume' else mounts['/outputs']['Source']==old['path'])
  fstype=dk(['exec','--user',f'{uid}:{gid}',cid,'/usr/bin/stat','-f','-c','%T','/tmp']).decode().strip();need(re.fullmatch('[A-Za-z0-9_/-]+',fstype) and fstype not in ('tmpfs','ramfs'))
  stdout=dk(['exec','-i','--user',f'{uid}:{gid}',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-e',READER],135,(json.dumps({'uid':uid,'gid':gid,'programs':c['programs']},sort_keys=True)+'\n').encode())
  rows=[json.loads(q) for q in stdout.splitlines()];need(len(rows)==4 and all(q['passed'] is True for q in rows) and [q.get('scenario') for q in rows[:3]]==['real-sstore-and-compacted-baseline','bounded-retention-and-matched-control','sst-reclaim-and-cold-logical-invariants']);summary=rows[3]
  need(summary['format']=='nano-pruning-sst-reclaim-v1' and summary['fixtureTransactions']==64 and summary['sstoreWrites']==65536 and summary['deletedStates']==63 and summary['actualCliInvocations']==3 and summary['sstReclaimedBytes']>=65536 and summary['allocatedFileBytesReclaimed']>=65536 and summary['elapsedMs']<=120000 and summary['maxRssKiB']<=131072 and summary['fullEvmPruningQualified'] is False and summary['productionQualified'] is False)
  row,_=read_control(path,known);need(row==original);receipt=save('disk-probe-admission.private.json',{'input':known,'programs':c['programs'],'oldOutputs':old,'newTmpVolume':v,'filesystemType':fstype,'stdout':digest(stdout),'guestOriginalNineAndShaBeforeAfterEqual':True})
  result={'format':'nano-pruning-sst-disk-probe-v1','passed':True,'owner':c['owner'],'imageId':image,'tmpMountType':'volume','filesystemType':fstype,'oldOutputsReadOnly':True,'originalExecutableNineAndShaUnchanged':True,'newTmpEmptyAfterTest':True,'selectedTestRuns':1,'compiledTranslationUnits':0,'startupTestRuns':0,'sstSummary':summary,'admission':receipt,'hostBackingSpaceGuaranteed':False,'runtimeDutiesResumed':False,'productionQualified':False}
 except BaseException as error:first=error
 finally:
  for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP):signal.signal(sig,signal.SIG_IGN)
  END=time.monotonic()+25;CLEAN=True
  try:
   if ACTIVE is not None:stop(ACTIVE)
   for rec in reversed(containers):
    cid=rec['id']
    if not re.fullmatch('[0-9a-f]{64}',cid or ''):
     ids=dk(['ps','-a','--no-trunc','--filter','name=^/'+rec['name']+'$','--filter','label='+label+'='+nonce,'--format','{{.ID}}'],5).decode().splitlines();need(len(ids)<=1);cid=ids[0] if ids else None
    if cid:own(info(cid),rec['name']);dk(['rm','-f',cid],5)
    need(not dk(['ps','-a','--no-trunc','--filter','name=^/'+rec['name']+'$','--format','{{.ID}}'],5).strip())
   if volume_attempted:v=inspect_volume(volume);need(v['Name']==volume and v['Driver']=='local' and (v.get('Labels') or {}).get(label)==nonce)
   row,_=read_control(path,known);need(row==original);save('cleanup-public.json',{'ownedContainersAbsent':True,'newVolumeRetained':volume_attempted,'volumeRemoved':False})
  except BaseException as error:
   try:save('cleanup-failure-public.json',{'passed':False,'firstFailurePreserved':first is not None,'errorType':type(error).__name__})
   except BaseException:pass
   if first is None:first=error
 if first is not None:raise first
 result['ownedContainersAbsent']=True;pin=save('result-public.json',result);print(json.dumps({'passed':True,'result':pin,'admission':receipt},sort_keys=True),flush=True)

def interrupted(*_):raise InterruptedError('SST disk probe interrupted')
if __name__=='__main__':
 for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP):signal.signal(sig,interrupted)
 try:main()
 except BaseException:
  if OUT is not None:
   try:save('failure-public.json',{'passed':False,'rawErrorDetailsExported':False,'productionQualified':False})
   except BaseException:pass
  print('SST disk probe refused',file=sys.stderr);sys.exit(1)
