#!/usr/bin/env node
import {nativeRuntimeEnvironment} from './native-runtime-env.mjs';
import {linuxSourceFiles,createMacLinuxFiles,linuxInventory,linuxBootstrapProof} from './linux-owned-files.mjs';
import {runFreshLifecycle,requireLifecycleRuntimeAuthority,lifecycleRollingFinalityWindow,lifecyclePrivateDirectory,readLifecycleKnownFile,writeLifecycleKnownFile,admitLifecycleKnownFiles,lifecycleSubmissionPlan,validatePreparedLifecycleAction,validateLifecycleAcceptedProjection,validateLifecycleTerminalProjection,lifecycleOperationDigest} from './lifecycle-runtime.mjs';
import {lifecycleWholeCopyProgram,lifecycleOwnerProjection} from './lifecycle-owner.mjs';
import {deriveLifecycleParameters,validateLifecycleDeposit} from './lifecycle.mjs';
import {freshSoftwareLocators,freshSoftwareFileMounts} from './program-locators.mjs';
import {createNativeRotationCopies,linuxRotationCopy,validateRotationCopySummary} from './rotation-owned-copy.mjs';
import {monotonicClockConfiguration} from './local-clock.mjs';
import {capturePeerSyncing} from './peer-syncing.mjs';
import {pruningRestoreMode,pruningRestoreConfiguration,verifyPrunedRestoreAudit,validateBackupRestoreMode} from './pruning-restore.mjs';
// Authorized isolated devnet only; code is deployed through Git, not this script.
import {execFile,spawn} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdir,mkdtemp,readdir,writeFile,readFile,appendFile,open,lstat,realpath,chmod} from 'node:fs/promises';
import {openSync,closeSync,constants as fsConstants} from 'node:fs';
import {resolve,join,dirname,basename} from 'node:path';
import {fileURLToPath} from 'node:url';
import {tmpdir} from 'node:os';
import {setTimeout as delay} from 'node:timers/promises';
import {createHash,randomUUID} from 'node:crypto';
import {validateStoppedValidators} from './validator-gate.mjs';
import {partitionRemote} from './partition.mjs';
import {fixtureStake,majorityIslandPolicy,majorityCutDuration} from './majority-island.mjs';
import {partitionSchedule,runPartitionCycles} from './partition-cycles.mjs';
import {asymmetricRemote,createRemoteTc,asymmetricHoldDuration} from './asymmetric.mjs';
import {observeRecovery} from './recovery-observation.mjs';
import {sampleClock} from './clock-sample.mjs';
import {captureFaultProgress} from './fault-progress.mjs';
import {createRotationCapture,ownership,rotationNodes,waitRotationWindow,freshRotationParameters,freshRotationOwnership,freshPartitionFinalityObserver} from './rotation.mjs';
import {runBoundedLoad} from './bounded-load.mjs';
import {stoppedReceipt,pinnedRestoreConfiguration,restoreAuditRequest,validateRestoredFinality} from './restore-config.mjs';
import {selectRestoreRuntime} from './restore-runtime.mjs';
import {waitRestoredRecovery} from './restore-recovery.mjs';
import {restoredFinalityAuditRequest,verifyRestoredFinalityAudit} from './restore-offline-audit.mjs';
import {startFinalityMonitor} from './monitor.mjs';
import {qualifyAlertFault} from './alert-fault.mjs';
import {selectCandidates,verifyPackagedImage} from './candidate-images.mjs';
import {selectHostBuildPaths,selectRemoteSourceRepository,remoteDockerArgument,selectNativeBuildPaths,selectBuildArtifacts,qualifiedRetainedArtifacts} from './build-paths.mjs';
import {advanceRestartConfiguration} from './restart-pins.mjs';
import {decodeHttpResponse,httpStatusTrailer} from './http-response.mjs';
import {hashHarness} from './harness-manifest.mjs';
import {prepareFresh,validateFreshCampaignSpec,validateFreshOwnerAdmission,verifyFreshBootstrapTransportInventory,checkFreshInitialRuntimeClock,launchFreshInitialRuntime} from './prepare.mjs';
import {verifyIsolatedFixtureDomain,publicSha256,canonicalPublicJson} from '../capella/isolated-fixture-domain.mjs';
import {createIsolatedBootstrapPlan} from '../capella/isolated-bootstrap-plan.mjs';
import {isolatedBootstrapStateCheckpoint,isolatedRuntimeObservation,isolatedPendingObservation} from '../capella/isolated-runtime-observation.mjs';
const freshArguments=process.argv.slice(2);
if(freshArguments[3]==='--fresh-campaign'){
 if(freshArguments.length!==7||freshArguments[5]!=='--fresh-campaign-sha')throw Error('Usage: run.mjs user@IPv4 source-base-revision new-output --fresh-campaign reviewed-spec.json --fresh-campaign-sha SHA256');
 await runFreshCampaign(freshArguments);
}else{
const exec=promisify(execFile),[target,revision,outputArg]=process.argv.slice(2);
const fork=process.env.NANO_TEST_CONSENSUS_PROFILE??'bellatrix';
if(!['bellatrix','capella'].includes(fork))throw Error('Unsupported consensus profile');
if(!/^[a-z_][a-z0-9_-]*@[0-9.]+$/.test(target??'')||!(/^[0-9a-f]{40}$/).test(revision??'')||!outputArg)throw Error('Usage: run.mjs user@IPv4 core-revision fresh-output');
const peerSyncing=process.env.NANO_TEST_PEER_SYNCING==='1';
if(peerSyncing&&fork!=='capella')throw Error('Peer syncing qualification requires Capella');
const monitor=process.env.NANO_TEST_FINALITY_MONITOR==='1';
const alertDelivery=process.env.NANO_TEST_ALERT_DELIVERY==='1';
const buildVolume=process.env.NANO_TEST_BUILD_VOLUME==='1';
if(process.env.NANO_TEST_NETWORK_WITH_CORE!==undefined&&!['0','1'].includes(process.env.NANO_TEST_NETWORK_WITH_CORE))throw Error('Invalid network build selector');
const hostBuildPaths=selectHostBuildPaths(buildVolume,process.env.NANO_TEST_BUILD_SUBDIRECTORY,process.env.NANO_TEST_BUILD_SUBDIRECTORY_ARM64,process.env.NANO_TEST_BUILD_SUBDIRECTORY_AMD64,process.env.NANO_TEST_NETWORK_WITH_CORE==='1');
const buildPaths=hostBuildPaths.arm64;
const buildArtifacts=selectBuildArtifacts(buildVolume,process.env.NANO_TEST_BUILDER_IMAGE_ARM64,process.env.NANO_TEST_BUILDER_IMAGE_AMD64,process.env.NANO_TEST_BUILD_VOLUME_ARM64,process.env.NANO_TEST_BUILD_VOLUME_AMD64);
const remoteRepository=selectRemoteSourceRepository(target.split('@')[0],process.env.NANO_TEST_REMOTE_SOURCE_REPOSITORY);
const candidateFile=process.env.NANO_TEST_CANDIDATE_MANIFEST;
if(candidateFile&&buildVolume)throw Error('Select packaged images or build volume, not both');
const candidates=candidateFile?selectCandidates(JSON.parse(await readFile(resolve(candidateFile),'utf8')),revision):null;
const privateTransport=buildVolume||Boolean(candidates);
const output=resolve(outputArg),repo=process.cwd(),image=candidates?.arm64.image??(buildVolume?buildArtifacts.arm64.image:`blockchain-nano:${revision.slice(0,7)}`),remoteImage=candidates?.amd64.image??(buildVolume?buildArtifacts.amd64.image:image);
const nativePaths=selectNativeBuildPaths(repo,process.env.NANO_TEST_NATIVE_BUILD_DIRECTORY,process.env.NANO_TEST_NATIVE_NETWORK_DIRECTORY);
const nofile=process.env.NANO_DOCKER_NOFILE??'65536';
if(!/^[1-9]\d*$/.test(nofile)||!Number.isSafeInteger(Number(nofile)))throw Error('NANO_DOCKER_NOFILE requires a positive safe integer');
const descriptorArgs=['--ulimit',`nofile=${nofile}:${nofile}`];
const artifactMount=buildVolume?['--mount',buildArtifacts.arm64.mount]:[];
const runtimeCommand=buildVolume?[buildPaths.nano]:[];
const traceArgs=process.env.NANO_TEST_NETWORK_TRACE==='1'?['--env','NANO_TEST_NETWORK_TRACE=1']:[];
const id=Date.now().toString(36),local=`nano-qualification-mac-${id}`,remote=`nano-qualification-vps-${id}`;
const partition=process.env.NANO_TEST_WAN_PARTITION==='1',asymmetric=process.env.NANO_TEST_WAN_ASYMMETRIC==='1',testNetwork=remote+'-net';
if(partition&&asymmetric)throw Error('Select only one WAN fault');
if(process.env.NANO_TEST_ASYMMETRIC_HOLD_MS!==undefined&&!asymmetric)throw Error('Asymmetric hold requires asymmetric mode');
const asymmetricHoldMs=asymmetric?asymmetricHoldDuration(process.env.NANO_TEST_ASYMMETRIC_HOLD_MS):null;
const rotation=process.env.NANO_TEST_WAN_ROTATION==='1';
const boundedLoad=process.env.NANO_TEST_BOUNDED_LOAD==='1';
const partitionCuts=partitionSchedule(process.env.NANO_TEST_PARTITION_SCHEDULE_MS);
const majorityIsland=process.env.NANO_TEST_MAJORITY_ISLAND==='1';
if(process.env.NANO_TEST_MAJORITY_CUT_MS!==undefined&&!majorityIsland)throw Error('Majority cut duration requires majority island mode');
const majorityCutMs=majorityIsland?majorityCutDuration(process.env.NANO_TEST_MAJORITY_CUT_MS):null;
const faultProgress=majorityIsland||process.env.NANO_TEST_FAULT_PROGRESS==='1';
if(faultProgress&&(fork!=='capella'||!partition))throw Error('Fault progress diagnostics require Capella partition campaign');
const backupRestore=process.env.NANO_TEST_BACKUP_RESTORE==='1';
const historicalPruning=pruningRestoreMode(process.env.NANO_TEST_HISTORICAL_STATE_PRUNING,{profile:fork,backupRestore});
const pinnedRestart=process.env.NANO_TEST_PINNED_RESTART==='1';
if(partitionCuts&&(!candidates||fork!=='capella'||!partition||asymmetric||rotation||boundedLoad||backupRestore||pinnedRestart||monitor))throw Error('Repeated partitions require standalone packaged Capella partition campaign');
if(majorityIsland&&(!(candidates||qualifiedRetainedArtifacts(buildVolume,buildArtifacts))||fork!=='capella'||!partition||asymmetric||rotation||boundedLoad||backupRestore||pinnedRestart||monitor||partitionCuts))throw Error('Majority island requires standalone attested Capella partition campaign');
if(pinnedRestart&&(!candidates||fork!=='capella'||monitor||backupRestore||partition||asymmetric||rotation||boundedLoad))throw Error('Pinned container replacement requires standalone packaged Capella images');
validateBackupRestoreMode({backupRestore,profile:fork,reviewedArtifacts:Boolean(buildVolume||candidates),partition,asymmetric,rotation,boundedLoad,historicalPruning});
const restoreRuntime=backupRestore?selectRestoreRuntime({buildVolume,packaged:Boolean(candidates),buildNano:buildPaths.nano}):null;
if(boundedLoad&&(fork!=='capella'||asymmetric||rotation))throw Error('Bounded load requires Capella; asymmetric/rotation combinations are not qualified yet');
if(rotation&&(fork!=='capella'||!partition||!buildVolume))throw Error('Rotation requires Capella, the isolated partition and build volumes');
if(alertDelivery&&(!monitor||!candidates||fork!=='capella'||partition||asymmetric||rotation||boundedLoad||backupRestore||pinnedRestart))throw Error('Alert fault requires standalone packaged Capella monitor campaign');
const remoteRoot=`/home/${target.split('@')[0]}/nano-qualification-${id}`,ip=target.split('@')[1];
const quote=s=>"'"+String(s).replaceAll("'","'\\''")+"'";
const sh=async (args,timeout=30000)=>(await exec('ssh',['-o','BatchMode=yes','-o','ConnectTimeout=8',target,args.map(quote).join(' ')],{timeout,maxBuffer:8*1024*1024})).stdout;
const remoteArtifact=a=>remoteDockerArgument(a,image,remoteImage,buildVolume,hostBuildPaths,buildArtifacts);
const docker=async(args,onRemote=false,timeout=30000)=>onRemote?sh(['sudo','-n','docker',...args.map(remoteArtifact)],timeout):(await exec('docker',args,{timeout,maxBuffer:8*1024*1024})).stdout;
const captureLogs=async(name,onRemote)=>{
 const args=onRemote?['ssh','-o','BatchMode=yes',target,['sudo','-n','docker','logs',name].map(quote).join(' ')+' 2>&1']:['docker','logs',name];
 const result=await exec(args[0],args.slice(1),{timeout:30000,maxBuffer:8*1024*1024});
 return result.stdout+result.stderr;
};
const json=async p=>JSON.parse(await readFile(p,'utf8'));
// Preflight before creating fixtures, so compilation cannot age the genesis.
const artifacts=[];
await exec('git',['diff','--quiet',revision,'--','src','include','network','third_party']);
await sh(['git','-C',remoteRepository,'diff','--quiet',revision,'--','src','include','network','third_party']);
for(const onRemote of [false,true]){
 const info=JSON.parse(await docker(['image','inspect',image,'--format','{{json .}}'],onRemote));
 if((!buildVolume&&info.Config.Labels?.['org.opencontainers.image.revision']!==revision)||info.Architecture!==(onRemote?'amd64':'arm64'))throw Error('Build revision/architecture mismatch');
 if(candidates){
  const spec=candidates[onRemote?'amd64':'arm64'];
  const hashes=await docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--entrypoint','/usr/bin/sha256sum',image,'/usr/local/bin/nano','/usr/local/bin/nano-p2p'],onRemote);
  artifacts.push({host:onRemote?'vps':'docker-mac',imageId:info.Id,revision,executables:verifyPackagedImage(spec,info,hashes),packaged:true});
 }
 if(buildVolume){
  const built=(await docker(['run','--rm','--network','none',...artifactMount,image,'cat',buildPaths.revision],onRemote)).trim();
  if(built!==revision)throw Error('Build volume source revision mismatch');
  const hashes=await docker(['run','--rm','--network','none',...artifactMount,image,'sha256sum',buildPaths.nano,buildPaths.p2p,...(rotation?[buildPaths.rotationAudit]:[])],onRemote);
  artifacts.push({host:onRemote?'vps':'docker-mac',imageId:info.Id,revision:built,sha256:hashes});
 }
}
await mkdir(output);
const log=async event=>appendFile(join(output,'run.jsonl'),JSON.stringify({time:Date.now(),...event})+'\n');
const assets=join(output,'assets'),prepared=join(output,'prepared');
let monitorTask,restartReceipt,restartPriorText,restartOriginal;
let native,tunnel,localStarted=false,remoteStarted=false,restoreReceipt,restoreStartedMs,restoreProof;
let resourceTask,resourceStop=false,resourceFailure;
let rotationTask,rotationStop=false,rotationFailure,rotationProof,rotationCutoff;
const rotationCapture=rotation?createRotationCapture():null;
const clockSamples=async phase=>{
 for(const [host,remoteClock] of [['vps',()=>sh(['date','+%s%3N'])],['docker-mac',()=>docker(['exec',local,'date','+%s%3N'])]]){
  try{await log({event:'clock-sample',phase,host,...await sampleClock(remoteClock)});}
  catch(error){await log({event:'clock-sample-unavailable',phase,host,message:error.message});}
 }
};
const request=async(kind,path,body)=>{
 const args=['-sS','--max-time','5','-H','Accept: application/json','--write-out',httpStatusTrailer];
 if(body)args.push('-H','Content-Type: application/json','--data',JSON.stringify(body));
 args.push((kind==='native'?'http://127.0.0.1:18545':'http://127.0.0.1:8545')+path);
 const text=kind==='native'?(await exec('curl',args,{timeout:8000})).stdout:await docker(['exec',kind==='remote'?remote:local,'curl',...args],kind==='remote');
 return decodeHttpResponse(text,kind,path);
};
const rpc=async(kind,method,params=[])=>{
 const r=await request(kind,'/',{jsonrpc:'2.0',id:1,method,params});if(r.error)throw Error(JSON.stringify(r.error));return r.result;
};
try{
 const {nano:binary,p2p}=nativePaths;
 if(rotation)await log({event:'rotation-audit-binary',sha256:createHash('sha256').update(await readFile(nativePaths.rotationAudit)).digest('hex')});
 const harness=await hashHarness(repo);
 if(monitor&&(fork!=='capella'||!privateTransport||partition||asymmetric||backupRestore||rotation||boundedLoad))throw Error('Monitor qualification requires standalone private Capella campaign');
 await log({event:'harness',peerSyncing,monitor,alertDelivery,partition,asymmetric,rotation,boundedLoad,backupRestore,pinnedRestart,partitionCuts,majorityIsland,majorityCutMs,faultProgress,fork,buildVolume,buildPaths:buildVolume?hostBuildPaths:null,nativePaths,remoteRepository,buildArtifacts:buildVolume?buildArtifacts:null,packagedImages:Boolean(candidates),artifacts,sha256:harness});
 await log({event:'binaries',revision,nanoSha256:createHash('sha256').update(await readFile(binary)).digest('hex'),p2pSha256:createHash('sha256').update(await readFile(p2p)).digest('hex')});
 await exec(nativePaths.transitionTests[fork],[fork==='capella'?'--export-network':'--export-kurtosis',assets],{timeout:30000});
 const topology=[{name:'mac-native',host:'mac',mode:'native',rpcPort:18545,p2pPort:30305,p2pBinary:p2p,runtimeDirectory:join(prepared,'mac-native')},{name:'mac-docker',host:'mac',mode:'docker',rpcPort:8545,p2pPort:30303},{name:'vps-docker',host:target,mode:'docker',rpcPort:8545,p2pPort:30303}];
 await writeFile(join(output,'topology.json'),JSON.stringify(topology));
 await exec(process.execPath,['tests/multihost/prepare.mjs',assets,prepared,join(output,'topology.json')]);
 if(historicalPruning)for(const name of ['mac-native','mac-docker','vps-docker']){const path=join(prepared,name,'node.json');await writeFile(path,JSON.stringify(pruningRestoreConfiguration(await json(path),true),null,2));}
 await log({event:'historical-state-pruning-mode',enabled:historicalPruning});
 if(process.env.NANO_TEST_WALL_CLOCK_DRIFT_MS!==undefined){
  for(const name of ['mac-native','mac-docker','vps-docker']){
   const path=join(prepared,name,'node.json');
   await writeFile(path,JSON.stringify(monotonicClockConfiguration(await json(path),process.env.NANO_TEST_WALL_CLOCK_DRIFT_MS,await json(join(prepared,name,'profile.json'))),null,2));
  }
  await log({event:'monotonic-local-clock-mode',maximumWallClockDriftMs:process.env.NANO_TEST_WALL_CLOCK_DRIFT_MS,peerTimeAuthority:false});
 }
 if(buildVolume)for(const name of ['mac-docker','vps-docker']){const path=join(prepared,name,'node.json'),c=await json(path);c.p2p_binary=hostBuildPaths[name==='vps-docker'?'amd64':'arm64'].p2p;await writeFile(path,JSON.stringify(c));}
 await sh(['mkdir',remoteRoot]);
 // Only newly generated PUBLIC fixtures cross SSH; software arrived via Git.
 await exec('scp',['-q','-r',join(prepared,'vps-docker'),`${target}:${remoteRoot}/config`],{timeout:30000});
 if(partition||asymmetric)await docker(['network','create','--label','nano.qualification=multihost',testNetwork],true);
 await docker(['run','--init',...descriptorArgs,...((partition||asymmetric)?['--network',testNetwork]:[]),...traceArgs,'-d','--name',remote,'--label','nano.qualification=multihost','--label','org.opencontainers.image.revision='+revision,...artifactMount,'--memory','3g','--cpus','2','-p',privateTransport?'127.0.0.1:30307:30303':'30307:30303','--mount',`type=bind,src=${remoteRoot}/config,dst=/config,readonly`,'--mount',`type=volume,src=${remote}-data,dst=/var/lib/nano`,'--env-file',`${remoteRoot}/config/fixture.env`,image,...runtimeCommand,'run','/config/genesis.json','/var/lib/nano/chain','/config/node.json'],true);remoteStarted=true;
 let peer;
 for(let i=0;i<30;i++){try{peer=(await rpc('remote','nno_getNetworkInfo')).node?.peerId;if(peer)break;}catch{}await delay(1000);}
 if(!peer||!/^[A-Za-z0-9]+$/.test(peer))throw Error('Remote peer ID unavailable');
 if(privateTransport){
  const fd=openSync(join(output,'tunnel.log'),'a');tunnel=spawn('ssh',['-N','-o','BatchMode=yes','-o','ExitOnForwardFailure=yes','-o','ServerAliveInterval=15','-L','127.0.0.1:30307:127.0.0.1:30307',target],{stdio:['ignore',fd,fd]});closeSync(fd);
  tunnel.on('error',error=>log({event:'tunnel-error',message:error.message}));await delay(1000);
  if(tunnel.exitCode!==null||tunnel.signalCode!==null)throw Error('Private SSH tunnel failed');
 }
 const boot=privateTransport?`/ip4/127.0.0.1/tcp/30307/p2p/${peer}`:`/ip4/${ip}/tcp/30307/p2p/${peer}`;
 for(const name of ['mac-native','mac-docker']){const path=join(prepared,name,'node.json'),c=await json(path);c.bootnodes=[privateTransport&&name==='mac-docker'?`/dns4/host.docker.internal/tcp/30307/p2p/${peer}`:boot];await writeFile(path,JSON.stringify(c,null,2));}
 await docker(['run','--init',...descriptorArgs,...(majorityIsland?['-p','127.0.0.1:30306:30303']:[]),...traceArgs,'-d','--name',local,'--label','nano.qualification=multihost','--label','org.opencontainers.image.revision='+revision,...artifactMount,'--memory','3g','--cpus','2','--mount',`type=bind,src=${join(prepared,'mac-docker')},dst=/config,readonly`,'--mount',`type=volume,src=${local}-data,dst=/var/lib/nano`,'--env-file',join(prepared,'mac-docker','fixture.env'),image,...runtimeCommand,'run','/config/genesis.json','/var/lib/nano/chain','/config/node.json']);localStarted=true;
 if(majorityIsland){
  let localPeer;
  for(let i=0;i<30;i++){try{localPeer=(await rpc('local','nno_getNetworkInfo')).node?.peerId;if(localPeer)break;}catch{}await delay(1000);}
  if(!localPeer||!/^[A-Za-z0-9]+$/.test(localPeer))throw Error('Majority Docker peer ID unavailable');
  const path=join(prepared,'mac-native','node.json'),c=await json(path);
  c.bootnodes.push(`/ip4/127.0.0.1/tcp/30306/p2p/${localPeer}`);await writeFile(path,JSON.stringify(c,null,2));
  await log({event:'majority-direct-bootstrap',peerId:localPeer,address:c.bootnodes.at(-1)});
 }
 const nd=join(prepared,'mac-native');await mkdir(join(nd,'data'));
 const environment=Object.fromEntries((await readFile(join(nd,'fixture.env'),'utf8')).trim().split('\n').map(line=>{const at=line.indexOf('=');return [line.slice(0,at),line.slice(at+1)];}));
 const fd=openSync(join(output,'native.log'),'a');native=spawn(binary,['run',join(nd,'genesis.json'),join(nd,'data/chain'),join(nd,'node.json')],{env:{...process.env,...environment},stdio:['ignore',fd,fd]});closeSync(fd);
 native.on('error',error=>log({event:'native-spawn-error',message:error.message}));
 await log({event:'started',nativePid:native.pid,localContainer:local,remoteContainer:remote,bootnode:boot,physicalHosts:2,validatorKeys:64});
 if(boundedLoad||partitionCuts||majorityIsland)resourceTask=(async()=>{
  while(!resourceStop){
   const timeMs=Date.now();
   const probes=[['mac-native',async()=>(await exec('ps',['-p',String(native.pid),'-o','pid=,rss=,%cpu=,etime=,command='],{timeout:20000})).stdout],
    ['mac-docker',()=>docker(['stats','--no-stream','--format','{{json .}}',local],false,20000)],
    ['vps-docker',()=>docker(['stats','--no-stream','--format','{{json .}}',remote],true,20000)]];
   const samples=await Promise.all(probes.map(async([host,probe])=>{
    try{return {host,exitCode:0,stdout:await probe(),stderr:''};}
    catch(error){return {host,error:error.message};}
   }));
   if(samples.some(s=>s.error))throw Error('Resource collection incomplete: '+JSON.stringify(samples));
   await appendFile(join(output,'resources.jsonl'),JSON.stringify({timeMs,samples})+'\n');
   while(!resourceStop&&Date.now()-timeMs<20000)await delay(1000);
  }
 })().catch(error=>{resourceFailure=error;});
 const deadline=Date.now()+300000;let ready=false;const observedRoots=new Map();
 while(Date.now()<deadline){
  try{
   const heads=await Promise.all(['native','local','remote'].map(kind=>rpc(kind,'eth_getBlockByNumber',['finalized',false])));
   const peers=await Promise.all(['native','local','remote'].map(kind=>rpc(kind,'net_peerCount')));
   await log({event:'warming',heads:heads.map(h=>h&&({number:h.number,hash:h.hash})),peers});
   const kinds=['native','local','remote'];
   const checkpoints=await Promise.all(kinds.map(async kind=>(await request(kind,'/eth/v1/beacon/states/head/finality_checkpoints')).data.finalized));
   for(const c of checkpoints){
    const known=observedRoots.get(c.epoch);
    if(known&&known!==c.root){const error=Error('Conflicting finalized roots during startup');error.fatal=true;throw error;}
    observedRoots.set(c.epoch,c.root);
   }
   if(heads.every(h=>h&&h.hash===heads[0].hash)&&peers.every(n=>BigInt(n)>0n)&&checkpoints.every(c=>BigInt(c.epoch)>=2n&&c.epoch===checkpoints[0].epoch&&c.root===checkpoints[0].root)){
    const blocks=await Promise.all(kinds.map(async kind=>(await request(kind,'/eth/v2/beacon/blocks/'+checkpoints[0].root)).data.message));
    if(blocks.every(b=>b.body.execution_payload.block_hash===heads[0].hash)){ready=true;await log({event:'startup-finality-bound',checkpoint:checkpoints[0],execution:heads[0].hash});break;}
   }
  }catch(error){if(error.fatal)throw error;await log({event:'warming-error',message:error.message});}
  await delay(3000);
 }
 if(!ready)throw Error('Three-node startup/finality deadline exceeded');
 if(peerSyncing)await capturePeerSyncing({request,record:log,phase:'startup'});
 const genesis=await rpc('native','eth_getBlockByNumber',['0x0',false]);
 const observers={revision,pinnedStateRoots:fork==='capella',settleRecoveryGaps:partition||asymmetric||backupRestore||pinnedRestart,genesisHash:genesis.hash,nodes:[{name:'mac-native',kind:'native',url:'http://127.0.0.1:18545'},{name:'mac-docker',kind:'docker',container:local,url:'http://127.0.0.1:8545'},{name:'vps-docker',kind:'ssh-docker',ssh:target,container:remote,url:'http://127.0.0.1:8545'}]};
 await writeFile(join(output,'observers.json'),JSON.stringify(observers,null,2));
 const previousFinality=new Map();
 const faultSample=async()=>{
  const rows=await Promise.all(['native','local','remote'].map(async(kind,i)=>{
   const info=majorityIsland?await rpc(kind,'nno_getNetworkInfo'):null;
   const peers=await rpc(kind,'net_peerCount');
   const cp=await request(kind,'/eth/v1/beacon/states/head/finality_checkpoints');
   const progress=faultProgress?await captureFaultProgress(path=>request(kind,path)):null;
   return {name:observers.nodes[i].name,peers,...(info?{peerId:info.node.peerId,connectedPeerIds:info.peers.connected.map(p=>p.peerId)}:{}),checkpoint:cp.data.finalized,...(progress?{progress}:{})};
  }));
  for(const row of rows){
   const epoch=BigInt(row.checkpoint.epoch),key=String(epoch),root=row.checkpoint.root;
   if(observedRoots.has(key)&&observedRoots.get(key)!==root)throw Error('Conflicting finalized roots across campaign');
   if(previousFinality.has(row.name)&&epoch<previousFinality.get(row.name))throw Error('Finality regressed across campaign');
   observedRoots.set(key,root);previousFinality.set(row.name,epoch);
  }
  return rows;
 };
 const rotationHeads=()=>Promise.all(['native','local','remote'].map(async(kind,i)=>{
  const h=(await request(kind,'/eth/v1/beacon/headers/head')).data;
  return {name:rotationNodes[i],root:h.root,slot:Number(h.header.message.slot)};
 }));
 const captureRotation=async()=>{
  const heads=await rotationHeads();
  for(let i=0;i<heads.length;++i){
   let tip=heads[i].root;
   for(let depth=0;depth<32&&!rotationCapture.has(rotationNodes[i],tip);++depth){
    const args=['-fsS','--max-time','5','-H','Accept: application/octet-stream',(i===0?'http://127.0.0.1:18545':'http://127.0.0.1:8545')+'/eth/v2/beacon/blocks/'+tip];
    let command='curl',argv=args;
    if(i===1){command='docker';argv=['exec',local,'curl',...args];}
    if(i===2){command='ssh';argv=['-o','BatchMode=yes','-o','ConnectTimeout=8',target,['sudo','-n','docker','exec',remote,'curl',...args].map(quote).join(' ')];}
    const wire=(await exec(command,argv,{encoding:'buffer',timeout:15000,maxBuffer:8*1024*1024})).stdout;
    rotationCapture.add(rotationNodes[i],tip,wire);
    if(wire.readBigUInt64LE(100)<=112n)break;
    tip=rotationCapture.parent(tip);
   }
  }
  await log({event:'rotation-capture',heads,...rotationCapture.stats()});return heads;
 };
 if(rotation){
  await waitRotationWindow({sample:async()=>{await faultSample();return rotationHeads();},record:log});
  // One writer; await shutdown before assembling proof. A failed capture remains
  // fatal even if the network subsequently reaches finality.
  rotationTask=(async()=>{while(!rotationStop){await captureRotation();await delay(3000);}})().catch(error=>{rotationFailure=error;});
 }
 const faultRecord=async event=>{
  await log(event);
  if(rotation&&event.event==='partition-disconnected'){
   const heads=await rotationHeads();
   await log({event:'rotation-fault-start',heads});
   if(heads.some(h=>h.slot>=128))throw Error('Rotation boundary passed before effective partition');
  }
  if(rotation&&event.event==='partition-reconnected'){
   const heads=await rotationHeads();
   if(heads.some(h=>h.slot<128))throw Error('Partition did not cross the rotation on all nodes');
   rotationCutoff=Math.max(...heads.map(h=>h.slot));
   await log({event:'rotation-recovery-cutoff',heads,minimumInclusionSlot:rotationCutoff});
  }
 };
 if(backupRestore){
  await log({event:'backup-restore-stopping',container:remote,sourceVolume:remote+'-data'});
  await docker(['stop','--time','20',remote],true);restoreStartedMs=Date.now();
  const stopped=JSON.parse(await docker(['inspect',remote,'--format','{{json .State}}'],true));
  if(stopped.Running||stopped.ExitCode!==0||stopped.OOMKilled)throw Error('Backup source did not stop cleanly');
  const priorLogs=await captureLogs(remote,true);await writeFile(join(output,'vps-before-restore.log'),priorLogs);restoreReceipt=stoppedReceipt(priorLogs);
  const restoreDirectory=join(output,'restore');await mkdir(restoreDirectory);
  await writeFile(join(restoreDirectory,'stop-receipt.json'),JSON.stringify(restoreReceipt,null,2));
  await writeFile(join(restoreDirectory,'source-state.json'),JSON.stringify(stopped));
  const original=await json(join(prepared,'vps-docker/node.json'));
  await writeFile(join(restoreDirectory,'node.json'),JSON.stringify(pinnedRestoreConfiguration(original,restoreReceipt),null,2));
  await writeFile(join(restoreDirectory,'audit-request.json'),JSON.stringify(restoreAuditRequest(original,restoreReceipt),null,2));
  // Configuration and stop commitments travel separately from the archive.
  restoreProof=remoteRoot+'/restore-proof';
  await sh(['mkdir',restoreProof]);await sh(['cp','-a',remoteRoot+'/config',remoteRoot+'/restore-config']);
  await exec('scp',['-q',join(restoreDirectory,'node.json'),`${target}:${remoteRoot}/restore-config/node.json`],{timeout:10000});
  await exec('scp',['-q',join(restoreDirectory,'audit-request.json'),join(restoreDirectory,'stop-receipt.json'),`${target}:${restoreProof}/`],{timeout:10000});
  const active=(await docker(['ps','--filter','volume='+remote+'-data','--format','{{.Names}}'],true)).trim();if(active)throw Error('Backup source volume still used by a running container');
  await docker(['run','--rm','--network','none',...descriptorArgs,...artifactMount,'--cpus','2','--memory','3g','--mount',`type=volume,src=${remote}-data,dst=/source,readonly`,'--mount',`type=bind,src=${remoteRoot}/config,dst=/config,readonly`,'--mount',`type=bind,src=${restoreProof},dst=/proof`,'--mount',`type=bind,src=${remoteRepository},dst=/work,readonly`,...restoreRuntime.archiveArgs,image,...restoreRuntime.archiveCommand,'/work/tests/multihost/backup-restore.sh','/source','/proof/backup','/config/genesis.json','/proof/audit-request.json',restoreRuntime.nano,...restoreRuntime.ownerArgs],true,90000);
  const audit=JSON.parse(await sh(['sudo','-n','cat',restoreProof+'/backup/audit.json']));
  verifyRestoredFinalityAudit(await json(join(restoreDirectory,'audit-request.json')),restoreReceipt,audit);
  if(historicalPruning)verifyPrunedRestoreAudit(audit);
  const metadata=['source-before.sha256','source-after.sha256','restored-before.sha256','restored-after.sha256','archive.sha256','inputs.sha256','audit.json','passed.txt',...(candidates?['restored-owner-after.sha256','restored-owner.txt']:[])];
  for(const name of metadata)await writeFile(join(restoreDirectory,name),await sh(['sudo','-n','cat',restoreProof+'/backup/'+name]));
  const elapsedMs=Date.now()-restoreStartedMs,step=Number(BigInt(original.capella.clock.maximumForwardStepMs));
  if(elapsedMs>=step-10000)throw Error('Offline restore used the configured clock-recovery budget');
  await log({event:'backup-restore-audit-passed',elapsedMs,maximumForwardStepMs:step,audit,archive:restoreProof+'/backup/data.tar.gz'});
  // Original volume stays offline. Replace only the container; same saved peer ID.
  await docker(['rm',remote],true);remoteStarted=false;
  await docker(['run','--init',...descriptorArgs,...traceArgs,'-d','--name',remote,'--label','nano.qualification=multihost','--label','org.opencontainers.image.revision='+revision,...artifactMount,'--memory','3g','--cpus','2','-p','127.0.0.1:30307:30303','--mount',`type=bind,src=${remoteRoot}/restore-config,dst=/config,readonly`,'--mount',`type=bind,src=${restoreProof}/backup/restored,dst=/var/lib/nano`,'--env-file',`${remoteRoot}/restore-config/fixture.env`,image,...runtimeCommand,'run','/config/genesis.json','/var/lib/nano/chain','/config/node.json'],true);remoteStarted=true;
  let restoredPeer;const onlineDeadline=Date.now()+60000;
  while(Date.now()<onlineDeadline){try{restoredPeer=(await rpc('remote','nno_getNetworkInfo')).node?.peerId;if(restoredPeer)break;}catch{}await delay(1000);}
  if(restoredPeer!==peer)throw Error('Restored node did not retain its P2P identity');
  await log({event:'backup-restore-online',elapsedMs:Date.now()-restoreStartedMs,peerId:restoredPeer,restoredData:restoreProof+'/backup/restored',independentSignerPins:restoreReceipt.validatorKeys.length});
  await waitRestoredRecovery({saved:restoreReceipt.finalized,record:log,sample:async()=>{
   const rows=await faultSample(),heads=await rotationHeads();
   return rows.map((r,i)=>({...r,headSlot:heads[i].slot}));
  }});

 }
 if(pinnedRestart){
  await log({event:'pinned-replacement-stopping',container:remote,image:remoteImage});
  await docker(['stop','--time','20',remote],true);const stoppedAt=Date.now();
  const state=JSON.parse(await docker(['inspect',remote,'--format','{{json .State}}'],true));
  if(state.Running||state.ExitCode!==0||state.OOMKilled)throw Error('Replacement source did not stop cleanly');
  restartPriorText=await captureLogs(remote,true);restartReceipt=stoppedReceipt(restartPriorText);
  restartOriginal=await json(join(prepared,'vps-docker/node.json'));
  const dir=join(output,'replacement');await mkdir(dir);
  await writeFile(join(dir,'before.log'),restartPriorText);
  await writeFile(join(dir,'before-state.json'),JSON.stringify(state));
  await writeFile(join(dir,'independent-stop-receipt.json'),JSON.stringify(restartReceipt,null,2));
  const pinned=pinnedRestoreConfiguration(restartOriginal,restartReceipt);
  await writeFile(join(dir,'pinned-node.json'),JSON.stringify(pinned,null,2));
  await sh(['cp','-a',remoteRoot+'/config',remoteRoot+'/replacement-config']);
  await exec('scp',['-q',join(dir,'pinned-node.json'),`${target}:${remoteRoot}/replacement-config/node.json`],{timeout:10000});
  if(Date.now()-stoppedAt>=Number(BigInt(restartOriginal.capella.clock.maximumForwardStepMs))-10000)throw Error('Replacement exhausted clock recovery budget');
  await docker(['rm',remote],true);remoteStarted=false;
  await docker(['run','--init',...descriptorArgs,...traceArgs,'-d','--name',remote,'--label','nano.qualification=multihost','--label','org.opencontainers.image.revision='+revision,'--memory','3g','--cpus','2','-p','127.0.0.1:30307:30303','--mount',`type=bind,src=${remoteRoot}/replacement-config,dst=/config,readonly`,'--mount',`type=volume,src=${remote}-data,dst=/var/lib/nano`,'--env-file',`${remoteRoot}/replacement-config/fixture.env`,image,'run','/config/genesis.json','/var/lib/nano/chain','/config/node.json'],true);remoteStarted=true;
  let freshPeer;const deadline=Date.now()+60000;
  while(Date.now()<deadline){try{freshPeer=(await rpc('remote','nno_getNetworkInfo')).node?.peerId;if(freshPeer)break;}catch{}await delay(1000);}
  if(freshPeer!==peer)throw Error('Replacement lost P2P identity');
  await log({event:'pinned-replacement-online',peerId:freshPeer,elapsedMs:Date.now()-stoppedAt,chainCommitment:restartReceipt.restart,independentSignerPins:restartReceipt.validatorKeys.length,sameImage:true,dataRestored:false});
  await waitRestoredRecovery({saved:restartReceipt.finalized,record:log,sample:async()=>{const rows=await faultSample(),heads=await rotationHeads();return rows.map((r,i)=>({...r,headSlot:heads[i].slot}));}});
 }
 await clockSamples('before-fault');
 if(boundedLoad)await runBoundedLoad({rpc,genesisHash:genesis.hash,output,sampleFinality:faultSample,freshFinalizedState:historicalPruning,
  ...(partition?{duringSubmit:submit=>partitionRemote({docker:args=>docker(args,true),container:remote,
   network:testNetwork,revision,record:faultRecord,sample:faultSample,duringCut:submit})}:{})});
 if(alertDelivery)await qualifyAlertFault({repo,output,prepared,local,remote,target,record:log,docker:args=>docker(args,true)});
 if(monitor)monitorTask=await startFinalityMonitor({repo,output,prepared,local,remote,target,record:log});
 let islandPolicy;
 if(majorityIsland){
  const stake=fixtureStake({profileBytes:await readFile(join(prepared,'mac-native','profile.json')),initialBytes:await readFile(join(prepared,'mac-native','initial.json')),configuration:(await json(join(prepared,'mac-native','node.json'))).capella,manifest:await json(join(prepared,'manifest.json'))});
  const rows=await faultSample();islandPolicy=majorityIslandPolicy({stake,peerIds:Object.fromEntries(rows.map(r=>[r.name,r.peerId])),record:log});
 }
 await observeRecovery({record:log,partition:async()=>{
  if(partitionCuts){
   const result=await runPartitionCycles({cuts:partitionCuts,sample:faultSample,record:log,
    partition:({cutMs,sample,record})=>partitionRemote({docker:args=>docker(args,true),container:remote,network:testNetwork,revision,record,sample,cutMs})});
   await log({event:'partition-schedule-passed',...result});
  }else if(partition&&!boundedLoad)await partitionRemote({docker:args=>docker(args,true),container:remote,network:testNetwork,revision,record:faultRecord,sample:faultSample,...(majorityIsland?{cutMs:majorityCutMs,policy:islandPolicy}:{})});
  if(asymmetric){
   const tc=await createRemoteTc({docker:args=>docker(args,true),sh,container:remote,network:testNetwork,revision});
   await asymmetricRemote({tc,sample:faultSample,record:log,holdMs:asymmetricHoldMs});
  }
 },
 observe:()=>exec(process.execPath,['tests/multihost/observe.mjs',join(output,'observers.json'),join(output,'observations')],{timeout:480000,maxBuffer:8*1024*1024})});
 await log({event:'finality-passed'});
 if(peerSyncing)await capturePeerSyncing({request,record:log,phase:'finalized'});
 if(monitorTask){await monitorTask.stop(true);monitorTask=undefined;}
 if(backupRestore){
  const stable=await json(join(output,'observations/result.json'));
  if(BigInt(stable.firstEpoch)<BigInt(restoreReceipt.finalized.epoch)||BigInt(stable.lastEpoch)<BigInt(restoreReceipt.finalized.epoch)+3n)throw Error('Restored finality window did not advance beyond saved checkpoint');
  await log({event:'backup-restore-common-finality',savedFinalized:restoreReceipt.finalized,stable});
 }

 if(rotation){
  rotationStop=true;await rotationTask;if(rotationFailure)throw rotationFailure;
  await captureRotation();
  const checkpoints=await faultSample(),first=checkpoints[0].checkpoint;
  if(!checkpoints.every(r=>r.checkpoint.root===first.root&&r.checkpoint.epoch===first.epoch))throw Error('No common rotation checkpoint at capture');
  rotationProof={...rotationCapture.proof(first.root,rotationCutoff),owners:ownership(await json(join(prepared,'manifest.json')))};
  await writeFile(join(output,'rotation-capture.json'),JSON.stringify(rotationProof));
  await log({event:'rotation-captured-awaiting-offline-audit',finalized:first,minimumInclusionSlot:rotationCutoff});
 }
}catch(error){await log({event:'failed',message:error.stack});process.exitCode=1;}
finally{
 if(monitorTask){try{await monitorTask.stop();}catch(e){await log({event:'monitor-stop-failed',message:e.message});process.exitCode=1;}}
 resourceStop=true;if(resourceTask)await resourceTask;
 if(resourceFailure){await log({event:'resource-collection-failed',message:resourceFailure.stack});process.exitCode=1;}
 rotationStop=true;if(rotationTask)await rotationTask;
 if(rotationCapture)await writeFile(join(output,'rotation-raw-captures.json'),JSON.stringify(rotationCapture.dump()));
 if(rotationFailure){await log({event:'rotation-capture-failed',message:rotationFailure.stack});process.exitCode=1;}
 if(localStarted&&remoteStarted)await clockSamples('before-stop');
 if(native&&native.exitCode===null&&native.signalCode===null){native.kill('SIGTERM');for(let i=0;i<30&&native.exitCode===null&&native.signalCode===null;i++)await delay(1000);if(native.exitCode===null&&native.signalCode===null)native.kill('SIGKILL');}
 for(const [name,onRemote,started] of [[local,false,localStarted],[remote,true,remoteStarted]])if(started){
  try{await docker(['stop','--time','20',name],onRemote);await writeFile(join(output,onRemote?'vps.log':'docker-mac.log'),await captureLogs(name,onRemote));}catch(error){await log({event:'cleanup-error',name,message:error.message});process.exitCode=1;}
 }
 if(tunnel&&tunnel.exitCode===null&&tunnel.signalCode===null){tunnel.kill('SIGTERM');for(let i=0;i<30&&tunnel.exitCode===null&&tunnel.signalCode===null;i++)await delay(100);if(tunnel.exitCode===null&&tunnel.signalCode===null)tunnel.kill('SIGKILL');}
 // Retain stopped containers/volumes and native DBs for inspection; no pruning.
}
if(native&&localStarted&&remoteStarted){
 try{
  const validators=await validateStoppedValidators(output,fork);
  if(partitionCuts&&(!candidates||fork!=='capella'||!partition||asymmetric||rotation||boundedLoad||backupRestore||pinnedRestart||monitor))throw Error('Repeated partitions require standalone packaged Capella partition campaign');
if(pinnedRestart&&!process.exitCode){
   const finalText=await readFile(join(output,'vps.log'),'utf8'),terminal=stoppedReceipt(finalText);
   validateRestoredFinality(restartReceipt,await json(join(output,'observations/result.json')),terminal);
   const advanced=advanceRestartConfiguration(restartOriginal,restartPriorText,finalText);
   await writeFile(join(output,'replacement/current-pinned-node.json'),JSON.stringify(advanced.configuration,null,2));
   await writeFile(join(output,'replacement/current-stop-receipt.json'),JSON.stringify(terminal,null,2));
   await log({event:'pinned-replacement-passed',advancedSignerPins:advanced.advancedSignerPins,prior:advanced.prior,current:advanced.current,rollbackToDifferentVersionQualified:false});
  }

  if(backupRestore&&!process.exitCode){
   const restored=stoppedReceipt(await readFile(join(output,'vps.log'),'utf8')),stable=await json(join(output,'observations/result.json'));
   validateRestoredFinality(restoreReceipt,stable,restored);
   const stopped=JSON.parse(await docker(['inspect',remote,'--format','{{json .State}}'],true));
   if(stopped.Running||stopped.ExitCode!==0||stopped.OOMKilled)throw Error('Restored writer did not stop cleanly before ancestry audit');
   const request=restoredFinalityAuditRequest(await json(join(prepared,'vps-docker/node.json')),restoreReceipt,stable,restored);
   const requestPath=join(output,'restore/finalized-audit-request.json');await writeFile(requestPath,JSON.stringify(request,null,2));
   await exec('scp',['-q',requestPath,`${target}:${restoreProof}/finalized-audit-request.json`],{timeout:10000});
   const audit=JSON.parse(await docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','2g','--cpus','1','--tmpfs','/tmp:rw,nosuid,noexec,size=64m',...artifactMount,'--mount',`type=bind,src=${remoteRoot}/restore-config,dst=/config,readonly`,...(restoreRuntime.auditUsesDirectCopy?['--mount',`type=bind,src=${restoreProof}/backup/restored,dst=/data,readonly`,'--mount',`type=bind,src=${restoreProof}/finalized-audit-request.json,dst=/request.json,readonly`]:['--mount',`type=bind,src=${restoreProof},dst=/proof,readonly`]),'--entrypoint',restoreRuntime.nano,image,'audit-capella','/config/genesis.json',restoreRuntime.auditUsesDirectCopy?'/data/chain':'/proof/backup/restored/chain',restoreRuntime.auditUsesDirectCopy?'/request.json':'/proof/finalized-audit-request.json'],true,180000));
   await writeFile(join(output,'restore/finalized-audit.json'),JSON.stringify(audit,null,2));
   verifyRestoredFinalityAudit(request,restored,audit);
   if(historicalPruning)verifyPrunedRestoreAudit(audit);
   await log({event:'backup-restore-finalized-ancestry-passed',request,audit});
   await docker(['run','--rm','--network','none','--mount',`type=volume,src=${remote}-data,dst=/source,readonly`,'--mount',`type=bind,src=${restoreProof},dst=/proof,readonly`,...restoreRuntime.archiveArgs,image,...restoreRuntime.archiveCommand,'-lc','set -e; cd /source; find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum | cmp /proof/backup/source-before.sha256 -'],true);
   await log({event:'backup-restore-passed',sourceStillUnchanged:true,restoredSignerStatus:restored.validator,stable});
  }

  if(rotation&&!process.exitCode){
   const expectedTokens={};
   for(const [i,file] of ['native.log','docker-mac.log','vps.log'].entries()){
    const receipts=(await readFile(join(output,file),'utf8')).split('\n').flatMap(line=>{try{const r=JSON.parse(line);return r.event==='capella-runtime-stopped'?[r]:[];}catch{return [];}});
    if(receipts.length!==1)throw Error('Missing stopped rotation receipt');expectedTokens[rotationNodes[i]]=receipts[0].restart;
   }
   const proofFile=join(output,'rotation-proof.json');await writeFile(proofFile,JSON.stringify({...rotationProof,expectedTokens}));
   // Public proof only. Executables were built from Git on each host.
   await exec('scp',['-q',proofFile,`${target}:${remoteRoot}/rotation-proof.json`],{timeout:30000});
   const audits=[];
   const nd=join(output,'prepared/mac-native');
   audits.push(JSON.parse((await exec(nativePaths.rotationAudit,[join(nd,'genesis.json'),join(nd,'node.json'),join(nd,'data/chain'),proofFile,rotationNodes[0]],{timeout:120000,maxBuffer:1024*1024})).stdout));
   await writeFile(join(output,'rotation-audits.json'),JSON.stringify(audits,null,2));
   for(const [name,onRemote,index] of [[local,false,1],[remote,true,2]]){
    const config=onRemote?remoteRoot+'/config':join(output,'prepared/mac-docker');
    const proof=onRemote?remoteRoot+'/rotation-proof.json':proofFile;
    audits.push(JSON.parse(await docker(['run','--rm','--network','none',...artifactMount,'--mount',`type=bind,src=${config},dst=/config,readonly`,'--mount',`type=bind,src=${proof},dst=/proof.json,readonly`,'--mount',`type=volume,src=${name}-data,dst=/var/lib/nano`,image,buildPaths.rotationAudit,'/config/genesis.json','/config/node.json','/var/lib/nano/chain','/proof.json',rotationNodes[index]],onRemote,120000)));
    await writeFile(join(output,'rotation-audits.json'),JSON.stringify(audits,null,2));
   }
   for(const audit of audits)if(audit.stateSha256!==audits[0].stateSha256||audit.finalizedRoot!==audits[0].finalizedRoot||audit.committeeHash!==audits[0].committeeHash)throw Error('Reopened rotation audits disagree');
   await writeFile(join(output,'rotation-audits.json'),JSON.stringify(audits,null,2));
   await log({event:'rotation-offline-audits-passed',audits});
  }
  await log({event:process.exitCode?'validators-healthy-after-failed-campaign':'passed',validators});
 }
 catch(error){await log({event:'failed',message:error.stack});process.exitCode=1;}
}

} // Existing campaign modes above retain their original selection and flow.

// NEW synthetic-domain branch of this launcher. Legacy modes above are not
// silently rebound to new binaries, keys, archives or source revisions.
async function runFreshCampaign(args){
 const [target,revision,outputArg,,specFile,,specSha]=args,exec=promisify(execFile);
 const need=(ok,why)=>{if(!ok)throw Error(why);},sha=b=>createHash('sha256').update(b).digest('hex');
 const canonical=v=>Array.isArray(v)?'['+v.map(canonical).join(',')+']':v&&typeof v==='object'?'{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}':JSON.stringify(v);
 const names=['mac-native','mac-docker','vps-docker'],output=resolve(outputArg),harnessRoot=resolve(dirname(fileURLToPath(import.meta.url)),'../..');
 need(/^[a-z_][a-z0-9_-]*@[0-9.]+$/.test(target??'')&&/^[0-9a-f]{40}$/.test(revision??'')&&/^[0-9a-f]{64}$/.test(specSha??''),'Explicit reviewed fresh campaign arguments required');
 // This first campaign closes finality and two-root recovery. Legacy fault,
 // image, old build-volume and exporter selections cannot alter this branch.
 for(const k of Object.keys(process.env))if(k.startsWith('NANO_TEST_')&&k!=='NANO_TEST_NETWORK_TRACE')need(process.env[k]===undefined,'Fresh campaign disallows legacy NANO_TEST selectors: '+k);
 const materialFields=['dev','ino','mode','uid','gid','nlink','size','mtimeNs','ctimeNs'],held=new Map(),newFiles=new Map(),material=s=>materialFields.map(k=>String(s[k]));
 const publicExportLeaves=['isolated-domain.json','genesis-validators.ssz','genesis-state.ssz','genesis.json','profile.json','initial.json'],publicExportProbePaths=new Set();
 // Diagnostics disclose only an explicitly public locator and fixed field names.
 // They never publish either compared value or alter an admission predicate.
 const diagnosticPublicPaths=new Set([resolve(specFile),join(harnessRoot,'tests/capella/isolated-fixture-domain.mjs'),join(harnessRoot,'tests/multihost/backup-restore.sh')]);let publicCheckStage='public-input-admission';
 const materialChanges=(a,b)=>materialFields.filter((_,i)=>a[i]!==b[i]);
 const rowChanges=(a,b)=>[...['path','sha256','bytes'].filter(k=>a[k]!==b[k]),...materialChanges(a.material,b.material)];
 function inputNeed(ok,why,path,changedFieldNames){if(!ok){const diagnostic={stage:publicCheckStage,publicPath:diagnosticPublicPaths.has(path)?path:null,changedFieldNames:[...new Set(changedFieldNames)]};const e=Error(why+' '+JSON.stringify(diagnostic));e.publicInputDiagnostic=diagnostic;throw e;}}
 async function observe(path,expected,body=false,admit=true){
  path=resolve(path);if(newFiles.has(path)&&!held.has(path)){expected=newFiles.get(path);admit=false;}need(await realpath(path)===path,'Linked public input path refused');const f=await open(path,fsConstants.O_RDONLY|fsConstants.O_NOFOLLOW|fsConstants.O_NONBLOCK),before=await f.stat({bigint:true});
  try{
   need(before.isFile()&&before.nlink===1n&&before.size>0n&&before.size<=BigInt(body||publicExportProbePaths.has(path)&&!held.has(path)?16*1024*1024:1073741824),'Public software/document bound/type');
   const hash=createHash('sha256'),buffer=Buffer.alloc(65536),chunks=[];let size=0;for(;;){const {bytesRead}=await f.read(buffer,0,buffer.length,null);if(!bytesRead)break;size+=bytesRead;need(BigInt(size)<=before.size,'Input grew');hash.update(buffer.subarray(0,bytesRead));if(body)chunks.push(Buffer.from(buffer.subarray(0,bytesRead)));}
   const after=await f.stat({bigint:true}),linked=await lstat(path,{bigint:true}),row={path,sha256:hash.digest('hex'),bytes:size,material:material(after)};
   inputNeed(size===Number(before.size)&&canonical(material(before))===canonical(row.material)&&canonical(row.material)===canonical(material(linked)),'Public input changed during FD read',path,[...(size===Number(before.size)?[]:['bytes']),...materialChanges(material(before),row.material),...materialChanges(row.material,material(linked))]);
   if(expected)inputNeed(row.sha256===expected.sha256&&(expected.bytes===undefined||row.bytes===expected.bytes),'Public input SHA/bytes differs',path,[...(row.sha256===expected.sha256?[]:['sha256']),...(expected.bytes===undefined||row.bytes===expected.bytes?[]:['bytes'])]);
   if(publicExportProbePaths.has(path)&&expected?.material&&!held.has(path))inputNeed(canonical(expected.material)===canonical(row.material),'New public export first admission material differs',path,materialChanges(expected.material,row.material));
   if(held.has(path))inputNeed(canonical(held.get(path))===canonical(row),'Previously admitted public input changed',path,rowChanges(held.get(path),row));else if(admit)held.set(path,row);
   return body?Buffer.concat(chunks):row;
  }finally{await f.close();}
 }
 async function fence(){for(const z of [...held.values()])await observe(z.path,z);}
 async function save(path,value){const bytes=Buffer.isBuffer(value)?value:Buffer.from(typeof value==='string'?value:JSON.stringify(value,null,2)+'\n');const f=await open(path,'wx',0o600);try{await f.writeFile(bytes);await f.sync();}finally{await f.close();}const directory=await open(dirname(path),'r');try{await directory.sync();}finally{await directory.close();}const expected={sha256:sha(bytes),bytes:bytes.length};await observe(path,expected,false,false);newFiles.set(resolve(path),expected);return bytes;}
 const spec=validateFreshCampaignSpec(JSON.parse(await observe(resolve(specFile),{sha256:specSha},true)),{target,sourceBaseRevision:revision});
 need(!spec.lifecycle||spec.lifecycle.runtimeDriverEnabled,'Fresh lifecycle runtime driver is unavailable; managed submission and owner-local terminal audit are required');
 if(spec.lifecycle)requireLifecycleRuntimeAuthority(spec);
 // One frozen environment for every fresh native executable, before seed/ledger reads.
 const nativeEnv=nativeRuntimeEnvironment(process.env);
 const nativeExec=(program,args,options={})=>exec(program,args,{...options,env:nativeRuntimeEnvironment(options.env===undefined?nativeEnv:options.env)});
 need(!output.includes(',')&&await realpath(dirname(output))===dirname(output),'New output parent must be canonical');
 const within=(a,b)=>a===b||a.startsWith(b+'/');
 for(const z of [spec.fixture.seedFile,spec.fixture.ledgerDirectory,...names.flatMap(n=>[spec.owners[n].coreBuildRoot,spec.owners[n].networkBuildRoot,...freshSoftwareLocators(n,spec.owners[n]).map(row=>row.hostPath)])])need(!within(output,z)&&!within(z,output),'Fresh output/code/seed/ledger roots must be disjoint');
 const seedStat=await lstat(spec.fixture.seedFile),ledgerStat=await lstat(spec.fixture.ledgerDirectory);
 need(await realpath(spec.fixture.seedFile)===spec.fixture.seedFile&&seedStat.isFile()&&seedStat.nlink===1&&seedStat.size===32&&(seedStat.mode&0o777)===0o600&&seedStat.uid===process.getuid(),'Fresh externally provisioned 32-byte seed0600 required');
 need(await realpath(spec.fixture.ledgerDirectory)===spec.fixture.ledgerDirectory&&ledgerStat.isDirectory()&&(ledgerStat.mode&0o777)===0o700&&ledgerStat.uid===process.getuid(),'Persistent operator ledger0700 required');
 for(const path of [spec.overlayManifest.path,...names.map(name=>spec.owners[name].admission.path),spec.fixture.legacyPublicIdentity.path])diagnosticPublicPaths.add(resolve(path));
 await observe(spec.overlayManifest.path,spec.overlayManifest,true);
 for(const name of names){const a=spec.owners[name];validateFreshOwnerAdmission(JSON.parse(await observe(a.admission.path,a.admission,true)),spec,name);}
 const legacy=JSON.parse(await observe(spec.fixture.legacyPublicIdentity.path,spec.fixture.legacyPublicIdentity,true));
 await mkdir(output,{mode:0o700});const record=async row=>appendFile(join(output,'run.jsonl'),JSON.stringify({timeMs:Date.now(),...row})+'\n',{mode:0o600});
 const quote=s=>"'"+String(s).replaceAll("'","'\\''")+"'",sshArgs=['-o','BatchMode=yes','-o','ConnectTimeout=8'];
 const sh=async(a,timeout=30000)=>(await exec('ssh',[...sshArgs,target,a.map(quote).join(' ')],{timeout,maxBuffer:16*1024*1024})).stdout;
 // stdin is used only by the bounded local known-file transport. Private bytes
 // never enter argv or the run log; timed-out owned containers are retained for
 // the existing exact UUID/image cleanup and the campaign remains FAILED.
 async function dockerInput(args,input,timeout){
  need(Buffer.isBuffer(input)&&input.length>0&&input.length<=134217728,'Bounded known stdin');const child=tracked(spawn('docker',args,{stdio:['pipe','pipe','pipe']})),parts=[[],[]],sizes=[0,0];let timed=false,overflow=false;
  const done=new Promise(resolve=>{child.once('error',()=>resolve({code:null,signal:null}));child.once('close',(code,signal)=>resolve({code,signal}));});
  for(const [i,stream]of [child.stdout,child.stderr].entries())stream.on('data',b=>{sizes[i]+=b.length;if(sizes[i]<=67108864)parts[i].push(b);else{overflow=true;child.kill('SIGKILL');}});child.stdin.on('error',()=>{});child.stdin.end(input);
  const timer=setTimeout(()=>{timed=true;child.kill('SIGKILL');},timeout);let exit;try{exit=await done;}finally{clearTimeout(timer);}need(!timed&&!overflow&&exit.code===0&&exit.signal===null,'Known stdin operation refused');return Buffer.concat(parts[0]);
 }
 const ownedRuns=[];let operation=0;const docker=async(a,remote=false,timeout=30000,input=null)=>{need(input===null||!remote,'Local stdin transport only');if(a[0]==='run'){a=[...a];const at=a.indexOf('--name'),name=at>=0?a[at+1]:'nano-qualification-op-'+uuid+'-'+(++operation);if(at<0)a.splice(1,0,'--name',name);a.splice(1,0,'--label',label);ownedRuns.push({name,remote});}return remote?sh(['sudo','-n','docker',...a],timeout):input===null?(await exec('docker',a,{timeout,maxBuffer:16*1024*1024})).stdout:dockerInput(a,input,timeout);};
 const uuid=randomUUID(),remoteRoot='/home/'+target.split('@')[0]+'/nano-fresh-'+uuid,containers={ 'mac-docker':'nano-qualification-mac-'+uuid,'vps-docker':'nano-qualification-vps-'+uuid};
 const label='nano.fresh.campaign='+uuid,assets=join(output,'assets'),prepared=join(output,'prepared'),journals=join(output,'imported-journals'),proof=join(output,'bootstrap-proof'),partitionNetwork=containers['vps-docker']+'-net';
 let partitionNetworkCreated=false,partitionNetworkAbsent=true,freshRotationProof;const freshPeerIds=new Map();
 const configAt=name=>name==='vps-docker'?remoteRoot+'/config':join(prepared,name),owner=name=>spec.owners[name],mounts=name=>['--mount',`type=bind,src=${owner(name).coreBuildRoot},dst=/outputs,readonly`,'--mount',`type=bind,src=${owner(name).networkBuildRoot},dst=/network-build,readonly`,...freshSoftwareFileMounts(name,owner(name),{protectedPaths:[spec.fixture.seedFile,spec.fixture.ledgerDirectory,output,remoteRoot]})];
 const safe=name=>['--pull=never','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user',owner(name).uid+':'+owner(name).gid,'--memory','3g','--cpus','2','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=64m',...mounts(name)];
 // Fixed public software paths only. No SDK/private build receipts/maps leave
 // the VM; ROOT's separately retained admission supplies their provenance.
 const metadata='set -eu; while [ "$#" -gt 0 ]; do role=$1; path=$2; shift 2; test "$(realpath -- "$path")" = "$path"; test ! -L "$path"; test -f "$path"; exec 9<"$path"; before=$(stat -Lc "%d:%i:%f:%u:%g:%h:%s:%y:%z" /proc/self/fd/9); test "$before" = "$(stat -c "%d:%i:%f:%u:%g:%h:%s:%y:%z" "$path")"; test "$(stat -Lc %h /proc/self/fd/9)" = 1; size=$(stat -Lc %s /proc/self/fd/9); test "$size" -gt 0; test "$size" -le 1073741824; hash=$(sha256sum /proc/self/fd/9); hash=${hash%% *}; if [ "$role" = coreArchive ]; then magic=$(od -An -tx1 -N8 /proc/self/fd/9 | tr -d " \n"); test "$magic" = 213c617263683e0a; arch=archive; else magic=$(od -An -tx1 -N4 /proc/self/fd/9 | tr -d " \n"); test "$magic" = 7f454c46; arch=$(od -An -tu2 -j18 -N2 /proc/self/fd/9 | tr -d " \n"); fi; test "$before" = "$(stat -Lc "%d:%i:%f:%u:%g:%h:%s:%y:%z" /proc/self/fd/9)"; test "$before" = "$(stat -c "%d:%i:%f:%u:%g:%h:%s:%y:%z" "$path")"; test "$(realpath -- "$path")" = "$path"; printf "%s %s %s %s\n" "$role" "$hash" "$size" "$arch"; exec 9<&-; done';
 const hostMetadataMarker='printf "%s %s %s %s\n" "$role" "$hash" "$size" "$arch";';need(metadata.split(hostMetadataMarker).length===2,'Exact existing metadata output derivation');
 const hostMetadata=metadata.replace(hostMetadataMarker,'materialHash=$(printf "%s" "$before" | sha256sum); materialHash=${materialHash%% *}; printf "%s %s %s %s %s\n" "$role" "$hash" "$size" "$arch" "$materialHash";'),hostSoftwareMaterials=new Map();
 async function explicitHostSoftware(name,a){
  const selected=freshSoftwareLocators(name,a).filter(row=>!row.legacy);if(!selected.length)return;
  if(name==='mac-docker'){for(const row of selected){diagnosticPublicPaths.add(row.hostPath);await observe(row.hostPath,{sha256:row.sha256,bytes:row.bytes});}return;}
  need(name==='vps-docker','Only actual Linux host source projection');
  // Probe the real HOST before any file bind, never compare VM/container inode
  // namespaces. Known artifact bytes precede the unique material admission.
  const raw=await sh(['sh','-ec',hostMetadata,'sh',...selected.flatMap(row=>[row.role,row.hostPath])]);
  const expected=new Map(selected.map(row=>[row.role,row])),seen=new Set(),lines=raw.trim().split('\n');need(lines.length===selected.length,'Exact explicit host software roles');
  for(const line of lines){const m=/^([A-Za-z0-9]+) ([0-9a-f]{64}) ([1-9][0-9]*) (archive|62|183) ([0-9a-f]{64})$/.exec(line);need(m&&expected.has(m[1])&&!seen.has(m[1]),'Only fixed host software fields may egress');seen.add(m[1]);const pin=expected.get(m[1]);need(m[2]===pin.sha256&&Number(m[3])===pin.bytes&&m[4]===(pin.role==='coreArchive'?'archive':'62'),'Explicit host file must match external pin and architecture');const key=name+'\0'+pin.role;need(!hostSoftwareMaterials.has(key)||hostSoftwareMaterials.get(key)===m[5],'Previously admitted host software nine fields changed');if(!hostSoftwareMaterials.has(key))hostSoftwareMaterials.set(key,m[5]);}
 }
 async function software(){const rows=[];for(const name of names){const a=owner(name);
  if(name==='mac-native'){for(const [role,z] of Object.entries({coreArchive:a.coreArchive,...a.programs})){
   diagnosticPublicPaths.add(resolve(z.path));const got=await observe(z.path,z);if(role!=='coreArchive'){const f=await open(z.path,'r');try{const b=Buffer.alloc(8);await f.read(b,0,8,0);need(b.readUInt32LE(0)===0xfeedfacf&&b.readUInt32LE(4)===0x0100000c,'Native Mach-O arm64 required');}finally{await f.close();}}rows.push({owner:name,role,sha256:got.sha256,bytes:got.bytes,architecture:role==='coreArchive'?'archive':'arm64'});
  }}else{
   await explicitHostSoftware(name,a);const info=JSON.parse(await docker(['image','inspect',a.image,'--format','{{json .}}'],name==='vps-docker'));need(info.Id===a.image&&info.Architecture===(name==='vps-docker'?'amd64':'arm64'),'Existing image identity/architecture');
   const selected={coreArchive:a.coreArchive,...a.programs},raw=await docker(['run','--rm',...safe(name),'--entrypoint','sh',a.image,'-ec',metadata,'sh',...Object.entries(selected).flatMap(([role,z])=>[role,z.path])],name==='vps-docker');
   const got=raw.trim().split('\n').map(line=>{const m=/^([A-Za-z0-9]+) ([0-9a-f]{64}) ([1-9][0-9]*) (archive|62|183)$/.exec(line);need(m,'Only fixed public software fields may egress');return {role:m[1],sha256:m[2],bytes:Number(m[3]),architecture:m[4]==='archive'?'archive':Number(m[4])};});need(Array.isArray(got)&&got.length===Object.keys(selected).length&&new Set(got.map(z=>z.role)).size===got.length,'Exact fixed software roles');for(const z of got){need(Object.keys(z).sort().join(',')==='architecture,bytes,role,sha256'&&selected[z.role]&&z.sha256===selected[z.role].sha256&&z.bytes===selected[z.role].bytes&&z.architecture===(z.role==='coreArchive'?'archive':name==='vps-docker'?62:183),'Selected public ELF/archive differs');rows.push({owner:name,...z});}
  }
 }return rows;}
 let initialSoftware,macLinux,macConfig,macBootstrap,vpsBootstrapTransport;
 const configMount=name=>{if(name==='mac-docker'){need(macConfig,'Admitted Linux configuration generation');return macConfig.mounts;}return ['--mount',`type=bind,src=${configAt(name)},dst=/config,readonly`];};
 const macNodeBytes=()=>{need(macConfig,'Current immutable Mac Linux node');return Buffer.from(macConfig.node);};
 async function macConfigGeneration(nodeBytes=null){const files=[{name:'isolated-domain.json',bytes:await observe(join(assets,'isolated-domain.json'),null,true)},...await Promise.all(Object.values(manifest.files).map(async d=>({name:d.path,bytes:await observe(join(assets,d.path),d,true)})))];macConfig=await macLinux.generation(files,nodeBytes??await readFile(join(prepared,'mac-docker','node.json')));await macLinux.verifyAll();}
 async function ownedRuntime(name,running=null){const x=JSON.parse(await docker(['inspect',containers[name],'--format','{{json .}}'],name==='vps-docker'));need(x.Name==='/'+containers[name]&&x.Image===owner(name).image&&x.Config.Labels?.['nano.fresh.campaign']===uuid&&(running===null||x.State.Running===running),'Exact owned runtime image/UUID/state');return x;}
 async function macBootstrapPrepare({documents,planBytes}){
  const selected=owner('mac-docker'),v=volumes.get('mac-docker');macBootstrap=await macLinux.bootstrap({documents,planBytes,importer:selected.programs.bootstrapImport.path,auditor:selected.programs.bootstrapAudit.path,dataVolume:v.data,witnessVolume:v.witness,exports:{isHeld:name=>held.has(join(assets,name)),observe:(name,expected=null,withBytes=false,admit=true)=>observe(join(assets,name),expected,withBytes,admit)}});
 }
 const initialRuntimeClockChecks=new Map();
 async function initialClockInput(name){
  need(names.includes(name),'Exact initial runtime owner');const node=JSON.parse(name==='mac-docker'?macNodeBytes():await observe(join(prepared,name,'node.json'),null,true,false)),profile=JSON.parse(await observe(join(assets,'profile.json'),manifest.files.profile,true));return {plan,node,profile,approvedClock:spec.clock};
 }
 async function initialRuntimeLaunch(name,launch){
  need(!initialRuntimeClockChecks.has(name),'Initial runtime clock gate is one-shot per owner');const input=await initialClockInput(name);
  if(interrupted)throw interrupted;
  return launchFreshInitialRuntime(input,()=>BigInt(Date.now()),proof=>{initialRuntimeClockChecks.set(name,proof);return launch();});
 }
 const started=new Set(),volumes=new Map(),originalVolumes=new Map();let native,tunnel,failure,manifest,plan,preparedManifest,remotePeer,restoreReceipt,restoreStopEnvelope,restoreAudit,restoredMounts;
 let lifecycleNativeLog=join(output,'native.log');let interrupted;const onSignal=()=>{interrupted??=Error('Fresh campaign interrupted');};process.once('SIGINT',onSignal);process.once('SIGTERM',onSignal);
 const lifecycleWholeTimer=spec.lifecycle?setTimeout(()=>{interrupted??=Error('Fresh lifecycle whole-run deadline');},spec.lifecycle.deadlines.maximumRunMs):null;
 // Track close from spawn time: exitCode alone does not join stdio/handles.
 const closedChildren=new WeakSet(),exited=child=>child.exitCode!==null||child.signalCode!==null;
 const childAbsent=child=>!child||exited(child)&&closedChildren.has(child);
 function tracked(child){child.once('close',()=>closedChildren.add(child));return child;}
 async function waitJoined(child,milliseconds){const until=performance.now()+milliseconds;while(!childAbsent(child)&&performance.now()<until)await delay(50);return childAbsent(child);}
 async function terminateJoined(child,name,termMilliseconds){
  if(!child)return false;let forced=false;if(!exited(child))child.kill('SIGTERM');
  if(!await waitJoined(child,termMilliseconds)){if(!exited(child)){child.kill('SIGKILL');forced=true;}need(await waitJoined(child,5000),name+' exit/close join deadline');}
  need(childAbsent(child),name+' still has an unjoined process handle');return forced;
 }
 const runtimeRoot=name=>name==='mac-native'?join(prepared,name):'/var/lib/nano-data',rootFor=(name,role)=>name==='mac-native'?preparedManifest.nodes.find(n=>n.name===name)[role==='data'?'dataRoot':'witnessRoot']:join(role==='data'?'/var/lib/nano-data':'/var/lib/nano-witness',plan.campaignNamespace,name);
 const dataMount=(name,readOnly=false)=>['--mount',`type=volume,src=${volumes.get(name).data},dst=/var/lib/nano-data${readOnly?',readonly':''}`,'--mount',`type=volume,src=${volumes.get(name).witness},dst=/var/lib/nano-witness${readOnly?',readonly':''}`];
 const logs=async name=>{if(name==='mac-native')return spec.lifecycle?lifecycleLogTail(lifecycleNativeLog):readFile(lifecycleNativeLog,'utf8');need(name!=='vps-docker','VPS raw logs remain in situ');const x=await exec('docker',['logs',containers[name]],{timeout:30000,maxBuffer:16*1024*1024});return x.stdout+x.stderr;};
 const remoteProjection=String.raw`import os,sys,json,hashlib,stat,subprocess,pathlib,re,time
root=pathlib.Path(sys.argv[1]);root.mkdir(mode=0o700,exist_ok=True)
error=os.open(root/'projection-errors.private.log',os.O_WRONLY|os.O_CREAT|os.O_APPEND|os.O_NOFOLLOW,0o600);os.dup2(error,2);os.close(error)
def need(v):
 if not v:raise ValueError('fixed remote projection refusal')
def saved(p,b):
 p.parent.mkdir(mode=0o700,parents=True,exist_ok=True);fd=os.open(p,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 try:
  v=memoryview(b)
  while v:v=v[os.write(fd,v):]
  os.fsync(fd);os.lseek(fd,0,os.SEEK_SET);need(os.read(fd,len(b)+1)==b and not os.read(fd,1));s=os.fstat(fd);t=os.lstat(p);need(s.st_ino==t.st_ino and s.st_dev==t.st_dev and s.st_nlink==1 and stat.S_ISREG(s.st_mode))
 finally:os.close(fd)
 d=os.open(p.parent,os.O_RDONLY|os.O_DIRECTORY);os.fsync(d);os.close(d);return hashlib.sha256(b).hexdigest()
def read(p):
 p=pathlib.Path(p);need(p.is_relative_to(root));fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  s=os.fstat(fd);need(stat.S_ISREG(s.st_mode) and s.st_nlink==1 and 0<s.st_size<=67108864);b=b''
  while len(b)<s.st_size:
   x=os.read(fd,min(1048576,s.st_size-len(b)));need(bool(x));b+=x
  need(not os.read(fd,1));t=os.fstat(fd);u=os.lstat(p);fields=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns');need(all(getattr(s,k)==getattr(t,k)==getattr(u,k) for k in fields));return b
 finally:os.close(fd)
def transport_trees(archives):
 need(type(archives) is list and len(archives)==2 and os.geteuid()==1000 and os.getegid()==1000)
 top=os.lstat(root);need(root.resolve()==root and stat.S_ISDIR(top.st_mode) and top.st_uid==1000 and top.st_gid==1000 and stat.S_IMODE(top.st_mode)==0o700)
 deadline=time.monotonic()+55
 def tick():need(time.monotonic()<=deadline)
 def field(b):
  head,sep,tail=b.partition(b'\0');need(not sep or not any(tail));return head.decode('utf-8')
 def octal(b):
  need(re.fullmatch(b' *[0-7]+[\x00 ]*',b));return int(b.strip(b'\x00 '),8)
 def parse(raw):
  need(len(raw)%512==0);at=0;members={};ended=False
  while at+512<=len(raw):
   tick();h=raw[at:at+512]
   if h==bytes(512):need(len(raw)-at>=1024 and not any(raw[at:]));ended=True;break
   need(h[257:263]==b'ustar\0' and h[263:265]==b'00' and octal(h[148:156])==sum(h[:148])+8*32+sum(h[156:]))
   name=field(h[:100]);prefix=field(h[345:500]);path=(prefix+'/' if prefix else '')+name;kind=h[156:157];size=octal(h[124:136]);mode=octal(h[100:108]);octal(h[108:116]);octal(h[116:124]);octal(h[136:148])
   need(kind in (b'0',b'\0',b'5') and not any(h[157:257]));directory=kind==b'5'
   need(bool(path) and not path.startswith('/'))
   if directory and path in ('.','./'):path=''
   elif path.startswith('./'):path=path[2:]
   if directory and path.endswith('/'):path=path[:-1]
   need(path=='' and directory or path and not path.startswith('/') and not any(c in path for c in '\0\r\n') and all(p not in ('','.','..') for p in path.split('/')))
   need(path not in members and len(members)<10000 and mode in ((0o700,0o755) if directory else (0o600,0o644)) and (not directory or size==0))
   start=at+512;end=start+size;at=start+((size+511)//512)*512;need(end<=at<=len(raw) and not any(raw[end:at]));members[path]=(directory,mode,start,end)
  need(ended and '' in members and members[''][0])
  for path in members:
   if path:parent=path.rpartition('/')[0];need(parent in members and members[parent][0])
  return members
 validated=[]
 fields=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
 for name,z in zip(('config','proof'),archives):
  tick();need(type(z) is dict and set(z)=={'name','sha256','bytes'} and z['name']==name and type(z['bytes']) is int and 0<z['bytes']<=16777216 and re.fullmatch('[0-9a-f]{64}',z['sha256']))
  p=root/('transport-'+name+'.ustar');need(p.resolve()==p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
  try:
   s=os.fstat(fd);need(stat.S_ISREG(s.st_mode) and s.st_nlink==1 and s.st_uid==1000 and s.st_gid==1000 and stat.S_IMODE(s.st_mode)==0o600 and s.st_size==z['bytes']);chunks=[];remaining=s.st_size
   while remaining:tick();b=os.read(fd,min(1048576,remaining));need(bool(b));chunks.append(b);remaining-=len(b)
   need(not os.read(fd,1));raw=b''.join(chunks);t=os.fstat(fd);u=os.lstat(p);need(all(getattr(s,k)==getattr(t,k)==getattr(u,k) for k in fields) and p.resolve()==p and hashlib.sha256(raw).hexdigest()==z['sha256'])
  finally:os.close(fd)
  validated.append((name,raw,parse(raw)))
 files=directories=0
 for name,raw,members in validated:
  tick();destination=root/name;os.mkdir(destination,0o700)
  for path,(directory,mode,start,end) in sorted(members.items(),key=lambda row:(row[0].count('/'),row[0])):
   if directory and path:os.mkdir(destination/path,0o700)
  for path,(directory,mode,start,end) in members.items():
   tick()
   if directory:continue
   p=destination/path;fd=os.open(p,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
   try:
    view=memoryview(raw)[start:end]
    while view:tick();n=os.write(fd,view);need(n>0);view=view[n:]
    os.fchmod(fd,mode);os.fsync(fd);s=os.fstat(fd);need(stat.S_ISREG(s.st_mode) and s.st_nlink==1 and s.st_uid==1000 and s.st_gid==1000 and stat.S_IMODE(s.st_mode)==mode and s.st_size==end-start);os.lseek(fd,0,os.SEEK_SET);position=start
    while position<end:tick();b=os.read(fd,min(1048576,end-position));need(bool(b) and b==raw[position:position+len(b)]);position+=len(b)
    need(not os.read(fd,1));t=os.fstat(fd);u=os.lstat(p);need(all(getattr(s,k)==getattr(t,k)==getattr(u,k) for k in fields) and p.resolve()==p)
   finally:os.close(fd)
   files+=1
  for path,(directory,mode,start,end) in sorted(members.items(),key=lambda row:row[0].count('/'),reverse=True):
   if directory:
    p=destination/path;fd=os.open(p,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
    try:os.fchmod(fd,mode);os.fsync(fd);s=os.fstat(fd);t=os.lstat(p);need(all(getattr(s,k)==getattr(t,k) for k in fields) and s.st_uid==1000 and s.st_gid==1000 and stat.S_IMODE(s.st_mode)==mode and p.resolve()==p)
    finally:os.close(fd)
    directories+=1
 tick();d=os.open(root,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 try:os.fsync(d)
 finally:os.close(d)
 return {'format':'nano-fresh-capella-two-ustar-extraction-v1','passed':True,'archivesVerified':2,'filesExtracted':files,'directoriesExtracted':directories,'hostOwnershipImported':False,'rawArchiveExported':False,'runtimeStarted':False,'productionQualified':False}
def token(t):
 need(type(t) is dict and set(t)=={'sequence','digest'} and re.fullmatch('0x[1-9a-f][0-9a-f]*',t['sequence']) and re.fullmatch('0x[0-9a-f]{64}',t['digest']));return t
def checkpoint(t):
 need(type(t) is dict and set(t)=={'epoch','root'} and re.fullmatch('(?:0|[1-9][0-9]*|0x(?:0|[1-9a-f][0-9a-f]*))',t['epoch']) and re.fullmatch('0x[0-9a-f]{64}',t['root']));return t
def command(argv,prefix):
 need(type(argv) is list and all(type(x) is str for x in argv));outputs=[root/(prefix+'-stdout.private.json'),root/(prefix+'-stderr.private.log')];fds=[]
 try:
  for p in outputs:fds.append(os.fdopen(os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600),'wb',buffering=0))
  z=subprocess.run(argv,stdout=fds[0],stderr=fds[1],timeout=180,check=False)
 finally:
  for f in fds:os.fsync(f.fileno());f.close()
 saved(root/(prefix+'-exit.json'),json.dumps({'exitCode':z.returncode}).encode());need(z.returncode==0);return read(outputs[0])

try:
 x=json.loads(sys.argv[2]);mode=x['mode'];prefix=x['prefix'];need(re.fullmatch('[a-z0-9-]+',prefix));result=None
 if mode=='extract-transport':
  need(set(x)=={'mode','prefix','archives'});result=transport_trees(x['archives'])
 elif mode=='stop-log':
  raw=command(x['argv'],prefix);rows=[]
  for line in raw.decode().splitlines():
   try:r=json.loads(line)
   except ValueError:continue
   if r.get('event')=='capella-runtime-stopped':rows.append(r)
  need(len(rows)==1);r=rows[0];v=r['validator'];need(v['healthy'] is True and v['failure'] is None and r['clockHealthy'] is True)
  keys=[]
  for k in r['validatorKeys']:
   q={'index':k['index'],'restart':token(k['restart'])}
   if k['index'] is None:need(re.fullmatch('0x[0-9a-f]{96}',k['publicKey']));q['publicKey']=k['publicKey']
   else:need(re.fullmatch('0x(?:0|[1-9a-f][0-9a-f]*)',k['index']))
   keys.append(q)
  counters={k:v[k] for k in ('localKeys','awaitingRegistration','proposalsAdmitted','attestationsAdmitted','syncMessagesAdmitted')};need(all(type(z) is int and 0<=z<=18446744073709551615 for z in counters.values()));need(re.fullmatch('0x(?:0|[1-9a-f][0-9a-f]*)',r['justifiedEpoch']))
  result={'event':'capella-runtime-stopped','restart':token(r['restart']),'validatorKeys':keys,'finalized':checkpoint(r['finalized']),'justifiedEpoch':r['justifiedEpoch'],'clockHealthy':True,'validator':{'healthy':True,'failure':None,**counters}}
 elif mode in ('capture-pair','capture-once'):
  argv=x['argv'];raw=command(argv,prefix+'-first');first=json.loads(raw);need(first['passed'] is True and first['canonicalStoreSnapshotVerified'] is True and first['primaryAndWitnessBlobClosuresVerified'] is True);firstSha=hashlib.sha256(raw).hexdigest();last=first;lastSha=firstSha
  if mode=='capture-pair':
   stop=json.loads(read(x['stopFile']));pins={'format':'nano-isolated-stopped-external-witness-commitments-v1','sourceCaptureSha256':firstSha,'core':token(first['core']['witness']),'validatorKeys':[{'index':k['index'],**({'publicKey':k['publicKey']} if k['index'] is None else {}),'restart':token(k['witness'])} for k in first['validatorKeys']]};stop['witnessCommitments']=pins
   second=root/(prefix+'-stop-second.private.json');saved(second,json.dumps(stop,sort_keys=True,separators=(',',':')).encode());old='type=bind,src='+x['stopFile']+',dst=/stop.json,readonly';need(argv.count(old)==1);argv=[('type=bind,src='+str(second)+',dst=/stop.json,readonly') if z==old else z for z in argv];raw=command(argv,prefix+'-second');last=json.loads(raw);lastSha=hashlib.sha256(raw).hexdigest();need(last['passed'] is True and last['externalWitnessTokensMatched'] is True)
  need(last['owner']=='vps-docker' and re.fullmatch('[0-9a-f]{64}',last['campaignId']) and last['primaryWitnessPairs']==33 and last['databasesReadOnly']==68)
  flags=('canonicalStoreSnapshotVerified','primaryAndWitnessBlobClosuresVerified','allObservedFileBytesAndInodesUnchanged','observedWitnessPairsAuthenticated','externalStopTokensMatched','externalWitnessTokensMatched','externalWitnessPinOriginExternallyRequired');need(all(type(last[k]) is bool for k in flags))
  result={'format':'nano-fresh-capella-stopped-capture-public-v1','passed':True,'owner':'vps-docker','campaignId':last['campaignId'],'primaryWitnessPairs':33,'databasesReadOnly':68,'firstCaptureSha256':firstSha,'lastCaptureSha256':lastSha,**{k:last[k] for k in flags},'rawCapsuleExported':False,'productionQualified':False}
 elif mode=='bootstrap-audit':
  need(set(x)=={'mode','prefix','argv','expected'});expected=x['expected'];pins=('campaignId','manifestSha256','planDocumentSha256','planSha256','anchorsSha256','sourceInventorySha256');need(type(expected) is dict and set(expected)==set(pins)|{'keys'} and all(type(expected[k]) is str and re.fullmatch('[0-9a-f]{64}',expected[k]) for k in pins) and type(expected['keys']) is list and len(expected['keys'])==33)
  raw=command(x['argv'],prefix);r=json.loads(raw);need(r['format']=='nano-isolated-bootstrap-physical-audit-v1' and r['passed'] is True and r['owner']=='vps-docker' and r['primaryWitnessPairs']==33 and r['coldPairReopens']==66 and type(r['newSignaturesCreated']) is int and r['newSignaturesCreated']==0 and type(r['signerCallbacksInvoked']) is int and r['signerCallbacksInvoked']==0 and all(r[k]==expected[k] for k in pins))
  for k in ('privateKeysRead','envRead','journalWritesRequested','sourceTreeWritesRequested','runtimeStarted','networkUsed','productionQualified'):need(r[k] is False)
  need(type(r['perKey']) is list and len(r['perKey'])==33)
  for offset,(row,key) in enumerate(zip(r['perKey'],expected['keys'])):
   need(type(row) is dict and set(row)=={'publicKeyIndex','primaryCommitment','witnessCommitment','proposalSlot','attestationTarget','primaryInventorySha256','witnessInventorySha256'} and type(row['publicKeyIndex']) is int and row['publicKeyIndex']==key['publicKeyIndex']==32+offset and row['primaryCommitment']==key['runtimeKey']['restart'] and row['witnessCommitment']==key['expectedWitnessCommitment'] and row['proposalSlot']==key['importedHistory']['proposalSlot'] and row['attestationTarget'] is None and all(type(row[k]) is str and re.fullmatch('[0-9a-f]{64}',row[k]) for k in ('primaryInventorySha256','witnessInventorySha256')))
  flags=('fullFileBytesMatchSource','copiedFilesInodesUnchanged','namespaceAndKeyFloorsVerified');need(all(r[k] is True for k in flags));need(all(type(r[k]) is str and re.fullmatch('[0-9a-f]{64}',r[k]) for k in ('dataInventorySha256','witnessInventorySha256')));result={'passed':True,'owner':'vps-docker','primaryWitnessPairs':33,'coldPairReopens':66,'newSignaturesCreated':0,'journalWritesRequested':False,**{k:True for k in flags},**{k:expected[k] for k in pins},'reportSha256':hashlib.sha256(raw).hexdigest(),'rawCapsuleExported':False}
 elif mode=='rotation-http-digest':
  need(set(x)=={'mode','prefix','argv','blockRoot','sha256','bytes'} and re.fullmatch('0x[0-9a-f]{64}',x['blockRoot']) and re.fullmatch('[0-9a-f]{64}',x['sha256']) and type(x['bytes']) is int and 184<=x['bytes']<=8388608)
  raw=command(x['argv'],prefix);marker=b'\nNANO_ROTATION_HTTP_STATUS:';need(marker in raw);body,status=raw.rsplit(marker,1);need(status==b'200' and len(body)==x['bytes'] and hashlib.sha256(body).hexdigest()==x['sha256'])
  result={'format':'nano-fresh-rotation-http-digest-v1','blockRoot':x['blockRoot'],'sha256':x['sha256'],'bytes':x['bytes'],'status':200,'rawSszExported':False}
 elif mode=='rotation-owned-copy':
  need(set(x)=={'mode','prefix','argv','afterAudit'} and type(x['afterAudit']) is bool);raw=command(x['argv'],prefix);r=json.loads(raw)
  keys=('format','passed','databaseRoles','sourceFiles','sourceDirectories','payloadBytes','sourceNineFieldAndShaUnchanged','sourceNamespaceUnchanged','copiesMatchSourceBytes','independentInodes','privateDirectoryModes','privateFileModes','copiesUnchangedAfterAudit','copyProofSha256','stabilityProbeCount','stabilityElapsedMs','stabilitySpanMs','rawInventoryExported','productionQualified')
  need(type(r) is dict and set(r)==set(keys) and r['format']=='nano-fresh-rotation-owned-copy-v1' and r['passed'] is True and type(r['databaseRoles']) is int and r['databaseRoles']==2 and r['copiesUnchangedAfterAudit'] is x['afterAudit'] and r['rawInventoryExported'] is r['productionQualified'] is False and re.fullmatch('[0-9a-f]{64}',r['copyProofSha256']))
  for k in ('sourceNineFieldAndShaUnchanged','sourceNamespaceUnchanged','copiesMatchSourceBytes','independentInodes','privateDirectoryModes','privateFileModes'):need(r[k] is True)
  for k in ('sourceFiles','sourceDirectories','payloadBytes','stabilityProbeCount','stabilityElapsedMs','stabilitySpanMs'):need(type(r[k]) is int and 0<=r[k]<=1073741824)
  need(r['sourceFiles']<=40000 and r['sourceDirectories']<=40000 and r['payloadBytes']<=1073741824 and r['stabilityProbeCount']>=3 and 2000<=r['stabilitySpanMs']<=r['stabilityElapsedMs']<=30000);result=r
 elif mode=='rotation-audit':
  raw=command(x['argv'],prefix);r=json.loads(raw)
  keys=('format','status','passed','owner','campaignId','boundarySlot','minimumInclusionSlot','finalized','headRoot','headStateRoot','headSlot','registryCount','replayedBlocks','canonicalArchiveBytes','threeOwnerObservedBlocks','remoteBlocks','remotePositions','initialCommitteeHash','beforeBoundaryCommitteeHash','rotatedCommitteeHash','finalizedCommitteeHash','completePrimaryWitnessClosureVerified','externalCoreTokensMatched','physicalCopiesUnchanged','canonicalSignedStateReplayVerified','historicalForkChoiceVerified','newSignaturesCreated','databaseOpenMode','runtimeStarted','productionQualified','beforeBoundaryActiveValidators','beforeBoundaryTotalEffectiveBalance','macIslandEffectiveBalance','vpsIslandEffectiveBalance','neitherIslandHasSupermajority')
  need(type(r) is dict and set(r)==set(keys) and r['format']=='nano-fresh-capella-rotation-audit-v1' and r['status']=='PASS_SCOPED_FRESH_ROTATION_AUDIT' and r['passed'] is True and r['owner']=='vps-docker' and re.fullmatch('[0-9a-f]{64}',r['campaignId']) and type(r['newSignaturesCreated']) is int and r['newSignaturesCreated']==0 and r['databaseOpenMode']=='READ_ONLY' and r['historicalForkChoiceVerified'] is r['runtimeStarted'] is r['productionQualified'] is False)
  for k in ('completePrimaryWitnessClosureVerified','externalCoreTokensMatched','physicalCopiesUnchanged','canonicalSignedStateReplayVerified','neitherIslandHasSupermajority'):need(r[k] is True)
  for k in ('registryCount','replayedBlocks','canonicalArchiveBytes','threeOwnerObservedBlocks','remoteBlocks','remotePositions','beforeBoundaryActiveValidators'):need(type(r[k]) is int and 0<=r[k]<=18446744073709551615)
  for k in ('headRoot','headStateRoot','initialCommitteeHash','beforeBoundaryCommitteeHash','rotatedCommitteeHash','finalizedCommitteeHash'):need(type(r[k]) is str and re.fullmatch('0x[0-9a-f]{64}',r[k]))
  for k in ('boundarySlot','minimumInclusionSlot','headSlot','beforeBoundaryTotalEffectiveBalance','macIslandEffectiveBalance','vpsIslandEffectiveBalance'):need(type(r[k]) is str and re.fullmatch('0x(?:0|[1-9a-f][0-9a-f]*)',r[k]))
  checkpoint(r['finalized']);result={**r,'reportSha256':hashlib.sha256(raw).hexdigest(),'rawCapsuleExported':False}
 elif mode in ('audit-read','audit-run'):
  raw=read(x['path']) if mode=='audit-read' else command(x['argv'],prefix);r=json.loads(raw);need(r['canonicalHistoryVerified'] is True and r['externalFinalizedAncestorVerified'] is True and type(r['checkpointSequence']) is int and 0<r['checkpointSequence']<=18446744073709551615 and re.fullmatch('0x[0-9a-f]{64}',r['checkpointDigest']))
  result={'canonicalHistoryVerified':True,'externalFinalizedAncestorVerified':True,'checkpointSequence':r['checkpointSequence'],'checkpointDigest':r['checkpointDigest'],'requiredFinalized':checkpoint(r['requiredFinalized']),'finalized':checkpoint(r['finalized']),'reportSha256':hashlib.sha256(raw).hexdigest(),'rawCapsuleExported':False}
 else:raise ValueError('unknown projection mode')
 print(json.dumps({'format':'nano-fresh-capella-remote-public-projection-v1','passed':True,'result':result},separators=(',',':')))
except BaseException:
 import traceback;traceback.print_exc(file=sys.stderr);print('{"format":"nano-fresh-capella-remote-public-projection-v1","passed":false}')
`;

 async function remotePublic(payload,timeout=240000){const x=JSON.parse(await sh(['python3','-c',remoteProjection,remoteRoot,JSON.stringify(payload)],timeout));need(x.format==='nano-fresh-capella-remote-public-projection-v1'&&x.passed===true&&x.result,'Fixed remote projection refused; details remain VM');return x.result;}
 // Only NEW synthetic imports, before publishing any source inventory.
 // Once selected, that inventory is never rebased after a transport mismatch.
 async function stageBootstrapTransport(){
  need(macBootstrap&&macLinux&&!vpsBootstrapTransport,'One independent Linux bootstrap transport required');
  const configuration=await Promise.all(['genesis.json','profile.json','initial.json','isolated-domain.json','node.json','fixture.env'].map(async name=>({name,bytes:await observe(['node.json','fixture.env'].includes(name)?join(prepared,'vps-docker',name):join(assets,name),null,true,false)})));
  vpsBootstrapTransport=await macBootstrap.vpsTransport(configuration);
  try{
   const archives=[];for(const {name,bytes:buffer} of vpsBootstrapTransport.archives){
    if(interrupted)throw interrupted;need(Buffer.isBuffer(buffer)&&buffer.length>0&&buffer.length<=16*1024*1024,'Bounded known Linux USTAR required');const path=join(output,'transport-'+name+'.ustar'),bytes=await save(path,buffer);archives.push({name,path,sha256:sha(bytes),bytes:bytes.length});
   }
   if(interrupted)throw interrupted;await sh(['mkdir','-m','700',remoteRoot]);for(const z of archives)await observe(z.path,z,false,false);
   await exec('scp',['-q',...archives.map(z=>z.path),target+':'+remoteRoot+'/'],{timeout:120000,maxBuffer:16*1024*1024});if(interrupted)throw interrupted;for(const z of archives)await observe(z.path,z,false,false);
   const result=await remotePublic({mode:'extract-transport',prefix:'transport',archives:archives.map(({name,sha256,bytes})=>({name,sha256,bytes}))},60000);
   need(result&&Object.keys(result).sort().join(',')==='archivesVerified,directoriesExtracted,filesExtracted,format,hostOwnershipImported,passed,productionQualified,rawArchiveExported,runtimeStarted'&&result.format==='nano-fresh-capella-two-ustar-extraction-v1'&&result.passed===true&&result.archivesVerified===2&&Number.isSafeInteger(result.filesExtracted)&&result.filesExtracted>0&&result.filesExtracted<=20000&&Number.isSafeInteger(result.directoriesExtracted)&&result.directoriesExtracted>0&&result.directoriesExtracted<=20000&&result.hostOwnershipImported===false&&result.rawArchiveExported===false&&result.runtimeStarted===false&&result.productionQualified===false,'Fixed USTAR extraction result required');
   if(interrupted)throw interrupted;await save(join(output,'transport-public.json'),result);
  }finally{try{await macBootstrap.fenceSource();}finally{await macLinux.verifyAll();}}
 }
 // Only NEW public exports: no old HELD row is sampled into a new authority here.
 async function stablePublicExportInventory(){
  const began=performance.now(),deadlineMs=30000,deadline=began+deadlineMs,paths=publicExportLeaves.map(leaf=>join(assets,leaf));let probeCount=0,stableSamples=0,stableSince=0,previous;
  for(const path of paths){need(!held.has(path)&&!newFiles.has(path),'Public export stability must precede first admission');publicExportProbePaths.add(path);}
  while(performance.now()<deadline){
   if(interrupted)throw interrupted;const rows=[];
   for(const path of paths){need(performance.now()<deadline,'New public export pre-admission stability deadline');rows.push(await observe(path,null,false,false));need(performance.now()<=deadline,'New public export pre-admission stability deadline');}
   if(interrupted)throw interrupted;const fingerprint=canonical(rows),now=performance.now();probeCount++;
   if(fingerprint===previous)stableSamples++;else{previous=fingerprint;stableSamples=1;stableSince=now;}
   if(stableSamples>=3&&now-stableSince>=2000)return {inventory:new Map(rows.map(row=>[row.path,row])),report:{format:'nano-fresh-public-export-pre-admission-quiescence-v1',publicFiles:6,probeCount,elapsedMs:Math.floor(now-began),stableSpanMs:Math.floor(now-stableSince),deadlineMs,stableSamples,requiredStableSamples:3,sampleSpacingMs:1000,strictWholeMaterialNineAndSha256Equality:true,firstAdmissionNotYetPublished:true,postAdmissionRebaseline:false,runtimeStarted:false,productionQualified:false}};
   need(deadline-performance.now()>1000,'New public export pre-admission stability deadline');await delay(1000);
  }
  throw Error('New public export pre-admission stability deadline');
 }
 async function stableBootstrapInventory(program){
  const began=performance.now(),deadlineMs=30000,deadline=began+deadlineMs;let probeCount=0,stableSamples=0,stableSince=0,previous;
  while(performance.now()<deadline){
   if(interrupted)throw interrupted;const remaining=deadline-performance.now();need(remaining>=1,'New bootstrap pre-admission quiescence deadline');
   const text=(await nativeExec(program,['--inventory',journals],{timeout:Math.min(180000,Math.floor(remaining)),maxBuffer:16*1024*1024})).stdout;
   if(interrupted)throw interrupted;const inventory=JSON.parse(text),fingerprint=canonical(inventory),now=performance.now();probeCount++;need(now<=deadline,'New bootstrap pre-admission quiescence deadline');
   if(fingerprint===previous)stableSamples++;else{previous=fingerprint;stableSamples=1;stableSince=now;}
   if(stableSamples>=3&&now-stableSince>=2000)return {inventory,report:{format:'nano-fresh-bootstrap-pre-admission-quiescence-v1',probeCount,elapsedMs:Math.floor(now-began),stableSpanMs:Math.floor(now-stableSince),deadlineMs,stableSamples,requiredStableSamples:3,sampleSpacingMs:1000,strictWholeInventoryEquality:true,runtimeStarted:false,productionQualified:false}};
   need(deadline-performance.now()>1000,'New bootstrap pre-admission quiescence deadline');await delay(1000);
  }
  throw Error('New bootstrap pre-admission quiescence deadline');
 }
 async function stopped(name,prefixHint=null){
  if(name==='mac-native'){need(native&&native.pid,'Native process was not started');const forced=await terminateJoined(native,'Native runtime',30000);need(!forced,'Native clean-stop deadline required SIGKILL');need(native.exitCode===0,'Native runtime did not exit cleanly');}
  else{await ownedRuntime(name,true);await docker(['stop','--time','20',containers[name]],name==='vps-docker',30000);const x=JSON.parse(await docker(['inspect',containers[name],'--format','{{json .State}}'],name==='vps-docker'));need(x.Running===false&&x.ExitCode===0&&x.OOMKilled===false,'Runtime clean stopped state required');}
  started.delete(name);if(name==='vps-docker'){const prefix=prefixHint??'stopped-vps-'+(restoreReceipt?'restored':'initial'),receipt=await remotePublic({mode:'stop-log',prefix,argv:['sudo','-n','docker','logs',containers[name]]});stoppedReceipt(JSON.stringify(receipt));await save(join(output,prefix+'-public.json'),receipt);return {text:JSON.stringify(receipt),receipt};}const text=await logs(name);await save(join(output,prefixHint?prefixHint+'.private.log':name+'-'+(name==='vps-docker'&&restoreReceipt?'restored':'initial')+'.log'),text);return {text,receipt:stoppedReceipt(text)};
 }
 async function request(name,path,body){if(interrupted)throw interrupted;const argv=['-sS','--max-time','5','-H','Accept: application/json','--write-out',httpStatusTrailer];if(body)argv.push('-H','Content-Type: application/json','--data',JSON.stringify(body));argv.push('http://127.0.0.1:'+(name==='mac-native'?18545:8545)+path);const text=name==='mac-native'?(await exec('curl',argv,{timeout:8000,maxBuffer:16*1024*1024})).stdout:await docker(['exec',containers[name],'curl',...argv],name==='vps-docker');return decodeHttpResponse(text,name,path);}
 async function rpc(name,method,params=[]){const x=await request(name,'/',{jsonrpc:'2.0',id:1,method,params});need(!x.error,'Fresh RPC returned an error');return x.result;}
 async function finality(baseline,phase,bootstrap,timeoutMs=480000){const observer=isolatedRuntimeObservation({baseline,...(bootstrap?{bootstrapStateCheckpoint:isolatedBootstrapStateCheckpoint(JSON.parse(await readFile(join(assets,'initial.json'),'utf8')))}:{}),requiredAdvances:phase==='initial'?2:3,requiredStableSamples:3});const until=Date.now()+timeoutMs;
  while(Date.now()<until){const rows=[];for(const name of names){try{const row=await observer.sample({name,read:p=>request(name,p),peers:await rpc(name,'net_peerCount')});if(row)rows.push(row);}catch(e){if(!isolatedPendingObservation(e))throw e;}}
   const completed=observer.observe(rows);await record({event:'fresh-finality-sample',phase,rows,progress:observer.snapshot()});if(completed){const execution=[];for(const name of names){const block=(await request(name,'/eth/v2/beacon/blocks/'+completed.root)).data.message,payload=block.body.execution_payload.block_hash,head=await rpc(name,'eth_getBlockByNumber',['0x'+BigInt(block.body.execution_payload.block_number).toString(16),false]);need(head?.hash===payload&&BigInt(head.number)===BigInt(block.body.execution_payload.block_number),'Finalized checkpoint must bind execution');execution.push(payload);}need(execution.every(h=>h===execution[0]),'Three finalized executions differ');if(spec.lifecycle)need(Date.now()<until,'Lifecycle common-finality deadline before publication');const snapshot=observer.snapshot(),result={epochs:snapshot.distinctAdvances.length,firstEpoch:baseline.epoch,lastEpoch:completed.epoch,lastCheckpoint:completed,lastExecutionHash:execution[0]};await save(join(output,phase+'-finality.json'),{result,observation:snapshot,physicalHosts:2,runtimes:3,productionQualified:false});if(spec.lifecycle)need(Date.now()<until,'Lifecycle common-finality deadline after publication');return {result,observation:snapshot};}await delay(3000);
  }throw Error('Fresh common-finality deadline');
 }
 async function auditCopy(name){if(interrupted)throw interrupted;const remote=name==='vps-docker',a=owner(name),cfg=configAt(name);let text;
  if(name==='mac-native')text=(await nativeExec(a.programs.bootstrapAudit.path,[join(assets,'isolated-domain.json'),join(proof,'plan.json'),join(proof,'anchors.json'),join(proof,'source-inventory.json'),name,rootFor(name,'data'),rootFor(name,'witness')],{timeout:180000,maxBuffer:16*1024*1024})).stdout;
  else if(name==='mac-docker'){need(macBootstrap&&macConfig,'Independent Linux bootstrap proof and configuration required');return macBootstrap.auditCopy(macConfig.volume);}
  else{const argv=['run','--rm',...safe(name),...dataMount(name,true),...configMount(name),'--mount',`type=bind,src=${remoteRoot+'/proof'},dst=/proof,readonly`,'--entrypoint',a.programs.bootstrapAudit.path,a.image,'/config/isolated-domain.json','/proof/plan.json','/proof/anchors.json','/proof/source-inventory.json',name,rootFor(name,'data'),rootFor(name,'witness')];if(remote){const ownName='nano-qualification-op-'+uuid+'-'+(++operation);ownedRuns.push({name:ownName,remote:true});argv.splice(1,0,'--name',ownName,'--label',label);need(vpsBootstrapTransport,'Held Linux transport pins required');text=JSON.stringify(await remotePublic({mode:'bootstrap-audit',prefix:'bootstrap-vps-audit',expected:vpsBootstrapTransport.expected,argv:['sudo','-n','docker',...argv]}));}else text=await docker(argv,false,180000);}
  const r=JSON.parse(text);need(r.passed===true&&r.owner===name&&r.primaryWitnessPairs===(name==='vps-docker'?33:16)&&r.newSignaturesCreated===0&&r.journalWritesRequested===false&&r.fullFileBytesMatchSource===true&&r.copiedFilesInodesUnchanged===true,'Actual destination journal audit required');if(remote)for(const k of ['campaignId','manifestSha256','planDocumentSha256','planSha256','anchorsSha256','sourceInventorySha256'])need(r[k]===vpsBootstrapTransport.expected[k],'VPS audit must bind original Linux source evidence');await save(join(output,name+'-bootstrap-audit.json'),r);
 }
 const rotationAuditFields=['format','status','passed','owner','campaignId','boundarySlot','minimumInclusionSlot','finalized','headRoot','headStateRoot','headSlot','registryCount','replayedBlocks','canonicalArchiveBytes','threeOwnerObservedBlocks','remoteBlocks','remotePositions','initialCommitteeHash','beforeBoundaryCommitteeHash','rotatedCommitteeHash','finalizedCommitteeHash','completePrimaryWitnessClosureVerified','externalCoreTokensMatched','physicalCopiesUnchanged','canonicalSignedStateReplayVerified','historicalForkChoiceVerified','newSignaturesCreated','databaseOpenMode','runtimeStarted','productionQualified','beforeBoundaryActiveValidators','beforeBoundaryTotalEffectiveBalance','macIslandEffectiveBalance','vpsIslandEffectiveBalance','neitherIslandHasSupermajority'];
 function validateRotationAudit(r,name){
  const extra=name==='vps-docker'?['reportSha256','rawCapsuleExported']:[];need(r&&Object.keys(r).sort().join(',')===[...rotationAuditFields,...extra].sort().join(',')&&r.format==='nano-fresh-capella-rotation-audit-v1'&&r.status==='PASS_SCOPED_FRESH_ROTATION_AUDIT'&&r.passed===true&&r.owner===name&&r.campaignId===manifest.campaignId&&r.boundarySlot===freshRotationProof.boundarySlot&&r.minimumInclusionSlot===freshRotationProof.minimumInclusionSlot&&canonical(r.finalized)===canonical(freshRotationProof.finalized)&&r.newSignaturesCreated===0&&r.databaseOpenMode==='READ_ONLY'&&r.historicalForkChoiceVerified===false&&r.runtimeStarted===false&&r.productionQualified===false,'Exact fresh rotation audit scope required');
  for(const key of ['completePrimaryWitnessClosureVerified','externalCoreTokensMatched','physicalCopiesUnchanged','canonicalSignedStateReplayVerified','neitherIslandHasSupermajority'])need(r[key]===true,'Rotation RO/token/replay/quorum requirement');
  for(const key of ['registryCount','replayedBlocks','canonicalArchiveBytes','threeOwnerObservedBlocks','remoteBlocks','remotePositions','beforeBoundaryActiveValidators'])need(Number.isSafeInteger(r[key])&&r[key]>=0,'Bounded actual rotation counter');
  need(r.registryCount>=64&&r.registryCount<=65&&r.replayedBlocks>0&&r.replayedBlocks<=257&&r.beforeBoundaryActiveValidators>0&&r.beforeBoundaryActiveValidators<=r.registryCount&&r.canonicalArchiveBytes<=24*1024*1024&&r.threeOwnerObservedBlocks>=3&&r.remoteBlocks>=3&&r.remotePositions>=3,'Rotation replay/participation bounds');
  for(const key of ['headRoot','headStateRoot','initialCommitteeHash','beforeBoundaryCommitteeHash','rotatedCommitteeHash','finalizedCommitteeHash'])need(/^0x[0-9a-f]{64}$/.test(r[key]),'Exact public rotation commitment');
  for(const key of ['boundarySlot','minimumInclusionSlot','headSlot','beforeBoundaryTotalEffectiveBalance','macIslandEffectiveBalance','vpsIslandEffectiveBalance'])need(/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(r[key]),'Canonical public rotation quantity');
  need(r.beforeBoundaryCommitteeHash!==r.rotatedCommitteeHash&&r.initialCommitteeHash!==r.rotatedCommitteeHash&&BigInt(r.beforeBoundaryTotalEffectiveBalance)>0n&&BigInt(r.macIslandEffectiveBalance)+BigInt(r.vpsIslandEffectiveBalance)===BigInt(r.beforeBoundaryTotalEffectiveBalance)&&3n*BigInt(r.macIslandEffectiveBalance)<2n*BigInt(r.beforeBoundaryTotalEffectiveBalance)&&3n*BigInt(r.vpsIslandEffectiveBalance)<2n*BigInt(r.beforeBoundaryTotalEffectiveBalance),'Real committee change and neither island supermajority required');
  if(name==='vps-docker')need(r.rawCapsuleExported===false&&/^[0-9a-f]{64}$/.test(r.reportSha256),'VPS rotation report fixed projection');return r;
 }
 async function captureFreshRotationBlocks(finalizedRoot,minimumInclusionSlot){
  const capture=createRotationCapture(),marker=Buffer.from('\nNANO_ROTATION_HTTP_STATUS:');let tip=finalizedRoot;
  for(let depth=0;tip!==manifest.initialHeadRoot;++depth){
   if(interrupted)throw interrupted;need(depth<256,'Complete rotation ancestry work bound');
   for(const name of ['mac-native','mac-docker']){
    const argv=['-sS','--max-time','5','-H','Accept: application/octet-stream','--write-out',marker.toString()+'%{http_code}','http://127.0.0.1:'+(name==='mac-native'?18545:8545)+'/eth/v2/beacon/blocks/'+tip],command=name==='mac-native'?'curl':'docker',args=name==='mac-native'?argv:['exec',containers[name],'curl',...argv];
    const raw=(await exec(command,args,{encoding:'buffer',timeout:15000,maxBuffer:8*1024*1024+64})).stdout,at=raw.lastIndexOf(marker);need(at>=0&&raw.subarray(at+marker.length).toString()==='200','Signed rotation HTTP request must actually return200');capture.add(name,tip,raw.subarray(0,at));
   }
   const expected=capture.digest(tip),r=await remotePublic({mode:'rotation-http-digest',prefix:'rotation-http-'+(++operation),blockRoot:tip,...expected,argv:['sudo','-n','docker','exec',containers['vps-docker'],'curl','-sS','--max-time','5','-H','Accept: application/octet-stream','--write-out',marker.toString()+'%{http_code}','http://127.0.0.1:8545/eth/v2/beacon/blocks/'+tip]});
   need(r&&Object.keys(r).sort().join(',')==='blockRoot,bytes,format,rawSszExported,sha256,status'&&r.format==='nano-fresh-rotation-http-digest-v1'&&r.blockRoot===tip&&r.status===200&&r.rawSszExported===false,'Only in-situ VPS public block digest may egress');capture.addDigest('vps-docker',tip,r.sha256,r.bytes);tip=capture.parent(tip);
  }
  return capture.freshProof(manifest.initialHeadRoot,finalizedRoot,minimumInclusionSlot);
 }
 async function runFreshPartitionRotation(initial,profileBytes){
  const parameters=freshRotationParameters(profileBytes),monitor=freshPartitionFinalityObserver(initial.observation),options=spec.partitionRotation;await save(join(output,'partition-rotation-profile.json'),{...parameters,source:'pinned-public-profile',boundaryFixedBeforeWindow:true,majorityIslandClaimed:false,productionQualified:false});
  const readHeads=async()=>{const heads=[];for(const name of names){const x=(await request(name,'/eth/v1/beacon/headers/head')).data;need(/^0x[0-9a-f]{64}$/.test(x?.root)&&typeof x?.header?.message?.slot==='string'&&/^(?:0|[1-9][0-9]*)$/.test(x.header.message.slot)&&BigInt(x.header.message.slot)<=BigInt(Number.MAX_SAFE_INTEGER),'Exact current public head');heads.push({name,root:x.root,slot:Number(x.header.message.slot)});}return heads;};
  const sample=async()=>{
   const deadline=performance.now()+30000;
   while(performance.now()<deadline){
    if(interrupted)throw interrupted;const rows=[];let pending=false;
    for(const name of names){try{const peers=await rpc(name,'net_peerCount'),row=await monitor.sample(name,path=>request(name,path),peers);if(!row){pending=true;break;}const info=await rpc(name,'nno_getNetworkInfo');need(info.node?.peerId===freshPeerIds.get(name)&&Array.isArray(info.peers?.connected),'Unchanged fault peer identity');rows.push({...row,peerId:info.node.peerId,connectedPeerIds:info.peers.connected.map(p=>p.peerId)});}catch(e){if(!isolatedPendingObservation(e))throw e;pending=true;break;}}
    if(!pending&&rows.length===3){for(const [a,b] of [['mac-native','mac-docker'],['mac-docker','mac-native']])need(rows.find(r=>r.name===a).connectedPeerIds.includes(freshPeerIds.get(b)),'Direct Mac island link must remain connected');return rows;}
    await record({event:'fresh-partition-pending-authenticated-finality',reason:'genuine-state/header-pair-or-recognized-read-capacity'});need(deadline-performance.now()>500,'Bounded partition finality sampling deadline');await delay(500);
   }throw Error('Bounded partition finality sampling deadline');
  };
  await waitRotationWindow({sample:async()=>{await sample();return readHeads();},record,clock:()=>performance.now(),boundary:parameters.boundarySlot,lead:parameters.leadSlots,timeoutMs:options.windowTimeoutMs});let cutoff;
  const faultRecord=async event=>{await record(event);if(event.event==='partition-disconnected'){const heads=await readHeads();need(heads.every(h=>h.slot<parameters.boundarySlot),'Fixed rotation boundary missed before effective partition');await save(join(output,'partition-rotation-disconnected.json'),{heads,boundarySlot:parameters.boundarySlot,quorumLossExpected:true,instantFinalityFreezeClaimed:false,productionQualified:false});}if(event.event==='partition-reconnected')await save(join(output,'partition-rotation-reconnected.json'),{heads:await readHeads(),boundarySlot:parameters.boundarySlot,minimumInclusionSlot:cutoff,productionQualified:false});};
  await partitionRemote({docker:args=>docker(args,true),container:containers['vps-docker'],network:partitionNetwork,revision,freshCampaign:uuid,sample,record:faultRecord,cutMs:parameters.cutMs,recoveryMs:options.recoveryMs,afterCut:async()=>{const heads=await readHeads();need(heads.every(h=>h.slot>=parameters.boundarySlot),'Partition did not cross the actual period boundary on all owners');cutoff=Math.max(...heads.map(h=>h.slot));await save(join(output,'partition-rotation-isolated-cutoff.json'),{heads,boundarySlot:parameters.boundarySlot,minimumInclusionSlot:cutoff,observedBeforeReconnect:true,productionQualified:false});}});need(Number.isSafeInteger(cutoff)&&cutoff>=parameters.boundarySlot,'Verified post-partition cutoff');
  const recovered=(await sample())[0].checkpoint,completed=await finality(recovered,'partition-recovered',false),checkpoint=completed.result.lastCheckpoint;
  freshRotationProof={format:'nano-fresh-capella-rotation-proof-v1',campaignId:manifest.campaignId,inputManifestSha256:sha(await observe(join(assets,'isolated-domain.json'),null,true)),boundarySlot:'0x'+parameters.boundarySlot.toString(16),minimumInclusionSlot:'0x'+cutoff.toString(16),finalized:{epoch:'0x'+BigInt(checkpoint.epoch).toString(16),root:checkpoint.root},ownership:freshRotationOwnership(manifest,preparedManifest),blocks:await captureFreshRotationBlocks(checkpoint.root,cutoff)};
  await save(join(output,'partition-rotation-proof.json'),freshRotationProof);await record({event:'fresh-partition-rotation-awaiting-stopped-readonly-audits',finalized:freshRotationProof.finalized,boundarySlot:freshRotationProof.boundarySlot,minimumInclusionSlot:freshRotationProof.minimumInclusionSlot,publicCapturedBlocks:Object.keys(freshRotationProof.blocks).length,productionQualified:false});
 }
 // MacDocker's individual file bind may expose UID0. Keep original inputs
 // untouched and stage exact bytes through one owner-private directory bind.
 async function stageMacDockerRotationInputs(stopPath,proofPath){
  const paths=[stopPath,proofPath].map(p=>resolve(p)),maximum=[16*1024*1024,64*1024*1024],originals=[],buffers=[];
  async function originalFence(){for(const row of originals){const got=await observe(row.path,row,false,false);inputNeed(canonical(got)===canonical(row),'Original rotation audit input material changed',row.path,rowChanges(row,got));}}
  for(let i=0;i<2;++i){
   const path=paths[i],known=held.get(path)??newFiles.get(path);need(known,'Known rotation audit input byte authority required');const row=await observe(path,known,false,false);need(row.bytes>0&&row.bytes<=maximum[i],'Rotation audit input byte bound');originals.push(row);
   const f=await open(path,fsConstants.O_RDONLY|fsConstants.O_NOFOLLOW|fsConstants.O_NONBLOCK),hash=createHash('sha256'),chunks=[];let size=0;
   try{const before=await f.stat({bigint:true});need(canonical(material(before))===canonical(row.material),'Rotation source FD differs from its first material authority');const chunk=Buffer.alloc(65536);for(;;){if(interrupted)throw interrupted;const {bytesRead:n}=await f.read(chunk,0,chunk.length,null);if(!n)break;size+=n;need(size<=row.bytes,'Rotation input grew');hash.update(chunk.subarray(0,n));chunks.push(Buffer.from(chunk.subarray(0,n)));}need(size===row.bytes&&hash.digest('hex')===row.sha256&&canonical(material(await f.stat({bigint:true})))===canonical(row.material)&&canonical(material(await lstat(path,{bigint:true})))===canonical(row.material),'Rotation source bytes or nine fields changed during read');}finally{await f.close();}
   buffers.push(Buffer.concat(chunks));
  }
  await originalFence();const temporary=await realpath(tmpdir());for(const root of [output,...paths.map(dirname)])need(!within(root,temporary)&&!within(temporary,root),'MacDocker audit inputs require a temporary parent outside originals');
  const directory=await mkdtemp(join(temporary,'nano-fresh-rotation-inputs-'+uuid+'-')),directoryStat=await lstat(directory,{bigint:true});need(await realpath(directory)===directory&&directoryStat.isDirectory()&&directoryStat.uid===BigInt(process.getuid())&&(Number(directoryStat.mode)&0o7777)===0o700,'Private MacDocker audit input directory required');
  const targets=['stop.json','proof.json'].map(leaf=>join(directory,leaf)),pins=buffers.map(bytes=>({sha256:sha(bytes),bytes:bytes.length}));
  await save(join(output,'rotation-audit-inputs-mac-docker-location.private.json'),{format:'nano-fresh-rotation-mac-input-location-private-v1',directory,retained:true,deletionRequested:false,productionQualified:false});
  try{
   for(let i=0;i<2;++i){need(!held.has(targets[i]),'New audit input must not overlap an old authority');await save(targets[i],buffers[i]);}
   const modes=materialFields.indexOf('mode'),uids=materialFields.indexOf('uid'),ids=new Set(originals.map(row=>row.material[0]+':'+row.material[1]));
   async function snapshot(){
    need(await realpath(directory)===directory,'Canonical staged input directory required');const f=await open(directory,fsConstants.O_RDONLY|fsConstants.O_NOFOLLOW|fsConstants.O_NONBLOCK);
    try{const before=await f.stat({bigint:true});need(before.isDirectory()&&before.uid===BigInt(process.getuid())&&(Number(before.mode)&0o7777)===0o700&&canonical(material(before))===canonical(material(await lstat(directory,{bigint:true})))&&canonical((await readdir(directory)).sort())===canonical(['proof.json','stop.json']),'Exact private staged input namespace required');const rows=[],seen=new Set(ids);seen.add(String(before.dev)+':'+String(before.ino));for(let i=0;i<2;++i){const row=await observe(targets[i],pins[i],false,false),id=row.material[0]+':'+row.material[1];need(!seen.has(id)&&BigInt(row.material[uids])===BigInt(process.getuid())&&(Number(row.material[modes])&0o7777)===0o600,'Private independent rotation audit input required');seen.add(id);rows.push(row);}need(canonical(material(before))===canonical(material(await f.stat({bigint:true})))&&canonical(material(before))===canonical(material(await lstat(directory,{bigint:true}))),'Staged input directory changed during observation');return {directory:{path:directory,material:material(before)},files:rows};}finally{await f.close();}
   }
   const began=performance.now(),deadline=began+30000;let last=null,count=0,series=0,probes=0,span=0,admitted;
   while(performance.now()<deadline){if(interrupted)throw interrupted;await originalFence();const next=await snapshot(),now=performance.now();need(now<deadline,'Rotation audit input first-admission deadline');++probes;if(last&&canonical(last)===canonical(next))++count;else{count=1;series=now;}last=next;span=now-series;if(count>=3&&span>=2000){admitted=next;break;}need(deadline-performance.now()>1000,'Rotation audit input first-admission deadline');await delay(1000);}
   need(admitted&&count>=3&&span>=2000&&performance.now()<deadline,'Three strict new audit input snapshots required');
   async function destinationFence(){const got=await snapshot();inputNeed(canonical(got.directory)===canonical(admitted.directory),'Admitted MacDocker input directory material changed',directory,rowChanges(admitted.directory,got.directory));for(let i=0;i<2;++i)inputNeed(canonical(got.files[i])===canonical(admitted.files[i]),'Admitted MacDocker audit input material changed',targets[i],rowChanges(admitted.files[i],got.files[i]));}
   await originalFence();await destinationFence();need(performance.now()<deadline,'Rotation audit input first-admission deadline');const summary={format:'nano-fresh-rotation-mac-private-input-stage-v1',passed:true,inputFiles:2,knownBytesExact:true,sourceNineFieldsAndShaUnchanged:true,newIndependentFiles:true,privateDirectoryMode:'0700',privateFileMode:'0600',firstAdmissionProbeCount:probes,firstAdmissionStableSpanMs:Math.floor(span),firstAdmissionElapsedMs:Math.ceil(performance.now()-began),directoryBindReadOnly:true,rawInputsExported:false,productionQualified:false};
   return {directory,summary,async verify(){await originalFence();await destinationFence();return {...summary,strictAfterAuditFencePassed:true};}};
  }catch(e){await originalFence();throw e;}
 }
 async function auditFreshRotation(terminal){
  need(freshRotationProof&&terminal.length===3,'Complete fresh rotation proof and stopped owners required');const proofPath=join(output,'partition-rotation-proof.json'),audits=[];await exec('scp',['-q',proofPath,target+':'+remoteRoot+'/partition-rotation-proof.json'],{timeout:60000});
  // The GC inventory is intentionally stricter than runtime directory creation.
  // Only new independent core copies become0700/0600; original roots stay RO.
  for(const name of names){
   if(interrupted)throw interrupted;const a=owner(name),stopName='terminal-'+name+'-stop-second.json';let result,copied;
   if(name==='mac-native'){
    // NEW owner-private OS-temp copies only; the runtime and all source roots
    // retain their existing locations, modes, material authorities and bytes.
    const temporary=await realpath(tmpdir());need(resolve(temporary)===temporary&&!temporary.includes(','),'Canonical OS temporary directory required');
    for(const root of [output,rootFor(name,'data'),rootFor(name,'witness')])need(!within(root,temporary)&&!within(temporary,root),'Native audit temporary parent must be outside campaign/source roots');
    const copyParent=await mkdtemp(join(temporary,'nano-fresh-rotation-audit-'+uuid+'-')),copyParentStat=await lstat(copyParent,{bigint:true});need(await realpath(copyParent)===copyParent&&copyParentStat.isDirectory()&&copyParentStat.uid===BigInt(process.getuid())&&(Number(copyParentStat.mode)&0o7777)===0o700,'Fresh owner-private temporary audit directory required');
    for(const root of [output,rootFor(name,'data'),rootFor(name,'witness')]){need(!within(root,copyParent)&&!within(copyParent,root),'Temporary audit copy must be disjoint from originals');const before=await lstat(root,{bigint:true});need(before.dev!==copyParentStat.dev||before.ino!==copyParentStat.ino,'Temporary copy root must have an independent inode');}
    await save(join(output,'rotation-audit-copy-'+name+'-location.private.json'),{format:'nano-fresh-rotation-native-copy-location-private-v1',owner:name,copyParent,destination:join(copyParent,'owned-core'),retained:true,deletionRequested:false,productionQualified:false});
    const copy=await createNativeRotationCopies({dataRoot:rootFor(name,'data'),witnessRoot:rootFor(name,'witness'),destination:join(copyParent,'owned-core'),checkpoint:()=>{if(interrupted)throw interrupted;}});await save(join(output,'rotation-audit-copy-'+name+'-before.json'),copy.summary);
    try{result=JSON.parse((await nativeExec(a.programs.rotationAudit.path,['--fresh',join(prepared,name,'isolated-domain.json'),join(prepared,name,'node.json'),join(output,stopName),proofPath,copy.dataRoot,copy.witnessRoot,name],{timeout:180000,maxBuffer:1024*1024})).stdout);}finally{copied=await copy.verify();await save(join(output,'rotation-audit-copy-'+name+'-after.json'),copied);}
   }else{
    const remote=name==='vps-docker',volume=containers[name]+'-rotation-audit';
    for(const v of Object.values(volumes.get(name)))need(!(await docker(['ps','--filter','volume='+v,'--format','{{.Names}}'],remote)).trim(),'Source core volumes must remain stopped before copying');
    need(!(await docker(['volume','ls','--filter','name=^'+volume+'$','--format','{{.Name}}'],remote)).trim(),'Fresh audit copy volume required');await docker(['volume','create','--label',label,volume],remote);const identity=JSON.parse(await docker(['volume','inspect',volume,'--format','{{json .}}'],remote));need(identity.Name===volume&&identity.Labels?.['nano.fresh.campaign']===uuid,'Only fresh owned audit volume allowed');
    const copyMount=readOnly=>['--mount',`type=volume,src=${volume},dst=/rotation-audit${readOnly?',readonly':''}`];
    await docker(['run','--rm','--pull=never','--network','none','--read-only','--user','0:0','--cap-drop','ALL','--cap-add','CHOWN','--security-opt','no-new-privileges',...copyMount(false),'--entrypoint','sh',a.image,'-ec','test -z "$(ls -A /rotation-audit)"; chmod 700 /rotation-audit; chown "$1:$2" /rotation-audit','sh',String(a.uid),String(a.gid)],remote);
    const copyCommand=async(mode,pin)=>{const argv=['run','--rm',...safe(name),...dataMount(name,true),...copyMount(mode==='verify'),'--entrypoint','perl',a.image,'-e',linuxRotationCopy,rootFor(name,'data')+'/chain',rootFor(name,'witness')+'/chain-witness','/rotation-audit',mode,pin];let raw;if(remote){const ownName='nano-qualification-op-'+uuid+'-'+(++operation);ownedRuns.push({name:ownName,remote:true});argv.splice(1,0,'--name',ownName,'--label',label);raw=await remotePublic({mode:'rotation-owned-copy',prefix:'rotation-owned-copy-'+mode,afterAudit:mode==='verify',argv:['sudo','-n','docker',...argv]});}else raw=JSON.parse(await docker(argv,false,180000));return validateRotationCopySummary(raw,mode==='verify');};
    const before=await copyCommand('create','-');await save(join(output,'rotation-audit-copy-'+name+'-before.json'),before);
    let inputStage;
    try{
     const stopFile=remote?remoteRoot+'/terminal-vps-docker-stop-second.private.json':join(output,stopName),proofFile=remote?remoteRoot+'/partition-rotation-proof.json':proofPath;
     if(!remote){const v=await macLinux.create('rotation-inputs');await macLinux.write(v,'/proof',[{name:'stop.json',bytes:await observe(stopFile,null,true)},{name:'proof.json',bytes:await observe(proofFile,null,true)}],'rotation-inputs');inputStage={mounts:macLinux.mount(v,'/audit-inputs'),summary:{format:'nano-fresh-rotation-mac-linux-input-stage-v1',passed:true,inputFiles:2,knownBytesExact:true,linuxOwnedVolume:true,rawInputsExported:false,productionQualified:false},verify:async()=>{await macLinux.verifyAll();return {passed:true,strictAfterAuditFencePassed:true,productionQualified:false};}};await save(join(output,'rotation-audit-inputs-mac-docker-before.json'),inputStage.summary);}
     const inputMounts=remote?['--mount',`type=bind,src=${stopFile},dst=/stop.json,readonly`,'--mount',`type=bind,src=${proofFile},dst=/rotation-proof.json,readonly`]:inputStage.mounts,argv=['run','--rm',...safe(name),...copyMount(true),...configMount(name),...inputMounts,'--entrypoint',a.programs.rotationAudit.path,a.image,'--fresh','/config/isolated-domain.json','/config/node.json',remote?'/stop.json':'/audit-inputs/stop.json',remote?'/rotation-proof.json':'/audit-inputs/proof.json','/rotation-audit/data','/rotation-audit/witness',name];
     if(remote){const ownName='nano-qualification-op-'+uuid+'-'+(++operation);ownedRuns.push({name:ownName,remote:true});argv.splice(1,0,'--name',ownName,'--label',label);result=await remotePublic({mode:'rotation-audit',prefix:'partition-rotation-audit',argv:['sudo','-n','docker',...argv]});}else result=JSON.parse(await docker(argv,false,180000));
    }finally{try{copied=await copyCommand('verify',before.copyProofSha256);await save(join(output,'rotation-audit-copy-'+name+'-after.json'),copied);}finally{if(inputStage)await save(join(output,'rotation-audit-inputs-mac-docker-after.json'),await inputStage.verify());}}
   }
   audits.push(validateRotationAudit(result,name));
  }
  for(const r of audits)for(const key of ['campaignId','boundarySlot','minimumInclusionSlot','finalized','beforeBoundaryCommitteeHash','rotatedCommitteeHash','beforeBoundaryTotalEffectiveBalance','macIslandEffectiveBalance','vpsIslandEffectiveBalance'])need(canonical(r[key])===canonical(audits[0][key]),'Stopped canonical rotation audits disagree');await save(join(output,'partition-rotation-audits.json'),audits);
 }
 async function startDocker(name,restoredMounts,lifecycleLaunch=null,initial=false){if(interrupted)throw interrupted;need(!(await docker(['ps','-a','--filter','name=^/'+containers[name]+'$','--format','{{.Names}}'],name==='vps-docker')).trim(),'New runtime exact name absence');const a=owner(name),launch=()=>docker(['run','--pull=never','--init','-d','--name',containers[name],'--label',label,'--label','nano.qualification=multihost','--label','org.opencontainers.image.revision='+revision,'--user',a.uid+':'+a.gid,'--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','3g','--cpus','2','--ulimit','nofile=65536:65536','--tmpfs','/tmp:rw,nosuid,nodev,noexec,size=64m',...mounts(name),...(name==='vps-docker'?['-p','127.0.0.1:30307:30303',...(spec.partitionRotation?['--network',partitionNetwork]:[])]:spec.partitionRotation?['-p','127.0.0.1:30306:30303']:[]),...configMount(name),...(restoredMounts??dataMount(name)),...(lifecycleLaunch?.mounts??[]),...(lifecycleLaunch?.env??[]),'--env-file',configAt(name)+'/fixture.env',a.image,a.programs.nano.path,'run','/config/genesis.json',rootFor(name,'data')+'/chain','/config/node.json'],name==='vps-docker');if(initial)await initialRuntimeLaunch(name,launch);else await launch();started.add(name);}
 async function captureOwner(name,stop,path,mountOverride){if(interrupted)throw interrupted;const bytes=await save(path,stop),a=owner(name);let raw;
  if(name==='mac-native')raw=(await nativeExec(a.programs.stoppedCapture.path,['--capture',join(prepared,name,'isolated-domain.json'),join(prepared,name,'node.json'),path,rootFor(name,'data'),rootFor(name,'witness'),name],{timeout:180000,maxBuffer:16*1024*1024})).stdout;
  else{need(name==='mac-docker'&&macBootstrap,'VPS complete capsules stay in situ');const remote=false,stopPin=(await macLinux.write(macBootstrap.proofVolume,'/proof',[{name:basename(path),bytes}],'capture-stop'))[0],stopFile=stopPin.path;await macLinux.verifyAll();const selected=mountOverride??(remote&&restoredMounts?restoredMounts.map(x=>x.startsWith('type=')?x+',readonly':x):dataMount(name,true));
   raw=await docker(['run','--rm',...safe(name),...selected,...configMount(name),...macLinux.mount(macBootstrap.proofVolume,'/proof'),'--entrypoint',a.programs.stoppedCapture.path,a.image,'--capture','/config/isolated-domain.json','/config/node.json',stopFile,rootFor(name,'data'),rootFor(name,'witness'),name],remote,180000);
  }
  if(name==='mac-docker')await macLinux.verifyAll();const result=JSON.parse(raw);need(result.passed===true&&result.owner===name&&result.campaignId===manifest.campaignId&&result.manifestSha256===sha(await readFile(join(assets,'isolated-domain.json')))&&result.externalStopReceiptSha256===sha(bytes)&&result.canonicalStoreSnapshotVerified===true&&result.primaryAndWitnessBlobClosuresVerified===true,'Complete stopped P/W capture and exact public inputs required');return result;
 }
 function witnessStop(receipt,capture,bytes){return {...receipt,witnessCommitments:{format:'nano-isolated-stopped-external-witness-commitments-v1',sourceCaptureSha256:sha(bytes),core:capture.core.witness,validatorKeys:capture.validatorKeys.map(k=>k.index===null?{index:null,publicKey:k.publicKey,restart:k.witness}:{index:k.index,restart:k.witness})}};}
 function captureVmArgv(stopFile,mountOverride){const a=owner('vps-docker'),name='nano-qualification-op-'+uuid+'-'+(++operation);ownedRuns.push({name,remote:true});return ['sudo','-n','docker','run','--rm','--name',name,'--label',label,...safe('vps-docker'),...(mountOverride??(restoredMounts?restoredMounts.map(x=>x.startsWith('type=')?x+',readonly':x):dataMount('vps-docker',true))),'--mount',`type=bind,src=${configAt('vps-docker')},dst=/config,readonly`,'--mount',`type=bind,src=${stopFile},dst=/stop.json,readonly`,'--entrypoint',a.programs.stoppedCapture.path,a.image,'--capture','/config/isolated-domain.json','/config/node.json','/stop.json',rootFor('vps-docker','data'),rootFor('vps-docker','witness'),'vps-docker'];}
 async function capturePair(name,receipt,prefix){
  if(name==='vps-docker'){const local=join(output,prefix+'-stop-first.json'),stopFile=remoteRoot+'/'+prefix+'-stop-first.json';await save(local,receipt);await exec('scp',['-q',local,target+':'+stopFile],{timeout:10000});const projected=await remotePublic({mode:'capture-pair',prefix,stopFile,argv:captureVmArgv(stopFile)});need(projected.rawCapsuleExported===false&&projected.externalWitnessTokensMatched===true&&projected.campaignId===manifest.campaignId,'Scoped VPS capture proof required');await save(join(output,prefix+'-capture-public.json'),projected);return {remoteStopFile:remoteRoot+'/'+prefix+'-stop-second.private.json'};}
  const first=await captureOwner(name,receipt,join(output,prefix+'-stop-first.json')),firstBytes=await save(join(output,prefix+'-capture-first.json'),first),pins=witnessStop(receipt,first,firstBytes);const second=await captureOwner(name,pins,join(output,prefix+'-stop-second.json'));need(second.externalWitnessTokensMatched===true&&second.externalWitnessPinOriginExternallyRequired===true,'Retained external witness pin must match');await save(join(output,prefix+'-capture-second.json'),second);return pins;
 }

 // Lifecycle-only adapter. Ordinary recovery/rotation branches do not call it.
 const lifecycleArtifacts=new Map(),lifecycleSourceFences=[],lifecycleTerminalCaptures=new Map(),lifecycleRemoteIndexes=[],lifecycleRemoteFiles=[];let lifecyclePending,lifecycleQualified=false,lifecycleSequence=0;
 const lifecycleCheck=()=>{if(interrupted)throw interrupted;};
 async function lifecycleLogTail(path,strict=true){const f=await open(path,fsConstants.O_RDONLY|fsConstants.O_NOFOLLOW|fsConstants.O_NONBLOCK);try{const a=await f.stat({bigint:true});need(a.isFile()&&a.nlink===1n&&a.uid===BigInt(process.getuid())&&(Number(a.mode)&0o7777)===0o600,'Private managed log');const size=Number(a.size>16777216n?16777216n:a.size),bytes=Buffer.alloc(size),start=a.size-BigInt(size);let at=0;while(at<size){const x=await f.read(bytes,at,size-at,Number(start)+at);need(x.bytesRead>0,'Managed log truncated');at+=x.bytesRead;}if(strict)need(canonical(material(a))===canonical(material(await f.stat({bigint:true})))&&canonical(material(a))===canonical(material(await lstat(path,{bigint:true}))),'Stopped managed log changed');return bytes.toString();}finally{await f.close();}}
 const lifecyclePrefix=label=>'lifecycle-'+label+'-'+(++lifecycleSequence);
 const lifecycleJson=v=>Buffer.from(canonical(v));
 function lifecycleRemoteDocker(argv){need(argv[0]==='run','Explicit owner operation');const name='nano-qualification-op-'+uuid+'-'+(++operation);ownedRuns.push({name,remote:true});return ['sudo','-n','docker','run','--name',name,'--label',label,...argv.slice(1)];}
 const lifecycleRemoteGuest=p=>{need(within(p,remoteRoot)&&p!==remoteRoot,'Only owner-local lifecycle artifact');return '/lifecycle-remote'+p.slice(remoteRoot.length);};
 async function lifecycleRemote(x,timeout=650000){need(spec.lifecycle.privateVpsLifecycleTransferAuthorized===true,'Private VPS lifecycle transfer not authorized');const r=JSON.parse(await sh(['python3','-c',lifecycleOwnerProjection,remoteRoot,JSON.stringify(x)],timeout));need(r.format==='nano-fresh-lifecycle-owner-public-v1'&&r.passed===true&&r.result,'Owner-local lifecycle refusal; private details remain VPS');return r.result;}
 async function lifecycleWriteRemote(path,bytes){const pin={path,sha256:sha(bytes),bytes:bytes.length};const r=await lifecycleRemote({mode:'write-known',path,hex:bytes.toString('hex'),sha256:pin.sha256,bytes:pin.bytes},40000);need(r.written===true&&r.knownBytesPinnedBeforeWrite===true&&r.sha256===pin.sha256&&r.bytes===pin.bytes,'Known remote output pin');pin.materialSha256=r.materialSha256;lifecycleRemoteFiles.push(pin);return pin;}
 async function lifecycleRuntimeConfig(name,receipt,shim=false){const path=join(prepared,name,'node.json'),previous=JSON.parse(name==='mac-docker'?macNodeBytes():await readFile(path,'utf8')),identities=preparedManifest.nodes.find(n=>n.name===name).signerIdentities;
  const configured=pinnedRestoreConfiguration(previous,receipt,identities);configured.p2p_binary=owner(name).programs[shim?'lifecycleSubmit':'p2p'].path;const bytes=Buffer.from(JSON.stringify(configured,null,2)+'\n');if(name==='mac-docker'){await macConfigGeneration(bytes);return configured;}await writeFile(path,bytes,{mode:0o600});await observe(path,{sha256:sha(bytes),bytes:bytes.length},false,false);
  if(name==='vps-docker'){need(spec.lifecycle.privateVpsLifecycleTransferAuthorized===true,'Private configuration transfer not authorized');await exec('scp',['-q',path,target+':'+configAt(name)+'/node.json'],{timeout:10000});await observe(path,{sha256:sha(bytes),bytes:bytes.length},false,false);}return configured;
 }
 async function lifecycleResume(name,receipt,control=null){
  await lifecycleRuntimeConfig(name,receipt,Boolean(control));const a=owner(name);if(name!=='mac-native'){await ownedRuntime(name,false);await docker(['rm',containers[name]],name==='vps-docker');}
  const launch=control?{mounts:control.mounts??['--mount',`type=bind,src=${control.directory},dst=/lifecycle-owner`,...(control.extraMounts??[])],env:['--env','NANO_TEST_FRESH_SUBMISSION_PLAN='+control.planPath,'--env','NANO_TEST_FRESH_SUBMISSION_PLAN_SHA256='+control.planSha256]}:null;
  if(name==='mac-native'){
   const nd=join(prepared,name),environment=Object.fromEntries((await readFile(join(nd,'fixture.env'),'utf8')).trim().split('\n').map(s=>{const at=s.indexOf('=');return [s.slice(0,at),s.slice(at+1)];})),logPath=join(output,lifecyclePrefix('native-runtime')+'.private.log'),fd=openSync(logPath,'wx',0o600);
   lifecycleNativeLog=logPath;try{native=tracked(spawn(a.programs.nano.path,['run',join(nd,'genesis.json'),rootFor(name,'data')+'/chain',join(nd,'node.json')],{env:nativeRuntimeEnvironment(nativeEnv,{...environment,...(control?{NANO_TEST_FRESH_SUBMISSION_PLAN:control.planPath,NANO_TEST_FRESH_SUBMISSION_PLAN_SHA256:control.planSha256}:{})}),stdio:['ignore',fd,fd]}));}finally{closeSync(fd);}let error;native.once('error',e=>{error=e;});await delay(50);need(native.pid&&!error,'Actual lifecycle managed supervisor spawn');started.add(name);
  }else await startDocker(name,name==='vps-docker'?restoredMounts:null,launch);
  const until=performance.now()+60000;let peer;while(performance.now()<until){lifecycleCheck();try{peer=(await rpc(name,'nno_getNetworkInfo')).node?.peerId;if(peer)break;}catch{lifecycleCheck();}await delay(500);}need(peer===freshPeerIds.get(name),'Resumed managed child must expose same real P2P identity');
 }
 async function lifecycleCapture(name,receipt,prefix,mountOverride=null,external=null){
  if(name==='vps-docker'){
   const before=join(output,prefix+'-stop-first.json'),stopFile=external?external.stopPath:remoteRoot+'/'+prefix+'-stop-first.json';if(!external){await save(before,receipt);await exec('scp',['-q',before,target+':'+stopFile],{timeout:10000});}const r=await remotePublic({mode:external?'capture-once':'capture-pair',prefix,stopFile,argv:captureVmArgv(stopFile,mountOverride)});need(r.externalWitnessTokensMatched&&r.rawCapsuleExported===false&&r.campaignId===manifest.campaignId,'Lifecycle complete VPS P/W capture');
   const capturePath=remoteRoot+'/'+prefix+(external?'-first-stdout.private.json':'-second-stdout.private.json'),stopPath=external?external.stopPath:remoteRoot+'/'+prefix+'-stop-second.private.json',pins=await lifecycleRemote({mode:'capture-pin',capturePath,captureSha256:r.lastCaptureSha256,stopPath});
   return {name,receipt,stopPath,capturePath,stopArtifact:{path:stopPath,sha256:pins.stopSha256,bytes:pins.stopBytes},captureArtifact:{path:lifecycleRemoteGuest(capturePath),sha256:r.lastCaptureSha256,bytes:pins.captureBytes},headSlot:pins.headSlot,finalized:pins.finalized};
  }
  const pins=await capturePair(name,receipt,prefix),stopPath=join(output,prefix+'-stop-second.json'),capturePath=join(output,prefix+'-capture-second.json'),sp=newFiles.get(stopPath),cp=newFiles.get(capturePath);need(sp&&cp,'Known stopped capture output buffers');return {name,receipt,pins,stopPath,capturePath,stopArtifact:{path:stopPath,...sp},captureArtifact:{path:capturePath,...cp}};
 }
 async function lifecycleCoreCopy(name,prefix){
  if(name==='mac-native'){const directory=await lifecyclePrivateDirectory('nano-lifecycle-core-'),copy=await createNativeRotationCopies({dataRoot:rootFor(name,'data'),witnessRoot:rootFor(name,'witness'),destination:join(directory,'owned-core'),checkpoint:lifecycleCheck});return {data:copy.dataRoot,witness:copy.witnessRoot,mounts:[],verify:copy.verify};}
  const a=owner(name),remote=name==='vps-docker',volume=containers[name]+'-'+prefix;need(!(await docker(['volume','ls','--filter','name=^'+volume+'$','--format','{{.Name}}'],remote)).trim(),'New lifecycle core copy volume');await docker(['volume','create','--label',label,volume],remote);
  const inspect=JSON.parse(await docker(['volume','inspect',volume,'--format','{{json .}}'],remote));need(inspect.Name===volume&&inspect.Labels?.['nano.fresh.campaign']===uuid,'Lifecycle volume exact ownership');const copyMount=readOnly=>['--mount',`type=volume,src=${volume},dst=/rotation-audit${readOnly?',readonly':''}`];
  await docker(['run','--rm','--pull=never','--network','none','--read-only','--user','0:0','--cap-drop','ALL','--cap-add','CHOWN','--entrypoint','sh',...copyMount(false),a.image,'-ec','test -z "$(ls -A /rotation-audit)"; chmod 700 /rotation-audit; chown "$1:$2" /rotation-audit','sh',String(a.uid),String(a.gid)],remote);
  const originalMounts=remote&&restoredMounts?restoredMounts.map(x=>x.startsWith('type=')?x+',readonly':x):dataMount(name,true);
  const run=async(mode,pin)=>{const argv=['run','--rm',...safe(name),...originalMounts,...copyMount(mode==='verify'),'--entrypoint','perl',a.image,'-e',linuxRotationCopy,rootFor(name,'data')+'/chain',rootFor(name,'witness')+'/chain-witness','/rotation-audit',mode,pin];const r=remote?await remotePublic({mode:'rotation-owned-copy',prefix:lifecyclePrefix('core-'+mode),afterAudit:mode==='verify',argv:lifecycleRemoteDocker(argv)},190000):JSON.parse(await docker(argv,false,190000));return validateRotationCopySummary(r,mode==='verify');};
  const first=await run('create','-');return {data:'/rotation-audit/data',witness:'/rotation-audit/witness',mounts:copyMount(true),verify:()=>run('verify',first.copyProofSha256)};
 }
 async function lifecycleControl(name,captured,prefix){
  if(name==='mac-docker'){
   const controlVolume=await macLinux.create(prefix+'-controls'),resultVolume=await macLinux.create(prefix+'-results'),node=macNodeBytes(),stop=await observe(captured.stopPath,captured.stopArtifact,true,false),manifestBytes=await observe(join(assets,'isolated-domain.json'),null,true),docs=[{name:'isolated-domain.json',bytes:manifestBytes},...await Promise.all(Object.values(manifest.files).map(async d=>({name:d.path,bytes:await observe(join(assets,d.path),d,true)}))),{name:'node.json',bytes:node},{name:'stop.json',bytes:stop}];
   await macLinux.write(controlVolume,'/known',docs,'lifecycle-control');await macLinux.verifyAll();return {name,directory:null,base:'/lifecycle-owner',node,stopPin:{sha256:sha(stop),bytes:stop.length},resultDirectory:null,resultBase:'/lifecycle-results',controlVolume,resultVolume,manifest:{path:'/lifecycle-owner/isolated-domain.json',sha256:sha(manifestBytes),bytes:manifestBytes.length},mounts:[...macLinux.mount(controlVolume,'/lifecycle-owner'),...macLinux.mount(resultVolume,'/lifecycle-results',false)]};
  }
  const a=owner(name),remote=name==='vps-docker',directory=remote?remoteRoot+'/'+prefix:await lifecyclePrivateDirectory('nano-lifecycle-control-'),base=name==='mac-native'?directory:'/lifecycle-owner',node=Buffer.from(await readFile(join(prepared,name,'node.json')));
  // Four public domain files remain bound to the previously held manifest pins.
  const docs=[['isolated-domain.json',await observe(join(assets,'isolated-domain.json'),null,true)],...Object.values(manifest.files).map(z=>[z.path,null]),['node.json',node]];
  for(const row of docs)if(row[1]===null)row[1]=await observe(join(assets,row[0]),manifest.files[Object.keys(manifest.files).find(k=>manifest.files[k].path===row[0])],true);
  const resultDirectory=remote?remoteRoot+'/'+prefix+'-results':await lifecyclePrivateDirectory('nano-lifecycle-results-'),resultBase=name==='mac-native'?resultDirectory:'/lifecycle-results';if(remote)await sh(['mkdir','-m','700',resultDirectory]);let stopPin,controlAdmission;
  if(remote){const files=docs.map(([leaf,bytes])=>({path:directory+'/'+leaf,source:configAt(name)+'/'+leaf,sha256:sha(bytes),bytes:bytes.length}));const r=await lifecycleRemote({mode:'stage',directory,files,stopSource:{path:captured.stopPath,pin:captured.stopArtifact}},40000);need(r.staged&&r.rawInputsExported===false,'Known private lifecycle input stage');stopPin={sha256:r.stopSha256,bytes:r.stopBytes};controlAdmission={path:directory+'/admitted-inputs.private.json',sha256:r.admissionSha256,bytes:r.admissionBytes,materialSha256:r.admissionMaterialSha256};lifecycleRemoteIndexes.push(controlAdmission);}
  else{const stop=await observe(captured.stopPath,captured.stopArtifact,true,false);docs.push(['stop.json',stop]);const rows=[];for(const [leaf,bytes] of docs)rows.push(await writeLifecycleKnownFile(join(directory,leaf),bytes));const admission=await admitLifecycleKnownFiles(rows,{checkpoint:lifecycleCheck});await admission.verify();lifecycleArtifacts.set(directory,{admission,rows});stopPin={sha256:sha(stop),bytes:stop.length};}
  return {name,directory,base,node,stopPin,controlAdmission,resultDirectory,resultBase,manifest:{path:join(base,'isolated-domain.json'),sha256:manifest.files?sha(await observe(join(assets,'isolated-domain.json'),null,true)):null,bytes:docs[0][1].length},mounts:name==='mac-native'?[]:['--mount',`type=bind,src=${directory},dst=/lifecycle-owner`,'--mount',`type=bind,src=${resultDirectory},dst=/lifecycle-results`]};
 }
 async function lifecycleTool(name,captured,core,control,{stage,action=null,expectedHead=null,evidence=null}){
  const a=owner(name),request={format:'nano-fresh-lifecycle-request-v1',owner:name,manifestSha256:control.manifest.sha256,configurationSha256:sha(control.node),stopSha256:control.stopPin.sha256,expectedHead,identities:spec.lifecycle.identities,resources:{...spec.lifecycle.resources,maximumWallSeconds:stage==='prepare_action'?'0x'+Math.min(60,Number(BigInt(spec.lifecycle.resources.maximumWallSeconds))).toString(16):spec.lifecycle.resources.maximumWallSeconds},stage,action,syntheticActionAuthorized:stage==='prepare_action',intentionallyAuthorConflictingHeaders:stage==='prepare_action'&&action==='proposer_slashing',crossOwnerEvidenceTransferAuthorized:stage==='terminal'&&spec.lifecycle.crossOwnerEvidenceTransferAuthorized,evidence},bytes=lifecycleJson(request),prefix=lifecyclePrefix(stage.replace('_','-')),requestHost=name==='mac-docker'?null:join(control.directory,prefix+'-request.json'),requestGuest=join(control.base,prefix+'-request.json'),resultGuest=join(control.resultBase,prefix+'-result');
  if(name==='mac-docker')await macLinux.write(control.controlVolume,'/known',[{name:prefix+'-request.json',bytes}],'lifecycle-request');else if(name==='vps-docker')await lifecycleWriteRemote(requestHost,bytes);else{const requestPin=await writeLifecycleKnownFile(requestHost,bytes),admission=await admitLifecycleKnownFiles([requestPin],{checkpoint:lifecycleCheck});lifecycleArtifacts.set(requestHost,{admission});}
  const argv=[stage==='prepare_action'?'--fresh-action':'--fresh-audit',control.manifest.path,join(control.base,'node.json'),join(control.base,'stop.json'),requestGuest,sha(bytes),core.data,core.witness,name,resultGuest];let r;
  try{
   if(name==='mac-native')r=JSON.parse((await nativeExec(a.programs.lifecycleTool.path,argv,{env:stage==='prepare_action'?{...nativeEnv,...Object.fromEntries((await readFile(join(prepared,name,'fixture.env'),'utf8')).trim().split('\n').map(s=>{const i=s.indexOf('=');return [s.slice(0,i),s.slice(i+1)];}))}:Object.fromEntries(Object.entries(nativeEnv).filter(([k])=>!k.startsWith('NANO_TEST_CAPELLA_NETWORK_KEY_'))),timeout:Number(BigInt(request.resources.maximumWallSeconds))*1000+10000,maxBuffer:1048576})).stdout);
   else{const d=['run','--rm',...safe(name),...core.mounts,...control.mounts,...(stage==='prepare_action'?['--env-file',configAt(name)+'/fixture.env']:[]),...(stage==='terminal'?['--mount',`type=bind,src=${remoteRoot},dst=/lifecycle-remote,readonly`]:[]),'--entrypoint',a.programs.lifecycleTool.path,a.image,...argv];r=name==='vps-docker'?await lifecycleRemote({mode:'tool',prefix,controlAdmission:control.controlAdmission,argv:lifecycleRemoteDocker(d),seconds:Number(BigInt(request.resources.maximumWallSeconds))+10}):JSON.parse(await docker(d,false,Number(BigInt(request.resources.maximumWallSeconds))*1000+10000));}
  }finally{await core.verify();if(name==='mac-docker')await macLinux.verifyAll();else if(name!=='vps-docker')await lifecycleArtifacts.get(control.directory).admission.verify();}
  need(r.passed===true&&r.owner===name&&r.campaignId===manifest.campaignId&&r.physicalCopiesUnchanged===true&&r.productionQualified===false,'Authenticated lifecycle tool result');
  return {result:r,outGuest:resultGuest,outHost:name==='mac-docker'?null:join(control.resultDirectory,prefix+'-result'),resultVolume:control.resultVolume};
 }
 async function lifecyclePrepareAndSubmit({kind,id}){
  const phaseBegan=performance.now(),phaseUntil=phaseBegan+spec.lifecycle.deadlines.actionMs,name=id.owner,prefix=lifecyclePrefix(kind.replace('_','-')),before=await stopped(name,prefix+'-runtime-stop'),captured=await lifecycleCapture(name,before.receipt,prefix+'-capture'),core=await lifecycleCoreCopy(name,prefix+'-core'),control=await lifecycleControl(name,captured,prefix+'-control');
  const observed=(await lifecycleTool(name,captured,core,control,{stage:'observe'})).result;need(observed.status==='OBSERVED_SCOPED_SEALED_HEAD'&&observed.completePrimaryWitnessClosuresVerified&&observed.externalCoreTokensMatched&&observed.canonicalSignedStateReplayVerified===false&&observed.newSignaturesCreated===0,'Sealed stopped HEAD authority');
  const expectedHead={root:observed.headRoot,stateRoot:observed.headStateRoot,slot:observed.headSlot},preparedAction=await lifecycleTool(name,captured,core,control,{stage:'prepare_action',action:kind,expectedHead}),r=validatePreparedLifecycleAction(preparedAction.result,{campaignId:manifest.campaignId,owner:name,kind,publicKey:id.publicKey,expectedHead});
  const isMacLinux=name==='mac-docker',actionGuest=join(preparedAction.outGuest,'action.private.json'),actionHost=isMacLinux?null:join(preparedAction.outHost,'action.private.json'),acceptedVolume=isMacLinux?await macLinux.create(prefix+'-accepted'):null,resultHost=isMacLinux?null:join(control.directory,'accepted-job.private.json'),resultGuest=isMacLinux?'/proof/accepted-job.private.json':join(control.base,'accepted-job.private.json');let planPin,oldAdmission,submissionAdmission,linuxActionArtifact,linuxAcceptedArtifact;
  if(isMacLinux){
   const action=await macLinux.readNested(control.resultVolume,actionGuest,{sha256:r.actionSha256,bytes:r.actionBytes},'action'),document=JSON.parse(action.bytes);need(document.owner===name&&document.publicKey===id.publicKey&&document.kind===kind&&document.campaignId===manifest.campaignId&&canonical(document)===action.bytes.toString(),'Exact canonical Linux prepared action');linuxActionArtifact=action.artifact;
   const p=lifecycleSubmissionPlan({campaignId:manifest.campaignId,owner:name,manifest:control.manifest,action:{path:actionGuest,sha256:r.actionSha256,bytes:r.actionBytes},realP2p:owner(name).programs.p2p,resultFile:resultGuest,maximumWallSeconds:Math.min(60,Math.floor(spec.lifecycle.deadlines.actionMs/1000))});planPin=(await macLinux.write(control.controlVolume,'/known',[{name:'plan.private.json',bytes:p.bytes}],'submission-plan'))[0];await macLinux.verifyAll();
  }else if(name==='vps-docker'){
   const p=await lifecycleRemote({mode:'plan',campaignId:manifest.campaignId,kind,directory:control.directory,manifest:control.manifest,action:{path:actionHost,sha256:r.actionSha256,bytes:r.actionBytes},guestAction:{path:actionGuest,sha256:r.actionSha256,bytes:r.actionBytes},realP2p:owner(name).programs.p2p,resultFile:resultGuest,maximumWallSeconds:Math.min(60,Math.floor(spec.lifecycle.deadlines.actionMs/1000)),planFile:join(control.directory,'plan.private.json')},40000);need(p.knownBytesPinnedBeforeWrite&&p.firstAdmissionStable&&p.actionSha256===r.actionSha256,'Plan uses authenticated C++ prepared pin');planPin={sha256:p.planSha256,bytes:p.planBytes};submissionAdmission={path:join(control.directory,'submission-inputs.private.json'),sha256:p.admissionSha256,bytes:p.admissionBytes,materialSha256:p.admissionMaterialSha256};lifecycleRemoteIndexes.push(submissionAdmission);
  }else{
   const action=await readLifecycleKnownFile(actionHost,{sha256:r.actionSha256,bytes:r.actionBytes},{maximum:65536}),document=JSON.parse(action.bytes);need(document.owner===name&&document.publicKey===id.publicKey&&document.kind===kind&&document.campaignId===manifest.campaignId&&canonical(document)===action.bytes.toString(),'Exact canonical prepared action');const p=lifecycleSubmissionPlan({campaignId:manifest.campaignId,owner:name,manifest:control.manifest,action:{path:actionGuest,sha256:r.actionSha256,bytes:r.actionBytes},realP2p:owner(name).programs.p2p,resultFile:resultGuest,maximumWallSeconds:Math.min(60,Math.floor(spec.lifecycle.deadlines.actionMs/1000))});planPin=await writeLifecycleKnownFile(join(control.directory,'plan.private.json'),p.bytes);oldAdmission=await admitLifecycleKnownFiles([{path:actionHost,sha256:r.actionSha256,bytes:r.actionBytes},planPin],{checkpoint:lifecycleCheck});
  }
  if(oldAdmission)lifecycleArtifacts.set(join(control.directory,'plan.private.json'),{admission:oldAdmission});need(performance.now()<phaseUntil,'Whole action phase deadline before resumed writer');need(performance.now()-phaseBegan<=Number(BigInt(spec.clock.maximumDowntimeMs)),'Action preparation exceeded original clock catch-up authority');await lifecycleResume(name,before.receipt,{directory:control.directory,...(isMacLinux?{mounts:[...macLinux.mount(control.controlVolume,'/lifecycle-owner'),...macLinux.mount(control.resultVolume,'/lifecycle-results'),...macLinux.mount(acceptedVolume,'/proof',false)]}:{extraMounts:['--mount',`type=bind,src=${control.resultDirectory},dst=/lifecycle-results,readonly`]}),planPath:join(control.base,'plan.private.json'),planSha256:planPin.sha256});
  const until=phaseUntil;let accepted;
  while(performance.now()<until){lifecycleCheck();if(name==='vps-docker'){
    const v=await lifecycleRemote({mode:'accepted-log',prefix:lifecyclePrefix('accepted-log'),admission:submissionAdmission,logsArgv:['sudo','-n','docker','logs','--tail','200',containers[name]],campaignId:manifest.campaignId,kind,actionSha256:r.actionSha256,payloadDigest:r.payloadDigest,resultPath:resultHost},35000);if(!v.pending){need(v.admission&&v.acceptedProjection,'Accepted private index pin');lifecycleRemoteIndexes.push(v.admission);accepted=validateLifecycleAcceptedProjection(v.acceptedProjection,{campaignId:manifest.campaignId,owner:name,kind,actionSha256:r.actionSha256});break;}
   }else{const logsText=name==='mac-native'?await lifecycleLogTail(lifecycleNativeLog,false):await docker(['logs','--tail','200',containers[name]]),events=logsText.split('\n').flatMap(line=>{try{const x=JSON.parse(line);return x.event==='fresh-lifecycle-child-submission-accepted'?[x]:[];}catch{return [];}});need(events.length<=1,'No repeated managed submission');if(events.length){const e=events[0],pin={sha256:e.receiptSha256,bytes:e.receiptBytes},file=isMacLinux?await macLinux.readNested(acceptedVolume,resultGuest,pin,'accepted'):await readLifecycleKnownFile(resultHost,pin,{maximum:65536}),j=JSON.parse(file.bytes);if(isMacLinux)linuxAcceptedArtifact=file.artifact;need(Object.keys(j).sort().join(',')==='actionSha256,campaignId,format,job,kind,owner,sequence,session'&&j.format==='nano-fresh-lifecycle-accepted-job-v1'&&j.owner===name&&j.campaignId===manifest.campaignId&&j.kind===kind&&j.actionSha256===r.actionSha256&&/^0x[0-9a-f]{64}$/.test(j.session)&&/^0x[1-9a-f][0-9a-f]*$/.test(j.sequence),'Actual owner-local Job identity');need(Object.keys(j.job).sort().join(',')==='cancelRequested,decision,payloadDigest,sequence,state'&&j.job.sequence===j.sequence&&j.job.payloadDigest===r.payloadDigest&&!j.job.cancelRequested&&j.job.state==='completed'&&j.job.decision==='accepted','Actual completed accepted Job');accepted={format:'nano-fresh-lifecycle-accepted-public-v1',accepted:true,campaignId:manifest.campaignId,owner:name,kind,actionSha256:r.actionSha256,receiptSha256:pin.sha256,receiptBytes:pin.bytes,rawJobExported:false,sessionExported:false,productionQualified:false};if(isMacLinux)await macLinux.verifyAll();else{await oldAdmission.verify();const resultAdmission=await admitLifecycleKnownFiles([{path:resultHost,...pin}],{checkpoint:lifecycleCheck});lifecycleArtifacts.set(resultHost,{admission:resultAdmission});}break;}}
   await delay(500);
  }
  need(performance.now()<phaseUntil,'Whole action phase deadline');need(accepted,'Managed child acceptance deadline; uncertain Submit is never retried');await save(join(output,prefix+'-accepted-public.json'),accepted);
  const a={actionArtifact:{path:isMacLinux?actionGuest:name==='vps-docker'?lifecycleRemoteGuest(actionHost):actionHost,sha256:r.actionSha256,bytes:r.actionBytes},acceptedJobArtifact:{path:isMacLinux?resultGuest:name==='vps-docker'?lifecycleRemoteGuest(resultHost):resultHost,sha256:accepted.receiptSha256,bytes:accepted.receiptBytes},actionHost,acceptedHost:resultHost,linuxActionArtifact,linuxAcceptedArtifact,payloadDigest:r.payloadDigest,kind,publicKey:id.publicKey,resolvedIndex:r.resolvedIndex,preparedHead:expectedHead,owner:name};lifecycleArtifacts.set(kind,a);return a;
 }
 async function lifecycleWalk(name,start,minimumSlot,find,ms){const until=performance.now()+ms,maximum=Number(BigInt(spec.lifecycle.resources.maximumHistoryBlocks));let tip=start,bytes=0;
  for(let n=0;tip!==manifest.initialHeadRoot;++n){lifecycleCheck();need(n<maximum&&performance.now()<until,'Bounded finalized lifecycle ancestry');const r=await request(name,'/eth/v2/beacon/blocks/'+tip);need(r.execution_optimistic===false&&r.finalized===true&&r.data?.message?.body,'Actual finalized canonical block required');bytes+=Buffer.byteLength(JSON.stringify(r));need(BigInt(bytes)<=2n*BigInt(spec.lifecycle.resources.maximumArchiveBytes)+BigInt(maximum)*4096n,'Bounded public block projection bytes');const m=r.data.message,slot=BigInt(m.slot);if(slot<=minimumSlot)return null;const found=find({root:tip,slot,message:m});if(found)return found;need(/^0x[0-9a-f]{64}$/.test(m.parent_root)&&m.parent_root!==tip,'Strict ancestry parent');tip=m.parent_root;}
  return null;
 }
 async function lifecycleFindFinalized(name,checkpoint,minimumSlot,find,ms){const until=performance.now()+ms;let required=checkpoint;
  while(performance.now()<until){lifecycleCheck();const h=await request(name,'/eth/v1/beacon/headers/finalized');need(h.execution_optimistic===false&&h.data?.root,'Finalized header');const got=await lifecycleWalk(name,h.data.root,minimumSlot,find,Math.max(1,until-performance.now()));if(got)return got;await delay(3000);}throw Error('Actual finalized lifecycle event deadline');
 }
 function lifecycleOwnerCopyResult(r,after){need(r.format==='nano-fresh-lifecycle-owner-copy-v1'&&r.passed===true&&r.roleRoots===2&&r.copiesUnchangedAfterAudit===after&&r.sourceNineFieldAndShaUnchanged===true&&r.sourceNamespaceUnchanged===true&&r.copiesMatchSourceBytes===true&&r.independentInodes===true&&r.privateDirectoryModes===true&&r.privateFileModes===true&&r.rawInventoryExported===false&&r.productionQualified===false,'Whole independent owner-copy scope');for(const key of ['sourceFiles','sourceDirectories','payloadBytes'])need(Number.isSafeInteger(r[key])&&r[key]>=0,'Safe actual owner-copy counter');return r;}
 async function lifecycleRestore(phase,id,beforeProposal){
  const phaseBegan=performance.now(),name='vps-docker',a=owner(name),prefix=lifecyclePrefix('restore-'+phase),before=await stopped(name,prefix+'-runtime-stop'),captured=await lifecycleCapture(name,before.receipt,prefix+'-source'),destination=remoteRoot+'/'+prefix;await sh(['mkdir','-m','700',destination]);
  const fullCopy=lifecycleWholeCopyProgram(plan.campaignNamespace,name,spec.lifecycle.resources),sourceMounts=restoredMounts?restoredMounts.map(x=>x.startsWith('type=')?x+',readonly':x):dataMount(name,true),copyMount=readOnly=>['--mount',`type=bind,src=${destination},dst=/rotation-audit${readOnly?',readonly':''}`],cmd=(mode,pin)=>lifecycleRemoteDocker(['run','--rm',...safe(name),...sourceMounts,...copyMount(mode==='verify'),'--entrypoint','perl',a.image,'-e',fullCopy,rootFor(name,'data'),rootFor(name,'witness'),'/rotation-audit',mode,pin]);
  // Whole-tree role-root projection is separate from the old two-DB envelope.
  const copy=lifecycleOwnerCopyResult(await lifecycleRemote({mode:'owner-copy',prefix:prefix+'-copy',afterAudit:false,resources:spec.lifecycle.resources,argv:cmd('create','-')},195000),false);
  const targetData=destination+'/data',targetWitness=destination+'/witness',replacement=['--mount',`type=bind,src=${targetData},dst=/var/lib/nano-data`,'--mount',`type=bind,src=${targetWitness},dst=/var/lib/nano-witness`];
  const restored=await lifecycleCapture(name,before.receipt,prefix+'-restored',replacement.map(x=>x.startsWith('type=')?x+',readonly':x),captured);
  // Both captures independently authenticate the same external P/W pins. They
  // remain owner-local and are compared byte/token-for-byte by closing C++.
  const verified=lifecycleOwnerCopyResult(await lifecycleRemote({mode:'owner-copy',prefix:prefix+'-verify',afterAudit:true,resources:spec.lifecycle.resources,argv:cmd('verify',copy.copyProofSha256)},195000),true);
  lifecycleSourceFences.push({prefix,program:lifecycleSourceOnlyCopyFence(fullCopy),argv:pin=>lifecycleRemoteDocker(['run','--rm',...safe(name),...sourceMounts,...copyMount(true),'--entrypoint','perl',a.image,'-e',lifecycleSourceOnlyCopyFence(fullCopy),rootFor(name,'data'),rootFor(name,'witness'),'/rotation-audit','verify',pin]),pin:copy.copyProofSha256});
  need(performance.now()-phaseBegan<spec.lifecycle.deadlines.restoreMs,'Whole restore phase deadline before resumed writer');need(performance.now()-phaseBegan<=Number(BigInt(spec.clock.maximumDowntimeMs)),'Restore preparation exceeded original clock catch-up authority');restoredMounts=replacement;await lifecycleResume(name,before.receipt);need(performance.now()-phaseBegan<spec.lifecycle.deadlines.restoreMs,'Whole restore phase deadline');need(performance.now()-phaseBegan<=Number(BigInt(spec.clock.maximumDowntimeMs)),'Whole restore exceeded original clock catch-up authority');
  return {cutoffSlot:'0x'+BigInt(captured.headSlot).toString(16),beforeProposalRoot:beforeProposal.root,afterProposalRoot:null,dutyPublicKey:id.publicKey,sourceCapture:captured.captureArtifact,restoredCapture:restored.captureArtifact,finalized:before.receipt.finalized};
 }
 function lifecycleSourceOnlyCopyFence(program){
  // Same original proof rows and scans; resumed copies are EXPECTED to change.
  // Delete only the scan of COPY targets, never source scans or source fields.
  const part="for my $i(0,1){my %c=%{$proof->{copies}[$i]};need(delete($c{root}) eq $targets[$i]&&same(\\%c,scan($targets[$i],1,\\%sourceids)));}";
  need(program.split(part).length===2,'Source-only fence exact derivation');let result=program.replace(part,'').replace("copiesUnchangedAfterAudit=>$mode eq 'verify'?$yes:$no","copiesUnchangedAfterAudit=>$no");for(const key of ['copiesMatchSourceBytes','independentInodes','privateDirectoryModes','privateFileModes'])result=result.replace(key+'=>$yes',key+'=>$no');return result;
 }
 async function lifecycleTransferTerminal(evidence,{crossOwnerEvidenceTransferAuthorized}){
  need(spec.lifecycle.privateVpsLifecycleTransferAuthorized===true&&([...lifecycleArtifacts.values()].filter(x=>x.owner&&x.owner!=='vps-docker').length===0||crossOwnerEvidenceTransferAuthorized===true),'Specific private cross-owner transfer authority');const dir=remoteRoot+'/lifecycle-cross-owner';await sh(['mkdir','-m','700',dir]);
  for(const role of ['voluntaryExit','slashing']){const a=lifecycleArtifacts.get(role==='voluntaryExit'?'voluntary_exit':'proposer_slashing');if(a.owner==='vps-docker')continue;
   for(const [field,host] of [['action',a.actionHost],['acceptedJob',a.acceptedHost]]){const pin=evidence.actions[role][field];let localHost=host;if(a.owner==='mac-docker'){const z=field==='action'?a.linuxActionArtifact:a.linuxAcceptedArtifact;need(z&&z.sha256===pin.sha256&&z.bytes===pin.bytes,'Producer-known Linux cross-owner artifact');const read=await macLinux.readNested(z.volume,z.path,z,z.kind),local=join(output,'cross-owner-'+role+'-'+field+'.private.json');await save(local,read.bytes);await macLinux.verifyAll();localHost=local;}const before=await readLifecycleKnownFile(localHost,pin,{maximum:65536}),destination=dir+'/'+role+'-'+field+'.private.json';await exec('scp',['-q',localHost,target+':'+destination],{timeout:10000});await readLifecycleKnownFile(localHost,pin,{maximum:65536,expectedMaterial:before.material});if(a.owner==='mac-docker')await macLinux.verifyAll();const checked=await lifecycleRemote({mode:'read-pin',path:destination,pin});need(checked.sha256===pin.sha256&&checked.bytes===pin.bytes&&checked.rawInputsExported===false,'Cross-owner transferred exact approved Artifact');evidence.actions[role][field]={path:lifecycleRemoteGuest(destination),sha256:pin.sha256,bytes:pin.bytes};}
  }
 }
 async function lifecycleClosing(terminal){
  need(lifecyclePending&&terminal.length===3,'Completed live stages and three stopped owners');for(const z of terminal)need(z.receipt.clockHealthy===true&&z.receipt.validator.healthy===true&&z.receipt.validator.failure===null&&z.receipt.validator.attestationsAdmitted>0&&z.receipt.validator.proposalsAdmitted>0,'Actual healthy duties on every stopped owner');
  const last=terminal.find(z=>z.name==='vps-docker'),prefix=lifecyclePrefix('terminal'),captured=lifecycleTerminalCaptures.get('vps-docker'),core=await lifecycleCoreCopy('vps-docker',prefix+'-core'),control=await lifecycleControl('vps-docker',captured,prefix+'-control');
  for(const admission of lifecycleRemoteIndexes)await lifecycleRemote({mode:'fence-index',admission},35000);await lifecycleRemote({mode:'fence-known',files:lifecycleRemoteFiles},35000);const observed=(await lifecycleTool('vps-docker',captured,core,control,{stage:'observe'})).result;need(observed.status==='OBSERVED_SCOPED_SEALED_HEAD'&&observed.completePrimaryWitnessClosuresVerified&&observed.externalCoreTokensMatched,'Final stopped sealed HEAD');const r=(await lifecycleTool('vps-docker',captured,core,control,{stage:'terminal',expectedHead:{root:observed.headRoot,stateRoot:observed.headStateRoot,slot:observed.headSlot},evidence:lifecyclePending.evidence})).result;
  validateLifecycleTerminalProjection(r,{campaignId:manifest.campaignId,requiredFinalized:lifecyclePending.evidence.requiredFinalized});
  for(const z of lifecycleSourceFences){const result=await lifecycleRemote({mode:'original-source-fence',prefix:z.prefix+'-original-final-fence',argv:z.argv(z.pin)},195000);need(result.format==='nano-fresh-lifecycle-original-source-fence-v1'&&result.sourceNineFieldAndShaUnchanged===true&&result.sourceNamespaceUnchanged===true&&result.copyProofSha256===z.pin&&result.resumedCopiesExpectedToChange===true&&result.resumedCopiesRechecked===false&&result.rawInventoryExported===false&&result.productionQualified===false,'All prior source owner roots unchanged after independent resumed writers');}
  for(const {admission} of lifecycleArtifacts.values())if(admission)await admission.verify();await macLinux.verifyAll();for(const admission of lifecycleRemoteIndexes)await lifecycleRemote({mode:'fence-index',admission},35000);await lifecycleRemote({mode:'fence-known',files:lifecycleRemoteFiles},35000);await save(join(output,'lifecycle-terminal-public.json'),r);lifecycleQualified=true;
 }
 async function lifecycleBranch(initial,profileBytes){
  const depositPath=join(assets,'deposit.json'),depositBytes=await observe(depositPath,null,true),depositDocument=JSON.parse(depositBytes);diagnosticPublicPaths.add(depositPath);
  lifecyclePending=await runFreshLifecycle({spec,domain:manifest,prepared:preparedManifest,profileBytes,depositDocument,initialFinality:initial,ops:{
   check:lifecycleCheck,sleep:delay,record,header:(name,selector)=>request(name,'/eth/v1/beacon/headers/'+selector),validator:async(name,state,pub)=>{try{return await request(name,'/eth/v1/beacon/states/'+state+'/validators/'+pub);}catch(e){if(e.status===404)return null;throw e;}},submitDeposit:(name,raw)=>rpc(name,'eth_sendRawTransaction',[raw]),
   finality:(baseline,phase,timeout)=>finality({epoch:BigInt(baseline.epoch).toString(),root:baseline.root},phase,false,timeout),
   finalizedProposal:async(id,index,checkpoint,after,ms)=>lifecycleFindFinalized(id.owner,checkpoint,after,x=>BigInt(x.message.proposer_index)===BigInt(index)?{root:x.root,slot:'0x'+x.slot.toString(16)}:null,ms),
   restore:lifecycleRestore,prepareAndSubmit:lifecyclePrepareAndSubmit,
   finalizedOperation:async(name,kind,action,checkpoint,ms)=>(await lifecycleFindFinalized(name,checkpoint,BigInt(action.preparedHead.slot),x=>{const list=x.message.body[kind==='voluntary_exit'?'voluntary_exits':'proposer_slashings'];need(Array.isArray(list),'Actual operation list');const occurrences=list.filter(op=>lifecycleOperationDigest(kind,op)===action.payloadDigest);need(occurrences.length<=1,'Duplicate exact action');return occurrences.length?{root:x.root}:null;},ms)).root,
   finalizedWithdrawal:async(id,row,checkpoint,ms)=>lifecycleFindFinalized(id.owner,checkpoint,0n,x=>{if(x.slot/BigInt(lifecyclePending?.parameters?.slotsPerEpoch??JSON.parse(profileBytes).common[0].ffg[0])<BigInt(row.withdrawableEpoch))return null;const list=x.message.body.execution_payload.withdrawals;need(Array.isArray(list),'Actual execution withdrawals');const found=list.find(w=>BigInt(w.validator_index)===BigInt(row.resolvedIndex)&&BigInt(w.amount)>0n);return found?{root:x.root,slot:'0x'+x.slot.toString(16),withdrawalIndex:'0x'+BigInt(found.index).toString(16)}:null;},ms),
   stageTerminalEvidence:lifecycleTransferTerminal
  }});await save(join(output,'lifecycle-pending-terminal-public.json'),{format:lifecyclePending.format,restoresExecuted:2,payoutCandidatesObserved:2,canonicalReplayQualified:false,productionQualified:false});
 }

 try{
  if(interrupted)throw interrupted;publicCheckStage='initial-software-read';initialSoftware=await software();publicCheckStage='initial-software-complete';await save(join(output,'software-public.json'),initialSoftware);const helperPath=join(harnessRoot,linuxSourceFiles.helper.path),copierPath=join(harnessRoot,linuxSourceFiles.copier.path),helperBytes=await observe(helperPath,linuxSourceFiles.helper,true),copierBytes=await observe(copierPath,linuxSourceFiles.copier,true);macLinux=createMacLinuxFiles({docker,image:owner('mac-docker').image,uuid,prefix:containers['mac-docker'],softwareMounts:()=>mounts('mac-docker'),helperPath,helperBytes,copierBytes,save:(name,b)=>save(join(output,name),b),verifyHelper:()=>observe(helperPath,linuxSourceFiles.helper),checkpoint:lifecycleCheck});await macLinux.preflight(owner('mac-docker').programs.bootstrapImport.path);publicCheckStage='initial-held-fence';await fence();publicCheckStage='campaign-body';await save(join(output,'harness.json'),await hashHarness(harnessRoot));
  // Compilation, image and public artifact checks finish BEFORE the new clock
  // and reserved key domain are exported. Never retry/reset a failed exporter.
  const a=owner('mac-native');await nativeExec(a.programs.exporter.path,['--export-isolated-deposit-network',assets],{env:{...nativeEnv,NANO_TEST_ISOLATED_CAPELLA_SEED_FILE:spec.fixture.seedFile,NANO_TEST_ISOLATED_CAPELLA_LEDGER_DIR:spec.fixture.ledgerDirectory},timeout:180000,maxBuffer:16*1024*1024});
  for(const leaf of publicExportLeaves)diagnosticPublicPaths.add(join(assets,leaf));
  publicCheckStage='public-export-pre-admission-stability';const publicExportStability=await stablePublicExportInventory();if(interrupted)throw interrupted;publicCheckStage='public-export-first-admission';
  const manifestBytes=await observe(join(assets,'isolated-domain.json'),publicExportStability.inventory.get(join(assets,'isolated-domain.json')),true);manifest=JSON.parse(manifestBytes);const publicFiles=new Map();for(const z of Object.values(manifest.files)){const path=join(assets,z.path),expected=publicExportStability.inventory.get(path);need(expected&&expected.sha256===z.sha256&&expected.bytes===z.bytes,'Public export manifest and last stable inventory differ');publicFiles.set(z.path,await observe(path,expected,true));}
  publicCheckStage='public-export-immediate-held-fence';await fence();publicCheckStage='campaign-body';
  const publicResult=verifyIsolatedFixtureDomain({manifest,artifacts:publicFiles,legacy});
  if(spec.lifecycle){
   // Refuse a mismatched exported deposit before journal import, transport or
   // starting any runtime. lifecycleBranch rechecks this held file before send.
   const depositPath=join(assets,'deposit.json');diagnosticPublicPaths.add(depositPath);
   validateLifecycleDeposit(JSON.parse(await observe(depositPath,null,true)),manifest,spec.lifecycle.identities.voluntaryExit);
   const parameters=deriveLifecycleParameters(publicFiles.get('profile.json'),spec.lifecycle,{maximumBootstrapAgeMs:Number(BigInt(spec.clock.maximumDowntimeMs))});lifecycleRollingFinalityWindow(parameters,spec.lifecycle.deadlines.finalityMs);
  }
  const publicAudit={format:'nano-isolated-public-export-audit-v1',verdict:'PASSED_PUBLIC_BINDINGS_ONLY',verifiedAtUtc:new Date().toISOString(),publicExportDirectory:assets,verifierSha256:sha(await observe(join(harnessRoot,'tests/capella/isolated-fixture-domain.mjs'),null,true)),officialPublicSourceCommit:'194a91fd9e6f97118156421aac4c80e6e47a9a7f',publicExportFiles:[{name:'isolated-domain.json',sha256:sha(manifestBytes),bytes:manifestBytes.length},...Object.values(manifest.files).map(z=>({name:z.path,sha256:z.sha256,bytes:z.bytes}))],legacyPublicInputs:[spec.fixture.legacyPublicIdentity],legacyGenesisValidatorsRoot:legacy.genesisValidatorsRoot,result:publicResult,scope:Object.fromEntries(['exporterExecuted','signerInvoked','privateArtifactsRead','ledgerRead','networkUsed','blsCryptographyVerified','actualSigningOrderObserved','liveRuntimeVerified','productionQualified'].map(k=>[k,false]))};
  await mkdir(proof,{mode:0o700});const publicBytes=await save(join(proof,'public-audit.json'),publicAudit);await save(join(proof,'public-export-pre-admission-quiescence.json'),publicExportStability.report);
  const importText=(await nativeExec(a.programs.bootstrapImport.path,[assets,sha(manifestBytes),journals],{timeout:180000,maxBuffer:16*1024*1024})).stdout,privateAudit=JSON.parse(importText);need(privateAudit.passed===true,'Bootstrap import must actually pass');const privateBytes=await save(join(proof,'import-audit.json'),privateAudit);
  const bootstrapStability=await stableBootstrapInventory(a.programs.bootstrapAudit.path);
  for(const leaf of ['journal-bindings.json','bootstrap-interchange.json'])diagnosticPublicPaths.add(join(journals,leaf));
  const journalBytes=await observe(join(journals,'journal-bindings.json'),null,true),interchangeBytes=await observe(join(journals,'bootstrap-interchange.json'),null,true),initial=publicFiles.get('initial.json');
  const documents={manifest:manifestBytes,publicAudit:publicBytes,privateAudit:privateBytes,journalBindings:journalBytes,initial,bootstrapInterchange:interchangeBytes},evidencePins=Object.fromEntries(Object.entries(documents).map(([k,v])=>[k,sha(v)]));
  plan=createIsolatedBootstrapPlan({campaignNamespace:'isolated-capella-'+manifest.campaignId,documents,evidencePins,assignments:names.map((name,n)=>({owner:name,publicKeyIndices:Array.from({length:n===2?33:16},(_,i)=>i+(n===0?0:n===1?16:32))}))});
  if(interrupted)throw interrupted;const planBytes=await save(join(proof,'plan.json'),plan),inventoryBytes=await save(join(proof,'source-inventory.json'),bootstrapStability.inventory);await save(join(proof,'bootstrap-pre-admission-quiescence.json'),bootstrapStability.report);
  await save(join(proof,'anchors.json'),{format:'nano-isolated-bootstrap-physical-audit-anchors-v1',fixtureManifestSha256:sha(manifestBytes),planDocumentSha256:sha(planBytes),planSha256:plan.planSha256,sourceInventorySha256:sha(inventoryBytes)});
  const topology=names.map((name,i)=>({name,host:i===2?target:'mac',mode:i===0?'native':'docker',rpcPort:i===0?18545:8545,p2pPort:i===0?30305:30303,runtimeDirectory:join(prepared,name),...(i===0?{p2pBinary:a.programs.p2p.path}:{})}));await save(join(output,'topology.json'),topology);preparedManifest=await prepareFresh(assets,prepared,topology,plan);
  for(const name of names){const path=join(prepared,name,'node.json'),node=JSON.parse(await readFile(path,'utf8'));node.capella.clock.maximumStartupElapsedMs=spec.clock.maximumStartupElapsedMs;node.capella.clock.startupCatchup={maximumDowntimeMs:spec.clock.maximumDowntimeMs,maximumSteps:spec.clock.maximumSteps};await writeFile(path,JSON.stringify(node,null,2)+'\n',{mode:0o600});}
  await macConfigGeneration();volumes.set('mac-docker',{data:await macLinux.create('runtime-data'),witness:await macLinux.create('runtime-witness')});originalVolumes.set('mac-docker',{...volumes.get('mac-docker')});await macBootstrapPrepare({documents,planBytes});await stageBootstrapTransport();
  for(const name of ['vps-docker']){
   const remote=true,a=owner(name),base=containers[name];volumes.set(name,{data:base+'-data',witness:base+'-witness'});originalVolumes.set(name,{...volumes.get(name)});
   for(const v of Object.values(volumes.get(name)))await docker(['volume','create','--label',label,v],remote);
   const v=volumes.get(name);await docker(['run','--rm','--pull=never','--name',base+'-init','--label',label,'--network','none','--read-only','--user','0:0','--cap-drop','ALL','--cap-add','CHOWN','--security-opt','no-new-privileges','--mount',`type=volume,src=${v.data},dst=/data`,'--mount',`type=volume,src=${v.witness},dst=/witness`,'--entrypoint','sh',a.image,'-ec','chmod 700 /data /witness; chown "$1:$2" /data /witness','sh',String(a.uid),String(a.gid)],remote);
   await docker(['run','--rm',...safe(name),...dataMount(name),'--mount',`type=bind,src=${configAt(name)},dst=/staged,readonly`,'--entrypoint','sh',a.image,'-ec','cp -Rp /staged/data/. /var/lib/nano-data/; cp -Rp /staged/witness/. /var/lib/nano-witness/'],remote,60000);
  }
  for(const name of names)await auditCopy(name);const originalInventory=JSON.parse((await nativeExec(a.programs.bootstrapAudit.path,['--inventory',journals],{timeout:180000,maxBuffer:16*1024*1024})).stdout);
  // This new synthetic-bootstrap transport check reports directory time changes
  // separately; every recorded file field and SHA remains exact. No runtime has
  // started, and historical input/stopped-capture fences remain unchanged.
  const transportInventory=verifyFreshBootstrapTransportInventory(JSON.parse(inventoryBytes),originalInventory);await save(join(proof,'source-inventory-after-transport.json'),originalInventory);await save(join(proof,'bootstrap-transport-inventory-check.json'),transportInventory);
  const exportAge=BigInt(Date.now())-BigInt(plan.anchor.genesisTimeMs);need(exportAge>=0n&&exportAge<=BigInt(spec.clock.maximumDowntimeMs),'New fixture exceeded explicitly approved startup catch-up age');
  const preparedClockChecks=[];for(const name of names)preparedClockChecks.push({owner:name,...checkFreshInitialRuntimeClock({...await initialClockInput(name),nowMs:BigInt(Date.now())})});await save(join(output,'initial-prepared-clock-checks.json'),{format:'nano-fresh-prepared-initial-clocks-v1',owners:preparedClockChecks,runtimeStarted:false,productionQualified:false});
  if(spec.partitionRotation){partitionNetworkCreated=true;partitionNetworkAbsent=false;await docker(['network','create','--label',label,partitionNetwork],true);}
  await startDocker('vps-docker',undefined,null,true);for(let i=0;i<60;i++){if(interrupted)throw interrupted;try{remotePeer=(await rpc('vps-docker','nno_getNetworkInfo')).node?.peerId;if(remotePeer)break;}catch{if(interrupted)throw interrupted;}await delay(1000);}if(interrupted)throw interrupted;need(/^[A-Za-z0-9]+$/.test(remotePeer??''),'VPS private peer identity unavailable');
  const tunnelFd=openSync(join(output,'tunnel.log'),'wx',0o600);try{tunnel=tracked(spawn('ssh',[...sshArgs,'-N','-o','ExitOnForwardFailure=yes','-o','ServerAliveInterval=15','-L','127.0.0.1:30307:127.0.0.1:30307',target],{stdio:['ignore',tunnelFd,tunnelFd]}));}finally{closeSync(tunnelFd);}let tunnelError;tunnel.on('error',e=>{tunnelError=e;});await delay(1000);need(!tunnelError&&tunnel.exitCode===null&&tunnel.signalCode===null,'Private SSH tunnel unavailable');
  for(const name of ['mac-native','mac-docker']){const path=join(prepared,name,'node.json'),c=JSON.parse(await readFile(path,'utf8'));c.bootnodes=[name==='mac-native'?'/ip4/127.0.0.1/tcp/30307/p2p/'+remotePeer:'/dns4/host.docker.internal/tcp/30307/p2p/'+remotePeer];await writeFile(path,JSON.stringify(c,null,2)+'\n',{mode:0o600});}
  await macConfigGeneration();await startDocker('mac-docker',undefined,null,true);if(spec.partitionRotation){let localPeer;for(let i=0;i<60;i++){if(interrupted)throw interrupted;try{localPeer=(await rpc('mac-docker','nno_getNetworkInfo')).node?.peerId;if(localPeer)break;}catch{if(interrupted)throw interrupted;}await delay(1000);}need(/^[A-Za-z0-9]+$/.test(localPeer??''),'Mac Docker direct-island identity required');const path=join(prepared,'mac-native','node.json'),c=JSON.parse(await readFile(path,'utf8'));c.bootnodes.push('/ip4/127.0.0.1/tcp/30306/p2p/'+localPeer);await writeFile(path,JSON.stringify(c,null,2)+'\n',{mode:0o600});}const nd=join(prepared,'mac-native'),env=Object.fromEntries((await readFile(join(nd,'fixture.env'),'utf8')).trim().split('\n').map(s=>{const at=s.indexOf('=');return [s.slice(0,at),s.slice(at+1)];}));const fd=openSync(join(output,'native.log'),'wx',0o600);let nativeError;try{await initialRuntimeLaunch('mac-native',()=>{native=tracked(spawn(a.programs.nano.path,['run',join(nd,'genesis.json'),rootFor('mac-native','data')+'/chain',join(nd,'node.json')],{env:nativeRuntimeEnvironment(nativeEnv,env),stdio:['ignore',fd,fd]}));native.on('error',e=>{nativeError=e;});});}finally{closeSync(fd);}await delay(50);need(native.pid&&!nativeError,'Native runtime spawn failed');started.add('mac-native');
  need(initialRuntimeClockChecks.size===3&&names.every(name=>initialRuntimeClockChecks.has(name)),'All three first spawns passed distinct fresh clock checks');await save(join(output,'initial-first-spawn-clock-checks.json'),{format:'nano-fresh-first-spawn-clock-checks-v1',owners:names.map(name=>({owner:name,...initialRuntimeClockChecks.get(name)})),runtimeStartupVerified:false,productionQualified:false});
  const peerIds=[];for(const name of names){const until=Date.now()+60000;let info;while(Date.now()<until){if(interrupted)throw interrupted;try{info=await rpc(name,'nno_getNetworkInfo');if(info?.node?.peerId)break;}catch{if(interrupted)throw interrupted;}await delay(1000);}if(interrupted)throw interrupted;need(info?.node?.peerId&&!peerIds.includes(info.node.peerId),'Distinct three runtime peer identities required');peerIds.push(info.node.peerId);freshPeerIds.set(name,info.node.peerId);need((await rpc(name,'eth_getBlockByNumber',['0x0',false]))?.hash===manifest.executionGenesisHash,'Fresh execution genesis mismatch');}
  await record({event:'fresh-started',campaignId:manifest.campaignId,sourceBaseRevision:revision,overlayManifestSha256:spec.overlayManifest.sha256,physicalHosts:2,runtimes:3,registeredValidators:64,pendingValidators:1,validatorAllocation:[16,16,33],productionQualified:false});
  const initialFinality=await finality({epoch:'0',root:manifest.initialHeadRoot},'initial',true);if(spec.partitionRotation)await runFreshPartitionRotation(initialFinality,publicFiles.get('profile.json'));if(spec.lifecycle)await lifecycleBranch(initialFinality,publicFiles.get('profile.json'));
  if(spec.backupRestore){
   const before=await stopped('vps-docker');restoreReceipt=before.receipt;const nonce=randomUUID();restoreStopEnvelope={format:'nano-fresh-capella-independent-stop-v1',stopNonce:nonce,receipt:restoreReceipt};const stopBytes=await save(join(output,'independent-stop.json'),restoreStopEnvelope),stopSha=sha(stopBytes);
   const withWitness=await capturePair('vps-docker',restoreReceipt,'backup-source');
   const rd=join(output,'restore');await mkdir(rd,{mode:0o700});const original=JSON.parse(await readFile(join(prepared,'vps-docker','node.json'),'utf8')),identities=preparedManifest.nodes[2].signerIdentities,pinned=pinnedRestoreConfiguration(original,restoreReceipt,identities);const request=restoreAuditRequest(original,restoreReceipt);
   await save(join(rd,'node.json'),pinned);await save(join(rd,'audit-request.json'),request);await sh(['mkdir','-m','700',remoteRoot+'/restore']);await exec('scp',['-q',join(output,'independent-stop.json'),join(rd,'audit-request.json'),target+':'+remoteRoot+'/restore/'],{timeout:10000});
   // The original data AND witness volumes stay stopped/read-only. The shell
   // receives the independent receipt SHA, never pins derived from its archive.
   const aa=owner('vps-docker');const script=join(harnessRoot,'tests/multihost/backup-restore.sh');await observe(script,null,true);await exec('scp',['-q',script,target+':'+remoteRoot+'/restore/backup-restore.sh'],{timeout:10000});
   const begun=Date.now();await docker(['run','--rm',...safe('vps-docker'),...dataMount('vps-docker',true),'--mount',`type=bind,src=${configAt('vps-docker')},dst=/config,readonly`,'--mount',`type=bind,src=${remoteRoot}/restore,dst=/proof`,'--entrypoint','bash',aa.image,'/proof/backup-restore.sh','--two-roots',rootFor('vps-docker','data'),rootFor('vps-docker','witness'),'/proof/backup','/config/genesis.json','/proof/audit-request.json',aa.programs.nano.path,'/proof/independent-stop.json',stopSha],true,180000);
   need(Date.now()-begun<=Number(BigInt(spec.clock.maximumDowntimeMs)),'Restore exceeded explicit downtime bound');const stopAgain=JSON.parse(await observe(join(output,'independent-stop.json'),{sha256:stopSha,bytes:stopBytes.length},true,false));need(stopAgain.stopNonce===nonce&&canonical(stopAgain.receipt)===canonical(restoreReceipt),'Independent stop receipt changed');
   restoreAudit=await remotePublic({mode:'audit-read',prefix:'backup-audit',path:remoteRoot+'/restore/backup/audit.json'});verifyRestoredFinalityAudit(request,restoreReceipt,restoreAudit);await save(join(rd,'audit.json'),restoreAudit);
   await docker(['rm',containers['vps-docker']],true);await exec('scp',['-q',join(rd,'node.json'),target+':'+configAt('vps-docker')+'/node.json'],{timeout:10000});
   restoredMounts=['--mount',`type=bind,src=${remoteRoot}/restore/backup/restored-data,dst=${rootFor('vps-docker','data')}`,'--mount',`type=bind,src=${remoteRoot}/restore/backup/restored-witness,dst=${rootFor('vps-docker','witness')}`];
   // Same virtual namespaces, explicitly new physical roots. Independent P/W
   // capture runs on those exact restored mounts before any writer is started.
   const restoredCapture=await remotePublic({mode:'capture-once',prefix:'restored-before-runtime',argv:captureVmArgv(withWitness.remoteStopFile,restoredMounts.map(x=>x.startsWith('type=')?x+',readonly':x))});need(restoredCapture.externalWitnessTokensMatched===true,'Restored copies must authenticate original independent witness pins');await save(join(rd,'restored-capture.json'),restoredCapture);
   await startDocker('vps-docker',restoredMounts);let resumedPeer;for(let i=0;i<60;i++){if(interrupted)throw interrupted;try{resumedPeer=(await rpc('vps-docker','nno_getNetworkInfo')).node?.peerId;if(resumedPeer)break;}catch{if(interrupted)throw interrupted;}await delay(1000);}if(interrupted)throw interrupted;need(resumedPeer===remotePeer,'Restored peer identity changed');
   await finality({epoch:BigInt(restoreReceipt.finalized.epoch).toString(),root:restoreReceipt.finalized.root},'restored',false);
  }
 }catch(e){failure=e;await record({event:'fresh-failed',message:e.stack,productionQualified:false});}
 finally{
  const terminal=[];for(const name of [...started])try{const x=await stopped(name);terminal.push({name,receipt:x.receipt});}catch(e){failure??=e;await record({event:'fresh-clean-stop-failed',owner:name,message:e.message});}
  // Also join a child spawned before it could be added to started.
  for(const [child,name,timeout] of [[native,'Native runtime',30000],[tunnel,'Tunnel',5000]])try{if(await terminateJoined(child,name,timeout))failure??=Error(name+' cleanup required SIGKILL');}catch(e){failure??=e;}
  publicCheckStage='terminal-capture-and-recovery-audit';
  try{need(childAbsent(native)&&childAbsent(tunnel),'Native runtime and tunnel must exit and close before stopped captures');need(terminal.length===3,'All three terminal signer receipts required');for(const z of terminal){if(spec.lifecycle)lifecycleTerminalCaptures.set(z.name,await lifecycleCapture(z.name,z.receipt,'terminal-'+z.name));else await capturePair(z.name,z.receipt,'terminal-'+z.name);}if(spec.partitionRotation)await auditFreshRotation(terminal);if(spec.lifecycle)await lifecycleClosing(terminal);for(const z of terminal)need(z.receipt.validator.healthy===true&&z.receipt.validator.failure===null&&z.receipt.validator.attestationsAdmitted>0,'Each owner must admit duties');if(spec.backupRestore){const restored=terminal.find(z=>z.name==='vps-docker')?.receipt,stable=JSON.parse(await readFile(join(output,'restored-finality.json'),'utf8')).result;validateRestoredFinality(restoreReceipt,stable,restored);
    const original=JSON.parse(await readFile(join(prepared,'vps-docker','node.json'),'utf8')),request=restoredFinalityAuditRequest(original,restoreReceipt,stable,restored,preparedManifest.nodes[2].signerIdentities),requestPath=join(output,'restored-finalized-audit-request.json');await save(requestPath,request);await exec('scp',['-q',requestPath,target+':'+remoteRoot+'/restore/finalized-audit-request.json'],{timeout:10000});const a=owner('vps-docker');const auditName='nano-qualification-op-'+uuid+'-'+(++operation);ownedRuns.push({name:auditName,remote:true});const audit=await remotePublic({mode:'audit-run',prefix:'restored-finalized-audit',argv:['sudo','-n','docker','run','--name',auditName,'--label',label,'--rm',...safe('vps-docker'),...restoredMounts.map(x=>x.startsWith('type=')?x+',readonly':x),'--mount',`type=bind,src=${configAt('vps-docker')},dst=/config,readonly`,'--mount',`type=bind,src=${remoteRoot}/restore/finalized-audit-request.json,dst=/request.json,readonly`,'--entrypoint',a.programs.nano.path,a.image,'audit-capella','/config/genesis.json',rootFor('vps-docker','data')+'/chain','/request.json']});verifyRestoredFinalityAudit(request,restored,audit);await save(join(output,'restored-finalized-audit.json'),audit);
   }
   publicCheckStage='final-linux-fences';await macLinux.verifyAll();if(macBootstrap)await macBootstrap.fenceSource();publicCheckStage='final-software-read';const finalSoftware=await software();publicCheckStage='final-software-comparison';need(initialSoftware&&canonical(finalSoftware)===canonical(initialSoftware),'Exact public software changed');publicCheckStage='final-held-fence';await fence();publicCheckStage='final-new-output-fence';for(const [path,z] of newFiles)await observe(path,z,false,false);publicCheckStage='terminal-receipts-publication';await save(join(output,'terminal-receipts.json'),terminal);publicCheckStage='terminal-receipts-complete';
  }catch(e){failure??=e;await record({event:'fresh-final-fence-failed',stage:publicCheckStage,message:e.message,publicInputDiagnostic:e.publicInputDiagnostic??null});}
  for(const x of ownedRuns)try{const raw=await docker(['ps','-a','--filter','name=^/'+x.name+'$','--format','{{.Names}}'],x.remote);if(raw.trim()){const inspect=JSON.parse(await docker(['inspect',x.name,'--format','{{json .}}'],x.remote));need(inspect.Name==='/'+x.name&&inspect.Image===owner(x.remote?'vps-docker':'mac-docker').image&&inspect.Config.Labels?.['nano.fresh.campaign']===uuid,'Cleanup owner image/label mismatch');if(inspect.State.Running){await docker(['stop','--time','10',x.name],x.remote,20000);failure??=Error('Late owned container required cleanup');}}}catch(e){failure??=e;}
  try{if(macLinux){await macLinux.verifyAll();if(macBootstrap)await macBootstrap.fenceSource();}}catch(e){failure??=e;}
  // No pruning. Named volumes, stopped containers, private synthetic fixture
  // and exact reports remain for ROOT's separate cold audit and next phase.
  for(const name of ['mac-docker','vps-docker'])try{const active=(await docker(['ps','--filter','label='+label,'--format','{{.Names}}'],name==='vps-docker')).trim();need(!active,'Owned campaign still has running container');}catch(e){failure??=e;}
  if(partitionNetworkCreated)try{
   const network=JSON.parse(await docker(['network','inspect',partitionNetwork,'--format','{{json .}}'],true));need(network.Name===partitionNetwork&&network.Labels?.['nano.fresh.campaign']===uuid,'Only owned fresh partition network may be cleaned');const found=(await docker(['ps','-a','--filter','name=^/'+containers['vps-docker']+'$','--format','{{.Names}}'],true)).trim();if(found){need(found===containers['vps-docker'],'Exact partition container');const state=JSON.parse(await docker(['inspect',found,'--format','{{json .}}'],true));need(state.State.Running===false,'Partition network cleanup requires stopped runtime');if(state.NetworkSettings.Networks[partitionNetwork])await docker(['network','disconnect',partitionNetwork,found],true);}await docker(['network','rm',partitionNetwork],true);const left=(await docker(['network','ls','--filter','label='+label,'--format','{{.Name}}'],true)).trim().split('\n');need(!left.includes(partitionNetwork),'Owned partition network remains');partitionNetworkAbsent=true;
  }catch(e){failure??=e;}
  const nativeProcessAbsent=childAbsent(native),tunnelProcessAbsent=childAbsent(tunnel);try{need(nativeProcessAbsent&&tunnelProcessAbsent,'Native runtime/tunnel exit and close join required at terminal');}catch(e){failure??=e;}
  if(interrupted)failure??=interrupted;process.removeListener('SIGINT',onSignal);process.removeListener('SIGTERM',onSignal);if(lifecycleWholeTimer)clearTimeout(lifecycleWholeTimer);
  await save(join(output,'result.json'),{format:'nano-fresh-capella-three-owner-result-v1',passed:!failure,sourceBaseRevision:revision,overlayManifestSha256:spec.overlayManifest.sha256,campaignId:manifest?.campaignId??null,runtimes:3,physicalHosts:2,registeredSyntheticKeys:64,pendingSyntheticKeys:1,backupRestoreRequested:spec.backupRestore,lifecycleRequested:Boolean(spec.lifecycle),freshSyntheticLifecycleQualified:Boolean(spec.lifecycle)&&lifecycleQualified&&!failure,partitionRotationRequested:Boolean(spec.partitionRotation),partitionRotationQualified:Boolean(spec.partitionRotation)&&!failure,partitionNetworkAbsent,nativeProcessAbsent,tunnelProcessAbsent,originalOperationalRecoveryQualified:false,networkProductionQualified:false,productionQualified:false});
 }
 if(failure)throw failure;
}
