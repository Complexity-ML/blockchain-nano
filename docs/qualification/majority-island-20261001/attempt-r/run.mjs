#!/usr/bin/env node
// Authorized isolated devnet only; code is deployed through Git, not this script.
import {execFile,spawn} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdir,writeFile,readFile,appendFile} from 'node:fs/promises';
import {openSync,closeSync} from 'node:fs';
import {resolve,join} from 'node:path';
import {setTimeout as delay} from 'node:timers/promises';
import {createHash} from 'node:crypto';
import {validateStoppedValidators} from './validator-gate.mjs';
import {partitionRemote} from './partition.mjs';
import {fixtureStake,majorityIslandPolicy} from './majority-island.mjs';
import {partitionSchedule,runPartitionCycles} from './partition-cycles.mjs';
import {asymmetricRemote,createRemoteTc} from './asymmetric.mjs';
import {observeRecovery} from './recovery-observation.mjs';
import {sampleClock} from './clock-sample.mjs';
import {captureFaultProgress} from './fault-progress.mjs';
import {createRotationCapture,ownership,rotationNodes,waitRotationWindow} from './rotation.mjs';
import {runBoundedLoad} from './bounded-load.mjs';
import {stoppedReceipt,pinnedRestoreConfiguration,restoreAuditRequest,validateRestoredFinality} from './restore-config.mjs';
import {waitRestoredRecovery} from './restore-recovery.mjs';
import {startFinalityMonitor} from './monitor.mjs';
import {selectCandidates,verifyPackagedImage} from './candidate-images.mjs';
import {advanceRestartConfiguration} from './restart-pins.mjs';
import {decodeHttpResponse,httpStatusTrailer} from './http-response.mjs';
const exec=promisify(execFile),[target,revision,outputArg]=process.argv.slice(2);
const fork=process.env.NANO_TEST_CONSENSUS_PROFILE??'bellatrix';
if(!['bellatrix','capella'].includes(fork))throw Error('Unsupported consensus profile');
if(!/^[a-z_][a-z0-9_-]*@[0-9.]+$/.test(target??'')||!(/^[0-9a-f]{40}$/).test(revision??'')||!outputArg)throw Error('Usage: run.mjs user@IPv4 core-revision fresh-output');
const monitor=process.env.NANO_TEST_FINALITY_MONITOR==='1';
const buildVolume=process.env.NANO_TEST_BUILD_VOLUME==='1';
const candidateFile=process.env.NANO_TEST_CANDIDATE_MANIFEST;
if(candidateFile&&buildVolume)throw Error('Select packaged images or build volume, not both');
const candidates=candidateFile?selectCandidates(JSON.parse(await readFile(resolve(candidateFile),'utf8')),revision):null;
const privateTransport=buildVolume||Boolean(candidates);
const output=resolve(outputArg),repo=process.cwd(),image=candidates?.arm64.image??(buildVolume?'nano-build-dependencies:local':`blockchain-nano:${revision.slice(0,7)}`),remoteImage=candidates?.amd64.image??image;
const nofile=process.env.NANO_DOCKER_NOFILE??'65536';
if(!/^[1-9]\d*$/.test(nofile)||!Number.isSafeInteger(Number(nofile)))throw Error('NANO_DOCKER_NOFILE requires a positive safe integer');
const descriptorArgs=['--ulimit',`nofile=${nofile}:${nofile}`];
const artifactMount=buildVolume?['--mount','type=volume,src=nano-linux-sync-build,dst=/build,readonly']:[];
const runtimeCommand=buildVolume?['/build/nano']:[];
const traceArgs=process.env.NANO_TEST_NETWORK_TRACE==='1'?['--env','NANO_TEST_NETWORK_TRACE=1']:[];
const id=Date.now().toString(36),local=`nano-qualification-mac-${id}`,remote=`nano-qualification-vps-${id}`;
const partition=process.env.NANO_TEST_WAN_PARTITION==='1',asymmetric=process.env.NANO_TEST_WAN_ASYMMETRIC==='1',testNetwork=remote+'-net';
if(partition&&asymmetric)throw Error('Select only one WAN fault');
const rotation=process.env.NANO_TEST_WAN_ROTATION==='1';
const boundedLoad=process.env.NANO_TEST_BOUNDED_LOAD==='1';
const partitionCuts=partitionSchedule(process.env.NANO_TEST_PARTITION_SCHEDULE_MS);
const majorityIsland=process.env.NANO_TEST_MAJORITY_ISLAND==='1';
const faultProgress=majorityIsland||process.env.NANO_TEST_FAULT_PROGRESS==='1';
if(faultProgress&&(fork!=='capella'||!partition))throw Error('Fault progress diagnostics require Capella partition campaign');
const backupRestore=process.env.NANO_TEST_BACKUP_RESTORE==='1';
const pinnedRestart=process.env.NANO_TEST_PINNED_RESTART==='1';
if(partitionCuts&&(!candidates||fork!=='capella'||!partition||asymmetric||rotation||boundedLoad||backupRestore||pinnedRestart||monitor))throw Error('Repeated partitions require standalone packaged Capella partition campaign');
if(majorityIsland&&(!candidates||fork!=='capella'||!partition||asymmetric||rotation||boundedLoad||backupRestore||pinnedRestart||monitor||partitionCuts))throw Error('Majority island requires standalone packaged Capella partition campaign');
if(pinnedRestart&&(!candidates||fork!=='capella'||monitor||backupRestore||partition||asymmetric||rotation||boundedLoad))throw Error('Pinned container replacement requires standalone packaged Capella images');
if(backupRestore&&(fork!=='capella'||!buildVolume||partition||asymmetric||rotation||boundedLoad))throw Error('Backup restore requires standalone Capella build-volume campaign');
if(boundedLoad&&(fork!=='capella'||asymmetric||rotation))throw Error('Bounded load requires Capella; asymmetric/rotation combinations are not qualified yet');
if(rotation&&(fork!=='capella'||!partition||!buildVolume))throw Error('Rotation requires Capella, the isolated partition and build volumes');
const remoteRoot=`/home/${target.split('@')[0]}/nano-qualification-${id}`,ip=target.split('@')[1];
const quote=s=>"'"+String(s).replaceAll("'","'\\''")+"'";
const sh=async (args,timeout=30000)=>(await exec('ssh',['-o','BatchMode=yes','-o','ConnectTimeout=8',target,args.map(quote).join(' ')],{timeout,maxBuffer:8*1024*1024})).stdout;
const docker=async(args,onRemote=false,timeout=30000)=>onRemote?sh(['sudo','-n','docker',...args.map(a=>a===image?remoteImage:a)],timeout):(await exec('docker',args,{timeout,maxBuffer:8*1024*1024})).stdout;
const captureLogs=async(name,onRemote)=>{
 const args=onRemote?['ssh','-o','BatchMode=yes',target,['sudo','-n','docker','logs',name].map(quote).join(' ')+' 2>&1']:['docker','logs',name];
 const result=await exec(args[0],args.slice(1),{timeout:30000,maxBuffer:8*1024*1024});
 return result.stdout+result.stderr;
};
const json=async p=>JSON.parse(await readFile(p,'utf8'));
// Preflight before creating fixtures, so compilation cannot age the genesis.
const artifacts=[];
await exec('git',['diff','--quiet',revision,'--','src','include','network','third_party']);
await sh(['git','-C',`/home/${target.split('@')[0]}/blockchain-nano-git`,'diff','--quiet',revision,'--','src','include','network','third_party']);
for(const onRemote of [false,true]){
 const info=JSON.parse(await docker(['image','inspect',image,'--format','{{json .}}'],onRemote));
 if((!buildVolume&&info.Config.Labels?.['org.opencontainers.image.revision']!==revision)||info.Architecture!==(onRemote?'amd64':'arm64'))throw Error('Build revision/architecture mismatch');
 if(candidates){
  const spec=candidates[onRemote?'amd64':'arm64'];
  const hashes=await docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--entrypoint','/usr/bin/sha256sum',image,'/usr/local/bin/nano','/usr/local/bin/nano-p2p'],onRemote);
  artifacts.push({host:onRemote?'vps':'docker-mac',imageId:info.Id,revision,executables:verifyPackagedImage(spec,info,hashes),packaged:true});
 }
 if(buildVolume){
  const built=(await docker(['run','--rm','--network','none',...artifactMount,image,'cat','/build/production-source-revision.txt'],onRemote)).trim();
  if(built!==revision)throw Error('Build volume source revision mismatch');
  const hashes=await docker(['run','--rm','--network','none',...artifactMount,image,'sha256sum','/build/nano','/build/network/nano-p2p',...(rotation?['/build/nano-capella-rotation-audit']:[])],onRemote);
  artifacts.push({host:onRemote?'vps':'docker-mac',imageId:info.Id,revision:built,sha256:hashes});
 }
}
await mkdir(output);
const log=async event=>appendFile(join(output,'run.jsonl'),JSON.stringify({time:Date.now(),...event})+'\n');
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
 const binary=join(repo,'build-release/nano'),p2p=join(repo,'build-network/nano-p2p');
 if(rotation)await log({event:'rotation-audit-binary',sha256:createHash('sha256').update(await readFile(join(repo,'build-release/nano-capella-rotation-audit'))).digest('hex')});
 const harness={};for(const name of ['run.mjs','majority-island.mjs','partition.mjs','partition-cycles.mjs','asymmetric.mjs','observe.mjs','validator-gate.mjs','recovery-observation.mjs','clock-sample.mjs','fault-progress.mjs','prepare.mjs','rotation.mjs','bounded-load.mjs','load-receipts.mjs','restore-config.mjs','restore-recovery.mjs','backup-restore.sh','monitor.mjs','candidate-images.mjs','restart-pins.mjs','http-response.mjs'])harness[name]=createHash('sha256').update(await readFile(join(repo,'tests/multihost',name))).digest('hex');
 if(monitor&&(fork!=='capella'||!privateTransport||partition||asymmetric||backupRestore||rotation||boundedLoad))throw Error('Monitor qualification requires standalone private Capella campaign');
 await log({event:'harness',monitor,partition,asymmetric,rotation,boundedLoad,backupRestore,pinnedRestart,partitionCuts,majorityIsland,faultProgress,fork,buildVolume,packagedImages:Boolean(candidates),artifacts,sha256:harness});
 await log({event:'binaries',revision,nanoSha256:createHash('sha256').update(await readFile(binary)).digest('hex'),p2pSha256:createHash('sha256').update(await readFile(p2p)).digest('hex')});
 const assets=join(output,'assets'),prepared=join(output,'prepared');
 await exec(join(repo,`build-release/nano-${fork}-transition-tests`),[fork==='capella'?'--export-network':'--export-kurtosis',assets],{timeout:30000});
 const topology=[{name:'mac-native',host:'mac',mode:'native',rpcPort:18545,p2pPort:30305,p2pBinary:p2p,runtimeDirectory:join(prepared,'mac-native')},{name:'mac-docker',host:'mac',mode:'docker',rpcPort:8545,p2pPort:30303},{name:'vps-docker',host:target,mode:'docker',rpcPort:8545,p2pPort:30303}];
 await writeFile(join(output,'topology.json'),JSON.stringify(topology));
 await exec(process.execPath,['tests/multihost/prepare.mjs',assets,prepared,join(output,'topology.json')]);
 if(buildVolume)for(const name of ['mac-docker','vps-docker']){const path=join(prepared,name,'node.json'),c=await json(path);c.p2p_binary='/build/network/nano-p2p';await writeFile(path,JSON.stringify(c));}
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
  await docker(['run','--rm','--network','none',...descriptorArgs,...artifactMount,'--cpus','2','--memory','3g','--mount',`type=volume,src=${remote}-data,dst=/source,readonly`,'--mount',`type=bind,src=${remoteRoot}/config,dst=/config,readonly`,'--mount',`type=bind,src=${restoreProof},dst=/proof`,'--mount',`type=bind,src=/home/${target.split('@')[0]}/blockchain-nano-git,dst=/work,readonly`,image,'bash','/work/tests/multihost/backup-restore.sh','/source','/proof/backup','/config/genesis.json','/proof/audit-request.json','/build/nano'],true,90000);
  const audit=JSON.parse(await sh(['sudo','-n','cat',restoreProof+'/backup/audit.json']));if(audit.canonicalHistoryVerified!==true)throw Error('Restored canonical audit not verified');
  const metadata=['source-before.sha256','source-after.sha256','restored-before.sha256','restored-after.sha256','archive.sha256','inputs.sha256','audit.json','passed.txt'];
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
 if(boundedLoad)await runBoundedLoad({rpc,genesisHash:genesis.hash,output,sampleFinality:faultSample,
  ...(partition?{duringSubmit:submit=>partitionRemote({docker:args=>docker(args,true),container:remote,
   network:testNetwork,revision,record:faultRecord,sample:faultSample,duringCut:submit})}:{})});
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
  }else if(partition&&!boundedLoad)await partitionRemote({docker:args=>docker(args,true),container:remote,network:testNetwork,revision,record:faultRecord,sample:faultSample,...(majorityIsland?{cutMs:192000,policy:islandPolicy}:{})});
  if(asymmetric){
   const tc=await createRemoteTc({docker:args=>docker(args,true),sh,container:remote,network:testNetwork,revision});
   await asymmetricRemote({tc,sample:faultSample,record:log});
  }
 },
 observe:()=>exec(process.execPath,['tests/multihost/observe.mjs',join(output,'observers.json'),join(output,'observations')],{timeout:480000,maxBuffer:8*1024*1024})});
 await log({event:'finality-passed'});
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
   await docker(['run','--rm','--network','none','--mount',`type=volume,src=${remote}-data,dst=/source,readonly`,'--mount',`type=bind,src=${restoreProof},dst=/proof,readonly`,image,'bash','-lc','set -e; cd /source; find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum | cmp /proof/backup/source-before.sha256 -'],true);
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
   audits.push(JSON.parse((await exec(join(repo,'build-release/nano-capella-rotation-audit'),[join(nd,'genesis.json'),join(nd,'node.json'),join(nd,'data/chain'),proofFile,rotationNodes[0]],{timeout:120000,maxBuffer:1024*1024})).stdout));
   await writeFile(join(output,'rotation-audits.json'),JSON.stringify(audits,null,2));
   for(const [name,onRemote,index] of [[local,false,1],[remote,true,2]]){
    const config=onRemote?remoteRoot+'/config':join(output,'prepared/mac-docker');
    const proof=onRemote?remoteRoot+'/rotation-proof.json':proofFile;
    audits.push(JSON.parse(await docker(['run','--rm','--network','none',...artifactMount,'--mount',`type=bind,src=${config},dst=/config,readonly`,'--mount',`type=bind,src=${proof},dst=/proof.json,readonly`,'--mount',`type=volume,src=${name}-data,dst=/var/lib/nano`,image,'/build/nano-capella-rotation-audit','/config/genesis.json','/config/node.json','/var/lib/nano/chain','/proof.json',rotationNodes[index]],onRemote,120000)));
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
