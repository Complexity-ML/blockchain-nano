// ROOT-only prospective local qualification. No operation occurs on import.
import {open,lstat,readdir,realpath,mkdir,mkdtemp} from 'node:fs/promises';
import {constants} from 'node:fs';
import {spawn} from 'node:child_process';
import {createHash} from 'node:crypto';
import {join,dirname,resolve,basename} from 'node:path';
import {tmpdir} from 'node:os';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {setTimeout as delay} from 'node:timers/promises';
import {checkFreshStartupAge} from './assemble-local-inputs.mjs';

const need=b=>{if(!b)throw Error('Local supervisor qualification refused');};
const sha=b=>createHash('sha256').update(b).digest('hex');
const fields=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const row=s=>fields.map(k=>String(s[k]));
const same=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const canonical=v=>JSON.stringify(v,(_k,x)=>x&&typeof x==='object'&&!Array.isArray(x)?Object.fromEntries(Object.entries(x).sort()):x);
const exact=(v,keys)=>need(v&&typeof v==='object'&&!Array.isArray(v)&&same(Object.keys(v).sort(),[...keys].sort()));
const q=n=>'0x'+BigInt(n).toString(16);
const fixedOwner='mac-native'; // Logical 16-key fixture owner; physical execution is Docker ARM64.
const maxFile=64*1024*1024,maxLog=32*1024*1024;
const transportSha='7dd88b7d99ab15e9f2b1e52aac796265e6ed9bed24323e66584b3f31138cc0f6',transportBytes=12273;

export async function runLocalSupervisor(workOrderPath,expectedSha,outputPath){
 let stage='inputs',output=null,controls=null,work=null,deadline=performance.now()+900000,interrupted=false,cleaning=false,logOrdinal=0;
 const held=new Map(),produced=new Map(),runs=new Set(),clients=new Map(),linuxAuthorities=[];
 let linuxOrdinal=0,transportReady=false;
 const check=()=>need(cleaning||(!interrupted&&performance.now()<deadline));
 const signal=()=>{interrupted=true;};process.on('SIGINT',signal);process.on('SIGTERM',signal);
 const absolute=p=>{need(typeof p==='string'&&p.startsWith('/')&&resolve(p)===p&&p!=='/'&&!/[\0\r\n,]/.test(p));return p;};
 async function path(p){absolute(p);need(await realpath(p)===p);}
 async function read(p,pin=null,admit=true){
  check();await path(p);const f=await open(p,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
  try{const a=await f.stat({bigint:true});need(a.isFile()&&a.nlink>=1n&&a.size>=0n&&a.size<=BigInt(maxFile));
   const b=await f.readFile();need(BigInt(b.length)===a.size);const z=await f.stat({bigint:true}),s=await lstat(p,{bigint:true});need(same(row(a),row(z))&&same(row(a),row(s)));
   const h=sha(b);if(pin){exact(pin,['path','sha256','bytes']);need(pin.path===p&&pin.sha256===h&&pin.bytes===b.length);}
   const old=held.get(p);if(old)need(same(old.material,row(a))&&old.sha256===h);
   if(admit&&!old)held.set(p,{material:row(a),sha256:h});return {bytes:b,material:row(a),sha256:h};
  }finally{await f.close();}
 }
 async function fence(){for(const p of held.keys())await read(p);}
 async function write(p,b){
  need(Buffer.isBuffer(b));const f=await open(p,constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);
  try{await f.writeFile(b);await f.sync();const s=await f.stat({bigint:true});need(s.isFile()&&s.nlink===1n&&s.uid===BigInt(process.getuid())&&(s.mode&0o777n)===0o600n);produced.set(p,{sha256:sha(b),bytes:b.length,identity:row(s).slice(0,6)});}finally{await f.close();}
  const d=await open(dirname(p),constants.O_RDONLY|constants.O_DIRECTORY|constants.O_NOFOLLOW);try{await d.sync();}finally{await d.close();}
 }
 async function save(name,v){need(output);const p=join(output,name);await write(p,Buffer.isBuffer(v)?v:Buffer.from(JSON.stringify(v)+'\n'));return p;}
 async function settle(paths){
  const began=performance.now();let previous=null,stable=0,since=0,last;
  while(performance.now()-began<30000){check();await fence();last=[];
   for(const p of paths){const r=await read(p,null,false),expected=produced.get(p);need(expected&&r.sha256===expected.sha256&&r.bytes.length===expected.bytes&&same(r.material.slice(0,6),expected.identity));last.push([p,r.material,r.sha256]);}
   const now=performance.now();if(same(last,previous))++stable;else{stable=1;since=now;}previous=last;
   if(stable>=3&&now-since>=2000){for(const [p,m,h] of last){const r=await read(p,null,false);need(same(r.material,m)&&r.sha256===h);need(!held.has(p));held.set(p,{material:m,sha256:h});}await fence();return;}
   await delay(1000);
  }throw Error('Local supervisor qualification refused');
 }
 async function control(name,v){
  need(transportReady&&/^[A-Za-z0-9_.-]{1,180}$/.test(name)&&name!=='.'&&name!=='..'&&!name.startsWith('.known-files-'));
  const p=join(controls,name);await write(p,Buffer.isBuffer(v)?v:Buffer.from(JSON.stringify(v)));await settle([p]);
  const b=(await read(p)).bytes;await linuxWrite('/known',[{name,body:b}],'control-'+(++linuxOrdinal));
  return {path:'/known/'+name,sha256:produced.get(p).sha256,bytes:produced.get(p).bytes};
 }
 async function run(file,args,name,ms=60000,input=null){
  check();need(Number.isInteger(ms)&&ms>0&&ms<=180000);name=String(++logOrdinal).padStart(3,'0')+'-'+name;const logs=[[],[]],sizes=[0,0];let timedOut=false,overflow=false,spawned=false;
  need(input===null||Buffer.isBuffer(input)&&input.length<=128*1024*1024);let inputError=false;
  const child=spawn(file,file===docker?['--host',work.dockerEndpoint,...args]:args,{stdio:[input===null?'ignore':'pipe','pipe','pipe'],env:{PATH:process.env.PATH,HOME:process.env.HOME}});
  if(input!==null){child.stdin.on('error',()=>{inputError=true;});child.stdin.end(input);}
  const done=new Promise(res=>{child.once('spawn',()=>{spawned=true;});child.once('error',()=>res({code:null,signal:null,spawnError:true}));child.once('close',(code,signal)=>res({code,signal}));});
  for(let i=0;i<2;i++)[child.stdout,child.stderr][i].on('data',b=>{sizes[i]+=b.length;if(sizes[i]<=maxLog)logs[i].push(b);else{overflow=true;child.kill('SIGKILL');}});
  const timer=setTimeout(()=>{timedOut=true;child.kill('SIGKILL');},cleaning?ms:Math.min(ms,Math.max(1,deadline-performance.now())));
  let exit;try{exit=await done;}finally{clearTimeout(timer);}
  const stdout=Buffer.concat(logs[0]),stderr=Buffer.concat(logs[1]);await save(name+'.stdout',stdout);await save(name+'.stderr',stderr);await save(name+'.exit.json',{spawned,...exit,timedOut,overflow});
  need(spawned&&!timedOut&&!overflow&&!inputError&&exit.code===0&&exit.signal===null);check();return stdout;
 }
 const mount=(s,t,ro=true)=>['--mount',`type=bind,src=${s},dst=${t}${ro?',readonly':''}`];
 const volume=(s,t,ro=true)=>['--mount',`type=volume,src=${s},dst=${t}${ro?',readonly':''}`];
 let docker,image,label,rootConfig,manifest,identities,copyModule,restoreModule,dataRoot,witnessRoot;
 const name=s=>work.containerPrefix+'-'+s;
 async function owns(n){const x=JSON.parse((await run(docker,['inspect',n],'inspect-'+n,10000)).toString())[0];need(x.Name==='/'+n&&x.Image===image&&x.Config.Labels?.['nano.local-supervisor']===label);return x;}
 async function absent(n){const b=await run(docker,['ps','-a','--format','{{.Names}}','--filter','name=^/'+n+'$'],'absent-'+n+'-'+runs.size,10000);need(!b.toString().trim());}
 function common(n,{dataRO=true,copyRO=true,workRO=true,configRO=true,controlRO=true,envFile=null,bootstrap=true}={}){
  const a=['run','--name',n,'--label','nano.local-supervisor='+label,'--pull','never','--platform','linux/arm64','--network','none','--cpus','2','--memory','4g','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user',work.uid+':'+work.gid,'--tmpfs','/tmp:rw,noexec,nosuid,nodev,size=128m','--tmpfs',`/run:rw,noexec,nosuid,nodev,size=1m,uid=${work.uid},gid=${work.gid},mode=700`];
  a.push(...mount(work.programDirectory,'/core'),...mount(work.networkDirectory,'/network-build'),...volume(work.configVolume,'/config',configRO),...volume(work.controlVolume,'/known',controlRO),...volume(work.dataVolume,'/data',dataRO),...volume(work.witnessVolume,'/witness',dataRO));
  if(bootstrap)a.push(...volume(work.bootstrapVolume,'/bootstrap'));if(work.copyVolume)a.push(...volume(work.copyVolume,'/rotation-audit',copyRO));if(work.actionVolume)a.push(...volume(work.actionVolume,'/action-output',workRO));if(envFile)a.push('--env-file',envFile);return a;
 }
 async function job(s,argv,opts={},ms=90000,input=null){const n=name(s);await absent(n);runs.add(n);return run(docker,[...common(n,opts),...(input===null?[]:['-i']),'--entrypoint',argv[0],image,...argv.slice(1)],s,ms,input);}
 function transportResult(r,mode,files,payloadBytes){
  exact(r,['format','mode','passed','filesWritten','filesVerified','payloadBytes','knownBytesMatched','fullNineFieldPinsRetained','privateModesAndOwnerVerified','rootFdIdentityRetained','existingFilesMutated','namespaceInventoryQualified','authorityName','authoritySha256','authorityBytes','authorityMaterial','stabilitySamples','stabilitySpanMs','productionQualified']);
  need(r.format==='nano-linux-known-files-result-v1'&&r.mode===mode&&r.passed===true&&r.filesWritten===(mode==='write'?files:0)&&r.filesVerified===files&&r.payloadBytes===payloadBytes);
  for(const k of ['knownBytesMatched','fullNineFieldPinsRetained','privateModesAndOwnerVerified','rootFdIdentityRetained'])need(r[k]===true);
  need(r.existingFilesMutated===false&&r.namespaceInventoryQualified===false&&r.productionQualified===false&&/^\.known-files-[0-9a-f]{64}\.authority\.private\.json$/.test(r.authorityName)&&/^[0-9a-f]{64}$/.test(r.authoritySha256)&&Number.isSafeInteger(r.authorityBytes)&&r.authorityBytes>0&&r.authorityBytes<=1048576);
  need(Array.isArray(r.authorityMaterial)&&r.authorityMaterial.length===9&&r.authorityMaterial.every(x=>typeof x==='string'&&x.length>0&&x.length<256&&!/[\0\r\n]/.test(x)));
  need(Number.isSafeInteger(r.stabilitySamples)&&r.stabilitySamples>=3&&r.stabilitySamples<=65536&&Number.isSafeInteger(r.stabilitySpanMs)&&r.stabilitySpanMs>=2000&&r.stabilitySpanMs<=30000);return r;
 }
 async function linuxWrite(root,files,tag){
  need(transportReady&&(root==='/known'||root==='/config')&&files.length>0&&files.length<=64);
  const names=new Set();let bytes=0;const frameFiles=files.map(({name,body})=>{need(/^[A-Za-z0-9_.-]{1,180}$/.test(name)&&name!=='.'&&name!=='..'&&!name.startsWith('.known-files-')&&!names.has(name)&&Buffer.isBuffer(body)&&body.length<=maxFile);names.add(name);bytes+=body.length;return {name,sha256:sha(body),bytes:body.length,hex:body.toString('hex')};});need(bytes<=maxFile);
  const frame=Buffer.from(canonical({format:'nano-linux-known-files-v1',files:frameFiles}));need(frame.length<=128*1024*1024);
  const r=transportResult(JSON.parse((await job('transport-write-'+tag,['perl','/core/known-files.pl','write',root,sha(frame),String(frame.length)],{configRO:root!=='/config',controlRO:root!=='/known'},40000,frame)).toString()),'write',files.length,bytes);
  need(r.authorityName==='.known-files-'+sha(frame)+'.authority.private.json');linuxAuthorities.push({root,files:files.length,payloadBytes:bytes,authorityName:r.authorityName,authoritySha256:r.authoritySha256,authorityBytes:r.authorityBytes,authorityMaterial:r.authorityMaterial});
  await save('linux-authority-'+linuxAuthorities.length+'.private.json',r);return r;
 }
 async function linuxFence(tag){
  need(transportReady);for(const [i,a]of linuxAuthorities.entries()){
   const input=Buffer.from(canonical({format:'nano-linux-known-files-verify-v1',authorityMaterial:a.authorityMaterial}));need(input.length<=4096);
   const r=transportResult(JSON.parse((await job('transport-verify-'+tag+'-'+i,['perl','/core/known-files.pl','verify',a.root,a.authoritySha256,String(a.authorityBytes),a.authorityName],{},40000,input)).toString()),'verify',a.files,a.payloadBytes);
   need(r.authorityName===a.authorityName&&r.authoritySha256===a.authoritySha256&&r.authorityBytes===a.authorityBytes&&same(r.authorityMaterial,a.authorityMaterial));
  }
 }
 async function initVolume(s,v){
  const n=name('init-'+s);await absent(n);runs.add(n);
  const existing=await run(docker,['volume','ls','-q','--filter','name=^'+v+'$'],'new-volume-'+s,10000);need(!existing.toString().trim());
  await run(docker,['volume','create','--label','nano.local-supervisor='+label,v],'create-'+s,10000);
  await run(docker,['run','--name',n,'--label','nano.local-supervisor='+label,'--pull','never','--network','none','--cpus','2','--memory','128m','--read-only','--cap-drop','ALL','--cap-add','CHOWN','--security-opt','no-new-privileges','--user','0:0',...volume(v,'/new',false),'--entrypoint','/bin/sh',image,'-ec',`test -z "$(ls -A /new)"; chmod 700 /new; chown ${work.uid}:${work.gid} /new`],'init-'+s,10000);
 }
 async function inventory(s){return job(s,['/core/nano-capella-isolated-bootstrap-audit','--inventory','/bootstrap']);}
 function rows(b){return b.toString().split('\n').flatMap(s=>{try{const x=JSON.parse(s);return x&&typeof x==='object'?[x]:[];}catch{return [];}});}
 function one(xs,event){const a=xs.filter(x=>x.event===event);need(a.length===1);return a[0];}
 async function start(s,config,plan=null){
  await linuxFence(s+'-before');const n=name(s);await absent(n);runs.add(n);const args=common(n,{dataRO:false,workRO:!plan,envFile:work.environment.path});args.push('--init');
  if(plan)args.push('-e','NANO_TEST_FRESH_SUBMISSION_PLAN='+plan.path,'-e','NANO_TEST_FRESH_SUBMISSION_PLAN_SHA256='+plan.sha256);
  args.push('--entrypoint','/core/nano',image,'run','/config/genesis.json',dataRoot+'/chain',config.path);
  let startupAge=null;if(!plan){stage='first-start-clock-preflight';const initial=JSON.parse((await read(work.publicFiles['initial.json'].path)).bytes),profile=JSON.parse((await read(work.publicFiles['profile.json'].path)).bytes);startupAge=checkFreshStartupAge({durableTimeMs:initial.clock[1],capella:rootConfig.capella,slotDurationMs:profile.common[1],nowMs:BigInt(Date.now())});stage='first-runtime';}
  const child=spawn(docker,['--host',work.dockerEndpoint,...args],{stdio:['ignore','pipe','pipe'],env:{PATH:process.env.PATH,HOME:process.env.HOME}}),buffers=[[],[]],sizes=[0,0];
  const item={child,closed:false,spawned:false,exit:null,overflow:false,buffers};clients.set(n,item);
  item.done=new Promise(res=>{child.once('spawn',()=>{item.spawned=true;});child.once('error',()=>{item.closed=true;item.exit={code:null,signal:null,spawnError:true};res();});child.once('close',(code,signal)=>{item.closed=true;item.exit={code,signal};res();});});
  for(let i=0;i<2;i++)[child.stdout,child.stderr][i].on('data',b=>{sizes[i]+=b.length;if(sizes[i]<=maxLog)buffers[i].push(b);else{item.overflow=true;child.kill('SIGKILL');}});
  if(startupAge)await save('first-start-clock-preflight.json',startupAge);
  const until=performance.now()+90000;while(performance.now()<until){check();need(!item.closed&&!item.overflow);const xs=rows(Buffer.concat(buffers[0]));if(['capella-runtime-ready','p2p-process','altair-ipc-ready',...(plan?['fresh-lifecycle-child-submission-accepted']:[])].every(e=>xs.some(x=>x.event===e))){
    const ready=one(xs,'capella-runtime-ready'),ipc=one(xs,'altair-ipc-ready'),p2p=one(xs,'p2p-process');need(ready.executionBound===true&&ipc.profile==='capella'&&ipc.networkProtocolActive===true&&Number.isSafeInteger(p2p.pid)&&p2p.pid>1);if(plan){const e=one(xs,'fresh-lifecycle-child-submission-accepted');exact(e,['event','accepted','receiptSha256','receiptBytes','authExported','newSignaturesCreated','execPending','productionQualified']);need(e.accepted===true&&e.authExported===false&&e.newSignaturesCreated===0&&e.execPending===true&&e.productionQualified===false);}
    // Read executable identities only: never /proc/*/environ, cmdline, memory or tokens.
    const probe='use strict;use warnings;use JSON::PP;my $p=shift;die unless $p=~/^[1-9][0-9]*$/;open(my $f,"<","/proc/$p/status")or die;my $s=do{local $/;<$f>};close $f;my($a)=$s=~/^PPid:\\s+(\\d+)$/m;die unless $a&&$a>1;my $c=readlink("/proc/$p/exe");my $n=readlink("/proc/$a/exe");die unless defined$c&&defined$n&&$c eq "/network-build/nano-p2p"&&$n eq "/core/nano";print JSON::PP->new->canonical->encode({childIsRealP2p=>JSON::PP::true,parentIsRealNano=>JSON::PP::true,parentChildRelationshipVerified=>JSON::PP::true});';
    const result=JSON.parse((await run(docker,['exec','--user',work.uid+':'+work.gid,n,'perl','-e',probe,String(p2p.pid)],s+'-proc',10000)).toString());exact(result,['childIsRealP2p','parentIsRealNano','parentChildRelationshipVerified']);need(Object.values(result).every(x=>x===true));return {n,item,ipc,accepted:plan?one(xs,'fresh-lifecycle-child-submission-accepted'):null,processIdentity:result};
   }await delay(200);
  }throw Error('Local supervisor qualification refused');
 }
 async function validatorHttp(first){
  const port=Number(rootConfig.rpc_port);need(Number.isSafeInteger(port)&&port>0&&port<=65535);
  const positive=work.identities.slashing.publicKey,negative='0x'+'00'.repeat(48);need(/^0x[0-9a-f]{96}$/.test(positive)&&!manifest.publicKeys.some(x=>x.publicKey===negative));
  const client='use strict;use warnings;use IO::Socket::INET;my($p,$path)=@ARGV;die unless $p=~/^[1-9][0-9]*$/&&$p<=65535&&$path=~m{^/eth/v1/beacon/states/head/validators/0x[0-9a-f]{96}$};local$SIG{ALRM}=sub{die "bounded HTTP refused\\n"};alarm 2;my$s=IO::Socket::INET->new(PeerAddr=>"127.0.0.1",PeerPort=>$p,Proto=>"tcp",Timeout=>2)or die;$s->autoflush(1);print $s "GET $path HTTP/1.1\\r\\nHost: 127.0.0.1\\r\\nAccept: application/json\\r\\nConnection: close\\r\\n\\r\\n" or die;my$r="";while(1){my$n=sysread($s,my$b,4096);die unless defined$n;last unless$n;die if length($r)+$n>65536;$r.=$b;}close$s;alarm 0;print$r;';
  function decode(raw){need(raw.length>0&&raw.length<=65536);const at=raw.indexOf('\r\n\r\n');need(at>0&&at<=8192);const head=raw.subarray(0,at).toString('ascii').split('\r\n'),status=/^HTTP\/1\.[01] ([0-9]{3}) /.exec(head.shift());need(status);const headers=new Map();for(const line of head){const i=line.indexOf(':');need(i>0);const k=line.slice(0,i).toLowerCase(),v=line.slice(i+1).trim();need(!headers.has(k)&&k!=='transfer-encoding');headers.set(k,v);}need(/^(?:0|[1-9][0-9]*)$/.test(headers.get('content-length'))&&Number(headers.get('content-length'))===raw.length-at-4);need(headers.get('content-type')?.startsWith('application/json'));return {status:Number(status[1]),body:JSON.parse(raw.subarray(at+4))};}
  const results=[];for(const [i,key]of [positive,negative].entries()){const raw=await run(docker,['exec','--user',work.uid+':'+work.gid,first.n,'perl','-e',client,String(port),'/eth/v1/beacon/states/head/validators/'+key],'validator-http-'+(i+1),5000);results.push(decode(raw));}
  const [yes,no]=results;need(yes.status===200&&yes.body.execution_optimistic===false&&typeof yes.body.finalized==='boolean'&&yes.body.data?.validator?.pubkey===positive&&/^(?:0|[1-9][0-9]*)$/.test(yes.body.data.index));need(no.status===404&&no.body.code===404&&no.body.message==='Validator not found in selected state');
  return {publicKey:positive,index:yes.body.data.index,reads:2,positive200:1,negative404:1};
 }
 async function stop(r,s){
  await owns(r.n);await run(docker,['stop','--time','30',r.n],s+'-stop-command',35000);await Promise.race([r.item.done,delay(5000).then(()=>{throw Error('Local supervisor qualification refused');})]);
  need(r.item.spawned&&!r.item.overflow&&r.item.exit.code===0&&r.item.exit.signal===null);const out=Buffer.concat(r.item.buffers[0]);await save(s+'.stdout',out);await save(s+'.stderr',Buffer.concat(r.item.buffers[1]));await save(s+'.exit.json',r.item.exit);r.item.saved=true;
  const receipt=one(rows(out),'capella-runtime-stopped');need(receipt.clockHealthy===true&&receipt.validator?.healthy===true&&receipt.validator.failure===null&&receipt.validator.localKeys===16&&receipt.validatorKeys?.length===16);return receipt;
 }
 async function capture(s,config,stop){
  const r=JSON.parse((await job(s,['/core/nano-capella-isolated-stopped-capture','--capture','/config/isolated-domain.json',config.path,stop.path,dataRoot,witnessRoot,fixedOwner])).toString());
  for(const k of ['passed','canonicalStoreSnapshotVerified','primaryAndWitnessBlobClosuresVerified','allObservedFileBytesAndInodesUnchanged','observedWitnessPairsAuthenticated','externalStopTokensMatched'])need(r[k]===true);
  need(r.owner===fixedOwner&&r.primaryWitnessPairs===16&&r.newSignaturesCreated===0&&r.journalWritesRequested===false);return r;
 }
 async function pair(s,config,receipt){
  const firstStop=await control(s+'-stop-first.json',receipt),first=await capture(s+'-capture-first',config,firstStop);need(first.externalWitnessTokensMatched===false);
  const raw=Buffer.from(JSON.stringify(first));const stop={...receipt,witnessCommitments:{format:'nano-isolated-stopped-external-witness-commitments-v1',sourceCaptureSha256:sha(raw),core:first.core.witness,validatorKeys:first.validatorKeys.map(k=>k.index===null?{index:null,publicKey:k.publicKey,restart:k.witness}:{index:k.index,restart:k.witness})}};
  await save(s+'-first-capture.private.json',raw);const externalStop=await control(s+'-stop-second.json',stop),second=await capture(s+'-capture-second',config,externalStop);
  need(second.externalWitnessTokensMatched===true&&second.externalWitnessPinOriginExternallyRequired===true&&same(second.fullFileInodeInventories,first.fullFileInodeInventories)&&same(second.core,first.core)&&same(second.validatorKeys,first.validatorKeys));
  await save(s+'-second-capture.private.json',second);return {stop,externalStop,capture:second};
 }
 async function cleanup(){
  cleaning=true;let failed=false;for(const n of runs){try{const x=await owns(n);if(x.State.Running)await run(docker,['stop','--time','30',n],'cleanup-stop-'+n,35000);await run(docker,['rm','-f',n],'cleanup-remove-'+n,15000);await absent(n);}catch{failed=true;}}
  for(const [n,x] of clients)try{await Promise.race([x.done,delay(5000).then(()=>{throw Error();})]);if(!x.saved){await save(n+'-cleanup.stdout',Buffer.concat(x.buffers[0]));await save(n+'-cleanup.stderr',Buffer.concat(x.buffers[1]));await save(n+'-cleanup.exit.json',x.exit);x.saved=true;}}catch{failed=true;}
  need(!failed);
 }
 let result,error;
 try{
  need(/^[0-9a-f]{64}$/.test(expectedSha));const order=await read(absolute(workOrderPath));need(order.sha256===expectedSha);work=JSON.parse(order.bytes);
  exact(work,['format','uid','gid','image','docker','dockerEndpoint','programDirectory','networkDirectory','publicDirectory','programs','modules','publicFiles','configuration','environment','plan','anchors','sourceInventory','importAudit','freshAuthority','bootstrapVolume','dataVolume','witnessVolume','containerPrefix','identities','resources','authorizeSyntheticProposerSlashing','transportScript','configVolume','controlVolume']);
  need(work.format==='nano-local-supervisor-work-order-v2'&&work.uid===501&&work.gid===20&&process.getuid()===501&&process.getgid()===20&&work.authorizeSyntheticProposerSlashing===true&&/^sha256:[0-9a-f]{64}$/.test(work.image)&&/^nano-local-[a-z0-9-]{8,60}$/.test(work.containerPrefix));image=work.image;
  for(const p of [work.programDirectory,work.networkDirectory,work.publicDirectory])await path(p);
  const expectedPrograms=['nano','nano-p2p','nano-capella-isolated-bootstrap-audit','nano-capella-isolated-stopped-capture','nano-capella-deposit-audit','nano-capella-lifecycle-submit'];exact(work.programs,expectedPrograms);
  for(const n of expectedPrograms){const a=work.programs[n];need(a.path===join(n==='nano-p2p'?work.networkDirectory:work.programDirectory,n));const b=await read(a.path,a);need(b.bytes.length>=64&&b.bytes.subarray(0,4).equals(Buffer.from([127,69,76,70]))&&b.bytes[4]===2&&b.bytes[5]===1&&b.bytes.readUInt16LE(18)===183&&BigInt(b.material[3])===1n&&BigInt(b.material[4])===501n&&(BigInt(b.material[2])&0o111n)!==0n);}
  need(typeof work.dockerEndpoint==='string'&&work.dockerEndpoint.startsWith('unix:///'));const socket=work.dockerEndpoint.slice(7);await path(socket);need((await lstat(socket)).isSocket());docker=work.docker.path;await read(docker,work.docker);await read(await realpath(process.execPath));await read(await realpath(fileURLToPath(import.meta.url)));await read(await realpath(fileURLToPath(new URL('./assemble-local-inputs.mjs',import.meta.url))));
  need(work.transportScript.path===join(work.programDirectory,'known-files.pl')&&work.transportScript.sha256===transportSha&&work.transportScript.bytes===transportBytes);await read(work.transportScript.path,work.transportScript);
  exact(work.modules,['restore','copy']);for(const a of Object.values(work.modules))await read(a.path,a);
  restoreModule=await import(pathToFileURL(work.modules.restore.path));copyModule=await import(pathToFileURL(work.modules.copy.path));need(typeof restoreModule.pinnedRestoreConfiguration==='function'&&typeof copyModule.linuxRotationCopy==='string'&&typeof copyModule.validateRotationCopySummary==='function');
  for(const k of ['configuration','environment','plan','anchors','sourceInventory','importAudit','freshAuthority'])await read(work[k].path,work[k]);
  const expectedPublic=['isolated-domain.json','genesis.json','profile.json','initial.json','genesis-validators.ssz','genesis-state.ssz'];need(same((await readdir(work.publicDirectory)).sort(),expectedPublic.sort()));exact(work.publicFiles,expectedPublic);for(const [n,a]of Object.entries(work.publicFiles)){need(a.path===join(work.publicDirectory,n));await read(a.path,a);}
  manifest=JSON.parse((await read(work.publicFiles['isolated-domain.json'].path)).bytes);need(manifest.synthetic===true&&manifest.productionQualified===false&&manifest.registryCount===64&&manifest.publicKeys.length===65&&/^[0-9a-f]{64}$/.test(manifest.campaignId));label=manifest.campaignId;
  const authority=JSON.parse((await read(work.freshAuthority.path)).bytes);exact(authority,['format','campaignId','campaignNamespace','freshLedgerReserved','runtimePreviouslyStarted','bootstrapVolume','dataVolume','witnessVolume','bootstrapInventorySha256']);need(authority.format==='nano-local-supervisor-fixture-authority-v1'&&authority.campaignId===label&&authority.campaignNamespace==='isolated-capella-'+label&&authority.freshLedgerReserved===true&&authority.runtimePreviouslyStarted===false);
  for(const k of ['bootstrapVolume','dataVolume','witnessVolume'])need(work[k]===authority[k]&&/^[a-zA-Z0-9][a-zA-Z0-9_.-]{0,120}$/.test(work[k]));need(work.configVolume===work.containerPrefix+'-config'&&work.controlVolume===work.containerPrefix+'-known');
  for(const v of [work.configVolume,work.controlVolume])need(/^[a-zA-Z0-9][a-zA-Z0-9_.-]{0,120}$/.test(v));
  need(new Set([work.bootstrapVolume,work.dataVolume,work.witnessVolume,work.configVolume,work.controlVolume]).size===5);
  const imported=JSON.parse((await read(work.importAudit.path)).bytes);need(imported.passed===true&&imported.campaignId===label&&imported.inputManifestSha256===work.publicFiles['isolated-domain.json'].sha256&&imported.validatorPrivatePublicBindings===65&&imported.configKeyBindings===65&&imported.bootstrapBlsSignaturesVerified===4&&imported.offlineJournalPrimariesCreated===65&&imported.offlineJournalWitnessesCreated===65&&imported.offlineJournalPairsReopened===65&&imported.newSignaturesCreated===0&&imported.runtimeStarted===false); // Actual destination bytes/tokens are rechecked below.
  rootConfig=JSON.parse((await read(work.configuration.path)).bytes);need(rootConfig.rpc_host==='127.0.0.1'&&rootConfig.bootnodes?.length===0&&rootConfig.p2p_binary==='/network-build/nano-p2p'&&!rootConfig.altair&&!rootConfig.bellatrix&&!rootConfig.bft&&rootConfig.capella.automaticRecovery!==true&&rootConfig.capella.initial?.file==='/config/initial.json'&&rootConfig.capella.profile?.file==='/config/profile.json');
  dataRoot='/data/'+authority.campaignNamespace+'/'+fixedOwner;witnessRoot='/witness/'+authority.campaignNamespace+'/'+fixedOwner;
  const assignment=JSON.parse((await read(work.plan.path)).bytes).owners.find(x=>x.owner===fixedOwner);need(assignment?.keys.length===16&&rootConfig.capella.validator.keys.length===16&&rootConfig.capella.recoveryWitness===witnessRoot+'/chain-witness');identities=assignment.keys.map(k=>({keyEnv:k.runtimeKey.keyEnv,publicKey:k.publicKey}));
  const envRead=await read(work.environment.path);need((BigInt(envRead.material[2])&0o777n)===0o600n&&BigInt(envRead.material[3])===1n&&BigInt(envRead.material[4])===501n);const envLines=envRead.bytes.toString().trim().split('\n');need(envLines.length===16);const env=Object.fromEntries(envLines.map(s=>{need(/^[A-Z0-9_]+=0x[0-9a-f]{64}$/.test(s));return s.split('=');}));need(Object.keys(env).length===16);
  for(let i=0;i<16;i++){const k=rootConfig.capella.validator.keys[i],a=assignment.keys[i];need(k.keyEnv===a.runtimeKey.keyEnv&&k.index===q(a.publicKeyIndex)&&same(k.restart,a.runtimeKey.restart)&&a.publicKey===manifest.publicKeys[a.publicKeyIndex].publicKey&&env[k.keyEnv]&&k.journal===dataRoot+'/journal-'+a.publicKeyIndex&&k.recoveryWitness===witnessRoot+'/witness-'+a.publicKeyIndex);}
  exact(work.identities,['voluntaryExit','slashing']);for(const id of Object.values(work.identities)){exact(id,['owner','keyEnv','publicKey']);need(id.owner===fixedOwner&&identities.some(x=>same(x,{keyEnv:id.keyEnv,publicKey:id.publicKey})));}need(work.identities.voluntaryExit.publicKey!==work.identities.slashing.publicKey);
  exact(work.resources,['maximumWallSeconds','maximumHistoryBlocks','maximumArchiveBytes','maximumIndexBytes','maximumInventoryFiles','maximumInventoryBytes']);for(const v of Object.values(work.resources))need(typeof v==='string'&&/^0x[1-9a-f][0-9a-f]*$/.test(v));need(BigInt(work.resources.maximumWallSeconds)<=60n&&BigInt(work.resources.maximumHistoryBlocks)<=256n&&BigInt(work.resources.maximumInventoryFiles)<=40000n&&BigInt(work.resources.maximumInventoryBytes)<=1073741824n);
  const parent=await realpath(dirname(absolute(outputPath)));need(parent===dirname(outputPath));for(const p of [work.programDirectory,work.networkDirectory,work.publicDirectory])need(outputPath!==p&&!outputPath.startsWith(p+'/')&&!p.startsWith(outputPath+'/'));await mkdir(outputPath,{mode:0o700});output=outputPath;const os=await lstat(output);need(os.isDirectory()&&os.uid===501&&(os.mode&0o777)===0o700);controls=await mkdtemp(join(await realpath(tmpdir()),'nano-local-supervisor-'));need((await lstat(controls)).mode%512===0o700);
  const imageInfo=JSON.parse((await run(docker,['image','inspect',image],'image',10000)).toString())[0];need(imageInfo.Id===image&&imageInfo.Architecture==='arm64');
  for(const v of [work.bootstrapVolume,work.dataVolume,work.witnessVolume]){const info=JSON.parse((await run(docker,['volume','inspect',v],'volume-'+v,10000)).toString())[0];need(info.Labels?.['nano.local-supervisor']===label);}
  work.copyVolume=work.containerPrefix+'-core-copy';work.actionVolume=work.containerPrefix+'-actions';
  need(new Set([work.bootstrapVolume,work.dataVolume,work.witnessVolume,work.configVolume,work.controlVolume,work.copyVolume,work.actionVolume]).size===7);
  await initVolume('config',work.configVolume);await initVolume('known',work.controlVolume);await initVolume('copy',work.copyVolume);await initVolume('action',work.actionVolume);transportReady=true;
  await linuxWrite('/config',await Promise.all(Object.entries(work.publicFiles).map(async([name,a])=>({name,body:(await read(a.path,a)).bytes}))),'public-six');
  const config=await control('node-first.json',(await read(work.configuration.path)).bytes);await control('bootstrap-plan.json',(await read(work.plan.path)).bytes);await control('anchors.json',(await read(work.anchors.path)).bytes);await control('source-inventory.json',(await read(work.sourceInventory.path)).bytes);await control('copy.pl',Buffer.from(copyModule.linuxRotationCopy));
  stage='bootstrap';const original=await inventory('source-before');need(sha(original)===authority.bootstrapInventorySha256);
  await job('no-prior-core',['/bin/sh','-ec',`test ! -e '${dataRoot}/chain'; test ! -e '${witnessRoot}/chain-witness'`]);
  const bootstrap=JSON.parse((await job('bootstrap-audit',['/core/nano-capella-isolated-bootstrap-audit','/config/isolated-domain.json','/known/bootstrap-plan.json','/known/anchors.json','/known/source-inventory.json',fixedOwner,dataRoot,witnessRoot])).toString());need(bootstrap.passed===true&&bootstrap.primaryWitnessPairs===16&&bootstrap.coldPairReopens===32&&bootstrap.newSignaturesCreated===0);
  stage='first-runtime';const first=await start('first-runtime',config);stage='first-runtime-validator-http';const http=await validatorHttp(first);const stop1=await stop(first,'first-runtime');const sealed=await pair('first',config,stop1);need(first.ipc.consensusIdentity===sealed.capture.consensusIdentity);
  stage='physical-core-copy';const copied=JSON.parse((await job('copy-create',['perl','/known/copy.pl',dataRoot+'/chain',witnessRoot+'/chain-witness','/rotation-audit','create','-'],{copyRO:false},180000)).toString());copyModule.validateRotationCopySummary(copied,false);
  let action,requestPin,observed;
  try{
   stage='sealed-observe';const request={format:'nano-fresh-lifecycle-request-v1',owner:fixedOwner,manifestSha256:work.publicFiles['isolated-domain.json'].sha256,configurationSha256:config.sha256,stopSha256:sealed.externalStop.sha256,expectedHead:null,identities:work.identities,resources:work.resources,stage:'observe',action:null,syntheticActionAuthorized:false,intentionallyAuthorConflictingHeaders:false,crossOwnerEvidenceTransferAuthorized:false,evidence:null};
   const obsRequest=await control('observe-request.json',request);observed=JSON.parse((await job('observe',['/core/nano-capella-deposit-audit','--fresh-audit','/config/isolated-domain.json',config.path,sealed.externalStop.path,obsRequest.path,obsRequest.sha256,'/rotation-audit/data','/rotation-audit/witness',fixedOwner,'/action-output/observe'],{workRO:false},65000)).toString());need(observed.passed===true&&observed.completePrimaryWitnessClosuresVerified===true&&observed.externalCoreTokensMatched===true&&observed.newSignaturesCreated===0&&observed.envRead===false);
   const selected=observed.identities.find(x=>x.publicKey===work.identities.slashing.publicKey);need(selected?.publicKey===http.publicKey&&BigInt(selected.resolvedIndex)===BigInt(http.index));need(selected?.registered===true&&selected.withdrawalCredentials?.startsWith('0x01')&&selected.slashed===false&&BigInt(selected.activationEpoch)<=BigInt(selected.epoch)&&BigInt(selected.epoch)<BigInt(selected.exitEpoch));
   const actionRequest={...request,expectedHead:{root:observed.headRoot,stateRoot:observed.headStateRoot,slot:observed.headSlot},stage:'prepare_action',action:'proposer_slashing',syntheticActionAuthorized:true,intentionallyAuthorConflictingHeaders:true};requestPin=await control('action-request.json',actionRequest);
   stage='fresh-action';action=JSON.parse((await job('action',['/core/nano-capella-deposit-audit','--fresh-action','/config/isolated-domain.json',config.path,sealed.externalStop.path,requestPin.path,requestPin.sha256,'/rotation-audit/data','/rotation-audit/witness',fixedOwner,'/action-output/action'],{workRO:false,envFile:work.environment.path},65000)).toString());need(action.passed===true&&action.kind==='proposer_slashing'&&action.newSignaturesCreated===2&&action.submitted===false&&action.canonicalInclusionVerified===false&&action.headRoot===observed.headRoot);
  }finally{const verified=JSON.parse((await job('copy-verify',['perl','/known/copy.pl',dataRoot+'/chain',witnessRoot+'/chain-witness','/rotation-audit','verify',copied.copyProofSha256],{},180000)).toString());copyModule.validateRotationCopySummary(verified,true);need(verified.copyProofSha256===copied.copyProofSha256);}
  // The original runtime cores will now legitimately advance; their stopped-copy fence has completed.
  stage='independent-plan';const plan={format:'nano-fresh-lifecycle-submission-plan-v1',campaignId:label,owner:fixedOwner,manifest:{...work.publicFiles['isolated-domain.json'],path:'/config/isolated-domain.json'},action:{path:'/action-output/action/action.private.json',sha256:action.actionSha256,bytes:action.actionBytes},realP2p:{...work.programs['nano-p2p'],path:'/network-build/nano-p2p'},resultFile:'/action-output/accepted.private.json',maximumWallSeconds:60};const planPin=await control('submission-plan.json',plan);
  const restored=restoreModule.pinnedRestoreConfiguration(rootConfig,stop1,identities);restored.p2p_binary='/core/nano-capella-lifecycle-submit';const config2=await control('node-second.json',restored);await fence();
  stage='real-supervisor-admission';const second=await start('second-runtime',config2,planPin);need(second.ipc.consensusIdentity===sealed.capture.consensusIdentity);const stop2=await stop(second,'second-runtime');await pair('second',config2,stop2);
  stage='accepted-job';need(Number.isSafeInteger(second.accepted.receiptBytes)&&second.accepted.receiptBytes>0&&second.accepted.receiptBytes<=65536&&/^[0-9a-f]{64}$/.test(second.accepted.receiptSha256));
  const acceptedReader=String.raw`use strict;use warnings;use Fcntl qw(:DEFAULT :mode);use Digest::SHA ();use Cwd qw(abs_path);use IO::Handle ();local$SIG{ALRM}=sub{die "accepted receipt refused\n"};alarm 5;my($p,$expected,$size)=@ARGV;die "accepted receipt refused\n" unless @ARGV==3&&$p eq '/action-output/accepted.private.json'&&$expected=~/\A[0-9a-f]{64}\z/&&$size=~/\A[1-9][0-9]*\z/&&length($size)<=5&&$size<=65536&&abs_path($p) eq $p;sub row{my($p,$follow)=@_;open(my$f,'-|','stat',$follow?'-Lc':'-c','%d|%i|%f|%h|%u|%g|%s|%y|%z','--',$p)or die "accepted receipt refused\n";local$/;my$r=<$f>;die "accepted receipt refused\n" unless close($f)&&defined$r&&length($r)<1024;chomp$r;my@r=split(/\|/,$r,-1);die "accepted receipt refused\n" unless @r==9;return$r;}sysopen(my$f,$p,O_RDONLY|O_NOFOLLOW|O_NONBLOCK)or die "accepted receipt refused\n";my@s=stat$f;die "accepted receipt refused\n" unless @s&&S_ISREG($s[2])&&$s[3]==1&&$s[4]==501&&$s[5]==20&&($s[2]&07777)==0600&&$s[7]==$size;my$fd='/proc/'.$$.'/fd/'.fileno($f);my$before=row($fd,1);die "accepted receipt refused\n" unless $before eq row($p,0);my$body='';my$hash=Digest::SHA->new(256);while(1){my$n=sysread($f,my$b,4096);die "accepted receipt refused\n" unless defined$n;last unless$n;die "accepted receipt refused\n" if length($body)+$n>$size;$body.=$b;$hash->add($b);}die "accepted receipt refused\n" unless length($body)==$size&&$hash->hexdigest eq $expected&&$before eq row($fd,1)&&$before eq row($p,0)&&abs_path($p) eq $p;close($f)or die "accepted receipt refused\n";binmode STDOUT;print STDOUT $body or die "accepted receipt refused\n";STDOUT->flush or die "accepted receipt refused\n";alarm 0;`;
  const acceptedBytes=await job('accepted-read',['perl','-e',acceptedReader,'/action-output/accepted.private.json',second.accepted.receiptSha256,String(second.accepted.receiptBytes)],{},10000);const receipt=JSON.parse(acceptedBytes);exact(receipt,['format','campaignId','owner','kind','actionSha256','session','sequence','job']);exact(receipt.job,['sequence','payloadDigest','cancelRequested','state','decision']);need(receipt.format==='nano-fresh-lifecycle-accepted-job-v1'&&receipt.campaignId===label&&receipt.owner===fixedOwner&&receipt.kind==='proposer_slashing'&&receipt.actionSha256===action.actionSha256&&receipt.sequence===receipt.job.sequence&&receipt.job.payloadDigest===action.payloadDigest&&receipt.job.state==='completed'&&receipt.job.decision==='accepted'&&receipt.job.cancelRequested===false&&sha(acceptedBytes)===second.accepted.receiptSha256&&acceptedBytes.length===second.accepted.receiptBytes);
  stage='final-fences';need((await inventory('source-after')).equals(original));need(same((await readdir(work.publicDirectory)).sort(),expectedPublic.sort()));await linuxFence('final');await fence();
  result={format:'nano-local-supervisor-proposer-slashing-result-v1',passed:true,logicalFixtureOwner:fixedOwner,physicalExecution:'local Docker ARM64',linuxOwnedConfigAndControls:true,linuxKnownFileAuthorities:linuxAuthorities.length,linuxKnownFilesFinalFence:true,validatorHttpReads:2,validatorHttpPositive200:1,validatorHttpNegative404:1,validatorHttpFromRealNano:true,actualNanoStarts:2,healthyStops:2,bootstrapColdPairReopens:32,fullStoppedCaptures:4,signerPairsPerCapture:16,externalWitnessPinsMatched:true,coreCopyAndSourceFencesPassed:true,authenticatedStoppedHead:true,actionSignatures:2,actionRequestSha256:requestPin.sha256,actionSha256:action.actionSha256,realSupervisorRpcAccepted:true,realP2pExecutableVerified:true,realNanoParentVerified:true,ipcConsensusIdentityMatched:true,acceptedReceiptSha256:sha(acceptedBytes),canonicalInclusionVerified:false,economicPenaltyVerified:false,wholeLifecycleQualified:false,multiownerFinalityQualified:false,privateVpsAuditExecuted:false,productionQualified:false};
 }catch(e){error=e;}
 finally{
  try{if(docker&&work&&output)await cleanup();if(output){await fence();for(const[p,e]of produced){const r=await read(p,null,false);need(r.sha256===e.sha256&&r.bytes.length===e.bytes&&same(r.material.slice(0,6),e.identity));}}}catch(e){error??=e;}
  process.off('SIGINT',signal);process.off('SIGTERM',signal);
 }
 if(error){const refusal={format:'nano-local-supervisor-proposer-slashing-result-v1',passed:false,refusedStage:stage,detailsExported:false,productionQualified:false};if(output)await save('terminal-refusal.private.json',refusal);throw Object.assign(Error('Local supervisor qualification refused'),{publicResult:refusal});}
 result.ownedContainersAbsent=true;await save('terminal-public.json',result);return result;
}

if(process.argv[1]&&import.meta.url===pathToFileURL(resolve(process.argv[1])).href){
 try{need(process.argv.length===5);console.log(JSON.stringify(await runLocalSupervisor(process.argv[2],process.argv[3],process.argv[4])));}catch(e){console.log(JSON.stringify(e.publicResult??{format:'nano-local-supervisor-proposer-slashing-result-v1',passed:false,refusedStage:'setup',detailsExported:false,productionQualified:false}));process.exitCode=1;}
}
