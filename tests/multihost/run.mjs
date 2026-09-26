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
import {observeRecovery} from './recovery-observation.mjs';
const exec=promisify(execFile),[target,revision,outputArg]=process.argv.slice(2);
if(!/^[a-z_][a-z0-9_-]*@[0-9.]+$/.test(target??'')||!(/^[0-9a-f]{40}$/).test(revision??'')||!outputArg)throw Error('Usage: run.mjs user@IPv4 core-revision fresh-output');
const output=resolve(outputArg),repo=process.cwd(),image=`blockchain-nano:${revision.slice(0,7)}`;
const traceArgs=process.env.NANO_TEST_NETWORK_TRACE==='1'?['--env','NANO_TEST_NETWORK_TRACE=1']:[];
const id=Date.now().toString(36),local=`nano-qualification-mac-${id}`,remote=`nano-qualification-vps-${id}`;
const partition=process.env.NANO_TEST_WAN_PARTITION==='1',testNetwork=remote+'-net';
const remoteRoot=`/home/${target.split('@')[0]}/nano-qualification-${id}`,ip=target.split('@')[1];
const quote=s=>"'"+String(s).replaceAll("'","'\\''")+"'";
const sh=async args=>(await exec('ssh',['-o','BatchMode=yes','-o','ConnectTimeout=8',target,args.map(quote).join(' ')],{timeout:30000,maxBuffer:8*1024*1024})).stdout;
const docker=async(args,onRemote=false)=>onRemote?sh(['sudo','-n','docker',...args]):(await exec('docker',args,{timeout:30000,maxBuffer:8*1024*1024})).stdout;
const captureLogs=async(name,onRemote)=>{
 const args=onRemote?['ssh','-o','BatchMode=yes',target,['sudo','-n','docker','logs',name].map(quote).join(' ')+' 2>&1']:['docker','logs',name];
 const result=await exec(args[0],args.slice(1),{timeout:30000,maxBuffer:8*1024*1024});
 return result.stdout+result.stderr;
};
const json=async p=>JSON.parse(await readFile(p,'utf8'));
// Preflight before creating fixtures, so compilation cannot age the genesis.
for(const onRemote of [false,true]){
 const info=JSON.parse(await docker(['image','inspect',image,'--format','{{json .}}'],onRemote));
 if(info.Config.Labels?.['org.opencontainers.image.revision']!==revision||info.Architecture!==(onRemote?'amd64':'arm64'))throw Error('Build revision/architecture mismatch');
}
await mkdir(output);
const log=async event=>appendFile(join(output,'run.jsonl'),JSON.stringify({time:Date.now(),...event})+'\n');
let native,localStarted=false,remoteStarted=false;
const request=async(kind,path,body)=>{
 const args=['-fsS','--max-time','5','-H','Accept: application/json'];
 if(body)args.push('-H','Content-Type: application/json','--data',JSON.stringify(body));
 args.push((kind==='native'?'http://127.0.0.1:18545':'http://127.0.0.1:8545')+path);
 const text=kind==='native'?(await exec('curl',args,{timeout:8000})).stdout:await docker(['exec',kind==='remote'?remote:local,'curl',...args],kind==='remote');
 return JSON.parse(text);
};
const rpc=async(kind,method,params=[])=>{
 const r=await request(kind,'/',{jsonrpc:'2.0',id:1,method,params});if(r.error)throw Error(JSON.stringify(r.error));return r.result;
};
try{
 const binary=join(repo,'build-release/nano'),p2p=join(repo,'build-network/nano-p2p');
 const harness={};for(const name of ['run.mjs','partition.mjs','observe.mjs','validator-gate.mjs','recovery-observation.mjs'])harness[name]=createHash('sha256').update(await readFile(join(repo,'tests/multihost',name))).digest('hex');
 await log({event:'harness',partition,sha256:harness});
 await log({event:'binaries',revision,nanoSha256:createHash('sha256').update(await readFile(binary)).digest('hex'),p2pSha256:createHash('sha256').update(await readFile(p2p)).digest('hex')});
 const assets=join(output,'assets'),prepared=join(output,'prepared');
 await exec(join(repo,'build-release/nano-bellatrix-transition-tests'),['--export-kurtosis',assets],{timeout:30000});
 const topology=[{name:'mac-native',host:'mac',mode:'native',rpcPort:18545,p2pPort:30305,p2pBinary:p2p,runtimeDirectory:join(prepared,'mac-native')},{name:'mac-docker',host:'mac',mode:'docker',rpcPort:8545,p2pPort:30303},{name:'vps-docker',host:target,mode:'docker',rpcPort:8545,p2pPort:30303}];
 await writeFile(join(output,'topology.json'),JSON.stringify(topology));
 await exec(process.execPath,['tests/multihost/prepare.mjs',assets,prepared,join(output,'topology.json')]);
 await sh(['mkdir',remoteRoot]);
 // Only newly generated PUBLIC fixtures cross SSH; software arrived via Git.
 await exec('scp',['-q','-r',join(prepared,'vps-docker'),`${target}:${remoteRoot}/config`],{timeout:30000});
 if(partition)await docker(['network','create','--label','nano.qualification=multihost',testNetwork],true);
 await docker(['run','--init',...(partition?['--network',testNetwork]:[]),...traceArgs,'-d','--name',remote,'--label','nano.qualification=multihost','--memory','3g','--cpus','2','-p','30307:30303','--mount',`type=bind,src=${remoteRoot}/config,dst=/config,readonly`,'--mount',`type=volume,src=${remote}-data,dst=/var/lib/nano`,'--env-file',`${remoteRoot}/config/fixture.env`,image,'run','/config/genesis.json','/var/lib/nano/chain','/config/node.json'],true);remoteStarted=true;
 let peer;
 for(let i=0;i<30;i++){try{peer=(await rpc('remote','nno_getNetworkInfo')).node?.peerId;if(peer)break;}catch{}await delay(1000);}
 if(!peer||!/^[A-Za-z0-9]+$/.test(peer))throw Error('Remote peer ID unavailable');
 const boot=`/ip4/${ip}/tcp/30307/p2p/${peer}`;
 for(const name of ['mac-native','mac-docker']){const path=join(prepared,name,'node.json'),c=await json(path);c.bootnodes=[boot];await writeFile(path,JSON.stringify(c,null,2));}
 await docker(['run','--init',...traceArgs,'-d','--name',local,'--label','nano.qualification=multihost','--memory','3g','--cpus','2','--mount',`type=bind,src=${join(prepared,'mac-docker')},dst=/config,readonly`,'--mount',`type=volume,src=${local}-data,dst=/var/lib/nano`,'--env-file',join(prepared,'mac-docker','fixture.env'),image,'run','/config/genesis.json','/var/lib/nano/chain','/config/node.json']);localStarted=true;
 const nd=join(prepared,'mac-native');await mkdir(join(nd,'data'));
 const environment=Object.fromEntries((await readFile(join(nd,'fixture.env'),'utf8')).trim().split('\n').map(line=>{const at=line.indexOf('=');return [line.slice(0,at),line.slice(at+1)];}));
 const fd=openSync(join(output,'native.log'),'a');native=spawn(binary,['run',join(nd,'genesis.json'),join(nd,'data/chain'),join(nd,'node.json')],{env:{...process.env,...environment},stdio:['ignore',fd,fd]});closeSync(fd);
 native.on('error',error=>log({event:'native-spawn-error',message:error.message}));
 await log({event:'started',nativePid:native.pid,localContainer:local,remoteContainer:remote,bootnode:boot,physicalHosts:2,validatorKeys:64});
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
 const observers={revision,genesisHash:genesis.hash,nodes:[{name:'mac-native',kind:'native',url:'http://127.0.0.1:18545'},{name:'mac-docker',kind:'docker',container:local,url:'http://127.0.0.1:8545'},{name:'vps-docker',kind:'ssh-docker',ssh:target,container:remote,url:'http://127.0.0.1:8545'}]};
 await writeFile(join(output,'observers.json'),JSON.stringify(observers,null,2));
 await observeRecovery({record:log,partition:async()=>{if(partition)await partitionRemote({docker:args=>docker(args,true),container:remote,network:testNetwork,revision,record:log,
  sample:()=>Promise.all(['native','local','remote'].map(async(kind,i)=>({name:observers.nodes[i].name,
   peers:await rpc(kind,'net_peerCount'),
   checkpoint:(await request(kind,'/eth/v1/beacon/states/head/finality_checkpoints')).data.finalized}))) });},
 observe:()=>exec(process.execPath,['tests/multihost/observe.mjs',join(output,'observers.json'),join(output,'observations')],{timeout:480000,maxBuffer:8*1024*1024})});
 await log({event:'finality-passed'});
}catch(error){await log({event:'failed',message:error.stack});process.exitCode=1;}
finally{
 if(native&&native.exitCode===null&&native.signalCode===null){native.kill('SIGTERM');for(let i=0;i<30&&native.exitCode===null&&native.signalCode===null;i++)await delay(1000);if(native.exitCode===null&&native.signalCode===null)native.kill('SIGKILL');}
 for(const [name,onRemote,started] of [[local,false,localStarted],[remote,true,remoteStarted]])if(started){
  try{await docker(['stop','--time','20',name],onRemote);await writeFile(join(output,onRemote?'vps.log':'docker-mac.log'),await captureLogs(name,onRemote));}catch(error){await log({event:'cleanup-error',name,message:error.message});process.exitCode=1;}
 }
 // Retain stopped containers/volumes and native DBs for inspection; no pruning.
}
if(native&&localStarted&&remoteStarted){
 try{const validators=await validateStoppedValidators(output);await log({event:process.exitCode?'validators-healthy-after-failed-campaign':'passed',validators});}
 catch(error){await log({event:'failed',message:error.stack});process.exitCode=1;}
}
