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
export async function supervise({sample,launch,clock,sleep:wait,record=async()=>{},stopRequested=()=>false,probeOnly=false,c8=null,startedAt=null,closeoutStarted=null}){
 if(c8!==null)return superviseC8({sample,launch,clock,wait,record,stopRequested,probeOnly,c8,startedAt,closeoutStarted});
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

export function c8BudgetParameters(value){
 exact(value,['maximumRunMs','closeoutReserveMs'],'c8-budget-fields');
 need(Number.isSafeInteger(value.maximumRunMs)&&value.maximumRunMs>0&&value.maximumRunMs<=2147483647&&Number.isSafeInteger(value.closeoutReserveMs)&&value.closeoutReserveMs>0&&value.closeoutReserveMs<value.maximumRunMs,'c8-budget-range');
 return Object.freeze({...value});
}
// Typed metadata only. The caller must independently pin/read all three bodies.
export function c8GuardContract(c,spec,bindings){
 exact(c,['format','tools','launcher','spec','sourceBaseRevision','outputDirectory','campaignOutputDirectory','dockerEndpoint','proofVolume','maximumRunMs','closeoutReserveMs','softwareBindingsArtifact'],'c8-config-fields');
 const budget=c8BudgetParameters({maximumRunMs:c.maximumRunMs,closeoutReserveMs:c.closeoutReserveMs});
 need(c.format==='nano-external-campaign-resource-guard-v2'&&/^[0-9a-f]{40}$/.test(c.sourceBaseRevision)&&spec.format==='nano-fresh-capella-three-owner-campaign-c8-v2'&&spec.sourceBaseRevision===c.sourceBaseRevision&&spec.lifecycle?.deadlines?.maximumRunMs===budget.maximumRunMs,'c8-spec-budget-binding');
 exact(bindings,['format','sourceBaseRevision','overlayManifest','owners'],'c8-software-bindings');
 need(bindings.format==='nano-c8-selected-software-bindings-v1'&&bindings.sourceBaseRevision===c.sourceBaseRevision&&canonical(bindings.overlayManifest)===canonical(spec.overlayManifest)&&canonical(bindings.owners)===canonical(spec.owners),'c8-exact-software-binding');
 const v=c.proofVolume;exact(v,['name','driver','labels'],'c8-proof-volume');
 need(typeof v.name==='string'&&/^nano-[a-z0-9][a-z0-9_.-]{0,122}$/.test(v.name)&&v.driver==='local'&&v.labels&&typeof v.labels==='object'&&!Array.isArray(v.labels)&&Object.keys(v.labels).length>0&&Object.entries(v.labels).every(([k,z])=>k.length>0&&k.length<=128&&typeof z==='string'&&z.length<=1024)&&Object.keys(v.labels).some(k=>k.startsWith('nano.')),'c8-owned-proof-volume');
 return Object.freeze({...budget,proofVolume:Object.freeze({...v,labels:Object.freeze({...v.labels})}),samplerSeconds:Math.ceil(budget.maximumRunMs/1000),launcherArgs:Object.freeze([c.launcher.path,TARGET,c.sourceBaseRevision,c.campaignOutputDirectory,'--fresh-campaign',c.spec.path,'--fresh-campaign-sha',c.spec.sha256,'--closeout-reserve-ms',String(budget.closeoutReserveMs)])});
}
export const c8ProbeInspectionTemplate='{"Id":{{json .Id}},"Name":{{json .Name}},"Image":{{json .Image}},"Config":{"Labels":{{json .Config.Labels}}},"HostConfig":{"Mounts":{{json .HostConfig.Mounts}},"RestartPolicy":{{json .HostConfig.RestartPolicy}}},"Mounts":{{json .Mounts}}}';
export const c8ProbeImageTemplate='{"Id":{{json .Id}},"Os":{{json .Os}},"Architecture":{{json .Architecture}},"Volumes":{{json (index .Config "Volumes")}}}';
export function validateC8ProbeImage(v){
 exact(v,['Id','Os','Architecture','Volumes'],'probe-image-projection');need(v.Id===IMAGE&&v.Os==='linux'&&v.Architecture==='arm64','probe-image');
 need(v.Volumes===null||v.Volumes&&typeof v.Volumes==='object'&&!Array.isArray(v.Volumes)&&Object.keys(v.Volumes).length===0,'probe-image-anonymous-volumes');return v;
}
export function validateC8ProbeIdentity(v,{id,name,uuid}){
 exact(v,['Id','Name','Image','Config','HostConfig','Mounts'],'probe-projection');exact(v.Config,['Labels'],'probe-config-projection');
 need(v?.Id===id&&/^[0-9a-f]{64}$/.test(id)&&v.Name==='/'+name&&v.Image===IMAGE&&v.Config?.Labels?.['nano.external.resource-guard']===uuid,'probe-container-identity');
 return v;
}
export function validateC8OwnedProbe(v,binding){
 validateC8ProbeIdentity(v,binding);exact(v.HostConfig,['Mounts','RestartPolicy'],'probe-host-projection');const {volumeName}=binding;
 const host=v.HostConfig?.Mounts,mounts=v.Mounts;
 need(Array.isArray(host)&&host.length===1&&host[0].Type==='volume'&&host[0].Source===volumeName&&host[0].Target==='/probe'&&host[0].ReadOnly===true&&host[0].VolumeOptions?.NoCopy===true,'probe-volume-nocopy');
 need(Array.isArray(mounts)&&mounts.length===1&&mounts[0].Type==='volume'&&mounts[0].Name===volumeName&&mounts[0].Destination==='/probe'&&mounts[0].RW===false,'probe-volume-readonly');
 need(v.HostConfig.RestartPolicy?.Name==='no'&&v.HostConfig.RestartPolicy.MaximumRetryCount===0,'probe-no-restart');return v;
}
export function validateC8CloseoutReceipt(bytes,expected){
 need(Buffer.isBuffer(bytes)&&bytes.length>0&&bytes.length<=4096,'c8-closeout-receipt-bytes');
 let value;try{value=JSON.parse(bytes.toString('utf8'));}catch{throw new ResourceGuardError('c8-closeout-receipt-json');}
 exact(value,['format','phase','controllerPid','sourceBaseRevision','specSha256','maximumRunMs','closeoutReserveMs','productionQualified'],'c8-closeout-receipt-fields');
 need(bytes.equals(Buffer.from(JSON.stringify(value,null,2)+'\n')),'c8-closeout-receipt-canonical');
 need(value.format==='nano-fresh-c8-closeout-started-v1'&&value.phase==='closeout'&&value.controllerPid===expected.pid&&value.sourceBaseRevision===expected.sourceBaseRevision&&value.specSha256===expected.specSha256&&value.maximumRunMs===expected.maximumRunMs&&value.closeoutReserveMs===expected.closeoutReserveMs&&value.productionQualified===false,'c8-closeout-receipt-binding');return true;
}
export async function readC8CloseoutReceipt(output,expected){
 const file=resolve(output,'campaign-closeout-started.json');let fd,primary;
 try{
  need(await realpath(output)===output,'c8-closeout-parent');const parent=await lstat(output,{bigint:true});need(parent.isDirectory()&&parent.uid===BigInt(process.getuid())&&(parent.mode&0o7777n)===0o700n,'c8-closeout-parent');
  fd=await open(file,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);const s=await fd.stat({bigint:true}),m=row(s);
  need(s.isFile()&&s.nlink===1n&&s.uid===BigInt(process.getuid())&&(s.mode&0o7777n)===0o600n&&s.size>0n&&s.size<=4096n,'c8-closeout-file');
  const bytes=Buffer.alloc(Number(s.size));let at=0;while(at<bytes.length){const got=await fd.read(bytes,at,bytes.length-at,at);need(got.bytesRead>0,'c8-closeout-short-read');at+=got.bytesRead;}
  const extra=Buffer.alloc(1);need((await fd.read(extra,0,1,at)).bytesRead===0&&same(m,row(await fd.stat({bigint:true})))&&same(m,row(await lstat(file,{bigint:true})))&&await realpath(file)===file,'c8-closeout-full-nine');
  return validateC8CloseoutReceipt(bytes,expected);
 }catch(e){if(e?.code==='ENOENT')return false;primary=new ResourceGuardError('c8-closeout-receipt-refused');throw primary;}finally{if(fd)try{await fd.close();}catch{if(!primary)throw new ResourceGuardError('c8-closeout-close-refused');}}
}
// Every v2 file/stdout verdict stays provisional. Root must observe the real
// outer exit/time and independently check actors before adopting it.
export async function publishC8GuardResult({result,write,clock,interrupted,overallEnd}){
 let first=result.reason??null,published=false;
 const check=()=>{if(interrupted())first??='wrapper-interrupted';if(clock()>=overallEnd)first??='campaign-wall-timeout';};
 check();const provisional={...result,passed:false,reason:first,terminalEvidenceReady:first===null&&result.passed===true,terminalVerdictProvisional:true,effectiveTerminalExitRequired:true};
 try{await write(provisional);published=true;}catch{first??='c8-result-publication-refused';}
 check();const ready=first===null&&result.passed===true&&published;
 return {report:{...provisional,reason:first,terminalEvidenceReady:ready,resultPublicationCompleted:published,preterminalFileMayExist:true},exitCode:ready?0:1};
}
async function superviseC8({sample,launch,clock,wait,record,stopRequested,probeOnly,c8,startedAt,closeoutStarted}){
 const b=c8BudgetParameters(c8);need(Number.isFinite(startedAt)&&startedAt>=0&&typeof closeoutStarted==='function','c8-supervisor-origin');
 const workEnd=startedAt+b.maximumRunMs-b.closeoutReserveMs,overallEnd=startedAt+b.maximumRunMs;
 let child=null,stop=null,first=null,signalSent=false,samples=0,phaseObserved=false,workChecked=false,last=startedAt;
 const now=()=>{const t=clock();need(Number.isFinite(t)&&t>=last,'c8-monotone-clock');last=t;return t;};
 const overall=()=>need(now()<overallEnd,'campaign-wall-timeout'),work=()=>{overall();need(now()<workEnd,'campaign-work-deadline');};
 const checked=async phase=>{const s=await sample();evaluateResources(s,phase,now());await record({kind:'resource-sample',phase,...s});++samples;};
 try{
  for(let i=0;i<3;++i){work();need(!stopRequested(),'wrapper-interrupted');await checked('startup');work();if(i<2)await wait(Math.min(policy.intervalMs,Math.max(0,workEnd-now())));}
  work();need(!stopRequested(),'wrapper-interrupted');if(probeOnly)return {passed:true,probeOnly:true,launcherStarted:false,samples,sigintSent:false,launcherJoined:false};
  child=await launch();stop=ownedStopLatch(child);overall();
  for(;;){
   overall();need(!stopRequested(),'wrapper-interrupted');
   if(!workChecked&&now()>=workEnd){workChecked=true;phaseObserved=(await closeoutStarted(child))===true;overall();if(!phaseObserved)throw new ResourceGuardError('campaign-work-deadline');}
   if(child.closed)break;
   await wait(Math.max(0,Math.min(policy.intervalMs,(workChecked?overallEnd:workEnd)-now())));overall();
   // Recheck before accepting a child which closed while awaiting the timer.
   if(!workChecked&&now()>=workEnd){workChecked=true;phaseObserved=(await closeoutStarted(child))===true;overall();if(!phaseObserved)throw new ResourceGuardError('campaign-work-deadline');}
   if(child.closed)break;await checked('running');overall();
  }
  overall();need(child.exitCode===0&&child.signalCode===null,'launcher-nonzero');
 }catch(e){first=codeOf(e);}
 if(child&&first&&!child.closed){try{signalSent=stop();}catch(e){first??=codeOf(e);}const until=workChecked?overallEnd:Math.min(overallEnd,now()+policy.cleanupGraceMs);while(!child.closed&&now()<until)await wait(Math.min(1000,until-now()));}
 // No v1 cleanupGrace is added after overallEnd; actual outer terminal remains
 // independently required and descendants are not claimed dead by this handle.
 if(now()>=overallEnd)first??='campaign-wall-timeout';
 return {passed:first===null,reason:first,probeOnly,launcherStarted:child!==null,samples,sigintSent:signalSent,launcherJoined:child?.closed??false,launcherExitCode:child?.exitCode??null,launcherSignal:child?.signalCode??null,launcherUnjoined:child!==null&&!child.closed,maximumRunMs:b.maximumRunMs,closeoutReserveMs:b.closeoutReserveMs,closeoutPhaseObserved:phaseObserved,elapsedMs:Math.ceil(now()-startedAt),durationMeasuredBeforePublication:true,outerTerminalIndependentlyRequired:true};
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
 const wrapperStartedAt=performance.now();
 const mode=args.at(-1);need(mode==='--probe-only'||mode==='--launch-reviewed-campaign','explicit-mode-required');const probeOnly=mode==='--probe-only';args=args.slice(0,-1);need(args.length===6&&args[0]==='--config'&&args[2]==='--config-sha256'&&args[4]==='--config-bytes'&&shaPattern.test(args[3])&&/^[1-9][0-9]*$/.test(args[5]),'usage');
 need(process.platform==='darwin'&&process.getuid()===501&&process.getgid()===20,'mac-owner-platform');const cp={path:args[1],sha256:args[3],bytes:Number(args[5])};need(isAbsolute(cp.path)&&resolve(cp.path)===cp.path&&cp.bytes<=1024*1024,'config-size-path');const firstConfig=await readPinned(cp,true);cp.material=firstConfig.material;const c=JSON.parse(firstConfig.bytes.toString('utf8'));
 const isC8=c.format==='nano-external-campaign-resource-guard-v2';
 exact(c,['format','tools','launcher','spec','sourceBaseRevision','outputDirectory','campaignOutputDirectory','dockerEndpoint','proofVolume',...(isC8?['maximumRunMs','closeoutReserveMs','softwareBindingsArtifact']:[])],'config-fields');need((isC8||c.format==='nano-external-campaign-resource-guard-v1')&&/^[0-9a-f]{40}$/.test(c.sourceBaseRevision)&&c.dockerEndpoint===ENDPOINT,'config-authority');exact(c.tools,['node','docker','ssh','vmStat'],'tools');for(const a of [...Object.values(c.tools),c.launcher,c.spec,...(isC8?[c.softwareBindingsArtifact]:[])])artifact(a);
 need(c.launcher.path===ROOT+'/tests/multihost/run.mjs'&&c.spec.bytes<=1024*1024&&(!isC8||c.softwareBindingsArtifact.bytes<=1024*1024)&&c.tools.node.path===await realpath(process.execPath)&&c.tools.ssh.path==='/usr/bin/ssh'&&c.tools.vmStat.path==='/usr/bin/vm_stat','fixed-program-paths');
 if(!isC8){exact(c.proofVolume,['name','driver','labels'],'proof-volume');need(c.proofVolume.name===VOLUME&&c.proofVolume.driver==='local'&&c.proofVolume.labels&&typeof c.proofVolume.labels==='object'&&!Array.isArray(c.proofVolume.labels)&&Object.entries(c.proofVolume.labels).every(([k,v])=>typeof k==='string'&&typeof v==='string'),'proof-volume-fields');}
 const pins=[cp,...Object.values(c.tools),c.launcher,c.spec,...(isC8?[c.softwareBindingsArtifact]:[])],fence=async()=>{for(const p of pins)await readPinned(p);};await fence();const spec=JSON.parse((await readPinned(c.spec,true)).bytes.toString('utf8'));
 let contract=null;if(isC8)contract=c8GuardContract(c,spec,JSON.parse((await readPinned(c.softwareBindingsArtifact,true)).bytes.toString('utf8')));
 else need(spec.format==='nano-fresh-capella-three-owner-campaign-v1'&&spec.sourceBaseRevision===c.sourceBaseRevision&&spec.owners?.['vps-docker']?.host===TARGET&&spec.lifecycle?.deadlines?.maximumRunMs===policy.maximumRunMs,'fixed-campaign-binding');
 need(!isC8||spec.owners?.['vps-docker']?.host===TARGET,'c8-fixed-remote-target');
 const overallEnd=isC8?wrapperStartedAt+contract.maximumRunMs:null,proofVolume=isC8?contract.proofVolume:c.proofVolume,proofName=isC8?proofVolume.name:VOLUME;
 need(!isC8||performance.now()<overallEnd-contract.closeoutReserveMs,'c8-preparation-work-deadline');
 const codeRoots=Object.values(spec.owners??{}).flatMap(v=>[v.coreBuildRoot,v.networkBuildRoot]).filter(p=>typeof p==='string'&&isAbsolute(p));
 const outputs=[c.outputDirectory,c.campaignOutputDirectory];for(const p of outputs){need(typeof p==='string'&&isAbsolute(p)&&resolve(p)===p&&!/[\0\r\n]/.test(p)&&await realpath(dirname(p))===dirname(p),'new-output-path');const parent=await lstat(dirname(p),{bigint:true});need(parent.isDirectory()&&parent.uid===501n&&!(parent.mode&0o022n),'output-parent-owner');need(!overlaps(p,ROOT)&&codeRoots.every(root=>!overlaps(p,root))&&pins.every(a=>!overlaps(p,a.path))&&!overlaps(p,spec.fixture?.seedFile??ROOT)&&!overlaps(p,spec.fixture?.ledgerDirectory??ROOT),'output-protected-overlap');try{await lstat(p);throw new ResourceGuardError('output-already-exists');}catch(e){if(e.code!=='ENOENT')throw e;}}need(!overlaps(...outputs),'output-campaign-overlap');
 await mkdir(c.outputDirectory,{mode:0o700});const privateFile=async(name,body)=>{need(/^[a-z0-9][a-z0-9.-]*$/.test(name),'private-leaf');const h=await open(resolve(c.outputDirectory,name),constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600);try{await h.writeFile(body);await h.sync();}finally{await h.close();}if(isC8){const directory=await open(c.outputDirectory,constants.O_RDONLY|constants.O_DIRECTORY|constants.O_NOFOLLOW);try{await directory.sync();}finally{await directory.close();}}};const events=await open(resolve(c.outputDirectory,'resources.public.jsonl'),constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600),logs=[];await mkdir(resolve(c.outputDirectory,'docker-config'),{mode:0o700});
 const uuid=randomUUID().replaceAll('-',''),name='nano-resource-guard-'+uuid,label='nano.external.resource-guard';let id=null,creationAttempted=false,first=null,summary=null,launcherChild=null,interrupted=false;const onSignal=()=>{interrupted=true;};for(const s of ['SIGINT','SIGTERM','SIGHUP'])process.on(s,onSignal);
 const docker=async(a,deadline)=>command(c.tools.docker,['-H',ENDPOINT,'--config',resolve(c.outputDirectory,'docker-config'),...a],{deadline,cwd:c.outputDirectory});
 const json=raw=>{try{return JSON.parse(raw);}catch{throw new ResourceGuardError('sample-json');}};
 const absent=async(deadline)=>need((await docker(['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--format','{{.ID}}'],deadline)).trim()==='','probe-container-preexists');
 const volume=async(deadline)=>{const v=json(await docker(['volume','inspect',proofName,'--format','{{json .}}'],deadline));need(v.Name===proofName&&v.Driver==='local'&&canonical(v.Labels??{})===canonical(proofVolume.labels),'proof-volume-identity');return v;};
 const owned=async(deadline,{cleanup=false}={})=>{const v=json(await docker(['inspect',id,'--format',isC8?c8ProbeInspectionTemplate:'{{json .}}'],deadline));if(isC8)return (cleanup?validateC8ProbeIdentity:validateC8OwnedProbe)(v,{id,name,uuid,volumeName:proofName});need(v.Id===id&&v.Name==='/'+name&&v.Image===IMAGE&&v.Config.Labels?.[label]===uuid,'probe-container-identity');need(v.HostConfig.Mounts?.length===1&&v.HostConfig.Mounts[0].VolumeOptions?.NoCopy===true,'probe-volume-nocopy');need(v.Mounts?.length===1&&v.Mounts[0].Type==='volume'&&v.Mounts[0].Name===VOLUME&&v.Mounts[0].Destination==='/probe'&&v.Mounts[0].RW===false,'probe-volume-readonly');return v;};
 const linuxCommand=paths=>String.raw`printf '%s\n' NANO_DF_BEGIN; /usr/bin/df -Pk ${paths}; printf '%s\n' NANO_DF_END; /usr/bin/awk '$1 == "MemAvailable:" {print "NANO_MEM_KIB:" $2}' /proc/meminfo`;
 const vmCommand=linuxCommand('/probe'),remoteCommand=String.raw`set -eu; printf '%s\n' NANO_DF_BEGIN; /usr/bin/sudo -n /usr/bin/df -Pk / /home/ubuntu /var/lib/docker; printf '%s\n' NANO_DF_END; /usr/bin/awk '$1 == "MemAvailable:" {print "NANO_MEM_KIB:" $2}' /proc/meminfo`;
 try{
  const until=isC8?Math.min(performance.now()+30000,overallEnd-contract.closeoutReserveMs):performance.now()+30000;const image=json(await docker(['image','inspect',IMAGE,'--format',isC8?c8ProbeImageTemplate:'{{json .}}'],until));if(isC8)validateC8ProbeImage(image);else need(image.Id===IMAGE&&image.Os==='linux'&&image.Architecture==='arm64','probe-image');const before=await volume(until);await absent(until);
  if(isC8)await privateFile('sampler-intent.json',JSON.stringify({format:'nano-c8-resource-sampler-intent-v1',uuid,name,image:IMAGE,dockerEndpoint:ENDPOINT,specSha256:c.spec.sha256,sourceBaseRevision:c.sourceBaseRevision,maximumRunMs:contract.maximumRunMs,closeoutReserveMs:contract.closeoutReserveMs,productionQualified:false})+'\n');
  need(!isC8||!interrupted&&performance.now()<overallEnd-contract.closeoutReserveMs,'c8-sampler-work-deadline');creationAttempted=true;
  id=(await docker(['create',...(isC8?['--restart=no','--no-healthcheck']:[]),'--pull','never','--name',name,'--label',label+'='+uuid,'--read-only','--network','none','--cap-drop','ALL','--security-opt','no-new-privileges:true','--user','501:20','--pids-limit','16','--memory','64m','--cpus','0.1','--mount','type=volume,src='+proofName+',dst=/probe,readonly,volume-nocopy','--entrypoint','/bin/sh',IMAGE,'-c',isC8?'exec /bin/sleep '+contract.samplerSeconds:'exec /bin/sleep 28800'],until)).trim();need(/^[0-9a-f]{64}$/.test(id),'probe-container-id');need(canonical(await volume(until))===canonical(before),'proof-volume-changed');await owned(until);await docker(['start',id],until);
  const runSample=async()=>{const startedMs=performance.now(),deadline=startedMs+policy.sampleTimeoutMs;const observed=await Promise.allSettled([
   (async()=>{const [disk,text]=await Promise.all([statfs(dirname(c.campaignOutputDirectory),{bigint:true}),command(c.tools.vmStat,[],{deadline,cwd:c.outputDirectory})]);const physical=totalmem(),free=freemem();need(Number.isSafeInteger(physical)&&Number.isSafeInteger(free)&&free>=0&&free<=physical&&disk.bavail>=0n&&disk.bsize>0n,'mac-capacity');return {diskBytes:String(disk.bavail*disk.bsize),diskMetric:'mac-statfs-bavail',...parseVmStat(text,String(physical)),osFreeBytes:String(free)};})(),
   (async()=>{await volume(deadline);await owned(deadline);return parseLinuxCapacity(await docker(['exec',id,'/bin/sh','-ec',vmCommand],deadline),1);})(),
   (async()=>parseLinuxCapacity(await command(c.tools.ssh,['-o','BatchMode=yes','-o','ConnectTimeout=2','-o','ConnectionAttempts=1','-o','StrictHostKeyChecking=yes','-o','ServerAliveInterval=2','-o','ServerAliveCountMax=1',TARGET,remoteCommand],{deadline,cwd:c.outputDirectory}),3))()
  ]);const failed=observed.find(v=>v.status==='rejected');if(failed)throw failed.reason;const [mac,vm,vps]=observed.map(v=>v.value);return {startedMs,finishedMs:performance.now(),surfaces:{macHost:mac,macDockerVm:vm,vpsHost:vps}};};
  const sampler=async()=>{let timer;try{return await Promise.race([runSample(),new Promise((_,reject)=>{timer=setTimeout(()=>reject(new ResourceGuardError('resource-sample-timeout')),policy.sampleTimeoutMs);})]);}finally{clearTimeout(timer);}};
  const launch=async()=>{await fence();need(!probeOnly&&!interrupted,'launcher-guard');need(!isC8||performance.now()<overallEnd-contract.closeoutReserveMs,'c8-launch-work-deadline');const child=spawn(c.tools.node.path,isC8?contract.launcherArgs:[c.launcher.path,TARGET,c.sourceBaseRevision,c.campaignOutputDirectory,'--fresh-campaign',c.spec.path,'--fresh-campaign-sha',c.spec.sha256],{cwd:ROOT,env:process.env,shell:false,detached:false,stdio:['ignore',logs[0].fd,logs[1].fd]});launcherChild=child;const handle={pid:child.pid,closed:false,exitCode:null,signalCode:null,kill:s=>child.kill(s)};child.once('error',()=>{handle.exitCode=-1;});child.once('close',(exitCode,signalCode)=>{handle.closed=true;handle.exitCode=exitCode;handle.signalCode=signalCode;});return handle;};
  if(!probeOnly)for(const leaf of ['launcher-stdout.private.log','launcher-stderr.private.log'])logs.push(await open(resolve(c.outputDirectory,leaf),constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o600));
  await fence();summary=await supervise({sample:sampler,launch,clock:()=>performance.now(),sleep,stopRequested:()=>interrupted,probeOnly,...(isC8?{c8:{maximumRunMs:contract.maximumRunMs,closeoutReserveMs:contract.closeoutReserveMs},startedAt:wrapperStartedAt,closeoutStarted:child=>readC8CloseoutReceipt(c.campaignOutputDirectory,{pid:child.pid,sourceBaseRevision:c.sourceBaseRevision,specSha256:c.spec.sha256,maximumRunMs:contract.maximumRunMs,closeoutReserveMs:contract.closeoutReserveMs})}:{}),record:async event=>{await events.writeFile(JSON.stringify(event)+'\n');await events.sync();}});if(!summary.passed)first=summary.reason;
 }catch(e){first??=codeOf(e);}
 finally{
  // Cleanup is independent of launcher/source drift: only original Docker and
  // the UUID/name/image of our newly attempted sampler container authorize rm.
  const until=isC8?Math.min(performance.now()+15000,overallEnd):performance.now()+15000;try{if(creationAttempted){if(!/^[0-9a-f]{64}$/.test(id??'')){const found=(await docker(['ps','-a','--no-trunc','--filter','name=^/'+name+'$','--filter','label='+label+'='+uuid,'--format','{{.ID}}'],until)).trim();need(found===''||/^[0-9a-f]{64}$/.test(found),'probe-cleanup-discovery');id=found||null;}if(id){await owned(until,{cleanup:isC8});await docker(['rm','-f',id],until);}await absent(until);}}catch(e){first??=codeOf(e);}
  try{await fence();}catch(e){first??=codeOf(e);}if(isC8){for(const [h,code] of [...logs.map(h=>[h,'private-log-close']),[events,'resource-log-close']]){try{await h.sync();}catch{first??=code;}try{await h.close();}catch{first??=code;}}}else{for(const h of logs){try{await h.sync();await h.close();}catch{first??='private-log-close';}}try{await events.sync();await events.close();}catch{first??='resource-log-close';}for(const s of ['SIGINT','SIGTERM','SIGHUP'])process.off(s,onSignal);}if(summary?.launcherUnjoined)launcherChild?.unref();
 }
 if(isC8){
  const result={format:'nano-external-campaign-resource-guard-result-v2',...summary,passed:first===null&&summary?.passed===true,reason:first,policy:'test-resource-bounds-not-protocol',probeOnly,sourceBaseRevision:c.sourceBaseRevision,productionQualified:false};
  const terminal=await publishC8GuardResult({result,write:value=>privateFile('result-public.json',JSON.stringify(value)+'\n'),clock:()=>performance.now(),interrupted:()=>interrupted,overallEnd});
  for(const s of ['SIGINT','SIGTERM','SIGHUP'])process.off(s,onSignal);process.stdout.write(JSON.stringify(terminal.report)+'\n');process.exitCode=terminal.exitCode;return;
 }
 const result={format:isC8?'nano-external-campaign-resource-guard-result-v2':'nano-external-campaign-resource-guard-result-v1',passed:first===null&&summary?.passed===true,reason:first,policy:'test-resource-bounds-not-protocol',probeOnly,sourceBaseRevision:c.sourceBaseRevision,...summary,productionQualified:false};result.passed=first===null&&summary?.passed===true;result.reason=first;await privateFile('result-public.json',JSON.stringify(result)+'\n');process.stdout.write(JSON.stringify(result)+'\n');if(!result.passed)process.exitCode=1;
}

if(process.argv[1]&&fileURLToPath(import.meta.url)===resolve(process.argv[1]))main(process.argv.slice(2)).catch(e=>{process.stderr.write(JSON.stringify({passed:false,reason:codeOf(e),productionQualified:false})+'\n');process.exitCode=1;});
