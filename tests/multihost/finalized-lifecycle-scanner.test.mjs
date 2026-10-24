// SOURCE ONLY: not executed by this author. All clocks/HTTP/sleeps are injected.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createFinalizedLifecycleScanner,refreshFinalizedLifecycleScanners,FinalizedLifecycleScanError,resolveActiveOwnerDuties,matchOwnerFinalizedDuty,matchFixedOwnerDuty,revalidateOwnerDuty,ownerProposalCutoff} from './finalized-lifecycle-scanner.mjs';
const root=n=>'0x'+BigInt(n).toString(16).padStart(64,'0'),initial=root(999),pub=n=>'0x'+BigInt(n).toString(16).padStart(96,'0'),cp=(n,epoch=n)=>({epoch:String(epoch),root:root(n)});
const block=(n,parent=n-1,slot=n*10,proposer=n)=>({execution_optimistic:false,finalized:true,data:{message:{slot:String(slot),proposer_index:String(proposer),parent_root:parent===0?initial:root(parent),state_root:root(100+n),body:{}}}});
function harness({deadline=1000,limits={},alterBlock=null,alterHeader=null,onSleep=null}={}){
 let at=0,tip=5;const calls={headers:0,blocks:[],sleeps:[]},blocks=new Map(Array.from({length:8},(_,i)=>[root(i+1),block(i+1)]));
 const deps={initialRoot:initial,limits:{maximumBlocks:32,maximumBytes:100000n,maximumSpanSlots:1000n,...limits},now:()=>at,check:()=>{},sleep:async ms=>{calls.sleeps.push(ms);at+=ms;onSleep?.({setTime:t=>{at=t;},setTip:t=>{tip=t;}});},readFinalizedHeader:async()=>{calls.headers++;const b=blocks.get(root(tip)).data.message,h={execution_optimistic:false,data:{root:root(tip),header:{message:{slot:b.slot,parent_root:b.parent_root,state_root:b.state_root}}}};return alterHeader?.(h,{setTime:t=>{at=t;}})??h;},readFinalizedBlock:async r=>{calls.blocks.push(r);const b=structuredClone(blocks.get(r));assert(b,'Known synthetic root requested');return alterBlock?.(r,b,{setTime:t=>{at=t;}})??b;}};
 const scanner=createFinalizedLifecycleScanner(deps),query=(match=()=>null,extra={})=>({checkpoint:cp(2),minimumSlot:0n,match,phaseDeadline:deadline,...extra});
 return {scanner,calls,blocks,query,setTime:t=>{at=t;},setTip:t=>{tip=t;},time:()=>at};
}
const error=code=>e=>e instanceof FinalizedLifecycleScanError&&e.code===code;

test('candidate is accepted only after the actual common checkpoint was traversed; older canonical candidates also work',async()=>{
 const h=harness();let matchedBeforeAnchor=false;
 const result=await h.scanner.pollOnce(h.query(record=>{matchedBeforeAnchor||=!h.calls.blocks.includes(root(2));return record.root===root(4)?{root:record.root}:null;}));
 assert.equal(result.root,root(4));assert.equal(matchedBeforeAnchor,false);assert.deepEqual(h.calls.blocks,[root(5),root(4),root(3),root(2)]);
 const older=await h.scanner.pollOnce(h.query(record=>record.root===root(1)?{root:record.root}:null));assert.equal(older.root,root(1));assert.equal(h.calls.blocks.at(-1),root(1));
});

test('same tip needs no block fetch; an extension joins the cursor without refetching old history',async()=>{
 const h=harness();assert.equal(await h.scanner.pollOnce(h.query(()=>null,{minimumSlot:45n})),null);const reads=h.calls.blocks.length;
 assert.equal(await h.scanner.pollOnce(h.query(()=>null,{minimumSlot:45n})),null);assert.equal(h.calls.blocks.length,reads);assert.equal(h.calls.headers,2);
 h.setTip(6);const found=await h.scanner.pollOnce(h.query(r=>r.root===root(6)?{root:r.root}:null));assert.equal(found.root,root(6));assert.deepEqual(h.calls.blocks.slice(reads),[root(6)]);
 const before=h.calls.blocks.length;await h.scanner.pollOnce(h.query(()=>null,{checkpoint:cp(4),minimumSlot:45n}));assert.equal(h.calls.blocks.length,before);assert.equal(h.scanner.snapshot().checkpoint.root,root(4));assert.equal(h.scanner.snapshot().retainedBlocks,3);
 const full=harness({limits:{maximumBlocks:4}});await full.scanner.pollOnce(full.query(()=>({root:root(5)})));assert.equal(full.scanner.snapshot().retainedBlocks,4);full.setTip(6);assert.equal((await full.scanner.pollOnce(full.query(()=>({root:root(6)}),{checkpoint:cp(3)}))).root,root(6));assert.equal(full.scanner.snapshot().retainedBlocks,4);
});

test('rolling authenticated checkpoint refresh spans more than2048slots without larger windows and joins all refresh branches',async()=>{
 let at=0,tip=5,reads=0;const initialLong=root(1000000),wire=new Map(Array.from({length:2900},(_,i)=>{const n=i+1,b=block(n,n-1,n);if(n===1)b.data.message.parent_root=initialLong;return [root(n),b];}));
 const scanner=createFinalizedLifecycleScanner({initialRoot:initialLong,limits:{maximumBlocks:600,maximumBytes:1000000n,maximumSpanSlots:1024n},now:()=>at,check:()=>{},sleep:async ms=>{at+=ms;},readFinalizedHeader:async()=>{at++;const m=wire.get(root(tip)).data.message;return {execution_optimistic:false,data:{root:root(tip),header:{message:{slot:m.slot,parent_root:m.parent_root,state_root:m.state_root}}}};},readFinalizedBlock:async r=>{reads++;at++;return structuredClone(wire.get(r));}});
 await scanner.pollOnce({checkpoint:cp(2,0),minimumSlot:4n,match:r=>r.root===root(5)?{root:r.root}:null,phaseDeadline:5000});
 for(const next of [405,805,1205,1605,2005,2405]){
  const oldCount=scanner.snapshot().retainedBlocks,before=reads;tip=next;
  assert.deepEqual(await refreshFinalizedLifecycleScanners([scanner],{checkpoint:cp(next,Math.floor(next/8)),phaseDeadline:5000,now:()=>at,check:()=>{}}),{refreshedScanners:1});
  assert(reads-before<=600-oldCount,'Retained plus staged block budget is never enlarged');assert.equal(scanner.snapshot().tip,root(next));assert.equal(scanner.snapshot().checkpoint.root,root(next));assert.equal(scanner.snapshot().retainedBlocks,1);assert(BigInt(scanner.snapshot().retainedBytes)<=1000000n);
 }
 assert(tip>2048);const beforeLate=scanner.snapshot();tip=2805;await assert.rejects(refreshFinalizedLifecycleScanners([scanner],{checkpoint:cp(2805,350),phaseDeadline:at+10,now:()=>at,check:()=>{}}),error('event-not-observed'));assert.deepEqual(scanner.snapshot(),beforeLate);
 let release,joined=false,settled=false;const gate=new Promise(resolve=>{release=resolve;}),first=new Error('first source refusal');
 const pending=refreshFinalizedLifecycleScanners([{pollOnce:async()=>{throw first;}},{pollOnce:async()=>{await gate;joined=true;return null;}}],{checkpoint:cp(2),phaseDeadline:100,now:()=>0,check:()=>{}});pending.then(()=>{settled=true;},()=>{settled=true;});await Promise.resolve();assert.equal(settled,false);release();await assert.rejects(pending,e=>e===first);assert.equal(joined,true);
 let clock=0;await assert.rejects(refreshFinalizedLifecycleScanners([{pollOnce:async()=>{clock=100;return null;}}],{checkpoint:cp(2),phaseDeadline:100,now:()=>clock,check:()=>{}}),error('event-not-observed'));
});

test('false common root, conflicting checkpoint or non-descendant finalized extension refuse without cache replacement',async()=>{
 const first=harness();let matches=0;await assert.rejects(first.scanner.pollOnce(first.query(()=>{matches++;return {root:root(5)};},{checkpoint:cp(77)})),error('history-invalid'));assert.equal(matches,0);assert.equal(first.scanner.snapshot(),null);
 const h=harness();await h.scanner.pollOnce(h.query(()=>({root:root(5)})));const before=h.scanner.snapshot();
 for(const checkpoint of [cp(1,1),cp(3,2),cp(77,6)])await assert.rejects(h.scanner.pollOnce(h.query(()=>({root:root(5)}),{checkpoint})),error('history-invalid'));
 assert.deepEqual(h.scanner.snapshot(),before);h.blocks.set(root(6),block(6,4));h.setTip(6);await assert.rejects(h.scanner.pollOnce(h.query(()=>({root:root(6)}))),error('history-invalid'));assert.deepEqual(h.scanner.snapshot(),before);
});

test('parent loops, self parent, equal/reversed slots and unauthenticated projections cannot produce a candidate',async()=>{
 for(const alter of [
  (r,b)=>{if(r===root(4))b.data.message.slot='50';},
  (r,b)=>{if(r===root(2))b.data.message.parent_root=root(4);},
  (r,b)=>{if(r===root(5))b.data.message.parent_root=root(5);},
  (r,b)=>{if(r===root(3))b.finalized=false;},
  (r,b)=>{if(r===root(3))b.execution_optimistic=true;},
  (r,b)=>{if(r===root(2))b.data.message.parent_root=root(0);}
 ]){
  let matched=0;const h=harness({alterBlock:(r,b)=>{alter(r,b);return b;}});await assert.rejects(h.scanner.pollOnce(h.query(()=>{matched++;return {root:root(5)};})),error('history-invalid'));assert.equal(matched,0);assert.equal(h.scanner.snapshot(),null);
 }
 const changed=harness({alterHeader:h=>({...h,data:{...h.data,header:{message:{...h.data.header.message,slot:'51'}}}})});await assert.rejects(changed.scanner.pollOnce(changed.query(()=>({root:root(5)}))),error('history-invalid'));
});

test('declared history byte and slot capacities fail distinctly from a searched deadline with no event',async()=>{
 for(const limits of [{maximumBlocks:3},{maximumBytes:1n},{maximumSpanSlots:20n}]){const h=harness({limits});await assert.rejects(h.scanner.pollOnce(h.query(()=>({root:root(5)}))),error('capacity'));assert.equal(h.scanner.snapshot(),null);}
 const span=harness({limits:{maximumSpanSlots:30n}});await span.scanner.pollOnce(span.query(()=>({root:root(5)})));const before=span.scanner.snapshot();span.setTip(6);await assert.rejects(span.scanner.pollOnce(span.query(()=>({root:root(6)}))),error('capacity'));assert.deepEqual(span.scanner.snapshot(),before);assert.equal((await span.scanner.pollOnce(span.query(()=>({root:root(6)}),{checkpoint:cp(3)}))).root,root(6));
 const h=harness({deadline:5});await assert.rejects(h.scanner.find(h.query(()=>null,{minimumSlot:45n})),error('event-not-observed'));assert.equal(h.time(),5);assert.equal(h.calls.headers,1);assert.deepEqual(h.calls.sleeps,[5]);
});

test('absolute phase deadline refuses late header block callback result and sleep; Promise predicates never pass',async()=>{
 const lateHead=harness({deadline:100,alterHeader:(h,c)=>{c.setTime(100);return h;}});await assert.rejects(lateHead.scanner.pollOnce(lateHead.query(()=>({root:root(5)}))),error('event-not-observed'));assert.deepEqual(lateHead.calls.blocks,[]);
 const lateBlock=harness({deadline:100,alterBlock:(r,b,c)=>{if(r===root(2))c.setTime(100);return b;}});await assert.rejects(lateBlock.scanner.pollOnce(lateBlock.query(()=>({root:root(5)}))),error('event-not-observed'));assert.equal(lateBlock.scanner.snapshot(),null);
 const callback=harness({deadline:100});await assert.rejects(callback.scanner.pollOnce(callback.query(()=>{callback.setTime(100);return {root:root(5)};})),error('event-not-observed'));
 const getter=harness({deadline:100});await assert.rejects(getter.scanner.pollOnce(getter.query(()=>({get root(){getter.setTime(100);return root(5);}}))),error('event-not-observed'));
 const promise=harness();await assert.rejects(promise.scanner.pollOnce(promise.query(async()=>({root:root(5)}))),error('callback-invalid'));
 const oversleep=harness({deadline:10,onSleep:c=>c.setTime(11)});await assert.rejects(oversleep.scanner.find(oversleep.query(()=>null,{minimumSlot:45n})),error('event-not-observed'));
});

function ownerFixture(){
 const signerIdentities=Array.from({length:16},(_,i)=>({keyEnv:'SYNTHETIC_'+i,publicKey:pub(i+1)})),head={execution_optimistic:false,data:{root:root(50),header:{message:{slot:'80',state_root:root(180)}}}};
 const responses=signerIdentities.map((id,i)=>({stateRoot:root(180),publicKey:id.publicKey,response:{execution_optimistic:false,data:{index:String(i),validator:{pubkey:id.publicKey,slashed:false,activation_epoch:'0',exit_epoch:U64String}}}}));
 return {owner:'mac-native',signerIdentities,head,responses,slotsPerEpoch:8n};
}
const U64String=((1n<<64n)-1n).toString();
test('owner selection resolves all sixteen at one sealed state and chooses an actual finalized proposer independently of first key',async()=>{
 const input=ownerFixture(),duties=resolveActiveOwnerDuties(input);assert.equal(duties.length,16);assert.equal(duties[0].index,'0x0');assert(Object.isFrozen(duties));
 const h=harness();h.blocks.get(root(5)).data.message.proposer_index='65';h.blocks.get(root(4)).data.message.proposer_index='11';const selected=await h.scanner.pollOnce(h.query(r=>matchOwnerFinalizedDuty(duties,r,cp(2))));
 assert.equal(selected.index,'0xb');assert.equal(selected.publicKey,pub(12));assert.equal(selected.beforeProposalRoot,root(4));assert.notEqual(selected.publicKey,pub(1));
 const fixed={...selected,owner:'mac-native'},other={root:root(9),slot:90n,message:{proposer_index:'0'}};
 assert.equal(matchFixedOwnerDuty(fixed,other,79n),null);assert.deepEqual(matchFixedOwnerDuty(fixed,{...other,message:{proposer_index:'11'}},79n),{root:root(9),slot:'0x5a'});assert.equal(matchFixedOwnerDuty(fixed,{...other,slot:79n,message:{proposer_index:'11'}},79n),null);
 const current=ownerFixture(),fresh=()=>({head:structuredClone(current.head),stateRoot:root(180),publicKey:pub(12),response:structuredClone(current.responses[11].response),slotsPerEpoch:8n});assert.equal(revalidateOwnerDuty(selected,fresh()).publicKey,pub(12));
 for(const edit of [x=>x.response.data.validator.slashed=true,x=>x.response.data.index='0',x=>x.response.data.validator.pubkey=pub(99),x=>x.response.data.validator.activation_epoch='11',x=>x.response.data.validator.exit_epoch='10',x=>x.stateRoot=root(181)]){const f=fresh();edit(f);assert.throws(()=>revalidateOwnerDuty(selected,f),error('history-invalid'));}
 for(const edit of [x=>x.responses[4].stateRoot=root(9999),x=>x.responses[4].publicKey=pub(88),x=>x.responses[4].response.data.index='0',x=>x.signerIdentities[4].keyEnv=x.signerIdentities[0].keyEnv,x=>x.responses[4].response.execution_optimistic=true]){const f=ownerFixture();edit(f);assert.throws(()=>resolveActiveOwnerDuties(f),error('history-invalid'));}
});

test('strict future duty floor is max stopped head and authenticated journal slot, never the signing root',async()=>{
 assert.equal(ownerProposalCutoff('79','60'),79n);assert.equal(ownerProposalCutoff('79','100'),100n);assert.equal(ownerProposalCutoff('0x4f',null),79n);assert.equal(ownerProposalCutoff('0x4f','0x64'),100n);
 assert.equal(ownerProposalCutoff(79,'60'),79n);assert.equal(ownerProposalCutoff(79,'0x64'),100n);assert.equal(ownerProposalCutoff(0,null),0n);
 for(const bad of [-1,79.1,Number.MAX_SAFE_INTEGER+1,NaN,Infinity])assert.throws(()=>ownerProposalCutoff(bad,'60'),error('history-invalid'));
 for(const missing of [undefined,{},'01','-1'])assert.throws(()=>ownerProposalCutoff('79',missing),error('history-invalid'));
 const source=await readFile(new URL('./run.mjs',import.meta.url),'utf8'),expression='ownerProposalCutoff(source.headSlot,selectedJournal.proposal===null?null:selectedJournal.proposal.slot)';assert.equal(source.split(expression).length,2);
 assert.equal(Function('ownerProposalCutoff','source','selectedJournal','return '+expression)(ownerProposalCutoff,{headSlot:79},{proposal:{slot:'0x64'}}),100n);
});

test('caller integration persists the exact decision before returning to STOP and shares one selection deadline',async()=>{
 const source=await readFile(new URL('./run.mjs',import.meta.url),'utf8'),begin='   selectDuty:async(name,checkpoint)=>{',end='   },restore:restoreMacOwner,finality:';
 assert.equal(source.split(begin).length,2);assert.equal(source.split(end).length,2);const section=source.slice(source.indexOf(begin),source.indexOf(end,source.indexOf(begin)));
 for(const marker of ['const phaseEnd=',"'/eth/v1/beacon/states/'+state+'/validators/'",'resolveActiveOwnerDuties(',"'owner-duty-selection-'",'return selected;'])assert.equal(section.split(marker).length,2);
 assert(section.indexOf('resolveActiveOwnerDuties(')<section.indexOf("'owner-duty-selection-'"));assert(section.indexOf("'owner-duty-selection-'")<section.indexOf('return selected;'));assert(section.includes('phaseEnd)'));assert(!section.includes('break;'));
 assert(source.includes('ownerProposalCutoff(source.headSlot,selectedJournal.proposal===null?null:selectedJournal.proposal.slot)'));assert(source.includes('matchFixedOwnerDuty(duty,x,cutoff)'));
 const restore=source.slice(source.indexOf(' async function restoreMacOwner(name,duty){'),source.indexOf(' async function runMacOwnerRecoveryPhase('));for(const marker of ['ownerProposalCutoff(',"await lifecycleResume(name,before.receipt"])assert.equal(restore.split(marker).length,2);assert(restore.indexOf('ownerProposalCutoff(')<restore.indexOf('await lifecycleResume(name,before.receipt'));
 const body=section.slice(begin.length);
 for(const scenario of ['normal','late-save','inactive']){
  const f=ownerFixture(),h=harness({deadline:100}),events=[];h.blocks.get(root(5)).data.message.proposer_index='65';h.blocks.get(root(4)).data.message.proposer_index='11';let saved,stops=0,headReads=0,scanCalls=0;
  const deps={performance:{now:h.time},spec:{lifecycle:{deadlines:{finalityMs:100}}},lifecycleCheck:()=>{},need:(v,s)=>{if(!v)throw Error(s);},request:async(name,path)=>{assert.equal(name,'mac-native');h.setTime(h.time()+1);if(path==='/eth/v1/beacon/headers/head'){headReads++;const head=structuredClone(f.head);if(headReads===2)head.data.header.message.state_root=root(181);return head;}if(path==='/eth/v1/beacon/states/'+root(181)+'/validators/'+pub(12)){const response=structuredClone(f.responses[11].response);if(scenario==='inactive')response.data.validator.slashed=true;return response;}const row=f.responses.find(e=>path==='/eth/v1/beacon/states/'+e.stateRoot+'/validators/'+e.publicKey);assert(row,'Every initial row uses the same sealed state, final row uses fixed selected key');return row.response;},preparedManifest:{nodes:[{name:'mac-native',signerIdentities:f.signerIdentities}]},slots:8n,resolveActiveOwnerDuties,matchOwnerFinalizedDuty,revalidateOwnerDuty,lifecycleFindFinalized:async(name,checkpoint,minimumSlot,match,ms,phaseDeadline)=>{scanCalls++;assert.equal(ms,100);assert.equal(phaseDeadline,100);return h.scanner.pollOnce({checkpoint,minimumSlot,match,phaseDeadline});},save:async(path,value)=>{assert.equal(stops,0);assert.equal(path,'/synthetic/owner-duty-selection-mac-native.json');saved=structuredClone(value);events.push('decision');if(scenario==='late-save')h.setTime(100);},join:(a,b)=>a+'/'+b,output:'/synthetic'};
  const select=Function(...Object.keys(deps),'return async(name,checkpoint)=>{'+body+'};')(...Object.values(deps));
  const selectThenStop=async()=>{const duty=await select('mac-native',cp(2));events.push('selected');assert(Object.isFrozen(duty));assert(Object.isFrozen(duty.commonCheckpoint));++stops;events.push('stop');return duty;};
  if(scenario==='late-save'){await assert.rejects(selectThenStop(),/Whole owner duty selection deadline/);assert.equal(stops,0);assert.deepEqual(events,['decision']);}
  else if(scenario==='inactive'){await assert.rejects(selectThenStop(),/same unslashed selected key/);assert.equal(stops,0);assert.deepEqual(events,[]);}
  else{const duty=await selectThenStop();assert.equal(duty.publicKey,pub(12));assert.equal(saved.publicKey,duty.publicKey);assert.equal(saved.selectionRecordedBeforeStop,true);assert.equal(saved.selectionRevalidatedStateRoot,root(181));assert.deepEqual(events,['decision','selected','stop']);assert.equal(matchFixedOwnerDuty(duty,{root:root(9),slot:90n,message:{proposer_index:'0'}},79n),null);}
  assert.equal(headReads,2);assert.equal(scanCalls,1,'Invalidation never reselects a different owner key');
 }
 const findStart=' async function lifecycleFindFinalized(',findEnd=' function lifecycleOwnerCopyResult(';for(const marker of [findStart,findEnd])assert.equal(source.split(marker).length,2);const findBody=source.slice(source.indexOf(findStart),source.indexOf(findEnd));assert(findBody.includes('await scanner.find('));assert(findBody.indexOf('await scanner.find(')<findBody.indexOf('deadline after scan'));
 let clock=0;const fd={performance:{now:()=>clock},need:(v,s)=>{if(!v)throw Error(s);},names:['mac-native'],spec:{lifecycle:{deadlines:{finalityMs:100}}},finalizedLifecycleScanners:new Map([['mac-native',{find:async()=>{clock=100;return {root:root(5)};}}]]),lifecycleCheck:()=>{}};
 const actualFind=Function(...Object.keys(fd),findBody+'\nreturn lifecycleFindFinalized;')(...Object.values(fd));await assert.rejects(actualFind('mac-native',cp(2),0n,()=>null,100,100),/deadline after scan/);
 const fStart=' async function finality(baseline,phase,bootstrap,timeoutMs=480000){',fEnd=' async function auditCopy(name){';for(const marker of [fStart,fEnd])assert.equal(source.split(marker).length,2);const common=source.slice(source.indexOf(fStart),source.indexOf(fEnd));
 for(const marker of ['const monotonicUntil=','await refreshFinalizedLifecycleScanners(','phaseDeadline:monotonicUntil',"await save(join(output,phase+"])assert.equal(common.split(marker).length,2);
 assert(common.indexOf('Three finalized executions differ')<common.indexOf('await refreshFinalizedLifecycleScanners('));assert(common.indexOf('await refreshFinalizedLifecycleScanners(')<common.indexOf("await save(join(output,phase+"));assert(common.includes('deadline after scanner refresh'));assert(common.includes('performance.now()<monotonicUntil'));
});
