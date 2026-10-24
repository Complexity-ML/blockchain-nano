// Only children created detached by this campaign may enter this registry.
const need=(ok,why)=>{if(!ok)throw Error('Fresh campaign command: '+why);};
const wait=ms=>new Promise(resolve=>setTimeout(resolve,ms));

export function createCampaignCommands({deadline,execFile,kill=(pid,sig)=>process.kill(pid,sig),now=()=>performance.now(),sleep=wait,setTimer=setTimeout,clearTimer=clearTimeout}={}){
 need(deadline&&[execFile,kill,now,sleep,setTimer,clearTimer].every(v=>typeof v==='function'),'deadline and callbacks');
 const active=new Set(),persistent=new Set();
 const valid=child=>Number.isSafeInteger(child?.pid)&&child.pid>1&&child.pid!==28722&&child.pid!==process.pid;
 function groupPresent(child){
  if(child?.pid===undefined)return false;
  need(valid(child),'reserved or invalid owned PID');
  try{kill(-child.pid,0);return true;}catch(e){if(e?.code==='ESRCH')return false;throw e;}
 }
 function signalGroup(child,signal){
  if(child?.pid===undefined)return;
  need(valid(child),'reserved or invalid owned PID');
  try{kill(-child.pid,signal);}catch(e){if(e?.code!=='ESRCH')throw e;}
 }
 async function terminate(child,closed,graceMs){
  let forced=false,first;const secondary=[];
  const retain=e=>{if(!first)first=e;else secondary.push(String(e));};
  const gone=()=>closed()&&!groupPresent(child);
  try{if(gone())return false;}catch(e){retain(e);}
  try{signalGroup(child,'SIGTERM');}catch(e){retain(e);}
  let until=now()+graceMs;
  while(now()<until){try{if(gone())break;}catch(e){retain(e);break;}await sleep(Math.min(25,Math.max(1,until-now())));}
  try{if(!gone()){forced=true;signalGroup(child,'SIGKILL');}}catch(e){retain(e);}
  until=now()+3000;
  while(now()<until){try{if(gone())break;}catch(e){retain(e);break;}await sleep(Math.min(25,Math.max(1,until-now())));}
  try{need(gone(),'owned group and close join required');}catch(e){retain(e);}
  if(first){try{Object.defineProperty(first,'campaignCommandSecondaryFailures',{value:secondary,configurable:true});}catch{}throw first;}
  return forced;
 }
 function execute(program,args,options={},cleanup=false){
  const requested=options.timeout??30000;let timeout;
  if(cleanup){need(deadline.phase==='closeout','cleanup only during closeout');need(Number.isSafeInteger(requested)&&requested>0,'bounded cleanup timeout');timeout=Math.min(requested,30000);}
  else{deadline.check();timeout=deadline.remaining(requested);}
  // Cleanup has an independent bounded grace even when it begins just before
  // overallEnd. The global deadline still fails the campaign, but must not
  // cancel the final exact-owner stop and leave a signing runtime active.
  const signals=[...(!cleanup?[deadline.signal]:[]),...(options.signal?[options.signal]:[])];
  for(const signal of signals){need(typeof signal?.addEventListener==='function'&&typeof signal.removeEventListener==='function','AbortSignal');if(signal.aborted)throw signal.reason??Error('Fresh campaign command aborted');}
  const {input,signal,timeout:ignoredTimeout,...rest}=options;
  let child,closed=false,primary,output={stdout:'',stderr:''},settling=false,timer;
  const secondary=[],listeners=[];let resolve,reject;
  const promise=new Promise((yes,no)=>{resolve=yes;reject=no;});promise.catch(()=>{});
  const record={promise,stop:null,child:null,closed:()=>closed};
  const retain=e=>{if(!primary)primary=e;else if(primary!==e)secondary.push(String(e));};
  async function finish(){
   if(settling)return;settling=true;clearTimer(timer);for(const [s,f]of listeners)s.removeEventListener('abort',f);
   try{if(await terminate(child,()=>closed,3000))retain(Error('Fresh campaign command required SIGKILL'));}catch(e){retain(e);}
   let joined=false;try{joined=closed&&!groupPresent(child);}catch(e){retain(e);}
   if(joined)active.delete(record);
   if(primary){try{Object.defineProperty(primary,'campaignCommandSecondaryFailures',{value:secondary,configurable:true});}catch{}reject(primary);}else resolve(output);
  }
  record.stop=e=>{retain(e);void finish();};
  try{
   child=execFile(program,args,{...rest,detached:true,timeout:0},(error,stdout,stderr)=>{output={stdout,stderr};if(error)retain(error);});
   record.child=child;active.add(record);
   child.once('error',retain);child.once('close',()=>{closed=true;void finish();});
   for(const s of signals){const f=()=>record.stop(s.reason??Error('Fresh campaign command aborted'));listeners.push([s,f]);s.addEventListener('abort',f,{once:true});}
   timer=setTimer(()=>record.stop(Error('Fresh campaign command timeout')),timeout);
   if(input!==undefined){need(child.stdin&&Buffer.isBuffer(input),'known stdin buffer');child.stdin.once('error',e=>record.stop(e));child.stdin.end(input);}
   else child.stdin?.end();
   for(const s of signals)if(s.aborted)record.stop(s.reason??Error('Fresh campaign command aborted'));
  }catch(e){if(child){record.stop(e);}else reject(e);}
  return promise;
 }
 async function closeAll(){
  const records=[...active],errors=[];
  for(const record of records)record.stop(Error('Fresh campaign cancelled owned in-flight command'));
  for(const record of records){try{await record.promise;}catch(e){errors.push(e);}if(active.has(record))try{await terminate(record.child,record.closed,1000);active.delete(record);}catch(e){errors.push(e);}}
  if(errors.length){const first=errors.shift();try{Object.defineProperty(first,'campaignCommandCloseoutFailures',{value:errors.map(String),configurable:true});}catch{}throw first;}
 }
 function cancelAll(reason){for(const record of [...active])record.stop(reason);}
 function ownPersistent(child){persistent.add(child);return child;}
 async function terminatePersistent(child,closed,graceMs){need(persistent.has(child),'only registered persistent child');return terminate(child,closed,graceMs);}
 return Object.freeze({exec:(program,args,options)=>execute(program,args,options),execOwnedCleanup:(program,args,options)=>execute(program,args,options,true),closeAll,cancelAll,ownPersistent,terminatePersistent});
}
