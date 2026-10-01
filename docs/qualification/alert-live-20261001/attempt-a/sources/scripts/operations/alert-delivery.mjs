#!/usr/bin/env node
// Delivery only to an explicitly configured receiver; never signs or controls nodes.
import {createHash} from 'node:crypto';
import {readFile,open,unlink} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
import {durableJson} from './durable-json.mjs';
const digest=v=>createHash('sha256').update(JSON.stringify(v)).digest('hex');
const hash=s=>typeof s==='string'&&/^[0-9a-f]{64}$/.test(s);
export function configuration(c){
 if(!c||!hash(c.monitorIdentity))throw Error('Pinned monitor identity required');
 const u=new URL(c.url);if(u.username||u.password||u.hash||!(u.protocol==='https:'||(u.protocol==='http:'&&u.hostname==='127.0.0.1')))throw Error('HTTPS or local test receiver required');
 for(const k of ['timeoutMs','maximumResponseBytes','maximumPending'])if(!Number.isSafeInteger(c[k])||c[k]<=0)throw Error('Positive '+k+' required');
 return {monitorIdentity:c.monitorIdentity,url:u.href,timeoutMs:c.timeoutMs,maximumResponseBytes:c.maximumResponseBytes,maximumPending:c.maximumPending};
}
function payload(c,r){
 if(r.identity!==c.monitorIdentity||!Number.isSafeInteger(r.time)||r.time<0||typeof r.healthy!=='boolean'||typeof r.commonFinality!=='boolean')throw Error('Unbound/invalid monitor report');
 const clean=values=>{if(!Array.isArray(values))throw Error('Invalid faults');const unique=new Map();for(const a of values){if(!/^[a-z0-9-]{1,64}$/.test(a?.code)||!/^[a-z0-9-]{1,64}$/.test(a?.node))throw Error('Invalid fault identity');unique.set(a.code+':'+a.node,{code:a.code,node:a.node});}return [...unique.values()].sort((a,b)=>(a.code+':'+a.node).localeCompare(b.code+':'+b.node));};
 const out={version:1,monitorIdentity:c.monitorIdentity,observedAt:r.time,healthy:r.healthy,commonFinality:r.commonFinality,alerts:clean(r.alerts),safetyFaults:clean(r.safetyFaults)};
 if(out.healthy&&(out.alerts.length||out.safetyFaults.length))throw Error('Healthy report contains faults');return out;
}
const eventId=e=>digest({sequence:e.sequence,payload:e.payload});
function validatedState(c,prior){
 const identity=digest(c);
 const s=prior?structuredClone(prior):{version:1,identity,sequence:0,lastFingerprint:null,lastObservedAt:0,pending:[]};
 if(s.version!==1||s.identity!==identity||!Number.isSafeInteger(s.sequence)||s.sequence<0||!Number.isSafeInteger(s.lastObservedAt)||s.lastObservedAt<0||!(s.lastFingerprint===null||hash(s.lastFingerprint))||!Array.isArray(s.pending)||s.pending.length>c.maximumPending)throw Error('Alert state/clock mismatch');
 let seq=0;for(const e of s.pending){if(!Number.isSafeInteger(e.sequence)||e.sequence<=seq||e.sequence>s.sequence||!hash(e.eventId)||e.eventId!==eventId(e))throw Error('Corrupt alert queue');const checked=payload(c,{identity:e.payload.monitorIdentity,time:e.payload.observedAt,...e.payload});if(JSON.stringify(checked)!==JSON.stringify(e.payload))throw Error('Invalid queued payload');seq=e.sequence;}
 return s;
}
export function enqueue(config,report,prior){
 const c=configuration(config),p=payload(c,report),s=validatedState(c,prior);
 if(p.observedAt<s.lastObservedAt)throw Error('Alert state/clock mismatch');
 // Normal progress/asynchronous common checkpoints must not cause notification spam.
 const fingerprint=digest({healthy:p.healthy,alerts:p.alerts,safetyFaults:p.safetyFaults});
 if(fingerprint!==s.lastFingerprint&&(!p.healthy||s.lastFingerprint!==null)){
  if(s.pending.length>=c.maximumPending||s.sequence===Number.MAX_SAFE_INTEGER)throw Error('Alert queue capacity');
  const e={sequence:++s.sequence,payload:p};e.eventId=eventId(e);s.pending.push(e);
 }
 s.lastFingerprint=fingerprint;s.lastObservedAt=p.observedAt;return s;
}
export async function deliver(config,report,file,{request=fetch,persist=durableJson}={}){
 const c=configuration(config),lockFile=file+'.lock',lock=await open(lockFile,'wx',0o600);
 try{
  let prior;try{prior=JSON.parse(await readFile(file,'utf8'));}catch(e){if(e.code!=='ENOENT')throw e;}
  const state=report===null?validatedState(c,prior):enqueue(c,report,prior);await persist(file,state);let delivered=0;
  while(state.pending.length){
   const e=state.pending[0];let response;
   try{response=await request(c.url,{method:'POST',redirect:'error',signal:AbortSignal.timeout(c.timeoutMs),headers:{'Content-Type':'application/json','Idempotency-Key':e.eventId},body:JSON.stringify({...e.payload,eventId:e.eventId})});let size=0;for await(const chunk of response.body??[]){size+=chunk.length;if(size>c.maximumResponseBytes)throw Error('Receiver response budget');}if(!response.ok)throw Error('Receiver status');}
   catch{await response?.body?.cancel().catch(()=>{});return {delivered,pending:state.pending.length,deliveryFailed:true};}
   // If this commit fails after acceptance, restart resends the SAME identifier.
   state.pending.shift();await persist(file,state);++delivered;
  }
  return {delivered,pending:0,deliveryFailed:false};
 }finally{await lock.close();await unlink(lockFile);}
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 try{if(process.argv.length!==5)throw Error('Usage: alert-delivery.mjs config.json report.json|--drain queue.json');const r=await deliver(JSON.parse(await readFile(process.argv[2],'utf8')),process.argv[3]==='--drain'?null:JSON.parse(await readFile(process.argv[3],'utf8')),process.argv[4]);console.log(JSON.stringify(r));process.exitCode=r.deliveryFailed?2:0;}catch(e){console.error(e.message);process.exitCode=1;}
}
