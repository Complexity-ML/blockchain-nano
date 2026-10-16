#!/usr/bin/env node
// Local read-only monitoring. Never signs, restarts a node or sends notifications.
import {createHash} from 'node:crypto';
import {readFile,open,unlink} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
import {durableJson} from './durable-json.mjs';
import {transportRequest} from './monitor-transport.mjs';
const root=v=>typeof v==='string'&&/^0x[0-9a-f]{64}$/.test(v);
const integer=v=>{if(typeof v!=='string'||v.length>78||! /^(0|[1-9][0-9]*|0x[0-9a-f]+)$/.test(v))throw Error('Invalid unsigned quantity');return BigInt(v);};
export function configuration(c){
 if(!c||!root(c.genesisHash)||typeof c.configName!=='string'||!c.configName||!Array.isArray(c.nodes)||!c.nodes.length)throw Error('Pinned chain, profile and nodes required');
 const chainId=integer(c.chainId).toString();
 for(const key of ['staleMs','timeoutMs','maximumResponseBytes','maximumEpochs'])if(!Number.isSafeInteger(c[key])||c[key]<=0)throw Error('Positive '+key+' required');
 if(!Number.isSafeInteger(c.minimumPeers)||c.minimumPeers<0)throw Error('Invalid minimumPeers');
 const names=new Set(),urls=new Set(),nodes=c.nodes.map(n=>{
  if(!/^[a-z0-9-]+$/.test(n.name)||names.has(n.name))throw Error('Duplicate/invalid name');names.add(n.name);
  const u=new URL(n.url);if(u.protocol!=='http:'||u.hostname!=='127.0.0.1'||u.username||u.password||u.pathname!=='/'||u.hash||u.search)throw Error('Loopback RPC required');
  const kind=n.kind??'http';if(!['http','docker','ssh-docker'].includes(kind))throw Error('Invalid monitor transport');
  if(kind!=='http'&&!/^[a-zA-Z0-9][a-zA-Z0-9_.-]*$/.test(n.container??''))throw Error('Invalid monitor container');
  if(kind==='ssh-docker'&&!/^[a-z_][a-z0-9_-]*@[a-zA-Z0-9.-]+$/.test(n.ssh??''))throw Error('Invalid monitor SSH target');
  const transport=kind+':'+(n.ssh??'local')+':'+(n.container??'')+':'+u.origin;if(urls.has(transport))throw Error('Duplicate monitor transport');urls.add(transport);
  return {name:n.name,url:u.origin,kind,...(kind!=='http'?{container:n.container}:{}),...(kind==='ssh-docker'?{ssh:n.ssh}:{})};
 });
 return {chainId,genesisHash:c.genesisHash,configName:c.configName,staleMs:c.staleMs,timeoutMs:c.timeoutMs,maximumResponseBytes:c.maximumResponseBytes,maximumEpochs:c.maximumEpochs,minimumPeers:c.minimumPeers,nodes};
}
const identity=c=>createHash('sha256').update(JSON.stringify(c)).digest('hex');
export const monitorIdentity=config=>identity(configuration(config));
export function evaluate(config,samples,time,previous){
 const c=configuration(config);if(!Number.isSafeInteger(time)||time<0)throw Error('Invalid observation time');
 const state=previous?structuredClone(previous):{version:1,identity:identity(c),time,nodes:{},roots:{},safetyFaults:[]};
 if(state.version!==1||state.identity!==identity(c)||!Number.isSafeInteger(state.time)||time<state.time||!state.nodes||!state.roots||!Array.isArray(state.safetyFaults))throw Error('Monitor state/clock mismatch');
 if(!Array.isArray(samples)||samples.length!==c.nodes.length||new Set(samples.map(s=>s.name)).size!==samples.length||samples.some(s=>!c.nodes.some(n=>n.name===s.name)))throw Error('Sample identities differ');
 if(Object.keys(state.roots).length>c.maximumEpochs||Object.entries(state.roots).some(([epoch,value])=>!root(value)||integer(epoch).toString()!==epoch))throw Error('Malformed/excessive persisted root history');
 const alerts=[],accepted=new Set();const fault=(code,node)=>{alerts.push({code,node});if(!state.safetyFaults.some(a=>a.code===code&&a.node===node))state.safetyFaults.push({code,node});};
 for(const s of samples){
  const prior=state.nodes[s.name];
  if(prior&&(!root(prior.root)||integer(prior.epoch)<0n||state.roots[integer(prior.epoch).toString()]!==prior.root||!Number.isSafeInteger(prior.changed)||prior.changed<0||prior.changed>state.time))throw Error('Malformed persisted progress');
  if(s.error){alerts.push({code:'unavailable',node:s.name});continue;}
  let epoch;
  try{epoch=integer(s.epoch);if(epoch>18446744073709551615n||!root(s.root)||!Number.isSafeInteger(s.peers)||s.peers<0)throw Error('Invalid observation');}catch{alerts.push({code:'invalid-observation',node:s.name});continue;}
  const key=epoch.toString(),known=state.roots[key];
  if(prior&&epoch<integer(prior.epoch)){fault('finality-regression',s.name);continue;}
  if(known&&known!==s.root){fault('conflicting-finality',s.name);continue;}
  if(!known&&Object.keys(state.roots).length>=c.maximumEpochs){fault('history-capacity',s.name);continue;}
  state.roots[key]=s.root;accepted.add(s.name);
  const changed=!prior||epoch>integer(prior.epoch)?time:prior.changed;
  state.nodes[s.name]={epoch:key,root:s.root,changed};
  if(time-changed>=c.staleMs)alerts.push({code:'finality-stalled',node:s.name});
  if(s.peers<c.minimumPeers)alerts.push({code:'insufficient-peers',node:s.name});
 }
 // Once all fixed observers advance, old roots are unnecessary: regression is
 // checked against each observer's persisted highest epoch before comparison.
 if(c.nodes.every(n=>state.nodes[n.name])){
  const floor=c.nodes.map(n=>integer(state.nodes[n.name].epoch)).reduce((a,b)=>a<b?a:b);
  for(const key of Object.keys(state.roots))if(integer(key)<floor)delete state.roots[key];
 }
 state.time=time;
 const common=c.nodes.every(n=>state.nodes[n.name])&&accepted.size===c.nodes.length&&c.nodes.every(n=>state.nodes[n.name].epoch===state.nodes[c.nodes[0].name].epoch&&state.nodes[n.name].root===state.nodes[c.nodes[0].name].root);
 return {state,report:{identity:state.identity,time,healthy:!alerts.length&&!state.safetyFaults.length,commonFinality:common,alerts,safetyFaults:state.safetyFaults,samples}};
}
export async function sample(config,node,request){
 const c=configuration(config),endpoint=c.nodes.find(n=>n.name===node.name);
 if(!endpoint||new URL(node.url).origin!==endpoint.url)throw Error('Unconfigured monitor endpoint');
 request??=endpoint.kind==='http'?fetch:transportRequest(endpoint,c);
 const read=async(path,options={})=>{
  const signal=AbortSignal.timeout(c.timeoutMs),r=await request(endpoint.url+path,{...options,redirect:'error',signal,headers:{Accept:'application/json',...options.headers}});
  if(r.redirected)throw Error('Redirected monitor response rejected');
  if(!r.ok)throw Error('HTTP '+r.status);const chunks=[];let bytes=0;
  for await(const chunk of r.body){bytes+=chunk.length;if(bytes>c.maximumResponseBytes)throw Error('Response budget exceeded');chunks.push(chunk);}
  return JSON.parse(Buffer.concat(chunks).toString('utf8'));
 };
 const rpc=async(method,params=[])=>{const r=await read('',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method,params})});if(!r||typeof r!=='object'||Array.isArray(r)||Object.hasOwn(r,'error')||r.id!==1||r.jsonrpc!=='2.0'||!Object.hasOwn(r,'result'))throw Error('RPC response rejected');return r.result;};
 try{
  const chain=await rpc('eth_chainId'),genesis=await rpc('eth_getBlockByNumber',['0x0',false]);
  if(integer(chain).toString()!==c.chainId||genesis?.hash!==c.genesisHash)throw Error('Chain/genesis mismatch');
  if((await read('/eth/v1/config/spec')).data?.CONFIG_NAME!==c.configName)throw Error('Profile mismatch');
  const h=(await read('/eth/v1/beacon/headers/head')).data;
  if(!root(h?.root)||!root(h.header?.message?.state_root))throw Error('Missing pinned Beacon state');
  const f=(await read('/eth/v1/beacon/states/'+h.header.message.state_root+'/finality_checkpoints')).data?.finalized;
  const count=integer(await rpc('net_peerCount'));if(count>BigInt(Number.MAX_SAFE_INTEGER))throw Error('Peer count overflow');
  return {name:node.name,epoch:f?.epoch,root:f?.root,headRoot:h.root,stateRoot:h.header.message.state_root,peers:Number(count)};
 }catch(error){return {name:node.name,error:String(error.message).slice(0,300)};}
}
export async function run(configFile,stateFile){
 const lockFile=stateFile+'.lock',lock=await open(lockFile,'wx',0o600);
 try{
  const config=configuration(JSON.parse(await readFile(configFile,'utf8')));let prior;
  try{prior=JSON.parse(await readFile(stateFile,'utf8'));}catch(e){if(e.code!=='ENOENT')throw e;}
  if(prior?.pendingDelivery)throw Error('Pending delivery requires monitor-and-deliver recovery');
  const samples=await Promise.all(config.nodes.map(n=>sample(config,n)));const result=evaluate(config,samples,Date.now(),prior);
  await durableJson(stateFile,result.state);
  console.log(JSON.stringify(result.report));return result.report.healthy?0:2;
 }finally{await lock.close();await unlink(lockFile);}
}

if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 if(process.argv.length!==4){console.error('Usage: finality-monitor.mjs config.json state.json');process.exitCode=1;}
 else try{process.exitCode=await run(process.argv[2],process.argv[3]);}catch(e){console.error(e.message);process.exitCode=1;}
}
