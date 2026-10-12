#!/usr/bin/env node
// Private live baseline from authenticated physical bootstrap journals. This
// controller never manufactures keys or reuses a consumed startup directory.
import assert from 'node:assert/strict';
import {execFile,spawn} from 'node:child_process';
import {promisify} from 'node:util';
import {readFile,writeFile,mkdir,appendFile,lstat} from 'node:fs/promises';
import {openSync,closeSync,constants,fstatSync,readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {createServer} from 'node:net';
import {join,resolve,basename} from 'node:path';
import {setTimeout as wait} from 'node:timers/promises';
import {performance} from 'node:perf_hooks';
import {persistentRuntimeDirectory} from './persistent-runtime-directory.mjs';
import {waitRestoredReadiness} from './restored-readiness.mjs';
import {isolatedRuntimeObservation,isolatedPendingObservation,isolatedBootstrapStateCheckpoint} from './isolated-runtime-observation.mjs';
import {beaconObservationError} from './restored-http-observation.mjs';
import {readPinnedJson} from './pinned-json-input.mjs';
import {verifyIsolatedCoreCmake} from './isolated-core-source.mjs';
const exec=promisify(execFile),names=['mac-native','mac-docker','vps-docker'];
const [workArg,target='ubuntu@145.239.199.166']=process.argv.slice(2);
assert.equal(target,'ubuntu@145.239.199.166');
const work=await persistentRuntimeDirectory(workArg),evidence=resolve(work,'..'),repo=process.cwd();
assert.ok(['isolated-three-node-cb532-20261012','isolated-three-node-cb532-restored-20261012'].includes(basename(work)));
const restoring=basename(work)==='isolated-three-node-cb532-restored-20261012',captures={};
const out=join(work,restoring?'live-restored':'live-baseline');await mkdir(out,{mode:0o700});
const fixture=join(evidence,'isolated-fixture-exporter/native-controls/campaign-fixture');
const remoteWork='/home/ubuntu/nanobees-qualification/'+basename(work);
const dockerBin='/Users/boris/.docker/bin/docker',sshBin='/usr/bin/ssh';
const q=x=>"'"+String(x).replaceAll("'","'\\''")+"'";
const run=async(bin,args,timeout=30000)=>(await exec(bin,args,{timeout,maxBuffer:32*1024*1024})).stdout;
const ssh=(args,timeout)=>run(sshBin,['-4','-o','BatchMode=yes','-o','ConnectTimeout=8',target,args.map(q).join(' ')],timeout);
const docker=(args,remote=false,timeout)=>remote?ssh(['sudo','-n','docker',...args],timeout):run(dockerBin,args,timeout);
const save=(p,v)=>writeFile(p,JSON.stringify(v,null,2)+'\n',{mode:0o600});
const json=async p=>JSON.parse(await readFile(p,'utf8'));
const hash=async p=>createHash('sha256').update(await readFile(p)).digest('hex');
const events=s=>s.split('\n').flatMap(line=>{try{return [JSON.parse(line)];}catch{return [];}});
const record=async value=>{const row={time:new Date().toISOString(),...value};console.log(JSON.stringify(row));await appendFile(join(out,'events.jsonl'),JSON.stringify(row)+'\n',{mode:0o600});};
const port=()=>new Promise((ok,no)=>{const socket=createServer();socket.on('error',no);socket.listen(0,'127.0.0.1',()=>{const p=socket.address().port;socket.close(()=>ok(p));});});
const instance=Date.now().toString(36),local='nano-isolated-cb532-mac-'+instance,remote='nano-isolated-cb532-vps-'+instance;
let runtimeRestoreReady=false,physicalAuditsPassed=false,native,tunnel,localCreated=false,remoteCreated=false,failure,common,result='FAILED',controllerHashes,plan,assets,observer,baseline;
const inspect=async(name,onRemote)=>JSON.parse(await docker(['inspect',name,'--format','{{json .State}}'],onRemote));
const logs=async(name,onRemote)=>{const r=onRemote?await exec(sshBin,['-4','-o','BatchMode=yes','-o','ConnectTimeout=8','-o','ServerAliveInterval=10','-o','ServerAliveCountMax=2',target,['sudo','-n','docker','logs',name].map(q).join(' ')+' 2>&1'],{timeout:30000,maxBuffer:32*1024*1024}):await exec(dockerBin,['logs',name],{timeout:30000,maxBuffer:32*1024*1024});return r.stdout+r.stderr;};
const spawnLog=(bin,args,name,env)=>{const fd=openSync(join(out,name),'ax',0o600);const child=spawn(bin,args,{env,stdio:['ignore',fd,fd]});closeSync(fd);child.on('error',e=>{failure??=e;});return child;};
const stop=async child=>{if(!child||!child.pid||child.exitCode!==null||child.signalCode!==null)return;child.kill('SIGTERM');for(let n=0;n<200&&child.exitCode===null&&child.signalCode===null;n++)await wait(100);if(child.exitCode===null&&child.signalCode===null){child.kill('SIGKILL');for(let n=0;n<50&&child.exitCode===null&&child.signalCode===null;n++)await wait(100);}if(child.exitCode===null&&child.signalCode===null)throw Error('Managed process is still alive after bounded SIGKILL wait');};
const controllers=['isolated-three-node-run.mjs','isolated-runtime-observation.mjs','persistent-runtime-directory.mjs','restored-readiness.mjs','restored-http-observation.mjs','restored-finality-progress.mjs','finality-observation.mjs','isolated-fixture-domain.mjs','pinned-json-input.mjs','isolated-core-source.mjs'];
const controllerBindings=async()=>Object.fromEntries(await Promise.all(controllers.map(async name=>[name,await hash(join(repo,'tests/capella',name))])));
try{
 const pins=(await readPinnedJson(join(work,'pins-before-copy.json'),'766d2bbe7eb48fbfa88f38b503dd8404eb3bc2dc23637bf8c5c15d3dad832af7')).value;
 assert.equal(pins.fixtureManifestSha256,'ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6');
 assert.equal(pins.planDocumentSha256,'0d234e822f27f9152527ea627a7f6782db438e96376063a3474e03f79d8a2d51');
 assert.equal(pins.sourceInventorySha256,'bc2dd8b8ea94de6dd80373f4ac0498918c001d830c7fceaf04baf85c1ea08fc5');
 assert.equal(await hash(join(work,'audit-inputs/manifest.json')),pins.fixtureManifestSha256);
 plan=(await readPinnedJson(join(work,'audit-inputs/plan.json'),pins.planDocumentSha256)).value;
 assert.equal(plan.planSha256,'4e1e6f5763fd0258ee05857abc1ef233b4ed8ccff1e6923c1b21b8040dbb104f');
 assert.equal(plan.campaignNamespace,'isolated-capella-cb532955c358607d94e4e1074e26be4f4a235dd61b1a00ac2117129973a385ed');
 for(const [i,name] of names.entries()){
  const audit=await json(join(work,name+'-physical-audit.json'));
  assert.equal(audit.passed,true);assert.equal(audit.owner,name);assert.equal(audit.manifestSha256,pins.fixtureManifestSha256);
  assert.equal(audit.planDocumentSha256,pins.planDocumentSha256);assert.equal(audit.planSha256,plan.planSha256);
  assert.equal(audit.anchorsSha256,'766d2bbe7eb48fbfa88f38b503dd8404eb3bc2dc23637bf8c5c15d3dad832af7');
  assert.equal(audit.sourceInventorySha256,pins.sourceInventorySha256);assert.equal(audit.primaryWitnessPairs,[16,16,33][i]);
  assert.equal(audit.coldPairReopens,[32,32,66][i]);assert.equal(audit.newSignaturesCreated,0);assert.equal(audit.runtimeStarted,false);
 }
 if(restoring){
  const indexInput=await readPinnedJson(join(evidence,'isolated-stopped-capture-20261012/capture-index.json'));
  const prepared=(await readPinnedJson(join(work,'full-restore-preparation.json'))).value;assert.equal(prepared.captureIndexSha256,indexInput.sha256);
  const index=indexInput.value;assert.equal(index.complete,true);
  assert.equal(prepared.sourceAllWritersStopped,true);assert.equal(prepared.sourceFullInventoriesUnchanged,true);assert.equal(prepared.sourceDestinationInodesIndependent,true);assert.equal(prepared.fullFilesBytesAndModesCopied,true);
  for(const owner of names){
   const path=join(evidence,'isolated-stopped-capture-20261012',owner+'-capture.public.json');
   const capture=(await readPinnedJson(path,index.captures[owner].sha256)).value;assert.equal(capture.passed,true);assert.equal(capture.owner,owner);assert.equal(capture.campaignId,plan.campaignId);assert.equal(capture.manifestSha256,pins.fixtureManifestSha256);
   assert.equal(capture.allObservedFileBytesAndInodesUnchanged,true);assert.equal(capture.externalStopTokensMatched,true);assert.equal(capture.independentWitnessCurrentIntentsMatched,true);assert.equal(capture.canonicalStoreSnapshotVerified,true);assert.deepEqual(capture.restart,capture.core.primary);assert.deepEqual(capture.recoveryWitnessCommitment,capture.core.witness);assert.equal(capture.envRead,false);assert.equal(capture.runtimeStarted,false);assert.equal(capture.primaryWitnessPairs,owner==='vps-docker'?33:16);
   const destPath=join(work,owner+'-restored-capture.public.json');const dest=(await readPinnedJson(destPath,prepared.destinationCaptureHashes[owner])).value;
   assert.equal(dest.passed,true);assert.equal(dest.owner,owner);assert.equal(dest.manifestSha256,pins.fixtureManifestSha256);assert.equal(dest.campaignId,plan.campaignId);assert.equal(dest.externalStopTokensMatched,true);assert.equal(dest.independentWitnessCurrentIntentsMatched,true);assert.equal(dest.canonicalStoreSnapshotVerified,true);assert.deepEqual(dest.core,capture.core);assert.equal(dest.allObservedFileBytesAndInodesUnchanged,true);assert.deepEqual(dest.restart,capture.restart);assert.deepEqual(dest.recoveryWitnessCommitment,capture.recoveryWitnessCommitment);
   assert.deepEqual(dest.validatorKeys.map(k=>({i:k.publicKeyIndex,p:k.primary,w:k.witness,pub:k.publicKey,proposal:k.proposal,attestation:k.attestation,snapshotSha256:k.snapshotSha256,witnessSnapshotSha256:k.witnessSnapshotSha256})),capture.validatorKeys.map(k=>({i:k.publicKeyIndex,p:k.primary,w:k.witness,pub:k.publicKey,proposal:k.proposal,attestation:k.attestation,snapshotSha256:k.snapshotSha256,witnessSnapshotSha256:k.witnessSnapshotSha256})));
   captures[owner]=capture;
  }
 }
 physicalAuditsPassed=true;
 const copies=await json(join(work,'copy-receipt.json'));assert.equal(copies.sourceFullInventoryUnchanged,true);assert.equal(copies.sourceDestinationInodesIndependent,true);
 assets=(await readPinnedJson(join(evidence,'isolated-runtime-assets-20261012/runtime-assets-manifest.json'),'ab31dd791b3e761808bbe889e95a5f902a4bf000fed4feba4ea7c84fab11b6af')).value;
 assert.equal(assets.productionCoreSourceDiffEmpty,true);assert.equal(assets.relaySourceDiffEmpty,true);
 const sanityBinding=assets.assets['mac-native'].p2p.protocolSanityQualification;
 const sanity=(await readPinnedJson(sanityBinding.receipt,sanityBinding.receiptSha256)).value;
 assert.equal(sanity.passed,true);assert.equal(sanity.binarySha256,assets.assets['mac-native'].p2p.sha256);
 await run('git',['diff','--quiet',assets.coreRevision,'--','src','include','cmake','third_party']);
 assert.equal(assets.coreRevision,'85eb492f641452c4746ee60f027eacefa775b0b0');
 const baselineCmake=Buffer.from(await run('git',['show',assets.coreRevision+':CMakeLists.txt']));
 const cmakeBinding=verifyIsolatedCoreCmake(baselineCmake,await readFile(join(repo,'CMakeLists.txt')));
 await save(join(out,'cmake-source-binding.json'),cmakeBinding);
 await run('git',['diff','--quiet',assets.relayRevision,'--','network']);
 assert.equal((await ssh(['git','-C','/home/ubuntu/blockchain-nano-git','rev-parse','HEAD'])).trim(),(await run('git',['rev-parse','HEAD'])).trim());
 const processes=await run('ps',['-axo','pid,command']);assert.ok(!processes.split('\n').some(line=>/\/nano(?: |$)/.test(line)&&/\brun\b/.test(line)),'An existing native Nano writer is active');
 for(const onRemote of [false,true]){
  const ids=(await docker(['ps','-q'],onRemote)).trim().split('\n').filter(Boolean);
  if(ids.length){const active=JSON.parse(await docker(['inspect',...ids],onRemote));assert.ok(!active.some(c=>c.Name.includes('nano-isolated-cb532')||c.Config?.Labels?.['nano.test.campaign']===plan.campaignId),'An isolated campaign container is already active');const root=onRemote?remoteWork:work;assert.ok(!active.some(c=>c.Mounts.some(m=>m.Source===root||m.Source.startsWith(root+'/')||root.startsWith(m.Source+'/'))),'An existing container holds the startup trees');}
 }
 for(const [owner,a] of Object.entries(assets.assets)){
  if(owner==='mac-native'){for(const b of [a.nano,a.p2p])assert.equal(await hash(b.path),b.sha256);}
  else{const receipt=await docker(['run','--rm','--pull','never','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','-v',a.buildVolume+':/build:ro','--entrypoint','sha256sum',a.image,a.nano.path,a.p2p.path],owner==='vps-docker');for(const b of [a.nano,a.p2p])assert.ok(receipt.split('\n').includes(b.sha256+'  '+b.path));}
 }
 controllerHashes=await controllerBindings();await save(join(out,'controller-hashes.json'),controllerHashes);
 const initialBytes=await readFile(join(fixture,'initial.json'));assert.equal(createHash('sha256').update(initialBytes).digest('hex'),plan.initialBinding.sha256);
 const initial=JSON.parse(initialBytes.toString('utf8')),bootstrapStateCheckpoint=isolatedBootstrapStateCheckpoint(initial);
 const clockTimes=restoring?names.map(owner=>{assert.equal(captures[owner].publishedGenesisTimeMs,plan.anchor.genesisTimeMs);return BigInt(captures[owner].publishedClockTimeMs);}):[BigInt(plan.anchor.timeMs)];
 const stoppedTime=clockTimes.reduce((a,b)=>a<b?a:b);assert.ok(stoppedTime>=BigInt(plan.anchor.timeMs));
 const downtime=BigInt(Date.now())-stoppedTime;assert.ok(downtime>=0n&&downtime<7200000n,'Actual downtime is outside the qualified finite startup budget');
 await record({event:'preflight-verified',campaign:plan.campaignId,coreRevision:assets.coreRevision,relayRevision:assets.relayRevision,physicalBootstrapPairs:65,keysDerivedByController:false,fullRestore:restoring,actualDowntimeMs:downtime.toString(),startupCatchup:{maximumDowntimeMs:7200000,maximumSteps:60},ordinaryClockRulesUnchanged:true});
 const config=join(work,'config');await mkdir(config,{mode:0o700});
 for(const f of ['genesis.json','profile.json','initial.json']){const bytes=await readFile(join(fixture,f));await writeFile(join(config,f),bytes,{flag:'wx',mode:0o600});}
 const environment={...process.env};for(const k of Object.keys(environment))if(k.startsWith('NANO_TEST_CAPELLA_NETWORK_KEY_')||k==='NANO_TEST_EXIT_BOOTSTRAP')delete environment[k];
 const keyPins=await json(join(work,'private-key-input-pins.json'));
 const keyAudit=await json(join(work,'key-readonly-audit.json'));
 assert.equal(keyAudit.passed,true);assert.equal(keyAudit.inputManifestSha256,pins.fixtureManifestSha256);assert.equal(keyAudit.validatorPrivatePublicBindings,65);assert.equal(keyAudit.runtimeStarted,false);
 const keyCapsule={};
 for(const f of ['env-0.json','env-1.json']){
  const path=join(fixture,f),fd=openSync(path,constants.O_RDONLY|constants.O_NOFOLLOW);
  try{const st=fstatSync(fd);if(!st.isFile()||st.uid!==process.getuid()||(st.mode&0o7777)!==0o600)throw Error('Private fixture environment owner/mode refused');
   const bytes=readFileSync(fd);if(createHash('sha256').update(bytes).digest('hex')!==keyPins[f])throw Error('Private fixture input changed since cryptographic verification');
   const value=JSON.parse(bytes.toString('utf8'));
   for(const [key,secret] of Object.entries(value)){if(key==='NANO_TEST_NETWORK_TRACE'){if(secret!=='1')throw Error('Private fixture trace flag refused');continue;}if(!/^NANO_TEST_CAPELLA_NETWORK_KEY_(?:[0-9]|[1-5][0-9]|6[0-4])$/.test(key)||typeof secret!=='string'||!/^0x[0-9a-f]{64}$/.test(secret)||Object.hasOwn(keyCapsule,key))throw Error('Private fixture environment schema refused');keyCapsule[key]=secret;}
  }finally{closeSync(fd);}
 }
 if(Object.keys(keyCapsule).length!==65)throw Error('Private fixture environment does not cover all isolated keys');
 const envFor=owner=>Object.fromEntries([...plan.owners.find(x=>x.owner===owner).keys.map(x=>[x.runtimeKey.keyEnv,keyCapsule[x.runtimeKey.keyEnv]]),['NANO_TEST_NETWORK_TRACE','1']]);
 const writeEnv=async owner=>{const text=Object.entries(envFor(owner)).map(([k,v])=>k+'='+v).join('\n')+'\n';await writeFile(join(config,owner+'.env'),text,{flag:'wx',mode:0o600});};
 const rp=JSON.parse(await ssh(['python3','-c','import socket,json; a=socket.socket();b=socket.socket();a.bind(("127.0.0.1",0));b.bind(("127.0.0.1",0));print(json.dumps([a.getsockname()[1],b.getsockname()[1]]))']));
 assert.equal(rp.length,2);assert.notEqual(rp[0],rp[1]);
 const lp=await port(),nr=await port(),pf=await port(),rf=await port();assert.equal(new Set([lp,nr,pf,rf]).size,4);
 const configurations=names.map((owner,i)=>{
  const nativeOwner=i===0,a=assets.assets[owner],dataPrefix=nativeOwner?join(work,'data'):'/data',witnessPrefix=nativeOwner?join(work,'witness'):'/witness';
  const data=join(dataPrefix,plan.campaignNamespace,owner),witness=join(witnessPrefix,plan.campaignNamespace,owner);
  return {owner,a,data,witness};
 });
 const template=(await readPinnedJson(join(fixture,'node-0.json'),'5f89362cd3d7119b16f83ded46a5c8f1fd23668268c1745ebf9ac2a6a9136f62')).value;
 assert.equal(await hash(join(config,'genesis.json')),'3373c76de09192ad440625613db97c17b50fbfeddb5adb3190c8f0eb83a22005');
 assert.equal(await hash(join(config,'profile.json')),plan.profileBinding.sha256);
 for(const [i,node] of configurations.entries()){
  const c=structuredClone(template);c.p2p_binary=node.a.p2p.path;c.p2p_identity=join(node.data,'peer.key');c.bootnodes=[];
  c.rpc_host='127.0.0.1';c.rpc_port=i===2?rp[0]:i===0?nr:8545;
  c.listen_addr=i===2?`/ip4/127.0.0.1/tcp/${rp[1]}`:i===0?'/ip4/127.0.0.1/tcp/0':'/ip4/0.0.0.0/tcp/30303';
  c.capella.profile.file=i===0?join(config,'profile.json'):'/config/profile.json';
  c.capella.initial.file=i===0?join(config,'initial.json'):'/config/initial.json';
  c.capella.recoveryWitness=join(node.witness,'chain-witness');
  c.capella.clock.startupCatchup={maximumDowntimeMs:'0x6ddd00',maximumSteps:'0x3c'};
  c.capella.validator.keys=plan.owners[i].keys.map(row=>{const key=structuredClone(row.runtimeKey);key.journal=key.journal.replace('@DATA@',i===0?join(work,'data'):'/data');key.recoveryWitness=key.recoveryWitness.replace('@WITNESS@',i===0?join(work,'witness'):'/witness');return key;});
  if(restoring){
   const captured=captures[node.owner];c.capella.initial=null;c.capella.restart=null;c.capella.recoveryCommitment=structuredClone(captured.restart);
   for(const key of c.capella.validator.keys){const record=captured.validatorKeys.find(x=>x.publicKeyIndex===Number(key.keyEnv.split('_').at(-1)));assert.ok(record);assert.deepEqual(record.restart,record.primary);assert.equal(record.publicKey,plan.owners[i].keys.find(x=>x.runtimeKey.keyEnv===key.keyEnv).publicKey);key.restart=structuredClone(record.restart);}
  }
  node.c=c;
 }
 await save(join(config,'vps-docker.json'),configurations[2].c);await writeEnv('vps-docker');
 await ssh(['mkdir','-m','700',remoteWork+'/config']);
 for(const f of ['genesis.json','profile.json','initial.json','vps-docker.json','vps-docker.env'])await run('/usr/bin/scp',['-4','-o','BatchMode=yes',join(config,f),target+':'+remoteWork+'/config/'+f]);
 const runArgs=(node,isRemote)=>['run','-d','--pull','never','--name',isRemote?remote:local,'--label','nano.test.campaign='+plan.campaignId,'--label','nano.test.owner='+node.owner,...(isRemote?['--network','host']:['-p',`127.0.0.1:${lp}:30303`]),'--user',isRemote?'1000:1000':'501:20','--ulimit','nofile=65536:65536','--memory','4g','--cpus','2','--read-only','--tmpfs','/tmp:rw,noexec,nosuid,size=64m','--cap-drop','ALL','--security-opt','no-new-privileges','-v',node.a.buildVolume+':/build:ro','-v',(isRemote?remoteWork:work)+'/data:/data','-v',(isRemote?remoteWork:work)+'/witness:/witness','-v',(isRemote?remoteWork:work)+'/config:/config:ro','--env-file',(isRemote?remoteWork:work)+'/config/'+node.owner+'.env','--entrypoint',node.a.nano.path,node.a.image,'run','/config/genesis.json',join(node.data,'chain'),'/config/'+node.owner+'.json'];
 await writeFile(join(out,'startup-reserved.json'),JSON.stringify({campaignId:plan.campaignId,timeMs:Date.now(),ownership:plan.owners.map(x=>({owner:x.owner,keys:x.keys.map(k=>k.publicKeyIndex)})),mustNotRerunInitialPins:true})+'\n',{flag:'wx',mode:0o600});
 // Created is set first so an ambiguous Docker result still enters cleanup.
 remoteCreated=true;await docker(runArgs(configurations[2],true),true);
 let peer;const readyUntil=performance.now()+180000;
 while(performance.now()<readyUntil){const state=await inspect(remote,true);assert.equal(state.Running,true,'VPS stopped during startup');const rows=events(await logs(remote,true));if(rows.some(x=>x.event==='altair-ipc-ready'&&x.networkProtocolActive===true)){peer=rows.filter(x=>x.event==='listen').at(-1)?.address;break;}await wait(1000);}
 assert.ok(peer?.startsWith(`/ip4/127.0.0.1/tcp/${rp[1]}/p2p/`),'VPS IPC readiness deadline');
 tunnel=spawnLog(sshBin,['-4','-N','-o','BatchMode=yes','-o','ExitOnForwardFailure=yes','-o','ServerAliveInterval=15','-L',`127.0.0.1:${pf}:127.0.0.1:${rp[1]}`,'-L',`127.0.0.1:${rf}:127.0.0.1:${rp[0]}`,target],'tunnel.log',environment);
 await wait(1000);assert.equal(tunnel.exitCode,null);assert.equal(tunnel.signalCode,null);
 const peerId=peer.split('/p2p/')[1];assert.match(peerId,/^[A-Za-z0-9]+$/);
 configurations[1].c.bootnodes=[`/dns4/host.docker.internal/tcp/${pf}/p2p/${peerId}`];
 await save(join(config,'mac-docker.json'),configurations[1].c);await writeEnv('mac-docker');
 localCreated=true;await docker(runArgs(configurations[1],false));
 configurations[0].c.bootnodes=[`/ip4/127.0.0.1/tcp/${pf}/p2p/${peerId}`];await save(join(config,'mac-native.json'),configurations[0].c);
 native=spawnLog(configurations[0].a.nano.path,['run',join(config,'genesis.json'),join(configurations[0].data,'chain'),join(config,'mac-native.json')],'native.log',{...environment,...envFor('mac-native')});
 await save(join(out,'resources.json'),{nativePid:native.pid,tunnelPid:tunnel.pid,localContainer:local,remoteContainer:remote,work,remoteWork,nativeRpcPort:nr,forwardRpcPort:rf,localPeerPort:lp,forwardPeerPort:pf});
 await record({event:'started',nativePid:native.pid,local,remote,loopbackOnly:true,physicalHosts:2,transport:'cpp-libp2p over SSH forwarding',keysDerivedByController:false});
 await waitRestoredReadiness({sample:async()=>{
  const nativeEvents=events(await readFile(join(out,'native.log'),'utf8'));
  const rows=[{name:'mac-native',running:native.exitCode===null&&native.signalCode===null,networkReady:nativeEvents.some(x=>x.event==='altair-ipc-ready'&&x.networkProtocolActive===true)}];
  for(const [name,onRemote,owner] of [[local,false,'mac-docker'],[remote,true,'vps-docker']]){const state=await inspect(name,onRemote),rowsLog=events(await logs(name,onRemote));rows.push({name:owner,running:state.Running&&!state.OOMKilled,networkReady:rowsLog.some(x=>x.event==='altair-ipc-ready'&&x.networkProtocolActive===true)});}
  assert.equal(tunnel.exitCode,null);assert.equal(tunnel.signalCode,null);return rows;
 },record,clock:()=>performance.now()});
 runtimeRestoreReady=restoring;
 const urls=[`http://127.0.0.1:${nr}`,null,`http://127.0.0.1:${rf}`];
 const request=async(n,path,body)=>{
  if(n===1){const args=['exec',local,'curl','-sS','--max-time','10','-H','Accept: application/json','--write-out','\n%{http_code}'];if(body)args.push('-H','Content-Type: application/json','--data',JSON.stringify(body));args.push('http://127.0.0.1:8545'+path);const text=await docker(args),cut=text.lastIndexOf('\n'),status=Number(text.slice(cut+1));if(status!==200)throw beaconObservationError(status,names[n],path,text.slice(0,cut));return JSON.parse(text.slice(0,cut));}
  const response=await fetch(urls[n]+path,{method:body?'POST':'GET',headers:{Accept:'application/json',...(body?{'Content-Type':'application/json'}:{})},...(body?{body:JSON.stringify(body)}:{}),signal:AbortSignal.timeout(10000)});if(!response.ok)throw beaconObservationError(response.status,names[n],path,await response.text());return response.json();
 };
 const rpc=async(n,method)=>{const x=await request(n,'/',{jsonrpc:'2.0',id:1,method,params:[]});assert.ok(!x.error,'RPC returned an error');return x.result;};
 baseline=restoring?{epoch:BigInt(captures['mac-native'].finalized.epoch).toString(),root:captures['mac-native'].finalized.root}:{epoch:BigInt(plan.anchor.finalized[0]).toString(),root:plan.anchor.finalized[1]};
 if(restoring)for(const owner of names)assert.deepEqual({epoch:BigInt(captures[owner].finalized.epoch).toString(),root:captures[owner].finalized.root},baseline,'Stopped sources must share the restoration baseline');
 observer=isolatedRuntimeObservation({baseline,...(baseline.epoch==='0'?{bootstrapStateCheckpoint}:{}),requiredAdvances:2,requiredStableSamples:3});
 const deadline=performance.now()+900000;
 while(performance.now()<deadline){
  if(failure)throw failure;assert.equal(native.exitCode,null,'Native exited');assert.equal(native.signalCode,null);assert.equal(tunnel.exitCode,null);assert.equal(tunnel.signalCode,null);
  for(const [name,onRemote] of [[local,false],[remote,true]])assert.equal((await inspect(name,onRemote)).Running,true,name+' exited');
  const rows=[];
  for(let n=0;n<3;n++)try{const row=await observer.sample({name:names[n],read:path=>request(n,path),peers:await rpc(n,'net_peerCount')});if(row)rows.push(row);else await record({event:'finality-read-pending',owner:names[n]});}catch(e){if(e.status)await record({event:'http-read-refusal',owner:names[n],path:e.path,status:e.status,responseBody:e.responseBody});if(isolatedPendingObservation(e)){await record({event:'observation-pending',owner:names[n],reason:'known Beacon read budget refusal'});continue;}throw e;}
  // Log separate serialized objects; the observer consumes original receipts.
  await record({event:'observation',rows});common=observer.observe(rows);await save(join(out,'distinct-finality-progress.json'),observer.snapshot());if(common)break;await wait(3000);
 }
 assert.ok(common,'Common finality did not advance through two distinct checkpoints before deadline');result='PASSED';
}catch(e){failure=e;process.exitCode=1;await record({event:'failed',message:e.message});}
finally{
 let allWritersStopped=true;
 try{await stop(native);}catch(e){allWritersStopped=false;result='FAILED';failure??=e;process.exitCode=1;await record({event:'cleanup-error',owner:'mac-native',message:e.message});}
 for(const [name,onRemote,created,file] of [[local,false,localCreated,'docker-mac.log'],[remote,true,remoteCreated,'vps.log']])if(created)try{if((await inspect(name,onRemote)).Running)await docker(['stop','--time','20',name],onRemote);const stopped=await inspect(name,onRemote);await save(join(out,file+'.state.json'),stopped);assert.equal(stopped.Running,false,'Container writer is still running after stop');await writeFile(join(out,file),await logs(name,onRemote),{mode:0o600});}catch(e){allWritersStopped=false;result='FAILED';failure??=e;process.exitCode=1;await record({event:'cleanup-error',owner:onRemote?'vps-docker':'mac-docker',message:e.message});}
 try{await stop(tunnel);if(tunnel?.pid)assert.ok(tunnel.exitCode!==null||tunnel.signalCode!==null,'SSH tunnel still running');}catch(e){allWritersStopped=false;result='FAILED';failure??=e;process.exitCode=1;await record({event:'cleanup-error',owner:'ssh-tunnel',message:e.message});}if(native)await save(join(out,'native-state.json'),{pid:native.pid,exitCode:native.exitCode,signalCode:native.signalCode});
 const receipts=[];
 for(const [n,file] of ['native.log','docker-mac.log','vps.log'].entries())try{const rows=events(await readFile(join(out,file),'utf8')),receipt=rows.filter(x=>x.event==='capella-runtime-stopped').at(-1);if(receipt){await save(join(out,names[n]+'-external-stop-receipt.json'),receipt);receipts.push(receipt);}if(result==='PASSED'){assert.ok(receipt?.clockHealthy&&receipt.validator?.healthy,'Owner stop is unhealthy');assert.ok(BigInt(receipt.finalized.epoch)>=BigInt(common.epoch),'Stopped finality behind observation');assert.equal(receipt.validator.localKeys,[16,16,33][n]);assert.ok(receipt.validator.proposalsAdmitted>0&&receipt.validator.attestationsAdmitted>0,'Each owner must actually participate');}}catch(e){result='FAILED';failure??=e;process.exitCode=1;}
 if(result==='PASSED')try{assert.equal(native.exitCode,0);assert.equal(native.signalCode,null);for(const [name,onRemote] of [[local,false],[remote,true]]){const state=await inspect(name,onRemote);assert.equal(state.Status,'exited');assert.equal(state.ExitCode,0);assert.equal(state.OOMKilled,false);}assert.deepEqual(controllerHashes,await controllerBindings());}catch(e){result='FAILED';failure??=e;process.exitCode=1;}
 await save(join(out,'result.json'),{format:'nano-isolated-three-node-baseline-v1',result,commonFinality:common??null,baseline:baseline??null,stopReceipts:receipts.length,failure:failure?.message??null,physicalBootstrapJournalTransportQualified:physicalAuditsPassed,allWritersStopped,localContainer:localCreated?local:null,remoteContainer:remoteCreated?remote:null,fullRuntimeRestoreAttempted:restoring,fullRuntimeRestoreReady:runtimeRestoreReady,networkScope:'private bootstrap 65-key/64-active three-node/two-host finality baseline only',productionQualified:false});
 await record({event:'terminal',result,commonFinality:common??null});
}
