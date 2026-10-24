// Injected, bounded finalized-event observation. No HTTP/runtime/clock imports.
const ROOT=/^0x[0-9a-f]{64}$/,ZERO='0x'+'0'.repeat(64),U64=(1n<<64n)-1n;
export class FinalizedLifecycleScanError extends Error{
 constructor(code,reason){super('Finalized lifecycle scanner: '+reason);this.name='FinalizedLifecycleScanError';this.code=code;this.reason=reason;}
}
const need=(ok,code,reason)=>{if(!ok)throw new FinalizedLifecycleScanError(code,reason);};
const plain=v=>v!==null&&typeof v==='object'&&Object.getPrototypeOf(v)===Object.prototype;
const quantity=(v,label)=>{need(typeof v==='bigint'||typeof v==='string'&&/^(?:0|[1-9][0-9]*|0x(?:0|[1-9a-f][0-9a-f]*))$/.test(v),'history-invalid',label);const n=BigInt(v);need(n>=0n&&n<=U64,'history-invalid',label);return n;};
const wireQuantity=(v,label)=>{need(typeof v==='string'&&/^(?:0|[1-9][0-9]*)$/.test(v),'history-invalid',label);return quantity(v,label);};
const isRoot=v=>typeof v==='string'&&ROOT.test(v);
function synchronous(value,label){
 if(value!==null&&(typeof value==='object'||typeof value==='function')&&typeof value.then==='function'){
  if(value instanceof Promise)value.catch(()=>{});
  throw new FinalizedLifecycleScanError('callback-invalid',label+' must be synchronous');
 }
 return value;
}
function checkpoint(value){need(plain(value)&&Object.keys(value).sort().join(',')==='epoch,root'&&isRoot(value.root)&&value.root!==ZERO,'history-invalid','explicit common checkpoint');return {epoch:quantity(value.epoch,'checkpoint epoch'),root:value.root};}

export function createFinalizedLifecycleScanner({readFinalizedHeader,readFinalizedBlock,now,sleep,check,initialRoot,limits,pollIntervalMs=3000}){
 need([readFinalizedHeader,readFinalizedBlock,now,sleep,check].every(f=>typeof f==='function'),'callback-invalid','complete injected capabilities');
 need(isRoot(initialRoot)&&initialRoot!==ZERO,'history-invalid','independent bootstrap sentinel');
 need(plain(limits)&&Number.isSafeInteger(limits.maximumBlocks)&&limits.maximumBlocks>0&&limits.maximumBlocks<=1000000&&typeof limits.maximumBytes==='bigint'&&limits.maximumBytes>0n&&limits.maximumBytes<=U64&&typeof limits.maximumSpanSlots==='bigint'&&limits.maximumSpanSlots>0n&&limits.maximumSpanSlots<=U64,'capacity','explicit finite limits');
 need(Number.isSafeInteger(pollIntervalMs)&&pollIntervalMs>0&&pollIntervalMs<=3000,'capacity','finite poll interval');
 const bounds=Object.freeze({...limits});let state=null,busy=false,lastClock=-Infinity;
 const time=()=>{const t=synchronous(now(),'clock');need(Number.isFinite(t)&&t>=0&&t>=lastClock,'history-invalid','monotone clock');lastClock=t;return t;};
 function guard(deadline){synchronous(check(),'checkpoint callback');need(Number.isFinite(deadline)&&deadline>0,'callback-invalid','absolute phase deadline');need(time()<deadline,'event-not-observed','absolute phase deadline expired');}
 async function awaited(operation,deadline){guard(deadline);const value=await operation();guard(deadline);return value;}
 function bytes(value,search,deadline){
  let copy,text;try{copy=structuredClone(value);text=JSON.stringify(copy);}catch{throw new FinalizedLifecycleScanError('history-invalid','plain bounded HTTP projection');}
  guard(deadline);need(typeof text==='string','history-invalid','JSON response');const n=BigInt(Buffer.byteLength(text));search.wireBytes+=n;
  need(search.wireBytes<=bounds.maximumBytes,'capacity','phase response byte limit');return {copy,bytes:n};
 }
 function header(raw,search,deadline){
  const {copy:h}=bytes(raw,search,deadline),m=h?.data?.header?.message;
  need(h?.execution_optimistic===false&&isRoot(h.data?.root)&&plain(m)&&isRoot(m.parent_root)&&isRoot(m.state_root),'history-invalid','nonoptimistic finalized header');
  return {root:h.data.root,slot:wireQuantity(m.slot,'header slot'),parent:m.parent_root,stateRoot:m.state_root};
 }
 function block(root,raw,search,deadline){
  const {copy:r,bytes:n}=bytes(raw,search,deadline),m=r?.data?.message;
  need(r?.execution_optimistic===false&&r.finalized===true&&plain(m)&&plain(m.body)&&isRoot(m.parent_root)&&m.parent_root!==ZERO&&isRoot(m.state_root),'history-invalid','canonical finalized block projection');
  wireQuantity(m.proposer_index,'proposer index');return {root,slot:wireQuantity(m.slot,'block slot'),parent:m.parent_root,stateRoot:m.state_root,message:m,bytes:n};
 }
 const retainedBytes=rows=>rows.reduce((n,r)=>n+r.bytes,0n);
 function capacity(rows,extraCount,extraBytes,headSlot,slot){
  need(rows.length+extraCount<=bounds.maximumBlocks,'capacity','retained and staged block limit');
  need(retainedBytes(rows)+extraBytes<=bounds.maximumBytes,'capacity','retained and staged byte limit');
  need(headSlot>=slot&&headSlot-slot<=bounds.maximumSpanSlots,'capacity','retained slot span limit');
 }
 function query(input){
  need(plain(input)&&Object.keys(input).sort().join(',')==='checkpoint,match,minimumSlot,phaseDeadline'&&typeof input.match==='function','callback-invalid','explicit search contract');
  const cp=checkpoint(input.checkpoint);need(cp.root!==initialRoot,'history-invalid','common checkpoint must be a published block, not bootstrap sentinel');guard(input.phaseDeadline);
  return {checkpoint:cp,minimumSlot:quantity(input.minimumSlot,'minimum event slot'),match:input.match,deadline:input.phaseDeadline,searched:new Set(),wireBytes:0n};
 }
 function matchRecord(record,search){
  if(record.slot<=search.minimumSlot||search.searched.has(record.root))return null;
  guard(search.deadline);const observed=structuredClone({root:record.root,slot:record.slot,message:record.message});guard(search.deadline);
  const found=synchronous(search.match(observed),'event predicate');guard(search.deadline);search.searched.add(record.root);
  if(found===null||found===undefined)return null;
  need(plain(found),'callback-invalid','predicate returns plain result or null');let result;try{result=structuredClone(found);}catch{throw new FinalizedLifecycleScanError('callback-invalid','detached predicate result');}
  guard(search.deadline);return result;
 }
 async function admitHead(search){
  const cp=search.checkpoint;
  if(state){
   need(cp.epoch>=state.checkpoint.epoch,'history-invalid','common checkpoint epoch regression');
   need(cp.root===state.checkpoint.root||cp.epoch>state.checkpoint.epoch,'history-invalid','same epoch conflicting common root');
   const prior=state.rows.findIndex(r=>r.root===state.checkpoint.root),known=state.rows.findIndex(r=>r.root===cp.root);need(prior>=0,'history-invalid','retained checkpoint authority');
   if(known>=0){
    need(known<=prior,'history-invalid','new checkpoint is not a descendant');
    // This ancestry was already traversed. Promotion/pruning does not rely on
    // a newly asserted header or reset the finalized cursor.
    if(cp.root!==state.checkpoint.root){guard(search.deadline);state={checkpoint:{...cp},rows:state.rows.slice(0,known+1)};}
   }else if(search.minimumSlot===U64){
    // Explicit no-event refresh can discard memo bodies BELOW the previously
    // proven checkpoint before staging a bounded extension. It admits neither
    // the requested new checkpoint nor a new cursor before continuity proof.
    guard(search.deadline);state={...state,rows:state.rows.slice(0,prior+1)};
   }
  }
  const h=header(await awaited(readFinalizedHeader,search.deadline),search,search.deadline);
  const old=state?.rows??[],oldByRoot=new Map(old.map(r=>[r.root,r])),pending=[],pendingRoots=new Set();let tip=h.root,childSlot=null,pendingBytes=0n;
  while(!oldByRoot.has(tip)){
   guard(search.deadline);need(tip!==ZERO&&tip!==initialRoot,'history-invalid','checkpoint/cursor not reached before bootstrap');
   need(!pendingRoots.has(tip),'history-invalid','parent cycle');pendingRoots.add(tip);
   capacity(old,pending.length+1,pendingBytes,h.slot,childSlot??h.slot);
   const r=block(tip,await awaited(()=>readFinalizedBlock(tip),search.deadline),search,search.deadline);
   need(childSlot===null?r.slot===h.slot:r.slot<childSlot,'history-invalid','strictly decreasing parent slots');
   if(pending.length===0)need(r.parent===h.parent&&r.stateRoot===h.stateRoot,'history-invalid','sealed header/block identity');
   need(!pendingRoots.has(r.parent),'history-invalid','parent cycle');pendingBytes+=r.bytes;capacity(old,pending.length+1,pendingBytes,h.slot,r.slot);pending.push(r);
   if(!state&&r.root===cp.root)break;
   need(r.parent!==r.root,'history-invalid','self parent');childSlot=r.slot;tip=r.parent;
  }
  if(state){
   need(tip===state.rows[0].root,'history-invalid','finalized tip regressed or branched before cursor');
   if(pending.length===0)need(h.slot===old[0].slot&&h.parent===old[0].parent&&h.stateRoot===old[0].stateRoot,'history-invalid','cached header identity changed');
   else need(old[0].slot<pending.at(-1).slot,'history-invalid','cursor parent slot regression');
  }
  const rows=[...pending,...old],index=rows.findIndex(r=>r.root===cp.root);need(index>=0,'history-invalid','common checkpoint not on finalized branch');
  if(state){const prior=rows.findIndex(r=>r.root===state.checkpoint.root);need(prior>=index,'history-invalid','new checkpoint is not a descendant');}
  // A proven advancing checkpoint bounds retained history. Older event search
  // may refetch a bounded suffix, but cannot replace this admitted anchor.
  const kept=state&&cp.root!==state.checkpoint.root?rows.slice(0,index+1):rows;
  capacity(kept,0,0n,h.slot,kept.at(-1).slot);guard(search.deadline);state={checkpoint:{...cp},rows:kept};
 }
 async function poll(search){
  guard(search.deadline);await admitHead(search);guard(search.deadline);
  for(const record of state.rows){const found=matchRecord(record,search);if(found!==null)return found;}
  for(;;){
   guard(search.deadline);const tail=state.rows.at(-1);
   if(tail.slot<=search.minimumSlot||tail.parent===initialRoot||tail.parent===ZERO)return null;
   need(!state.rows.some(r=>r.root===tail.parent),'history-invalid','cached parent cycle');
   capacity(state.rows,1,0n,state.rows[0].slot,tail.slot);
   const record=block(tail.parent,await awaited(()=>readFinalizedBlock(tail.parent),search.deadline),search,search.deadline);
   need(record.slot<tail.slot&&record.parent!==record.root&&!state.rows.some(r=>r.root===record.parent),'history-invalid','strict older parent slot without cycle');
   capacity(state.rows,1,record.bytes,state.rows[0].slot,record.slot);guard(search.deadline);state={...state,rows:[...state.rows,record]};
   const found=matchRecord(record,search);if(found!==null)return found;
  }
 }
 async function exclusive(input,repeat){
  need(!busy,'callback-invalid','one active scanner search');busy=true;
  try{
   const search=query(input);
   for(;;){guard(search.deadline);const found=await poll(search);guard(search.deadline);if(found!==null)return found;if(!repeat)return null;const remaining=search.deadline-time();guard(search.deadline);await awaited(()=>sleep(Math.min(pollIntervalMs,remaining)),search.deadline);guard(search.deadline);}
  }finally{busy=false;}
 }
 return Object.freeze({find:input=>exclusive(input,true),pollOnce:input=>exclusive(input,false),snapshot:()=>state?{checkpoint:{epoch:state.checkpoint.epoch.toString(),root:state.checkpoint.root},tip:state.rows[0].root,retainedBlocks:state.rows.length,retainedBytes:retainedBytes(state.rows).toString()}:null});
}

// Called only after the shared observer authenticated a new common checkpoint.
// Each scanner still traverses its OWN continuity; no reset or trusted bool.
export async function refreshFinalizedLifecycleScanners(scanners,{checkpoint:required,phaseDeadline,now,check}){
 need(Array.isArray(scanners)&&scanners.length<=3&&new Set(scanners).size===scanners.length&&scanners.every(x=>typeof x?.pollOnce==='function')&&typeof now==='function'&&typeof check==='function','callback-invalid','bounded distinct scanners and refresh capabilities');
 const parsed=checkpoint(required),cp={epoch:parsed.epoch.toString(),root:parsed.root};let last=-Infinity,failed=false,first;
 const retain=e=>{if(!failed){failed=true;first=e;}};
 const guard=()=>{synchronous(check(),'refresh checkpoint');const t=synchronous(now(),'refresh clock');need(Number.isFinite(t)&&t>=0&&t>=last,'history-invalid','monotone refresh clock');last=t;need(Number.isFinite(phaseDeadline)&&t<phaseDeadline,'event-not-observed','shared common-finality refresh deadline');};
 guard();const tasks=scanners.map(async scanner=>{try{guard();const result=await scanner.pollOnce({checkpoint:cp,minimumSlot:U64,match:()=>null,phaseDeadline});guard();need(result===null,'callback-invalid','no-event checkpoint refresh result');}catch(e){retain(e);throw e;}});
 // Joining every branch prevents a fast refusal from abandoning owned reads.
 await Promise.allSettled(tasks);try{guard();}catch(e){retain(e);}if(failed)throw first;
 return {refreshedScanners:scanners.length};
}

export function resolveActiveOwnerDuties({owner,signerIdentities,head,responses,slotsPerEpoch}){
 need(['mac-native','mac-docker'].includes(owner)&&Array.isArray(signerIdentities)&&signerIdentities.length===16&&Array.isArray(responses)&&responses.length===16,'history-invalid','sixteen prepared owner identities and responses');
 const m=head?.data?.header?.message;need(head?.execution_optimistic===false&&isRoot(head.data?.root)&&plain(m)&&isRoot(m.state_root),'history-invalid','sealed owner head');
 const slots=quantity(slotsPerEpoch,'slots per epoch');need(slots>0n,'history-invalid','slots per epoch');const epoch=wireQuantity(m.slot,'owner head slot')/slots,keys=new Set(),keyEnvs=new Set(),indices=new Set(),active=[];
 for(let i=0;i<16;++i){
  const id=signerIdentities[i],entry=responses[i],response=entry?.response,r=response?.data,v=r?.validator;
  need(plain(id)&&typeof id.keyEnv==='string'&&id.keyEnv.length>0&&!keyEnvs.has(id.keyEnv)&&/^0x[0-9a-f]{96}$/.test(id.publicKey)&&!keys.has(id.publicKey),'history-invalid','unique prepared owner keys');keys.add(id.publicKey);keyEnvs.add(id.keyEnv);
  need(plain(entry)&&entry.stateRoot===m.state_root&&entry.publicKey===id.publicKey,'history-invalid','response query binds the same sealed state and prepared key');
  need(response?.execution_optimistic===false&&plain(r)&&plain(v)&&v.pubkey===id.publicKey&&typeof v.slashed==='boolean','history-invalid','same sealed-state owner key row');
  const index=wireQuantity(r.index,'resolved owner index');need(!indices.has(index.toString()),'history-invalid','unique resolved owner indices');indices.add(index.toString());
  const activation=wireQuantity(v.activation_epoch,'activation epoch'),exit=wireQuantity(v.exit_epoch,'exit epoch');need(activation<=exit,'history-invalid','owner activation/exit chronology');
  if(!v.slashed&&activation<=epoch&&epoch<exit)active.push(Object.freeze({owner,publicKey:id.publicKey,index:'0x'+index.toString(16),selectionStateRoot:m.state_root}));
 }
 return Object.freeze(active);
}

export function matchOwnerFinalizedDuty(candidates,record,commonCheckpoint){
 need(Array.isArray(candidates)&&candidates.length>0&&isRoot(record?.root)&&plain(record.message),'history-invalid','resolved owner candidates and actual block');
 const index=wireQuantity(record.message.proposer_index,'actual proposer index'),matches=candidates.filter(d=>quantity(d.index,'candidate index')===index);
 need(matches.length<=1,'history-invalid','unique selected proposer identity');if(!matches.length)return null;const duty=matches[0];
 need(['mac-native','mac-docker'].includes(duty.owner)&&/^0x[0-9a-f]{96}$/.test(duty.publicKey)&&isRoot(duty.selectionStateRoot),'history-invalid','resolved selected owner identity');
 return {...duty,beforeProposalRoot:record.root,beforeProposalSlot:'0x'+quantity(record.slot,'proposal slot').toString(16),commonCheckpoint:{epoch:checkpoint(commonCheckpoint).epoch.toString(),root:commonCheckpoint.root}};
}

export function matchFixedOwnerDuty(duty,record,cutoffSlot){
 need(plain(duty)&&['mac-native','mac-docker'].includes(duty.owner)&&/^0x[0-9a-f]{96}$/.test(duty.publicKey)&&isRoot(record?.root)&&plain(record.message),'history-invalid','fixed post-restore owner identity');
 const index=quantity(duty.index,'fixed owner index'),slot=quantity(record.slot,'actual proposal slot'),cutoff=quantity(cutoffSlot,'post-restore cutoff');
 return wireQuantity(record.message.proposer_index,'actual proposer index')===index&&slot>cutoff?{root:record.root,slot:'0x'+slot.toString(16)}:null;
}

export function revalidateOwnerDuty(duty,{head,stateRoot,publicKey,response,slotsPerEpoch}){
 need(plain(duty)&&['mac-native','mac-docker'].includes(duty.owner)&&/^0x[0-9a-f]{96}$/.test(duty.publicKey),'history-invalid','fixed selected owner identity');
 const m=head?.data?.header?.message,v=response?.data?.validator,slots=quantity(slotsPerEpoch,'fresh slots per epoch');
 need(slots>0n&&head?.execution_optimistic===false&&isRoot(head.data?.root)&&plain(m)&&isRoot(m.state_root)&&m.state_root===stateRoot&&publicKey===duty.publicKey,'history-invalid','fresh selected-key sealed-state query');
 need(response?.execution_optimistic===false&&plain(v)&&v.pubkey===duty.publicKey&&v.slashed===false&&wireQuantity(response.data.index,'fresh selected index')===quantity(duty.index,'fixed selected index'),'history-invalid','same unslashed selected key/index');
 const slot=wireQuantity(m.slot,'fresh selected slot'),epoch=slot/slots,activation=wireQuantity(v.activation_epoch,'fresh activation epoch'),exit=wireQuantity(v.exit_epoch,'fresh exit epoch');
 need(activation<=epoch&&epoch<exit,'history-invalid','selected key remains active before stop');
 return {...structuredClone(duty),selectionRevalidatedStateRoot:stateRoot,selectionRevalidatedHeadRoot:head.data.root,selectionRevalidatedSlot:'0x'+slot.toString(16)};
}

export function ownerProposalCutoff(headSlot,journalProposalSlot){
 if(typeof headSlot==='number'){need(Number.isSafeInteger(headSlot)&&headSlot>=0,'history-invalid','safe JSON stopped head slot');headSlot=BigInt(headSlot);}
 const head=quantity(headSlot,'stopped head slot');if(journalProposalSlot===null)return head;
 const proposed=quantity(journalProposalSlot,'stopped journal proposal slot');return proposed>head?proposed:head;
}
