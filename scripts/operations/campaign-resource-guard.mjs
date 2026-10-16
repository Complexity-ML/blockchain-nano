// External test resource policy. This module does not confer campaign authority.
import {spawn} from 'node:child_process';
import {open,lstat,realpath,mkdir,statfs} from 'node:fs/promises';
import {constants} from 'node:fs';
import {resolve,dirname,isAbsolute} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash,randomUUID} from 'node:crypto';
import {totalmem,freemem} from 'node:os';
import {performance} from 'node:perf_hooks';
import {setTimeout as sleep} from 'node:timers/promises';

const GiB=1024n**3n, MAX_U64=(1n<<64n)-1n;
const ROOT='/Users/boris/Dev/blockchain-nano';
const TARGET='ubuntu@145.239.199.166';
const IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';
const VOLUME='nano-local-supervisor-507081131d57-data';
const ENDPOINT='unix:///Users/boris/.docker/run/docker.sock';
const shaPattern=/^[0-9a-f]{64}$/;
export const policy=Object.freeze({intervalMs:10000,sampleTimeoutMs:5000,maximumRunMs:25200000,cleanupGraceMs:1800000,startDiskBytes:4n*GiB,floorDiskBytes:2n*GiB,startMemoryBytes:3n*GiB,floorMemoryBytes:GiB});
export class ResourceGuardError extends Error {constructor(code){super(code);this.name='ResourceGuardError';this.code=code;}}
const need=(v,code)=>{if(!v)throw new ResourceGuardError(code);};
const codeOf=e=>e instanceof ResourceGuardError?e.code:'guard-internal-refusal';
const exact=(v,keys,code)=>need(v&&typeof v==='object'&&!Array.isArray(v)&&Object.keys(v).sort().join('\0')===[...keys].sort().join('\0'),code);
const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
const uint=(v,code)=>{need(typeof v==='string'&&/^(0|[1-9][0-9]*)$/.test(v),code);const n=BigInt(v);need(n<=MAX_U64,code);return n;};
const inside=(p,root)=>p===root||p.startsWith(root+'/');
const overlaps=(a,b)=>inside(a,b)||inside(b,a);

export function parseVmStat(text,physicalBytes){
 const physical=uint(physicalBytes,'mac-total-memory');need(physical>0n&&typeof text==='string'&&text.length<=65536,'mac-vm-stat-shape');
 const lines=text.trim().split(/\r?\n/),page=/^Mach Virtual Memory Statistics: \(page size of ([1-9][0-9]*) bytes\)$/.exec(lines.shift()??'');need(page,'mac-page-size');
 const size=uint(page[1],'mac-page-size');need(size===4096n||size===16384n,'mac-page-size');const found=new Map();
 for(const line of lines){const m=/^Pages (free|inactive|speculative):\s+([0-9]+)\.$/.exec(line);if(!m)continue;need(!found.has(m[1]),'mac-duplicate-counter');found.set(m[1],uint(m[2],'mac-page-counter'));}
 need(found.size===3,'mac-missing-counter');const estimate=[...found.values()].reduce((a,b)=>a+b,0n)*size;need(estimate<=MAX_U64&&estimate<=physical,'mac-memory-overflow');
 return {memoryBytes:String(estimate),memoryMetric:'mac-free-inactive-speculative-estimate',pageSizeBytes:String(size),freePages:String(found.get('free')),inactivePages:String(found.get('inactive')),speculativePages:String(found.get('speculative')),physicalBytes};
}

export function parseLinuxCapacity(text,expectedRows){
 need(typeof text==='string'&&text.length<=65536&&Number.isSafeInteger(expectedRows)&&expectedRows>0,'linux-sample-shape');
 const lines=text.trim().split(/\r?\n/);need(lines.shift()==='NANO_DF_BEGIN','linux-df-marker');const end=lines.indexOf('NANO_DF_END');need(end>=2,'linux-df-end');
 const df=lines.splice(0,end);lines.shift();need(/^Filesystem\s+1024-blocks\s+Used\s+Available\s+Capacity\s+Mounted on$/.test(df.shift()??''),'linux-df-header');need(df.length===expectedRows,'linux-df-rows');
 const rows=df.map(line=>{const m=/^\S+\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)%\s+(\/\S*)$/.exec(line);need(m,'linux-df-row');const total=uint(m[1],'linux-df-value'),used=uint(m[2],'linux-df-value'),available=uint(m[3],'linux-df-value');need(used<=total&&available<=total&&uint(m[4],'linux-df-value')<=100n,'linux-df-range');return available*1024n;});
 need(lines.length===1,'linux-memory-rows');const mem=/^NANO_MEM_KIB:([0-9]+)$/.exec(lines[0]);need(mem,'linux-memory-row');const memory=uint(mem[1],'linux-memory-value')*1024n,disk=rows.reduce((a,b)=>a<b?a:b);need(memory<=MAX_U64&&disk<=MAX_U64,'linux-capacity-overflow');return {diskBytes:String(disk),memoryBytes:String(memory),memoryMetric:'linux-MemAvailable',diskMetric:'minimum-df-available-kib'};
}

export function evaluateResources(sample,phase,nowMs){
 need(phase==='startup'||phase==='running','resource-phase');need(sample&&Number.isFinite(sample.startedMs)&&Number.isFinite(sample.finishedMs)&&sample.startedMs<=sample.finishedMs&&sample.finishedMs<=nowMs&&nowMs-sample.startedMs<=policy.sampleTimeoutMs,'sample-stale-or-invalid');
 exact(sample.surfaces,['macHost','macDockerVm','vpsHost'],'resource-surfaces');const diskFloor=phase==='startup'?policy.startDiskBytes:policy.floorDiskBytes,memoryFloor=phase==='startup'?policy.startMemoryBytes:policy.floorMemoryBytes;
 for(const name of ['macHost','macDockerVm','vpsHost']){const row=sample.surfaces[name];need(row&&row.memoryMetric===(name==='macHost'?'mac-free-inactive-speculative-estimate':'linux-MemAvailable'),'memory-metric');need(uint(row.diskBytes,'disk-unknown')>=diskFloor,'disk-below-'+phase+'-'+name);need(uint(row.memoryBytes,'memory-unknown')>=memoryFloor,'memory-below-'+phase+'-'+name);}
 return true;
}

// The handle must be returned by this invocation's spawn. No PID lookup occurs.
export function ownedStopLatch(child){
 const pid=child?.pid;need(Number.isSafeInteger(pid)&&pid>1&&pid!==28722&&typeof child.kill==='function','owned-child-identity');let sent=false;
 return ()=>{need(child.pid===pid,'owned-child-identity-changed');if(sent||child.closed)return false;sent=true;need(child.kill('SIGINT')===true,'owned-child-sigint-refused');return true;};
}

// Dependency injection keeps policy/signal tests free of processes, clocks and IO.
export async function supervise({sample,launch,clock,sleep:wait,record=async()=>{},stopRequested=()=>false,probeOnly=false}){
 let child=null,stop=null,first=null,signalSent=false,samples=0,started=null;
 const checked=async phase=>{const s=await sample();evaluateResources(s,phase,clock());await record({kind:'resource-sample',phase,...s});++samples;};
 try{
  for(let i=0;i<3;++i){need(!stopRequested(),'wrapper-interrupted');await checked('startup');if(i<2)await wait(policy.intervalMs);}
  need(!stopRequested(),'wrapper-interrupted');if(probeOnly)return {passed:true,probeOnly:true,launcherStarted:false,samples,sigintSent:false,launcherJoined:false};
  child=await launch();stop=ownedStopLatch(child);started=clock();
  while(!child.closed){need(!stopRequested(),'wrapper-interrupted');need(clock()-started<policy.maximumRunMs,'campaign-wall-timeout');await wait(Math.min(policy.intervalMs,policy.maximumRunMs-(clock()-started)));if(child.closed)break;need(!stopRequested(),'wrapper-interrupted');need(clock()-started<policy.maximumRunMs,'campaign-wall-timeout');await checked('running');}
  need(!stopRequested(),'wrapper-interrupted');need(child.exitCode===0&&child.signalCode===null,'launcher-nonzero');
 }catch(e){first=codeOf(e);}
 if(child&&first&&!child.closed){try{signalSent=stop();}catch(e){first??=codeOf(e);}const until=clock()+policy.cleanupGraceMs;while(!child.closed&&clock()<until)await wait(Math.min(1000,until-clock()));}
 return {passed:first===null,reason:first,probeOnly,launcherStarted:child!==null,samples,sigintSent:signalSent,launcherJoined:child?.closed??false,launcherExitCode:child?.exitCode??null,launcherSignal:child?.signalCode??null,launcherUnjoined:child!==null&&!child.closed};
}

const row=s=>['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'].map(k=>String(s[k]));
const same=(a,b)=>a.length===9&&b.length===9&&a.every((v,i)=>v===b[i]);
function artifact(a){exact(a,['path','sha256','bytes','material'],'artifact-fields');need(typeof a.path==='string'&&isAbsolute(a.path)&&resolve(a.path)===a.path&&!/[\0\r\n]/.test(a.path)&&shaPattern.test(a.sha256)&&Number.isSafeInteger(a.bytes)&&a.bytes>0&&a.bytes<=256*1024*1024,'artifact-shape');need(Array.isArray(a.material)&&a.material.length===9,'artifact-nine');for(const v of a.material)uint(v,'artifact-nine');need(a.material[6]===String(a.bytes),'artifact-size-nine');return a;}
async function readPinned(a,wantBytes=false){
 const expected=a.material;need(await realpath(a.path)===a.path,'artifact-physical-path');const fd=await open(a.path,constants.O_RDONLY|constants.O_NOFOLLOW);try{const before=await fd.stat({bigint:true});need(before.isFile()&&before.nlink>=1n&&!(before.mode&0o022n),'artifact-file-mode');const original=row(before);need((!expected||same(original,expected))&&same(original,row(await lstat(a.path,{bigint:true})))&&before.size===BigInt(a.bytes),'artifact-first-nine');const h=createHash('sha256'),chunks=[];let count=0;while(true){const b=Buffer.alloc(65536),{bytesRead:n}=await fd.read(b,0,b.length,null);if(!n)break;count+=n;need(count<=a.bytes,'artifact-read-limit');h.update(b.subarray(0,n));if(wantBytes)chunks.push(b.subarray(0,n));}need(count===a.bytes&&h.digest('hex')===a.sha256&&same(original,row(await fd.stat({bigint:true})))&&same(original,row(await lstat(a.path,{bigint:true})))&&await realpath(a.path)===a.path,'artifact-final-nine-or-sha');return {material:original,bytes:wantBytes?Buffer.concat(chunks):null};}finally{await fd.close();}
}
const cleanEnv=()=>({PATH:'/usr/bin:/bin:/usr/sbin:/sbin',LANG:'C',LC_ALL:'C',HOME:'/Users/boris'});

// Only fixed read-only sample commands use process groups. The launcher does not.
async function command(tool,args,{deadline,cwd}){
 deadline=Math.min(deadline,performance.now()+policy.sampleTimeoutMs);await readPinned(tool);const remaining=deadline-performance.now();need(remaining>600,'sample-deadline');let child,closed=false,result,value,first=null,outputBytes=0;const out=[],err=[];
 const done=new Promise(resolveDone=>{child=spawn(tool.path,args,{cwd,env:cleanEnv(),shell:false,detached:true,stdio:['ignore','pipe','pipe']});child.once('error',()=>{first??='sample-command-spawn';});child.once('close',(code,signal)=>{closed=true;result={code,signal};resolveDone();});for(const [stream,parts] of [[child.stdout,out],[child.stderr,err]])stream.on('data',b=>{outputBytes+=b.length;if(outputBytes>65536)first??='sample-command-output-limit';else parts.push(b);});});
 const waitUntil=async until=>{while(!closed&&performance.now()<until)await sleep(Math.min(10,Math.max(1,until-performance.now())));};
 try{await waitUntil(deadline-500);if(!closed)first??='sample-command-timeout';if(result?.code!==0||result?.signal!==null)first??='sample-command-nonzero';if(!first)value=Buffer.concat(out).toString('utf8');}catch(e){first??=codeOf(e);}
 finally{if(!closed){if(Number.isSafeInteger(child?.pid)&&child.pid>1&&child.pid!==28722){for(const [signal,budget] of [['SIGTERM',200],['SIGKILL',200]]){if(closed)break;try{process.kill(-child.pid,signal);}catch(e){if(e.code!=='ESRCH')first??='sample-command-group-cleanup';}await waitUntil(Math.min(deadline,performance.now()+budget));}}if(!closed){child.stdout.destroy();child.stderr.destroy();child.unref();}}try{await readPinned(tool);}catch(e){first??=codeOf(e);}void done;}
 if(first)throw new ResourceGuardError(first);return value;
}

async function main(args){
 const mode=args.at(-1);need(mode==='--probe-only'||mode==='--launch-reviewed-campaign','explicit-mode-required');const probeOnly=mode==='--probe-only';args=args.slice(0,-1);need(args.length===6&&args[0]==='--config'&&args[2]==='--config-sha256'&&args[4]==='--config-bytes'&&shaPattern.test(args[3])&&/^[1-9][0-9]*$/.test(args[5]),'usage');
 need(process.platform==='darwin'&&process.getuid()===501&&process.getgid()===20,'mac-owner-platform');const cp={path:args[1],sha256:args[3],bytes:Number(args[5])};need(isAbsolute(cp.path)&&resolve(cp.path)===cp.path&&cp.bytes<=1024*1024,'config-size-path');const firstConfig=await readPinned(cp,true);cp.material=firstConfig.material;const c=JSON.parse(firstConfig.bytes.toString('utf8'));
 exact(c,['format','tools','launcher','spec','sourceBaseRevision','outputDirectory','campaignOutputDirectory','dockerEndpoint','proofVolume'],'config-fields');need(c.format==='nano-external-campaign-resource-guard-v1'&&/^[0-9a-f]{40}$/.test(c.sourceBaseRevision)&&c.dockerEndpoint===ENDPOINT,'config-authority');exact(c.tools,['node','docker','ssh','vmStat'],'tools');for(const a of [...Object.values(c.tools),c.launcher,c.spec])artifact(a);
 need(c.launcher.path===ROOT+'/tests/multihost/run.mjs'&&c.spec.bytes<=1024*1024&&c.tools.node.path===await realpath(process.execPath)&&c.tools.ssh.path==='/usr/bin/ssh'&&c.tools.vmStat.path==='/usr/bin/vm_stat','fixed-program-paths');exact(c.proofVolume,['name','driver','labels'],'proof-volume');need(c.proofVolume.name===VOLUME&&c.proofVolume.driver==='local'&&c.proofVolume.labels&&typeof c.proofVolume.labels==='object'&&!Array.isArray(c.proofVolume.labels)&&Object.entries(c.proofVolume.labels).every(([k,v])=>typeof k==='string'&&typeof v==='string'),'proof-volume-fields');
 const pins=[cp,...Object.values(c.tools),c.launcher,c.spec],fence=async()=>{for(const p of pins)await readPinned(p);};await fence();const spec=JSON.parse((await readPinned(c.spec,true)).bytes.toString('utf8'));need(spec.format==='nano-fresh-capella-three-owner-campaign-v1'&&spec.sourceBaseRevision===c.sourceBaseRevision&&spec.owners?.['vps-docker']?.host===TARGET&&spec.lifecycle?.deadlines?.maximumRunMs===policy.maximumRunMs,'fixed-campaign-binding');
 const codeRoots=Object.values(spec.owners??{}).flatMap(v=>[v.coreBuildRoot,v.networkBuildRoot]).filter(p=>typeof p==='string'&&isAbsolute(p));
 const outputs=[c.outputDirectory,c.campaignOutputDirectory];for(const p of outputs){need(typeof p==='string'&&isAbsolute(p)&&resolve(p)===p&&!/[\0\r\n]/.test(p)&&await realpath(dirname(p))===dirname(p),'new-output-path');const parent=await lstat(dirname(p),{bigint:true});need(parent.isDirectory()&&parent.uid===501n&&!(parent.mode&0o022n),'output-parent-owner');need(!overlaps(p,ROOT)&&codeRoots.every(root=>!overlaps(p,root))&&pins.every(a=>!overlaps(p,a.path))&&!overlaps(p,spec.fixture?.seedFile??ROOT)&&!overlaps(p,spec.fixture?.ledgerDirectory??ROOT),'output-protected-overlap');try{await lstat(p);throw new ResourceGuardError('output-already-exists');}catch(e){if(e.code!=='ENOENT')throw e;}}need(!overlaps(...outputs),'output-campaign-overlap');
 await mkdir(c.outputDirectory,{mode:0o700});const privateFile=async(name,body)=>{need(/^[a-z0-9][a-z0-9.-]*$/.test(name),'private-leaf');const h=await open(resolve(c.outputDirectory,name),constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);try{await h.writeFile(body);await h.sync();}finally{await h.close();}};const events=await open(resolve(c.outputDirectory,'resources.public.jsonl'),constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600),logs=[];await mkdir(resolve(c.outputDirectory,'docker-config'),{mode:0o700});
 const uuid=randomUUID().replaceAll('-',''),name='nano-resource-guard-'+uuid,label='nano.external.resource-guard';let id=null,creationAttempted=false,first=null,summary=null,launcherChild=null,interrupted=false;const onSignal=()=>{interrupted=true;};for(const s of ['SIGINT','SIGTERM','SIGHUP'])process.on(s,onSignal);
 const docker=async(a,deadline)=>command(c.tools.docker,['-H',ENDPOINT,'--config',resolve(c.outputDirectory,'docker-config'),...a],{deadline,cwd:c.outputDirectory});
 const json=raw=>{try{return JSON.parse(raw);}catch{throw new ResourceGuardError('sample-json');}};
 const absent=async(deadline)=>need((await docker(['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],deadline)).trim()==='','probe-container-preexists');
 const volume=async(deadline)=>{const v=json(await docker(['volume','inspect',VOLUME,'--format','{{json .}}'],deadline));need(v.Name===VOLUME&&v.Driver==='local'&&canonical(v.Labels??{})===canonical(c.proofVolume.labels),'proof-volume-identity');return v;};
 const owned=async(deadline)=>{const v=json(await docker(['inspect',id,'--format','{{json .}}'],deadline));need(v.Id===id&&v.Name==='/'+name&&v.Image===IMAGE&&v.Config.Labels?.[label]===uuid,'probe-container-identity');need(v.Mounts?.length===1&&v.Mounts[0].Type==='volume'&&v.Mounts[0].Name===VOLUME&&v.Mounts[0].Destination==='/probe'&&v.Mounts[0].RW===false,'probe-volume-readonly');return v;};
 const linuxCommand=paths=>String.raw`printf '%s\n' NANO_DF_BEGIN; /usr/bin/df -Pk ${paths}; printf '%s\n' NANO_DF_END; /usr/bin/awk '$1 == "MemAvailable:" {print "NANO_MEM_KIB:" $2}' /proc/meminfo`;
 const vmCommand=linuxCommand('/probe'),remoteCommand=String.raw`set -eu; printf '%s\n' NANO_DF_BEGIN; /usr/bin/sudo -n /usr/bin/df -Pk / /home/ubuntu /var/lib/docker; printf '%s\n' NANO_DF_END; /usr/bin/awk '$1 == "MemAvailable:" {print "NANO_MEM_KIB:" $2}' /proc/meminfo`;
 try{
  const until=performance.now()+30000;const image=json(await docker(['image','inspect',IMAGE,'--format','{{json .}}'],until));need(image.Id===IMAGE&&image.Os==='linux'&&image.Architecture==='arm64','probe-image');const before=await volume(until);await absent(until);creationAttempted=true;
  id=(await docker(['create','--pull','never','--name',name,'--label',label+'='+uuid,'--read-only','--network','none','--cap-drop','ALL','--security-opt','no-new-privileges:true','--user','501:20','--pids-limit','16','--memory','64m','--cpus','0.1','--mount','type=volume,src='+VOLUME+',dst=/probe,readonly','--entrypoint','/bin/sh',IMAGE,'-c','exec /bin/sleep 28800'],until)).trim();need(/^[0-9a-f]{64}$/.test(id),'probe-container-id');need(canonical(await volume(until))===canonical(before),'proof-volume-changed');await owned(until);await docker(['start',id],until);
  const runSample=async()=>{const startedMs=performance.now(),deadline=startedMs+policy.sampleTimeoutMs;const observed=await Promise.allSettled([
   (async()=>{const [disk,text]=await Promise.all([statfs(dirname(c.campaignOutputDirectory),{bigint:true}),command(c.tools.vmStat,[],{deadline,cwd:c.outputDirectory})]);const physical=totalmem(),free=freemem();need(Number.isSafeInteger(physical)&&Number.isSafeInteger(free)&&free>=0&&free<=physical&&disk.bavail>=0n&&disk.bsize>0n,'mac-capacity');return {diskBytes:String(disk.bavail*disk.bsize),diskMetric:'mac-statfs-bavail',...parseVmStat(text,String(physical)),osFreeBytes:String(free)};})(),
   (async()=>{await volume(deadline);await owned(deadline);return parseLinuxCapacity(await docker(['exec',id,'/bin/sh','-ec',vmCommand],deadline),1);})(),
   (async()=>parseLinuxCapacity(await command(c.tools.ssh,['-o','BatchMode=yes','-o','ConnectTimeout=2','-o','ConnectionAttempts=1','-o','StrictHostKeyChecking=yes','-o','ServerAliveInterval=2','-o','ServerAliveCountMax=1',TARGET,remoteCommand],{deadline,cwd:c.outputDirectory}),3))()
  ]);const failed=observed.find(v=>v.status==='rejected');if(failed)throw failed.reason;const [mac,vm,vps]=observed.map(v=>v.value);return {startedMs,finishedMs:performance.now(),surfaces:{macHost:mac,macDockerVm:vm,vpsHost:vps}};};
  const sampler=async()=>{let timer;try{return await Promise.race([runSample(),new Promise((_,reject)=>{timer=setTimeout(()=>reject(new ResourceGuardError('resource-sample-timeout')),policy.sampleTimeoutMs);})]);}finally{clearTimeout(timer);}};
  const launch=async()=>{await fence();need(!probeOnly&&!interrupted,'launcher-guard');const child=spawn(c.tools.node.path,[c.launcher.path,TARGET,c.sourceBaseRevision,c.campaignOutputDirectory,'--fresh-campaign',c.spec.path,'--fresh-campaign-sha',c.spec.sha256],{cwd:ROOT,env:process.env,shell:false,detached:false,stdio:['ignore',logs[0].fd,logs[1].fd]});launcherChild=child;const handle={pid:child.pid,closed:false,exitCode:null,signalCode:null,kill:s=>child.kill(s)};child.once('error',()=>{handle.exitCode=-1;});child.once('close',(exitCode,signalCode)=>{handle.closed=true;handle.exitCode=exitCode;handle.signalCode=signalCode;});return handle;};
  if(!probeOnly)for(const leaf of ['launcher-stdout.private.log','launcher-stderr.private.log'])logs.push(await open(resolve(c.outputDirectory,leaf),constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600));
  await fence();summary=await supervise({sample:sampler,launch,clock:()=>performance.now(),sleep,stopRequested:()=>interrupted,probeOnly,record:async event=>{await events.writeFile(JSON.stringify(event)+'\n');await events.sync();}});if(!summary.passed)first=summary.reason;
 }catch(e){first??=codeOf(e);}
 finally{
  // Cleanup is independent of launcher/source drift: only original Docker and
  // the UUID/name/image of our newly attempted sampler container authorize rm.
  const until=performance.now()+15000;try{if(creationAttempted){if(!/^[0-9a-f]{64}$/.test(id??'')){const found=(await docker(['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--filter','label='+label+'='+uuid,'--format','{{.ID}}'],until)).trim();need(found===''||/^[0-9a-f]{64}$/.test(found),'probe-cleanup-discovery');id=found||null;}if(id){await owned(until);await docker(['rm','-f',id],until);}await absent(until);}}catch(e){first??=codeOf(e);}
  try{await fence();}catch(e){first??=codeOf(e);}for(const h of logs){try{await h.sync();await h.close();}catch{first??='private-log-close';}}try{await events.sync();await events.close();}catch{first??='resource-log-close';}for(const s of ['SIGINT','SIGTERM','SIGHUP'])process.off(s,onSignal);if(summary?.launcherUnjoined)launcherChild?.unref();
 }
 const result={format:'nano-external-campaign-resource-guard-result-v1',passed:first===null&&summary?.passed===true,reason:first,policy:'test-resource-bounds-not-protocol',probeOnly,sourceBaseRevision:c.sourceBaseRevision,...summary,productionQualified:false};result.passed=first===null&&summary?.passed===true;result.reason=first;await privateFile('result-public.json',JSON.stringify(result)+'\n');process.stdout.write(JSON.stringify(result)+'\n');if(!result.passed)process.exitCode=1;
}

if(process.argv[1]&&fileURLToPath(import.meta.url)===resolve(process.argv[1]))main(process.argv.slice(2)).catch(e=>{process.stderr.write(JSON.stringify({passed:false,reason:codeOf(e),productionQualified:false})+'\n');process.exitCode=1;});
