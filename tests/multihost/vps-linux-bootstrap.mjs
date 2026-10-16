// Fresh synthetic bootstrap transport only. No keys are imported or signed.
// The source is the already admitted Linux import, never the native SST tree.
import {createHash} from 'node:crypto';
const need=(v,m)=>{if(!v)throw Error('VPS Linux bootstrap: '+m);};
const sha=b=>createHash('sha256').update(b).digest('hex');
const parsed=b=>{try{return JSON.parse(b);}catch{throw Error('VPS Linux bootstrap: invalid bounded JSON');}};
const exact=(v,keys)=>need(v&&typeof v==='object'&&!Array.isArray(v)&&Object.keys(v).sort().join(',')===[...keys].sort().join(','),'exact fields');
const leaf=n=>typeof n==='string'&&/^[A-Za-z0-9_.-]{1,180}$/.test(n)&&!['.','..'].includes(n);
const path=p=>typeof p==='string'&&p.length>0&&p.length<=255&&p.split('/').every(leaf);
const identity=r=>r.device+':'+r.inode;
const inventoryKeys=['format','root','rootDevice','rootInode','rootUid','rootGid','rootMode','rootMtime','rootCtime','entries'];
const entryKeys=['path','kind','device','inode','uid','gid','mode','bytes','mtime','ctime','sha256'];
const stamp=s=>typeof s==='string'&&/^-?[0-9]+:[0-9]{1,9}$/.test(s);
const decimal=s=>typeof s==='string'&&/^(?:0|[1-9][0-9]*)$/.test(s);
const maximumArchive=16*1024*1024,maximumPayload=7*1024*1024;

// This selection preserves the original Linux inventory verbatim. It neither
// changes its owner/paths nor treats target inode metadata as source metadata.
export function selectVpsLinuxJournals(inventoryBytes,wholeInventoryBytes,namespace){
 need(Buffer.isBuffer(inventoryBytes)&&inventoryBytes.length>0&&inventoryBytes.length<=maximumArchive&&Buffer.isBuffer(wholeInventoryBytes)&&wholeInventoryBytes.length>0&&wholeInventoryBytes.length<=maximumArchive&&/^isolated-capella-[0-9a-f]{64}$/.test(namespace),'held inventories and campaign');
 const inventory=parsed(inventoryBytes),whole=parsed(wholeInventoryBytes);
 for(const [v,root] of [[inventory,'/bootstrap/import'],[whole,'/bootstrap']]){exact(v,inventoryKeys);need(v.format==='nano-isolated-bootstrap-file-inventory-v1'&&v.root===root&&v.rootUid===501&&v.rootGid===20&&v.rootMode===448&&decimal(v.rootDevice)&&decimal(v.rootInode)&&stamp(v.rootMtime)&&stamp(v.rootCtime)&&Array.isArray(v.entries)&&v.entries.length<=10000,'original Linux inventory root');}
 const names=new Set(),ids=new Set([inventory.rootDevice+':'+inventory.rootInode]),all=new Map();
 for(const r of inventory.entries){exact(r,entryKeys);need(path(r.path)&&!names.has(r.path)&&decimal(r.device)&&decimal(r.inode)&&!ids.has(identity(r))&&r.uid===501&&r.gid===20&&Number.isSafeInteger(r.bytes)&&r.bytes>=0&&r.bytes<=536870912&&stamp(r.mtime)&&stamp(r.ctime),'Linux entry identity');need(r.kind==='directory'&&r.mode===448&&r.sha256===null||r.kind==='file'&&r.mode===384&&/^[0-9a-f]{64}$/.test(r.sha256),'private Linux entry type/mode');names.add(r.path);ids.add(identity(r));all.set(r.path,r);}
 for(const r of inventory.entries){const at=r.path.lastIndexOf('/');if(at>=0)need(all.get(r.path.slice(0,at))?.kind==='directory','inventory parent exists');}
 const rows=[];let payload=0;
 for(let i=32;i<65;++i)for(const [role,target] of [['primary','data'],['witness','witness']]){
  const root=role+'-'+i;need(all.get(root)?.kind==='directory','all 33 distinct Linux primary/witness roots');
  for(const r of inventory.entries)if(r.path===root||r.path.startsWith(root+'/')){const suffix=r.path.slice(root.length),archivePath=target+'/'+namespace+'/vps-docker/'+(role==='primary'?'journal-':'witness-')+i+suffix;need(path(archivePath),'exact owner-scoped transport path');rows.push({sourcePath:'/bootstrap/import/'+r.path,archivePath,entry:r});if(r.kind==='file')payload+=r.bytes;}
 }
 need(rows.length>66&&rows.length<=9900&&payload<=maximumPayload,'bounded 66 Linux database trees');
 return {namespace,inventoryBytes:Buffer.from(inventoryBytes),wholeInventoryBytes:Buffer.from(wholeInventoryBytes),inventorySha256:sha(inventoryBytes),inventory,whole,rows,payload};
}

// No shell, writers, DB opener or arbitrary path. Input rows come from the
// original C++ Linux inventory; complete inventories are fenced by the caller
// before and after this FD reader, including failure paths.
export const vpsLinuxJournalReader=String.raw`use strict;use warnings;use Fcntl qw(:DEFAULT :mode);use Cwd qw(abs_path);use JSON::PP ();use Digest::SHA ();use Time::HiRes qw(clock_gettime CLOCK_MONOTONIC);
my$end=clock_gettime(CLOCK_MONOTONIC)+170;local$SIG{ALRM}=sub{die "Linux journal read refused\n"};alarm 175;sub need{die "Linux journal read refused\n" unless $_[0]}sub tick{need(clock_gettime(CLOCK_MONOTONIC)<$end)}
my$j=JSON::PP->new->canonical->ascii;my$b='';while(1){tick();my$n=sysread(STDIN,my$x,65536);need(defined$n);last unless$n;need(length($b)+$n<=16777216);$b.=$x;}my$c=$j->decode($b);need(ref$c eq'HASH'&&join(',',sort keys%$c)eq'format,inventory,rows,whole'&&$c->{format}eq'nano-vps-linux-journal-read-v1'&&ref$c->{rows}eq'ARRAY'&&@{$c->{rows}}>66&&@{$c->{rows}}<=9900&&$>==501&&$<==501);
sub row{my($p,$follow)=@_;tick();open(my$h,'-|','/usr/bin/stat',$follow?'-Lc':'-c','%d|%i|%f|%h|%u|%g|%s|%Y|%Z|%y|%z','--',$p)or die;local$/;my$s=<$h>;need(close($h)&&defined$s&&length$s<2048);my$n=($s=~s/\n\z//);need($n==1&&$s!~/[\r\n]/);my@v=split(/\|/,$s,-1);need(@v==11);my@t;for my$i(9,10){need($v[$i]=~/\.([0-9]{9}) [+-][0-9]{4}\z/);push@t,$v[$i-2].':'.(0+$1);}return[@v[0..6],@t];}
sub same{my($a,$b)=@_;return 0 unless ref$a eq'ARRAY'&&ref$b eq'ARRAY'&&@$a==9&&@$b==9;for my$i(0..8){return 0 unless defined$a->[$i]&&defined$b->[$i]&&!ref$a->[$i]&&!ref$b->[$i]&&$a->[$i]eq$b->[$i];}return 1;}sub fdpath{'/proc/'.$$.'/fd/'.fileno($_[0])}
sub matchrow{my($r,$e)=@_;need($r->[0]eq$e->{device}&&$r->[1]eq$e->{inode}&&$r->[4]==501&&$r->[5]==20&&$r->[6]==$e->{bytes}&&$r->[7]eq$e->{mtime}&&$r->[8]eq$e->{ctime});my$m=hex$r->[2];need(($m&07777)==$e->{mode}&&($e->{kind}eq'file'?S_ISREG($m)&&$r->[3]==1:S_ISDIR($m)));}
my@roots;for my$v($c->{whole},$c->{inventory}){my$p=$v->{root};need($p eq'/bootstrap'||$p eq'/bootstrap/import');need(abs_path($p)eq$p);sysopen(my$f,$p,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_NONBLOCK)or die;my$r=row(fdpath($f),1);need(same($r,row($p,0))&&S_ISDIR(hex$r->[2])&&$r->[0]eq$v->{rootDevice}&&$r->[1]eq$v->{rootInode}&&$r->[4]==501&&$r->[5]==20&&(hex($r->[2])&07777)==0700&&$r->[7]eq$v->{rootMtime}&&$r->[8]eq$v->{rootCtime});push@roots,[$p,$f,$r];}
my(%seen,%ids,@files);my$total=0;for my$z(@{$c->{rows}}){tick();need(ref$z eq'HASH'&&join(',',sort keys%$z)eq'archivePath,entry,sourcePath');my($p,$e)=@{$z}{qw(sourcePath entry)};need($p=~m{\A/bootstrap/import/(primary|witness)-(3[2-9]|[45][0-9]|6[0-4])(?:/[A-Za-z0-9_.-]+)*\z}&&!$seen{$p}++&&abs_path($p)eq$p&&$p eq'/bootstrap/import/'.$e->{path});my$m=$e->{kind}eq'file'?O_RDONLY:O_RDONLY|O_DIRECTORY;sysopen(my$f,$p,$m|O_NOFOLLOW|O_NONBLOCK)or die;my$r=row(fdpath($f),1);need(same($r,row($p,0)));matchrow($r,$e);need(!$ids{$r->[0].':'.$r->[1]}++);
 if($e->{kind}eq'file'){need($e->{bytes}<=7340032&&$e->{sha256}=~/\A[0-9a-f]{64}\z/);my$raw='';my$hash=Digest::SHA->new(256);while(1){tick();my$n=sysread($f,my$x,65536);need(defined$n);last unless$n;need(length($raw)+$n<=$e->{bytes}&&$total+$n<=7340032);$raw.=$x;$total+=$n;$hash->add($x);}need(length($raw)==$e->{bytes}&&$hash->hexdigest eq$e->{sha256});push@files,{path=>$z->{archivePath},hex=>unpack('H*',$raw)};}
 need(same($r,row(fdpath($f),1))&&same($r,row($p,0))&&abs_path($p)eq$p);close$f or die;}
for my$v(@roots){need(same($v->[2],row(fdpath($v->[1]),1))&&same($v->[2],row($v->[0],0))&&abs_path($v->[0])eq$v->[0]);close($v->[1])or die;}my$out=$j->encode({format=>'nano-vps-linux-journal-read-result-v1',files=>\@files});need(length$out<=16777216);print$out;alarm 0;
`;

export function vpsLinuxReadControl(selection){return Buffer.from(JSON.stringify({format:'nano-vps-linux-journal-read-v1',inventory:selection.inventory,whole:selection.whole,rows:selection.rows}));}
export function verifiedVpsLinuxFiles(selection,bytes){
 need(Buffer.isBuffer(bytes)&&bytes.length>0&&bytes.length<=maximumArchive,'private reader receipt bound');const result=parsed(bytes);exact(result,['format','files']);need(result.format==='nano-vps-linux-journal-read-result-v1'&&Array.isArray(result.files),'reader format');
 const expected=new Map(selection.rows.filter(x=>x.entry.kind==='file').map(x=>[x.archivePath,x.entry]));need(result.files.length===expected.size,'all source files exactly once');const files=[];
 for(const r of result.files){exact(r,['path','hex']);const e=expected.get(r.path);need(e&&typeof r.hex==='string'&&r.hex.length===2*e.bytes&&/^[0-9a-f]*$/.test(r.hex),'known Linux source file before decode');const b=Buffer.from(r.hex,'hex');need(b.length===e.bytes&&sha(b)===e.sha256,'known Linux source bytes');expected.delete(r.path);files.push({path:r.path,mode:384,bytes:b});}need(expected.size===0,'no missing source file');return files;
}

// Deterministic USTAR, known buffers only; no subprocess/tar traversal, owner
// import, symlink, hardlink, extended header or opaque native source path.
export function knownUstar(entries){
 need(Array.isArray(entries)&&entries.length>0&&entries.length<=9900,'bounded archive entries');const all=new Map([['',{path:'',mode:448,bytes:null}]]);
 for(const r of entries){exact(r,['path','mode','bytes']);need(path(r.path)&&!all.has(r.path)&&((r.bytes===null&&r.mode===448)||(Buffer.isBuffer(r.bytes)&&r.mode===384)),'known regular private file/directory');all.set(r.path,r);}
 for(const r of [...all.values()])if(r.path){let p=r.path;while(p.includes('/')){p=p.slice(0,p.lastIndexOf('/'));if(!all.has(p))all.set(p,{path:p,mode:448,bytes:null});else need(all.get(p).bytes===null,'archive parent must be a directory');}}
 need(all.size<=10000,'archive member count');const chunks=[];let total=1024;
 const oct=(h,at,width,n)=>{const s=n.toString(8);need(Number.isSafeInteger(n)&&n>=0&&s.length<width,'USTAR numeric bound');h.write(s.padStart(width-1,'0')+'\0',at,width,'ascii');};
 for(const r of [...all.values()].sort((a,b)=>a.path.split('/').length-b.path.split('/').length||a.path.localeCompare(b.path))){const h=Buffer.alloc(512),name=r.path?(r.path+(r.bytes===null?'/':'')):'./';let tail=name,prefix='';if(Buffer.byteLength(name)>100){const at=name.lastIndexOf('/',name.endsWith('/')?name.length-2:name.length-1);need(at>0,'USTAR path split');prefix=name.slice(0,at);tail=name.slice(at+1);}need(Buffer.byteLength(tail)<=100&&Buffer.byteLength(prefix)<=155,'USTAR path width');h.write(tail,0,100,'ascii');h.write(prefix,345,155,'ascii');oct(h,100,8,r.mode);oct(h,108,8,0);oct(h,116,8,0);oct(h,124,12,r.bytes?.length??0);oct(h,136,12,0);h.fill(32,148,156);h[156]=r.bytes===null?53:48;h.write('ustar\0',257,6,'ascii');h.write('00',263,2,'ascii');const sum=h.reduce((a,b)=>a+b,0),s=sum.toString(8);need(s.length<=6,'USTAR checksum');h.write(s.padStart(6,'0')+'\0 ',148,8,'ascii');chunks.push(h);if(r.bytes!==null){chunks.push(r.bytes);const pad=(512-r.bytes.length%512)%512;if(pad)chunks.push(Buffer.alloc(pad));}total+=512+Math.ceil((r.bytes?.length??0)/512)*512;need(total<=maximumArchive,'bounded transport USTAR');}
 chunks.push(Buffer.alloc(1024));return Buffer.concat(chunks,total);
}

export function buildVpsLinuxTransport({selection,readerBytes,configuration,planBytes}){
 const names=['genesis.json','profile.json','initial.json','isolated-domain.json','node.json','fixture.env'];need(Array.isArray(configuration)&&configuration.length===6&&configuration.every(x=>names.includes(x.name)&&Buffer.isBuffer(x.bytes)&&x.bytes.length>0)&&new Set(configuration.map(x=>x.name)).size===6,'only six owner-scoped configuration files');
 const env=configuration.find(x=>x.name==='fixture.env').bytes.toString('utf8'),lines=env.split('\n');need(lines.pop()===''&&lines.length===33,'exact VPS environment');for(let i=0;i<33;++i)need(new RegExp('^NANO_TEST_CAPELLA_NETWORK_KEY_'+(32+i)+'=0x[0-9a-f]{64}$').test(lines[i]),'no Mac owner environment transferred');
 const plan=parsed(planBytes);need(plan.campaignNamespace===selection.namespace&&plan.owners?.[2]?.owner==='vps-docker'&&plan.owners[2].keys.length===33&&plan.owners[2].keys.every((k,i)=>k.publicKeyIndex===32+i),'original plan VPS assignment');const manifestBytes=configuration.find(x=>x.name==='isolated-domain.json').bytes;need(sha(manifestBytes)===plan.inputEvidencePins.manifest,'original manifest pin');
 const anchors=Buffer.from(JSON.stringify({format:'nano-isolated-bootstrap-physical-audit-anchors-v1',fixtureManifestSha256:plan.inputEvidencePins.manifest,planDocumentSha256:sha(planBytes),planSha256:plan.planSha256,sourceInventorySha256:selection.inventorySha256})+'\n');
 const files=verifiedVpsLinuxFiles(selection,readerBytes),directories=selection.rows.filter(x=>x.entry.kind==='directory').map(x=>({path:x.archivePath,mode:448,bytes:null}));
 const config=knownUstar([...configuration.map(x=>({path:x.name,mode:384,bytes:x.bytes})),...directories,...files]),proof=knownUstar([{path:'plan.json',mode:384,bytes:planBytes},{path:'anchors.json',mode:384,bytes:anchors},{path:'source-inventory.json',mode:384,bytes:selection.inventoryBytes}]);
 return {archives:[{name:'config',bytes:config},{name:'proof',bytes:proof}],expected:{campaignId:plan.campaignId,manifestSha256:plan.inputEvidencePins.manifest,planDocumentSha256:sha(planBytes),planSha256:plan.planSha256,anchorsSha256:sha(anchors),sourceInventorySha256:selection.inventorySha256,keys:plan.owners[2].keys},publicScope:{format:'nano-vps-linux-bootstrap-transport-v1',sourceOwner:'mac-docker',sourceDatabaseFormat:'existing-linux-import',primaryWitnessPairs:33,sourceFiles:files.length,sourceInventorySha256:selection.inventorySha256,nativeSstFilesTransferred:false,macOwnerPrivateKeysTransferred:false,nativeEvidenceReplaced:false,sourceAuthorityRebased:false,newSignaturesCreated:0,runtimeStarted:false,productionQualified:false}};
}

export async function exportVpsLinuxBootstrap({selection,configuration,planBytes,read,fenceSource,verifyAll}){
 need(typeof read==='function'&&typeof fenceSource==='function'&&typeof verifyAll==='function','mandatory original-source fences');await fenceSource();await verifyAll();let readerBytes;
 try{readerBytes=await read(vpsLinuxReadControl(selection));}
 finally{try{await fenceSource();}finally{await verifyAll();}}
 return buildVpsLinuxTransport({selection,readerBytes,configuration,planBytes});
}
