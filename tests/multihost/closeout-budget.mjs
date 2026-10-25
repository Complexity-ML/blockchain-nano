// SOURCE planning inventory, never a deadline, launch or signing authority.
const need=(ok,m)=>{if(!ok)throw Error('Closeout budget inventory: '+m);};
const count=(n,label)=>{need(Number.isSafeInteger(n)&&n>=0,label+' nonnegative safe count');return n;};
const q=n=>'0x'+BigInt(n).toString(16);
const hex=s=>typeof s==='string'&&/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(s);
const plain=x=>x!==null&&typeof x==='object'&&!Array.isArray(x);
export function describeCloseoutBudget({phase,reserveMs,remainingGlobalMs,counts,linux,resourceBounds,signerCounts=null,macBootstrap,macOwnerRecoveryRequested}){
 need(['before-export','before-launch'].includes(phase)&&Number.isSafeInteger(reserveMs)&&reserveMs>0&&Number.isSafeInteger(remainingGlobalMs)&&remainingGlobalMs>=0,'original phase/reserve/global remainder');
 const countKeys=['heldFiles','newFiles','ownedOperations','ownerSourceFences','lifecycleSourceFences','localLifecycleAdmissions','lifecycleRemoteIndexes','lifecycleRemoteFiles'];
 need(plain(counts)&&Object.keys(counts).sort().join(',')===countKeys.sort().join(','),'exact registry count fields');for(const [k,n]of Object.entries(counts))count(n,k);
 need(plain(linux)&&['authoritySets','nestedFiles','volumes','authorityFiles'].every(k=>Number.isSafeInteger(linux[k])&&linux[k]>=0)&&hex(linux.authorityPayloadBytes)&&hex(linux.nestedPayloadBytes),'readonly Linux inventory');
 need(typeof macBootstrap==='boolean'&&typeof macOwnerRecoveryRequested==='boolean','actual feature selection');
 need(plain(resourceBounds)&&Object.values(resourceBounds).every(hex),'original declared resource quantities');
 if(signerCounts!==null)need(plain(signerCounts)&&['mac-native','mac-docker','vps-docker'].every(k=>Number.isSafeInteger(signerCounts[k])&&signerCounts[k]>=0),'prepared signer count');
 const A=BigInt(linux.authoritySets),N=BigInt(linux.nestedFiles),oneCurrentLinuxFence=70000n*(A+N);
 // The 70s is the SUM of existing requested command timers for one mount:
 // volume inspect30s + absence ps30s + verify/read run10s. FS is NOT included.
 const rows=[
  {id:'owned-inflight-command-join',calls:1,scope:'closeAll cancels and joins registered clients',unpriced:['current active client count is not exposed by the existing helper','TERM/KILL joins and OS scheduling']},
  {id:'runtime-stops',calls:3,scope:'native plus two exact owned Docker runtimes',unpriced:['identity inspections','remote stopped-log projection','receipt publications']},
  {id:'native-and-tunnel-joins',calls:2,requestedTermMs:[30000,5000],possibleKillJoinMs:3000,unpriced:['OS scheduling']},
  {id:'three-owner-capture-pairs',calls:3,cppCaptures:6,directNativeAndMacCppTimeoutMs:180000,vpsParentProjectionTimeoutMs:240000,unpriced:['VPS capsule-pin owner read','Mac stop write and four verifyAll passes','capture publications']},
  {id:'closing-vps-core-copy',creates:1,postToolVerifications:2,perCopyParentTimeoutMs:190000,setupCommands:4,defaultSetupCommandMs:30000,unpriced:['copy-tree file count and bytes','copy stability samples','remote projection fsync/FD closure']},
  {id:'closing-vps-controls-and-requests',resultDirectoryCommands:1,controlStageCommands:1,requestWriteCommands:2,directoryCommandMs:30000,stageOrWriteParentMs:40000,unpriced:['public manifest/config reads','known-file stability samples','future registered indexes/files']},
  {id:'lifecycle-closing-tools',calls:2,vpsParentTimeoutMs:650000,scope:'observe then terminal; not the three owner audits',unpriced:['core copy','owner control stage','known-file stability','post-tool fences']},
  {id:'terminal-owner-audits',calls:macOwnerRecoveryRequested?3:0,nativeTimeoutMs:180000,macJobTimeoutMs:180000,vpsProjectionTimeoutMs:240000,unpriced:['Mac volume create/write','Mac job metadata','post-audit verifyAll','request and result publications']},
  {id:'mac-original-owner-source-fences',passes:2,currentRows:counts.ownerSourceFences,currentCalls:2*counts.ownerSourceFences,expectedCallsAfterCompletedMacRecoveries:macOwnerRecoveryRequested?4:0,unpriced:['bounded-tree file count and bytes','Mac job mount inspections','local asynchronous filesystem latency']},
  {id:'software-fences',calls:2,knownDockerRuns:4,knownImageInspections:4,vpsHostCommands:2,defaultCommandMs:30000,unpriced:['native loader checks','native and host software reads']},
  {id:'linux-fences',calls:2,currentAuthoritySets:linux.authoritySets,currentNestedFiles:linux.nestedFiles,requestedOneCurrentPassMs:q(oneCurrentLinuxFence),unpriced:['future authority/nested growth','two helper reads per job','all filesystem latency']},
  {id:'bootstrap-source-fences',calls:macBootstrap?2:0,jobsPerFence:2,mountsPerJob:1,requestedEachInventoryRunMs:20000,requestedCurrentEnvelopeMs:q(macBootstrap?2n*2n*(30000n+30000n+20000n):0n),unpriced:['helper reads','filesystem latency']},
  {id:'held-input-fences',calls:2,currentRows:counts.heldFiles,unpriced:['future held rows','full content read for every row','up to1GiB per ordinary observe']},
  {id:'new-output-fences',calls:3,currentRows:counts.newFiles,unpriced:['future outputs','full content read for every row','late terminal report additions']},
  {id:'lifecycle-original-source-fences',currentCalls:counts.lifecycleSourceFences,expectedAfterCompletedVpsRestores:2,vpsTimeoutMs:195000,unpriced:['future source fences','original full-tree bytes and namespace']},
  {id:'lifecycle-local-artifact-fences',currentCalls:counts.localLifecycleAdmissions,unpriced:['per-admission row count and bytes','local asynchronous filesystem latency']},
  {id:'lifecycle-remote-index-fences',passes:2,currentCalls:2*counts.lifecycleRemoteIndexes,perCallTimeoutMs:35000,scope:'before closing tools and again after readonly cleanup',unpriced:['future indexes, including closing control/core admissions','all files referenced by each index']},
  {id:'lifecycle-remote-known-fence',calls:2,currentRows:counts.lifecycleRemoteFiles,parentTimeoutMs:35000,scope:'before closing tools and again in final fence',unpriced:['future known files','bytes and filesystem latency']},
  {id:'owned-operation-cleanup',loops:3,currentRows:counts.ownedOperations,perRow:'absence ps, optional inspect, optional exact-owner stop',defaultCommandMs:30000,stopCommandMs:20000,unpriced:['new jobs produced by captures, software and readonly fences']},
  {id:'terminal-container-absence',calls:2,defaultCommandMs:30000,unpriced:['command process-group joins']},
  {id:'durable-publications',fixedAtLeast:6,firstAdmissionDeadlineMs:30000,stableSamples:3,stableSpanAtLeastMs:2000,unpriced:['capture/audit publications','failure logs','fsync and async filesystem latency','up to1GiB per save']},
  {id:'deadline-and-provisional-verdict',calls:'before/after publication plus final',unpriced:['outer actual terminal observation is a separate authority']}
 ];
 return {format:'nano-fresh-closeout-budget-inventory-v1',phase,scope:'fresh lifecycle; backupRestore and partitionRotation excluded by its existing validator',configuredReserveMs:reserveMs,remainingOriginalGlobalMs:remainingGlobalMs,
  observedRegistryCounts:{...counts},observedLinuxInventory:{...linux},originalResourceBounds:{...resourceBounds},preparedSignerCounts:signerCounts,
  currentCaptureDatabaseRoles:signerCounts===null?null:Object.fromEntries(Object.entries(signerCounts).map(([owner,n])=>[owner,q(2n*(BigInt(n)+1n))])),
  currentOneLinuxFenceRequestedCommandMs:q(oneCurrentLinuxFence),serialRows:rows,
  sourceReferences:['run.mjs: fresh finally and actual callers','linux-owned-files.mjs: job/verifyAll/bootstrap','lifecycle-runtime.mjs: NEW first admission'],
  snapshotTakenBeforeThisPublication:true,futureRegistryCountsBounded:false,asynchronousFilesystemCallsAbortable:false,
  configuredTimersArePhysicalCompletionBounds:false,completeRequiredReserveMs:null,admissionComplete:false,
  reserveSufficiencyQualified:false,coreAuditMemoryQualified:false,wallClockCompletionGuaranteed:false,campaignAuthorized:false,productionQualified:false};
}
