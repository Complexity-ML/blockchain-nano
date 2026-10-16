#!/usr/bin/env python3
"""Root-only AMD selected helper proposal. Source author performed no operations."""
import argparse, hashlib, json, os, pathlib, re, resource, shlex, signal, stat, subprocess, sys, tarfile, time, uuid
R=pathlib.Path('/home/ubuntu/blockchain-nano-git')
BASE=pathlib.Path('/home/ubuntu/nano-candidate-e37f349c-5b37-444a-901c-7b7a8747c63a')
REV='bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d'
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
CORE_SOURCE=pathlib.Path('/home/ubuntu/nano-rpc-eip1898-amd-5976e656d1c14d5195a7df93b3a4530b/attempt01/outputs/libnano_core.a')
CORE_PIN={'sha256':'e49483a58f71b9b66f83a88b7c5631f6657d77fcae3c85bc57424fbe6bcbed92','bytes':40819792}
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
REFS={
 'artifacts':('control06/new-artifacts-first-admission.private.json','4e9ff3b48b05c78e3fec34ac3f02f6c2088ae73d4b20e5a5ea493c496749f906',4945),
 'links':('control06/actual-consumer-links.private.json','55ff7525510d2bad38cddd387157d3e9f4eeb1397e66f1712bde4e718a8b5b53',654679),
 'archives':('control06/actual-linked-new-archives.private.json','3a4f182a5e882a4f3713122ac79b781d94dbb8b87fd1a93cf90f14bfa18af5fb',6182185),
 'llvm':('control06/portable-llvm-producers.private.json','ccc3bd6d6450ab0da86cc9d8afd991d1366f1b90898383872374daf4c6e212a1',3273)}
ROLES={
 'bootstrapAudit':('nano-capella-isolated-bootstrap-audit','tests/capella/isolated_bootstrap_audit.cpp',10),
 'stoppedCapture':('nano-capella-isolated-stopped-capture','tests/capella/isolated_stopped_capture.cpp',65),
 'lifecycleTool':('nano-capella-deposit-audit','tests/capella/deposit_audit.cpp',97),
 'lifecycleSubmit':('nano-capella-lifecycle-submit','tests/multihost/lifecycle_submit.cpp',8)}
TREES={'include':'73572ff29b1dea911e79292d65bf1381ffb86018','src':'203a093354f3906e1c7449a5be8a8a3d47c32bb5','network':'8c56a9d154e25ecdf0f72d25cc403eb152c3f328','third_party':'9ff7872a2174571f645aa5d61b037c947af339c6','tests/support':'a3978f97a17ab00da25512b1687247dfcaeaa50d','tests/capella':'0ff1c69d8eb4c27e722c3ff77370d5a3a6c211aa','tests/multihost':'8577c917bec892be4ffac8d36d02eea72cff22c0'}
SELECTED=['include','src','network','third_party','CMakeLists.txt','tests/support','tests/capella','tests/multihost/lifecycle_submit.cpp']
EMPTY={f'/work/third_party/mcl/{part}/.emptydir' for part in ('bin','lib','obj')}
HEADER='/work/tests/support/fresh_lifecycle_terminal.hpp'
BEFORE={'sha256':'07993c65ce97b3e2db1bd640abaabd7abce393e1dea57135bb08c0ad8f2729b8','bytes':24334}
AFTER={'sha256':'6681926f496290afc2b98495315b8ae5ba1c646095aece0e21287cca83b28629','bytes':24376}
CMAKE_PIN={'sha256':'21131c9b019452fb327a13cb4b8758b805921789c2484a0a92177fb8ef131df8','bytes':174949}
ENV={'PATH':'/usr/bin:/bin','HOME':'/home/ubuntu','LANG':'C','LC_ALL':'C','GIT_OPTIONAL_LOCKS':'0','GIT_NO_REPLACE_OBJECTS':'1','GIT_NO_LAZY_FETCH':'1','GIT_TERMINAL_PROMPT':'0'}
HELD={};OUT=None;OUT_CREATED=False;ACTIVE=None;COUNTER=0;DEADLINE=time.monotonic()+1100;CLEAN_DEADLINE=None
os.umask(0o077)
def need(ok):
 if not ok:raise RuntimeError('selected AMD helpers refused')
def interrupted(*_):raise RuntimeError('selected AMD helpers interrupted')
for sig in (signal.SIGTERM,signal.SIGINT,signal.SIGHUP):signal.signal(sig,interrupted)
def digest(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
def material(path,limit=536870912):
 p=pathlib.Path(path);need(p.is_absolute() and p.resolve()==p)
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  st=os.fstat(fd);need(stat.S_ISREG(st.st_mode) and st.st_nlink>=1 and 0<=st.st_size<=limit)
  row={k:getattr(st,k) for k in F};parts=[];total=0
  while True:
   data=os.read(fd,min(1048576,limit+1-total))
   if not data:break
   total+=len(data);need(total<=st.st_size and total<=limit);parts.append(data)
  need(total==st.st_size and row=={k:getattr(os.fstat(fd),k) for k in F}=={k:getattr(p.lstat(),k) for k in F})
  raw=b''.join(parts);return dict(row,sha256=hashlib.sha256(raw).hexdigest()),raw
 finally:os.close(fd)
def hold(path,pin=None,limit=536870912):
 p=str(pathlib.Path(path));row,raw=material(p,limit)
 if pin is not None:need(row==pin if 'st_dev' in pin else digest(raw)==pin)
 need(p not in HELD or HELD[p]==row);HELD[p]=row;return raw
def fence():
 for p,row in HELD.items():need(material(p)[0]==row)
def original(row):
 need(isinstance(row,dict) and set(row)=={'path','material','sha256','bytes'} and isinstance(row['material'],list) and len(row['material'])==9)
 need(re.fullmatch('[0-9a-f]{64}',row['sha256']) is not None)
 z={k:int(v) for k,v in zip(F,row['material'])};need(z['st_size']==row['bytes']);return dict(z,sha256=row['sha256'])
def old_file(row):
 p=pathlib.Path(row['path']);need(p.is_relative_to(BASE/'core') or p.is_relative_to(BASE/'source'));return hold(p,original(row))
def write_new(path,raw,mode=0o600):
 p=pathlib.Path(path);fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,mode)
 try:
  at=0
  while at<len(raw):n=os.write(fd,raw[at:]);need(n>0);at+=n
  os.fsync(fd)
 finally:os.close(fd)
 os.chmod(p,mode)
def save(leaf,obj):
 need(OUT_CREATED and isinstance(leaf,str) and re.fullmatch('[A-Za-z0-9_.-]{1,160}',leaf) and leaf not in ('.','..'))
 write_new(OUT/leaf,(json.dumps(obj,sort_keys=True)+'\n').encode())
def alive(pid):
 try:os.killpg(pid,0);return True
 except ProcessLookupError:return False
def stop(q):
 for sig,seconds in ((signal.SIGTERM,2),(signal.SIGKILL,2)):
  if alive(q.pid):
   try:os.killpg(q.pid,sig)
   except ProcessLookupError:pass
  until=time.monotonic()+seconds
  while time.monotonic()<until and (q.poll() is None or alive(q.pid)):time.sleep(.025)
 need(q.poll() is not None and not alive(q.pid))
def run(name,argv,timeout=60,expected=0,limit=33554432,cwd=None,cleanup=False,capture=True):
 global COUNTER,ACTIVE
 need(OUT_CREATED);COUNTER+=1;label=f'{COUNTER:03d}-{name}';need(re.fullmatch('[A-Za-z0-9_.-]+',label))
 budget=(CLEAN_DEADLINE if cleanup else DEADLINE)-time.monotonic();need(budget>0);timeout=min(timeout,budget)
 if cleanup:
  for p in ('/usr/bin/docker','/usr/bin/sudo'):need(material(p)[0]==HELD[p])
 else:fence()
 save(label+'-argv.private.json',argv);start=time.monotonic()
 def file_limit():resource.setrlimit(resource.RLIMIT_FSIZE,(limit,limit))
 with (OUT/(label+'-stdout.private.log')).open('xb') as out,(OUT/(label+'-stderr.private.log')).open('xb') as err:
  q=subprocess.Popen(argv,stdout=out,stderr=err,stdin=subprocess.DEVNULL,cwd=cwd,env=ENV,start_new_session=True,preexec_fn=file_limit);ACTIVE=q
  try:
   while q.poll() is None:
    need(time.monotonic()-start<timeout and out.tell()<=limit and err.tell()<=limit);time.sleep(.025)
   need(not alive(q.pid));out.flush();err.flush();os.fsync(out.fileno());os.fsync(err.fileno())
  except BaseException:
   try:stop(q)
   except BaseException:pass
   raise
  finally:
   if q.poll() is not None and not alive(q.pid):ACTIVE=None
  need(q.returncode is not None and (expected is None or q.returncode==expected))
 stdout_path=OUT/(label+'-stdout.private.log');stderr=material(OUT/(label+'-stderr.private.log'),min(limit,33554432))[1];raw=material(stdout_path,limit)[1] if capture else stdout_path;need(stdout_path.stat().st_size<=limit and len(stderr)<=limit)
 save(label+'-exit.json',{'step':name,'exitCode':q.returncode,'elapsedMs':int((time.monotonic()-start)*1000)})
 if not cleanup:fence()
 return raw,stderr,q.returncode
def git(name,args,limit=16777216):
 raw,err,code=run(name,['/usr/bin/git','--no-optional-locks','-C',str(R),*args],90,limit=limit);need(not err and code==0);return raw
def stable_new(name,pins):
 start=time.monotonic();previous=None;count=0;since=start;observed=[]
 while True:
  table={str(p):material(p)[0] for p in pins}
  for p,known in pins.items():need({'sha256':table[str(p)]['sha256'],'bytes':table[str(p)]['st_size']}==known)
  now=time.monotonic();observed.append({'elapsedMs':int((now-start)*1000),'table':table})
  if table==previous:count+=1
  else:previous=table;count=1;since=now
  need(now-start<30)
  if count>=3 and now-since>=2:break
  time.sleep(1.05)
 for p,row in table.items():hold(p,row)
 save(name+'-first-admission.private.json',{'observations':observed,'historicalSourceFullNineRecovered':False});return table

WORKER=b'#!/usr/bin/perl\n# Root-only source proposal. The author has not run Perl or any project program.\nuse strict;\nuse warnings;\nuse Fcntl qw(:DEFAULT :mode);\nuse POSIX qw(setsid WNOHANG);\nuse Cwd qw(abs_path);\nuse Time::HiRes qw(clock_gettime CLOCK_MONOTONIC sleep);\nuse JSON::PP ();\nuse Digest::SHA ();\nuse Text::ParseWords qw(shellwords);\nuse IO::Handle ();\nuse IO::Select ();\n\numask(0077);\nmy $J=JSON::PP->new->canonical(1)->ascii(1);\nmy ($yes,$no)=(JSON::PP::true,JSON::PP::false);\nsub now {return clock_gettime(CLOCK_MONOTONIC);}\nmy $deadline=now()+590;\nmy $active;\nmy (%held,%logical_guards,@steps,@observations);\nmy $expected;\nmy %counts=(independentMCommands=>0,compiledTranslationUnits=>0,linkCommands=>0,loaderCommands=>0,argumentRefusals=>0);\nmy $file_limit=536870912;\nmy $stream_limit=16777216;\nmy $out=\'/outputs\';\nmy $output_admitted=0;\nmy $stage=\'startup\';\n$SIG{TERM}=sub{die "worker interrupted\\n";};\n$SIG{INT}=sub{die "worker interrupted\\n";};\n$SIG{HUP}=sub{die "worker interrupted\\n";};\n\nsub need {die "Selected helpers worker refused\\n" unless $_[0];}\nsub tick {need(now()<$deadline);}\nsub same {return $J->encode($_[0]) eq $J->encode($_[1]);}\nsub keys_are {my ($h,@k)=@_;return ref($h) eq \'HASH\' && join(\'|\',sort keys %$h) eq join(\'|\',sort @k);}\nsub leaf {return defined($_[0])&&!ref($_[0])&&$_[0]=~/\\A[A-Za-z0-9_.-]{1,160}\\z/&&$_[0] ne \'.\'&&$_[0] ne \'..\';}\nsub fd_path {return \'/proc/\'.$$.\'/fd/\'.fileno($_[0]);}\nsub stop_group {\n my ($pid)=@_;return 1 unless defined($pid);\n local $SIG{TERM}=\'IGNORE\';local $SIG{INT}=\'IGNORE\';local $SIG{HUP}=\'IGNORE\';\n my $waiting=waitpid($pid,WNOHANG)==0;\n kill(\'TERM\',-$pid);kill(\'TERM\',$pid) if $waiting;\n my $until=now()+0.5;\n while(now()<$until&&($waiting||kill(0,-$pid))){$waiting=0 if $waiting&&waitpid($pid,WNOHANG)!=0;sleep(0.025);}\n kill(\'KILL\',-$pid) if kill(0,-$pid);kill(\'KILL\',$pid) if $waiting;\n $until=now()+0.5;\n while($waiting&&now()<$until){$waiting=0 if waitpid($pid,WNOHANG)!=0;sleep(0.025) if $waiting;}\n return !$waiting&&!kill(0,-$pid);\n}\nsub child_exec {\n my ($argv,$stdout,$stderr)=@_;\n need(ref($argv) eq \'ARRAY\'&&@$argv&&$argv->[0]=~m{\\A/});\n for(@$argv){need(defined($_)&&!ref($_)&&index($_,"\\0")==-1);}\n my $pid=fork();need(defined($pid));\n if(!$pid){\n  eval {\n   setsid()!=-1 or die;\n   chdir($out) or die;\n   open(STDIN,\'<\',\'/dev/null\') or die;\n   open(STDOUT,\'>&\',$stdout) or die;\n   open(STDERR,\'>&\',$stderr) or die;\n   exec {$argv->[0]} @$argv or die;\n  };\n  POSIX::_exit(126);\n }\n $active=$pid;return $pid;\n}\n\n# GNU stat supplies exact timestamps as strings. Never use Perl\'s rounded stat\n# timestamps as an authority. Both rows use the worker PID for its open FD.\nsub stat_pair {\n my ($fh,$path)=@_;tick();\n pipe(my $reader,my $writer) or die "Selected helpers worker refused\\n";\n sysopen(my $err,\'/dev/null\',O_WRONLY) or die "Selected helpers worker refused\\n";\n my $pid=child_exec([\'/usr/bin/stat\',\'-Lc\',\'%d|%i|%f|%h|%u|%g|%s|%y|%z\',\'--\',fd_path($fh),$path],$writer,$err);\n close($writer);close($err);my $raw=\'\';my $ok;\n eval {\n  my $select=IO::Select->new($reader);my $until=now()+10;\n  while(1){tick();need(now()<$until);next unless $select->can_read(0.1);my $n=sysread($reader,my $b,2048);need(defined($n));last unless $n;$raw.=$b;need(length($raw)<4096);}\n  my ($wait,$status);while(1){tick();need(now()<$until);$wait=waitpid($pid,WNOHANG);need($wait>=0);if($wait==$pid){$status=$?;last;}sleep(0.001);}\n  need($status==0&&!kill(0,-$pid));$ok=1;\n };\n my $error=$@;close($reader);my $stopped=$ok?1:stop_group($pid);$active=undef if $stopped;die $error unless $ok;\n my $removed=($raw=~s/\\n\\z//);need($removed==1&&$raw!~/\\r/);\n my @lines=split(/\\n/,$raw,-1);need(@lines==2);\n my @rows;\n for my $line(@lines){my @r=split(/\\|/,$line,-1);need(@r==9);for(@r){need(length($_)>0&&length($_)<256&&$_!~/[\\r\\n]/);}push @rows,\\@r;}\n need(same($rows[0],$rows[1]));return $rows[0];\n}\nsub read_regular {\n my ($logical,$capture,$limit)=@_;tick();$limit//=$file_limit;\n need(defined($logical)&&!ref($logical)&&$logical=~m{\\A/}&&$logical!~/[\\r\\n\\0]/);\n my $path=abs_path($logical);need(defined($path)&&$path=~m{\\A/(?:work|old|baseline|outputs|controls|inputs|usr|lib|lib64)(?:/|\\z)});\n sysopen(my $fh,$path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK) or die "Selected helpers worker refused\\n";\n my @s=stat($fh);my @p=lstat($path);need(@s&&@p&&S_ISREG($s[2])&&S_ISREG($p[2])&&$s[7]<=$limit);\n my $before=stat_pair($fh,$path);my ($bytes,$body)=(0,\'\');my $sha=Digest::SHA->new(256);\n while(1){tick();my $n=sysread($fh,my $b,65536);need(defined($n));last unless $n;$bytes+=$n;need($bytes<=$limit&&$bytes<=$s[7]);$sha->add($b);$body.=$b if $capture;}\n need($bytes==$s[7]&&same($before,stat_pair($fh,$path))&&abs_path($logical) eq $path);\n close($fh) or die "Selected helpers worker refused\\n";\n my $row={material=>$before,bytes=>0+$bytes,sha256=>$sha->hexdigest};\n return ($path,$row,$body);\n}\nsub material {my (undef,$r)=read_regular($_[0],0);return $r;}\nsub read_bytes {my (undef,undef,$b)=read_regular($_[0],1,$_[1]//$stream_limit);return $b;}\nsub hold {\n my ($path)=@_;my ($real,$row)=read_regular($path,0);\n need(!exists($held{$real})||same($held{$real},$row));need(!exists($logical_guards{$path})||$logical_guards{$path} eq $real);$logical_guards{$path}=$real;$held{$real}=$row;return $row;\n}\nsub fence {for my $p(sort keys %logical_guards){my $r=abs_path($p);need(defined($r)&&$r eq $logical_guards{$p});}for my $p(sort keys %held){need(same($held{$p},material($p)));}}\nsub save {\n my ($name,$value)=@_;need($output_admitted&&leaf($name));my $body=$J->encode($value)."\\n";\n sysopen(my $fh,$out.\'/\'.$name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Selected helpers worker refused\\n";\n my $at=0;while($at<length($body)){tick();my $n=syswrite($fh,$body,length($body)-$at,$at);need(defined($n)&&$n>0);$at+=$n;}\n $fh->sync or die "Selected helpers worker refused\\n";close($fh) or die "Selected helpers worker refused\\n";\n}\nsub call {\n my ($name,$argv,$wanted)=@_;$wanted//=0;need(leaf($name));tick();\n save($name.\'-argv.private.json\',$argv);\n sysopen(my $stdout,$out.\'/\'.$name.\'-stdout.private.log\',O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Selected helpers worker refused\\n";\n sysopen(my $stderr,$out.\'/\'.$name.\'-stderr.private.log\',O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Selected helpers worker refused\\n";\n my $start=now();my $pid=child_exec($argv,$stdout,$stderr);my ($status,$done,$ok);\n eval {\n  while(1){\n   tick();need(now()-$start<179);\n   need((stat($stdout))[7]<=$stream_limit&&(stat($stderr))[7]<=$stream_limit);\n   my $w=waitpid($pid,WNOHANG);need($w>=0);\n   if($w==$pid){$status=$?;$done=1;last;}sleep(0.025);\n  }\n  need(!kill(0,-$pid));$ok=1;\n };\n my $error=$@;my $stopped=$ok?1:stop_group($pid);$active=undef if $stopped;\n $stdout->sync; $stderr->sync;close($stdout);close($stderr);\n die $error unless $ok;\n need($done&&($status&127)==0);my $exit=$status>>8;\n my $event={step=>$name,exitCode=>$exit,elapsedMs=>int((now()-$start)*1000)};\n push @steps,$event;save($name.\'-exit.json\',$event);need($exit==$wanted);\n return read_bytes($out.\'/\'.$name.\'-stdout.private.log\');\n}\nsub deps {\n my ($raw)=@_;need($raw!~/\\0/);$raw=~s/\\\\\\n/ /g;need($raw=~s/\\A[^:\\n]+://);\n my @words=shellwords($raw);need(@words>0);my %unique;\n for(@words){need(m{\\A/}&&!/[\\r\\n]/);$unique{$_}=1;}\n return [sort keys %unique];\n}\n# Dependency comparisons are lexical and explicitly remapped only for retained MD.\n# The current -M spellings must also resolve to the same file after normalization.\nsub normal_path {\n my ($path)=@_;need(defined($path)&&!ref($path)&&$path=~m{\\A/}&&$path!~/[\\0\\r\\n]/);\n my @parts;for my $part(split(m{/+},$path)){next if $part eq \'\'||$part eq \'.\';if($part eq \'..\'){need(@parts);pop @parts;}else{push @parts,$part;}}\n return \'/\'.join(\'/\',@parts);\n}\nsub normalized_deps {\n my ($names,$map,$current)=@_;my %found;\n for my $name(@$names){my $p=$name;\n  for my $entry(@$map){my ($from,$to)=@{$entry}{qw(from to)};if($p eq $from||index($p,$from.\'/\')==0){$p=$to.substr($p,length($from));last;}}\n  my $n=normal_path($p);\n  if($current){my ($a,$b)=(abs_path($p),abs_path($n));need(defined($a)&&defined($b)&&$a eq $b);}\n  $found{$n}=1;\n }\n return [sort keys %found];\n}\nsub pin_shape {\n my ($pin,$with_path)=@_;need(keys_are($pin,$with_path?qw(path sha256 bytes):qw(sha256 bytes)));\n need(defined($pin->{sha256})&&!ref($pin->{sha256})&&$pin->{sha256}=~/\\A[0-9a-f]{64}\\z/);\n need(defined($pin->{bytes})&&!ref($pin->{bytes})&&$pin->{bytes}=~/\\A[1-9][0-9]*\\z/&&$pin->{bytes}<=$file_limit);\n if($with_path){need(defined($pin->{path})&&!ref($pin->{path})&&$pin->{path}=~m{\\A/inputs/}&&normal_path($pin->{path}) eq $pin->{path});}\n}\nsub matches_pin {my ($row,$pin)=@_;return $row->{sha256} eq $pin->{sha256}&&$row->{bytes}==$pin->{bytes};}\nsub stable_paths {\n my ($name,$paths)=@_;my $start=now();my ($previous,$series,$count,$table)=(undef,$start,0,undef);my %resolutions;\n while(1){\n  $table={};%resolutions=();\n  for my $p(@$paths){my ($real,$row)=read_regular($p,0);need(!exists($held{$real})||same($held{$real},$row));\n   need(!exists($logical_guards{$p})||$logical_guards{$p} eq $real);$resolutions{$p}=$real;\n   if($real=~m{\\A/(?:work|old)/}){need(exists($expected->{$real})&&matches_pin($row,$expected->{$real}));}\n   else{need($real=~m{\\A/(?:inputs|baseline|outputs|controls|usr|lib|lib64)/});}\n   $table->{$real}=$row;\n  }\n  my $current=now();need($current-$start<30);push @observations,{step=>$name,elapsedMs=>int(($current-$start)*1000),table=>$table,resolvedPaths=>{%resolutions}};\n  my $sample={table=>$table,resolvedPaths=>{%resolutions}};\n  if(defined($previous)&&same($previous,$sample)){++$count;}else{$count=1;$series=$current;}\n  last if $count>=3&&$current-$series>=2;$previous=$sample;sleep(1);\n }\n for my $p(keys %$table){need(!exists($held{$p})||same($held{$p},$table->{$p}));$held{$p}=$table->{$p};}\n for my $p(keys %resolutions){need(!exists($logical_guards{$p})||$logical_guards{$p} eq $resolutions{$p});$logical_guards{$p}=$resolutions{$p};}\n return $table;\n}\nsub stable_new {\n my ($name,$paths,$mode,$uid,$gid)=@_;my %known;\n for my $p(@$paths){need(!exists($held{$p})&&abs_path($p) eq $p);need(chmod($mode,$p)==1);my $r=material($p);$known{$p}={sha256=>$r->{sha256},bytes=>$r->{bytes}};}\n my $start=now();my ($previous,$series,$count,$table)=(undef,$start,0,undef);\n while(1){$table={};\n  for my $p(@$paths){my @s=lstat($p);need(@s&&S_ISREG($s[2])&&$s[3]==1&&$s[4]==$uid&&$s[5]==$gid&&($s[2]&07777)==$mode);\n   my $r=material($p);need(matches_pin($r,$known{$p}));$table->{$p}=$r;}\n  my $current=now();need($current-$start<30);\n  push @observations,{step=>$name,elapsedMs=>int(($current-$start)*1000),table=>$table};\n  if(defined($previous)&&same($previous,$table)){++$count;}else{$count=1;$series=$current;}\n  last if $count>=3&&$current-$series>=2;$previous=$table;sleep(1);\n }\n for my $p(@$paths){need(same($table->{$p},material($p)));$held{$p}=$table->{$p};$logical_guards{$p}=$p;}\n return $table;\n}\nsub checked_call {my ($name,$argv,$wanted)=@_;fence();my $body=call($name,$argv,$wanted);fence();return $body;}\nsub flags_ok {\n my ($argv)=@_;need(ref($argv) eq \'ARRAY\'&&@$argv>=1&&@$argv<=512&&$argv->[0] eq \'/usr/bin/clang++\');\n for my $word(@$argv){need(defined($word)&&!ref($word)&&length($word)<=8192&&$word!~/[\\0\\r\\n]/&&$word!~/\\A@/);\n  need($word!~/\\A(?:-c|-o|-M|-MM|-MD|-MMD|-MP|-MF|-MT|-MQ|-MJ|-E|-S|-fsyntax-only)\\z/&&$word!~/\\A(?:-MF|-MT|-MQ|-MJ).+/);\n  need($word!~/\\A(?:-save-temps|-ftime-trace|-fprofile|-fmodules-cache-path|-serialize-diagnostics|-fplugin|-load)/&&$word!~/(?:^|,)@/);\n }\n}\nsub dependency_map {\n my ($map)=@_;need(ref($map) eq \'ARRAY\'&&@$map<=2);my %seen;\n for my $e(@$map){need(keys_are($e,qw(from to))&&defined($e->{from})&&defined($e->{to})&&!ref($e->{from})&&!ref($e->{to}));\n  need(($e->{from} eq \'/overlay\'&&$e->{to} eq \'/work\')||($e->{from} eq \'/outputs/third_party\'&&$e->{to} eq \'/old/third_party\'));need(!$seen{$e->{from}}++);\n }\n}\nsub elf_check {\n my ($path,$machine)=@_;my $raw=read_bytes($path,$file_limit);need(length($raw)>=64&&substr($raw,0,7) eq "\\x7fELF\\x02\\x01\\x01");\n my ($type,$em,$version)=unpack(\'vvV\',substr($raw,16,8));need(($type==2||$type==3)&&$em==$machine&&$version==1);\n need(unpack(\'v\',substr($raw,52,2))==64);\n}\nsub loader_paths {\n my ($text)=@_;need(length($text)&&$text!~/not found/);my %paths;\n for my $line(split(/\\n/,$text)){next if $line=~/\\A\\s*linux-vdso(?:[.]so)?[^\\s]*\\s+\\(0x[0-9a-f]+\\)\\s*\\z/;\n  my $p;if($line=~/\\A\\s*[^\\s]+\\s+=>\\s+(\\/[^\\s]+)\\s+\\(0x[0-9a-f]+\\)\\s*\\z/){$p=$1;}\n  elsif($line=~/\\A\\s*(\\/[^\\s]+)\\s+\\(0x[0-9a-f]+\\)\\s*\\z/){$p=$1;}else{need(0);}\n  need($p=~m{\\A/(?:old|usr|lib|lib64)/});$paths{$p}=1;\n }\n need(keys(%paths)>0);return [sort keys %paths];\n}\nsub json_refusal {\n my ($role,$stdout,$stderr)=@_;need($stderr eq \'\');my $got=eval{$J->decode($stdout)};need(!$@&&ref($got) eq \'HASH\');my $want;\n if($role eq \'bootstrapImport\'){$want={format=>\'nano-isolated-fixture-private-binding-audit-v1\',passed=>$no,refusedStage=>\'arguments\',errorDetailsSuppressed=>$yes,newSignaturesCreated=>0,signerCallbacksInvoked=>0,runtimeStarted=>$no,networkLaunched=>$no,productionQualified=>$no};}\n elsif($role eq \'bootstrapAudit\'){$want={format=>\'nano-isolated-bootstrap-physical-audit-v1\',passed=>$no,refusedStage=>\'arguments\',diagnostic=>\'expected manifest plan anchors source-inventory owner dataRoot witnessRoot\',newSignaturesCreated=>0,journalWritesRequested=>$no,runtimeStarted=>$no,productionQualified=>$no};}\n elsif($role eq \'stoppedCapture\'){$want={format=>\'nano-isolated-stopped-recovery-pins-v1\',passed=>$no,refusedStage=>\'arguments\',diagnostic=>\'usage: --capture public-manifest public-runtime-config external-stop-receipt dataOwnerRoot witnessOwnerRoot owner\',newSignaturesCreated=>0,journalWritesRequested=>$no,runtimeStarted=>$no,productionQualified=>$no};}\n else{need(0);}need(same($got,$want));\n}\nsub main {\n need(@ARGV==0);\n # No inherited fixture, key, loader, compiler-control, or runtime variables.\n %ENV=(PATH=>\'/usr/bin:/bin\',HOME=>\'/outputs\',TMPDIR=>\'/tmp\',LANG=>\'C\',LC_ALL=>\'C\');\n my $recipe=$J->decode(read_bytes(\'/controls/recipes.json\'));$expected=$J->decode(read_bytes(\'/controls/source-expected.private.json\'));my $core_pin=$J->decode(read_bytes(\'/controls/core-pin.json\'));\n need(keys_are($recipe,qw(format owner uid gid imageId elfMachine programs))&&$recipe->{format} eq \'nano-current-linux-selected-helpers-recipes-v1\'&&ref($expected) eq \'HASH\');pin_shape($core_pin,0);\n my ($uid,$gid)=@{$recipe}{qw(uid gid)};\n my $arm=$recipe->{owner} eq \'mac-docker\';my $amd=$recipe->{owner} eq \'vps-docker\';need($arm||$amd);\n need($arm?($uid==501&&$gid==20&&$recipe->{elfMachine}==183&&$recipe->{imageId} eq \'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb\'):($uid==1000&&$gid==1000&&$recipe->{elfMachine}==62&&$recipe->{imageId} eq \'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14\'));\n need($>==$uid&&$<==$uid&&abs_path($out) eq $out);my @s=lstat($out);need(@s&&S_ISDIR($s[2])&&($s[2]&07777)==0700&&$s[4]==$uid&&$s[5]==$gid);\n opendir(my $dir,$out) or die "Selected helpers worker refused\\n";my @existing=grep {$_ ne \'.\'&&$_ ne \'..\'} readdir($dir);closedir($dir);need(!@existing);$output_admitted=1;\n need(ref($recipe->{programs}) eq \'ARRAY\'&&@{$recipe->{programs}}>=1&&@{$recipe->{programs}}<=($arm?5:4));\n my %sources=(bootstrapImport=>\'/work/tests/capella/isolated_bootstrap_import.cpp\',bootstrapAudit=>\'/work/tests/capella/isolated_bootstrap_audit.cpp\',stoppedCapture=>\'/work/tests/capella/isolated_stopped_capture.cpp\',lifecycleTool=>\'/work/tests/capella/deposit_audit.cpp\',lifecycleSubmit=>\'/work/tests/multihost/lifecycle_submit.cpp\');\n my %targets=(bootstrapImport=>\'nano-capella-isolated-bootstrap-import\',bootstrapAudit=>\'nano-capella-isolated-bootstrap-audit\',stoppedCapture=>\'nano-capella-isolated-stopped-capture\',lifecycleTool=>\'nano-capella-deposit-audit\',lifecycleSubmit=>\'nano-capella-lifecycle-submit\');\n my (%seen,@initial);need(keys(%$expected)>0&&keys(%$expected)<=20000);\n for my $p(sort keys %$expected){need($p=~m{\\A/(?:work|old)/}&&normal_path($p) eq $p&&abs_path($p) eq $p);pin_shape($expected->{$p},0);}\n for my $program(@{$recipe->{programs}}){need(keys_are($program,qw(role source compileFlags linkPrefix linkTail reuse)));my $role=$program->{role};need(defined($role)&&!ref($role)&&exists($sources{$role})&&!$seen{$role}++&&($arm||$role ne \'bootstrapImport\')&&$program->{source} eq $sources{$role});\n  flags_ok($program->{compileFlags});flags_ok($program->{linkPrefix});my $tail=$program->{linkTail};need(ref($tail) eq \'ARRAY\'&&@$tail&&@$tail<=256&&grep($_ eq \'/baseline/libnano_core.a\',@$tail)==1);\n  for my $word(@$tail){need(defined($word)&&!ref($word)&&$word!~/[\\0\\r\\n]/&&$word!~/(?:^|,)@/&&$word=~m{\\A(?:/baseline/libnano_core[.]a\\z|/(?:old|usr|lib|lib64)/|-)});need($word!~/\\A(?:-o|-c|-M|-MM|-MD|-MMD|-MF|-MT|-MQ|-MJ)\\z/);push @initial,$word if $word=~m{\\A/};}\n  need(exists($expected->{$program->{source}}));push @initial,$program->{source};\n  my @macro=grep {index($_,\'-DNANO_TEST_SOURCE=\')==0} @{$program->{compileFlags}};\n  if($role eq \'lifecycleTool\'){need(@macro==1&&$macro[0] eq \'-DNANO_TEST_SOURCE="/work"\');}else{need(!@macro);}\n  need(!grep {index($_,\'NANO_SYNC_FAULT\')>=0} @{$program->{compileFlags}});\n  if(defined($program->{reuse})){my $r=$program->{reuse};need(keys_are($r,qw(object md dependencyPrefixMap)));pin_shape($r->{object},1);pin_shape($r->{md},1);dependency_map($r->{dependencyPrefixMap});need($r->{object}{path} ne $r->{md}{path});\n   for my $pin($r->{object},$r->{md}){need(matches_pin(material($pin->{path}),$pin));push @initial,$pin->{path};}\n  }\n }\n push @initial,\'/controls/recipes.json\',\'/controls/source-expected.private.json\',\'/controls/core-pin.json\',\'/controls/worker.pl\',\'/usr/bin/clang++\',\'/usr/bin/stat\',\'/usr/bin/ldd\',\'/baseline/libnano_core.a\';\n my %once;@initial=grep {!$once{$_}++} @initial;$stage=\'initial-input-admission\';stable_paths(\'initial-inputs\',\\@initial);fence();\n my $core=$held{\'/baseline/libnano_core.a\'};need(defined($core)&&matches_pin($core,$core_pin));\n my @results;\n for my $program(@{$recipe->{programs}}){my $role=$program->{role};my $source=$program->{source};my $flags=$program->{compileFlags};my $reuse=$program->{reuse};my $object=defined($reuse)?$reuse->{object}{path}:$out.\'/\'.$role.\'.cpp.o\';my $md=defined($reuse)?$reuse->{md}{path}:$object.\'.d\';my $target=$out.\'/\'.$role.\'.cpp.o\';\n  $stage=$role.\'-independent-m\';++$counts{independentMCommands};my $m=checked_call($role.\'-independent-m\',[@$flags,\'-M\',\'-MT\',$target,\'-o\',\'/dev/stdout\',$source]);my $literal=deps($m);my $normalized=normalized_deps($literal,[],1);need(grep($_ eq $source,@$normalized)==1);\n  stable_paths($role.\'-compile-dependencies\',$normalized);fence();my ($literal_equal,$normalized_equal,$mapped_equal,$previous_literal,$previous_normalized,$previous_mapped)=(undef,undef,undef,undef,undef,undef);\n  if(defined($reuse)){\n   need(matches_pin(material($object),$reuse->{object})&&matches_pin(material($md),$reuse->{md}));$previous_literal=deps(read_bytes($md));$previous_normalized=normalized_deps($previous_literal,[],0);$previous_mapped=normalized_deps($previous_literal,$reuse->{dependencyPrefixMap},0);\n   $literal_equal=same($literal,$previous_literal)?$yes:$no;$normalized_equal=same($normalized,$previous_normalized)?$yes:$no;$mapped_equal=same($normalized,$previous_mapped)?$yes:$no;need($mapped_equal);\n  }else{\n   $stage=$role.\'-compile\';++$counts{compiledTranslationUnits};checked_call($role.\'-compile\',[@$flags,\'-MD\',\'-MF\',$md,\'-MT\',$object,\'-o\',$object,\'-c\',$source]);\n   stable_new($role.\'-object-and-md\',[$object,$md],0400,$uid,$gid);my $actual=deps(read_bytes($md));$previous_literal=$actual;$previous_normalized=normalized_deps($actual,[],1);$previous_mapped=$previous_normalized;\n   $literal_equal=same($literal,$actual)?$yes:$no;$normalized_equal=same($normalized,$previous_normalized)?$yes:$no;$mapped_equal=$normalized_equal;need($normalized_equal);\n  }\n  my $object_row=hold($object);my $md_row=hold($md);fence();my $exe=$out.\'/\'.$targets{$role};$stage=$role.\'-link\';++$counts{linkCommands};checked_call($role.\'-link\',[@{$program->{linkPrefix}},$object,\'-o\',$exe,@{$program->{linkTail}}]);\n  my $new=stable_new($role.\'-software\',[$exe],0500,$uid,$gid);my $exe_row=$new->{$exe};elf_check($exe,$recipe->{elfMachine});fence();\n  $stage=$role.\'-loader\';++$counts{loaderCommands};my $ldd=checked_call($role.\'-ldd\',[\'/usr/bin/ldd\',$exe]);need(read_bytes($out.\'/\'.$role.\'-ldd-stderr.private.log\') eq \'\');my $runtime=loader_paths($ldd);stable_paths($role.\'-runtime-inputs\',$runtime);fence();\n  $stage=$role.\'-arguments\';++$counts{argumentRefusals};my $stdout=checked_call($role.\'-arguments\',[$exe],1);my $stderr=read_bytes($out.\'/\'.$role.\'-arguments-stderr.private.log\');\n  if($role eq \'lifecycleTool\'){need($stdout eq \'\'&&$stderr eq "usage: --audit synthetic-genesis node-config stopped-db external-audit-request deposit-metadata\\n");}\n  elsif($role eq \'lifecycleSubmit\'){need($stdout eq \'\'&&$stderr eq "{\\"event\\":\\"fresh-lifecycle-child-submission-refused\\",\\"accepted\\":false,\\"detailsExported\\":false,\\"productionQualified\\":false}\\n");}\n  else{json_refusal($role,$stdout,$stderr);}fence();need(same($exe_row,material($exe))&&same($object_row,material($object))&&same($md_row,material($md)));\n  push @results,{role=>$role,target=>$targets{$role},source=>$source,objectReused=>defined($reuse)?$yes:$no,dependencyMode=>\'MD\',currentMLiteralDependencies=>scalar(@$literal),currentMNormalizedDependencies=>scalar(@$normalized),comparisonMdLiteralDependencies=>scalar(@$previous_literal),comparisonMdNormalizedDependencies=>scalar(@$previous_normalized),comparisonMdRemappedDependencies=>scalar(@$previous_mapped),literalMEqualsMd=>$literal_equal,normalizedMEqualsMd=>$normalized_equal,normalizedMEqualsRemappedMd=>$mapped_equal,retainedDependencyPrefixMap=>defined($reuse)?$reuse->{dependencyPrefixMap}:[],object=>$object_row,dependencyFile=>$md_row,software=>$exe_row,softwarePath=>$exe,elfMachine=>0+$recipe->{elfMachine},loaderPassed=>$yes,argumentRefusalPassed=>$yes,independentMCommands=>1,compiledTranslationUnits=>defined($reuse)?0:1,linkCommands=>1,loaderCommands=>1,argumentRefusals=>1};\n }\n $stage=\'final-fences\';fence();save(\'sdk-and-inputs.private.json\',\\%held);save(\'first-dependency-observations.private.json\',\\@observations);\n save(\'software-admission.private.json\',{format=>\'nano-current-linux-selected-helper-software-binding-v1\',owner=>$recipe->{owner},imageId=>$recipe->{imageId},coreArchive=>$core,programs=>\\@results,guestMaterials=>\\%held,logicalResolutionGuards=>\\%logical_guards,originalGuestInputsFullNineAndShaUnchanged=>$yes,retainedHostNineRecovered=>$no,oldAuthoritiesRefreshed=>$no});\n my $binding=material(\'/outputs/software-admission.private.json\');my @public=map {my $p=$_;{role=>$p->{role},target=>$p->{target},objectReused=>$p->{objectReused},dependencyMode=>$p->{dependencyMode},currentMLiteralDependencies=>$p->{currentMLiteralDependencies},currentMNormalizedDependencies=>$p->{currentMNormalizedDependencies},comparisonMdLiteralDependencies=>$p->{comparisonMdLiteralDependencies},comparisonMdNormalizedDependencies=>$p->{comparisonMdNormalizedDependencies},comparisonMdRemappedDependencies=>$p->{comparisonMdRemappedDependencies},literalMEqualsMd=>$p->{literalMEqualsMd},normalizedMEqualsMd=>$p->{normalizedMEqualsMd},normalizedMEqualsRemappedMd=>$p->{normalizedMEqualsRemappedMd},retainedDependencyPrefixMap=>$p->{retainedDependencyPrefixMap},objectSha256=>$p->{object}{sha256},objectBytes=>$p->{object}{bytes},softwareSha256=>$p->{software}{sha256},softwareBytes=>$p->{software}{bytes},loaderPassed=>$p->{loaderPassed},argumentRefusalPassed=>$p->{argumentRefusalPassed}}} @results;\n my $result={format=>\'nano-current-linux-selected-helpers-qualification-v1\',passed=>$yes,owner=>$recipe->{owner},uid=>0+$uid,gid=>0+$gid,imageId=>$recipe->{imageId},imageIdentityVerifiedByWorker=>$no,elfMachine=>0+$recipe->{elfMachine},selectedPrograms=>scalar(@results),%counts,programs=>\\@public,coreSha256=>$core->{sha256},coreBytes=>$core->{bytes},softwareAdmissionSha256=>$binding->{sha256},softwareAdmissionBytes=>$binding->{bytes},originalGuestInputsFullNineAndShaUnchanged=>$yes,retainedHostNineRecovered=>$no,steps=>\\@steps,coreArchiveMutated=>$no,coreMembersRequalified=>$no,canonicalCmakeQualifiedByWorker=>$no,fullLoaderClosureQualified=>$no,lazySymbolsQualified=>$no,wholeSdkQualified=>$no,newSignaturesCreated=>0,realBootstrapImports=>0,realExports=>0,databaseOpened=>$no,oldSuiteExecuted=>$no,dockerImageBuilt=>$no,nodeRuntimeStarted=>$no,networkStarted=>$no,dutiesResumed=>$no,commonFinalityQualified=>$no,campaignAdmission=>$no,oldAuthoritiesRefreshed=>$no,productionQualified=>$no};\n save(\'result-public.json\',$result);print $J->encode($result)."\\n" or die "Selected helpers worker refused\\n";\n}\nmy $ok=eval{main();1;};\nunless($ok){my $stopped=stop_group($active);local $SIG{TERM}=\'IGNORE\';local $SIG{INT}=\'IGNORE\';local $SIG{HUP}=\'IGNORE\';my $post=$stopped?eval{fence();1;}:0;eval{save(\'worker-refusal.private.json\',{passed=>$no,stage=>$stage,originalGuestInputsUnchanged=>$post?$yes:$no,ownProcessGroupCleanupComplete=>$stopped?$yes:$no,completedCommandEvents=>scalar(@steps),%counts});};print STDERR "Selected helpers worker refused\\n";exit(1);}\nexit(0);\n'

def main():
 global OUT,OUT_CREATED,CLEAN_DEADLINE
 parser=argparse.ArgumentParser();parser.add_argument('--output-root',required=True);a=parser.parse_args()
 need(sys.platform.startswith('linux') and os.getuid()==os.getgid()==1000)
 candidate=pathlib.Path(a.output_root);need(candidate.is_absolute() and candidate.parent.resolve()==candidate.parent and candidate.parent.is_dir() and re.fullmatch('current-amd-helpers-[a-z0-9-]{1,100}',candidate.name) and not candidate.exists() and not candidate.is_symlink())
 need(candidate.parent.stat().st_uid==1000 and not candidate.is_relative_to(R) and not candidate.is_relative_to(BASE) and not candidate.is_relative_to(CORE_SOURCE.parent))
 candidate.mkdir(mode=0o700);OUT=candidate;OUT_CREATED=True
 for leaf in ('source','controls','outputs','baseline','docker-client'):(OUT/leaf).mkdir(mode=0o700)
 # OS utilities receive a new, explicit present-time admission; no old SDK/tool map.
 tools={pathlib.Path(p):digest(material(p)[1]) for p in ('/usr/bin/git','/usr/bin/docker','/usr/bin/sudo')}
 stable_new('os-utilities',tools);hold(pathlib.Path(__file__).resolve())
 need(git('git-head',['rev-parse','HEAD']).decode().strip()==REV and git('git-clean',['status','--porcelain'])==b'')
 found={}
 raw=git('git-selected-trees',['ls-tree','-z',REV,'--',*TREES])
 for entry in raw.split(b'\0'):
  if not entry:continue
  meta,path=entry.split(b'\t',1);mode,kind,oid=meta.decode().split();key=path.decode();need(mode=='040000' and kind=='tree' and key not in found);found[key]=oid
 need(found==TREES)
 docs={key:json.loads(hold(BASE/rel,{'sha256':sha,'bytes':size},16777216)) for key,(rel,sha,size) in REFS.items()}
 need(set(docs['llvm'])=={'/outputs/third_party/mcl/bint64.o','/outputs/third_party/mcl/base64.o'})
 # Known current Core is copied once into a new file; no historical source-nine recovery.
 core_raw=hold(CORE_SOURCE,CORE_PIN);core=OUT/'baseline/libnano_core.a';write_new(core,core_raw,0o400);stable_new('current-core-copy',{core:CORE_PIN})
 need(os.statvfs(OUT).f_bavail*os.statvfs(OUT).f_frsize>=3*1024**3)
 wanted={}
 raw=git('git-selected-blobs',['ls-tree','-r','-z',REV,'--',*SELECTED])
 for entry in raw.split(b'\0'):
  if not entry:continue
  meta,path=entry.split(b'\t',1);mode,kind,oid=meta.decode().split();key=path.decode();p=pathlib.PurePosixPath(key)
  need(kind=='blob' and mode in ('100644','100755') and not p.is_absolute() and '..'not in p.parts and key not in wanted);wanted[key]=oid
 # Archive is bounded on disk by run/RLIMIT_FSIZE, then streamed without extractall.
 tar_path,err,code=run('git-archive',['/usr/bin/git','--no-optional-locks','-C',str(R),'archive',REV,*SELECTED],180,limit=1073741824,capture=False)
 need(not err and code==0);os.chmod(tar_path,0o400)
 expected={};seen=set();empty_seen=set();source=OUT/'source';total=0
 with tarfile.open(tar_path,'r:') as tf:
  for item in tf:
   rel=pathlib.PurePosixPath(item.name);need(not rel.is_absolute() and '..'not in rel.parts and (item.isdir()or item.isfile()))
   p=source/str(rel)
   if item.isdir():p.mkdir(parents=True,exist_ok=True);continue
   need(str(rel) in wanted and str(rel) not in seen and item.size<=33554432);seen.add(str(rel));total+=item.size;need(total<=1073741824)
   body=tf.extractfile(item).read(item.size+1);need(len(body)==item.size and hashlib.sha1(b'blob '+str(len(body)).encode()+b'\0'+body).hexdigest()==wanted[str(rel)])
   p.parent.mkdir(parents=True,exist_ok=True);write_new(p,body,0o444);guest='/work/'+str(rel)
   if not body:need(guest in EMPTY);empty_seen.add(guest)
   else:expected[guest]=digest(body)
 need(seen==set(wanted) and empty_seen==EMPTY)
 for p in sorted(source.rglob('*'),key=lambda p:len(p.parts),reverse=True):
  if p.is_dir():os.chmod(p,0o555)
 os.chmod(source,0o555)
 cmake=(source/'CMakeLists.txt').read_bytes();need(digest(cmake)==CMAKE_PIN)
 cmake_text=cmake.decode();need(expected[HEADER]==AFTER)
 # Fixed receipts bind actual producer rows. Only four selected consumers are used.
 mounts={};programs=[];joins={};orders={};recipe_info={}
 def mount(path,guest,row):
  p=pathlib.Path(path);need(p.is_relative_to(BASE/'core'));old_file(row)
  entry=(str(p),{'sha256':row['sha256'],'bytes':row['bytes']})
  need(guest not in mounts or mounts[guest]==entry);mounts[guest]=entry;expected[guest]=entry[1]
 def map_output(word):
  return '/baseline/libnano_core.a' if word=='/outputs/libnano_core.a' else '/old/'+word[len('/outputs/'):]
 all_shared=set()
 for role,(target,relative,project_count) in ROLES.items():
  need(cmake_text.count('add_executable('+target+' '+relative+')')==1 and cmake_text.count('target_link_libraries('+target+' PRIVATE nano_core)')==1)
  rec=docs['links'][target];need(set(rec)=={'compile','dependencyFile','executable','imageDependencies','linkRecipe','newSourceAndGeneratedDependencies','object'})
  need(rec['executable']==docs['artifacts']['files'][str(BASE/'core'/target)])
  comp=rec['compile'];need(comp['file']=='/work/'+relative and comp['directory']=='/outputs')
  argv=comp.get('arguments') or shlex.split(comp['command']);need(argv[0]=='/usr/bin/clang++' and argv[-2:]==['-c',comp['file']] and argv[-4]=='-o' and argv.count('-o')==argv.count('-c')==1)
  own=os.path.normpath(argv[-3] if argv[-3].startswith('/') else '/outputs/'+argv[-3]);need(own.startswith('/outputs/') and own.endswith('.cpp.o'))
  need(rec['object']['path']==str(BASE/'core'/own[len('/outputs/'):]) and rec['dependencyFile']['path']==str(BASE/'core'/(own[len('/outputs/'):]+'.d')))
  old_file(rec['object']);old_file(rec['dependencyFile']);flags=[w.replace('/outputs/third_party','/old/third_party') for w in argv[:-4]]
  macros=[w for w in flags if w.startswith('-DNANO_TEST_SOURCE=')]
  if role=='lifecycleTool':
   need(cmake_text.count('target_compile_definitions('+target+' PRIVATE NANO_TEST_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}")')==1 and len(macros)<=1)
   if macros:need(macros==['-DNANO_TEST_SOURCE="/work"'])
   else:flags.insert(1,'-DNANO_TEST_SOURCE="/work"')
  else:need(not macros)
  need(not any('NANO_SYNC_FAULT'in w for w in flags))
  local=rec['newSourceAndGeneratedDependencies'];need(isinstance(local,dict) and isinstance(rec['imageDependencies'],(dict,list)))
  projects=0;deltas=[]
  for guest,row in local.items():
   if guest.startswith('/work/'):
    need(row['path']==str(BASE/'source'/guest[len('/work/'):]))
    # Old source bytes are not re-read as an SDK/admission table. Their recorded byte pins are compared to independently verified current Git.
    old={'sha256':row['sha256'],'bytes':row['bytes']};need(guest in expected);projects+=1
    if expected[guest]!=old:
     need(role=='lifecycleTool' and guest==HEADER and old==BEFORE and expected[guest]==AFTER);deltas.append(guest)
   elif guest.startswith('/outputs/'):
    need(row['path']==str(BASE/'core'/guest[len('/outputs/'):]))
    mount(row['path'],map_output(guest),row)
   else:need(False)
  need(projects==project_count and (deltas==[HEADER] if role=='lifecycleTool' else not deltas))
  link=rec['linkRecipe']['argv'];need(isinstance(link,list) and len(link)>6 and link[0]==argv[0] and link.count('-o')==1)
  oi=link.index('-o');need(oi==4 and link[:3]==['/usr/bin/clang++','-O3','-DNDEBUG'])
  lo=os.path.normpath(link[3] if link[3].startswith('/') else '/outputs/'+link[3]);need(lo==own)
  destination=os.path.normpath(link[oi+1] if link[oi+1].startswith('/') else '/outputs/'+link[oi+1]);need(destination=='/outputs/'+target)
  prefix=link[:3];tail=[];order=[];archives=[];llvm=[]
  for word in link[oi+2:]:
   guest=os.path.normpath('/outputs/'+word) if not word.startswith(('/','-')) else word
   if guest.startswith('/outputs/'):
    if guest in docs['llvm']:
     row=docs['llvm'][guest]['object'];need(row['path']==str(BASE/'core'/guest[len('/outputs/'):]))
     mount(row['path'],map_output(guest),row);llvm.append(guest);all_shared.add(map_output(guest));order.append({'kind':'llvm','old':guest,'current':map_output(guest),'sha256':row['sha256']})
    else:
     need(guest in docs['archives'] and guest.endswith('.a'));row=docs['archives'][guest]['archive'];need(row['path']==str(BASE/'core'/guest[len('/outputs/'):]))
     archives.append(guest)
     if guest=='/outputs/libnano_core.a':order.append({'kind':'core','old':guest,'current':'/baseline/libnano_core.a','sha256':CORE_PIN['sha256']})
     else:mount(row['path'],map_output(guest),row);all_shared.add(map_output(guest));order.append({'kind':'archive','old':guest,'current':map_output(guest),'sha256':row['sha256']})
    tail.append(map_output(guest))
   else:need(guest.startswith(('/usr/','/lib/','/lib64/','-')));tail.append(guest)
  need(len(archives)==5 and archives.count('/outputs/libnano_core.a')==1 and len(llvm)==2 and set(llvm)==set(docs['llvm']))
  reuse=None
  if role!='lifecycleTool':
   objguest='/inputs/'+role+'.cpp.o';mdguest=objguest+'.d'
   for row,g in [(rec['object'],objguest),(rec['dependencyFile'],mdguest)]:
    entry=(row['path'],{'sha256':row['sha256'],'bytes':row['bytes']});need(g not in mounts);mounts[g]=entry
   reuse={'object':dict(path=objguest,sha256=rec['object']['sha256'],bytes=rec['object']['bytes']),'md':dict(path=mdguest,sha256=rec['dependencyFile']['sha256'],bytes=rec['dependencyFile']['bytes']),'dependencyPrefixMap':[{'from':'/outputs/third_party','to':'/old/third_party'}]}
  programs.append({'role':role,'source':'/work/'+relative,'compileFlags':flags,'linkPrefix':prefix,'linkTail':tail,'reuse':reuse})
  joins[role]={'currentProjectPinsMatched':projects-len(deltas),'explicitCurrentHeaderDeltas':deltas,'objectReused':reuse is not None,'originalObjectAndMdFullNineUnchanged':True,'oldImageDependencyPathsUsedAsHistoricalAuthority':False}
  orders[role]=order;recipe_info[role]={'recordFromFixedReceipt':True,'linkRecipeSha256':rec['linkRecipe']['sha256'],'linkRecipeBytes':rec['linkRecipe']['bytes'],'rawRecipeFileReread':False}
 need(len(all_shared)==6)
 recipes={'format':'nano-current-linux-selected-helpers-recipes-v1','owner':'vps-docker','uid':1000,'gid':1000,'imageId':IMAGE,'elfMachine':62,'programs':programs}
 need(digest(WORKER)=={'sha256':'44c7294e0e26a30149a9142ace83a1014dcbb3477ce414e5920d021c9c8cd134','bytes':25229})
 controls={'recipes.json':(json.dumps(recipes,sort_keys=True)+'\n').encode(),'source-expected.private.json':(json.dumps(expected,sort_keys=True)+'\n').encode(),'core-pin.json':(json.dumps(CORE_PIN,sort_keys=True)+'\n').encode(),'worker.pl':WORKER}
 for leaf,body in controls.items():write_new(OUT/'controls'/leaf,body,0o444)
 os.chmod(OUT/'controls',0o555);os.chmod(OUT/'baseline',0o555)
 save('effective-source-joins.private.json',{'sourceBaseRevision':REV,'overlays':[],'selectedGitTreeEntries':found,'currentCmake':CMAKE_PIN,'projectSourcePins':expected,'knownZeroByteVendorMarkersExcludedOnlyFromWorkerMap':sorted(empty_seen),'selectedTargets':list(ROLES),'selectedTargetCmakeAsserted':True,'canonicalWholeCmakeQualified':False,'sourceManifestProposalPin':{'sha256':'9180f4812342f5bb0ad80ab17d422790cda1be9378ac9c70026d0dae30539849','bytes':19782,'scope':'public source proposal; not runtime admission'},'roleProjectJoins':joins,'linkInputOrders':orders,'recipeProvenance':recipe_info})
 docker=['/usr/bin/sudo','-n','/usr/bin/docker','--host','unix:///var/run/docker.sock','--config',str(OUT/'docker-client')]
 def dk(name,args,seconds=30,expected=0,cleanup=False):return run(name,docker+args,seconds,expected,cleanup=cleanup)
 name='nano-current-amd-helpers-'+uuid.uuid4().hex;labelkey='nano.current.amd.helpers';label=labelkey+'='+name;cid=None;creation_attempted=False;first_error=None;cleanup_complete=False;result=None
 try:
  raw,err,_=dk('image-inspect',['image','inspect',IMAGE,'--format','{{json .}}']);image=json.loads(raw);need(not err and image['Id']==IMAGE and image['Architecture']=='amd64' and image['Os']=='linux')
  raw,err,_=dk('name-absent',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}']);need(not raw.strip() and not err)
  mounts_argv=[]
  for guest,(host,_) in sorted(mounts.items()):mounts_argv+=['--mount','type=bind,src='+host+',dst='+guest+',readonly']
  argv=['create','--pull=never','--name',name,'--label',label,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--pids-limit','128','--user','1000:1000','--tmpfs','/tmp:rw,nosuid,nodev,size=128m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(core)+',dst=/baseline/libnano_core.a,readonly','--mount','type=bind,src='+str(OUT/'controls')+',dst=/controls,readonly','--mount','type=bind,src='+str(OUT/'outputs')+',dst=/outputs',*mounts_argv,'--workdir','/outputs','--entrypoint','/bin/sh',IMAGE,'-ec','exec sleep 900']
  save('owned-container.private.json',{'name':name,'label':label,'image':IMAGE,'outputs':str(OUT/'outputs')});fence();creation_attempted=True
  raw,err,_=dk('create-owned-container',argv);cid=raw.decode().strip();need(not err and re.fullmatch('[0-9a-f]{64}',cid))
  raw,err,_=dk('start-owned-container',['start',cid]);need(not err and raw.decode().strip()==cid)
  prime=r"use strict;use warnings;use Digest::SHA ();use JSON::PP ();my @r;for my $p (@ARGV){open(my $f,'<',$p)or die;binmode($f);my $d=Digest::SHA->new(256);$d->addfile($f);push @r,{sha256=>$d->hexdigest,bytes=>-s $p};}print JSON::PP->new->canonical->encode(\@r);"
  names=sorted(controls);raw,err,_=dk('prime-known-controls',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','-e',prime,*['/controls/'+n for n in names]])
  need(not err and json.loads(raw)==[digest(controls[n]) for n in names]);stable_new('controls',{OUT/'controls'/n:digest(controls[n]) for n in names})
  save('original-selected-inputs-before-worker.private.json',HELD)
  raw,err,exitcode=dk('qualify-selected-helpers',['exec','--user','1000:1000',cid,'/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','LANG=C','LC_ALL=C','/usr/bin/perl','/controls/worker.pl'],610,expected=None)
  need(exitcode==0 and not err)
  body=material(OUT/'outputs/result-public.json',16777216)[1];result=json.loads(body);need(json.loads(raw)==result)
  need(result['passed'] is True and result['owner']=='vps-docker' and result['uid']==result['gid']==1000 and result['imageId']==IMAGE and result['elfMachine']==62 and result['productionQualified'] is False)
  need(result['selectedPrograms']==4 and result['independentMCommands']==4 and result['compiledTranslationUnits']==1 and result['linkCommands']==4 and result['loaderCommands']==4 and result['argumentRefusals']==4)
  need({'sha256':result['coreSha256'],'bytes':result['coreBytes']}==CORE_PIN)
  binding=material(OUT/'outputs/software-admission.private.json',16777216)[1];need(digest(binding)=={'sha256':result['softwareAdmissionSha256'],'bytes':result['softwareAdmissionBytes']})
  public={p['role']:p for p in result['programs']};need(set(public)==set(ROLES))
  productpins={OUT/'outputs/software-admission.private.json':digest(binding)}
  for role,(target,_,_) in ROLES.items():
   z=public[role];need(z['target']==target and z['objectReused']==(role!='lifecycleTool') and z['normalizedMEqualsRemappedMd'] is True and z['loaderPassed'] is True and z['argumentRefusalPassed'] is True)
   p=OUT/'outputs'/target;row,software=material(p);pin={'sha256':z['softwareSha256'],'bytes':z['softwareBytes']};need(digest(software)==pin and stat.S_IMODE(row['st_mode'])==0o500 and row['st_uid']==row['st_gid']==1000 and row['st_nlink']==1);productpins[p]=pin
  stable_new('qualified-host-products',productpins)
 except BaseException as exc:first_error=exc;raise
 finally:
  for sig in (signal.SIGTERM,signal.SIGINT,signal.SIGHUP):signal.signal(sig,signal.SIG_IGN)
  CLEAN_DEADLINE=time.monotonic()+60
  try:
   if creation_attempted:
    if cid is None:
     raw,err,_=dk('cleanup-discover',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],10,cleanup=True);need(not err and len(raw.splitlines())<=1);cid=raw.decode().strip() or None
    if cid:
     need(re.fullmatch('[0-9a-f]{64}',cid));raw,err,_=dk('cleanup-inspect',['inspect',cid,'--format','{{json .}}'],10,cleanup=True);item=json.loads(raw);need(not err and item['Id']==cid and item['Name']=='/'+name and item['Config']['Labels'].get(labelkey)==name and item['Config']['Image']==IMAGE and item['Image']==IMAGE)
     dk('cleanup-remove',['rm','-f',cid],15,cleanup=True)
    raw,err,_=dk('cleanup-absence',['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],10,cleanup=True);need(not raw.strip() and not err)
   cleanup_complete=True;save('cleanup-public.json',{'ownedContainerAbsent':True,'creationAttempted':creation_attempted,'ownedOutputsRetained':True,'imagesBuilt':0,'oldSdkMapsRead':False})
  except BaseException as error:
   save('cleanup-failure-public.json',{'passed':False,'errorType':type(error).__name__,'firstFailurePreserved':first_error is not None})
   if first_error is None:raise
 need(cleanup_complete and result is not None);fence()
 need(git('final-git-head',['rev-parse','HEAD']).decode().strip()==REV and git('final-git-clean',['status','--porcelain'])==b'')
 result.update({'rootSourceRevision':REV,'rootSelectedTargetCmakeAsserted':True,'rootProjectSourceJoins':joins,'rootSixSharedLibraryFilesDeduplicated':len(all_shared),'rootSelectedOriginalRowsFullNineAndShaUnchanged':True,'historicalSourceFullNineRecovered':False,'oldSdkTablesRead':False,'oldSourceAdmissionRead':False,'fixedProducerReceiptsRead':4,'rootCurrentCoreCopiedKnownBytes':True,'rootHostQualifiedProgramsStableSamples':3,'imageIdentityVerifiedByRoot':True,'rootOwnedContainerAbsent':True,'runtimeFixturesQualified':False,'oldAuthoritiesRefreshed':False,'productionQualified':False})
 save('result-public.json',result);print(json.dumps(result,sort_keys=True),flush=True)

if __name__=='__main__':
 failed=False
 try:main()
 except BaseException:
  failed=True
  if OUT_CREATED:save('failure-public.json',{'passed':False,'oldSdkTablesRead':False,'oldSourceAdmissionRead':False,'oldAuthoritiesRefreshed':False,'compilerOrTestSuccessClaimed':False,'rawErrorDetailsExported':False})
  raise
 finally:
  try:
   if ACTIVE is not None:stop(ACTIVE)
   fence()
   if OUT_CREATED:save('originals-final-fence-public.json',{'passed':True,'heldSelectedFiles':len(HELD),'firstFailurePreserved':failed,'oldAuthoritiesRefreshed':False})
  except BaseException as error:
   if OUT_CREATED:save('originals-final-fence-failure-public.json',{'passed':False,'firstFailurePreserved':failed,'errorType':type(error).__name__})
   if not failed:raise
