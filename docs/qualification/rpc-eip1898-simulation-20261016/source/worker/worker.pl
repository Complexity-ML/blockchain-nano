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
my $stage='startup';
$SIG{TERM}=sub{die "worker interrupted\n";};
$SIG{INT}=sub{die "worker interrupted\n";};
$SIG{HUP}=sub{die "worker interrupted\n";};

sub need {die "RPC worker refused\n" unless $_[0];}
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
 pipe(my $reader,my $writer) or die "RPC worker refused\n";
 sysopen(my $err,'/dev/null',O_WRONLY) or die "RPC worker refused\n";
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
 my $path=abs_path($logical);need(defined($path)&&$path=~m{\A/(?:work|old|baseline|outputs|controls|usr|lib|lib64)(?:/|\z)});
 sysopen(my $fh,$path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK) or die "RPC worker refused\n";
 my @s=stat($fh);my @p=lstat($path);need(@s&&@p&&S_ISREG($s[2])&&S_ISREG($p[2])&&$s[7]<=$limit);
 my $before=stat_pair($fh,$path);my ($bytes,$body)=(0,'');my $sha=Digest::SHA->new(256);
 while(1){tick();my $n=sysread($fh,my $b,65536);need(defined($n));last unless $n;$bytes+=$n;need($bytes<=$limit&&$bytes<=$s[7]);$sha->add($b);$body.=$b if $capture;}
 need($bytes==$s[7]&&same($before,stat_pair($fh,$path))&&abs_path($logical) eq $path);
 close($fh) or die "RPC worker refused\n";
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
 my ($name,$value)=@_;need(leaf($name));my $body=$J->encode($value)."\n";
 sysopen(my $fh,$out.'/'.$name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "RPC worker refused\n";
 my $at=0;while($at<length($body)){tick();my $n=syswrite($fh,$body,length($body)-$at,$at);need(defined($n)&&$n>0);$at+=$n;}
 $fh->sync or die "RPC worker refused\n";close($fh) or die "RPC worker refused\n";
}
sub call {
 my ($name,$argv,$wanted)=@_;$wanted//=0;need(leaf($name));tick();
 save($name.'-argv.private.json',$argv);
 sysopen(my $stdout,$out.'/'.$name.'-stdout.private.log',O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "RPC worker refused\n";
 sysopen(my $stderr,$out.'/'.$name.'-stderr.private.log',O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "RPC worker refused\n";
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
sub members {
 my ($path)=@_;my $b=read_bytes($path,$file_limit);need(substr($b,0,8) eq "!<arch>\n");my ($at,$strings)=(8,'');my @table;
 while($at<length($b)){
  my $h=substr($b,$at,60);need(length($h)==60&&substr($h,58,2) eq "`\n");my $name=substr($h,0,16);$name=~s/\s+\z//;
  my $n=substr($h,48,10);$n=~s/\s+\z//;need($n=~/\A[0-9]+\z/);$n=0+$n;$at+=60;need($n<=length($b)-$at);
  my $payload=substr($b,$at,$n);$at+=$n+($n%2);
  if($name eq '//'){$strings=$payload;next;}next if $name eq '/'||$name eq '/SYM64/';
  if($name=~m{\A/([0-9]+)\z}){my $offset=0+$1;my $end=index($strings,"/\n",$offset);need($offset<length($strings)&&$end>=$offset);$name=substr($strings,$offset,$end-$offset);}
  else{$name=~s{/\z}{};}
  need(length($name)>0);push @table,[$name,$n,Digest::SHA::sha256_hex($payload)];
 }
 need($at==length($b));return \@table;
}
sub json_rows {my ($raw)=@_;return [map {$J->decode($_)} grep {/\S/} split(/\n/,$raw)];}
sub main {
 need(@ARGV==0&&$>==501&&$<==501);need(abs_path($out) eq $out);my @s=lstat($out);need(@s&&S_ISDIR($s[2])&&($s[2]&07777)==0700&&$s[4]==501&&$s[5]==20);
 opendir(my $dir,$out) or die "RPC worker refused\n";my @existing=grep {$_ ne '.'&&$_ ne '..'} readdir($dir);closedir($dir);need(!@existing);
 my $recipe=$J->decode(read_bytes('/controls/recipes.json'));$expected=$J->decode(read_bytes('/controls/source-expected.private.json'));
 need(keys_are($recipe,qw(coreFlags testFlags linkTail))&&ref($expected) eq 'HASH'&&ref($recipe->{linkTail}) eq 'ARRAY');
 for my $p('/controls/recipes.json','/controls/source-expected.private.json','/controls/worker.pl','/usr/bin/clang++','/usr/bin/ar','/usr/bin/ranlib','/usr/bin/cp','/usr/bin/stat','/baseline/libnano_core.a'){hold($p);}
 $stage='test';my $test=compile_one('test',$recipe->{testFlags},'/work/tests/test_rpc_simulation_block_id.cpp','/outputs/test.cpp.o');
 my $libs=$recipe->{linkTail};for my $p(@$libs){hold($p) if $p=~m{\A/}&&$p ne '/outputs/libnano_core.a';}
 my $old=members('/baseline/libnano_core.a');need(@$old==143&&grep($_->[0] eq 'rpc.cpp.o',@$old)==1);
 my @link=('/usr/bin/clang++','-O3','-DNDEBUG','/outputs/test.cpp.o','-o');
 my @tail=map {$_ eq '/outputs/libnano_core.a'?'/baseline/libnano_core.a':$_} @$libs;
 $stage='baseline';call('link-baseline',[@link,'/outputs/baseline-test',@tail]);my $baseline=call('baseline-new-selector',['/outputs/baseline-test','--eip1898-self-test'],1);
 need(same(json_rows($baseline),[{format=>'nano-rpc-simulation-eip1898-targeted-v1',passed=>$no,stage=>'canonical-selectors',checks=>4,expectedRefusals=>0,productionQualified=>$no}]));
 need(read_bytes('/outputs/baseline-new-selector-stderr.private.log') eq "selected simulation should succeed\n");
 fence();$stage='rpc';my $core=compile_one('rpc',$recipe->{coreFlags},'/work/src/rpc.cpp','/outputs/rpc.cpp.o');
 call('copy-core',['/usr/bin/cp','/baseline/libnano_core.a','/outputs/libnano_core.a']);call('replace-rpc-member',['/usr/bin/ar','r','/outputs/libnano_core.a','/outputs/rpc.cpp.o']);call('index-core',['/usr/bin/ranlib','/outputs/libnano_core.a']);
 my $new=members('/outputs/libnano_core.a');need(@$new==143);my @old_kept=grep {$_->[0] ne 'rpc.cpp.o'} @$old;my @new_kept=grep {$_->[0] ne 'rpc.cpp.o'} @$new;need(@old_kept==142&&same(\@old_kept,\@new_kept));
 my @new_rpc=grep {$_->[0] eq 'rpc.cpp.o'} @$new;my $row=material('/outputs/rpc.cpp.o');need(@new_rpc==1&&$new_rpc[0][1]==$row->{bytes}&&$new_rpc[0][2] eq $row->{sha256});
 $stage='candidate';call('link-new',[@link,'/outputs/nano-rpc-simulation-block-id-tests',@$libs]);my $actual=call('new-selector',['/outputs/nano-rpc-simulation-block-id-tests','--eip1898-self-test']);fence();
 my $rows=json_rows($actual);my @names=qw(canonical-hash-number-and-pending malformed-and-unknown-resource-precedence immutable-context-and-stale-hash-refusal hash-selector-cancellation-and-deadline known-header-without-historical-state);my @refusals=(0,45,51,55,58);
 need(@$rows==6);for my $i(0..4){need(keys_are($rows->[$i],qw(scenario passed checks expectedRefusals))&&$rows->[$i]{scenario} eq $names[$i]&&same($rows->[$i]{passed},$yes)&&$rows->[$i]{expectedRefusals}==$refusals[$i]);}
 my $summary=$rows->[5];need(keys_are($summary,qw(format passed groups checks expectedRefusals oldSuiteExecuted networkStarted productionQualified))&&$summary->{format} eq 'nano-rpc-simulation-eip1898-targeted-v1'&&same($summary->{passed},$yes)&&$summary->{groups}==5&&$summary->{expectedRefusals}==58&&same($summary->{oldSuiteExecuted},$no)&&same($summary->{networkStarted},$no)&&same($summary->{productionQualified},$no));
 save('sdk-and-inputs.private.json',\%held);save('first-dependency-observations.private.json',\@observations);
 my $result={format=>'nano-rpc-eip1898-arm-thin-qualification-v1',passed=>$yes,compiledTranslationUnits=>2,archiveMembers=>143,oldMembersUnchanged=>142,actualRpcMemberMatchesNewObject=>$yes,independentMEqualsMd=>$yes,coreDependencies=>$core->{dependencies},testDependencies=>$test->{dependencies},baselineExpectedFailure=>$yes,baselineExpectedStage=>'canonical-selectors',targetedTestSummary=>$summary,baselineStdoutSha256=>Digest::SHA::sha256_hex($baseline),newSelectorStdoutSha256=>Digest::SHA::sha256_hex($actual),coreSha256=>material('/outputs/libnano_core.a')->{sha256},executableSha256=>material('/outputs/nano-rpc-simulation-block-id-tests')->{sha256},steps=>\@steps,oldSuiteExecuted=>$no,dockerImageBuilt=>$no,nodeRuntimeStarted=>$no,campaignAdmission=>$no,productionQualified=>$no};
 save('result-public.json',$result);print $J->encode($result)."\n" or die "RPC worker refused\n";
}
my $ok=eval{main();1;};my $first=$@;
unless($ok){my $stopped=stop_group($active);local $SIG{TERM}='IGNORE';local $SIG{INT}='IGNORE';local $SIG{HUP}='IGNORE';my $post=$stopped?eval{fence();1;}:0;eval{save('worker-refusal.private.json',{passed=>$no,stage=>$stage,originalGuestInputsUnchanged=>$post?$yes:$no,ownProcessGroupCleanupComplete=>$stopped?$yes:$no,projectCommandCount=>scalar(@steps)});};print STDERR "RPC worker refused\n";exit(1);}
exit(0);
