// Only direct spawn(detached:true) children created by this campaign may enter this registry.
const need=(ok,why)=>{if(!ok)throw Error('Fresh campaign command: '+why);};
const wait=ms=>new Promise(resolve=>setTimeout(resolve,ms));

export function createCampaignCommands({deadline,spawn,kill=(pid,sig)=>process.kill(pid,sig),now=()=>performance.now(),sleep=wait,setTimer=setTimeout,clearTimer=clearTimeout}={}){
 need(deadline&&[spawn,kill,now,sleep,setTimer,clearTimer].every(v=>typeof v==='function'),'deadline and callbacks');
 const active=new Set(),persistent=new Set(),pids=new WeakMap();
 const valid=child=>Number.isSafeInteger(child?.pid)&&child.pid>1&&child.pid!==28722&&child.pid!==process.pid;
 function register(child){
  need(child&&typeof child.once==='function','owned child handle');
  if(pids.has(child))need(pids.get(child)===child.pid,'owned PID changed');else pids.set(child,child.pid);
  return child;
 }
 function heldPid(child){need(pids.has(child)&&pids.get(child)===child.pid,'unregistered or changed owned PID');return child.pid;}
 function groupPresent(child){
  if(!child)return false;if(heldPid(child)===undefined)return false;
  need(valid(child),'reserved or invalid owned PID');
  try{kill(-child.pid,0);return true;}catch(e){if(e?.code==='ESRCH')return false;throw e;}
 }
 function signalGroup(child,signal){
  if(!child)return;if(heldPid(child)===undefined)return;
  need(valid(child),'reserved or invalid owned PID');
  try{kill(-child.pid,signal);}catch(e){if(e?.code!=='ESRCH')throw e;}
 }
 async function terminate(child,closed,graceMs){
  let forced=false,first,hasFirst=false;const secondary=[];
  const retain=e=>{if(!hasFirst){first=e;hasFirst=true;}else secondary.push(String(e));};
  const gone=()=>closed()&&!groupPresent(child);
  try{if(gone())return false;}catch(e){retain(e);}
  try{signalGroup(child,'SIGTERM');}catch(e){retain(e);}
  let until=now()+graceMs;
  while(now()<until){try{if(gone())break;}catch(e){retain(e);break;}await sleep(Math.min(25,Math.max(1,until-now())));}
  try{if(!gone()){forced=true;signalGroup(child,'SIGKILL');}}catch(e){retain(e);}
  until=now()+3000;
  while(now()<until){try{if(gone())break;}catch(e){retain(e);break;}await sleep(Math.min(25,Math.max(1,until-now())));}
  try{need(gone(),'owned group and close join required');}catch(e){retain(e);}
  if(hasFirst){try{Object.defineProperty(first,'campaignCommandSecondaryFailures',{value:secondary,configurable:true});}catch{}throw first;}
  return forced;
 }
 // Do not put argv, input or captured private output in an exception message.
 const systemFailure=(stage,error)=>{
  const code=Number.isSafeInteger(error?.code)||typeof error?.code==='string'&&/^[A-Z0-9_]{1,80}$/.test(error.code)?error.code:null;
  return Object.assign(Error('Fresh campaign command '+stage+' failed'),{code});
 };
 const exitFailure=(code,signal)=>Object.assign(Error('Fresh campaign command exited unsuccessfully'),{code,signal});
 function execute(program,args,options={},cleanup=false){
  const requested=options.timeout??30000;let timeout;
  if(cleanup){need(deadline.phase==='closeout','cleanup only during closeout');need(Number.isSafeInteger(requested)&&requested>0,'bounded cleanup timeout');timeout=Math.min(requested,30000);}
  else{deadline.check();timeout=deadline.remaining(requested);}
  // An exact-owner stop begun just before overallEnd retains its own bounded
  // grace; the global verdict still fails and no new audit or duty is allowed.
  const signals=[...(!cleanup?[deadline.signal]:[]),...(options.signal?[options.signal]:[])];
  for(const signal of signals){need(typeof signal?.addEventListener==='function'&&typeof signal.removeEventListener==='function','AbortSignal');if(signal.aborted)throw signal.reason??Error('Fresh campaign command aborted');}
  const {input,signal,timeout:ignoredTimeout,maxBuffer=1048576,encoding:requestedEncoding='utf8',...rest}=options;
  need(Number.isSafeInteger(maxBuffer)&&maxBuffer>=0,'finite per-stream byte limit');
  need(input===undefined||Buffer.isBuffer(input)&&input.length<=134217728,'bounded known stdin buffer');
  const stdin=input===undefined?undefined:Buffer.from(input),encoding=typeof requestedEncoding==='string'&&Buffer.isEncoding(requestedEncoding)?requestedEncoding:null;
  let child,closed=false,primary,hasPrimary=false,settling=false,timer;
  const secondary=[],listeners=[],parts=[[],[]],sizes=[0,0],overflow=[false,false];let resolve,reject;
  const promise=new Promise((yes,no)=>{resolve=yes;reject=no;});promise.catch(()=>{});
  const record={promise,stop:null,child:null,closed:()=>closed};
  const retain=e=>{if(!hasPrimary){primary=e;hasPrimary=true;}else if(primary!==e)secondary.push(String(e));};
  const outputs=()=>Object.fromEntries(['stdout','stderr'].map((name,i)=>{const bytes=Buffer.concat(parts[i],sizes[i]);return [name,encoding===null?bytes:bytes.toString(encoding)];}));
  async function finish(){
   if(settling)return;settling=true;clearTimer(timer);for(const [s,f]of listeners)s.removeEventListener('abort',f);
   try{if(await terminate(child,()=>closed,3000))retain(Error('Fresh campaign command required SIGKILL'));}catch(e){retain(e);}
   let joined=false;try{joined=closed&&!groupPresent(child);}catch(e){retain(e);}
   if(joined)active.delete(record);else retain(Error('Fresh campaign command owned group and close join required'));
   const output=outputs();
   if(hasPrimary){try{Object.defineProperty(primary,'campaignCommandSecondaryFailures',{value:secondary,configurable:true});for(const name of ['stdout','stderr'])if(!Object.hasOwn(primary,name))Object.defineProperty(primary,name,{value:output[name],configurable:true});}catch{}reject(primary);}else resolve(output);
  }
  record.stop=e=>{retain(e);void finish();};
  const collect=(i,chunk)=>{
   if(overflow[i])return;
   try{
    const bytes=Buffer.isBuffer(chunk)?chunk:Buffer.from(chunk),available=maxBuffer-sizes[i],kept=Math.min(bytes.length,available);
    if(kept){parts[i].push(Buffer.from(bytes.subarray(0,kept)));sizes[i]+=kept;}
    if(bytes.length>available){overflow[i]=true;const error=Error('Fresh campaign command '+(i===0?'stdout':'stderr')+' byte limit exceeded');error.code='ERR_CHILD_PROCESS_STDIO_MAXBUFFER';record.stop(error);}
   }catch(error){record.stop(systemFailure(i===0?'stdout':'stderr',error));}
  };
  // Known stdin copying/options preparation must not age a work admission.
  // Recheck immediately before the only process-creation call.
  if(!cleanup){deadline.check();timeout=deadline.remaining(requested);}
  try{
   // execFile does not forward detached in Node18; spawn is the PGID authority.
   child=spawn(program,args,{...rest,detached:true,stdio:['pipe','pipe','pipe']});record.child=child;active.add(record);register(child);
   child.on('error',error=>record.stop(systemFailure('spawn',error)));
   child.once('exit',(code,signal)=>{if(code!==0||signal!==null)retain(exitFailure(code,signal));});
   child.once('close',(code,signal)=>{if(code!==0||signal!==null)retain(exitFailure(code,signal));closed=true;void finish();});
   need(child.stdout&&child.stderr&&child.stdin,'owned pipe stdio required');
   for(const [i,stream]of [child.stdout,child.stderr].entries()){stream.on('data',chunk=>collect(i,chunk));stream.on('error',error=>record.stop(systemFailure(i===0?'stdout':'stderr',error)));}
   child.stdin.on('error',error=>record.stop(systemFailure('stdin',error)));
   for(const s of signals){const f=()=>record.stop(s.reason??Error('Fresh campaign command aborted'));listeners.push([s,f]);s.addEventListener('abort',f,{once:true});}
   timer=setTimer(()=>record.stop(Error('Fresh campaign command timeout')),timeout);
   try{child.stdin.end(stdin);}catch(error){record.stop(systemFailure('stdin',error));}
   for(const s of signals)if(s.aborted)record.stop(s.reason??Error('Fresh campaign command aborted'));
  }catch(error){const failure=systemFailure('spawn setup',error);if(child)record.stop(failure);else reject(failure);}
  return promise;
 }
 async function closeAll(){
  const records=[...active],errors=[];
  for(const record of records)record.stop(Error('Fresh campaign cancelled owned in-flight command'));
  for(const record of records){try{await record.promise;}catch(e){errors.push(e);}if(active.has(record))try{await terminate(record.child,record.closed,1000);active.delete(record);}catch(e){errors.push(e);}}
  if(errors.length){const first=errors.shift();try{Object.defineProperty(first,'campaignCommandCloseoutFailures',{value:errors.map(String),configurable:true});}catch{}throw first;}
 }
 function cancelAll(reason){for(const record of [...active])record.stop(reason);}
 function ownPersistent(child){register(child);persistent.add(child);return child;}
 async function terminatePersistent(child,closed,graceMs){need(persistent.has(child),'only registered persistent child');return terminate(child,closed,graceMs);}
 return Object.freeze({exec:(program,args,options)=>execute(program,args,options),execOwnedCleanup:(program,args,options)=>execute(program,args,options,true),closeAll,cancelAll,ownPersistent,terminatePersistent});
}
