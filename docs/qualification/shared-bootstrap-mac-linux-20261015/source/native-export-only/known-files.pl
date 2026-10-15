# Source-only ROOT helper for fresh UID501 Linux volume leaves. Never run by author.
use strict;
use warnings;
use Fcntl qw(:DEFAULT :mode);
use Digest::SHA ();
use JSON::PP ();
use Cwd qw(abs_path);
use Time::HiRes qw(time sleep);
use IO::Handle ();

my $json = JSON::PP->new->canonical(1)->ascii(1);
my $max_frame = 134217728;
my $max_file = 67108864;
my $deadline = time() + 120;
my $root_fd;
my $root_path;
my $root_identity;
my $yes = JSON::PP::true;
my $no = JSON::PP::false;
$SIG{ALRM} = sub { die "known-files refused\n"; };
alarm(120);
umask(0077);
binmode(STDIN);
binmode(STDOUT);

sub need { die "known-files refused\n" unless $_[0]; }
sub tick { need(time() < $deadline); }
sub same { return $json->encode($_[0]) eq $json->encode($_[1]); }
sub keys_are { my ($v,@k)=@_; return ref($v) eq 'HASH' && join(',',sort keys %$v) eq join(',',sort @k); }
sub uint { my ($v,$max)=@_; return defined($v) && !ref($v) && "$v" =~ /\A(?:0|[1-9][0-9]*)\z/ && length("$v")<=10 && $v <= $max; }
sub sha { return defined($_[0]) && !ref($_[0]) && $_[0] =~ /\A[0-9a-f]{64}\z/; }
sub leaf { return defined($_[0]) && !ref($_[0]) && $_[0] =~ /\A[A-Za-z0-9_.-]{1,180}\z/ && $_[0] ne '.' && $_[0] ne '..'; }
sub fd_path { return '/proc/'.$$.'/fd/'.fileno($_[0]); }

# Keep these nine values as strings. Numeric checks use a separate Perl stat
# result, never the stored GNU-stat strings (the earlier JSON dualvar pitfall).
sub stat9 {
    my ($p,$follow)=@_; tick();
    open(my $pipe,'-|','stat',$follow?'-Lc':'-c','%d|%i|%f|%h|%u|%g|%s|%y|%z','--',$p) or die "known-files refused\n";
    local $/; my $raw=<$pipe>;
    need(close($pipe) && defined($raw) && length($raw)<1024);
    my $removed=($raw =~ s/\n\z//);
    need($removed==1 && $raw !~ /[\r\n]/);
    my @row=split(/\|/,$raw,-1); need(@row==9);
    return \@row;
}
sub stable_root {
    tick(); my @a=stat($root_fd); my @b=lstat($root_path);
    need(@a && @b && S_ISDIR($a[2]) && S_ISDIR($b[2]));
    need($a[4]==501 && $a[5]==20 && ($a[2]&07777)==0700);
    my @id=@a[0..5]; my @path_id=@b[0..5];
    need(same(\@id,\@path_id) && same(\@id,$root_identity));
    need(defined(abs_path($root_path)) && abs_path($root_path) eq $root_path);
}
sub root_open {
    my ($p)=@_; need(defined($p) && $p =~ m{\A/(?:export|proof|known|config)\z});
    need($>==501 && $<==501);
    need(defined(abs_path($p)) && abs_path($p) eq $p);
    sysopen($root_fd,$p,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_NONBLOCK) or die "known-files refused\n";
    $root_path=$p; my @s=stat($root_fd); need(@s);
    $root_identity=[@s[0..5]]; stable_root();
}
sub anchored { my ($name)=@_; need(leaf($name)); stable_root(); return fd_path($root_fd).'/'.$name; }
sub write_all {
    my ($fh,$body)=@_; my $at=0;
    while($at<length($body)){ tick(); my $n=syswrite($fh,$body,length($body)-$at,$at); need(defined($n)&&$n>0); $at+=$n; }
}
sub read_leaf {
    my ($name,$expected_sha,$expected_bytes,$material,$capture)=@_;
    need(leaf($name) && sha($expected_sha) && uint($expected_bytes,$max_file));
    my $p=anchored($name);
    sysopen(my $f,$p,O_RDONLY|O_NOFOLLOW|O_NONBLOCK) or die "known-files refused\n";
    my @s=stat($f);
    need(@s && S_ISREG($s[2]) && $s[3]==1 && $s[4]==501 && $s[5]==20 && ($s[2]&07777)==0600 && $s[7]==$expected_bytes);
    need(defined(abs_path($p)) && abs_path($p) eq $root_path.'/'.$name);
    my $before=stat9(fd_path($f),1); need(same($before,stat9($p,0)));
    need(!defined($material) || same($before,$material));
    my ($n,$body)=(0,''); my $hash=Digest::SHA->new(256);
    while(1){ tick(); my $got=sysread($f,my $buf,65536); need(defined($got)); last unless $got; $n+=$got; need($n<=$expected_bytes); $hash->add($buf); $body.=$buf if $capture; }
    need($n==$expected_bytes && $hash->hexdigest eq $expected_sha);
    need(same($before,stat9(fd_path($f),1)) && same($before,stat9($p,0)));
    close($f) or die "known-files refused\n"; stable_root();
    return {name=>$name,sha256=>$expected_sha,bytes=>0+$expected_bytes,material=>$before,($capture?(raw=>$body):())};
}
sub write_leaf {
    my ($name,$body,$expected_sha,$expected_bytes)=@_;
    need(leaf($name) && sha($expected_sha) && uint($expected_bytes,$max_file));
    need(length($body)==$expected_bytes && Digest::SHA::sha256_hex($body) eq $expected_sha);
    my $p=anchored($name);
    sysopen(my $f,$p,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW,0600) or die "known-files refused\n";
    write_all($f,$body); $f->sync or die "known-files refused\n";
    my @s=stat($f); need(@s && S_ISREG($s[2]) && $s[3]==1 && $s[4]==501 && $s[5]==20 && ($s[2]&07777)==0600 && $s[7]==$expected_bytes);
    need(same(stat9(fd_path($f),1),stat9($p,0)));
    close($f) or die "known-files refused\n";
    $root_fd->sync or die "known-files refused\n";
    return read_leaf($name,$expected_sha,$expected_bytes,undef,0);
}
sub read_stdin {
    my ($size)=@_; need(uint($size,$max_frame) && $size>0); my $raw='';
    while(length($raw)<$size){ tick(); my $n=sysread(STDIN,my $buf,($size-length($raw)>65536?65536:$size-length($raw))); need(defined($n)&&$n>0); $raw.=$buf; }
    my $n=sysread(STDIN,my $tail,1); need(defined($n)&&$n==0); return $raw;
}
sub canonical_doc {
    my ($raw)=@_; my $v=$json->decode($raw); my $encoded=$json->encode($v);
    need($raw eq $encoded || $raw eq $encoded."\n"); return $v;
}
sub read_verify_control {
    my $raw='';
    while(1){ tick(); my $n=sysread(STDIN,my $buf,1024); need(defined($n)); last unless $n; $raw.=$buf; need(length($raw)<=4096); }
    my $v=canonical_doc($raw);
    need(keys_are($v,qw(format authorityMaterial)) && $v->{format} eq 'nano-linux-known-files-verify-v1' && ref($v->{authorityMaterial}) eq 'ARRAY' && @{$v->{authorityMaterial}}==9);
    for my $x(@{$v->{authorityMaterial}}){need(defined($x)&&!ref($x)&&length($x)>0&&length($x)<256);}
    return $v->{authorityMaterial};
}
sub file_snapshot {
    my ($files,$pins)=@_; my @rows; my %ids;
    for my $i(0..$#$files){
        my $v=$files->[$i]; my $row=read_leaf($v->{name},$v->{sha256},$v->{bytes},defined($pins)?$pins->[$i]->{material}:undef,0);
        my $id=join(':',@{$row->{material}}[0,1]); need(!$ids{$id}++); push @rows,$row;
    }
    return \@rows;
}
sub receipt {
    my ($mode,$count,$bytes,$name,$authority_sha,$authority_bytes,$authority_material,$extra)=@_;
    print $json->encode({format=>'nano-linux-known-files-result-v1',mode=>$mode,passed=>$yes,filesWritten=>$mode eq 'write'?$count:0,filesVerified=>$count,payloadBytes=>$bytes,knownBytesMatched=>$yes,fullNineFieldPinsRetained=>$yes,privateModesAndOwnerVerified=>$yes,rootFdIdentityRetained=>$yes,existingFilesMutated=>$no,namespaceInventoryQualified=>$no,authorityName=>$name,authoritySha256=>$authority_sha,authorityBytes=>$authority_bytes,authorityMaterial=>$authority_material,%$extra,productionQualified=>$no})."\n" or die "known-files refused\n";
}
sub main {
    need(@ARGV>=1); my $mode=shift @ARGV;
    if($mode eq 'write'){
        need(@ARGV==3); my ($root,$frame_sha,$frame_bytes)=@ARGV;
        need(sha($frame_sha)); my $raw=read_stdin($frame_bytes); need(Digest::SHA::sha256_hex($raw) eq $frame_sha);
        my $frame=canonical_doc($raw);
        need(keys_are($frame,qw(format files)) && $frame->{format} eq 'nano-linux-known-files-v1' && ref($frame->{files}) eq 'ARRAY' && @{$frame->{files}}>=1 && @{$frame->{files}}<=64);
        my (%names,$total,@decoded); $total=0;
        for my $v(@{$frame->{files}}){
            need(keys_are($v,qw(name sha256 bytes hex)) && leaf($v->{name}) && $v->{name}!~/\A\.known-files-/ && !$names{$v->{name}}++ && sha($v->{sha256}) && uint($v->{bytes},$max_file) && defined($v->{hex}) && !ref($v->{hex}) && length($v->{hex})==2*$v->{bytes} && $v->{hex}=~/\A[0-9a-f]*\z/);
            $total+=$v->{bytes}; need($total<=$max_file); my $body=pack('H*',$v->{hex}); need(length($body)==$v->{bytes} && Digest::SHA::sha256_hex($body) eq $v->{sha256}); push @decoded,$body;
        }
        root_open($root); my $authority_name='.known-files-'.$frame_sha.'.authority.private.json';
        need(!lstat(anchored($authority_name)));
        for my $v(@{$frame->{files}}){ need(!lstat(anchored($v->{name}))); }
        for my $i(0..$#decoded){ my $v=$frame->{files}[$i]; write_leaf($v->{name},$decoded[$i],$v->{sha256},$v->{bytes}); }
        my $started=time(); my $until=$started+30; my ($previous,$series,$samples,$probes,$rows,$root_before)=(undef,0,0,0,undef,undef);
        while(time()<$until){
            tick(); my $sample=file_snapshot($frame->{files},undef); my $root_sample=stat9(fd_path($root_fd),1); stable_root();
            need(same($root_sample,stat9($root_path,0))); my $joined={files=>$sample,rootMaterial=>$root_sample}; my $now=time(); need($now<$until); ++$probes;
            if(defined($previous)&&same($previous,$joined)){++$samples;}else{$samples=1;$series=$now;}
            $previous=$joined; $rows=$sample; $root_before=$root_sample;
            last if $samples>=3 && $now-$series>=2;
            need($until-time()>1); sleep(1);
        }
        need($samples>=3 && time()-$series>=2 && time()<$until);
        my $authority={format=>'nano-linux-known-files-authority-v1',frameSha256=>$frame_sha,root=>$root,rootIdentity=>$root_identity,files=>$rows,stability=>{samples=>$samples,probes=>$probes,spanMs=>int((time()-$series)*1000)},namespaceInventoryQualified=>$no};
        my $body=$json->encode($authority)."\n"; my $authority_sha=Digest::SHA::sha256_hex($body); my $authority_bytes=length($body);
        need($authority_bytes<=1048576 && same($root_before,stat9(fd_path($root_fd),1)));
        my $authority_row=write_leaf($authority_name,$body,$authority_sha,$authority_bytes);
        need(same($rows,file_snapshot($frame->{files},$rows))); stable_root();
        read_leaf($authority_name,$authority_sha,$authority_bytes,$authority_row->{material},0);
        receipt('write',scalar(@$rows),$total,$authority_name,$authority_sha,$authority_bytes,$authority_row->{material},{stabilitySamples=>$samples,stabilitySpanMs=>$authority->{stability}{spanMs}});
    }elsif($mode eq 'verify'){
        need(@ARGV==4); my ($root,$expected,$bytes,$name)=@ARGV; root_open($root);
        need(sha($expected)&&uint($bytes,1048576)&&$bytes>0&&leaf($name)&&$name=~/\A\.known-files-[0-9a-f]{64}\.authority\.private\.json\z/);
        my $expected_material=read_verify_control();
        my $authority_row=read_leaf($name,$expected,$bytes,$expected_material,1); my $a=canonical_doc($authority_row->{raw});
        need(keys_are($a,qw(format frameSha256 root rootIdentity files stability namespaceInventoryQualified))&&$a->{format} eq 'nano-linux-known-files-authority-v1'&&sha($a->{frameSha256})&&$name eq '.known-files-'.$a->{frameSha256}.'.authority.private.json'&&$a->{root} eq $root&&same($a->{rootIdentity},$root_identity)&&ref($a->{files}) eq 'ARRAY'&&@{$a->{files}}>=1&&@{$a->{files}}<=64&&!$a->{namespaceInventoryQualified});
        need(keys_are($a->{stability},qw(samples probes spanMs))&&uint($a->{stability}{samples},65536)&&$a->{stability}{samples}>=3&&uint($a->{stability}{probes},65536)&&$a->{stability}{probes}>=$a->{stability}{samples}&&uint($a->{stability}{spanMs},30000)&&$a->{stability}{spanMs}>=2000);
        my (%names,$total); $total=0;
        for my $v(@{$a->{files}}){ need(keys_are($v,qw(name sha256 bytes material))&&leaf($v->{name})&&$v->{name}!~/\A\.known-files-/&&!$names{$v->{name}}++&&sha($v->{sha256})&&uint($v->{bytes},$max_file)&&ref($v->{material}) eq 'ARRAY'&&@{$v->{material}}==9); $total+=$v->{bytes}; need($total<=$max_file); }
        need(same($a->{files},file_snapshot($a->{files},$a->{files})));
        read_leaf($name,$expected,$bytes,$authority_row->{material},0); stable_root();
        receipt('verify',scalar(@{$a->{files}}),$total,$name,$expected,0+$bytes,$expected_material,{stabilitySamples=>0+$a->{stability}{samples},stabilitySpanMs=>0+$a->{stability}{spanMs}});
    }elsif($mode eq 'read'){
        need(@ARGV==4); my ($root,$name,$expected,$bytes)=@ARGV; root_open($root);
        my $row=read_leaf($name,$expected,$bytes,undef,1); stable_root(); write_all(\*STDOUT,$row->{raw});
    }else{need(0);}
    stable_root(); close($root_fd) or die "known-files refused\n"; alarm(0);
}
my $ok=eval {main();1;};
unless($ok){ print STDERR "known-files refused\n"; exit(1); }
exit(0);
