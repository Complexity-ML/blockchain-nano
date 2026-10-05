#!/usr/bin/env node
// Private, synthetic Capella exit campaign. Software arrives on the VPS through Git.
import {execFile,spawn} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdir,readFile,writeFile,appendFile} from 'node:fs/promises';
import {openSync,closeSync} from 'node:fs';
import {resolve,join} from 'node:path';
import {createHash,randomBytes} from 'node:crypto';
import {createServer} from 'node:net';
import {setTimeout as wait} from 'node:timers/promises';
import {selectExitArtifacts,verifyExitExecutableHashes,verifyExitVpsArtifacts} from './exit-artifacts.mjs';
import {monotonicClockConfiguration} from '../multihost/local-clock.mjs';
import {hashHarness} from '../multihost/harness-manifest.mjs';
const exec=promisify(execFile),[target,revision,outArg]=process.argv.slice(2);
if(!/^[a-z_][a-z0-9_-]*@[0-9.]+$/.test(target??'')||!/^([a-f0-9]{40})$/.test(revision??'')||!outArg)throw Error('Usage: exit-multihost.mjs user@IPv4 harness-revision NEW-output');
const repo=process.cwd(),out=resolve(outArg),fixture=join(out,'fixture'),id=randomBytes(6).toString('hex');
const remote='nano-exit-wan-'+id,remoteRoot='/tmp/'+remote,volume=remote+'-data';
const remoteRepo=`/home/${target.split('@')[0]}/blockchain-nano-git`;
const quote=s=>"'"+String(s).replaceAll("'","'\\''")+"'";
const run=async(bin,args,timeout=30000)=>(await exec(bin,args,{timeout,maxBuffer:16*1024*1024})).stdout;
const ssh=args=>run('ssh',['-o','BatchMode=yes','-o','ConnectTimeout=8',target,args.map(quote).join(' ')]);
const docker=args=>ssh(['sudo','-n','docker',...args]);
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
let artifacts,harness,native,tunnel,created=false,result='FAILED',failure,cleanupFailure=false;
const stopChild=async child=>{if(!child||child.exitCode!==null||child.signalCode!==null)return;child.kill('SIGTERM');for(let i=0;i<100&&child.exitCode===null&&child.signalCode===null;i++)await wait(100);if(child.exitCode===null&&child.signalCode===null){child.kill('SIGKILL');for(let i=0;i<50&&child.exitCode===null&&child.signalCode===null;i++)await wait(100);}};
try{
 check((await run('git',['rev-parse','HEAD'])).trim()===revision,'Local revision mismatch');
 check(process.env.NANO_TEST_EXIT_ARTIFACT_MANIFEST,'Explicit NANO_TEST_EXIT_ARTIFACT_MANIFEST required');
 artifacts=selectExitArtifacts(await json(resolve(process.env.NANO_TEST_EXIT_ARTIFACT_MANIFEST)));
 harness=await hashHarness(repo,['tests/capella/exit-multihost.mjs']);
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
 const ports=JSON.parse(await ssh(['python3','-c','import socket,json; a=socket.socket(); b=socket.socket(); a.bind(("127.0.0.1",0)); b.bind(("127.0.0.1",0)); print(json.dumps([a.getsockname()[1],b.getsockname()[1]]))']));
 check(ports.length===2&&ports.every(p=>Number.isInteger(p)&&p>1024&&p<65536)&&ports[0]!==ports[1],'Invalid reserved VPS ports');
 await run(exporter,['--export-exit-network',fixture],60000);
 const meta=await json(join(fixture,'fixture.json')),exit=await json(join(fixture,'exit.json'));
 check(meta.exitTest&&meta.chainId===1337&&meta.syntheticValidators===64&&meta.nodes===2&&exit.validatorIndex===0&&exit.epoch===0&&/^0x[0-9a-f]{224}$/.test(exit.ssz),'Unexpected synthetic exit fixture');
 let configs=await Promise.all([0,1].map(i=>json(join(fixture,`node-${i}.json`)))),envs=await Promise.all([0,1].map(i=>json(join(fixture,`env-${i}.json`))));
 const profile=await json(join(fixture,'profile.json'));
 configs=configs.map(c=>monotonicClockConfiguration(c,process.env.NANO_TEST_WALL_CLOCK_DRIFT_MS,profile));
 for(let n=0;n<2;n++){
  check(configs[n].capella.validator.keys.length===32,'Invalid key partition');
  for(let j=0;j<32;j++){const k=configs[n].capella.validator.keys[j],i=32*n+j;check(BigInt(k.index)===BigInt(i)&&k.keyEnv===`NANO_TEST_CAPELLA_NETWORK_KEY_${i}`&&BigInt(envs[n][k.keyEnv])===BigInt(i+1),'Non-public or misplaced key');}
  configs[n].capella.automaticRecovery=true;
 }
 const r=configs[1];r.rpc_port=ports[0];r.listen_addr=`/ip4/127.0.0.1/tcp/${ports[1]}`;r.p2p_binary=vps.executables.exporter.path;
 envs[1].NANO_TEST_EXIT_BOOTSTRAP=exit.ssz;envs[1].NANO_TEST_BLS_DAEMON=vps.executables.p2p.path;
 await save(join(fixture,'node-1.json'),r);await writeFile(join(fixture,'remote.env'),Object.entries(envs[1]).map(([k,v])=>`${k}=${v}`).join('\n')+'\n',{mode:0o600});
 await ssh(['mkdir',remoteRoot]);await run('scp',['-q','-r',fixture,`${target}:${remoteRoot}/config`]);
 await docker(['run','-d','--name',remote,'--label','nano.qualification=capella-exit-wan','--label','org.opencontainers.image.revision='+revision,'--network','host','--ulimit','nofile=65536:65536','--memory','4g','--cpus','2','--read-only','--tmpfs','/tmp','--cap-drop','ALL','--security-opt','no-new-privileges','--mount',`type=bind,src=${remoteRoot}/config,dst=/config,readonly`,'--mount',artifacts.mount,'--mount',`type=volume,src=${volume},dst=/var/lib/nano`,'--env-file',remoteRoot+'/config/remote.env','--entrypoint',vps.executables.nano.path,image,'run','/config/genesis.json','/var/lib/nano/chain','/config/node-1.json']);created=true;
 async function readyRemote(starts){const deadline=Date.now()+90000;while(Date.now()<deadline){const state=await inspect();check(state.State.Running&&!state.State.OOMKilled,'VPS node exited');const events=parseEvents(await remoteLogs());if(events.filter(e=>e.event==='altair-ipc-ready'&&e.networkProtocolActive===true).length>=starts)return events.filter(e=>e.event==='listen').at(-1).address;await wait(500);}throw Error('VPS startup deadline');}
 const address=await readyRemote(1);check(address.startsWith(`/ip4/127.0.0.1/tcp/${ports[1]}/p2p/`),'VPS listener mismatch');
 await record({event:'vps-binaries',sha256:verifyExitExecutableHashes(vps.executables,await docker(['exec',remote,'sha256sum',...Object.values(vps.executables).map(e=>e.path)]))});
 const rpcForward=await freePort();let p2pForward=await freePort();while(p2pForward===rpcForward)p2pForward=await freePort();
 const tf=openSync(join(out,'tunnel.log'),'a');tunnel=spawn('ssh',['-N','-o','BatchMode=yes','-o','ExitOnForwardFailure=yes','-o','ServerAliveInterval=15','-L',`127.0.0.1:${rpcForward}:127.0.0.1:${ports[0]}`,'-L',`127.0.0.1:${p2pForward}:127.0.0.1:${ports[1]}`,target],{stdio:['ignore',tf,tf]});closeSync(tf);tunnel.on('error',e=>{failure=e;});
 const m=configs[0],data=join(out,'mac-data');await mkdir(data);
 m.rpc_port=await freePort();m.listen_addr='/ip4/127.0.0.1/tcp/0';m.p2p_binary=p2p;m.p2p_identity=join(data,'peer.key');m.bootnodes=[address.replace(`/tcp/${ports[1]}/`,`/tcp/${p2pForward}/`)];m.capella.profile.file=join(fixture,'profile.json');m.capella.initial.file=join(fixture,'initial.json');m.capella.recoveryWitness=join(data,'witness');
 for(const k of m.capella.validator.keys){k.journal=join(data,'key-'+BigInt(k.index));k.recoveryWitness=k.journal+'-witness';}
 await save(join(out,'mac.json'),m);
 const urls=[`http://127.0.0.1:${m.rpc_port}`,`http://127.0.0.1:${rpcForward}`];
 async function request(n,path,body){const res=await fetch(urls[n]+path,{method:body?'POST':'GET',headers:{Accept:'application/json',...(body?{'Content-Type':'application/json'}:{})},...(body?{body:JSON.stringify(body)}:{}),signal:AbortSignal.timeout(10000)});if(!res.ok){const error=Error(`HTTP ${res.status} node ${n} ${path}`);error.httpStatus=res.status;error.node=n;throw error;}return res.json();}
 const rpc=async(n,method,params=[])=>{const d=await request(n,'/',{jsonrpc:'2.0',id:1,method,params});check(!d.error,`RPC ${method}: ${JSON.stringify(d.error)}`);return d.result;};
 let connected=false;for(let i=0;i<30;i++){try{await rpc(1,'eth_chainId');connected=true;break;}catch{check(tunnel.exitCode===null&&tunnel.signalCode===null,'SSH tunnel exited');await wait(500);}}check(connected,'SSH forward unavailable');
 const clean={...process.env};for(const k of Object.keys(clean))if(k.startsWith('NANO_TEST_CAPELLA_NETWORK_KEY_')||k==='NANO_TEST_EXIT_BOOTSTRAP')delete clean[k];
 const fd=openSync(join(out,'native.log'),'a');native=spawn(nativeBin,['run',join(fixture,'genesis.json'),join(data,'chain'),join(out,'mac.json')],{env:{...clean,...envs[0]},stdio:['ignore',fd,fd]});closeSync(fd);native.on('error',e=>{failure=e;});
 await record({event:'started',nativePid:native.pid,remote,volume,physicalHosts:2,validatorsPerHost:32,submissionHost:'vps',loopbackOnly:true});
 await save(join(out,'resources.json'),{target,revision,remote,volume,remoteRoot,urls,nativePid:native.pid,tunnelPid:tunnel.pid});
 let localReady=false;for(let i=0;i<60;i++){try{await rpc(0,'eth_chainId');localReady=true;break;}catch{check(native.exitCode===null&&native.signalCode===null,'Native node exited');await wait(500);}}check(localReady,'Native RPC startup deadline');
 let headersReady=false;const headerDeadline=Date.now()+90000;while(Date.now()<headerDeadline){try{for(let n=0;n<2;n++)await request(n,'/eth/v1/beacon/headers/head');headersReady=true;break;}catch(error){if(error.httpStatus!==404)throw error;}check(native.exitCode===null&&native.signalCode===null,'Native exited before signed head');await wait(1000);}check(headersReady,'Signed heads startup deadline');
 const blocks=[new Map(),new Map()],epochs=[0n,0n],roots=new Map(),exitRoots=new Map();let exitRoot,exitSlot,crashed=false,paidHeight,finalized,checkpoint,stable=0;
 async function observeChain(n,root){for(let depth=0;root!==meta.initialHead&&!blocks[n].has(root);depth++){
  check(depth<256,'Beacon ancestry exceeds fixture bounds');const d=await request(n,'/eth/v2/beacon/blocks/'+root),msg=d.data.message;
  check(d.version==='capella'&&!d.execution_optimistic,'Unqualified Beacon response');
  if(blocks[1-n].has(root))check(JSON.stringify(blocks[1-n].get(root))===JSON.stringify(msg),'Peers returned different Beacon block bodies');blocks[n].set(root,msg);
  for(const op of msg.body.voluntary_exits)if(op.message.validator_index==='0'){check(op.message.epoch==='0'&&op.signature==='0x'+exit.ssz.slice(34),'Included exit bytes differ');check(msg.body.voluntary_exits.filter(e=>e.message.validator_index==='0').length===1,'Duplicate exit within one block');exitRoots.set(root,BigInt(msg.slot));}
  root=msg.parent_root;
 }}
 const deadline=Date.now()+1200000;
 while(Date.now()<deadline){
  check(!failure&&native.exitCode===null&&native.signalCode===null&&tunnel.exitCode===null&&tunnel.signalCode===null,'Local process stopped');
  const rows=[];
  for(let n=0;n<2;n++){
   const cp=(await request(n,'/eth/v1/beacon/states/head/finality_checkpoints')).data.finalized,h=(await request(n,'/eth/v1/beacon/headers/head')).data,epoch=BigInt(cp.epoch);
   check(epoch>=epochs[n],'Finality regressed');check(!roots.has(cp.epoch)||roots.get(cp.epoch)===cp.root,'Conflicting finalized roots');epochs[n]=epoch;roots.set(cp.epoch,cp.root);
   await observeChain(n,h.root);const balance=await rpc(n,'eth_getBalance',[exit.recipient,'latest']);rows.push({node:n,checkpoint:cp,slot:h.header.message.slot,balance,peers:await rpc(n,'net_peerCount')});
   if(n===0&&BigInt(balance)>30000000000000000000n&&paidHeight===undefined)paidHeight=BigInt(await rpc(0,'eth_blockNumber'));
  }
  if(!crashed){for(const [root,slot] of exitRoots)if(blocks.every(b=>b.has(root))){exitRoot=root;exitSlot=slot;break;}}
  await record({event:'observation',rows,exitRoot,observedExitBranches:exitRoots.size,paidHeight:paidHeight?.toString()});
  if(!crashed&&exitRoot&&blocks.every(b=>b.has(exitRoot))){
   check(rows.every(r=>BigInt(r.checkpoint.epoch)<(exitSlot+BigInt(meta.slotsPerEpoch)-1n)/BigInt(meta.slotsPerEpoch)),'Exit already finalized before crash gate');
   const cutoff=BigInt(rows[0].slot)/BigInt(meta.slotsPerEpoch);await docker(['kill','--signal','KILL',remote]);const state=await inspect();check(!state.State.Running&&state.State.ExitCode===137,'VPS SIGKILL not observed');
   await record({event:'crash',exitRoot,cutoffEpoch:cutoff.toString(),containerId:state.Id,volume});await wait(meta.secondsPerSlot*2000);
   const cp=(await request(0,'/eth/v1/beacon/states/head/finality_checkpoints')).data.finalized;check(BigInt(cp.epoch)<=cutoff,'Half-stake survivor finalized beyond cutoff');
   await docker(['start',remote]);const restored=await readyRemote(2);check(restored===address,'Restart changed peer identity or listener');crashed=true;await record({event:'restarted',sameContainer:true,sameVolume:true,sameConfig:true,automaticRecovery:true});
   await wait(2000);continue;
  }
  const a=rows[0].checkpoint,b=rows[1].checkpoint;
  if(crashed&&paidHeight!==undefined&&a.epoch===b.epoch&&a.root===b.root&&rows.every(r=>BigInt(r.peers)>0n)){
   const heads=await Promise.all([0,1].map(n=>rpc(n,'eth_getBlockByNumber',['finalized',false])));
   if(heads.every(h=>h&&h.hash===heads[0].hash&&h.stateRoot===heads[0].stateRoot&&BigInt(h.number)>=paidHeight)){
    let beacon;try{beacon=await Promise.all([0,1].map(n=>request(n,'/eth/v2/beacon/blocks/'+a.root)));}
    catch(error){if(error.httpStatus!==404)throw error;const fresh=await Promise.all([0,1].map(n=>request(n,'/eth/v1/beacon/states/head/finality_checkpoints')));check(fresh[error.node].data.finalized.root!==a.root,'Current finalized checkpoint unavailable');await record({event:'checkpoint-changed-during-finality-read',previous:a,current:fresh.map(d=>d.data.finalized)});stable=0;await wait(1000);continue;}
    if(beacon.every(d=>d.finalized&&d.data.message.body.execution_payload.block_hash===heads[0].hash)){
     let tip=a.root,canonicalExits=[];for(let depth=0;tip!==meta.initialHead&&depth<256;depth++){if(exitRoots.has(tip))canonicalExits.push(tip);const m=blocks[0].get(tip);check(m,'Missing canonical ancestry');tip=m.parent_root;}
     check(tip===meta.initialHead&&canonicalExits.length===1,'Canonical finalized ancestry must contain exactly one exit');exitRoot=canonicalExits[0];if(++stable>=3){finalized=heads[0];checkpoint=a;break;}
    }else stable=0;
   }else stable=0;
  }else stable=0;
  await wait(3000);
 }
 check(finalized&&crashed,'Exit/finality/withdrawal deadline exceeded');
 let sum=0n,capital=0,payments=[];const height=BigInt(finalized.number);
 check(BigInt(await rpc(0,'eth_getBalance',[exit.recipient,'0x0']))===0n,'Recipient has non-withdrawal genesis funds');
 for(let number=0n;number<=height;number++){
  const tag='0x'+number.toString(16),a=await rpc(0,'eth_getBlockByNumber',[tag,false]),b=await rpc(1,'eth_getBlockByNumber',[tag,false]);
  check(a?.hash===b?.hash&&a?.stateRoot===b?.stateRoot&&JSON.stringify(a?.withdrawals)===JSON.stringify(b?.withdrawals),'Canonical execution history differs');
  for(const w of a.withdrawals??[])if(BigInt(w.validatorIndex)===0n){check(w.address.toLowerCase()===exit.recipient,'Wrong withdrawal recipient');const amount=BigInt(w.amount);sum+=amount*1000000000n;if(amount>30000000000n)capital++;payments.push({number:tag,...w});}
 }
 check(capital===1,'Capital payout missing or duplicated');for(let n=0;n<2;n++)check(BigInt(await rpc(n,'eth_getBalance',[exit.recipient,finalized.number]))===sum,'Finalized withdrawal balance differs from exact sum');
 await save(join(out,'payments.json'),payments);await record({event:'qualified-payments',checkpoint,exitRoot,finalizedNumber:finalized.number,finalizedHash:finalized.hash,balanceWei:sum.toString(),capitalPayments:capital,crashRecovery:true});result='PASSED';
}catch(error){failure=error;process.exitCode=1;await record({event:'failed',message:error.stack});}
finally{
 await stopChild(native);
 if(created){try{const state=await inspect();if(state.State.Running)await docker(['stop','--time','20',remote]);await save(join(out,'container-state.json'),await inspect());await writeFile(join(out,'vps.log'),await remoteLogs());}catch(error){cleanupFailure=true;await record({event:'cleanup-error',message:error.message});}}
 await stopChild(tunnel);
 if(result==='PASSED'){
  try{
   const terminalNative=await Promise.all(Object.values(artifacts.native).map(async e=>`${await hash(e.path)}  ${e.path}`));
   verifyExitExecutableHashes(artifacts.native,terminalNative.join('\n'));
   verifyExitVpsArtifacts(artifacts,JSON.parse(await docker(['image','inspect',artifacts.vps.image,'--format','{{json .}}'])),await docker(['run','--rm','--network','none','--read-only','--mount',artifacts.mount,'--entrypoint','cat',artifacts.vps.image,artifacts.revisionFile]),await docker(['run','--rm','--network','none','--read-only','--mount',artifacts.mount,'--entrypoint','sha256sum',artifacts.vps.image,...Object.values(artifacts.vps.executables).map(e=>e.path)]));
   check(JSON.stringify(await hashHarness(repo,['tests/capella/exit-multihost.mjs']))===JSON.stringify(harness),'Exit harness changed during campaign');
   for(const name of ['native.log','vps.log']){const stops=parseEvents(await readFile(join(out,name),'utf8')).filter(e=>e.event==='capella-runtime-stopped');const s=stops.at(-1);check(s?.clockHealthy&&s.validator?.healthy&&s.validator.localKeys===32&&s.validator.proposalsAdmitted>0&&s.validator.attestationsAdmitted>0,`${name}: validators not healthy or no distributed participation`);}check(!cleanupFailure,'Cleanup failure');}
  catch(error){result='FAILED';failure=error;process.exitCode=1;await record({event:'failed',message:error.stack});}
 }
 await save(join(out,'result.json'),{result,failure:failure?.message??null,revision,runtimeRevision:artifacts?.runtimeRevision,remote,volume,physicalHosts:2,syntheticValidators:64,automaticRecoveryFixture:true,productionQualified:false,transport:'cpp-libp2p over SSH tunnel',publicDeployment:false});await record({event:'completed',result});
}
