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

# Explicit new test-only mode. The established helper main below is unchanged.
sub rpc_tmp_empty {
 need(abs_path('/tmp') eq '/tmp');opendir(my $d,'/tmp') or die "Selected helpers worker refused\n";
 my @names=grep {$_ ne '.'&&$_ ne '..'} readdir($d);closedir($d);need(!@names);
}
sub rpc_summary {
 my ($raw)=@_;need($raw=~s/\n\z//&&$raw!~/\r/);my @lines=split(/\n/,$raw,-1);need(@lines==4);
 my @rows;for my $line(@lines){my $row=eval{$J->decode($line)};need(!$@&&ref($row) eq 'HASH');push @rows,$row;}
 my @names=('real-retention-before-versus-anchor-rpc','captured-rpc-state-survives-real-retention','retention-cold-reopen-and-rollback-floor');my @checks=(47,82,111);my @refusals=(6,12,20);
 for my $i(0..2){my $row=$rows[$i];need(keys_are($row,qw(scenario passed checks expectedRefusals))&&$row->{scenario} eq $names[$i]&&same($row->{passed},$yes)&&$row->{checks}==$checks[$i]&&$row->{expectedRefusals}==$refusals[$i]);}
 my $want=$J->decode('{"checks":112,"coreSourceModified":false,"expectedRefusals":20,"fixtureKeyPublic":true,"fixtureTransactions":6,"format":"nano-rpc-pruning-retention-boundary-v1","groups":3,"networkStarted":false,"oldSelectorSuiteExecuted":false,"passed":true,"productionQualified":false,"retentionAnchor":6,"retentionBefore":5,"runtimeDutiesResumed":false}');need(same($rows[3],$want));return $rows[3];
}
sub rpc_main {
 need(@ARGV==1&&$ARGV[0] eq '--rpc-pruning-current');
 %ENV=(PATH=>'/usr/bin:/bin',HOME=>'/outputs',TMPDIR=>'/tmp',LANG=>'C',LC_ALL=>'C');
 my $recipe=$J->decode(read_bytes('/controls/recipes.json'));$expected=$J->decode(read_bytes('/controls/source-expected.private.json'));my $core_pin=$J->decode(read_bytes('/controls/core-pin.json'));
 need(keys_are($recipe,qw(format owner uid gid imageId elfMachine sourceRevision sourceTreeEntries programs))&&$recipe->{format} eq 'nano-current-linux-rpc-test-recipes-v1'&&ref($expected) eq 'HASH');pin_shape($core_pin,0);
 need(defined($recipe->{sourceRevision})&&!ref($recipe->{sourceRevision})&&$recipe->{sourceRevision}=~/\A[0-9a-f]{40}\z/);
 my %tree_names=map {$_=>1} qw(include src network third_party tests/support tests/capella tests/multihost);my %trees;
 need(ref($recipe->{sourceTreeEntries}) eq 'ARRAY'&&@{$recipe->{sourceTreeEntries}}==7);
 for my $tree(@{$recipe->{sourceTreeEntries}}){need(keys_are($tree,qw(path oid))&&defined($tree->{path})&&defined($tree->{oid})&&!ref($tree->{path})&&!ref($tree->{oid})&&$tree_names{$tree->{path}}&&!$trees{$tree->{path}}++&&$tree->{oid}=~/\A[0-9a-f]{40}\z/);}
 my ($uid,$gid)=@{$recipe}{qw(uid gid)};my $arm=$recipe->{owner} eq 'mac-docker';my $amd=$recipe->{owner} eq 'vps-docker';need($arm||$amd);
 need($arm?($uid==501&&$gid==20&&$recipe->{elfMachine}==183&&$recipe->{imageId} eq 'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'):($uid==1000&&$gid==1000&&$recipe->{elfMachine}==62&&$recipe->{imageId} eq 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'));
 my $known_core=$arm?{sha256=>'e2127db4aa30c39fc0891eb22f008410898717ce75c7aff8d46a25d62345499d',bytes=>41053616}:{sha256=>'e49483a58f71b9b66f83a88b7c5631f6657d77fcae3c85bc57424fbe6bcbed92',bytes=>40819792};need(same($core_pin,$known_core));
 need($>==$uid&&$<==$uid&&abs_path($out) eq $out);my @s=lstat($out);need(@s&&S_ISDIR($s[2])&&($s[2]&07777)==0700&&$s[4]==$uid&&$s[5]==$gid);
 opendir(my $dir,$out) or die "Selected helpers worker refused\n";my @existing=grep {$_ ne '.'&&$_ ne '..'} readdir($dir);closedir($dir);need(!@existing);$output_admitted=1;
 need(ref($recipe->{programs}) eq 'ARRAY'&&@{$recipe->{programs}}==1);my $program=$recipe->{programs}[0];
 need(keys_are($program,qw(role source compileFlags linkPrefix linkTail reuse))&&$program->{role} eq 'rpcSimulationTest'&&$program->{source} eq '/work/tests/test_rpc_simulation_block_id.cpp'&&!defined($program->{reuse}));
 flags_ok($program->{compileFlags});flags_ok($program->{linkPrefix});need(!grep {index($_,'NANO_')>=0} (@{$program->{compileFlags}},@{$program->{linkPrefix}}));
 my $tail=$program->{linkTail};need(ref($tail) eq 'ARRAY'&&@$tail&&@$tail<=256&&grep($_ eq '/baseline/libnano_core.a',@$tail)==1);my @initial;
 for my $word(@$tail){need(defined($word)&&!ref($word)&&$word!~/[\0\r\n]/&&$word!~/(?:^|,)@/&&$word=~m{\A(?:/baseline/libnano_core[.]a\z|/(?:old|usr|lib|lib64)/|-)});need($word!~/\A(?:-o|-c|-M|-MM|-MD|-MMD|-MF|-MT|-MQ|-MJ)\z/);push @initial,$word if $word=~m{\A/};}
 need(scalar(grep {/\.a\z/} @$tail)==5&&scalar(grep {/\.o\z/} @$tail)==2);
 need(keys(%$expected)>0&&keys(%$expected)<=20000);
 for my $p(sort keys %$expected){need($p=~m{\A/(?:work|old)/}&&normal_path($p) eq $p&&abs_path($p) eq $p);pin_shape($expected->{$p},0);}
 my $source=$program->{source};my $source_pin={sha256=>'b8e7625d0dc3b34c2156a5e62180630442edc49cef922bf703bbb9e5a54fe491',bytes=>26990};need(exists($expected->{$source})&&same($expected->{$source},$source_pin));
 need(exists($expected->{'/work/CMakeLists.txt'}));my $cmake=read_bytes('/work/CMakeLists.txt');need(matches_pin(material('/work/CMakeLists.txt'),$expected->{'/work/CMakeLists.txt'}));
 for my $line('add_executable(nano-rpc-simulation-block-id-tests tests/test_rpc_simulation_block_id.cpp)','target_link_libraries(nano-rpc-simulation-block-id-tests PRIVATE nano_core)','add_test(NAME rpc_pruning_retention_boundaries COMMAND nano-rpc-simulation-block-id-tests --pruning-boundary-self-test)','set_tests_properties(rpc_pruning_retention_boundaries PROPERTIES TIMEOUT 45 LABELS "qualification_rpc_pruning_retention")'){my @matches=($cmake=~/^\Q$line\E$/mg);need(@matches==1);}
 push @initial,$source,'/work/CMakeLists.txt','/controls/recipes.json','/controls/source-expected.private.json','/controls/core-pin.json','/controls/worker.pl','/usr/bin/clang++','/usr/bin/stat','/usr/bin/ldd','/baseline/libnano_core.a';my %once;@initial=grep {!$once{$_}++}@initial;
 $stage='rpc-initial-input-admission';stable_paths('rpc-initial-inputs',\@initial);fence();my $core=$held{'/baseline/libnano_core.a'};need(defined($core)&&matches_pin($core,$known_core));
 my $flags=$program->{compileFlags};my $object=$out.'/rpcSimulationTest.cpp.o';my $md=$object.'.d';my $exe=$out.'/nano-rpc-simulation-block-id-tests';
 $stage='rpc-independent-m';++$counts{independentMCommands};my $m=checked_call('rpc-independent-m',[@$flags,'-M','-MT',$object,'-o','/dev/stdout',$source]);my $literal=deps($m);my $normalized=normalized_deps($literal,[],1);need(grep($_ eq $source,@$normalized)==1);stable_paths('rpc-compile-dependencies',$normalized);fence();
 $stage='rpc-compile';++$counts{compiledTranslationUnits};checked_call('rpc-compile',[@$flags,'-MD','-MF',$md,'-MT',$object,'-o',$object,'-c',$source]);stable_new('rpc-object-and-md',[$object,$md],0400,$uid,$gid);
 my $actual=deps(read_bytes($md));my $actual_normalized=normalized_deps($actual,[],1);my $literal_equal=same($literal,$actual)?$yes:$no;need(same($normalized,$actual_normalized));my $object_row=hold($object);my $md_row=hold($md);
 $stage='rpc-link';++$counts{linkCommands};checked_call('rpc-link',[@{$program->{linkPrefix}},$object,'-o',$exe,@$tail]);my $new=stable_new('rpc-software',[$exe],0500,$uid,$gid);my $exe_row=$new->{$exe};elf_check($exe,$recipe->{elfMachine});fence();
 $stage='rpc-loader';++$counts{loaderCommands};my $ldd=checked_call('rpc-ldd',['/usr/bin/ldd',$exe]);need(read_bytes($out.'/rpc-ldd-stderr.private.log') eq '');my $runtime=loader_paths($ldd);stable_paths('rpc-runtime-inputs',$runtime);fence();
 $stage='rpc-targeted-test';rpc_tmp_empty();my ($stdout,$error);my $saved_deadline=$deadline;
 eval{$deadline=now()+45 if $deadline>now()+45;$stdout=checked_call('rpc-targeted-test',[$exe,'--pruning-boundary-self-test']);1;}or $error=$@;
 $deadline=$saved_deadline;die $error if defined($error)&&length($error);need(read_bytes($out.'/rpc-targeted-test-stderr.private.log') eq '');my $summary=rpc_summary($stdout);rpc_tmp_empty();
 $stage='rpc-final-fences';fence();need(same($exe_row,material($exe))&&same($object_row,material($object))&&same($md_row,material($md))&&same($core,material('/baseline/libnano_core.a')));
 save('sdk-and-inputs.private.json',\%held);save('first-dependency-observations.private.json',\@observations);save('software-admission.private.json',{format=>'nano-current-linux-rpc-test-software-binding-v1',owner=>$recipe->{owner},imageId=>$recipe->{imageId},sourceRevision=>$recipe->{sourceRevision},sourceTreeEntriesDeclared=>$recipe->{sourceTreeEntries},sourceTranslationUnit=>$source_pin,coreArchive=>$core,object=>$object_row,dependencyFile=>$md_row,software=>$exe_row,softwarePath=>$exe,guestMaterials=>\%held,logicalResolutionGuards=>\%logical_guards,rpcTargetedSummary=>$summary,originalGuestInputsFullNineAndShaUnchanged=>$yes,retainedHostNineRecovered=>$no,oldAuthoritiesRefreshed=>$no});
 my $binding=material('/outputs/software-admission.private.json');my $result={format=>'nano-current-linux-rpc-simulation-block-id-qualification-v1',passed=>$yes,owner=>$recipe->{owner},uid=>0+$uid,gid=>0+$gid,imageId=>$recipe->{imageId},imageIdentityVerifiedByWorker=>$no,sourceRevisionDeclared=>$recipe->{sourceRevision},sourceGitProvenanceVerifiedByWorker=>$no,sourceTreeEntriesDeclared=>$recipe->{sourceTreeEntries},sourceTranslationUnit=>$source_pin,selectedTargetCmakeDeclarationsChecked=>$yes,fullCmakeConfigureExecuted=>$no,%counts,rpcTargetedRuns=>1,rpcTargetedSummary=>$summary,currentMLiteralDependencies=>scalar(@$literal),currentMNormalizedDependencies=>scalar(@$normalized),actualMdLiteralDependencies=>scalar(@$actual),actualMdNormalizedDependencies=>scalar(@$actual_normalized),literalMEqualsMd=>$literal_equal,normalizedMEqualsMd=>$yes,dependencyMode=>'MD',objectSha256=>$object_row->{sha256},objectBytes=>$object_row->{bytes},softwareSha256=>$exe_row->{sha256},softwareBytes=>$exe_row->{bytes},softwareAdmissionSha256=>$binding->{sha256},softwareAdmissionBytes=>$binding->{bytes},coreSha256=>$core->{sha256},coreBytes=>$core->{bytes},coreArchiveMutated=>$no,coreMembersRequalified=>$no,knownCorePreviouslyQualifiedMembers=>143,linkArchiveInputs=>5,linkAssemblyInputs=>2,loaderPassed=>$yes,fullLoaderClosureQualified=>$no,lazySymbolsQualified=>$no,wholeSdkQualified=>$no,databaseOpened=>$yes,databaseScope=>'owned synthetic /tmp fixture only',ownedSyntheticTmpNamespaceAbsentAfterTest=>$yes,oldSuiteExecuted=>$no,historicalRpcTestVariantExecuted=>$no,httpTransportQualified=>$no,estimateGasEip1898IsNanoExtension=>$yes,dockerImageBuilt=>$no,nodeRuntimeStarted=>$no,networkStarted=>$no,dutiesResumed=>$no,commonFinalityQualified=>$no,campaignAdmission=>$no,oldAuthoritiesRefreshed=>$no,retainedHostNineRecovered=>$no,steps=>\@steps,productionQualified=>$no};
 save('result-public.json',$result);print $J->encode($result)."\n" or die "Selected helpers worker refused\n";
}

sub main {
 need(@ARGV==0);
 # No inherited fixture, key, loader, compiler-control, or runtime variables.
 %ENV=(PATH=>'/usr/bin:/bin',HOME=>'/outputs',TMPDIR=>'/tmp',LANG=>'C',LC_ALL=>'C');
 my $recipe=$J->decode(read_bytes('/controls/recipes.json'));$expected=$J->decode(read_bytes('/controls/source-expected.private.json'));my $core_pin=$J->decode(read_bytes('/controls/core-pin.json'));
 need(keys_are($recipe,qw(format owner uid gid imageId elfMachine programs))&&$recipe->{format} eq 'nano-current-linux-selected-helpers-recipes-v1'&&ref($expected) eq 'HASH');pin_shape($core_pin,0);
 my ($uid,$gid)=@{$recipe}{qw(uid gid)};
 my $arm=$recipe->{owner} eq 'mac-docker';my $amd=$recipe->{owner} eq 'vps-docker';need($arm||$amd);
 need($arm?($uid==501&&$gid==20&&$recipe->{elfMachine}==183&&$recipe->{imageId} eq 'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'):($uid==1000&&$gid==1000&&$recipe->{elfMachine}==62&&$recipe->{imageId} eq 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'));
 need($>==$uid&&$<==$uid&&abs_path($out) eq $out);my @s=lstat($out);need(@s&&S_ISDIR($s[2])&&($s[2]&07777)==0700&&$s[4]==$uid&&$s[5]==$gid);
 opendir(my $dir,$out) or die "Selected helpers worker refused\n";my @existing=grep {$_ ne '.'&&$_ ne '..'} readdir($dir);closedir($dir);need(!@existing);$output_admitted=1;
 need(ref($recipe->{programs}) eq 'ARRAY'&&@{$recipe->{programs}}>=1&&@{$recipe->{programs}}<=($arm?5:4));
 my %sources=(bootstrapImport=>'/work/tests/capella/isolated_bootstrap_import.cpp',bootstrapAudit=>'/work/tests/capella/isolated_bootstrap_audit.cpp',stoppedCapture=>'/work/tests/capella/isolated_stopped_capture.cpp',lifecycleTool=>'/work/tests/capella/deposit_audit.cpp',lifecycleSubmit=>'/work/tests/multihost/lifecycle_submit.cpp');
 my %targets=(bootstrapImport=>'nano-capella-isolated-bootstrap-import',bootstrapAudit=>'nano-capella-isolated-bootstrap-audit',stoppedCapture=>'nano-capella-isolated-stopped-capture',lifecycleTool=>'nano-capella-deposit-audit',lifecycleSubmit=>'nano-capella-lifecycle-submit');
 my (%seen,@initial);need(keys(%$expected)>0&&keys(%$expected)<=20000);
 for my $p(sort keys %$expected){need($p=~m{\A/(?:work|old)/}&&normal_path($p) eq $p&&abs_path($p) eq $p);pin_shape($expected->{$p},0);}
 for my $program(@{$recipe->{programs}}){need(keys_are($program,qw(role source compileFlags linkPrefix linkTail reuse)));my $role=$program->{role};need(defined($role)&&!ref($role)&&exists($sources{$role})&&!$seen{$role}++&&($arm||$role ne 'bootstrapImport')&&$program->{source} eq $sources{$role});
  flags_ok($program->{compileFlags});flags_ok($program->{linkPrefix});my $tail=$program->{linkTail};need(ref($tail) eq 'ARRAY'&&@$tail&&@$tail<=256&&grep($_ eq '/baseline/libnano_core.a',@$tail)==1);
  for my $word(@$tail){need(defined($word)&&!ref($word)&&$word!~/[\0\r\n]/&&$word!~/(?:^|,)@/&&$word=~m{\A(?:/baseline/libnano_core[.]a\z|/(?:old|usr|lib|lib64)/|-)});need($word!~/\A(?:-o|-c|-M|-MM|-MD|-MMD|-MF|-MT|-MQ|-MJ)\z/);push @initial,$word if $word=~m{\A/};}
  need(exists($expected->{$program->{source}}));push @initial,$program->{source};
  my @macro=grep {index($_,'-DNANO_TEST_SOURCE=')==0} @{$program->{compileFlags}};
  if($role eq 'lifecycleTool'){need(@macro==1&&$macro[0] eq '-DNANO_TEST_SOURCE="/work"');}else{need(!@macro);}
  need(!grep {index($_,'NANO_SYNC_FAULT')>=0} @{$program->{compileFlags}});
  if(defined($program->{reuse})){my $r=$program->{reuse};need(keys_are($r,qw(object md dependencyPrefixMap)));pin_shape($r->{object},1);pin_shape($r->{md},1);dependency_map($r->{dependencyPrefixMap});need($r->{object}{path} ne $r->{md}{path});
   for my $pin($r->{object},$r->{md}){need(matches_pin(material($pin->{path}),$pin));push @initial,$pin->{path};}
  }
 }
 push @initial,'/controls/recipes.json','/controls/source-expected.private.json','/controls/core-pin.json','/controls/worker.pl','/usr/bin/clang++','/usr/bin/stat','/usr/bin/ldd','/baseline/libnano_core.a';
 my %once;@initial=grep {!$once{$_}++} @initial;$stage='initial-input-admission';stable_paths('initial-inputs',\@initial);fence();
 my $core=$held{'/baseline/libnano_core.a'};need(defined($core)&&matches_pin($core,$core_pin));
 my @results;
 for my $program(@{$recipe->{programs}}){my $role=$program->{role};my $source=$program->{source};my $flags=$program->{compileFlags};my $reuse=$program->{reuse};my $object=defined($reuse)?$reuse->{object}{path}:$out.'/'.$role.'.cpp.o';my $md=defined($reuse)?$reuse->{md}{path}:$object.'.d';my $target=$out.'/'.$role.'.cpp.o';
  $stage=$role.'-independent-m';++$counts{independentMCommands};my $m=checked_call($role.'-independent-m',[@$flags,'-M','-MT',$target,'-o','/dev/stdout',$source]);my $literal=deps($m);my $normalized=normalized_deps($literal,[],1);need(grep($_ eq $source,@$normalized)==1);
  stable_paths($role.'-compile-dependencies',$normalized);fence();my ($literal_equal,$normalized_equal,$mapped_equal,$previous_literal,$previous_normalized,$previous_mapped)=(undef,undef,undef,undef,undef,undef);
  if(defined($reuse)){
   need(matches_pin(material($object),$reuse->{object})&&matches_pin(material($md),$reuse->{md}));$previous_literal=deps(read_bytes($md));$previous_normalized=normalized_deps($previous_literal,[],0);$previous_mapped=normalized_deps($previous_literal,$reuse->{dependencyPrefixMap},0);
   $literal_equal=same($literal,$previous_literal)?$yes:$no;$normalized_equal=same($normalized,$previous_normalized)?$yes:$no;$mapped_equal=same($normalized,$previous_mapped)?$yes:$no;need($mapped_equal);
  }else{
   $stage=$role.'-compile';++$counts{compiledTranslationUnits};checked_call($role.'-compile',[@$flags,'-MD','-MF',$md,'-MT',$object,'-o',$object,'-c',$source]);
   stable_new($role.'-object-and-md',[$object,$md],0400,$uid,$gid);my $actual=deps(read_bytes($md));$previous_literal=$actual;$previous_normalized=normalized_deps($actual,[],1);$previous_mapped=$previous_normalized;
   $literal_equal=same($literal,$actual)?$yes:$no;$normalized_equal=same($normalized,$previous_normalized)?$yes:$no;$mapped_equal=$normalized_equal;need($normalized_equal);
  }
  my $object_row=hold($object);my $md_row=hold($md);fence();my $exe=$out.'/'.$targets{$role};$stage=$role.'-link';++$counts{linkCommands};checked_call($role.'-link',[@{$program->{linkPrefix}},$object,'-o',$exe,@{$program->{linkTail}}]);
  my $new=stable_new($role.'-software',[$exe],0500,$uid,$gid);my $exe_row=$new->{$exe};elf_check($exe,$recipe->{elfMachine});fence();
  $stage=$role.'-loader';++$counts{loaderCommands};my $ldd=checked_call($role.'-ldd',['/usr/bin/ldd',$exe]);need(read_bytes($out.'/'.$role.'-ldd-stderr.private.log') eq '');my $runtime=loader_paths($ldd);stable_paths($role.'-runtime-inputs',$runtime);fence();
  $stage=$role.'-arguments';++$counts{argumentRefusals};my $stdout=checked_call($role.'-arguments',[$exe],1);my $stderr=read_bytes($out.'/'.$role.'-arguments-stderr.private.log');
  if($role eq 'lifecycleTool'){need($stdout eq ''&&$stderr eq "usage: --audit synthetic-genesis node-config stopped-db external-audit-request deposit-metadata\n");}
  elsif($role eq 'lifecycleSubmit'){need($stdout eq ''&&$stderr eq "{\"event\":\"fresh-lifecycle-child-submission-refused\",\"accepted\":false,\"detailsExported\":false,\"productionQualified\":false}\n");}
  else{json_refusal($role,$stdout,$stderr);}fence();need(same($exe_row,material($exe))&&same($object_row,material($object))&&same($md_row,material($md)));
  push @results,{role=>$role,target=>$targets{$role},source=>$source,objectReused=>defined($reuse)?$yes:$no,dependencyMode=>'MD',currentMLiteralDependencies=>scalar(@$literal),currentMNormalizedDependencies=>scalar(@$normalized),comparisonMdLiteralDependencies=>scalar(@$previous_literal),comparisonMdNormalizedDependencies=>scalar(@$previous_normalized),comparisonMdRemappedDependencies=>scalar(@$previous_mapped),literalMEqualsMd=>$literal_equal,normalizedMEqualsMd=>$normalized_equal,normalizedMEqualsRemappedMd=>$mapped_equal,retainedDependencyPrefixMap=>defined($reuse)?$reuse->{dependencyPrefixMap}:[],object=>$object_row,dependencyFile=>$md_row,software=>$exe_row,softwarePath=>$exe,elfMachine=>0+$recipe->{elfMachine},loaderPassed=>$yes,argumentRefusalPassed=>$yes,independentMCommands=>1,compiledTranslationUnits=>defined($reuse)?0:1,linkCommands=>1,loaderCommands=>1,argumentRefusals=>1};
 }
 $stage='final-fences';fence();save('sdk-and-inputs.private.json',\%held);save('first-dependency-observations.private.json',\@observations);
 save('software-admission.private.json',{format=>'nano-current-linux-selected-helper-software-binding-v1',owner=>$recipe->{owner},imageId=>$recipe->{imageId},coreArchive=>$core,programs=>\@results,guestMaterials=>\%held,logicalResolutionGuards=>\%logical_guards,originalGuestInputsFullNineAndShaUnchanged=>$yes,retainedHostNineRecovered=>$no,oldAuthoritiesRefreshed=>$no});
 my $binding=material('/outputs/software-admission.private.json');my @public=map {my $p=$_;{role=>$p->{role},target=>$p->{target},objectReused=>$p->{objectReused},dependencyMode=>$p->{dependencyMode},currentMLiteralDependencies=>$p->{currentMLiteralDependencies},currentMNormalizedDependencies=>$p->{currentMNormalizedDependencies},comparisonMdLiteralDependencies=>$p->{comparisonMdLiteralDependencies},comparisonMdNormalizedDependencies=>$p->{comparisonMdNormalizedDependencies},comparisonMdRemappedDependencies=>$p->{comparisonMdRemappedDependencies},literalMEqualsMd=>$p->{literalMEqualsMd},normalizedMEqualsMd=>$p->{normalizedMEqualsMd},normalizedMEqualsRemappedMd=>$p->{normalizedMEqualsRemappedMd},retainedDependencyPrefixMap=>$p->{retainedDependencyPrefixMap},objectSha256=>$p->{object}{sha256},objectBytes=>$p->{object}{bytes},softwareSha256=>$p->{software}{sha256},softwareBytes=>$p->{software}{bytes},loaderPassed=>$p->{loaderPassed},argumentRefusalPassed=>$p->{argumentRefusalPassed}}} @results;
 my $result={format=>'nano-current-linux-selected-helpers-qualification-v1',passed=>$yes,owner=>$recipe->{owner},uid=>0+$uid,gid=>0+$gid,imageId=>$recipe->{imageId},imageIdentityVerifiedByWorker=>$no,elfMachine=>0+$recipe->{elfMachine},selectedPrograms=>scalar(@results),%counts,programs=>\@public,coreSha256=>$core->{sha256},coreBytes=>$core->{bytes},softwareAdmissionSha256=>$binding->{sha256},softwareAdmissionBytes=>$binding->{bytes},originalGuestInputsFullNineAndShaUnchanged=>$yes,retainedHostNineRecovered=>$no,steps=>\@steps,coreArchiveMutated=>$no,coreMembersRequalified=>$no,canonicalCmakeQualifiedByWorker=>$no,fullLoaderClosureQualified=>$no,lazySymbolsQualified=>$no,wholeSdkQualified=>$no,newSignaturesCreated=>0,realBootstrapImports=>0,realExports=>0,databaseOpened=>$no,oldSuiteExecuted=>$no,dockerImageBuilt=>$no,nodeRuntimeStarted=>$no,networkStarted=>$no,dutiesResumed=>$no,commonFinalityQualified=>$no,campaignAdmission=>$no,oldAuthoritiesRefreshed=>$no,productionQualified=>$no};
 save('result-public.json',$result);print $J->encode($result)."\n" or die "Selected helpers worker refused\n";
}
my $ok=eval{if(@ARGV==1&&$ARGV[0] eq '--rpc-pruning-current'){rpc_main();}else{main();}1;};
unless($ok){my $stopped=stop_group($active);local $SIG{TERM}='IGNORE';local $SIG{INT}='IGNORE';local $SIG{HUP}='IGNORE';my $post=$stopped?eval{fence();1;}:0;eval{save('worker-refusal.private.json',{passed=>$no,stage=>$stage,originalGuestInputsUnchanged=>$post?$yes:$no,ownProcessGroupCleanupComplete=>$stopped?$yes:$no,completedCommandEvents=>scalar(@steps),%counts});};print STDERR "Selected helpers worker refused\n";exit(1);}
exit(0);
