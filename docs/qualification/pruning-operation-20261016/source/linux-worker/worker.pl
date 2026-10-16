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
my $deadline=now()+1200;
my $active;
my (%held,%logical_guards,@steps,@observations);
my $expected;
my %counts=(independentMCommands=>0,compiledTranslationUnits=>0,linkCommands=>0,loaderCommands=>0,argumentRefusals=>0);
my $file_limit=536870912;
my $stream_limit=16777216;
my $out='/outputs';
my $output_admitted=0;
my $stage='startup';
$SIG{TERM}=sub{die "worker interrupted\n";};
$SIG{INT}=sub{die "worker interrupted\n";};
$SIG{HUP}=sub{die "worker interrupted\n";};

sub need {die "Selected helpers worker refused\n" unless $_[0];}
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
 pipe(my $reader,my $writer) or die "Selected helpers worker refused\n";
 sysopen(my $err,'/dev/null',O_WRONLY) or die "Selected helpers worker refused\n";
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
 sysopen(my $fh,$path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK) or die "Selected helpers worker refused\n";
 my @s=stat($fh);my @p=lstat($path);need(@s&&@p&&S_ISREG($s[2])&&S_ISREG($p[2])&&$s[7]<=$limit);
 my $before=stat_pair($fh,$path);my ($bytes,$body)=(0,'');my $sha=Digest::SHA->new(256);
 while(1){tick();my $n=sysread($fh,my $b,65536);need(defined($n));last unless $n;$bytes+=$n;need($bytes<=$limit&&$bytes<=$s[7]);$sha->add($b);$body.=$b if $capture;}
 need($bytes==$s[7]&&same($before,stat_pair($fh,$path))&&abs_path($logical) eq $path);
 close($fh) or die "Selected helpers worker refused\n";
 my $row={material=>$before,bytes=>0+$bytes,sha256=>$sha->hexdigest};
 return ($path,$row,$body);
}
sub material {my (undef,$r)=read_regular($_[0],0);return $r;}
sub read_bytes {my (undef,undef,$b)=read_regular($_[0],1,$_[1]//$stream_limit);return $b;}
sub hold {
 my ($path)=@_;my ($real,$row)=read_regular($path,0);
 need(!exists($held{$real})||same($held{$real},$row));need(!exists($logical_guards{$path})||$logical_guards{$path} eq $real);$logical_guards{$path}=$real;$held{$real}=$row;return $row;
}
sub fence {for my $p(sort keys %logical_guards){my $r=abs_path($p);need(defined($r)&&$r eq $logical_guards{$p});}for my $p(sort keys %held){need(same($held{$p},material($p)));}}
sub save {
 my ($name,$value)=@_;need($output_admitted&&leaf($name));my $body=$J->encode($value)."\n";
 sysopen(my $fh,$out.'/'.$name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Selected helpers worker refused\n";
 my $at=0;while($at<length($body)){tick();my $n=syswrite($fh,$body,length($body)-$at,$at);need(defined($n)&&$n>0);$at+=$n;}
 $fh->sync or die "Selected helpers worker refused\n";close($fh) or die "Selected helpers worker refused\n";
}
sub call {
 my ($name,$argv,$wanted)=@_;$wanted//=0;need(leaf($name));tick();
 save($name.'-argv.private.json',$argv);
 sysopen(my $stdout,$out.'/'.$name.'-stdout.private.log',O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Selected helpers worker refused\n";
 sysopen(my $stderr,$out.'/'.$name.'-stderr.private.log',O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Selected helpers worker refused\n";
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
# Dependency comparisons are lexical and explicitly remapped only for retained MD.
# The current -M spellings must also resolve to the same file after normalization.
sub normal_path {
 my ($path)=@_;need(defined($path)&&!ref($path)&&$path=~m{\A/}&&$path!~/[\0\r\n]/);
 my @parts;for my $part(split(m{/+},$path)){next if $part eq ''||$part eq '.';if($part eq '..'){need(@parts);pop @parts;}else{push @parts,$part;}}
 return '/'.join('/',@parts);
}
sub normalized_deps {
 my ($names,$map,$current)=@_;my %found;
 for my $name(@$names){my $p=$name;
  for my $entry(@$map){my ($from,$to)=@{$entry}{qw(from to)};if($p eq $from||index($p,$from.'/')==0){$p=$to.substr($p,length($from));last;}}
  my $n=normal_path($p);
  if($current){my ($a,$b)=(abs_path($p),abs_path($n));need(defined($a)&&defined($b)&&$a eq $b);}
  $found{$n}=1;
 }
 return [sort keys %found];
}
sub pin_shape {
 my ($pin,$with_path)=@_;need(keys_are($pin,$with_path?qw(path sha256 bytes):qw(sha256 bytes)));
 need(defined($pin->{sha256})&&!ref($pin->{sha256})&&$pin->{sha256}=~/\A[0-9a-f]{64}\z/);
 need(defined($pin->{bytes})&&!ref($pin->{bytes})&&$pin->{bytes}=~/\A[1-9][0-9]*\z/&&$pin->{bytes}<=$file_limit);
 if($with_path){need(defined($pin->{path})&&!ref($pin->{path})&&$pin->{path}=~m{\A/inputs/}&&normal_path($pin->{path}) eq $pin->{path});}
}
sub matches_pin {my ($row,$pin)=@_;return $row->{sha256} eq $pin->{sha256}&&$row->{bytes}==$pin->{bytes};}
sub stable_paths {
 my ($name,$paths)=@_;my $start=now();my ($previous,$series,$count,$table)=(undef,$start,0,undef);my %resolutions;
 while(1){
  $table={};%resolutions=();
  for my $p(@$paths){my ($real,$row)=read_regular($p,0);need(!exists($held{$real})||same($held{$real},$row));
   need(!exists($logical_guards{$p})||$logical_guards{$p} eq $real);$resolutions{$p}=$real;
   if($real=~m{\A/(?:work|old)/}){need(exists($expected->{$real})&&matches_pin($row,$expected->{$real}));}
   else{need($real=~m{\A/(?:inputs|baseline|outputs|controls|usr|lib|lib64)/});}
   $table->{$real}=$row;
  }
  my $current=now();need($current-$start<30);push @observations,{step=>$name,elapsedMs=>int(($current-$start)*1000),table=>$table,resolvedPaths=>{%resolutions}};
  my $sample={table=>$table,resolvedPaths=>{%resolutions}};
  if(defined($previous)&&same($previous,$sample)){++$count;}else{$count=1;$series=$current;}
  last if $count>=3&&$current-$series>=2;$previous=$sample;sleep(1);
 }
 for my $p(keys %$table){need(!exists($held{$p})||same($held{$p},$table->{$p}));$held{$p}=$table->{$p};}
 for my $p(keys %resolutions){need(!exists($logical_guards{$p})||$logical_guards{$p} eq $resolutions{$p});$logical_guards{$p}=$resolutions{$p};}
 return $table;
}
sub stable_new {
 my ($name,$paths,$mode,$uid,$gid)=@_;my %known;
 for my $p(@$paths){need(!exists($held{$p})&&abs_path($p) eq $p);need(chmod($mode,$p)==1);my $r=material($p);$known{$p}={sha256=>$r->{sha256},bytes=>$r->{bytes}};}
 my $start=now();my ($previous,$series,$count,$table)=(undef,$start,0,undef);
 while(1){$table={};
  for my $p(@$paths){my @s=lstat($p);need(@s&&S_ISREG($s[2])&&$s[3]==1&&$s[4]==$uid&&$s[5]==$gid&&($s[2]&07777)==$mode);
   my $r=material($p);need(matches_pin($r,$known{$p}));$table->{$p}=$r;}
  my $current=now();need($current-$start<30);
  push @observations,{step=>$name,elapsedMs=>int(($current-$start)*1000),table=>$table};
  if(defined($previous)&&same($previous,$table)){++$count;}else{$count=1;$series=$current;}
  last if $count>=3&&$current-$series>=2;$previous=$table;sleep(1);
 }
 for my $p(@$paths){need(same($table->{$p},material($p)));$held{$p}=$table->{$p};$logical_guards{$p}=$p;}
 return $table;
}
sub checked_call {my ($name,$argv,$wanted)=@_;fence();my $body=call($name,$argv,$wanted);fence();return $body;}
sub flags_ok {
 my ($argv)=@_;need(ref($argv) eq 'ARRAY'&&@$argv>=1&&@$argv<=512&&$argv->[0] eq '/usr/bin/clang++');
 for my $word(@$argv){need(defined($word)&&!ref($word)&&length($word)<=8192&&$word!~/[\0\r\n]/&&$word!~/\A@/);
  need($word!~/\A(?:-c|-o|-M|-MM|-MD|-MMD|-MP|-MF|-MT|-MQ|-MJ|-E|-S|-fsyntax-only)\z/&&$word!~/\A(?:-MF|-MT|-MQ|-MJ).+/);
  need($word!~/\A(?:-save-temps|-ftime-trace|-fprofile|-fmodules-cache-path|-serialize-diagnostics|-fplugin|-load)/&&$word!~/(?:^|,)@/);
 }
}
sub dependency_map {
 my ($map)=@_;need(ref($map) eq 'ARRAY'&&@$map<=2);my %seen;
 for my $e(@$map){need(keys_are($e,qw(from to))&&defined($e->{from})&&defined($e->{to})&&!ref($e->{from})&&!ref($e->{to}));
  need(($e->{from} eq '/overlay'&&$e->{to} eq '/work')||($e->{from} eq '/outputs/third_party'&&$e->{to} eq '/old/third_party'));need(!$seen{$e->{from}}++);
 }
}
sub elf_check {
 my ($path,$machine)=@_;my $raw=read_bytes($path,$file_limit);need(length($raw)>=64&&substr($raw,0,7) eq "\x7fELF\x02\x01\x01");
 my ($type,$em,$version)=unpack('vvV',substr($raw,16,8));need(($type==2||$type==3)&&$em==$machine&&$version==1);
 need(unpack('v',substr($raw,52,2))==64);
}
sub loader_paths {
 my ($text)=@_;need(length($text)&&$text!~/not found/);my %paths;
 for my $line(split(/\n/,$text)){next if $line=~/\A\s*linux-vdso(?:[.]so)?[^\s]*\s+\(0x[0-9a-f]+\)\s*\z/;
  my $p;if($line=~/\A\s*[^\s]+\s+=>\s+(\/[^\s]+)\s+\(0x[0-9a-f]+\)\s*\z/){$p=$1;}
  elsif($line=~/\A\s*(\/[^\s]+)\s+\(0x[0-9a-f]+\)\s*\z/){$p=$1;}else{need(0);}
  need($p=~m{\A/(?:old|usr|lib|lib64)/});$paths{$p}=1;
 }
 need(keys(%paths)>0);return [sort keys %paths];
}
sub json_refusal {
 my ($role,$stdout,$stderr)=@_;need($stderr eq '');my $got=eval{$J->decode($stdout)};need(!$@&&ref($got) eq 'HASH');my $want;
 if($role eq 'bootstrapImport'){$want={format=>'nano-isolated-fixture-private-binding-audit-v1',passed=>$no,refusedStage=>'arguments',errorDetailsSuppressed=>$yes,newSignaturesCreated=>0,signerCallbacksInvoked=>0,runtimeStarted=>$no,networkLaunched=>$no,productionQualified=>$no};}
 elsif($role eq 'bootstrapAudit'){$want={format=>'nano-isolated-bootstrap-physical-audit-v1',passed=>$no,refusedStage=>'arguments',diagnostic=>'expected manifest plan anchors source-inventory owner dataRoot witnessRoot',newSignaturesCreated=>0,journalWritesRequested=>$no,runtimeStarted=>$no,productionQualified=>$no};}
 elsif($role eq 'stoppedCapture'){$want={format=>'nano-isolated-stopped-recovery-pins-v1',passed=>$no,refusedStage=>'arguments',diagnostic=>'usage: --capture public-manifest public-runtime-config external-stop-receipt dataOwnerRoot witnessOwnerRoot owner',newSignaturesCreated=>0,journalWritesRequested=>$no,runtimeStarted=>$no,productionQualified=>$no};}
 else{need(0);}need(same($got,$want));
}

# Narrow Root operation. No old test main is retained or invoked here.
sub write_binary {
 my ($path,$raw)=@_;need($output_admitted&&$path=~m{\A/outputs/[A-Za-z0-9_.-]+\z});
 sysopen(my $f,$path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "Pruning operation refused\n";
 my $at=0;while($at<length($raw)){tick();my $n=syswrite($f,$raw,length($raw)-$at,$at);need(defined($n)&&$n>0);$at+=$n;}
 $f->sync or die "Pruning operation refused\n";close($f) or die "Pruning operation refused\n";
}
sub archive_members {
 my ($raw)=@_;need(substr($raw,0,8) eq "!<arch>\n");my ($at,$strings)=(8,undef);my @members;my %names;
 while($at<length($raw)){
  tick();need($at+60<=length($raw));my $header=substr($raw,$at,60);need(substr($header,58,2) eq "`\n");
  my $name=substr($header,0,16);$name=~s/ +\z//;my $size=substr($header,48,10);$size=~s/ +\z//;need($size=~/\A[0-9]+\z/);
  my $length=0+$size;my $body_at=$at+60;my $next=$body_at+$length+($length%2);need($length<=$file_limit&&$next>$at&&$next<=length($raw));my $body=substr($raw,$body_at,$length);
  if($name eq '//'){need(!defined($strings));$strings=$body;}
  elsif($name ne '/'&&$name ne '/SYM64/'){
   if($name=~m{\A#1/([0-9]+)\z}){my $n=0+$1;need($n>0&&$n<=length($body));$name=substr($body,0,$n);$name=~s/\0+\z//;$body=substr($body,$n);}
   elsif($name=~m{\A/([0-9]+)\z}){need(defined($strings));my $offset=0+$1;need($offset<length($strings));my $end=index($strings,"/\n",$offset);need($end>=$offset);$name=substr($strings,$offset,$end-$offset);}
   else{$name=~s{/\z}{};}
   unless($name eq '__.SYMDEF'||$name eq '__.SYMDEF SORTED'){
    need($name=~/\A[A-Za-z0-9_.+-]+[.]o\z/&&!$names{$name}++&&length($body)>0);
    push @members,{name=>$name,bytes=>length($body),sha256=>Digest::SHA::sha256_hex($body)};
   }
  }
  $at=$next;
 }
 need($at==length($raw)&&@members==143);return \@members;
}
sub compile_unit {
 my ($label,$source,$flags,$object,$uid,$gid)=@_;my $md=$object.'.d';
 $stage=$label.'-independent-m';++$counts{independentMCommands};my $m=checked_call($stage,[@$flags,'-M','-MT',$object,'-o','/dev/stdout',$source]);
 my $literal=deps($m);my $normalized=normalized_deps($literal,[],1);need(grep($_ eq $source,@$normalized)==1);
 stable_paths($label.'-dependencies',$normalized);fence();
 $stage=$label.'-compile';++$counts{compiledTranslationUnits};checked_call($stage,[@$flags,'-MD','-MF',$md,'-MT',$object,'-o',$object,'-c',$source]);
 stable_new($label.'-object-md',[$object,$md],0400,$uid,$gid);my $actual=deps(read_bytes($md));my $actual_normalized=normalized_deps($actual,[],1);need(same($normalized,$actual_normalized));
 return {source=>$source,objectPath=>$object,object=>hold($object),dependencyPath=>$md,dependencyFile=>hold($md),dependencyMode=>'MD',currentMLiteralDependencies=>scalar(@$literal),currentMNormalizedDependencies=>scalar(@$normalized),actualMdLiteralDependencies=>scalar(@$actual),actualMdNormalizedDependencies=>scalar(@$actual_normalized),literalMEqualsMd=>same($literal,$actual)?$yes:$no,normalizedMEqualsMd=>$yes};
}
sub empty_tmp {
 need(abs_path('/tmp') eq '/tmp');opendir(my $d,'/tmp') or die "Pruning operation refused\n";
 my @names=grep {$_ ne '.'&&$_ ne '..'}readdir($d);closedir($d);need(!@names);
}
sub timed_test {
 my ($label,$argv,$seconds,$wanted,$stderr)=@_;$wanted//=0;$stderr//='';empty_tmp();my ($raw,$error);my $saved=$deadline;
 eval{my $until=now()+$seconds;$deadline=$until if $until<$deadline;$raw=checked_call($label,$argv,$wanted);1;}or $error=$@;
 $deadline=$saved;die $error if defined($error)&&length($error);need(read_bytes($out.'/'.$label.'-stderr.private.log') eq $stderr);empty_tmp();return $raw;
}
sub startup_summary {
 my ($raw)=@_;need($raw=~s/\n\z//&&$raw!~/\r/);my @lines=split(/\n/,$raw,-1);need(@lines==4);my @rows;
 for my $line(@lines){my $r=eval{$J->decode($line)};need(!$@&&ref($r) eq 'HASH');push @rows,$r;}
 my @details=(
  {scenario=>'maintenance-flags-before-public-bootstrap-io',expectedRefusals=>18,invalidJsonTypesPerFlag=>6,witnessCreated=>$no,executionCheckpointPublished=>$no},
  {scenario=>'maintenance-flags-profile-and-cancellation-precedence',expectedRefusals=>27,missingProfileMaskedByFlag=>3,orderedFlagDiagnostics=>3,callerCancellationTypePreserved=>3},
  {scenario=>'maintenance-flags-accepted-types-preserve-profile-validation',expectedRefusals=>31,validAbsentFalseTruePolicies=>3,badPublicProfileCommitmentRefused=>$yes,clockSourceCalls=>0});
 for my $i(0..2){my $want={%{$details[$i]},result=>'PASS',newSignaturesCreated=>0,privateInputsRead=>$no,runtimeUTCQualified=>$no,runtimeDutiesResumed=>$no,productionQualified=>$no};need(same($rows[$i],$want));}
 my %last=%{$rows[3]};my $elapsed=delete $last{elapsedMs};my $rss=delete $last{maxRssKiB};need(defined($elapsed)&&!ref($elapsed)&&$elapsed=~/\A[0-9]+\z/&&$elapsed<=45000&&defined($rss)&&!ref($rss)&&$rss=~/\A[0-9]+\z/&&$rss<=131072);
 my $want={format=>'nano-capella-maintenance-startup-validation-v1',scenario=>'capella-maintenance-flags-self-test',result=>'PASS',groups=>3,expectedRefusals=>31,originalPrimaryLogicalRowsUnchanged=>$yes,freshWitnessCreated=>$no,completedTestRuntimeStarts=>0,clockSourceCalls=>0,legacySuiteExecuted=>$no,publicFixturesOnly=>$yes,newSignaturesCreated=>0,privateInputsRead=>$no,networkUsed=>$no,runtimeUTCQualified=>$no,runtimeDutiesResumed=>$no,productionQualified=>$no};need(same(\%last,$want));return $rows[3];
}
sub fixture_inputs {
 my $root='/work/tests/fixtures/capella-bootstrap-anchor-cb532';my $manifest=$root.'/manifest.json';
 need(exists($expected->{$manifest})&&$expected->{$manifest}{sha256} eq 'ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6');
 stable_paths('startup-fixture-manifest',[$manifest]);my $j=$J->decode(read_bytes($manifest));my @paths=($manifest);
 for my $key(qw(profile initial genesis genesisState)){
  my $r=$j->{files}{$key};need(ref($r) eq 'HASH'&&leaf($r->{path}));my $path=$root.'/'.$r->{path};need(exists($expected->{$path})&&$expected->{$path}{sha256} eq $r->{sha256}&&$expected->{$path}{bytes}==$r->{bytes});push @paths,$path;
 }
 stable_paths('startup-public-fixture',\@paths);return \@paths;
}

sub bounded_unsigned {
 my ($value,$maximum)=@_;need(defined($value)&&!ref($value)&&$value=~/\A[0-9]+\z/&&length($value)<=16&&$value<=$maximum);return 0+$value;
}
sub compact_report {
 my ($r,$schema)=@_;need(keys_are($r,qw(schema rocksdbVersion ownedCopyOnly beforeFiles afterFiles logicalBefore logicalAfter logicalContentUnchanged physicalPowerLoss productionMaintenanceQualified)));
 need($r->{schema} eq $schema&&$r->{rocksdbVersion}=~/\A[0-9]+[.][0-9]+[.][0-9]+\z/&&same($r->{ownedCopyOnly},$yes)&&same($r->{logicalContentUnchanged},$yes)&&same($r->{physicalPowerLoss},$no)&&same($r->{productionMaintenanceQualified},$no));
 for my $phase(qw(beforeFiles afterFiles)){my $f=$r->{$phase};need(keys_are($f,qw(files apparentBytes allocatedBytes sstBytes walBytes)));bounded_unsigned($f->{files},4096);for my $key(qw(apparentBytes allocatedBytes sstBytes walBytes)){bounded_unsigned($f->{$key},100663296);}need($f->{sstBytes}<=$f->{apparentBytes}&&$f->{walBytes}<=$f->{apparentBytes});}
 for my $phase(qw(logicalBefore logicalAfter)){my $l=$r->{$phase};need(keys_are($l,qw(entries keyValueBytes sha256FramedOrderedKeyValues)));bounded_unsigned($l->{entries},4096);bounded_unsigned($l->{keyValueBytes},33554432);need($l->{sha256FramedOrderedKeyValues}=~/\A[0-9a-f]{64}\z/);}
 need(same($r->{logicalBefore},$r->{logicalAfter}));return $r;
}
sub operation_sst_test {
 my $raw=timed_test('operation-sst-reclaim',['/outputs/nano-owned-copy-physical-admission-tests','--pruning-sst-reclaim'],120);
 need($raw=~s/\n\z//&&$raw!~/\r/);my @lines=split(/\n/,$raw,-1);need(@lines==4);my @rows;
 for my $line(@lines){my $r=eval{$J->decode($line)};need(!$@&&ref($r) eq 'HASH');push @rows,$r;}
 my $common={format=>'nano-pruning-sst-reclaim-scenario-v1',passed=>$yes,fixtureKeyPublic=>$yes,newOperationalSignaturesCreated=>0,runtimeDutiesResumed=>$no,productionQualified=>$no};
 my %first=%{$rows[0]};my $baseline=delete $first{baselineCli};compact_report($baseline,'nano/1');need($baseline->{afterFiles}{sstBytes}>=1048576);
 need(same(\%first,{%$common,scenario=>'real-sstore-and-compacted-baseline',blocks=>64,fixtureTransactions=>64,fixtureEcdsaSignatures=>64,sstoreWrites=>65536,liveStorageSlots=>1024}));
 need(same($rows[1],{%$common,scenario=>'bounded-retention-and-matched-control',retentionBatches=>16,controlCheckpointBatches=>16,maximumDeletedStatesPerBatch=>4,deletedStates=>63,retentionBefore=>64,retentionAnchor=>64,allOtherLogicalRowsCommitted=>$yes}));
 my %third=%{$rows[2]};my $control=delete $third{controlCli};my $pruned=delete $third{prunedCli};compact_report($control,'nano/1');compact_report($pruned,'nano/2-state-pruned');
 my $sst=delete $third{sstReclaimedBytes};my $allocated=delete $third{allocatedFileBytesReclaimed};bounded_unsigned($sst,100663296);bounded_unsigned($allocated,100663296);
 need($sst>=65536&&$allocated>=65536&&$control->{afterFiles}{sstBytes}-$pruned->{afterFiles}{sstBytes}==$sst&&$control->{afterFiles}{allocatedBytes}-$pruned->{afterFiles}{allocatedBytes}==$allocated);
 need(same(\%third,{%$common,scenario=>'sst-reclaim-and-cold-logical-invariants',originalSourceUnchanged=>$yes,coldStateRootsAndReceiptsVerified=>$yes,filesystemFileAllocationOnly=>$yes,hostOrDockerBackingSpaceGuaranteed=>$no}));
 my %last=%{$rows[3]};my $elapsed=delete $last{elapsedMs};my $rss=delete $last{maxRssKiB};bounded_unsigned($elapsed,120000);bounded_unsigned($rss,131072);need(delete($last{sstReclaimedBytes})==$sst&&delete($last{allocatedFileBytesReclaimed})==$allocated);
 my $want={format=>'nano-pruning-sst-reclaim-v1',passed=>$yes,scenarioRows=>3,actualCliInvocations=>3,fixtureBlocks=>64,fixtureTransactions=>64,fixtureEcdsaSignatures=>64,fixtureKeyPublic=>$yes,sstoreWrites=>65536,liveStorageSlots=>1024,retentionBatches=>16,controlCheckpointBatches=>16,deletedStates=>63,logicalMaximumBytes=>33554432,logicalMaximumKeys=>4096,fixtureWriteBufferBytes=>4194304,fixtureSharedMemtableBytes=>8388608,fixtureBlockCacheBytes=>4194304,minimumBaselineSstBytes=>1048576,minimumReclaimBytes=>65536,rawStateDbWritesOrDeletes=>$no,oldPhysicalAdmissionCasesReplayed=>$no,storageSourceModified=>$no,runtimeStarted=>$no,networkUsed=>$no,physicalPowerLossQualified=>$no,fullEvmPruningQualified=>$no,productionQualified=>$no};need(same(\%last,$want));return $rows[3];
}

sub operation_main {
 need(@ARGV==1&&$ARGV[0] eq '--pruning-operation');
 %ENV=(PATH=>'/usr/bin:/bin',HOME=>'/outputs',TMPDIR=>'/tmp',LANG=>'C',LC_ALL=>'C');
 my $recipe=$J->decode(read_bytes('/controls/recipes.json'));$expected=$J->decode(read_bytes('/controls/source-expected.private.json'));my $core_pin=$J->decode(read_bytes('/controls/core-pin.json'));
 need(keys_are($recipe,qw(format owner uid gid imageId elfMachine sourceRevision runtime programs))&&$recipe->{format} eq 'nano-linux-pruning-operation-recipes-v1'&&ref($expected) eq 'HASH');pin_shape($core_pin,0);
 need(defined($recipe->{sourceRevision})&&!ref($recipe->{sourceRevision})&&$recipe->{sourceRevision}=~/\A[0-9a-f]{40}\z/);
 my ($uid,$gid)=@{$recipe}{qw(uid gid)};my $arm=$recipe->{owner} eq 'mac-docker';my $amd=$recipe->{owner} eq 'vps-docker';need($arm||$amd);
 need($arm?($uid==501&&$gid==20&&$recipe->{elfMachine}==183&&$recipe->{imageId} eq 'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'):($uid==1000&&$gid==1000&&$recipe->{elfMachine}==62&&$recipe->{imageId} eq 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'));
 my $known_core=$arm?{sha256=>'e2127db4aa30c39fc0891eb22f008410898717ce75c7aff8d46a25d62345499d',bytes=>41053616}:{sha256=>'e49483a58f71b9b66f83a88b7c5631f6657d77fcae3c85bc57424fbe6bcbed92',bytes=>40819792};need($core_pin->{sha256} eq $known_core->{sha256}&&$core_pin->{bytes}==$known_core->{bytes});
 need($>==$uid&&$<==$uid&&abs_path($out) eq $out);my @s=lstat($out);need(@s&&S_ISDIR($s[2])&&($s[2]&07777)==0700&&$s[4]==$uid&&$s[5]==$gid);
 opendir(my $d,$out) or die "Pruning operation refused\n";my @existing=grep {$_ ne '.'&&$_ ne '..'}readdir($d);closedir($d);need(!@existing);$output_admitted=1;
 need(keys(%$expected)>0&&keys(%$expected)<=20000);
 for my $p(sort keys %$expected){need($p=~m{\A/(?:work|old)/}&&normal_path($p) eq $p&&abs_path($p) eq $p);pin_shape($expected->{$p},0);}
 my $rt=$recipe->{runtime};need(keys_are($rt,qw(source compileFlags))&&$rt->{source} eq '/work/src/consensus_capella_runtime.cpp');flags_ok($rt->{compileFlags});need(!grep {index($_,'NANO_')>=0}@{$rt->{compileFlags}});
 my $runtime_pin={sha256=>'57f18bb906274d62a8fc93f86059d0b28218660fc96129966dec9329c9761232',bytes=>28682};
 my $sst_pin={sha256=>'c18cc1b386bc65802a1ed1105f63765378b9885ff4feb6d3d04b3f6872bfdbd3',bytes=>28547};
 my $maintenance_pin={sha256=>'0d17feb4c32be71886a4dce875176498c3d7b3cf5bb0bf7e75f3866275c4435e',bytes=>12093};
 my $startup_pin={sha256=>'11fb02a13b2e80f5d4a11fab5336ba4492737fc0771b4745fb1b67c3777e4ad6',bytes=>24875};
 need(exists($expected->{$rt->{source}})&&same($expected->{$rt->{source}},$runtime_pin));
 my %sources=(startupValidation=>'/work/tests/test_capella_startup_deadline.cpp',sstReclaim=>'/work/tests/test_owned_copy_physical_admission.cpp',maintenance=>'/work/tools/storage-maintenance/compact-owned-copy.cpp');
 my %targets=(startupValidation=>'nano-capella-startup-deadline-tests',sstReclaim=>'nano-owned-copy-physical-admission-tests',maintenance=>'nano-storage-maintenance');
 need(ref($recipe->{programs}) eq 'ARRAY'&&@{$recipe->{programs}}==3);my (%programs,@initial);
 for my $p(@{$recipe->{programs}}){need(keys_are($p,qw(role source compileFlags linkPrefix linkTail)));my $role=$p->{role};need(defined($role)&&!ref($role)&&exists($sources{$role})&&!exists($programs{$role})&&$p->{source} eq $sources{$role});$programs{$role}=$p;flags_ok($p->{compileFlags});flags_ok($p->{linkPrefix});need(exists($expected->{$p->{source}}));push @initial,$p->{source};
  my @macro=grep {index($_,'NANO_')>=0}@{$p->{compileFlags}};
  if($role eq 'startupValidation'){need(@macro==1&&$macro[0] eq '-DNANO_TEST_SOURCE="/work"'&&same($expected->{$p->{source}},$startup_pin));}
  elsif($role eq 'sstReclaim'){need(same($expected->{$p->{source}},$sst_pin));need(@macro==2&&grep($_ eq '-DNANO_TEST_COMPACT="/outputs/nano-storage-maintenance"',@macro)==1&&grep($_ eq '-DNANO_TEST_MIGRATE="/outputs/nano-pruned-schema-migration"',@macro)==1);}
  else{need(!@macro&&same($expected->{$p->{source}},$maintenance_pin));}
  my $tail=$p->{linkTail};need(ref($tail) eq 'ARRAY'&&@$tail&&@$tail<=256&&grep($_ eq '/outputs/libnano_core.a',@$tail)==1);
  for my $word(@$tail){need(defined($word)&&!ref($word)&&$word!~/[\0\r\n]/&&$word!~/(?:^|,)@/&&$word=~m{\A(?:/outputs/libnano_core[.]a\z|/(?:old|usr|lib|lib64)/|-)});need($word!~/\A(?:-o|-c|-M|-MM|-MD|-MMD|-MF|-MT|-MQ|-MJ)\z/);push @initial,$word if $word=~m{\A/}&&$word ne '/outputs/libnano_core.a';}
  need(scalar(grep {/\.a\z/}@$tail)==5&&scalar(grep {/\.o\z/}@$tail)==2);
 }
 push @initial,$rt->{source},'/work/CMakeLists.txt','/controls/worker.pl','/controls/recipes.json','/controls/source-expected.private.json','/controls/core-pin.json','/usr/bin/clang++','/usr/bin/stat','/usr/bin/ldd','/usr/bin/ar','/baseline/libnano_core.a';my %once;@initial=grep {!$once{$_}++}@initial;
 $stage='operation-initial-admission';stable_paths('operation-initial-inputs',\@initial);fence();need(matches_pin($held{'/baseline/libnano_core.a'},$known_core));
 my $cmake=read_bytes('/work/CMakeLists.txt');
 for my $line('add_executable(nano-storage-maintenance tools/storage-maintenance/compact-owned-copy.cpp)','target_link_libraries(nano-storage-maintenance PRIVATE nano_core)','add_executable(nano-capella-startup-deadline-tests tests/test_capella_startup_deadline.cpp)','target_link_libraries(nano-capella-startup-deadline-tests PRIVATE nano_core)','add_executable(nano-owned-copy-physical-admission-tests tests/test_owned_copy_physical_admission.cpp)','target_link_libraries(nano-owned-copy-physical-admission-tests PRIVATE nano_core)'){my @matches=($cmake=~/^\s*\Q$line\E\s*$/mg);need(@matches==1);}
 my $fixtures=fixture_inputs();my @products;
 my $runtime=compile_unit('runtime',$rt->{source},$rt->{compileFlags},'/outputs/consensus_capella_runtime.cpp.o',$uid,$gid);push @products,$runtime;
 $stage='core-archive-copy';my $old_raw=read_bytes('/baseline/libnano_core.a',$file_limit);my $old_members=archive_members($old_raw);need(grep($_->{name} eq 'consensus_capella_runtime.cpp.o',@$old_members)==1);write_binary('/outputs/libnano_core.a',$old_raw);need(matches_pin(material('/outputs/libnano_core.a'),$known_core));
 $stage='core-archive-update';++$counts{archiveUpdates};checked_call($stage,['/usr/bin/ar','rcsD','/outputs/libnano_core.a','/outputs/consensus_capella_runtime.cpp.o']);my $new_members=archive_members(read_bytes('/outputs/libnano_core.a',$file_limit));my $other=0;
 for my $i(0..142){my $old=$old_members->[$i];my $new=$new_members->[$i];need($old->{name} eq $new->{name});if($old->{name} eq 'consensus_capella_runtime.cpp.o'){need(matches_pin($runtime->{object},$new));}else{need(same($old,$new));++$other;}}
 need($other==142);my $new_core=stable_new('operation-core-archive',['/outputs/libnano_core.a'],0400,$uid,$gid)->{'/outputs/libnano_core.a'};
 save('core-member-replacement.private.json',{oldArchive=>$held{'/baseline/libnano_core.a'},newArchive=>$new_core,oldMembers=>$old_members,newMembers=>$new_members,memberCount=>143,otherMembersByteExact=>142,replacement=>$runtime});
 for my $role(qw(startupValidation sstReclaim maintenance)){
  my $p=$programs{$role};my $record=compile_unit($role,$p->{source},$p->{compileFlags},$out.'/'.$role.'.cpp.o',$uid,$gid);my $exe=$out.'/'.$targets{$role};
  $stage=$role.'-link';++$counts{linkCommands};checked_call($stage,[@{$p->{linkPrefix}},$record->{objectPath},'-o',$exe,@{$p->{linkTail}}]);my $software=stable_new($role.'-software',[$exe],0500,$uid,$gid)->{$exe};elf_check($exe,$recipe->{elfMachine});
  $stage=$role.'-loader';++$counts{loaderCommands};my $ldd=checked_call($stage,['/usr/bin/ldd',$exe]);need(read_bytes($out.'/'.$stage.'-stderr.private.log') eq '');stable_paths($role.'-runtime-inputs',loader_paths($ldd));
  $record->{softwarePath}=$exe;$record->{software}=$software;$record->{loaderPassed}=$yes;
  if($role eq 'startupValidation'){
   my $baseline_exe='/outputs/nano-capella-startup-deadline-baseline';my @tail=map {$_ eq '/outputs/libnano_core.a'?'/baseline/libnano_core.a':$_}@{$p->{linkTail}};
   $stage='startup-baseline-link';++$counts{linkCommands};checked_call($stage,[@{$p->{linkPrefix}},$record->{objectPath},'-o',$baseline_exe,@tail]);my $old=stable_new('startup-baseline-software',[$baseline_exe],0500,$uid,$gid)->{$baseline_exe};elf_check($baseline_exe,$recipe->{elfMachine});$record->{baselineSoftwarePath}=$baseline_exe;$record->{baselineSoftware}=$old;
  }
  push @products,$record;
 }
 $stage='operation-startup-baseline';my $baseline_stdout=timed_test('operation-startup-baseline',['/outputs/nano-capella-startup-deadline-baseline','--maintenance-flags-self-test'],45,1,"maintenance refusal created witness or execution checkpoint\n");need($baseline_stdout eq '');
 my $baseline={expectedExitCode=>1,observedExitCode=>1,stderrExact=>"maintenance refusal created witness or execution checkpoint\n",stdoutEmpty=>$yes,startupObjectCompiledOnce=>$yes,oldFiveGroupsExecuted=>$no};
 $stage='operation-startup-validation';my $startup=startup_summary(timed_test('operation-startup-validation',['/outputs/nano-capella-startup-deadline-tests','--maintenance-flags-self-test'],45));
 my $sst=operation_sst_test();
 $stage='operation-final-fences';fence();need(same($new_core,material('/outputs/libnano_core.a')));
 need($counts{independentMCommands}==4&&$counts{compiledTranslationUnits}==4&&$counts{linkCommands}==4&&$counts{loaderCommands}==3&&$counts{archiveUpdates}==1&&$counts{argumentRefusals}==0);
 save('sdk-and-inputs.private.json',\%held);save('first-dependency-observations.private.json',\@observations);
 my $admission={format=>'nano-linux-pruning-operation-software-binding-v1',owner=>$recipe->{owner},imageId=>$recipe->{imageId},sourceRevisionDeclared=>$recipe->{sourceRevision},originalCore=>$held{'/baseline/libnano_core.a'},newCore=>$new_core,products=>\@products,publicFixtureInputs=>$fixtures,guestMaterials=>\%held,logicalResolutionGuards=>\%logical_guards,startupBaseline=>$baseline,startupSummary=>$startup,sstSummary=>$sst,oldAuthoritiesRefreshed=>$no};save('software-admission.private.json',$admission);my $binding=material('/outputs/software-admission.private.json');
 my @public=map {my $p=$_;my %v=map {$_=>$p->{$_}}qw(source dependencyMode currentMLiteralDependencies currentMNormalizedDependencies actualMdLiteralDependencies actualMdNormalizedDependencies literalMEqualsMd normalizedMEqualsMd);$v{objectSha256}=$p->{object}{sha256};$v{objectBytes}=$p->{object}{bytes};if(exists($p->{software})){$v{softwareSha256}=$p->{software}{sha256};$v{softwareBytes}=$p->{software}{bytes};$v{softwarePath}=$p->{softwarePath};$v{loaderPassed}=$yes;}if(exists($p->{baselineSoftware})){$v{baselineSoftwareSha256}=$p->{baselineSoftware}{sha256};$v{baselineSoftwareBytes}=$p->{baselineSoftware}{bytes};}\%v;}@products;
 my $result={format=>'nano-linux-pruning-operation-qualification-v1',passed=>$yes,owner=>$recipe->{owner},imageId=>$recipe->{imageId},sourceRevisionDeclared=>$recipe->{sourceRevision},%counts,selectedTestRuns=>3,newPositiveModeRuns=>2,expectedOldCoreRefusals=>1,coreArchiveMembers=>143,otherCoreMembersByteExact=>142,replacedCoreMembers=>1,originalCoreArchiveMutated=>$no,newCoreArchiveCreated=>$yes,newCoreSha256=>$new_core->{sha256},newCoreBytes=>$new_core->{bytes},products=>\@public,startupBaseline=>$baseline,startupSummary=>$startup,sstSummary=>$sst,softwareAdmissionSha256=>$binding->{sha256},softwareAdmissionBytes=>$binding->{bytes},originalGuestInputsFullNineAndShaUnchanged=>$yes,retainedHostNineRecovered=>$no,oldAuthoritiesRefreshed=>$no,databaseOpened=>$yes,databaseScope=>'owned synthetic fixtures only',ownedTmpEmptyAfterTests=>$yes,oldSuitesExecuted=>$no,fullCmakeConfigureExecuted=>$no,fullLoaderClosureQualified=>$no,wholeSdkQualified=>$no,imageIdentityVerifiedByWorker=>$no,sourceGitProvenanceVerifiedByWorker=>$no,newSignaturesAreFixtureOnly=>$yes,campaignAdmission=>$no,networkStarted=>$no,runtimeDutiesResumed=>$no,commonFinalityQualified=>$no,productionQualified=>$no,steps=>\@steps};
 save('result-public.json',$result);print $J->encode($result)."\n" or die "Pruning operation refused\n";
}
my $ok=eval{operation_main();1;};
unless($ok){my $stopped=stop_group($active);local $SIG{TERM}='IGNORE';local $SIG{INT}='IGNORE';local $SIG{HUP}='IGNORE';my $post=$stopped?eval{fence();1;}:0;eval{save('worker-refusal.private.json',{passed=>$no,stage=>$stage,originalGuestInputsUnchanged=>$post?$yes:$no,ownProcessGroupCleanupComplete=>$stopped?$yes:$no,completedCommandEvents=>scalar(@steps),%counts});};print STDERR "Pruning operation refused\n";exit(1);}
exit(0);
