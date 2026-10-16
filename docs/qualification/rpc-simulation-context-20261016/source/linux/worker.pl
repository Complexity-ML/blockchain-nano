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
my $deadline=now()+900;
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

sub need {die "Selected helpers worker refused at line ".(caller)[2]."\n" unless $_[0];}
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
 need($bytes==$s[7]&&abs_path($logical) eq $path);my $after=stat_pair($fh,$path);die "transient metadata during first read\n" unless same($before,$after);
 close($fh) or die "Selected helpers worker refused\n";
 my $row={material=>$before,bytes=>0+$bytes,sha256=>$sha->hexdigest};
 return ($path,$row,$body);
}
sub material {my (undef,$r)=read_regular($_[0],0);return $r;}
sub read_bytes {my (undef,undef,$b)=read_regular($_[0],1,$_[1]//$stream_limit);return $b;}
sub hold {
 my ($path)=@_;my ($real,$row)=read_regular($path,0);
 if(exists($held{$real})&&!same($held{$real},$row)){save('material-mismatch.private.json',{path=>$real,before=>$held{$real},after=>$row});die 'selected material changed: '.$real."\n";}need(!exists($logical_guards{$path})||$logical_guards{$path} eq $real);$logical_guards{$path}=$real;$held{$real}=$row;return $row;
}
sub fence {for my $p(sort keys %logical_guards){my $r=abs_path($p);need(defined($r)&&$r eq $logical_guards{$p});}for my $p(sort keys %held){my $current=material($p);if(!same($held{$p},$current)){save('material-mismatch.private.json',{path=>$p,before=>$held{$p},after=>$current});die 'selected material changed: '.$p."\n";}}}
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
 SAMPLE: while(1){
  $table={};%resolutions=();
  for my $p(@$paths){my ($real,$row);my $read_ok=eval{($real,$row)=read_regular($p,0);1;};if(!$read_ok){my $error=$@;my $physical=abs_path($p);if($error eq "transient metadata during first read\n"&&defined($physical)&&!exists($held{$physical})&&now()-$start<30){push @observations,{step=>$name,prospectiveSampleRefused=>$yes,path=>$p};sleep(0.25);next SAMPLE;}die $error;}if(exists($held{$real})&&!same($held{$real},$row)){save('material-mismatch.private.json',{path=>$real,before=>$held{$real},after=>$row});die 'selected material changed: '.$real."\n";}
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
sub archive_members {
 my ($path)=@_;my $raw=read_bytes($path,134217728);need(substr($raw,0,8) eq "!<arch>\n");my ($at,$names)=(8,'');my %members;
 while($at<length($raw)){
  need($at+60<=length($raw));my $h=substr($raw,$at,60);need(substr($h,58,2) eq "`\n");
  my $name=substr($h,0,16);$name=~s/\s+\z//;my $size=substr($h,48,10);$size=~s/\s+//g;need($size=~/\A\d+\z/&&$size<=134217728);$at+=60;need($at+$size<=length($raw));my $body=substr($raw,$at,$size);$at+=$size+($size%2);
  if($name eq '//'){$names=$body;next;}next if $name eq '/'||$name eq '/SYM64/';
  if($name=~m{\A/(\d+)\z}){my $offset=$1;need($offset<length($names));my $end=index($names,"/\n",$offset);need($end>=$offset);$name=substr($names,$offset,$end-$offset);}
  else{$name=~s{/\z}{};}
  need($name=~/\A[A-Za-z0-9_.-]+[.]o\z/&&!exists($members{$name}));
  $members{$name}={sha256=>Digest::SHA::sha256_hex($body),bytes=>length($body)};
 }
 need($at==length($raw)&&keys(%members)==143&&exists($members{'rpc.cpp.o'}));return \%members;
}
my $recipe_reuse={};
sub compile_one {
 my ($label,$source,$flags)=@_;$stage=$label.'-dependencies';
 my $m=call($label.'-m',[@$flags,'-M','-MT','probe',$source]);my $before=normalized_deps(deps($m),[],1);
 for my $p(@$before){my $row=hold($p);if(index($p,'/work/')==0){need(exists($expected->{$p})&&matches_pin($row,$expected->{$p}));}}
 fence();
 if($recipe_reuse->{$label}){my $r=$recipe_reuse->{$label};my $retained=normalized_deps(deps(read_bytes($r->{md})),[],1);need(same($before,$retained));hold($r->{object});save($label.'-object-reuse.json',{passed=>$yes,object=>$r->{object},md=>$r->{md},currentMMatchesRetainedMD=>$yes,newCompilation=>$no});return $r->{object};}
 $stage=$label.'-compile';my $object=$out.'/'.$label.'.o';
 call($label.'-cxx',[@$flags,'-MD','-MF',$out.'/'.$label.'.d','-c',$source,'-o',$object]);
 my $after=normalized_deps(deps(read_bytes($out.'/'.$label.'.d')),[],1);
 need(same($before,$after));fence();stable_paths($label.'-new-object-first-admission',[$object]);return $object;
}
sub link_one {
 my ($label,$object,$core,$recipe)=@_;my @tail=map {$_ eq '/baseline/libnano_core.a'?$core:$_} @{$recipe->{linkTail}};
 $stage=$label.'-link';fence();my $exe=$out.'/'.$label;call($label.'-link',[@{$recipe->{linkPrefix}},$object,'-o',$exe,@tail]);stable_paths($label.'-new-executable-first-admission',[$exe]);elf_check($exe,$recipe->{elfMachine});
 my $loader=call($label.'-loader',['/usr/bin/ldd',$exe]);need($loader!~/not found/);fence();return $exe;
}
sub main_context {
 need(@ARGV==1&&$ARGV[0] eq '--rpc-simulation-context-qualification');
 $output_admitted=1;$deadline=now()+1100;$stream_limit=33554432;
 my $recipe=$J->decode(read_bytes('/controls/recipe.json'));$expected=$J->decode(read_bytes('/controls/expected.json'));
 $recipe_reuse=$recipe->{reuse}//{};my $gid=POSIX::getgid();need($recipe->{format} eq 'nano-rpc-simulation-context-recipe-v1'&&$<==$recipe->{uid}&&$gid==$recipe->{gid});
 %ENV=(PATH=>'/usr/bin:/bin',HOME=>'/tmp',TMPDIR=>'/tmp',LANG=>'C',LC_ALL=>'C');
 # These are fresh read-only bind paths, not prior guest authorities. Their
 # independently retained host nine-field rows remain strict in the controller.
 # VirtioFS may initially expose uid/gid 0 before resolving the actual owner.
 for my $p(keys %{$recipe->{retainedPins}}){$expected->{$p}=$recipe->{retainedPins}{$p};}
 my @binds=sort keys %{$recipe->{retainedPins}};stable_paths('new-read-only-bind-first-admission',\@binds);
 for my $p(@binds){need(matches_pin(hold($p),$recipe->{retainedPins}{$p}));}
 save('new-bind-first-admission.private.json',{observations=>\@observations,priorGuestAuthoritiesRefreshed=>$no,hostAuthoritiesRetainedByController=>$yes});
 for my $p('/controls/recipe.json','/controls/expected.json','/controls/worker.pl','/usr/bin/clang++','/usr/bin/ar','/usr/bin/ldd','/work/tests/fixtures/python-rpc.json'){hold($p);}
 my $core='/baseline/libnano_core.a';my $before=archive_members($core);save('baseline-core-members.json',$before);
 my $newtest=compile_one('context','/work/tests/test_rpc_simulation_context.cpp',$recipe->{compileFlags});
 my @baseline;
 if(!$recipe->{baselineEvidence}){my $baseline=link_one('context-baseline',$newtest,$core,$recipe);
 for my $group(qw(fees balances nonces bounds)){
  $stage='baseline-'.$group;my $raw=call('baseline-'.$group,[$baseline,'--simulation-context-self-test','--group',$group],1);
  my @lines=grep {length($_)} split(/\n/,$raw);need(@lines==1);my $error=$J->decode($lines[0]);
  my $diagnostic=read_bytes($out.'/baseline-'.$group.'-stderr.private.log');
  need(!$error->{passed}&&$error->{stage} eq $group&&ref($error->{caseRecords}) eq 'ARRAY'&&@{$error->{caseRecords}}>0&&length($diagnostic)>0);
  push @baseline,{group=>$group,exitCode=>1,error=>$error,diagnostic=>$diagnostic};
 }
 save('causal-baseline.json',{newDisjointGroups=>\@baseline,expectedCausalFailures=>4,oldSuiteExecuted=>$no});
 }else{my $prior=$J->decode(read_bytes($recipe->{baselineEvidence}));need($prior->{expectedCausalFailures}==4&&@{$prior->{newDisjointGroups}}==4);save('causal-baseline-reused.json',{passed=>$yes,source=>$recipe->{baselineEvidence},pin=>$recipe->{retainedPins}{$recipe->{baselineEvidence}},newBaselineExecutions=>0});} 
 my $next;
 if($recipe->{candidateCore}){$next=$recipe->{candidateCore};need(matches_pin(hold($next),$recipe->{retainedPins}{$next}));}
 else{my $rpc=compile_one('rpc.cpp','/work/src/rpc.cpp',$recipe->{coreFlags});$next=$out.'/libnano_core.a';call('copy-core',['/bin/cp','--',$core,$next]);need(chmod(0600,$next)==1);call('replace-rpc-member',['/usr/bin/ar','rcs',$next,$rpc]);stable_paths('new-core-first-admission',[$next]);}
 my $after=archive_members($next);my $retained=0;
 for my $name(keys %$before){need(exists($after->{$name}));if($name ne 'rpc.cpp.o'){need(same($before->{$name},$after->{$name}));++$retained;}}
 need($retained==142&&!same($before->{'rpc.cpp.o'},$after->{'rpc.cpp.o'}));
 save('core-member-continuity.json',{passed=>$yes,members=>143,unchangedMembers=>$retained,replacedMember=>'rpc.cpp.o',oldRpc=>$before->{'rpc.cpp.o'},newRpc=>$after->{'rpc.cpp.o'},oldCore=>$held{abs_path($core)},newCore=>$held{abs_path($next)}});
 my $candidate=link_one('nano-rpc-simulation-context-tests',$newtest,$next,$recipe);
 $stage='candidate-new-groups';my $raw=call('context-all',[$candidate,'--simulation-context-self-test']);
 my @events=map {$J->decode($_)} grep {length($_)} split(/\n/,$raw);need(@events==5&&$events[-1]{passed}&&$events[-1]{format} eq 'nano-rpc-simulation-context-targeted-v1'&&$events[-1]{selectedGroup} eq 'all'&&$events[-1]{groups}==4&&$events[-1]{checks}==476&&$events[-1]{expectedRefusals}==126);
 my @names=qw(fees balances nonces bounds);my @checks=(104,264,336,476);my @refusals=(0,6,6,126);my @records=(34,66,32,126);
 for my $i(0..3){my $z=$events[$i];need($z->{scenario} eq 'rpc-simulation-context-'.$names[$i]&&$z->{passed}&&$z->{checks}==$checks[$i]&&$z->{expectedRefusals}==$refusals[$i]&&ref($z->{caseRecords}) eq 'ARRAY'&&@{$z->{caseRecords}}==$records[$i]);} save('candidate-events.json',\@events);
 my @regression;
 for my $test(@{$recipe->{regressions}}){
  my $obj=compile_one($test->{role},$test->{source},$recipe->{compileFlags});my $exe=link_one($test->{role},$obj,$next,$recipe);
  # The ordinary RPC fixture reads a public JSON fixture relative to cwd.
  my @cmd=$test->{role} eq 'nano-rpc-tests'?('/bin/sh','-ec','cd /work && exec /outputs/nano-rpc-tests'):($exe,@{$test->{arguments}});
  $stage=$test->{role}.'-regression';call($test->{role}.'-regression',\@cmd);push @regression,$test->{role};
 }
 fence();save('software-admission.private.json',{format=>'nano-rpc-simulation-context-software-v1',oldAuthoritiesRefreshed=>$no,guestMaterials=>\%held,logicalGuards=>\%logical_guards,steps=>\@steps});
 save('result-public.json',{format=>'nano-rpc-simulation-context-qualification-v1',passed=>$yes,architecture=>$recipe->{architecture},image=>$recipe->{image},causalBaselineFailures=>4,newGroups=>4,regressions=>\@regression,coreMembers=>143,unchangedCoreMembers=>142,replacedCoreMembers=>['rpc.cpp.o'],newCore=>$held{abs_path($next)},sourceFinalityAuthenticated=>$no,consensusQualified=>$no,productionQualified=>$no,oldAuthoritiesRefreshed=>$no});
}
my $ok=eval{main_context();1};my $error=$@;
if(!$ok){my $stopped=stop_group($active);$active=undef if $stopped;eval{save('failure-public.json',{passed=>$no,stage=>$stage,reason=>substr($error,0,800),activeChildStopped=>$stopped?$yes:$no,productionQualified=>$no});};print STDERR $error;exit 1;}
exit 0;
