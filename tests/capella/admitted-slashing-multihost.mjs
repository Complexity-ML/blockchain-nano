#!/usr/bin/env node
// Private admitted-member sanction / witnessed capture / controlled crash recovery probe. Software arrives on the VPS through Git.
import {execFile,spawn} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdir,readFile,writeFile,appendFile,cp} from 'node:fs/promises';
import {openSync,closeSync,writeSync,fsyncSync} from 'node:fs';
import {resolve,join} from 'node:path';
import {createHash,randomBytes} from 'node:crypto';
import {createServer} from 'node:net';
import {setTimeout as wait} from 'node:timers/promises';
import {selectExitArtifacts,verifyExitExecutableHashes,verifyExitVpsArtifacts} from './exit-artifacts.mjs';
import {monotonicClockConfiguration} from '../multihost/local-clock.mjs';
import {hashHarness} from '../multihost/harness-manifest.mjs';
import {durableFinalityObservation} from './finality-observation.mjs';
import {pinnedDepositRestart,pinnedQuiescentRestart} from './deposit-restart-pins.mjs';
import {writerObservationProgram,verifyWriterSuspension} from './writer-suspension.mjs';
const exec=promisify(execFile),[target,revision,outArg]=process.argv.slice(2);
if(!/^[a-z_][a-z0-9_-]*@[0-9.]+$/.test(target??'')||!/^([a-f0-9]{40})$/.test(revision??'')||!outArg)throw Error('Usage: admitted-slashing-multihost.mjs user@IPv4 harness-revision NEW-output');
const repo=process.cwd(),out=resolve(outArg),fixture=join(out,'fixture'),id=randomBytes(6).toString('hex');
const remote='nano-admitted-slashing-wan-'+id,remoteRoot='/tmp/'+remote;let volume;
const remoteRepo=`/home/${target.split('@')[0]}/blockchain-nano-git`;
const quote=s=>"'"+String(s).replaceAll("'","'\\''")+"'";
const run=async(bin,args,timeout=30000)=>(await exec(bin,args,{timeout,maxBuffer:16*1024*1024})).stdout;
const ssh=args=>run('ssh',['-o','BatchMode=yes','-o','ConnectTimeout=8',target,args.map(quote).join(' ')]);
const docker=(args,timeout=30000)=>run('ssh',['-o','BatchMode=yes','-o','ConnectTimeout=8',target,['sudo','-n','docker',...args].map(quote).join(' ')],timeout);
const json=async p=>JSON.parse(await readFile(p,'utf8'));
const save=(p,d)=>writeFile(p,JSON.stringify(d,null,2)+'\n');
const check=(ok,message)=>{if(!ok)throw Error(message);};
const hash=async p=>createHash('sha256').update(await readFile(p)).digest('hex');
const parseEvents=s=>s.split('\n').flatMap(l=>{try{return [JSON.parse(l)];}catch{return [];}});
const remoteLogs=async()=>{const r=await exec('ssh',['-o','BatchMode=yes',target,['sudo','-n','docker','logs',remote].map(quote).join(' ')+' 2>&1'],{timeout:30000,maxBuffer:16*1024*1024});return r.stdout;};
const inspect=async()=>JSON.parse(await docker(['inspect',remote,'--format','{{json .}}']));
const freePort=()=>new Promise((ok,no)=>{const s=createServer();s.on('error',no);s.listen(0,'127.0.0.1',()=>{const p=s.address().port;s.close(()=>ok(p));});});
await mkdir(out);
const record=async d=>{const row={time:new Date().toISOString(),...d};console.log(JSON.stringify(row));await appendFile(join(out,'events.jsonl'),JSON.stringify(row)+'\n');};
let checkpoint,restartedAt,restartCutoffSlot,blocks;
let artifacts,resume,tools,proof,m,r,data,harness,native,tunnel,suspended=false,created=false,result='FAILED',failure,cleanupFailure=false;
const stopChild=async child=>{if(!child||child.exitCode!==null||child.signalCode!==null)return;child.kill('SIGTERM');for(let i=0;i<100&&child.exitCode===null&&child.signalCode===null;i++)await wait(100);if(child.exitCode===null&&child.signalCode===null){child.kill('SIGKILL');for(let i=0;i<50&&child.exitCode===null&&child.signalCode===null;i++)await wait(100);}};
try{
 check((await run('git',['rev-parse','HEAD'])).trim()===revision,'Local revision mismatch');
 check(process.env.NANO_TEST_EXIT_ARTIFACT_MANIFEST,'Explicit NANO_TEST_EXIT_ARTIFACT_MANIFEST required');
 artifacts=selectExitArtifacts(await json(resolve(process.env.NANO_TEST_EXIT_ARTIFACT_MANIFEST)));
 harness=await hashHarness(repo,['tests/capella/admitted-slashing-multihost.mjs']);
 await save(join(out,'artifacts.json'),artifacts);await save(join(out,'harness.json'),harness);
 await run('git',['diff','--quiet',artifacts.runtimeRevision,'--','src','include','network','third_party']);
 check((await ssh(['git','-C',remoteRepo,'rev-parse','HEAD'])).trim()===revision,'VPS revision mismatch');
 check(!(await ssh(['git','-C',remoteRepo,'status','--porcelain'])).trim(),'VPS source is dirty');
 await ssh(['git','-C',remoteRepo,'diff','--quiet',artifacts.runtimeRevision,'--','src','include','network','third_party']);
 const {vps}=artifacts,{image}=vps,nativeBin=artifacts.native.nano.path,p2p=artifacts.native.p2p.path,exporter=artifacts.native.exporter.path;
 const nativeHashes=Object.fromEntries(await Promise.all(Object.values(artifacts.native).map(async e=>[e.path,await hash(e.path)])));
 verifyExitExecutableHashes(artifacts.native,Object.entries(nativeHashes).map(([p,h])=>`${h}  ${p}`).join('\n'));
 const inventory=['run','--rm','--network','none','--read-only','--mount',artifacts.mount,'--entrypoint','/bin/sh',image,'-c'];
 const imageInfo=JSON.parse(await docker(['image','inspect',image,'--format','{{json .}}']));
 const remoteRevision=await docker([...inventory,'cat '+quote(artifacts.revisionFile)]);
 const remoteHashes=await docker([...inventory,'sha256sum '+Object.values(vps.executables).map(e=>quote(e.path)).join(' ')]);
 verifyExitVpsArtifacts(artifacts,imageInfo,remoteRevision,remoteHashes);
 await record({event:'provenance',revision,runtimeRevision:artifacts.runtimeRevision,transport:'real cpp-libp2p TCP over private SSH forwarding',nativeHashes,harness,image:vps.image,buildVolume:vps.volume,remoteHashes,publicDeployment:false});
 check(process.env.NANO_TEST_DEPOSIT_RESUME_MANIFEST,'Pinned stopped-source manifest required');
 resume=await json(resolve(process.env.NANO_TEST_DEPOSIT_RESUME_MANIFEST));
 check(resume.runtimeRevision===artifacts.runtimeRevision&&/^nano-deposit-resume-wan-[a-f0-9]{12}-data$/.test(resume.volume)&&/^nano-deposit-resume-wan-[a-f0-9]{12}$/.test(resume.originalContainer),'Private source identity mismatch');
 check((await json(join(resume.sourceOutput,'result.json'))).result==='PASSED','Qualified cold source record required');
 const old=JSON.parse(await docker(['inspect',resume.originalContainer,'--format','{{json .}}']));check(!old.State.Running&&old.Mounts.some(x=>x.Name===resume.volume&&x.Destination==='/var/lib/nano'),'Original container still live or wrong volume');
 for(let n=0;n<2;n++){const sourceAudit=await json(resume.canonicalAudits[n]);const audit=sourceAudit.canonicalReplay;check(sourceAudit.perKeyJournalWitnessAuditQualified&&audit.canonicalHistoryVerified&&audit.externalFinalizedAncestorVerified&&BigInt(audit.finalized.epoch)===BigInt(resume.baseline.epoch)&&audit.finalized.root===resume.baseline.root,'Source canonical audit/finality mismatch');}
 volume=remote+'-data';await cp(join(resume.sourceOutput,'fixture'),fixture,{recursive:true,errorOnExist:true,force:false});
 await docker(['run','--rm','--network','none','--read-only','--mount',`type=volume,src=${resume.volume},dst=/source,readonly`,'--mount',`type=volume,src=${volume},dst=/restored`,'--entrypoint','/bin/sh',image,'-c','cp -a /source/. /restored/']);
 await cp(join(resume.sourceOutput,'mac-data'),join(out,'mac-data'),{recursive:true,errorOnExist:true,force:false});
 await record({event:'cold-stopped-data-copied',originalVolume:resume.volume,restoredVolume:volume,originalDatabasesPreserved:true,originalSignersRemainStopped:true});
 await save(join(out,'resume-source.json'),resume);
 tools=await json(resolve(process.env.NANO_TEST_SLASHING_TOOLS??''));
 check(tools.runtimeRevision===artifacts.runtimeRevision,'Tool runtime revision mismatch');
 for(const side of ['native','vps'])for(const name of ['helper','audit','capture']){const e=tools[side]?.[name];check(e&&/^\/[A-Za-z0-9_./-]+$/.test(e.path)&&!e.path.split('/').includes('..')&&/^[a-f0-9]{64}$/.test(e.sha256),'Pinned tool required');if(side==='native')check(await hash(e.path)===e.sha256,'Native tool hash mismatch');}
 const toolHash=await docker(['run','--rm','--network','none','--read-only','--mount',artifacts.mount,'--entrypoint','sha256sum',image,...Object.values(tools.vps).map(e=>e.path)]);
 for(const e of Object.values(tools.vps))check(toolHash.split('\n').includes(e.sha256+'  '+e.path),'VPS tool hash mismatch');
 await save(join(out,'tools.json'),tools);
 proof=await json(resume.proof);check(proof.offender===64&&proof.syntheticValidators===65&&proof.productionQualified===false,'Private admitted proof required');
 await save(join(fixture,'proof.json'),proof);
 const ports=JSON.parse(await ssh(['python3','-c','import socket,json; a=socket.socket(); b=socket.socket(); a.bind(("127.0.0.1",0)); b.bind(("127.0.0.1",0)); print(json.dumps([a.getsockname()[1],b.getsockname()[1]]))']));
 check(ports.length===2&&ports.every(p=>Number.isInteger(p)&&p>1024&&p<65536)&&ports[0]!==ports[1],'Invalid reserved VPS ports');

 const meta=await json(join(fixture,'fixture.json'));check(meta.chainId===1337&&meta.syntheticValidators===64&&meta.nodes===2,'Unexpected synthetic fixture');
 meta.initialHead=resume.baseline.root;check(/^0x[a-f0-9]{64}$/.test(meta.initialHead),'Fixture anchor missing');
 let configs=await Promise.all([0,1].map(i=>json(join(fixture,`node-${i}.json`)))),envs=await Promise.all([0,1].map(i=>json(join(fixture,`env-${i}.json`))));
 const profile=await json(join(fixture,'profile.json'));
 configs=configs.map(c=>monotonicClockConfiguration(c,process.env.NANO_TEST_WALL_CLOCK_DRIFT_MS,profile));
 for(let n=0;n<2;n++){
  check(configs[n].capella.validator.keys.length===(n===0?32:33),'Invalid key partition');
  for(let j=0;j<32;j++){const k=configs[n].capella.validator.keys[j],i=32*n+j;check(BigInt(k.index)===BigInt(i)&&k.keyEnv===`NANO_TEST_CAPELLA_NETWORK_KEY_${i}`&&BigInt(envs[n][k.keyEnv])===BigInt(i+1),'Non-public or misplaced key');}
  const receipt=await json(resume.stopReceipts[n]);configs[n]=pinnedDepositRestart(configs[n],receipt,resume.pendingPublicKey);
  // Administrative replay budget for downtime; consensus profile is unchanged.
  configs[n].capella.limits.replaySlots='0x400';
  configs[n].capella.clock.maximumForwardStepMs='0x'+(BigInt(meta.secondsPerSlot)*1000n*1024n).toString(16);
 }
 const deposit=await json(join(fixture,'deposit.json'));
 check(deposit.validatorIndex===64&&deposit.amountGwei===32000000000&&deposit.productionQualified===false,'Unexpected deposit fixture');
 const pending=configs[1].capella.validator.keys[32];check(pending.index===null&&pending.keyEnv==='NANO_TEST_CAPELLA_NETWORK_KEY_64'&&BigInt(envs[1][pending.keyEnv])===65n,'Pending public key missing');
 envs[1].NANO_TEST_NETWORK_TRACE='1';
 r=configs[1];r.rpc_port=ports[0];r.listen_addr=`/ip4/127.0.0.1/tcp/${ports[1]}`;r.p2p_binary=tools.vps.helper.path;envs[1].NANO_TEST_SLASHING_FILE='/config/proof.json';envs[1].NANO_TEST_SLASHING_DAEMON=vps.executables.p2p.path;
 await save(join(fixture,'node-1.json'),r);await writeFile(join(fixture,'remote.env'),Object.entries(envs[1]).map(([k,v])=>`${k}=${v}`).join('\n')+'\n',{mode:0o600});
 await ssh(['mkdir',remoteRoot]);await run('scp',['-q','-r',fixture,`${target}:${remoteRoot}/config`]);
 await docker(['run','-d','--name',remote,'--label','nano.qualification=capella-deposit-wan','--label','org.opencontainers.image.revision='+revision,'--network','host','--ulimit','nofile=65536:65536','--memory','4g','--cpus','2','--read-only','--tmpfs','/tmp','--cap-drop','ALL','--security-opt','no-new-privileges','--mount',`type=bind,src=${remoteRoot}/config,dst=/config,readonly`,'--mount',artifacts.mount,'--mount',`type=volume,src=${volume},dst=/var/lib/nano`,'--env-file',remoteRoot+'/config/remote.env','--entrypoint',vps.executables.nano.path,image,'run','/config/genesis.json','/var/lib/nano/chain','/config/node-1.json']);created=true;
 async function readyRemote(starts){const deadline=Date.now()+90000;while(Date.now()<deadline){const state=await inspect();check(state.State.Running&&!state.State.OOMKilled,'VPS node exited');const events=parseEvents(await remoteLogs());if(events.filter(e=>e.event==='altair-ipc-ready'&&e.networkProtocolActive===true).length>=starts)return events.filter(e=>e.event==='listen').at(-1).address;await wait(500);}throw Error('VPS startup deadline');}
 const address=await readyRemote(1);check(address.startsWith(`/ip4/127.0.0.1/tcp/${ports[1]}/p2p/`),'VPS listener mismatch');
 await record({event:'vps-binaries',sha256:verifyExitExecutableHashes(vps.executables,await docker(['exec',remote,'sha256sum',...Object.values(vps.executables).map(e=>e.path)]))});
 const rpcForward=await freePort();let p2pForward=await freePort();while(p2pForward===rpcForward)p2pForward=await freePort();
 const tf=openSync(join(out,'tunnel.log'),'a');tunnel=spawn('ssh',['-N','-o','BatchMode=yes','-o','ExitOnForwardFailure=yes','-o','ServerAliveInterval=15','-L',`127.0.0.1:${rpcForward}:127.0.0.1:${ports[0]}`,'-L',`127.0.0.1:${p2pForward}:127.0.0.1:${ports[1]}`,target],{stdio:['ignore',tf,tf]});closeSync(tf);tunnel.on('error',e=>{failure=e;});
 m=configs[0];data=join(out,'mac-data');
 m.rpc_port=await freePort();m.listen_addr='/ip4/127.0.0.1/tcp/0';m.p2p_binary=p2p;m.p2p_identity=join(data,'peer.key');m.bootnodes=[address.replace(`/tcp/${ports[1]}/`,`/tcp/${p2pForward}/`)];m.capella.profile.file=join(fixture,'profile.json');if(m.capella.initial)m.capella.initial.file=join(fixture,'initial.json');m.capella.recoveryWitness=join(data,'witness');
 for(const k of m.capella.validator.keys){k.journal=join(data,'key-'+BigInt(k.index));k.recoveryWitness=k.journal+'-witness';}
 await save(join(out,'mac.json'),m);
 const urls=[`http://127.0.0.1:${m.rpc_port}`,`http://127.0.0.1:${rpcForward}`];
 async function request(n,path,body){
  for(let attempt=0;attempt<3;attempt++){
   const res=await fetch(urls[n]+path,{method:body?'POST':'GET',headers:{Accept:'application/json',...(body?{'Content-Type':'application/json'}:{})},...(body?{body:JSON.stringify(body)}:{}),signal:AbortSignal.timeout(10000)});
   if(res.ok)return res.json();
   const raw=(await res.text()).slice(0,4096);let responseBody;try{responseBody=JSON.parse(raw);}catch{responseBody={raw};}
   await record({event:'http-read-refusal',node:n,path,httpStatus:res.status,attempt,responseBody});
   const retryable=!body&&path.startsWith('/eth/')&&res.status===503&&['Beacon read capacity exceeded','Beacon read cancelled or execution budget expired','Beacon read queue expired'].includes(responseBody.message);
   if(retryable&&attempt<2){await wait(1000);continue;}
   const error=Error(`HTTP ${res.status} node ${n} ${path}: ${raw}`);error.httpStatus=res.status;error.node=n;error.responseBody=responseBody;throw error;
  }
 }
 const rpc=async(n,method,params=[])=>{const d=await request(n,'/',{jsonrpc:'2.0',id:1,method,params});check(!d.error,`RPC ${method}: ${JSON.stringify(d.error)}`);return d.result;};
 let connected=false;for(let i=0;i<30;i++){try{await rpc(1,'eth_chainId');connected=true;break;}catch{check(tunnel.exitCode===null&&tunnel.signalCode===null,'SSH tunnel exited');await wait(500);}}check(connected,'SSH forward unavailable');
 const clean={...process.env};for(const k of Object.keys(clean))if(k.startsWith('NANO_TEST_CAPELLA_NETWORK_KEY_')||k==='NANO_TEST_EXIT_BOOTSTRAP')delete clean[k];
 const fd=openSync(join(out,'native.log'),'a');native=spawn(nativeBin,['run',join(fixture,'genesis.json'),join(data,'chain'),join(out,'mac.json')],{env:{...clean,...envs[0]},stdio:['ignore',fd,fd]});closeSync(fd);native.on('error',e=>{failure=e;});
 await record({event:'started',nativePid:native.pid,remote,volume,physicalHosts:2,initialValidatorsPerHost:32,pendingValidatorOnVps:64,submissionHost:'mac-native',loopbackOnly:true});
 await save(join(out,'resources.json'),{target,revision,remote,volume,remoteRoot,urls,data,nativePid:native.pid,tunnelPid:tunnel.pid});
 let localReady=false;for(let i=0;i<60;i++){try{await rpc(0,'eth_chainId');localReady=true;break;}catch{check(native.exitCode===null&&native.signalCode===null,'Native node exited');await wait(500);}}check(localReady,'Native RPC startup deadline');
 let headersReady=false;const headerDeadline=Date.now()+90000;while(Date.now()<headerDeadline){try{for(let n=0;n<2;n++)await request(n,'/eth/v1/beacon/headers/head');headersReady=true;break;}catch(error){if(error.httpStatus!==404)throw error;}check(native.exitCode===null&&native.signalCode===null,'Native exited before signed head');await wait(1000);}check(headersReady,'Signed heads startup deadline');
 await record({event:'pinned-resume-no-new-deposit',sourceOutput:resume.sourceOutput,baseline:resume.baseline,automaticRecovery:false,replaySlots:'0x400',maximumForwardStepMs:configs[0].capella.clock.maximumForwardStepMs});
 blocks=[new Map(),new Map()];const epochs=[BigInt(resume.baseline.epoch),BigInt(resume.baseline.epoch)],roots=new Map([[String(BigInt(resume.baseline.epoch)),resume.baseline.root]]);let stable=0,preCrash;const matches=op=>[op.attestation_1,op.attestation_2].every(a=>a.attesting_indices.length===1&&BigInt(a.attesting_indices[0])===64n);
 async function observe(n,root){for(let depth=0;root!==meta.initialHead&&!blocks[n].has(root);depth++){
  check(depth<1024,'Beacon ancestry budget');const d=await request(n,'/eth/v2/beacon/blocks/'+root),msg=d.data.message;
  check(d.version==='capella'&&!d.execution_optimistic,'Unqualified Beacon block');if(blocks[1-n].has(root))check(JSON.stringify(blocks[1-n].get(root))===JSON.stringify(msg),'Different authenticated block bodies');blocks[n].set(root,msg);root=msg.parent_root;
 }}
 function canonicalEvidence(root,n){let count=0;for(let depth=0;root!==meta.initialHead;depth++){check(depth<1024,'Canonical ancestry budget');const b=blocks[n].get(root);check(b,'Missing canonical block');count+=b.body.attester_slashings.filter(matches).length;root=b.parent_root;}return count;}
 const observer=async()=>JSON.parse(await docker(['exec',remote,'python3','-c',writerObservationProgram]));
 const durableSave=(p,d)=>{const fd=openSync(p,'wx',0o600);try{writeSync(fd,JSON.stringify(d,null,2)+'\n');fsyncSync(fd);}finally{closeSync(fd);}const dir=openSync(out,'r');try{fsyncSync(dir);}finally{closeSync(dir);}};
 async function captureBeforeCrash(finalized){
  // A publication in progress is refused. Resume and retry a bounded number of
  // times; every refusal is retained. Never remove the commitment guards.
  const original=await inspect(),expected={containerId:original.Id,executable:vps.executables.nano.path,volume};
  check((await docker(['ps','--filter','volume='+volume,'--format','{{.ID}}'])).trim()===original.Id.slice(0,12),'Uncontrolled volume writer');
  for(let attempt=0;attempt<3;attempt++){
   await docker(['kill','--signal','STOP',remote]);suspended=true;
   try{
    const samples=[await observer(),await observer()],observation=verifyWriterSuspension(original,await inspect(),samples,expected);
    const sourceRequest=await json(resume.auditRequests[1]);const terminal=await json(resume.stopReceipts[1]);
    const req={...sourceRequest,profile:{file:'/config/profile.json',sha256:r.capella.profile.sha256},anchor:{file:'/config/initial.json',sha256:sourceRequest.anchor.sha256},limits:r.capella.limits,requiredFinalized:finalized,minimumCheckpoint:terminal.restart,minimumValidatorKeys:terminal.validatorKeys,minimumAttestationTarget:'0x4c'};
    await save(join(out,'capture-request-'+attempt+'.json'),req);await run('scp',[join(out,'capture-request-'+attempt+'.json'),target+':'+remoteRoot+'/capture-request.json']);
    let raw;
    try{raw=await docker(['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--mount',artifacts.mount,'--mount',`type=volume,src=${volume},dst=/var/lib/nano,readonly`,'--mount',`type=bind,src=${remoteRoot}/config,dst=/config,readonly`,'--mount',`type=bind,src=${remoteRoot}/capture-request.json,dst=/request.json,readonly`,'--entrypoint',tools.vps.capture.path,image,'--capture','/config/genesis.json','/config/node-1.json','/var/lib/nano/chain','/request.json'],120000);}
    catch(error){await writeFile(join(out,'capture-refusal-'+attempt+'.log'),String(error.stderr??error.message));throw error;}
    const capsule=JSON.parse(raw);verifyWriterSuspension(original,await inspect(),[samples[0],await observer()],expected);
    const updated=pinnedQuiescentRestart(r,capsule,resume.pendingPublicKey,{writersSuspended:observation.writersSuspended,genesisHash:proof.genesisHash,finalized});
    durableSave(join(out,'pre-crash-capsule.json'),capsule);durableSave(join(out,'pre-crash-suspension.json'),observation);
    await record({event:'fresh-external-pins-durably-saved-before-kill',attempt,checkpoint:finalized,core:capsule.restart,localKeys:capsule.validatorKeys.length,capsuleSha256:await hash(join(out,'pre-crash-capsule.json')),writersSuspended:true});
    r=updated;await save(join(fixture,'node-1.json'),r);await run('scp',[join(fixture,'node-1.json'),target+':'+remoteRoot+'/config/node-1.json']);
    verifyWriterSuspension(original,await inspect(),[samples[0],await observer()],expected);
    await docker(['kill','--signal','KILL',remote]);suspended=false;const stopped=await inspect();check(stopped.Id===original.Id&&!stopped.State.Running&&stopped.State.ExitCode===137,'Controlled SIGKILL missing');
    await save(join(out,'crash-state.json'),stopped);await docker(['start',remote]);check(await readyRemote(2)===address,'Restart identity changed');check((await inspect()).Id===original.Id,'Different restarted container');
    restartedAt=Date.now();await record({event:'restarted-with-fresh-external-pins',sameContainer:true,sameVolume:true,automaticRecovery:false,preCrash:finalized});return;
   }catch(error){await record({event:'capture-attempt-refused',attempt,message:error.message});if(!suspended)throw error;await docker(['kill','--signal','CONT',remote]);suspended=false;if(attempt===2)throw error;await wait(1000);}
  }
 }
 const deadline=Date.now()+900000;
 while(Date.now()<deadline){
  check(!failure&&native.exitCode===null&&native.signalCode===null&&tunnel.exitCode===null&&tunnel.signalCode===null,'Campaign process stopped');check((await inspect()).State.Running,'VPS stopped');
  const rows=[];for(let n=0;n<2;n++){
   const stateCheckpoint=(await request(n,'/eth/v1/beacon/states/head/finality_checkpoints')).data.finalized,h=(await request(n,'/eth/v1/beacon/headers/head')).data;
   let cp=stateCheckpoint;
   if(BigInt(stateCheckpoint.epoch)>0n||epochs[n]>0n){
    const first=await request(n,'/eth/v1/beacon/headers/finalized');
    const observation=durableFinalityObservation(stateCheckpoint,first,roots,epochs[n]);
    const second=await request(n,'/eth/v1/beacon/headers/finalized');
    if(first.data.root!==second.data.root||!observation){await record({event:'finality-read-pending',node:n,stateCheckpoint,firstRoot:first.data.root,secondRoot:second.data.root});continue;}
    cp=observation.checkpoint;
    if(cp.root!==stateCheckpoint.root)await record({event:'head-state-checkpoint-lags-durable-finality',node:n,...observation});
   }

   const epoch=BigInt(cp.epoch);check(epoch>=epochs[n],'Finality regressed');check(!roots.has(cp.epoch)||roots.get(cp.epoch)===cp.root,'Conflicting finalized roots');epochs[n]=epoch;roots.set(cp.epoch,cp.root);
   await observe(n,h.root);if(epoch>0n)await observe(n,cp.root);else check(cp.root==='0x'+'0'.repeat(64)||cp.root===meta.initialHead,'Unexpected epoch-zero finality anchor');rows.push({node:n,checkpoint:cp,slot:h.header.message.slot,peers:await rpc(n,'net_peerCount')});
  }
  if(rows.length!==2){await wait(3000);continue;}
  await record({event:'observation',rows});const a=rows[0].checkpoint,b=rows[1].checkpoint;
  if(a.epoch===b.epoch&&a.root===b.root&&rows.every(r=>BigInt(r.peers)>0n)){
   const count=canonicalEvidence(a.root,0);check(count===canonicalEvidence(a.root,1),'Canonical proof disagreement');check(count<=1,'Duplicate canonical proof inclusion');
   if(count===1){
    if(!preCrash){preCrash=a;await captureBeforeCrash(a);continue;}
    if(BigInt(a.epoch)>=BigInt(preCrash.epoch)+2n&&++stable>=3){
     const boot=parseEvents(await remoteLogs()).filter(e=>e.event==='private-slashing-bootstrap-passed');check(boot.length===2&&boot.every(e=>e.offender===64&&e.forgedRefusals===4&&e.validAndReverseReplayAccepted),'Both forged/replay admission receipts required');
     checkpoint=a;await save(join(out,'common-finality.json'),{baseline:resume.baseline,preCrash,afterRestart:a,proofInclusionsPerNode:[count,count],elapsedMs:Date.now()-restartedAt});await record({event:'common-finality-advanced-after-admitted-sanction-and-pinned-recovery',checkpoint});break;
    }
   }
  }else stable=0;
  await wait(3000);
 }
 check(checkpoint,'Admitted sanction/pinned recovery/common-finality deadline exceeded');result='PASSED';

}catch(error){failure=error;process.exitCode=1;await record({event:'failed',message:error.stack});}
finally{
 await stopChild(native);
 if(created){try{if(suspended){await docker(['kill','--signal','CONT',remote]);suspended=false;}const state=await inspect();if(state.State.Running)await docker(['stop','--time','20',remote]);await save(join(out,'container-state.json'),await inspect());await writeFile(join(out,'vps.log'),await remoteLogs());}catch(error){cleanupFailure=true;await record({event:'cleanup-error',message:error.message});}}
 await stopChild(tunnel);
 if(result==='PASSED'){
  try{
   const terminalNative=await Promise.all(Object.values(artifacts.native).map(async e=>`${await hash(e.path)}  ${e.path}`));
   verifyExitExecutableHashes(artifacts.native,terminalNative.join('\n'));
   verifyExitVpsArtifacts(artifacts,JSON.parse(await docker(['image','inspect',artifacts.vps.image,'--format','{{json .}}'])),await docker(['run','--rm','--network','none','--read-only','--mount',artifacts.mount,'--entrypoint','cat',artifacts.vps.image,artifacts.revisionFile]),await docker(['run','--rm','--network','none','--read-only','--mount',artifacts.mount,'--entrypoint','sha256sum',artifacts.vps.image,...Object.values(artifacts.vps.executables).map(e=>e.path)]));
   check(JSON.stringify(await hashHarness(repo,['tests/capella/admitted-slashing-multihost.mjs']))===JSON.stringify(harness),'Deposit harness changed during campaign');
   for(const name of ['native.log','vps.log']){const stops=parseEvents(await readFile(join(out,name),'utf8')).filter(e=>e.event==='capella-runtime-stopped');const s=stops.at(-1);check(s?.clockHealthy&&s.validator?.healthy&&s.validator.localKeys===(name==='native.log'?32:33)&&s.validator.proposalsAdmitted>0&&s.validator.attestationsAdmitted>0,`${name}: validators not healthy or no distributed participation`);}check(!cleanupFailure,'Cleanup failure');
   await save(join(out,'canonical-blocks.json'),[...blocks[0]].map(([root,message])=>({root,message})));
   const stop=parseEvents(await readFile(join(out,'vps.log'),'utf8')).filter(e=>e.event==='capella-runtime-stopped').at(-1);
   check(stop.validator.localKeys===33&&stop.validator.awaitingRegistration===0,'Pending key not resolved at stop');
   await record({event:'network-probe-passed-offline-audit-still-required',checkpoint});
   for(const [n,name] of ['native.log','vps.log'].entries()){
    const terminal=parseEvents(await readFile(join(out,name),'utf8')).filter(e=>e.event==='capella-runtime-stopped').at(-1),config=n===0?m:r,sourceRequest=await json(resume.auditRequests[n]);
    const req={profile:{file:config.capella.profile.file,sha256:config.capella.profile.sha256},anchor:{file:n===0?join(fixture,'initial.json'):'/config/initial.json',sha256:sourceRequest.anchor.sha256},limits:config.capella.limits,expectedCheckpoint:terminal.restart,requiredFinalized:checkpoint,maximumBlocks:'0x400',maximumArchiveBytes:'0x20000000'};
    await save(join(out,n+'-audit-request.json'),req);await save(join(out,n+'-external-stop-receipt.json'),terminal);
   }}


  catch(error){result='FAILED';failure=error;process.exitCode=1;await record({event:'failed',message:error.stack});}
 }
 await save(join(out,'result.json'),{result,failure:failure?.message??null,revision,runtimeRevision:artifacts?.runtimeRevision,remote,volume,physicalHosts:2,initialSyntheticValidators:64,admittedOffender:64,offlineRegistryAndAntiSlashingAuditQualified:false,automaticRecoveryFixture:false,pinnedStoppedDataResume:true,originalFailuresPreserved:true,sourceOutput:resume?.sourceOutput,productionQualified:false,transport:'cpp-libp2p over SSH tunnel',publicDeployment:false});await record({event:'completed',result});
}
