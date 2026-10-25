// Synthetic filesystem/clock controls; Root executes. No actor/capsule/DB/network.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {constants} from 'node:fs';
import {createHash} from 'node:crypto';
import {describeCloseoutBudget} from './closeout-budget.mjs';
import {createTerminalEvidenceSequence} from './terminal-evidence.mjs';
const source=await readFile(new URL('./run.mjs',import.meta.url),'utf8');
const sha=b=>createHash('sha256').update(b).digest('hex');
const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
const fields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'];
const between=(s,a,b)=>{assert.equal(s.split(a).length,2);const at=s.indexOf(a),end=s.indexOf(b,at);assert(end>at);return s.slice(at,end);};
const code=between(source,' function checkCampaignIo(',' const spec=validateFreshCampaignSpec(');
const signalCode=between(source,'const onSignal=()=>{',';if(campaignBudget){for(const signal')+';';
function fixture({lateAt=null,statError=null,interrupted=null,admissionLate=false,closeout=true,closeError=null,closeFailureAt='all',pauseAt=null}={}){
 const events=[],bytes=Buffer.from('{"public":true}'),held=new Map(),newFiles=new Map(),expiry=Error('original global deadline');
 let clock=0,opened=0,closed=0,closeAttempts=0,admissions=0,release,pauseOnce=false,pausedResolve;const paused=new Promise(resolve=>pausedResolve=resolve);
 const stall=async at=>{if(pauseAt===at&&!pauseOnce){pauseOnce=true;await new Promise(resolve=>{release=resolve;pausedResolve();});}};
 const advance=at=>{if(lateAt===at)clock=100;};
 const row=()=>Object.assign(Object.fromEntries(fields.map((k,i)=>[k,BigInt(k==='size'?bytes.length:k==='nlink'?1:k==='mode'?33152:i+1)])),{isFile:()=>true});
 const material=s=>fields.map(k=>String(s[k]));
 const open=async(file,flags)=>{
  events.push('open:'+file);++opened;advance(flags==='wx'?'write-open':'read-open');
  let offset=0,stats=0;return {
   async stat(){events.push('stat:'+file);advance('stat');if(statError&&stats++===0)throw statError;return row();},
   async read(buffer,at,length){events.push('read:'+file);const n=Math.min(length,bytes.length-offset);bytes.copy(buffer,at,offset,offset+n);offset+=n;advance('read');await stall('read');return {bytesRead:n};},
   async writeFile(value){events.push('write:'+file);assert.deepEqual(value,bytes);advance('write');},
   async sync(){events.push('sync:'+file);advance(file==='/synthetic'?'parent-sync':'file-sync');},
   async close(){events.push('close:'+file);++closeAttempts;advance('close');const kind=file==='/synthetic'?'parent':flags==='wx'?'write-file':'read-file';if(closeError&&(closeFailureAt==='all'||closeFailureAt===kind))throw closeError;++closed;}
  };
 };
 let controller=new AbortController();const budget={phase:closeout?'closeout':'work',get signal(){return controller.signal;},check(){events.push('check');if(clock>=100)throw expiry;},enterCloseout(){controller.abort(Error('original work phase ended'));this.phase='closeout';controller=new AbortController();}};
 const deps={interrupted,closeoutInterrupted:null,campaignBudget:budget,campaignCommands:{cancelAll:reason=>events.push(reason)},
  resolve:p=>p,dirname:p=>p.slice(0,p.lastIndexOf('/')),realpath:async p=>{events.push('realpath');advance('realpath');return p;},
  open,fsConstants:constants,lstat:async()=>{advance('lstat');return row();},held,newFiles,publicExportProbePaths:new Set(),
  need:(ok,m)=>assert(ok,m),inputNeed:(ok,m)=>assert(ok,m),canonical,material,
  materialChanges:(a,b)=>fields.filter((_,i)=>a[i]!==b[i]),rowChanges:()=>[],createHash,Buffer,sha,
  admitNewLifecycleOutput:async(path,pin,creation,options)=>{++admissions;events.push('admit');assert.equal(pin.sha256,sha(bytes));assert.equal(pin.bytes,bytes.length);assert.deepEqual(creation,material(row()));assert.equal(options.isRetained(),false);if(admissionLate)clock=100;assert.equal(options.isRetained(),false);}
 };
 const api=Function(...Object.keys(deps),code+'\n'+signalCode+'\nreturn {checkCampaignIo,observe,save,fence,onSignal,get interruption(){return interrupted;},get closeoutInterruption(){return closeoutInterrupted;}};')(...Object.values(deps));
 return {api,deps,budget,events,bytes,held,newFiles,expiry,paused,release:()=>release(),setClock:n=>clock=n,get opened(){return opened;},get closed(){return closed;},get closeAttempts(){return closeAttempts;},get admissions(){return admissions;}};
}

test('real guarded observe preserves exact SHA/full9/cache predicates and closes successful reads',async()=>{
 const f=fixture(),row=await f.api.observe('/synthetic/input.json');assert.equal(row.sha256,sha(f.bytes));assert.equal(row.bytes,f.bytes.length);assert.equal(f.held.size,1);
 assert.deepEqual(await f.api.observe('/synthetic/input.json',row),row);assert.equal(f.opened,2);assert.equal(f.closed,2);
 await assert.rejects(f.api.observe('/synthetic/input.json',{...row,sha256:'0'.repeat(64)}));assert.equal(f.closed,3);
 assert.equal(f.held.get('/synthetic/input.json'),row,'original authority object is not replaced');
});
test('real observe refuses late realpath/read/stat while every opened FD is closed',async()=>{
 for(const point of ['realpath','read-open','stat','read','lstat','close']){
  const f=fixture({lateAt:point});await assert.rejects(f.api.observe('/synthetic/input.json'),e=>e===f.expiry);
  assert.equal(f.held.size,0,point);assert.equal(f.closed,f.opened,point);
  if(point==='realpath')assert.equal(f.opened,0);
  if(point==='read')assert.equal(f.events.filter(e=>e.startsWith('read:')).length,1,'no second read after expired await');
 }
});
test('real fstat failure after open now closes the descriptor and retains its original error',async()=>{
 const error=Error('original fstat error'),f=fixture({statError:error});await assert.rejects(f.api.observe('/synthetic/input.json'),e=>e===error);
 assert.equal(f.opened,1);assert.equal(f.closed,1);assert.equal(f.held.size,0);
});
test('real save stops after late open/write/sync without further writes or NEW admission',async()=>{
 for(const point of ['write-open','write','file-sync','parent-sync']){
  const f=fixture({lateAt:point});await assert.rejects(f.api.save('/synthetic/new.json',f.bytes),e=>e===f.expiry);
  assert.equal(f.opened,f.closed,point);assert.equal(f.newFiles.size,0);assert.equal(f.admissions,0);
  if(point==='write-open')assert.equal(f.events.filter(e=>e.startsWith('write:')).length,0);
  if(point==='write')assert.equal(f.events.filter(e=>e.startsWith('sync:')).length,0);
 }
});
test('real save callback checks the existing global budget inside NEW sampling without changing retained predicates',async()=>{
 const good=fixture();assert.deepEqual(await good.api.save('/synthetic/new.json',good.bytes),good.bytes);assert.deepEqual(good.newFiles.get('/synthetic/new.json'),{sha256:sha(good.bytes),bytes:good.bytes.length});assert.equal(good.opened,good.closed);
 const late=fixture({admissionLate:true});await assert.rejects(late.api.save('/synthetic/new.json',late.bytes),e=>e===late.expiry);
 assert.equal(late.admissions,1);assert.equal(late.newFiles.size,0);assert.equal(late.opened,late.closed);
});
test('real fence checks an empty expired closure and never proceeds to later rows after late IO',async()=>{
 const empty=fixture();empty.setClock(100);await assert.rejects(empty.api.fence(),e=>e===empty.expiry);assert.equal(empty.opened,0);
 const f=fixture({lateAt:'read'});f.held.set('/synthetic/first.json',{path:'/synthetic/first.json'});f.held.set('/synthetic/second.json',{path:'/synthetic/second.json'});
 await assert.rejects(f.api.fence(),e=>e===f.expiry);assert.equal(f.opened,1);assert.equal(f.closed,1);assert(!f.events.includes('open:/synthetic/second.json'));
 const reason=Error('original interruption'),interrupted=fixture({interrupted:reason,closeout:false});await assert.rejects(interrupted.api.fence(),e=>e===reason);assert.equal(interrupted.opened,0);
});
test('real pre-export caller refuses late planning-profile IO before exporter or additional output',async()=>{
 const start='  let planningParameters=null,planningMaximumBlockBytes=null;',end="  const a=owner('mac-native');const exportIsolated=";
 const body=between(source,start,end),f=fixture({lateAt:'read'});let exports=0,saves=0;
 const spec={lifecycle:{format:'nano-fresh-validator-lifecycle-v2',planningProfile:{path:'/synthetic/profile.json',sha256:sha(f.bytes),bytes:f.bytes.length}},fixture:{seedFile:'/synthetic/seed',ledgerDirectory:'/synthetic/ledger'},clock:{maximumDowntimeMs:'0x1'}};
 const deps={spec,need:(v,m)=>assert(v,m),within:(a,b)=>a===b||a.startsWith(b+'/'),campaignBudget:{workEnd:1000,check:f.api.checkCampaignIo},campaignCloseoutReserve:100,
  observe:f.api.observe,deriveLifecycleParameters:()=>assert.fail('late IO cannot derive profile'),deriveLifecycleCampaignBudget:()=>assert.fail('late IO cannot admit budget'),
  save:async()=>++saves,performance:{now:()=>0},join:(a,b)=>a+'/'+b,output:'/synthetic',sha,exporter:()=>++exports};
 const Async=Object.getPrototypeOf(async function(){}).constructor;await assert.rejects(new Async(...Object.keys(deps),body+'exporter();')(...Object.values(deps)),e=>e===f.expiry);
 assert.equal(exports,0);assert.equal(saves,0);assert.equal(f.opened,f.closed);
 const latePublication=fixture({lateAt:'write'}),reportBody=between(source,"  if(campaignBudget&&spec.lifecycle)await publishCloseoutBudget('before-export');",end),reportDeps={...deps,
  publishCloseoutBudget:async()=>latePublication.api.save('/synthetic/closeout-budget-before-export.json',latePublication.bytes),observe:()=>assert.fail('expired budget publication precedes planning read')};
 await assert.rejects(new Async(...Object.keys(reportDeps),reportBody+'exporter();')(...Object.values(reportDeps)),e=>e===latePublication.expiry);
 assert.equal(exports,0);assert.equal(saves,0);assert.equal(latePublication.newFiles.size,0);assert.equal(latePublication.opened,latePublication.closed);
 const exportReport="await publishCloseoutBudget('before-export')",launchReport="await publishCloseoutBudget('before-launch')",launchGate='const budgetPlan=deriveLifecycleCampaignBudget(await observe';
 for(const marker of [exportReport,launchReport,launchGate,start,"await startDocker('vps-docker',undefined,null,true)"])assert.equal(source.split(marker).length,2,'unique real publication/planning/launch marker');
 assert(source.indexOf(exportReport)<source.indexOf(start));assert(source.indexOf(launchReport)<source.indexOf(launchGate));assert(source.indexOf(launchGate)<source.indexOf("await startDocker('vps-docker',undefined,null,true)"));
});
test('readonly serial report includes growing Linux fanout and does not qualify an arbitrary reserve',()=>{
 const counts={heldFiles:4,newFiles:7,ownedOperations:12,ownerSourceFences:2,lifecycleSourceFences:2,localLifecycleAdmissions:3,lifecycleRemoteIndexes:4,lifecycleRemoteFiles:5};
 const linux={authoritySets:3,nestedFiles:2,volumes:6,authorityFiles:9,authorityPayloadBytes:'0x1000',nestedPayloadBytes:'0x20'};
 const input={phase:'before-launch',reserveMs:2700000,remainingGlobalMs:3000000,counts,linux,resourceBounds:{maximumInventoryFiles:'0x100',maximumInventoryBytes:'0x10000'},signerCounts:{'mac-native':16,'mac-docker':16,'vps-docker':33},macBootstrap:true,macOwnerRecoveryRequested:true};
 const r=describeCloseoutBudget(input);assert.equal(r.currentOneLinuxFenceRequestedCommandMs,'0x55730');assert.equal(r.currentCaptureDatabaseRoles['vps-docker'],'0x44');assert.equal(r.serialRows.find(x=>x.id==='bootstrap-source-fences').requestedCurrentEnvelopeMs,'0x4e200');
 assert.equal(r.serialRows.find(x=>x.id==='terminal-owner-audits').calls,3);assert.equal(r.serialRows.find(x=>x.id==='lifecycle-closing-tools').calls,2);
 assert.equal(r.serialRows.find(x=>x.id==='lifecycle-remote-index-fences').currentCalls,8);assert.equal(r.serialRows.find(x=>x.id==='lifecycle-remote-known-fence').calls,2);
 for(const value of [r,describeCloseoutBudget({...input,reserveMs:1}),describeCloseoutBudget({...input,reserveMs:2000000000})]){assert.equal(value.completeRequiredReserveMs,null);assert.equal(value.admissionComplete,false);assert.equal(value.reserveSufficiencyQualified,false);assert.equal(value.wallClockCompletionGuaranteed,false);assert.equal(value.campaignAuthorized,false);}
 assert.equal(r.observedLinuxInventory.authoritySets,3);assert.equal(r.futureRegistryCountsBounded,false);assert.equal(r.asynchronousFilesystemCallsAbortable,false);
});
test('real terminal sequence retains a late IO error and continues later owned cleanup stages',async()=>{
 const f=fixture({lateAt:'write'}),events=[],sequence=createTerminalEvidenceSequence();
 assert.equal(await sequence.attempt('late-publication',()=>f.api.save('/synthetic/new.json',f.bytes)),false);
 assert.equal(await sequence.attempt('later-owned-stop',async()=>{events.push('stop');}),true);
 assert.equal(sequence.firstFailure,f.expiry);assert.deepEqual(events,['stop']);assert.equal(f.newFiles.size,0);assert.equal(f.opened,f.closed);assert.equal(sequence.report().passed,false);
});
test('real Mac inventory getter is readonly and publishes counts rather than private locators',async()=>{
 const linuxSource=await readFile(new URL('./linux-owned-files.mjs',import.meta.url),'utf8'),getter=between(linuxSource,'  get budgetInventory(){','\n };');
 const authorities=[{v:'ignored-private-volume',root:'/ignored-private-root',receipt:{filesVerified:2,payloadBytes:12}}],
  nested=new Map([['ignored-private-path',{bytes:3}]]),volumes=new Set(['private-a','private-b']),before=JSON.stringify(authorities);
 const value=Function('authorities','nested','volumes','return ({'+getter+'});')(authorities,nested,volumes).budgetInventory;
 assert(Object.isFrozen(value));assert.deepEqual(value,{authoritySets:1,nestedFiles:1,volumes:2,authorityFiles:2,authorityPayloadBytes:'0xc',nestedPayloadBytes:'0x3'});
 assert.equal(JSON.stringify(authorities),before);assert(!JSON.stringify(value).includes('private'));assert.equal(nested.size,1);assert.equal(volumes.size,2);
});
test('real signal dispatch keeps WORK origin aborted while new bounded closeout IO retains FAILED verdict and second signal',async()=>{
 const f=fixture({closeout:false,pauseAt:'read'}),pending=f.api.observe('/synthetic/input.json');await f.paused;
 f.api.onSignal();const first=f.api.interruption,workSignal=f.budget.signal;assert(first instanceof Error);assert(f.events.includes(first));
 f.budget.enterCloseout();f.release();await assert.rejects(pending,e=>e===workSignal.reason);assert.equal(f.held.size,0);assert.equal(f.opened,f.closed);
 await f.api.save('/synthetic/closeout.json',f.bytes);assert.equal(f.newFiles.size,1);assert.equal(f.api.interruption,first);
 const terminal=createTerminalEvidenceSequence({firstFailure:first});await terminal.attempt('bounded-closeout-io',()=>f.api.observe('/synthetic/closeout.json',f.newFiles.get('/synthetic/closeout.json'),false,false));
 assert.equal(terminal.firstFailure,first);assert.equal(terminal.report().passed,false);
 f.api.onSignal();const second=f.api.closeoutInterruption;assert(second instanceof Error&&second!==first);const opened=f.opened;
 await assert.rejects(f.api.observe('/synthetic/after-second.json'),e=>e===second);assert.equal(f.opened,opened);assert.equal(f.api.interruption,first);
 f.setClock(100);await assert.rejects(f.api.save('/synthetic/after-deadline.json',f.bytes),e=>e===second);assert.equal(f.opened,opened);
 const late=fixture();late.setClock(100);await assert.rejects(late.api.save('/synthetic/no-extension.json',late.bytes),e=>e===late.expiry);assert.equal(late.opened,0);
});
test('real FD finally preserves the primary error when close also rejects and never admits late or refused close',async()=>{
 const primary=Error('original fstat failure'),secondary=Error('close refusal'),read=fixture({statError:primary,closeError:secondary});
 await assert.rejects(read.api.observe('/synthetic/input.json'),e=>e===primary);assert.equal(primary.campaignIoCloseFailure,secondary);assert.equal(read.closeAttempts,1);assert.equal(read.closed,0);assert.equal(read.held.size,0);
 for(const [point,kind]of [['write','write-file'],['parent-sync','parent']]){
  const close=Error('secondary close failure'),f=fixture({lateAt:point,closeError:close,closeFailureAt:kind});await assert.rejects(f.api.save('/synthetic/new.json',f.bytes),e=>e===f.expiry);
  assert.equal(f.expiry.campaignIoCloseFailure,close);assert.equal(f.newFiles.size,0);assert.equal(f.admissions,0);assert.equal(f.closeAttempts,f.opened);
 }
 const refused=fixture({closeError:secondary});await assert.rejects(refused.api.observe('/synthetic/input.json'),e=>e===secondary);assert.equal(refused.held.size,0);assert.equal(refused.closeAttempts,1);
});
