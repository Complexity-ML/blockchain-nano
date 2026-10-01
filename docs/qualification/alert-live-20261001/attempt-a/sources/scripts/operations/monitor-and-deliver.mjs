#!/usr/bin/env node
// One finite read-only observation and durable handoff; never controls validators.
import {open,readFile,unlink,realpath} from 'node:fs/promises';
import {dirname,basename,join,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {configuration,evaluate,sample,monitorIdentity} from './finality-monitor.mjs';
import {configuration as deliveryConfiguration,deliver} from './alert-delivery.mjs';
import {durableJson} from './durable-json.mjs';
export async function cycle(configFile,deliveryFile,stateFile,queueFile,{send=deliver}={}){
 const canonical=async file=>{try{return await realpath(file);}catch(e){if(e.code!=='ENOENT')throw e;return join(await realpath(dirname(resolve(file))),basename(file));}};
 [stateFile,queueFile]=await Promise.all([canonical(stateFile),canonical(queueFile)]);
 if(stateFile===queueFile||stateFile+'.lock'===queueFile||queueFile+'.lock'===stateFile)throw Error('Monitor and queue paths overlap');
 const config=configuration(JSON.parse(await readFile(configFile,'utf8'))),delivery=deliveryConfiguration(JSON.parse(await readFile(deliveryFile,'utf8')));
 if(delivery.monitorIdentity!==monitorIdentity(config))throw Error('Delivery identity differs from reviewed monitor');
 const lockFile=stateFile+'.lock',lock=await open(lockFile,'wx',0o600);
 try{
  let state;try{state=JSON.parse(await readFile(stateFile,'utf8'));}catch(e){if(e.code!=='ENOENT')throw e;}
  if(state)evaluate(config,config.nodes.map(n=>({name:n.name,error:'state-validation'})),state.time,state);
  let retried=false;
  if(state?.pendingDelivery){
   // Queue commit precedes HTTP; failures before this handoff keep the outbox.
   await send(delivery,state.pendingDelivery,queueFile);delete state.pendingDelivery;await durableJson(stateFile,state);retried=true;
  }
  const samples=await Promise.all(config.nodes.map(n=>sample(config,n)));
  const result=evaluate(config,samples,Date.now(),state);
  state={...result.state,pendingDelivery:result.report};await durableJson(stateFile,state);
  const deliveryResult=await send(delivery,result.report,queueFile);
  delete state.pendingDelivery;await durableJson(stateFile,state);
  return {report:result.report,delivery:deliveryResult,retried};
 }finally{await lock.close();await unlink(lockFile);}
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 try{if(process.argv.length!==6)throw Error('Usage: monitor-and-deliver.mjs monitor.json delivery.json state.json queue.json');const r=await cycle(...process.argv.slice(2));console.log(JSON.stringify(r));process.exitCode=r.report.healthy&&!r.delivery.deliveryFailed?0:2;}catch(e){console.error(e.message);process.exitCode=1;}
}
