#!/usr/bin/perl
# Root-only source proposal. The author has not run Perl or any project program.
use strict;
use warnings;
use Fcntl qw(:DEFAULT :mode);
use POSIX qw(setsid WNOHANG);
use Cwd qw(abs_path);
use Time::HiRes qw(clock_gettime CLOCK_MONOTONIC sleep);
use JSON::PP ();
use Digest::SHA ();
use Text::ParseWords qw(shellwords);
use IO::Handle ();
use IO::Select ();

umask(0077);
my $J=JSON::PP->new->canonical(1)->ascii(1);
my ($yes,$no)=(JSON::PP::true,JSON::PP::false);
sub now {return clock_gettime(CLOCK_MONOTONIC);}
my $deadline=now()+590;
my $active;
my (%held,@steps,@observations);
my $expected;
my $file_limit=536870912;
my $stream_limit=16777216;
my $out='/outputs';
my $output_admitted=0;
my $stage='startup';
$SIG{TERM}=sub{die "worker interrupted\n";};
$SIG{INT}=sub{die "worker interrupted\n";};
$SIG{HUP}=sub{die "worker interrupted\n";};

sub need {die "Nano relink worker refused\n" unless $_[0];}
sub tick {need(now()<$deadline);}
sub same {return $J->encode($_[0]) eq $J->encode($_[1]);}
sub keys_are {my ($h,@k)=@_;return ref($h) eq 'HASH' && join('|',sort keys %$h) eq join('|',sort @k);}
sub leaf {return defined($_[0])&&!ref($_[0])&&$_[0]=~/\A[A-Za-z0-9_.-]{1,160}\z/&&$_[0] ne '.'&&$_[0] ne '..';}
sub fd_path {return '/proc/'.$$.'/fd/'.fileno($_[0]);}
sub stop_group {
 my ($pid)=@_;return 1 unless defined($pid);
 local $SIG{TERM}='IGNORE';local $SIG{INT}='IGNORE';local $SIG{HUP}='IGNORE';
 my $waiting=waitpid($pid,WNOHANG)==0;
 kill('TERM',-$pid);kill('TERM',$pid) if $waiting;
 my $until=now()+0.5;
 while(now()<$until&&($waiting||kill(0,-$pid))){$waiting=0 if $waiting&&waitpid($pid,WNOHANG)!=0;sleep(0.025);}
 kill('KILL',-$pid) if kill(0,-$pid);kill('KILL',$pid) if $waiting;
 $until=now()+0.5;
 while($waiting&&now()<$until){$waiting=0 if waitpid($pid,WNOHANG)!=0;sleep(0.025) if $waiting;}
 return !$waiting&&!kill(0,-$pid);
}
sub child_exec {
 my ($argv,$stdout,$stderr)=@_;
 need(ref($argv) eq 'ARRAY'&&@$argv&&$argv->[0]=~m{\A/});
 for(@$argv){need(defined($_)&&!ref($_)&&index($_,"\0")==-1);}
 my $pid=fork();need(defined($pid));
 if(!$pid){
  eval {
   setsid()!=-1 or die;
   chdir($out) or die;
   open(STDIN,'<','/dev/null') or die;
   open(STDOUT,'>&',$stdout) or die;
   open(STDERR,'>&',$stderr) or die;
   exec {$argv->[0]} @$argv or die;
  };
  POSIX::_exit(126);
 }
 $active=$pid;return $pid;
}

# GNU stat supplies exact timestamps as strings. Never use Perl's rounded stat
# timestamps as an authority. Both rows use the worker PID for its open FD.
sub stat_pair {
 my ($fh,$path)=@_;tick();
 pipe(my $reader,my $writer) or die "Nano relink worker refused\n";
 sysopen(my $err,'/dev/null',O_WRONLY) or die "Nano relink worker refused\n";
 my $pid=child_exec(['/usr/bin/stat','-Lc','%d|%i|%f|%h|%u|%g|%s|%y|%z','--',fd_path($fh),$path],$writer,$err);
 close($writer);close($err);my $raw='';my $ok;
 eval {
  my $select=IO::Select->new($reader);my $until=now()+10;
  while(1){tick();need(now()<$until);next unless $select->can_read(0.1);my $n=sysread($reader,my $b,2048);need(defined($n));last unless $n;$raw.=$b;need(length($raw)<4096);}
  my ($wait,$status);while(1){tick();need(now()<$until);$wait=waitpid($pid,WNOHANG);need($wait>=0);if($wait==$pid){$status=$?;last;}sleep(0.001);}
  need($status==0&&!kill(0,-$pid));$ok=1;
 };
 my $error=$@;close($reader);my $stopped=$ok?1:stop_group($pid);$active=undef if $stopped;die $error unless $ok;
 my $removed=($raw=~s/\n\z//);need($removed==1&&$raw!~/\r/);
 my @lines=split(/\n/,$raw,-1);need(@lines==2);
 my @rows;
 for my $line(@lines){my @r=split(/\|/,$line,-1);need(@r==9);for(@r){need(length($_)>0&&length($_)<256&&$_!~/[\r\n]/);}push @rows,\@r;}
 need(same($rows[0],$rows[1]));return $rows[0];
}
sub read_regular {
 my ($logical,$capture,$limit)=@_;tick();$limit//=$file_limit;
 need(defined($logical)&&!ref($logical)&&$logical=~m{\A/}&&$logical!~/[\r\n\0]/);
 my $path=abs_path($logical);need(defined($path)&&$path=~m{\A/(?:work|old|baseline|outputs|controls|inputs|usr|lib|lib64)(?:/|\z)});
 sysopen(my $fh,$path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK) or die "Nano relink worker refused\n";
 my @s=stat($fh);my @p=lstat($path);need(@s&&@p&&S_ISREG($s[2])&&S_ISREG($p[2])&&$s[7]<=$limit);
 my $before=stat_pair($fh,$path);my ($bytes,$body)=(0,'');my $sha=Digest::SHA->new(256);
 while(1){tick();my $n=sysread($fh,my $b,65536);need(defined($n));last unless $n;$bytes+=$n;need($bytes<=$limit&&$bytes<=$s[7]);$sha->add($b);$body.=$b if $capture;}
 need($bytes==$s[7]&&same($before,stat_pair($fh,$path))&&abs_path($logical) eq $path);
 close($fh) or die "Nano relink worker refused\n";
 my $row={material=>$before,bytes=>0+$bytes,sha256=>$sha->hexdigest};
 return ($path,$row,$body);
}
sub material {my (undef,$r)=read_regular($_[0],0);return $r;}
sub read_bytes {my (undef,undef,$b)=read_regular($_[0],1,$_[1]//$stream_limit);return $b;}
sub hold {
 my ($path)=@_;my ($real,$row)=read_regular($path,0);
 need(!exists($held{$real})||same($held{$real},$row));$held{$real}=$row;return $row;
}
sub fence {for my $p(sort keys %held){need(same($held{$p},material($p)));}}
sub save {
 my ($name,$value)=@_;need($output_admitted&&leaf($name));my $body=$J->encode($value)."\n";
 sysopen(my $fh,$out.'/'.$name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Nano relink worker refused\n";
 my $at=0;while($at<length($body)){tick();my $n=syswrite($fh,$body,length($body)-$at,$at);need(defined($n)&&$n>0);$at+=$n;}
 $fh->sync or die "Nano relink worker refused\n";close($fh) or die "Nano relink worker refused\n";
}
sub call {
 my ($name,$argv,$wanted)=@_;$wanted//=0;need(leaf($name));tick();
 save($name.'-argv.private.json',$argv);
 sysopen(my $stdout,$out.'/'.$name.'-stdout.private.log',O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Nano relink worker refused\n";
 sysopen(my $stderr,$out.'/'.$name.'-stderr.private.log',O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Nano relink worker refused\n";
 my $start=now();my $pid=child_exec($argv,$stdout,$stderr);my ($status,$done,$ok);
 eval {
  while(1){
   tick();need(now()-$start<179);
   need((stat($stdout))[7]<=$stream_limit&&(stat($stderr))[7]<=$stream_limit);
   my $w=waitpid($pid,WNOHANG);need($w>=0);
   if($w==$pid){$status=$?;$done=1;last;}sleep(0.025);
  }
  need(!kill(0,-$pid));$ok=1;
 };
 my $error=$@;my $stopped=$ok?1:stop_group($pid);$active=undef if $stopped;
 $stdout->sync; $stderr->sync;close($stdout);close($stderr);
 die $error unless $ok;
 need($done&&($status&127)==0);my $exit=$status>>8;
 my $event={step=>$name,exitCode=>$exit,elapsedMs=>int((now()-$start)*1000)};
 push @steps,$event;save($name.'-exit.json',$event);need($exit==$wanted);
 return read_bytes($out.'/'.$name.'-stdout.private.log');
}
sub deps {
 my ($raw)=@_;need($raw!~/\0/);$raw=~s/\\\n/ /g;need($raw=~s/\A[^:\n]+://);
 my @words=shellwords($raw);need(@words>0);my %unique;
 for(@words){need(m{\A/}&&!/[\r\n]/);$unique{$_}=1;}
 return [sort keys %unique];
}
sub compile_one {
 my ($name,$argv,$source,$object)=@_;need(ref($argv) eq 'ARRAY'&&@$argv&&$argv->[0] eq '/usr/bin/clang++');
 my $m=call($name.'-independent-m',[@$argv,'-M','-MT',$object,'-o','/dev/stdout',$source]);my $before=deps($m);
 my $start=now();my ($previous,$series,$stable,$table)=(undef,$start,0,undef);
 while(1){
  $table={};
  for my $p(@$before){my ($real,$row)=read_regular($p,0);need(!exists($held{$real})||same($held{$real},$row));
   if($real=~m{\A/(?:work|old)/}){need(exists($expected->{$real})&&$row->{sha256} eq $expected->{$real}{sha256}&&$row->{bytes}==$expected->{$real}{bytes});}
   else{need($real=~m{\A/(?:usr|lib|lib64)/});}
   $table->{$real}=$row;
  }
  my $now=now();need($now-$start<30);push @observations,{step=>$name,elapsedMs=>int(($now-$start)*1000),table=>$table};
  if(defined($previous)&&same($previous,$table)){++$stable;}else{$stable=1;$series=$now;}
  last if $stable>=3&&$now-$series>=2;$previous=$table;sleep(1);
 }
 for my $p(keys %$table){need(!exists($held{$p})||same($held{$p},$table->{$p}));$held{$p}=$table->{$p};}
 fence();my $md=$object.'.d';call($name.'-compile',[@$argv,'-MD','-MF',$md,'-MT',$object,'-o',$object,'-c',$source]);
 need(same($before,deps(read_bytes($md))));fence();
 return {dependencies=>scalar(@$before),independentMEqualsMd=>$yes,object=>material($object),md=>material($md)};
}
# Preserve the old MD's logical paths; no implicit namespace rewrite occurs.
sub stable_paths {
 my ($name,$paths)=@_;my $start=now();my ($previous,$series,$count,$table)=(undef,$start,0,undef);
 while(1){
  $table={};
  for my $p(@$paths){my ($real,$row)=read_regular($p,0);need(!exists($held{$real})||same($held{$real},$row));
   if($real=~m{\A/(?:work|old)/}){need(exists($expected->{$real})&&$row->{sha256} eq $expected->{$real}{sha256}&&$row->{bytes}==$expected->{$real}{bytes});}
   else{need($real=~m{\A/(?:inputs|baseline|outputs|controls|usr|lib|lib64)/});}
   $table->{$real}=$row;
  }
  my $now=now();need($now-$start<30);push @observations,{step=>$name,elapsedMs=>int(($now-$start)*1000),table=>$table};
  if(defined($previous)&&same($previous,$table)){++$count;}else{$count=1;$series=$now;}
  last if $count>=3&&$now-$series>=2;$previous=$table;sleep(1);
 }
 for my $p(keys %$table){need(!exists($held{$p})||same($held{$p},$table->{$p}));$held{$p}=$table->{$p};}
 return $table;
}
sub stable_product {
 my ($path,$uid,$gid)=@_;my $start=now();my ($previous,$series,$count,$row)=(undef,$start,0,undef);
 while(1){
  my @s=lstat($path);need(@s&&S_ISREG($s[2])&&$s[3]==1&&$s[4]==$uid&&$s[5]==$gid&&($s[2]&07777)==0500);
  $row=material($path);my $now=now();need($now-$start<30);
  if(defined($previous)&&same($previous,$row)){++$count;}else{$count=1;$series=$now;}
  last if $count>=3&&$now-$series>=2;$previous=$row;sleep(1);
 }
 need(!exists($held{$path})||same($held{$path},$row));$held{$path}=$row;return $row;
}
sub main {
 need(@ARGV==0);
 my $recipe=$J->decode(read_bytes('/controls/recipes.json'));$expected=$J->decode(read_bytes('/controls/source-expected.private.json'));my $core_pin=$J->decode(read_bytes('/controls/core-pin.json'));
 need(keys_are($recipe,qw(mainFlags linkArgvTail uid gid reuseMain))&&ref($expected) eq 'HASH');
 need(keys_are($core_pin,qw(sha256 bytes))&&!ref($core_pin->{sha256})&&$core_pin->{sha256}=~/\A[0-9a-f]{64}\z/&&!ref($core_pin->{bytes})&&$core_pin->{bytes}=~/\A[1-9][0-9]*\z/&&$core_pin->{bytes}<=$file_limit);
 my ($uid,$gid)=@{$recipe}{qw(uid gid)};need(($uid==501&&$gid==20)||($uid==1000&&$gid==1000));
 need($>==$uid&&$<==$uid&&abs_path($out) eq $out);my @s=lstat($out);need(@s&&S_ISDIR($s[2])&&($s[2]&07777)==0700&&$s[4]==$uid&&$s[5]==$gid);
 opendir(my $dir,$out) or die "Nano relink worker refused\n";my @existing=grep {$_ ne '.'&&$_ ne '..'} readdir($dir);closedir($dir);need(!@existing);
 $output_admitted=1;
 my $flags=$recipe->{mainFlags};my $tail=$recipe->{linkArgvTail};my $reuse=$recipe->{reuseMain};
 need(ref($flags) eq 'ARRAY'&&@$flags&&$flags->[0] eq '/usr/bin/clang++'&&ref($tail) eq 'ARRAY'&&@$tail&&(same($reuse,$yes)||same($reuse,$no)));
 for my $word(@$flags,@$tail){need(defined($word)&&!ref($word)&&$word!~/[\0\r\n]/);}
 for my $word(@$flags){need($word!~/\A(?:-c|-o|-M|-MM|-MD|-MMD|-MP|-MF|-MT|-MQ)\z/&&$word!~/\A(?:-MF|-MT|-MQ|-MJ).+/);}
 need(grep($_ eq '/baseline/libnano_core.a',@$tail)==1);
 for my $word(@$tail){need($word=~m{\A(?:/baseline/libnano_core[.]a\z|/(?:old|usr|lib|lib64)/|-)});}
 for my $p('/controls/recipes.json','/controls/source-expected.private.json','/controls/core-pin.json','/controls/worker.pl','/usr/bin/clang++','/usr/bin/stat','/baseline/libnano_core.a'){hold($p);}
 my $core=$held{'/baseline/libnano_core.a'};need($core->{sha256} eq $core_pin->{sha256}&&$core->{bytes}==$core_pin->{bytes});
 my ($object,$md,$dependency_names,$main_record);
 $stage='main';
 if($reuse){
  $object='/inputs/main.cpp.o';$md='/inputs/main.cpp.o.d';hold($object);hold($md);
  my $m=call('main-independent-m',[@$flags,'-M','-MT','/outputs/main.cpp.o','-o','/dev/stdout','/work/src/main.cpp']);
  $dependency_names=deps($m);need(same($dependency_names,deps(read_bytes($md))));fence();
  $main_record={dependencies=>scalar(@$dependency_names),independentMEqualsMd=>$yes,object=>material($object),md=>material($md)};
 }else{
  $object='/outputs/main.cpp.o';$md=$object.'.d';$main_record=compile_one('main',$flags,'/work/src/main.cpp',$object);$dependency_names=deps(read_bytes($md));
 }
 need(grep($_ eq '/work/src/main.cpp',@$dependency_names)==1);
 my %paths=map {$_=>1} (@$dependency_names,$object,$md,'/baseline/libnano_core.a','/usr/bin/clang++','/usr/bin/stat','/controls/recipes.json','/controls/source-expected.private.json','/controls/core-pin.json','/controls/worker.pl',grep {m{\A/}} @$tail);
 my @paths=sort keys %paths;$stage='pre-link';stable_paths('before-link',\@paths);fence();
 need(same($main_record->{object},material($object))&&same($main_record->{md},material($md)));
 my $nano='/outputs/nano';my @link=('/usr/bin/clang++','-O3','-DNDEBUG',$object,'-o',$nano,@$tail);
 call('link-nano',\@link);fence();need(chmod(0500,$nano)==1);my $nano_record=stable_product($nano,$uid,$gid);fence();
 $stage='smokes';
 my $help=call('nano-help',[$nano,'--help']);
 my $help_prefix="Nano local chain tools\nUsage: nano <init|status|audit|mine|validate|import|serve|run> <genesis.json> <database-directory> [arguments]\n";
 need(index($help,$help_prefix)==0&&read_bytes('/outputs/nano-help-stderr.private.log') eq '');
 my $refusal=call('nano-rpc-arity',[$nano,'rpc'],1);
 need($refusal eq ''&&read_bytes('/outputs/nano-rpc-arity-stderr.private.log') eq "Nano: rpc <URL> <info|nodes|network|balance|send|deploy|receipt|mine> [arguments]\n");
 fence();need(same($nano_record,material($nano))&&same($main_record->{object},material($object))&&same($main_record->{md},material($md)));
 save('sdk-and-inputs.private.json',\%held);save('first-dependency-observations.private.json',\@observations);
 save('software-admission.private.json',{format=>'nano-current-core-linux-software-binding-v1',mainReused=>$reuse,mainObject=>$main_record->{object},mainDependencyFile=>$main_record->{md},coreArchive=>$core,nano=>$nano_record,guestMaterials=>\%held,originalInputsFullNineAndShaUnchanged=>$yes,oldAuthoritiesRefreshed=>$no});
 my $binding=material('/outputs/software-admission.private.json');
 my $result={format=>'nano-current-core-linux-thin-relink-v1',passed=>$yes,uid=>0+$uid,gid=>0+$gid,mainReused=>$reuse,compiledTranslationUnits=>$reuse?0:1,independentMCommands=>1,mainIndependentMEqualsMd=>$yes,mainDependencies=>$main_record->{dependencies},linkCommands=>1,smokeCommands=>2,helpPassed=>$yes,rpcArityExpectedRefusal=>$yes,expectedRefusals=>1,coreSha256=>$core->{sha256},coreBytes=>$core->{bytes},nanoSha256=>$nano_record->{sha256},nanoBytes=>$nano_record->{bytes},mainObjectSha256=>$main_record->{object}{sha256},softwareAdmissionSha256=>$binding->{sha256},softwareAdmissionBytes=>$binding->{bytes},originalGuestInputsFullNineAndShaUnchanged=>$yes,steps=>\@steps,coreArchiveMutated=>$no,coreMembersRequalified=>$no,rpcSuiteReplayed=>$no,oldSuiteExecuted=>$no,dockerImageBuilt=>$no,nodeRuntimeStarted=>$no,networkStarted=>$no,dutiesResumed=>$no,commonFinalityQualified=>$no,wholeSdkQualified=>$no,canonicalCmakeQualifiedByWorker=>$no,campaignAdmission=>$no,productionQualified=>$no};
 save('result-public.json',$result);print $J->encode($result)."\n" or die "Nano relink worker refused\n";
}
my $ok=eval{main();1;};
unless($ok){my $stopped=stop_group($active);local $SIG{TERM}='IGNORE';local $SIG{INT}='IGNORE';local $SIG{HUP}='IGNORE';my $post=$stopped?eval{fence();1;}:0;eval{save('worker-refusal.private.json',{passed=>$no,stage=>$stage,originalGuestInputsUnchanged=>$post?$yes:$no,ownProcessGroupCleanupComplete=>$stopped?$yes:$no,completedCommandEvents=>scalar(@steps)});};print STDERR "Nano relink worker refused\n";exit(1);}
exit(0);
