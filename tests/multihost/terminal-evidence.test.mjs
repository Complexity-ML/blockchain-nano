// New terminal-controller scenarios only. No run.mjs import, child process,
// RPC, Nano, Docker, SSH, key, journal or private campaign input is exercised.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {join} from 'node:path';
import {createHash} from 'node:crypto';
import {createTerminalEvidenceSequence} from './terminal-evidence.mjs';
import {dockerRuntimeInspectTemplate} from './docker-runtime-restart-guard.mjs';
const AsyncFunction=Object.getPrototypeOf(async function(){}).constructor;
const source=await readFile(new URL('./run.mjs',import.meta.url),'utf8');
const names=['mac-native','mac-docker','vps-docker'];
function uniqueBetween(text,start,end){
 assert.equal(text.split(start).length,2,'One exact source boundary');const offset=text.indexOf(start),until=text.indexOf(end,offset+start.length);assert(until>offset,'Exact source end boundary');return text.slice(offset,until);
}
// The complete actual fresh finally body, not a second model of its ordering.
const terminalBody=uniqueBetween(source,'\n finally{\n  if(campaignBudget){campaignCloseoutStartedAt=','\n }\n if(failure)throw failure;\n}').replace(/^\n finally\{\n/,'');
const fenceDeclaration=uniqueBetween(source,' async function fence(){','\n async function save(');
const canonical=JSON.stringify;
function terminalFixture(options={}){
 const events=[],writes=new Map(),recorded=[],state={lifecycleQualified:false,softwareCalls:0,afterCleanup:false,budgetChecks:0,budgetDisposals:0,removedSignals:[]};
 let clock=100;
 const deadlineError=options.deadlineError??Error('injected C8 deadline'),campaignBudget=options.c8?{phase:'work',overallEnd:10000,enterCloseout(){events.push('budget-closeout');this.phase='closeout';},check(){events.push('budget-check:'+(++state.budgetChecks));if(state.budgetChecks===options.deadlineFailureAt){clock=this.overallEnd;throw deadlineError;}if(clock>=this.overallEnd)throw deadlineError;},dispose(){events.push('budget-dispose');++state.budgetDisposals;}}:null;
 const campaignCommands=options.c8?{async closeAll(){events.push('close-inflight-clients');if(options.commandCloseFailure)throw options.commandCloseFailure;}}:null;
 const error=options.bodyFailure??null,closingError=options.closingError??null;
 const expectedSoftware=[{owner:'mac-native',role:'nano',sha256:'a'.repeat(64),bytes:1}],initialSoftware=options.noInitialSoftware?undefined:structuredClone(expectedSoftware);
 const terminal=names.map(name=>({name,receipt:{owner:name,clockHealthy:true,validator:{healthy:true,failure:null,attestationsAdmitted:1,proposalsAdmitted:1}}}));
 const native={joined:false},tunnel={joined:false},held=new Map([['/synthetic/source',{path:'/synthetic/source',sha256:'b'.repeat(64),bytes:1,material:['1','2','33152','501','20','1','1','100','200']}]]);
 const newFiles=new Map([['/synthetic/output/old.json',{sha256:'c'.repeat(64),bytes:1,material:['1','3','33152','501','20','1','1','100','200']}]]),observed=new Map([...held,...newFiles].map(([path,row])=>[path,structuredClone(row)]));
 const observe=async(path,pin)=>{events.push('observe:'+path);assert.deepEqual(observed.get(path),pin,'Retained full-nine/SHA authority must not refresh');};
 // This ordering fixture has no filesystem waits. The actual I/O checkpoints
 // are extracted and exercised with late awaits in closeout-io.test.mjs.
 const fence=new Function('held','observe','checkCampaignIo','campaignBudget',fenceDeclaration+';return fence;')(held,observe,()=>{},campaignBudget);
 const need=(ok,message)=>{if(!ok)throw Error(message);};
 async function save(path,value){
  const leaf=path.split('/').at(-1);events.push('save:'+leaf);if(options.publicationFailure===leaf)throw options.publicationError;
  if(leaf==='campaign-closeout-started.json')assert.equal(value.specSha256,specSha,'Closeout handoff binds the actual synthetic spec bytes');
  assert.equal(writes.has(path),false,'Terminal outputs are exclusive');writes.set(path,structuredClone(value));
  const pin={sha256:'d'.repeat(64),bytes:1};newFiles.set(path,pin);observed.set(path,structuredClone(pin));return Buffer.from('synthetic only');
 }
 const ownedRuns=[{name:'operation-a',remote:false},{name:'operation-b',remote:true}],uuid='synthetic-domain',label='nano.fresh.campaign='+uuid;let readonlyOperations=0;
 const readonlyOperation=mode=>{if(options.addReadonlyOperations){const name='readonly-'+mode+'-'+readonlyOperations++;ownedRuns.push({name,remote:true});events.push('readonly-create:'+name);}};
 async function docker(args,remote){
  if(args[0]==='ps'&&args[1]==='-a'){
   const name=args[3].slice('name=^/'.length,-1);events.push('cleanup:'+name);state.afterCleanup=true;if(name==='probe-2')state.lastProbeCleaned=true;
   if(options.cleanupFailure&&name==='operation-a')throw options.cleanupFailure;
   if(options.softwareMutation)state.softwareMutated=true;
   if(options.heldMutation)observed.get('/synthetic/source').material[8]='201';
   if(options.outputMutation)observed.get('/synthetic/output/old.json').material[8]='201';
   return options.runningContainer===name?name:'';
  }
  if(args[0]==='inspect'){assert.equal(args.length,4);assert.deepEqual(args.slice(2),['--format',dockerRuntimeInspectTemplate],'Same exact bounded projection as the actual runner');return JSON.stringify({Name:'/'+args[1],Image:options.wrongCleanupImage?'foreign-image':'image-'+(remote?'vps-docker':'mac-docker'),Config:{Labels:{'nano.fresh.campaign':uuid}},State:{Running:true}});}
  if(args[0]==='stop'){events.push('container-stop:'+args.at(-1));return '';}
  if(args[0]==='ps'){events.push('absence:'+(remote?'vps':'mac'));return options.activeAtEnd?'still-running':'';}
  throw Error('Unexpected synthetic Docker adapter request');
 }
 const fixtureSpec={lifecycle:{deadlines:{maximumRunMs:10000}},partitionRotation:options.partitionRequested??false,backupRestore:options.backupRequested??false,overlayManifest:{sha256:'e'.repeat(64)}};
 const specSha=createHash('sha256').update(Buffer.from(JSON.stringify(fixtureSpec))).digest('hex');
 const deps={
  createTerminalEvidenceSequence,failure:error,bodyFailureRecordError:options.bodyFailureRecordError,
  spec:fixtureSpec,specSha,dockerRuntimeInspectTemplate,
  campaignBudget,campaignCommands,campaignCloseoutStartedAt:null,campaignStartedAt:0,campaignCloseoutReserve:1000,performance:{now:()=>clock},
  nativeLoader:options.c8?{}:null,macOwnerRecovery:options.c8?{passed:true}:null,ownerRecoveryRecords:new Map(options.c8?names.slice(0,2).map(name=>[name,{}]):[]),
  ownerRecoverySourceFences:options.c8?names.slice(0,2).map(name=>({name,verifySource:async()=>{events.push('owner-source:'+name);if(options.ownerSourceFailure===name)throw options.ownerSourceError;}})):[],
  auditOwnerRecoveryTerminal:async()=>{events.push('owner-recovery-audit');if(options.ownerAuditFailure)throw options.ownerAuditFailure;},
  started:new Set(names),names,stopped:async name=>{events.push('stop:'+name);if(options.stopFailure===name)throw Error('injected stop failure');return terminal.find(row=>row.name===name);},
  lifecycleStopReceipts:{collect(){events.push('collect-stops');return terminal.filter(row=>options.staleStoppedReceipt||row.name!==options.stopFailure);}},lifecycleStopRootBinding:name=>'synthetic roots '+name,
  native,tunnel,terminateJoined:async(child,name)=>{events.push('join:'+name);if(options.joinFailure===name)throw options.joinError;child.joined=true;if(options.throwAfterClose===name)throw options.joinError;return options.forcedJoin===name;},childAbsent:child=>!child||child.joined,need,
  lifecycleTerminalCaptures:new Map(),lifecycleCapture:async(name)=>{
   events.push('capture:'+name);assert(native.joined&&tunnel.joined,'Captures only after both actual join adapters');if(options.captureFailure===name)throw options.captureError;
   await save('/synthetic/output/commitment-'+name+'.json',{owner:name,productionQualified:false});return {publicCapture:{owner:name,productionQualified:false}};
  },capturePair:async()=>assert.fail('Lifecycle fixture cannot use legacy capture adapter'),auditFreshRotation:async()=>{events.push('journal-audit:rotation');assert.fail('No unsafe journal audit');},readFile:async()=>{events.push('journal-read:backup');assert.fail('No unsafe journal read');},
  ownedRuntime:async(name,running)=>{events.push('owned-state:'+name);assert.equal(running,false);if(options.containerStillRunning===name)throw Error('Owner still running');return {State:{Running:false}};},
  lifecycleStoppedCapturesReceipt(rows,{campaignId}){assert.equal(campaignId,'f'.repeat(64));assert.deepEqual(rows.map(row=>row.owner).sort(),[...names].sort(),'No missing/duplicate capture promoted');return {owners:rows,captureOnly:true,lifecycleTerminalQualified:false,productionQualified:false};},
  manifest:{campaignId:'f'.repeat(64)},save,output:'/synthetic/output',join,
  lifecycleSourceFences:[0,1].map(i=>({prefix:'source-'+i,pin:String(i).repeat(64),argv:pin=>['synthetic',pin]})),
  lifecycleRemote:async request=>{
   events.push('remote:'+request.mode+':'+(request.prefix??request.admission?.id??'known'));
   readonlyOperation(request.mode);
   if(request.mode==='original-source-fence')return {format:'nano-fresh-lifecycle-original-source-fence-v1',sourceNineFieldAndShaUnchanged:!((options.sourceMutation||options.sourceMutationAfterLateProbeCleanup&&state.lastProbeCleaned)&&request.prefix.startsWith('source-0')),sourceNamespaceUnchanged:true,copyProofSha256:request.argv[1],resumedCopiesExpectedToChange:true,resumedCopiesRechecked:false,rawInventoryExported:false,productionQualified:false};
   if(request.mode==='fence-known'&&options.knownFilesMutationAfterProbe&&state.softwareCalls>=2)throw options.knownFilesError;
   assert(['fence-index','fence-known'].includes(request.mode));return {passed:true};
  },lifecycleArtifacts:new Map([[0,{admission:{verify:async()=>{events.push('artifact:0');readonlyOperation('artifact');}}}],[1,{admission:{verify:async()=>{events.push('artifact:1');readonlyOperation('artifact');}}}]]),lifecycleRemoteIndexes:[{id:0},{id:1}],lifecycleRemoteFiles:[],
  macLinux:{verifyAll:async()=>{events.push('linux-fence');}},macBootstrap:{fenceSource:async()=>{events.push('bootstrap-fence');}},initialSoftware,
  software:async()=>{++state.softwareCalls;events.push('software:'+state.softwareCalls);if(options.addProbeOperations)ownedRuns.push({name:'probe-'+state.softwareCalls,remote:false});return state.softwareMutated?[{...expectedSoftware[0],sha256:'0'.repeat(64)}]:structuredClone(expectedSoftware);},canonical,fence,newFiles,observe,ownedRuns,docker,
  owner:name=>({image:'image-'+name}),uuid,label,partitionNetworkCreated:false,partitionNetworkAbsent:true,partitionNetwork:'unused-network',containers:{'vps-docker':'unused-vps'},
  interrupted:options.interrupted,process:{removeListener:signal=>{events.push('remove-listener');state.removedSignals.push(signal);}},onSignal:()=>{},lifecycleWholeTimer:null,clearTimeout:()=>assert.fail('No timer created'),revision:'1'.repeat(40),
  record:async row=>{events.push('record:'+row.stage);recorded.push(row);if(options.recordFailure)throw options.recordFailure;},state,
  close:async()=>{events.push('closing');if(closingError)throw closingError;state.lifecycleQualified=!options.incompleteLifecycle;}
 };
 const execute=new AsyncFunction(...Object.keys(deps),"let publicCheckStage;let lifecycleQualified=false;const lifecycleClosing=async(...args)=>{await close(...args);lifecycleQualified=state.lifecycleQualified;};"+terminalBody+'\nreturn {failure,report:closeout.report(),failures:closeout.failures};');
 return {events,writes,recorded,state,held,newFiles,observed,run:()=>execute(...Object.values(deps)),result:()=>writes.get('/synthetic/output/result.json')};
}
const failedStages=result=>result.report.stages.filter(row=>!row.passed&&row.phase==='run').map(row=>row.stage);

test('terminal sequence preserves first error and separates recorder failures without skipping later checks',async()=>{
 const first=Error('first'),second=Error('second'),recorder=Error('recorder'),events=[];
 const sequence=createTerminalEvidenceSequence({firstFailure:first,recordFailure:async({stage,error})=>{events.push([stage,error]);throw recorder;}});
 await sequence.attempt('closing',async()=>{throw second;});await sequence.attempt('final-fence',async()=>{events.push('final-fence');});
 assert.equal(sequence.firstFailure,first);assert.deepEqual(sequence.failures.map(row=>row.error),[second,recorder]);assert.equal(sequence.report().passed,false);
 assert.deepEqual(sequence.report().stages,[{stage:'closing',phase:'run',passed:false},{stage:'closing',phase:'failure-record',passed:false},{stage:'final-fence',phase:'run',passed:true}]);assert.equal(events.at(-1),'final-fence');
 const copy=sequence.report();copy.firstFailure.stage='altered';copy.stages[0].passed=true;sequence.failures[0].stage='altered';assert.equal(sequence.report().firstFailure.stage,'campaign-body');assert.equal(sequence.report().stages[0].passed,false);assert.equal(sequence.failures[0].stage,'closing');assert.equal(JSON.stringify(sequence.report()).includes('recorder'),false);
});
test('terminal sequence refuses duplicate, invalid and concurrent stage execution before callback',async()=>{
 const sequence=createTerminalEvidenceSequence();let release,forbidden=0;const waiting=sequence.attempt('join',()=>new Promise(resolve=>{release=resolve;}));
 await assert.rejects(sequence.attempt('other',async()=>{++forbidden;}),/serial/);release();await waiting;
 await assert.rejects(sequence.attempt('join',async()=>{++forbidden;}),/unique/);await assert.rejects(sequence.attempt('../private',async()=>{++forbidden;}),/unique/);assert.equal(forbidden,0);
 const thrown=createTerminalEvidenceSequence();await thrown.attempt('falsey-throw',async()=>{throw null;});assert(thrown.firstFailure instanceof Error);assert.equal(thrown.firstFailure.cause,null);assert.equal(thrown.report().passed,false);
});
test('actual finally closes handles before captures, commits before replay, and fences after owned cleanup',async()=>{
 const f=terminalFixture(),result=await f.run();assert.equal(result.failure,null);assert.equal(result.report.passed,true);assert.equal(f.result().freshSyntheticLifecycleQualified,true);assert.equal(f.result().productionQualified,false);
 assert(f.events.indexOf('join:Tunnel')<f.events.indexOf('capture:mac-native'));assert(f.events.indexOf('save:lifecycle-stopped-captures-public.json')<f.events.indexOf('closing'));
 assert(f.events.indexOf('cleanup:operation-b')<f.events.findIndex(x=>x.startsWith('remote:original-source-fence')));assert(f.events.indexOf('cleanup:operation-b')<f.events.indexOf('software:2'));assert(f.events.indexOf('software:2')<f.events.indexOf('absence:mac'));
 assert.equal(f.state.softwareCalls,2);assert.equal(f.writes.get('/synthetic/output/lifecycle-stopped-captures-public.json').lifecycleTerminalQualified,false);assert(f.events.lastIndexOf('observe:/synthetic/output/terminal-evidence-stages.json')>f.events.indexOf('save:terminal-evidence-stages.json'));
});
test('actual finally records incomplete lifecycle independently while all terminal fences still execute',async()=>{
 const first=Error('Completed live stages and three stopped owners'),f=terminalFixture({closingError:first}),result=await f.run();
 assert.equal(result.failure,first);assert(failedStages(result).includes('lifecycle-closing'));assert.equal(f.state.softwareCalls,2);assert(f.events.includes('cleanup:operation-a'));assert(f.events.includes('remote:fence-index:1'));assert(f.events.includes('artifact:1'));assert(f.events.includes('absence:vps'));assert(f.result());assert.equal(f.result().passed,false);assert.equal(f.result().freshSyntheticLifecycleQualified,false);
 const incomplete=terminalFixture({incompleteLifecycle:true}),refused=await incomplete.run();assert.match(refused.failure.message,/complete terminal replay/);assert.equal(incomplete.result().passed,false);
});
test('actual finally retains body error through secondary cleanup, software and recording failures',async()=>{
 const first=Error('original body'),cleanup=Error('cleanup'),record=Error('recorder'),f=terminalFixture({bodyFailure:first,closingError:Error('incomplete stages'),cleanupFailure:cleanup,softwareMutation:true,recordFailure:record}),result=await f.run();
 assert.equal(result.failure,first);assert(result.failures.some(row=>row.error===cleanup));assert(result.failures.some(row=>row.error===record&&row.phase==='failure-record'));assert(failedStages(result).includes('post-cleanup-software-fence'));assert(f.events.includes('cleanup:operation-b'));assert(f.events.includes('absence:vps'));assert.equal(f.result().passed,false);
});
test('actual finally never captures after an unjoined child and still attempts cleanup and fences',async()=>{
 const first=Error('join refused'),f=terminalFixture({joinFailure:'Native runtime',joinError:first}),result=await f.run();assert.equal(result.failure,first);assert.equal(f.events.some(x=>x.startsWith('capture:')),false);assert.equal(f.events.includes('closing'),false);assert.equal(f.events.includes('cleanup:operation-b'),true);assert.equal(f.state.softwareCalls,2);assert(failedStages(result).includes('post-cleanup-process-join'));assert.equal(f.result().passed,false);
});
test('actual finally requires both successful clean joins even when a forced or throwing join already closed its child',async()=>{
 for(const name of ['Native runtime','Tunnel'])for(const failure of ['forcedJoin','throwAfterClose']){
  const joinError=Error('join failed after close'),f=terminalFixture({[failure]:name,joinError,partitionRequested:true,backupRequested:true}),result=await f.run();
  assert.equal(f.events.some(x=>x.startsWith('capture:')),false);assert.equal(f.events.some(x=>x.startsWith('journal-')),false);assert.equal(f.events.includes('closing'),false);assert(failedStages(result).includes(name==='Tunnel'?'join-tunnel':'join-native-runtime'));if(failure==='throwAfterClose')assert.equal(result.failure,joinError);else assert.match(result.failure.message,/SIGKILL/);assert(f.events.includes('cleanup:operation-b'));assert.equal(f.state.softwareCalls,2);assert.equal(f.result().passed,false);assert.equal(f.result().freshSyntheticLifecycleQualified,false);
 }
});
test('actual finally refuses stale stopped receipts and journal audits when an owner stop or stopped-state check fails',async()=>{
 for(const selected of [{stopFailure:'vps-docker',staleStoppedReceipt:true},{containerStillRunning:'vps-docker'}]){
  const f=terminalFixture({...selected,partitionRequested:true,backupRequested:true}),result=await f.run();assert.equal(f.events.includes('capture:vps-docker'),false);assert.equal(f.events.includes('closing'),false);assert(failedStages(result).includes('capture-vps-docker'));assert(failedStages(result).includes('audit-partition-rotation'));assert(failedStages(result).includes('audit-restored-finality'));assert(f.events.includes('cleanup:operation-b'));assert.equal(f.state.softwareCalls,2);assert.equal(f.result().passed,false);assert.equal(f.result().freshSyntheticLifecycleQualified,false);
 }
});
test('actual finally fences original sources and all retained families after the final software probes and their cleanup',async()=>{
 const f=terminalFixture({addProbeOperations:true,addReadonlyOperations:true}),result=await f.run();assert.equal(result.report.passed,true);const lastProbe=f.events.indexOf('cleanup:probe-2');assert(lastProbe>f.events.indexOf('software:2'));
 for(const [i,event] of f.events.entries())if(event.startsWith('remote:')||event.startsWith('artifact:'))assert(i>lastProbe,'Every retained fence family follows the last software probe cleanup');
 const lastFamily=Math.max(...f.events.map((event,i)=>event.startsWith('remote:')||event.startsWith('artifact:')?i:-1));const readonlyCleanups=f.events.filter(event=>event.startsWith('cleanup:readonly-'));assert.equal(readonlyCleanups.length,7);for(const event of readonlyCleanups)assert(f.events.indexOf(event)>lastFamily);assert.equal(f.state.softwareCalls,2,'No software replay after readonly fences');assert(f.events.indexOf('absence:mac')>f.events.lastIndexOf(readonlyCleanups.at(-1)));
 const changed=terminalFixture({addProbeOperations:true,sourceMutationAfterLateProbeCleanup:true}),refused=await changed.run();assert(failedStages(refused).includes('original-source-fence-0'));assert.equal(changed.result().passed,false);
 const knownFilesError=Error('known files changed by last probe'),known=terminalFixture({addProbeOperations:true,knownFilesMutationAfterProbe:true,knownFilesError}),k=await known.run();assert.equal(k.failure,knownFilesError);assert(failedStages(k).includes('lifecycle-known-files-fence'));
});
test('actual finally cleans only new readonly fence operations under owner guards and treats a late stop as failure',async()=>{
 for(const wrongCleanupImage of [false,true]){
  const firstName='readonly-original-source-fence-0',f=terminalFixture({addProbeOperations:true,addReadonlyOperations:true,runningContainer:firstName,wrongCleanupImage}),result=await f.run();assert.equal(f.result().passed,false);assert(failedStages(result).some(stage=>stage.startsWith('readonly-fence-cleanup-')));assert.equal(f.events.includes('container-stop:'+firstName),!wrongCleanupImage);assert(f.events.includes('cleanup:readonly-fence-known-6'));assert.equal(f.state.softwareCalls,2);assert.equal(f.events.filter(event=>event==='cleanup:operation-a').length,1,'Old operation must not be cleaned again in the readonly-only pass');assert(f.events.includes('absence:vps'));
 }
});
test('actual finally keeps individual owner commitments if one capture fails, without closing an incomplete set',async()=>{
 const first=Error('capture refused'),f=terminalFixture({captureFailure:'mac-native',captureError:first}),result=await f.run();assert.equal(result.failure,first);assert(f.writes.has('/synthetic/output/commitment-mac-docker.json'));assert(f.writes.has('/synthetic/output/commitment-vps-docker.json'));assert.equal(f.events.includes('closing'),false);assert(failedStages(result).includes('publish-stopped-captures'));assert.equal(f.state.softwareCalls,2);assert.equal(f.result().freshSyntheticLifecycleQualified,false);
});
test('actual finally refuses post-cleanup software, retained source and retained output mutation separately',async()=>{
 for(const [option,stage] of [['softwareMutation','post-cleanup-software-fence'],['heldMutation','post-cleanup-held-fence'],['outputMutation','post-cleanup-new-output-fence'],['sourceMutation','original-source-fence-0']]){
  const f=terminalFixture({[option]:true}),before=structuredClone([...f.held]),result=await f.run();assert(failedStages(result).includes(stage),option);assert.equal(f.result().passed,false);assert.deepEqual([...f.held],before,'Original authority remains immutable');assert(f.events.includes('remote:original-source-fence:source-1-original-final-fence'));assert(f.events.includes('artifact:1'));assert(f.events.includes('remote:fence-index:1'));assert(f.events.includes('absence:vps'));
 }
});
test('actual finally refuses absent initial admission and enforces owned image before stopping a late container',async()=>{
 const absent=terminalFixture({noInitialSoftware:true}),a=await absent.run();assert(failedStages(a).includes('pre-cleanup-software-fence'));assert(failedStages(a).includes('post-cleanup-software-fence'));assert.equal(absent.state.softwareCalls,0);
 const foreign=terminalFixture({runningContainer:'operation-a',wrongCleanupImage:true}),b=await foreign.run();assert.match(b.failure.message,/owner image\/label/);assert.equal(foreign.events.some(x=>x.startsWith('container-stop:')),false);assert(foreign.events.includes('cleanup:operation-b'));assert.equal(foreign.result().passed,false);
});
test('actual finally retains original failure when terminal publication itself fails',async()=>{
 for(const leaf of ['lifecycle-stopped-captures-public.json','terminal-evidence-stages.json','result.json']){
  const first=Error('body failure'),publicationError=Error('publication '+leaf),f=terminalFixture({bodyFailure:first,publicationFailure:leaf,publicationError}),result=await f.run();assert.equal(result.failure,first);assert(result.failures.some(row=>row.error===publicationError));assert(f.events.includes('absence:vps'));if(leaf==='lifecycle-stopped-captures-public.json')assert.equal(f.events.includes('closing'),false);if(f.result())assert.equal(f.result().passed,false);
 }
});

test('C8 actual finally joins clients before runtime cleanup and publishes only a provisional result',async()=>{
 const f=terminalFixture({c8:true}),result=await f.run(),published=f.result();
 assert.equal(result.failure,null);assert.equal(result.report.passed,true);
 assert.equal(f.events.filter(event=>event==='budget-closeout').length,1);
 assert.equal(f.events.filter(event=>event==='close-inflight-clients').length,1);
 assert(f.events.indexOf('budget-closeout')<f.events.indexOf('close-inflight-clients'));
 assert(f.events.indexOf('close-inflight-clients')<f.events.indexOf('stop:mac-native'));
 assert(f.events.indexOf('owner-recovery-audit')>f.events.indexOf('closing'));
 assert.equal(published.passed,false);assert.equal(published.terminalEvidenceReady,true);assert.equal(published.terminalVerdictProvisional,true);assert.equal(published.effectiveTerminalExitRequired,true);
 assert.equal(published.freshSyntheticLifecycleQualified,false);assert.equal(published.threeOwnerRestorationRequested,true);assert.equal(published.threeOwnerRestorationQualified,false);assert.equal(published.productionQualified,false);
 for(const name of names.slice(0,2))assert.equal(f.events.filter(event=>event==='owner-source:'+name).length,2,'Both original-source fence stages remain present');
 assert.equal(f.state.budgetChecks,3);assert.equal(f.state.budgetDisposals,1);assert.deepEqual(f.state.removedSignals,['SIGINT','SIGTERM','SIGHUP']);
 assert.equal(f.events.at(-4),'budget-dispose');
 const duration=f.writes.get('/synthetic/output/campaign-deadline-result.json');assert.equal(duration.globalDeadlineExceeded,false);assert.equal(duration.forcedDeadlineExtensionAuthorized,false);assert.equal(duration.campaignAuthorizedByThisReport,false);
});
test('C8 actual finally preserves an effective late deadline failure after a ready provisional publication',async()=>{
 const first=Error('deadline after provisional publication'),f=terminalFixture({c8:true,deadlineFailureAt:2,deadlineError:first}),result=await f.run();
 assert.equal(f.result().terminalEvidenceReady,true,'The prior ready snapshot is deliberately not a terminal verdict');assert.equal(f.result().passed,false);assert.equal(f.result().freshSyntheticLifecycleQualified,false);assert.equal(f.result().threeOwnerRestorationQualified,false);
 assert.equal(result.failure,first);assert.equal(result.report.passed,false);assert(failedStages(result).includes('global-deadline-after-publication'));assert(failedStages(result).includes('global-final-deadline-check'));
 assert(f.events.indexOf('save:result.json')<f.events.indexOf('budget-check:2'));assert.equal(f.writes.get('/synthetic/output/campaign-deadline-result.json').globalDeadlineExceeded,true);
 assert.equal(f.state.budgetDisposals,1);assert.deepEqual(f.state.removedSignals,['SIGINT','SIGTERM','SIGHUP']);
});
test('C8 actual finally retains a client-join failure while stopping owners and attempting all original fences',async()=>{
 const first=Error('owned command join failed'),sourceError=Error('original Native source changed'),f=terminalFixture({c8:true,commandCloseFailure:first,ownerSourceFailure:'mac-native',ownerSourceError:sourceError,addProbeOperations:true,addReadonlyOperations:true}),result=await f.run();
 assert.equal(result.failure,first);assert(result.failures.some(row=>row.error===sourceError));assert(failedStages(result).includes('join-owned-inflight-clients'));
 for(const name of names)assert(f.events.includes('stop:'+name));
 assert(f.events.includes('cleanup:operation-b'));assert(f.events.includes('cleanup:probe-2'));assert(f.events.includes('remote:fence-index:1'));assert(f.events.includes('artifact:1'));assert(f.events.includes('absence:mac'));assert(f.events.includes('absence:vps'));
 for(const name of names.slice(0,2))assert.equal(f.events.filter(event=>event==='owner-source:'+name).length,2,'A failed source cannot skip the other owner or the later source fence');
 assert.equal(f.result().passed,false);assert.equal(f.result().terminalEvidenceReady,false);assert.equal(f.result().threeOwnerRestorationQualified,false);assert.equal(f.state.budgetDisposals,1);
});
