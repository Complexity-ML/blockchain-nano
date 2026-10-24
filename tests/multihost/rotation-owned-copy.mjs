// Stopped synthetic copies only. This module never opens RocksDB.
// The legacy export copies two Core DBs; the owner export copies both full roots.
import {open,mkdir,lstat,readdir,realpath} from 'node:fs/promises';
import {constants} from 'node:fs';
import {join,resolve,dirname} from 'node:path';
import {createHash} from 'node:crypto';
import {setTimeout as delay} from 'node:timers/promises';

const fields=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const material=s=>fields.map(k=>String(s[k]));
const same=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const need=(ok)=>{if(!ok)throw Error('Stopped rotation physical copy refused');};
const identity=row=>row.material[0]+':'+row.material[1];
const digest=bytes=>createHash('sha256').update(bytes).digest('hex');
const format='nano-fresh-rotation-owned-copy-v1';
export const rotationCopyFields=['format','passed','databaseRoles','sourceFiles','sourceDirectories','payloadBytes','sourceNineFieldAndShaUnchanged','sourceNamespaceUnchanged','copiesMatchSourceBytes','independentInodes','privateDirectoryModes','privateFileModes','copiesUnchangedAfterAudit','copyProofSha256','stabilityProbeCount','stabilityElapsedMs','stabilitySpanMs','rawInventoryExported','productionQualified'];

export function validateRotationCopySummary(r,afterAudit){
 need(r&&Object.keys(r).sort().join(',')===[...rotationCopyFields].sort().join(',')&&r.format===format&&r.passed===true&&r.databaseRoles===2&&r.copiesUnchangedAfterAudit===afterAudit&&r.rawInventoryExported===false&&r.productionQualified===false&&/^[0-9a-f]{64}$/.test(r.copyProofSha256));
 for(const k of ['sourceFiles','sourceDirectories','payloadBytes'])need(Number.isSafeInteger(r[k])&&r[k]>=0);
 need(r.sourceFiles<=40000&&r.sourceDirectories<=40000&&r.payloadBytes<=1073741824);
 need(Number.isSafeInteger(r.stabilityProbeCount)&&r.stabilityProbeCount>=3&&Number.isSafeInteger(r.stabilityElapsedMs)&&r.stabilityElapsedMs<=30000&&Number.isSafeInteger(r.stabilitySpanMs)&&r.stabilitySpanMs>=2000&&r.stabilitySpanMs<=r.stabilityElapsedMs);
 for(const k of ['sourceNineFieldAndShaUnchanged','sourceNamespaceUnchanged','copiesMatchSourceBytes','independentInodes','privateDirectoryModes','privateFileModes'])need(r[k]===true);
 return r;
}

export async function createNativeRotationCopies(options){return createNativeStoppedCopies(options);}
export async function createNativeOwnerCopies({dataRoot,witnessRoot,destination,namespace,owner,resources,checkpoint=()=>{}}){
 need(/^isolated-capella-[0-9a-f]{64}$/.test(namespace)&&owner==='mac-native');
 for(const p of [dataRoot,witnessRoot])need(p.endsWith('/'+namespace+'/'+owner));
 const files=BigInt(resources.maximumInventoryFiles),bytes=BigInt(resources.maximumInventoryBytes);
 need(files>0n&&files<=BigInt(Number.MAX_SAFE_INTEGER)&&bytes>0n&&bytes<=BigInt(Number.MAX_SAFE_INTEGER));
 return createNativeStoppedCopies({dataRoot,witnessRoot,destination,checkpoint,wholeOwner:{namespace,owner,files:Number(files),bytes:Number(bytes)}});
}
async function createNativeStoppedCopies({dataRoot,witnessRoot,destination,checkpoint=()=>{},wholeOwner=null}){
 let until=performance.now()+180000;const check=()=>{checkpoint();need(performance.now()<until);};
 const roots=wholeOwner?[dataRoot,witnessRoot]:[join(dataRoot,'chain'),join(witnessRoot,'chain-witness')];
 const maximumFiles=wholeOwner?wholeOwner.files:20000,maximumBytes=wholeOwner?wholeOwner.bytes:536870912;
 const within=(a,b)=>a===b||b.startsWith(a+'/');
 need([dataRoot,witnessRoot,destination].every(p=>resolve(p)===p));
 need(!within(dataRoot,witnessRoot)&&!within(witnessRoot,dataRoot));
 for(const p of [dataRoot,witnessRoot])need(!within(p,destination)&&!within(destination,p));
 const canonicalDirectory=async p=>{const s=await lstat(p,{bigint:true});need(await realpath(p)===p&&s.isDirectory()&&s.uid===BigInt(process.getuid()));return s;};
 const rows=new Map(),sourceIds=new Set(),creatorFiles=new Map(),creatorDirectories=new Map(),newAnchors=new Map();
 const directoryIdentity=s=>[s.dev,s.ino,s.mode,s.uid,s.gid].map(String);
 async function newDirectory(p){
  await mkdir(p,{mode:0o700});if(!wholeOwner)return;
  const f=await open(p,constants.O_RDONLY|constants.O_DIRECTORY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
  try{const z=await f.stat({bigint:true});need(z.isDirectory()&&z.uid===BigInt(process.getuid())&&(Number(z.mode)&0o7777)===0o700&&same(material(z),material(await lstat(p,{bigint:true}))));creatorDirectories.set(p,directoryIdentity(z));}finally{await f.close();}
 }
 function checkNewAnchors(root,copy){
  if(!wholeOwner)return;need(copy.entries&&Object.keys(copy.entries).length>0);
  for(const [rel,z] of Object.entries(copy.entries)){const p=rel?join(root,rel):root,a=newAnchors.get(p);need(a&&a.kind===z.kind&&same(a.material,z.material.slice(0,8))&&(z.kind==='directory'||a.sha256===z.sha256&&a.bytes===z.bytes));}
 }

 async function scan(root,{privateModes=false,forbiddenIds=new Set()}={}){
  const entries=Object.create(null),seen=new Set();let files=0,dirs=0,bytes=0;
  async function visit(p,rel){
   check();const before=await lstat(p,{bigint:true});need(await realpath(p)===p&&before.uid===BigInt(process.getuid())&&(before.isDirectory()||before.isFile()));
   const id=before.dev+':'+before.ino;need(!seen.has(id)&&!forbiddenIds.has(id));seen.add(id);
   if(before.isDirectory()){
    need(++dirs<=maximumFiles&&(!privateModes||(Number(before.mode)&0o7777)===0o700));const names=(await readdir(p)).sort();
    for(const name of names){need(name!=='.'&&name!=='..'&&!/[\0\r\n]/.test(name));await visit(join(p,name),rel?rel+'/'+name:name);}
    need(same(material(before),material(await lstat(p,{bigint:true}))));entries[rel]={kind:'directory',material:material(before)};
   }else{
    need(before.nlink===1n&&before.size>=0n&&++files<=maximumFiles&&(!privateModes||(Number(before.mode)&0o7777)===0o600));bytes+=Number(before.size);need(bytes<=maximumBytes);
    const f=await open(p,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);const hash=createHash('sha256');let total=0;
    try{need(same(material(before),material(await f.stat({bigint:true}))));const chunk=Buffer.alloc(65536);for(;;){check();const {bytesRead:n}=await f.read(chunk,0,chunk.length,null);if(!n)break;total+=n;need(BigInt(total)<=before.size);hash.update(chunk.subarray(0,n));}need(BigInt(total)===before.size&&same(material(before),material(await f.stat({bigint:true})))&&same(material(before),material(await lstat(p,{bigint:true}))));}finally{await f.close();}
    entries[rel]={kind:'file',material:material(before),sha256:hash.digest('hex'),bytes:total};
   }
  }
  await visit(root,'');return {entries,files,dirs,bytes};
 }
 for(const root of roots){await canonicalDirectory(root);const row=await scan(root);rows.set(root,row);for(const z of Object.values(row.entries)){const id=identity(z);need(!sourceIds.has(id));sourceIds.add(id);}}
 await canonicalDirectory(dirname(destination));await newDirectory(destination);need((Number((await lstat(destination,{bigint:true})).mode)&0o7777)===0o700);
 const destinations=wholeOwner?[join(destination,'data',wholeOwner.namespace,wholeOwner.owner),join(destination,'witness',wholeOwner.namespace,wholeOwner.owner)]:[join(destination,'data','chain'),join(destination,'witness','chain-witness')];
 if(wholeOwner)for(const role of ['data','witness']){await newDirectory(join(destination,role));await newDirectory(join(destination,role,wholeOwner.namespace));}
 for(let i=0;i<2;++i){
  const source=roots[i],target=destinations[i],snapshot=rows.get(source);if(!wholeOwner)await mkdir(dirname(target),{mode:0o700});need((Number((await canonicalDirectory(dirname(target))).mode)&0o7777)===0o700);
  for(const [rel,z] of Object.entries(snapshot.entries).filter(([,z])=>z.kind==='directory').sort((a,b)=>a[0].split('/').length-b[0].split('/').length||a[0].localeCompare(b[0]))){check();await newDirectory(rel?join(target,rel):target);}
  for(const [rel,z] of Object.entries(snapshot.entries).filter(([,z])=>z.kind==='file')){
   check();const src=join(source,rel),dst=join(target,rel),input=await open(src,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);let output;
   try{
    need(same(z.material,material(await input.stat({bigint:true})))&&same(z.material,material(await lstat(src,{bigint:true}))));output=await open(dst,constants.O_RDWR|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);const hash=createHash('sha256'),chunk=Buffer.alloc(65536);let total=0;
    for(;;){check();const {bytesRead:n}=await input.read(chunk,0,chunk.length,null);if(!n)break;total+=n;need(total<=z.bytes);hash.update(chunk.subarray(0,n));for(let at=0;at<n;){check();const {bytesWritten:w}=await output.write(chunk,at,n-at,null);need(w>0);at+=w;}}
    need(total===z.bytes&&hash.digest('hex')===z.sha256&&same(z.material,material(await input.stat({bigint:true})))&&same(z.material,material(await lstat(src,{bigint:true}))));await output.sync();const d=await output.stat({bigint:true});need(d.isFile()&&d.nlink===1n&&d.uid===BigInt(process.getuid())&&(Number(d.mode)&0o7777)===0o600&&!sourceIds.has(d.dev+':'+d.ino)&&same(material(d),material(await lstat(dst,{bigint:true}))));if(wholeOwner)creatorFiles.set(dst,{kind:'file',material:material(d).slice(0,8),sha256:z.sha256,bytes:z.bytes});
   }finally{if(output)await output.close();await input.close();}
  }
  for(const [rel,z] of Object.entries(snapshot.entries).filter(([,z])=>z.kind==='directory')){const f=await open(rel?join(target,rel):target,'r');try{await f.sync();}finally{await f.close();}}const parent=await open(dirname(target),'r');try{await parent.sync();}finally{await parent.close();}
 }
 if(wholeOwner){
  // New directory link counts/size/mtime settle only while constructing children.
  // Creation identity is immutable; after construction all eight fields are fixed.
  for(const p of [...creatorDirectories.keys()].sort((a,b)=>b.length-a.length)){
   const f=await open(p,constants.O_RDONLY|constants.O_DIRECTORY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
   try{await f.sync();const z=await f.stat({bigint:true});need(same(creatorDirectories.get(p),directoryIdentity(z))&&same(material(z),material(await lstat(p,{bigint:true}))));newAnchors.set(p,{kind:'directory',material:material(z).slice(0,8)});}finally{await f.close();}
  }
  for(const [p,z] of creatorFiles)newAnchors.set(p,z);
 }
 let copies=[];const copyIds=new Set(sourceIds);
 for(let i=0;i<2;++i){
  const source=rows.get(roots[i]),copy=await scan(destinations[i],{privateModes:true,forbiddenIds:copyIds});checkNewAnchors(destinations[i],copy);need(same(Object.keys(source.entries).sort(),Object.keys(copy.entries).sort()));
  for(const [rel,z] of Object.entries(copy.entries)){copyIds.add(identity(z));const old=source.entries[rel];need(z.kind===old.kind&&(z.kind==='directory'||z.sha256===old.sha256&&z.bytes===old.bytes));}copies.push(copy);
  need(same(source,await scan(roots[i])));
 }
 // These NEW copies have no admitted material baseline until the final stable
 // sample. Source originals remain pinned throughout; never refresh their rows.
 const stableBegan=performance.now(),stableUntil=stableBegan+30000;let previous=null,consecutive=0,seriesAt=0,probeCount=0,stableSpan=0;
 while(performance.now()<stableUntil){
  check();const sample=[];
  for(let i=0,ids=new Set(sourceIds);i<2;++i){need(same(rows.get(roots[i]),await scan(roots[i])));const copy=await scan(destinations[i],{privateModes:true,forbiddenIds:ids}),source=rows.get(roots[i]);checkNewAnchors(destinations[i],copy);need(same(Object.keys(source.entries).sort(),Object.keys(copy.entries).sort()));for(const [rel,z] of Object.entries(copy.entries)){ids.add(identity(z));const old=source.entries[rel];need(z.kind===old.kind&&(z.kind==='directory'||z.sha256===old.sha256&&z.bytes===old.bytes));}sample.push(copy);}
  if(wholeOwner)for(const root of roots)need(same(rows.get(root),await scan(root)));const now=performance.now();need(now<stableUntil);++probeCount;
  if(previous&&same(previous,sample)){++consecutive;}else{consecutive=1;seriesAt=now;}
  previous=sample;stableSpan=now-seriesAt;
  if(consecutive>=3&&stableSpan>=2000){copies=sample;break;}
  need(stableUntil-performance.now()>1000);await delay(1000);
 }
 need(consecutive>=3&&stableSpan>=2000&&performance.now()<stableUntil);
 const stability={stabilityProbeCount:probeCount,stabilityElapsedMs:Math.ceil(performance.now()-stableBegan),stabilitySpanMs:Math.floor(stableSpan)};
 for(let i=0;i<2;++i){need(same(rows.get(roots[i]),await scan(roots[i])));need(same(copies[i],await scan(destinations[i],{privateModes:true,forbiddenIds:sourceIds})));}
 const privateProof={format:wholeOwner?'nano-fresh-native-owner-copy-private-proof-v1':'nano-fresh-rotation-owned-copy-private-proof-v1',sources:roots.map(root=>({root,...rows.get(root)})),copies:destinations.map((root,i)=>({root,...copies[i]}))};
 const raw=Buffer.from(JSON.stringify(privateProof)+'\n');need(raw.length<=67108864);const proofPath=join(destination,'physical-copy.private.json'),f=await open(proofPath,'wx',0o600);try{await f.writeFile(raw);await f.sync();}finally{await f.close();}const d=await open(destination,'r');try{await d.sync();}finally{await d.close();}
 async function proofFence(){const f=await open(proofPath,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);try{const before=await f.stat({bigint:true});need(before.isFile()&&before.nlink===1n&&before.uid===BigInt(process.getuid())&&(Number(before.mode)&0o7777)===0o600&&before.size===BigInt(raw.length));const h=createHash('sha256'),chunk=Buffer.alloc(65536);let n=0;for(;;){check();const {bytesRead:z}=await f.read(chunk,0,chunk.length,null);if(!z)break;n+=z;need(n<=raw.length);h.update(chunk.subarray(0,z));}need(n===raw.length&&h.digest('hex')===digest(raw)&&same(material(before),material(await f.stat({bigint:true})))&&same(material(before),material(await lstat(proofPath,{bigint:true}))));}finally{await f.close();}}await proofFence();
 const summary={format,passed:true,databaseRoles:2,sourceFiles:[...rows.values()].reduce((n,z)=>n+z.files,0),sourceDirectories:[...rows.values()].reduce((n,z)=>n+z.dirs,0),payloadBytes:[...rows.values()].reduce((n,z)=>n+z.bytes,0),sourceNineFieldAndShaUnchanged:true,sourceNamespaceUnchanged:true,copiesMatchSourceBytes:true,independentInodes:true,privateDirectoryModes:true,privateFileModes:true,copiesUnchangedAfterAudit:false,copyProofSha256:digest(raw),...stability,rawInventoryExported:false,productionQualified:false};
 const checked=after=>{if(!wholeOwner)return validateRotationCopySummary({...summary,copiesUnchangedAfterAudit:after},after);const {databaseRoles,...rest}=summary;need(summary.sourceFiles<=maximumFiles&&summary.payloadBytes<=maximumBytes);return {...rest,format:'nano-fresh-native-owner-copy-v1',roleRoots:2,copiesUnchangedAfterAudit:after};};
 return {dataRoot:wholeOwner?destinations[0]:dirname(destinations[0]),witnessRoot:wholeOwner?destinations[1]:dirname(destinations[1]),summary:checked(false),async verifySource(){until=performance.now()+180000;for(let i=0;i<2;++i)need(same(rows.get(roots[i]),await scan(roots[i])));await proofFence();return {sourceNineFieldAndShaUnchanged:true,sourceNamespaceUnchanged:true,copyProofSha256:digest(raw)};},async verify(){
  until=performance.now()+180000;
  for(let i=0;i<2;++i){need(same(rows.get(roots[i]),await scan(roots[i])));need(same(copies[i],await scan(destinations[i],{privateModes:true,forbiddenIds:sourceIds})));}
  await proofFence();return checked(true);
 }};
}

// Linux image already provides Perl and GNU stat. No Python/image installation.
// Full inventories and all database bytes stay in the owner-local audit volume.
export const linuxRotationCopy=String.raw`use strict;use warnings;use Fcntl qw(:DEFAULT :mode);use Digest::SHA ();use JSON::PP ();use Cwd qw(abs_path);use Time::HiRes qw(time);use IO::Handle ();
my($srcp,$srcw,$out,$mode,$expected)=@ARGV;my $until=time()+175;my $uid=$>;my $json=JSON::PP->new->canonical(1);my $format='nano-fresh-rotation-owned-copy-v1';
sub need{die "stopped rotation physical copy refused\n" unless $_[0]}sub tick{need(time()<$until)}
need(@ARGV==5&&($mode eq 'create'||$mode eq 'verify')&&$srcp=~m{^/}&&$srcw=~m{^/}&&$out eq '/rotation-audit');
sub stat9{my($p,$follow)=@_;tick();open(my $pipe,'-|','stat',$follow?'-Lc':'-c','%d|%i|%f|%h|%u|%g|%s|%y|%z','--',$p)or die "stat refused\n";local $/;my $s=<$pipe>;need(close($pipe)&&defined($s)&&length($s)<1024);chomp $s;my @a=split(/\|/,$s,-1);need(@a==9);return \@a;}
sub same{return $json->encode($_[0]) eq $json->encode($_[1]);}sub identity{return $_[0]->{material}[0].':'.$_[0]->{material}[1];}
sub canonical{my($p)=@_;need(defined(abs_path($p))&&abs_path($p) eq $p);}sub fdpath{return '/proc/'.$$.'/fd/'.fileno($_[0]);}
sub readfile{my($p,$pin,$capture)=@_;sysopen(my $f,$p,O_RDONLY|O_NOFOLLOW|O_NONBLOCK)or die "read refused\n";my @kind=stat($f);need(@kind&&S_ISREG($kind[2]));my $b=stat9(fdpath($f),1);my($links,$owner,$size)=@{$b}[3,4,6];need(same($b,stat9($p,0))&&$links==1&&$owner==$uid&&$size>=0&&$size<=($capture?67108864:536870912));need(!$capture||($kind[2]&07777)==0600);need(!$pin||same($b,$pin->{material}));my $hash=Digest::SHA->new(256);my $n=0;my $body='';while(1){tick();my $z=sysread($f,my $buf,65536);need(defined($z));last unless $z;$n+=$z;need($n<=$size);$hash->add($buf);$body.=$buf if $capture;}need($n==$size&&same($b,stat9(fdpath($f),1))&&same($b,stat9($p,0)));close($f)or die "close refused\n";return {material=>$b,bytes=>$n,sha256=>$hash->hexdigest,kind=>'file',($capture?(raw=>$body):())};}
sub scan{my($root,$private,$forbidden)=@_;my(%entries,%seen);my($files,$dirs,$bytes)=(0,0,0);my $walk;$walk=sub{my($p,$rel)=@_;tick();canonical($p);my @s=lstat($p);need(@s&&$s[4]==$uid&&(S_ISDIR($s[2])||S_ISREG($s[2])));my $a=stat9($p,0);my $id=$a->[0].':'.$a->[1];need(!$seen{$id}++&&!$forbidden->{$id});if(S_ISDIR($s[2])){need(++$dirs<=20000&&(!$private||($s[2]&07777)==0700));opendir(my $dh,$p)or die "directory refused\n";my @names=sort grep{$_ ne '.'&&$_ ne '..'}readdir($dh);closedir($dh);for my $name(@names){need($name!~/[\x00\r\n]/);$walk->($p.'/'.$name,length($rel)?$rel.'/'.$name:$name);}need(same($a,stat9($p,0)));$entries{$rel}={kind=>'directory',material=>$a};}else{need(++$files<=20000&&$s[3]==1&&(!$private||($s[2]&07777)==0600));my $z=readfile($p,undef);$bytes+=$z->{bytes};need($bytes<=536870912);$entries{$rel}=$z;}};$walk->($root,'');return {entries=>\%entries,files=>$files,dirs=>$dirs,bytes=>$bytes};}
sub mkdirprivate{my($p)=@_;mkdir($p,0700)or die "new directory refused\n";my @s=lstat($p);need($s[4]==$uid&&S_ISDIR($s[2])&&($s[2]&07777)==0700);}
my @sources=($srcp,$srcw);my @targets=($out.'/data/chain',$out.'/witness/chain-witness');canonical($out);my @o=lstat($out);need(S_ISDIR($o[2])&&$o[4]==$uid&&($o[2]&07777)==0700);for my $s(@sources){canonical($s);need($s ne $out&&index($s,$out.'/')!=0&&index($out,$s.'/')!=0);}need($srcp ne $srcw&&index($srcp,$srcw.'/')!=0&&index($srcw,$srcp.'/')!=0);
my $proofpath=$out.'/physical-copy.private.json';my($proof,$proofsha);my %sourceids;my $stability;
if($mode eq 'create'){
 need($expected eq '-');my @old=map{scan($_,0,{})}@sources;for my $s(@old){for my $z(values %{$s->{entries}}){my $id=identity($z);need(!$sourceids{$id}++);}}
 for my $i(0,1){my $src=$sources[$i];my $dst=$targets[$i];mkdirprivate($out.($i?'/witness':'/data'));my $rows=$old[$i]->{entries};for my $r(sort{scalar(split('/',$a))<=>scalar(split('/',$b))||$a cmp $b}grep{$rows->{$_}{kind} eq 'directory'}keys %$rows){mkdirprivate(length($r)?$dst.'/'.$r:$dst);}
  for my $r(sort grep{$rows->{$_}{kind} eq 'file'}keys %$rows){tick();my $z=$rows->{$r};my $sp=$src.'/'.$r;my $dp=$dst.'/'.$r;sysopen(my $in,$sp,O_RDONLY|O_NOFOLLOW|O_NONBLOCK)or die "source open refused\n";my $before=stat9(fdpath($in),1);need(same($before,$z->{material})&&same($before,stat9($sp,0)));sysopen(my $to,$dp,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW,0600)or die "destination refused\n";my($n,$h)=(0,Digest::SHA->new(256));while(1){tick();my $got=sysread($in,my $buf,65536);need(defined($got));last unless $got;$n+=$got;need($n<=$z->{bytes});$h->add($buf);my $at=0;while($at<$got){tick();my $w=syswrite($to,$buf,$got-$at,$at);need(defined($w)&&$w>0);$at+=$w;}}need($n==$z->{bytes}&&$h->hexdigest eq $z->{sha256}&&same($before,stat9(fdpath($in),1))&&same($before,stat9($sp,0)));require IO::Handle;$to->sync or die "sync refused\n";need(same(stat9(fdpath($to),1),stat9($dp,0)));close($to)or die "close refused\n";close($in)or die "close refused\n";
  }
 }
 my @copies;my %allids=%sourceids;for my $i(0,1){my $c=scan($targets[$i],1,\%allids);need(same([sort keys %{$c->{entries}}],[sort keys %{$old[$i]->{entries}}]));for my $r(keys %{$c->{entries}}){my $z=$c->{entries}{$r};$allids{identity($z)}=1;my $s=$old[$i]->{entries}{$r};need($z->{kind} eq $s->{kind}&&($z->{kind} eq 'directory'||$z->{bytes}==$s->{bytes}&&$z->{sha256} eq $s->{sha256}));}need(same($old[$i],scan($sources[$i],0,{})));push @copies,$c;}
 # Flush newly created directory entries; source paths receive no setters.
 for my $i(0,1){for my $r(grep{$copies[$i]->{entries}{$_}{kind} eq 'directory'}keys %{$copies[$i]->{entries}}){my $p=length($r)?$targets[$i].'/'.$r:$targets[$i];sysopen(my $d,$p,O_RDONLY|O_DIRECTORY|O_NOFOLLOW)or die "directory sync refused\n";$d->sync or die "directory sync refused\n";close($d);}}
 my($began,$previous,$series,$span,$count,$probes)=(time(),undef,0,0,0,0);my $deadline=$began+30;
 while(time()<$deadline){tick();my @sample;my %ids=%sourceids;for my $i(0,1){need(same($old[$i],scan($sources[$i],0,{})));my $c=scan($targets[$i],1,\%ids);need(same([sort keys %{$c->{entries}}],[sort keys %{$old[$i]->{entries}}]));for my $r(keys %{$c->{entries}}){my $z=$c->{entries}{$r};$ids{identity($z)}=1;my $s=$old[$i]->{entries}{$r};need($z->{kind} eq $s->{kind}&&($z->{kind} eq 'directory'||$z->{bytes}==$s->{bytes}&&$z->{sha256} eq $s->{sha256}));}push @sample,$c;}my $now=time();need($now<$deadline);++$probes;if(defined($previous)&&same($previous,\@sample)){++$count;}else{$count=1;$series=$now;}$previous=\@sample;$span=$now-$series;if($count>=3&&$span>=2){@copies=@sample;last;}need($deadline-time()>1);Time::HiRes::sleep(1);}
 need($count>=3&&$span>=2&&time()<$deadline);$stability={stabilityProbeCount=>$probes,stabilityElapsedMs=>int((time()-$began)*1000+0.999),stabilitySpanMs=>int($span*1000)};
 for my $i(0,1){need(same($old[$i],scan($sources[$i],0,{}))&&same($copies[$i],scan($targets[$i],1,\%sourceids)));}
 $proof={format=>'nano-fresh-rotation-owned-copy-private-proof-v1',stability=>$stability,sources=>[map{{root=>$sources[$_],%{$old[$_]}}}0,1],copies=>[map{{root=>$targets[$_],%{$copies[$_]}}}0,1]};my $raw=$json->encode($proof)."\n";need(length($raw)<=67108864);sysopen(my $f,$proofpath,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600)or die "proof refused\n";my $at=0;while($at<length($raw)){tick();my $n=syswrite($f,$raw,length($raw)-$at,$at);need(defined($n)&&$n>0);$at+=$n;}$f->sync or die "sync refused\n";close($f)or die "close refused\n";for my $p($out.'/data',$out.'/witness',$out){sysopen(my $d,$p,O_RDONLY|O_DIRECTORY|O_NOFOLLOW)or die "parent sync refused\n";$d->sync or die "parent sync refused\n";close($d);}$proofsha=Digest::SHA::sha256_hex($raw);
}else{
 need($expected=~/^[0-9a-f]{64}$/);my $row=readfile($proofpath,undef,1);need($row->{bytes}<=67108864&&$row->{sha256} eq $expected);my $raw=delete($row->{raw});need(Digest::SHA::sha256_hex($raw) eq $expected);$proof=$json->decode($raw);need($proof->{format} eq 'nano-fresh-rotation-owned-copy-private-proof-v1');$proofsha=$expected;$stability=$proof->{stability};
 for my $i(0,1){my %s=%{$proof->{sources}[$i]};need(delete($s{root}) eq $sources[$i]&&same(\%s,scan($sources[$i],0,{})));for my $z(values %{$s{entries}}){need(!$sourceids{identity($z)}++);}}
 for my $i(0,1){my %c=%{$proof->{copies}[$i]};need(delete($c{root}) eq $targets[$i]&&same(\%c,scan($targets[$i],1,\%sourceids)));}
}
my($files,$dirs,$bytes)=(0,0,0);for my $r(@{$proof->{sources}}){$files+=$r->{files};$dirs+=$r->{dirs};$bytes+=$r->{bytes};}my $yes=JSON::PP::true;my $no=JSON::PP::false;
print $json->encode({format=>$format,passed=>$yes,databaseRoles=>2,sourceFiles=>$files,sourceDirectories=>$dirs,payloadBytes=>$bytes,sourceNineFieldAndShaUnchanged=>$yes,sourceNamespaceUnchanged=>$yes,copiesMatchSourceBytes=>$yes,independentInodes=>$yes,privateDirectoryModes=>$yes,privateFileModes=>$yes,copiesUnchangedAfterAudit=>$mode eq 'verify'?$yes:$no,copyProofSha256=>$proofsha,%$stability,rawInventoryExported=>$no,productionQualified=>$no})."\n";
`;
