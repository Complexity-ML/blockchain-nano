#!/usr/bin/env node
// Isolated synthetic restored 65-validator fixture. No database crosses hosts.
import {execFile,spawn} from 'node:child_process';
import {promisify} from 'node:util';
import {readFile,writeFile,mkdir,copyFile,appendFile,readdir} from 'node:fs/promises';
import {openSync,closeSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {createServer} from 'node:net';
import {resolve,join} from 'node:path';
import {setTimeout as wait} from 'node:timers/promises';
import assert from 'node:assert/strict';
import {readDataManifest,verifyDataManifest,parseVolumeFileHashes,verifyFixtureFiles,verifyRuntimeVolumeReceipt,manifestDigest} from './restored-file-bindings.mjs';
import {verifyRestoredSourceStop} from './restored-source-stop.mjs';
import {restoredThreeNodePlan} from './restored-three-node-plan.mjs';
import {waitRestoredReadiness} from './restored-readiness.mjs';
import {restoredFinalityProgress} from './restored-finality-progress.mjs';
import {beaconObservationError,pendingBeaconObservation,verifyRestoredAncestry} from './restored-http-observation.mjs';
import {durableFinalityObservation} from './finality-observation.mjs';
const exec=promisify(execFile),[target,revision,preparedArg]=process.argv.slice(2);
assert.match(target??'',/^[a-z_][a-z0-9_-]*@[0-9.]+$/);assert.match(revision??'',/^[a-f0-9]{40}$/);assert.ok(preparedArg);
const repo=process.cwd(),prepared=resolve(preparedArg),out=join(prepared,'campaign');await mkdir(out);
const controllerFiles=['tests/capella/restored-file-bindings.mjs','tests/capella/restored-source-stop.mjs','tests/capella/restored-three-node-run.mjs','tests/capella/restored-three-node-plan.mjs','tests/capella/deposit-restart-pins.mjs','tests/capella/finality-observation.mjs','tests/capella/restored-readiness.mjs','tests/capella/restored-finality-progress.mjs','tests/capella/restored-http-observation.mjs','tests/capella/vps-local-launch-prepare.py'];let controllerHashes;
const evidence=resolve(process.env.NANO_TEST_RESTORED_PREPARATION??join(repo,'docs/qualification/restored-three-node-prepare-20261005'));assert.ok(evidence.startsWith(join(repo,'docs/qualification')+'/'),'Preparation must be a committed qualification bundle');
const json=async p=>JSON.parse(await readFile(p,'utf8')),save=(p,v)=>writeFile(p,JSON.stringify(v,null,2)+'\n');
const hash=async p=>createHash('sha256').update(await readFile(p)).digest('hex');
const q=x=>"'"+String(x).replaceAll("'","'\\''")+"'";
const run=async(bin,args,timeout=30000)=>(await exec(bin,args,{timeout,maxBuffer:32*1024*1024})).stdout;
const ssh=args=>run('ssh',['-o','BatchMode=yes','-o','ConnectTimeout=8',target,args.map(q).join(' ')]);
const docker=(args,remote=false)=>remote?ssh(['sudo','-n','docker',...args]):run('docker',args);
const record=async x=>{const row={time:new Date().toISOString(),...x};console.log(JSON.stringify(row));await appendFile(join(out,'events.jsonl'),JSON.stringify(row)+'\n');};
const events=s=>s.split('\n').flatMap(l=>{try{return [JSON.parse(l)];}catch{return [];}});
const port=()=>new Promise((ok,no)=>{const s=createServer();s.on('error',no);s.listen(0,'127.0.0.1',()=>{const p=s.address().port;s.close(()=>ok(p));});});
const instance=Date.now().toString(36),names=['mac-native','mac-docker','vps-docker'],local='nano-restored-three-node-mac-'+instance,remote='nano-restored-three-node-vps-'+instance,remoteRoot='/home/'+target.split('@')[0]+'/nanobees-qualification/runtime/'+remote;
let vpsVolume;const remoteRepo='/home/'+target.split('@')[0]+'/blockchain-nano-git';
let native,tunnel,localCreated=false,remoteCreated=false,failure,result='FAILED',common,artifacts,plan,urls;
const inspect=(name,onRemote)=>docker(['inspect',name,'--format','{{json .State}}'],onRemote).then(text=>({State:JSON.parse(text)}));
const logs=async(name,onRemote)=>{if(onRemote)return run('ssh',['-o','BatchMode=yes',target,['sudo','-n','docker','logs',name].map(q).join(' ')+' 2>&1']);const r=await exec('docker',['logs',name],{maxBuffer:32*1024*1024});return r.stdout+r.stderr;};
const spawnLog=(bin,args,name,env)=>{const fd=openSync(join(out,name),'a'),child=spawn(bin,args,{env,stdio:['ignore',fd,fd]});closeSync(fd);child.on('error',e=>failure=e);return child;};
const stop=async child=>{if(!child||child.exitCode!==null||child.signalCode!==null)return;child.kill('SIGTERM');for(let n=0;n<200&&child.exitCode===null&&child.signalCode===null;n++)await wait(100);if(child.exitCode===null&&child.signalCode===null){child.kill('SIGKILL');for(let n=0;n<50&&child.exitCode===null&&child.signalCode===null;n++)await wait(100);}};
try{
 assert.equal((await run('git',['rev-parse','HEAD'])).trim(),revision);assert.equal((await ssh(['git','-C',remoteRepo,'rev-parse','HEAD'])).trim(),revision);
 assert.equal((await run('git',['status','--porcelain'])).trim(),'');assert.equal((await ssh(['git','-C',remoteRepo,'status','--porcelain'])).trim(),'');
 const preparation=await json(join(evidence,'manifest.json'));if(preparation.sourceNativeStop){assert.equal(preparation.sourceNativeStop.exitCode,0);assert.equal(preparation.sourceNativeStop.signalCode,null);}vpsVolume=preparation.vpsCopiedVolume;assert.match(vpsVolume,/^nano-restored-three-node-vps-[a-z0-9-]+-data$/);
 artifacts=await json(join(evidence,'artifacts.json'));await run('git',['diff','--quiet',artifacts.coreRevision,'--','src','include','third_party']);await run('git',['diff','--quiet',artifacts.relayRevision,'--','network']);
 for(const [name,onRemote,expectedExit] of [[preparation.sourceMacContainer,false,preparation.sourceMacExpectedExit??0],[preparation.sourceVpsContainer,true,preparation.sourceVpsExpectedExit??0]]){let startupRefusalAudit,runtimeRefusalAudit;
  if(expectedExit===1){assert.equal(onRemote,true,'Exit1 recovery is scoped to the audited VPS source');const runtime=preparation.sourceRuntimeRefusalAudited===true;const file=runtime?'source-runtime-refusal-copy-audit.json':'source-startup-refusal-audit.json';assert.equal(await hash(join(evidence,file)),preparation.artifacts[file],'Refusal audit differs from committed preparation');if(runtime)runtimeRefusalAudit=await json(join(evidence,file));else startupRefusalAudit=await json(join(evidence,file));}
  verifyRestoredSourceStop((await inspect(name,onRemote)).State,{expectedExit,crashReadonlyAudited:preparation.sourceCrashReadonlyAudited,startupRefusalAudit,runtimeRefusalAudit});
 }
 const processes=await run('ps',['-axo','pid,command']);assert.ok(!processes.split('\n').some(l=>/\/nano(?: |$)/.test(l)&&/\brun\b/.test(l)),'An existing native writer is active');
 assert.equal((await docker(['ps','-q','--filter','volume='+vpsVolume],true)).trim(),'','Restored VPS volume already has a writer');
 const input=await json(join(evidence,'input.json'));plan=restoredThreeNodePlan(input);assert.deepEqual(plan,await json(join(evidence,'plan.json')));
 await verifyFixtureFiles(preparation.fixtureFiles,plan.configs[0].capella.profile.sha256);
 let runtimeVolumeReceipt;
 if(preparation.sourceRuntimeRefusalAudited===true){
  for(const f of ['vps-copied-file-hashes.json','vps-copied-volume-receipt.json'])assert.equal(await hash(join(evidence,f)),preparation.artifacts[f],'Unbound recovered-volume artifact');
  const expectedFiles=await json(join(evidence,'vps-copied-file-hashes.json'));runtimeVolumeReceipt=await json(join(evidence,'vps-copied-volume-receipt.json'));verifyRuntimeVolumeReceipt(expectedFiles,runtimeVolumeReceipt,vpsVolume);
  const currentFiles=await docker(['run','--rm','--network','none','--read-only','-v',vpsVolume+':/var/lib/nano:ro','--entrypoint','sh',artifacts.nodes['vps-docker'].image,'-c','test -z "$(find /var/lib/nano -type l -print -quit)" && find /var/lib/nano -type f -exec sha256sum {} + | sort'],true);
  verifyDataManifest(parseVolumeFileHashes(currentFiles),expectedFiles);
  assert.equal((await docker(['ps','-q','--filter','volume='+runtimeVolumeReceipt.sourceVolume],true)).trim(),'','Failed source volume has a writer');
  const sourceFiles=await docker(['run','--rm','--network','none','--read-only','-v',runtimeVolumeReceipt.sourceVolume+':/var/lib/nano:ro','--entrypoint','sh',artifacts.nodes['vps-docker'].image,'-c','test -z "$(find /var/lib/nano -type l -print -quit)" && find /var/lib/nano -type f -exec sha256sum {} + | sort'],true);
  assert.equal(manifestDigest(parseVolumeFileHashes(sourceFiles),'/source'),runtimeVolumeReceipt.sourceManifestSha256,'Original source volume changed');

 }

 for(const n of ['mac-native','mac-docker']){const hashes=await json(join(prepared,n,'data-copy-hashes.json'));verifyDataManifest(await readDataManifest(join(prepared,n,'data')),hashes);assert.ok(!(await readdir(join(prepared,n,'data'))).includes('peer.key'));}
 for(const [name,a] of Object.entries(artifacts.nodes)){
  if(name==='mac-native'){for(const x of [a.nano,a.p2p])assert.equal(await hash(x.path),x.sha256);}
  else{const text=await docker(['run','--rm','--network','none','-v',a.buildVolume+':/build:ro','--entrypoint','sha256sum',a.image,a.nano.path,a.p2p.path],name==='vps-docker');for(const x of [a.nano,a.p2p])assert.ok(text.split('\n').includes(x.sha256+'  '+x.path));}
 }
 controllerHashes=Object.fromEntries(await Promise.all(controllerFiles.map(async p=>[p,await hash(join(repo,p))])));await save(join(out,'controller-hashes.json'),controllerHashes);
 await save(join(out,'plan.json'),plan);await save(join(out,'artifacts.json'),artifacts);await record({event:'preflight-verified',revision,sourceWritersStopped:true,ownership:plan.ownership,physicalHosts:2,macDatabaseTransferred:false,networkTraceEnabled:true});
 const rp=JSON.parse(await ssh(['python3','-c','import socket,json; a=socket.socket();b=socket.socket();a.bind(("127.0.0.1",0));b.bind(("127.0.0.1",0));print(json.dumps([a.getsockname()[1],b.getsockname()[1]]))']));assert.equal(rp.length,2);assert.notEqual(rp[0],rp[1]);
 const lp=await port(),lr=await port(),nr=await port(),pf=await port(),rf=await port();assert.equal(new Set([lp,lr,nr,pf,rf]).size,5);
 const configs=structuredClone(plan.configs),fixture=join(prepared,'config');await mkdir(fixture);
 assert.equal(await hash(join(evidence,'input.json')),preparation.artifacts['input.json']);assert.equal(await hash(join(evidence,'plan.json')),preparation.artifacts['plan.json']);
 for(const f of ['genesis.json','profile.json','initial.json'])await copyFile(preparation.fixtureFiles[f].path,join(fixture,f));
 const envFor=n=>({...Object.fromEntries(plan.ownership[n].map(i=>['NANO_TEST_CAPELLA_NETWORK_KEY_'+i,'0x'+BigInt(i+1).toString(16).padStart(64,'0')])),NANO_TEST_NETWORK_TRACE:'1'});
 const environment={...process.env};for(const k of Object.keys(environment))if(k.startsWith('NANO_TEST_CAPELLA_NETWORK_KEY_')||k==='NANO_TEST_EXIT_BOOTSTRAP')delete environment[k];
 for(const [i,c] of configs.entries()){
  const a=artifacts.nodes[names[i]];c.p2p_binary=a.p2p.path;c.p2p_identity='/var/lib/nano/peer.key';c.bootnodes=[];c.rpc_host='127.0.0.1';c.rpc_port=i===2?rp[0]:8545;c.listen_addr=i===2?`/ip4/127.0.0.1/tcp/${rp[1]}`:'/ip4/0.0.0.0/tcp/30303';c.capella.profile.file='/config/profile.json';assert.deepEqual({...c.capella.network,enabled:true},configs[2].capella.network,'Network protocol budgets/profile differ');c.capella.network.enabled=true;
  // Same administrative catch-up bounds already qualified on the restored Mac copy.
  c.capella.clock=structuredClone(configs[0].capella.clock);
 }
 await save(join(fixture,'vps.json'),configs[2]);await writeFile(join(fixture,'vps.env'),Object.entries(envFor('vps-docker')).map(([k,v])=>k+'='+v).join('\n')+'\n',{mode:0o600});
 if(process.env.NANO_TEST_VPS_LOCAL_SOURCE){
  // Bind a VPS-local derivation to the independent plan without exporting
  // configuration, administrative credentials or signer environments.
  const stable=v=>Array.isArray(v)?v.map(stable):v&&typeof v==='object'?Object.fromEntries(Object.keys(v).sort().map(k=>[k,stable(v[k])])):v;
  const expected=createHash('sha256').update(JSON.stringify(stable(configs[2]))).digest('hex');
  const helper=await readFile(join(repo,'tests/capella/vps-local-launch-prepare.py'),'utf8');
  await ssh(['mkdir','-p','/home/'+target.split('@')[0]+'/nanobees-qualification/runtime']);
  const proof=JSON.parse(await ssh(['python3','-c',helper,process.env.NANO_TEST_VPS_LOCAL_SOURCE,remoteRoot,String(rp[0]),String(rp[1]),artifacts.nodes['vps-docker'].p2p.path,expected,JSON.stringify(Object.fromEntries(['retentionExperiment','coldCheckpoint','startupCatchup'].filter(k=>Object.hasOwn(input,k)).map(k=>[k,input[k]])))]));
  assert.equal(proof.configCanonicalSHA256,expected);assert.equal(proof.configPreparedLocally,true);assert.equal(proof.configurationExportedFromMac,false);assert.equal(proof.signingKeysExportedFromMac,false);
  for(const f of ['genesis.json','profile.json','initial.json'])assert.equal(proof.files[f],await hash(join(fixture,f)));
  if(runtimeVolumeReceipt){assert.equal(proof.runtimeCopySource?.actualOriginalAndRecoveredFilesVerified,true);assert.equal(proof.runtimeCopySource.sourceManifestSha256,runtimeVolumeReceipt.sourceManifestSha256);assert.equal(proof.runtimeCopySource.recoveredManifestSha256,runtimeVolumeReceipt.recoveredManifestSha256);assert.equal(proof.runtimeCopySource.captureSHA256,runtimeVolumeReceipt.captureSHA256);}

  await save(join(out,'vps-local-preparation-proof.json'),proof);
 }else{
  await ssh(['mkdir','-p',remoteRoot]);for(const f of ['genesis.json','profile.json','initial.json','vps.json','vps.env'])await run('scp',[join(fixture,f),target+':'+remoteRoot+'/'+f]);
 }
 const va=artifacts.nodes['vps-docker'];await docker(['run','-d','--name',remote,'--network','host','--ulimit','nofile=65536:65536','--memory','4g','--cpus','2','--read-only','--tmpfs','/tmp','--cap-drop','ALL','--security-opt','no-new-privileges','-v',va.buildVolume+':/build:ro','-v',vpsVolume+':/var/lib/nano','-v',remoteRoot+':/config:ro','--env-file',remoteRoot+'/vps.env','--entrypoint',va.nano.path,va.image,'run','/config/genesis.json','/var/lib/nano/chain','/config/vps.json'],true);remoteCreated=true;
 let peer;const readyUntil=Date.now()+180000;while(Date.now()<readyUntil){const s=await inspect(remote,true);assert.equal(s.State.Running,true,'VPS stopped during startup');const e=events(await logs(remote,true));if(e.some(x=>x.event==='altair-ipc-ready'&&x.networkProtocolActive===true)){peer=e.filter(x=>x.event==='listen').at(-1)?.address;break;}await wait(1000);}assert.ok(peer?.startsWith(`/ip4/127.0.0.1/tcp/${rp[1]}/p2p/`),'VPS IPC readiness deadline');
 tunnel=spawnLog('ssh',['-N','-o','BatchMode=yes','-o','ExitOnForwardFailure=yes','-o','ServerAliveInterval=15','-L',`127.0.0.1:${pf}:127.0.0.1:${rp[1]}`,'-L',`127.0.0.1:${rf}:127.0.0.1:${rp[0]}`,target],'tunnel.log',environment);await wait(1000);assert.equal(tunnel.exitCode,null);assert.equal(tunnel.signalCode,null);
 const peerId=peer.split('/p2p/')[1];assert.match(peerId,/^[A-Za-z0-9]+$/);configs[1].bootnodes=[`/dns4/host.docker.internal/tcp/${pf}/p2p/${peerId}`];await save(join(fixture,'mac-docker.json'),configs[1]);await writeFile(join(fixture,'mac-docker.env'),Object.entries(envFor('mac-docker')).map(([k,v])=>k+'='+v).join('\n')+'\n',{mode:0o600});
 const ma=artifacts.nodes['mac-docker'];await docker(['run','-d','--name',local,'--ulimit','nofile=65536:65536','--memory','4g','--cpus','2','--read-only','--tmpfs','/tmp','--cap-drop','ALL','--security-opt','no-new-privileges','-p',`127.0.0.1:${lp}:30303`,'-v',ma.buildVolume+':/build:ro','-v',join(prepared,'mac-docker/data')+':/var/lib/nano','-v',fixture+':/config:ro','--env-file',join(fixture,'mac-docker.env'),'--entrypoint',ma.nano.path,ma.image,'run','/config/genesis.json','/var/lib/nano/chain','/config/mac-docker.json']);localCreated=true;
 const c=configs[0],nd=join(prepared,'mac-native/data');c.rpc_host='127.0.0.1';c.rpc_port=nr;c.listen_addr='/ip4/127.0.0.1/tcp/0';c.p2p_identity=join(nd,'peer.key');c.bootnodes=[`/ip4/127.0.0.1/tcp/${pf}/p2p/${peerId}`];c.capella.profile.file=join(fixture,'profile.json');c.capella.recoveryWitness=c.capella.recoveryWitness.replace('/var/lib/nano',nd);for(const k of c.capella.validator.keys){k.journal=k.journal.replace('/var/lib/nano',nd);k.recoveryWitness=k.recoveryWitness.replace('/var/lib/nano',nd);}await save(join(fixture,'mac-native.json'),c);
 native=spawnLog(artifacts.nodes['mac-native'].nano.path,['run',join(fixture,'genesis.json'),join(nd,'chain'),join(fixture,'mac-native.json')],'native.log',{...environment,...envFor('mac-native')});
 urls=[`http://127.0.0.1:${nr}`,`http://127.0.0.1:${lr}`,`http://127.0.0.1:${rf}`];await save(join(out,'resources.json'),{nativePid:native.pid,tunnelPid:tunnel.pid,localContainer:local,remoteContainer:remote,vpsVolume,urls,prepared,remoteRoot});await record({event:'started',nativePid:native.pid,local,remote,ownership:plan.ownership,loopbackOnly:true,transport:'cpp-libp2p over SSH forwarding'});
 await waitRestoredReadiness({sample:async()=>{const nativeRows=events(await readFile(join(out,'native.log'),'utf8'));const rows=[{name:'mac-native',running:native.exitCode===null&&native.signalCode===null,networkReady:nativeRows.some(x=>x.event==='altair-ipc-ready'&&x.networkProtocolActive===true)}];for(const [name,rem,owner] of [[local,false,'mac-docker'],[remote,true,'vps-docker']]){const s=await inspect(name,rem),ev=events(await logs(name,rem));rows.push({name:owner,running:s.State.Running&&!s.State.OOMKilled,networkReady:ev.some(x=>x.event==='altair-ipc-ready'&&x.networkProtocolActive===true)});}assert.equal(tunnel.exitCode,null);assert.equal(tunnel.signalCode,null);return rows;},record});
 const request=async(n,path,body)=>{if(n===1){const args=['exec',local,'curl','-sS','--max-time','10','-H','Accept: application/json','--write-out','\n%{http_code}'];if(body)args.push('-H','Content-Type: application/json','--data',JSON.stringify(body));args.push('http://127.0.0.1:8545'+path);const text=await docker(args),cut=text.lastIndexOf('\n'),status=Number(text.slice(cut+1));if(status!==200){throw beaconObservationError(status,names[n],path,text.slice(0,cut));}return JSON.parse(text.slice(0,cut));}const response=await fetch(urls[n]+path,{method:body?'POST':'GET',headers:{Accept:'application/json',...(body?{'Content-Type':'application/json'}:{})},...(body?{body:JSON.stringify(body)}:{}),signal:AbortSignal.timeout(10000)});if(!response.ok){throw beaconObservationError(response.status,names[n],path,await response.text());}return response.json();};
 const rpc=async(n,method)=>{const x=await request(n,'/',{jsonrpc:'2.0',id:1,method,params:[]});assert.ok(!x.error,JSON.stringify(x.error));return x.result;};
 const baseline=BigInt(plan.baseline.epoch),previous=[baseline,baseline,baseline],known=new Map([[String(baseline),plan.baseline.root]]),blocks=[new Map(),new Map(),new Map()];const progress=restoredFinalityProgress(plan.baseline);const deadline=Date.now()+900000;
 const ancestry=(n,root)=>verifyRestoredAncestry({root,baseline:plan.baseline.root,node:n,caches:blocks,read:tip=>request(n,'/eth/v2/beacon/blocks/'+tip)});
 while(Date.now()<deadline){
  assert.ok(!failure);assert.equal(native.exitCode,null,'Native exited');assert.equal(native.signalCode,null);assert.equal(tunnel.exitCode,null);assert.equal(tunnel.signalCode,null);for(const [name,rem] of [[local,false],[remote,true]])assert.equal((await inspect(name,rem)).State.Running,true,name+' exited');
  const rows=[];for(let n=0;n<3;n++){
   try{const head=(await request(n,'/eth/v1/beacon/headers/head')).data,state=(await request(n,'/eth/v1/beacon/states/head/finality_checkpoints')).data.finalized;
    const a=await request(n,'/eth/v1/beacon/headers/finalized'),observation=durableFinalityObservation(state,a,known,previous[n]),b=await request(n,'/eth/v1/beacon/headers/finalized');if(a.data.root!==b.data.root||!observation){await record({event:'finality-read-pending',node:n,state,first:a.data.root,second:b.data.root});continue;}
    const cp=observation.checkpoint,epoch=BigInt(cp.epoch),key=String(epoch);assert.ok(epoch>=previous[n],'Finality regressed');if(known.has(key))assert.equal(cp.root,known.get(key),'Conflicting finalized roots');previous[n]=epoch;known.set(key,cp.root);await ancestry(n,cp.root);rows.push({name:names[n],checkpoint:cp,slot:head.header.message.slot,head:head.root,peers:await rpc(n,'net_peerCount')});
   }catch(e){if(e.status){await record({event:'http-read-refusal',node:n,path:e.path,httpStatus:e.status,responseBody:e.responseBody});}if(pendingBeaconObservation(e)){await record({event:'observation-pending',node:n,reason:'known Beacon read budget refusal'});continue;}if(e.status===404||e.cause?.code==='ECONNREFUSED'){await record({event:'startup-pending',node:n,message:e.message});continue;}throw e;}
  }
  await record({event:'observation',rows});common=progress.observe(rows);await save(join(out,'distinct-finality-progress.json'),progress.snapshot());if(common)break;await wait(3000);
 }
 assert.ok(common,'Three-node common finality did not advance through three distinct checkpoints before deadline');await save(join(out,'canonical-blocks.json'),blocks.map(m=>[...m].map(([root,message])=>({root,message}))));result='PASSED';
}catch(e){failure=e;process.exitCode=1;await record({event:'failed',message:e.stack});}
finally{
 await stop(native);
 for(const [name,rem,created,file] of [[local,false,localCreated,'docker-mac.log'],[remote,true,remoteCreated,'vps.log']])if(created){try{if((await inspect(name,rem)).State.Running)await docker(['stop','--time','20',name],rem);await save(join(out,file+'.state.json'),await inspect(name,rem));await writeFile(join(out,file),await logs(name,rem));}catch(e){result='FAILED';failure??=e;process.exitCode=1;await record({event:'cleanup-error',message:e.message});}}
 await stop(tunnel);
 if(native)await save(join(out,'native-state.json'),{pid:native.pid,exitCode:native.exitCode,signalCode:native.signalCode});
 const receipts=[];for(const [n,file] of ['native.log','docker-mac.log','vps.log'].entries())try{const rows=events(await readFile(join(out,file),'utf8')),receipt=rows.filter(x=>x.event==='capella-runtime-stopped').at(-1);if(receipt){await save(join(out,n+'-external-stop-receipt.json'),receipt);receipts.push(receipt);}if(result==='PASSED'){assert.ok(receipt?.clockHealthy&&receipt.validator?.healthy);assert.ok(BigInt(receipt.finalized.epoch)>=BigInt(common.epoch),'Stopped durable finality behind observed common checkpoint');assert.equal(receipt.validator.localKeys,[16,16,33][n]);assert.ok(receipt.validator.proposalsAdmitted>0&&receipt.validator.attestationsAdmitted>0,'Each owner must actually participate');}}catch(e){result='FAILED';failure??=e;process.exitCode=1;}
 if(result==='PASSED')try{assert.equal(native.exitCode,0);assert.equal(native.signalCode,null);for(const [name,rem] of [[local,false],[remote,true]]){const s=await inspect(name,rem);assert.equal(s.State.Status,'exited');assert.equal(s.State.ExitCode,0);assert.equal(s.State.OOMKilled,false);}assert.deepEqual(controllerHashes,Object.fromEntries(await Promise.all(controllerFiles.map(async p=>[p,await hash(join(repo,p))]))));}catch(e){result='FAILED';failure??=e;process.exitCode=1;}
 await save(join(out,'result.json'),{result,commonFinality:common??null,baseline:plan?.baseline??null,stopReceipts:receipts.length,failure:failure?.message??null,networkScope:'private restored three-node/two-host common finality only',productionQualified:false});await record({event:'terminal',result,commonFinality:common??null});
}
