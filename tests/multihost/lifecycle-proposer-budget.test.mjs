// Pure/synthetic controls only. No chain, signer, DB, Docker or SSH is started.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {validateLifecycleMode,lifecycleProposalMs,lifecycleSequentialPhaseBudget,deriveLifecycleParameters,deriveLifecycleCampaignBudget} from './lifecycle.mjs';
import {minimumMacOwnerRecoveryMs} from './owner-recovery.mjs';
import {observeLifecycleProposal} from './lifecycle-runtime.mjs';
const q=n=>'0x'+BigInt(n).toString(16),root=n=>'0x'+BigInt(n).toString(16).padStart(64,'0'),pub=n=>'0x'+BigInt(n).toString(16).padStart(96,'0');
const bytes=v=>Buffer.from(JSON.stringify(v)),sha=b=>createHash('sha256').update(b).digest('hex');
function profile(){const schedule=[q(2),q(32),q(4),q(256)];return {format:'nano-capella-profile-v1',common:[{exit:[...schedule,q(64)],registry:[...schedule,q(32000000000),q(16000000000)],slashing:[...schedule,q(64),q(64),q(512),q(8),q(1000000000),q(3)],ffg:[q(8),q(1000000000)],epochSync:[q(8),q(8),q(1),q(32000000000),q(10)],executionLimits:[q(2048)]},q(6000),q(4000),q(40),q(0),q(1337),q(0),q(6),q(8388608),q(32),q(1048576),q(1024)],withdrawals:[q(32000000000),q(4),q(16)],genesisVersion:'0x00000000',maximumBlsChanges:q(4)};}
function mode(v2=false){const raw=bytes(profile());return {format:v2?'nano-fresh-validator-lifecycle-v2':'nano-fresh-validator-lifecycle-v1',identities:{voluntaryExit:{owner:'vps-docker',keyEnv:'NANO_TEST_CAPELLA_NETWORK_KEY_64',publicKey:pub(65)},slashing:{owner:'mac-native',keyEnv:'NANO_TEST_CAPELLA_NETWORK_KEY_0',publicKey:pub(1)}},runtimeDriverEnabled:true,privateVpsLifecycleTransferAuthorized:true,crossOwnerEvidenceTransferAuthorized:true,syntheticActionAuthorized:true,intentionallyAuthorConflictingHeaders:true,deadlines:{registrationMs:100000,activationMs:100000,finalityMs:10000,restoreMs:10000,actionMs:30000,auditMs:30000,maximumEpochsAfterActivation:400,maximumRunMs:v2?22000000:20000000,...(v2?{proposerMs:40000}:{})},resources:{maximumWallSeconds:q(30),maximumHistoryBlocks:q(5000),maximumArchiveBytes:q(20000000),maximumIndexBytes:q(600000),maximumInventoryFiles:q(10000),maximumInventoryBytes:q(50000000)},...(v2?{planningProfile:{path:'/synthetic/profile.json',sha256:sha(raw),bytes:raw.length}}:{})};}
const planning=(m,extra={})=>deriveLifecycleCampaignBudget(bytes(profile()),m,{maximumBootstrapAgeMs:60000,closeoutReserveMs:100000,remainingWorkMs:m.deadlines.maximumRunMs-100000,...extra});

test('legacy and explicit V2 windows are distinct exact schemas',()=>{
 assert.equal(lifecycleProposalMs(mode()),10000);assert.equal(lifecycleProposalMs(mode(true)),40000);
 for(const change of [m=>delete m.deadlines.proposerMs,m=>m.deadlines.proposerMs=0,m=>m.deadlines.proposerMs=NaN,m=>m.deadlines.proposerMs=m.deadlines.maximumRunMs+1,m=>delete m.planningProfile,m=>m.planningProfile.sha256='bad',m=>m.planningProfile.seed='not permitted']){const m=mode(true);change(m);assert.throws(()=>validateLifecycleMode(m));}
 const legacy=mode();legacy.deadlines.proposerMs=40000;assert.throws(()=>validateLifecycleMode(legacy));
 const v2=mode(true);v2.privateVpsLifecycleTransferAuthorized=false;assert.equal(validateLifecycleMode(v2).privateVpsLifecycleTransferAuthorized,false,'budget schema never grants transfer authority');
});

test('actual sequential stages charge ten F plus four P, and both Mac recoveries separately',()=>{
 const legacy=mode(),phases=lifecycleSequentialPhaseBudget(legacy),parameters=deriveLifecycleParameters(bytes(profile()),legacy);
 assert.equal(phases.lifecycle,510000n);assert.equal(parameters.minimumWholeRunMs,q(19710000));assert.equal(phases.mac,80000n);assert.equal(minimumMacOwnerRecoveryMs(legacy.deadlines),phases.mac);
 const future=mode(true),p=lifecycleSequentialPhaseBudget(future);assert.equal(p.lifecycle-phases.lifecycle,4n*30000n);assert.equal(p.mac-phases.mac,4n*30000n);assert.equal(minimumMacOwnerRecoveryMs(future.deadlines),p.mac);
 const report=planning(future);assert.equal(report.minimumWorkMs,q(20040000));assert.equal(report.conservativeEpochHorizon,q(337));assert.equal(report.proposerMs,40000);assert.equal(report.finalityMs,10000);assert.equal(report.phaseCounts.lifecycleCommonFinality,6);assert.equal(report.phaseCounts.operationSearches,2);assert.equal(report.phaseCounts.withdrawalSearches,2);assert.equal(report.phaseCounts.initialCommonFinality,1);assert.equal(report.productionQualified,false);
 future.deadlines.maximumRunMs=20000000;assert.throws(()=>planning(future),/remaining global work/,'longer event windows must fit a newly declared total plus reserve');
});

test('global reserve, real remaining work, ancestry and archival horizon cannot fund each other',()=>{
 const m=mode(true),r=planning(m);assert.equal(r.archiveWorstCaseBytes,q(5000n*2160n));assert.equal(r.minimumIndexBytes,q(5000n*112n));
 assert.throws(()=>planning(m,{remainingWorkMs:Number(BigInt(r.minimumWorkMs))-1}),/remaining global work/);
 assert.throws(()=>planning(m,{closeoutReserveMs:m.deadlines.maximumRunMs}),/closeout reserve/);
 for(const change of [v=>v.resources.maximumHistoryBlocks=q(10),v=>v.resources.maximumIndexBytes=q(100),v=>v.resources.maximumArchiveBytes=q(5000n*2160n-1n)]){const v=mode(true);change(v);assert.throws(()=>planning(v));}
 const shortHorizon=mode(true);shortHorizon.deadlines.maximumEpochsAfterActivation=325;assert.throws(()=>planning(shortHorizon),/epoch horizon/);
 const long=mode(true);long.deadlines.maximumRunMs=100000000;long.deadlines.maximumEpochsAfterActivation=1000;long.deadlines.proposerMs=6200000;long.resources.maximumHistoryBlocks=q(20000);long.resources.maximumIndexBytes=q(3000000);long.resources.maximumArchiveBytes=q(100000000);assert.throws(()=>planning(long),/ancestry horizon/);
});

test('actual proposal wrapper clamps once to work end and rejects late, stale, reselected or beyond-horizon answers',async()=>{
 const make=({end=349,slot=100,index='0x40'}={})=>{let clock=100,calls=0;return {args:{ops:{check:()=>{},workDeadline:()=>350,finalizedProposal:async(id,resolved,checkpoint,after,ms,until)=>{++calls;assert.equal(id.publicKey,pub(65));assert.equal(resolved,'0x40');assert.equal(after,79n);assert.equal(ms,250);assert.equal(until,350);clock=end;return {root:root(9),slot:q(slot),resolvedIndex:index};}},id:{publicKey:pub(65)},index:'0x40',checkpoint:{epoch:'0x8',root:root(8)},after:79n,ms:500,wholeDeadline:1000,activationEpoch:5n,slotsPerEpoch:8n,maximumEpochs:20n,now:()=>clock},calls:()=>calls};};
 const good=make();assert.equal((await observeLifecycleProposal(good.args)).slot,q(100));assert.equal(good.calls(),1);
 for(const config of [{end:350},{end:99},{slot:79},{slot:208},{index:'0x3f'}]){const f=make(config);await assert.rejects(observeLifecycleProposal(f.args));assert.equal(f.calls(),1,'never switch key or retry a timed-out phase');}
});

test('real pre-export caller refuses insufficient V2 recipe before exporter or seed path can run',async()=>{
 const source=readFileSync(new URL('./run.mjs',import.meta.url),'utf8'),start='  let planningParameters=null,planningMaximumBlockBytes=null;',end="  const a=owner('mac-native');const exportIsolated=";
 assert.equal(source.split(start).length,2);assert.equal(source.split(end).length,2);const a=source.indexOf(start),b=source.indexOf(end,a);assert(b>a);const actual=source.slice(a,b),Async=Object.getPrototypeOf(async function(){}).constructor;
 for(const insufficient of [false,true]){let exportCalls=0,saves=0;const m=mode(true),raw=bytes(profile()),deps={spec:{lifecycle:m,clock:{maximumDowntimeMs:q(60000)},fixture:{seedFile:'/synthetic/seed.private',ledgerDirectory:'/synthetic/ledger'}},within:(a,b)=>a===b||a.startsWith(b+'/'),campaignBudget:{workEnd:insufficient?20039999:21900000,check:()=>{}},campaignCloseoutReserve:100000,performance:{now:()=>0},need:(ok,message)=>assert(ok,message),observe:async(file,pin)=>{assert.equal(file,m.planningProfile.path);assert.deepEqual(pin,m.planningProfile);return raw;},deriveLifecycleParameters,deriveLifecycleCampaignBudget,save:async(file,report)=>{++saves;assert.equal(file,'/synthetic/pre-export-lifecycle-budget.json');assert.equal(report.exporterInvokedByThisPlanningStep,false);},join:(a,b)=>a+'/'+b,output:'/synthetic',sha,exportMarker:()=>++exportCalls};
  const invoke=new Async(...Object.keys(deps),actual+'exportMarker();return planningParameters;');
  if(insufficient){await assert.rejects(invoke(...Object.values(deps)));assert.equal(saves,0);assert.equal(exportCalls,0);}else{assert.equal((await invoke(...Object.values(deps))).maximumRunMs,22000000);assert.equal(saves,1);assert.equal(exportCalls,1);}
 }
});

test('all real proposal call sites select P while finality, operations and withdrawals retain F',()=>{
 const runtime=readFileSync(new URL('./lifecycle-runtime.mjs',import.meta.url),'utf8'),run=readFileSync(new URL('./run.mjs',import.meta.url),'utf8');
 assert.equal(runtime.split('await proposal(').length-1,4);assert.equal(runtime.split('ops.finalizedOperation(').length-1,1);assert.equal(runtime.split('ops.finalizedWithdrawal(').length-1,1);
 assert(runtime.includes('mode.deadlines.finalityMs);'));assert(run.includes("ms,phaseDeadline,'proposal')"));
 assert.equal(run.split("proposerMs,phaseEnd,'proposal')").length-1,2,'both actual Mac selection/post-restore searches use their original P deadline');
 assert(run.includes("'initial',true,spec.lifecycle?.deadlines.finalityMs??480000"));
 assert(!run.includes('--resume-lifecycle'),'this source batch does not masquerade as an operational resume command');
});
