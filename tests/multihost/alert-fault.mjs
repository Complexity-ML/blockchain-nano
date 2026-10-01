import {createServer} from 'node:http';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdir,writeFile,appendFile,readFile} from 'node:fs/promises';
import {join} from 'node:path';
import {setTimeout as delay} from 'node:timers/promises';
import {monitorConfiguration} from './monitor.mjs';
import {monitorIdentity} from '../../scripts/operations/finality-monitor.mjs';
const exec=promisify(execFile);
export async function qualifyAlertFault({repo,output,prepared,local,remote,target,record,docker}){
 const directory=join(output,'alert-fault');await mkdir(directory);
 const config=await monitorConfiguration({prepared,local,remote,target});
 const configFile=join(directory,'monitor.json'),deliveryFile=join(directory,'delivery.json'),stateFile=join(directory,'state.json'),queueFile=join(directory,'queue.json');
 await writeFile(configFile,JSON.stringify(config,null,2)+'\n');
 const attempts=[],accepted=new Map();
 const receiver=createServer(async(req,res)=>{
  try{let text='';for await(const chunk of req){text+=chunk;if(text.length>1048576)throw Error('Receiver budget');}const event=JSON.parse(text);if(req.headers['idempotency-key']!==event.eventId||event.monitorIdentity!==monitorIdentity(config))throw Error('Unbound event');
   attempts.push(event);const status=attempts.length===1?503:200;if(status===200)accepted.set(event.eventId,event);
   await appendFile(join(directory,'receiver.jsonl'),JSON.stringify({status,event})+'\n');res.writeHead(status);res.end('recorded');
  }catch{res.writeHead(400);res.end('rejected');}
 });
 await new Promise(r=>receiver.listen(0,'127.0.0.1',r));let paused=false;
 const invoke=async()=>{
  let code=0,stdout;try{stdout=(await exec(process.execPath,[join(repo,'scripts/operations/monitor-and-deliver.mjs'),configFile,deliveryFile,stateFile,queueFile],{timeout:45000,maxBuffer:2*1024*1024})).stdout;}catch(e){if(e.code!==2||!e.stdout)throw e;code=2;stdout=e.stdout;}
  const result={code,...JSON.parse(stdout)};await appendFile(join(directory,'samples.jsonl'),JSON.stringify(result)+'\n');return result;
 };
 try{
  await writeFile(deliveryFile,JSON.stringify({monitorIdentity:monitorIdentity(config),url:'http://127.0.0.1:'+receiver.address().port+'/',timeoutMs:5000,maximumResponseBytes:4096,maximumPending:64},null,2)+'\n');
  const initial=await invoke();if(initial.code||!initial.report.healthy||attempts.length)throw Error('Alert baseline not healthy/quiet');
  const before=JSON.parse(await docker(['inspect',remote,'--format','{{json .State}}']));if(!before.Running||before.Paused)throw Error('Fault target not running');
  await docker(['pause',remote]);paused=true;const stopped=JSON.parse(await docker(['inspect',remote,'--format','{{json .State}}']));if(!stopped.Running||!stopped.Paused)throw Error('Target not paused');
  await record({event:'alert-fault-daemon-paused',container:remote});
  const fault=await invoke();if(fault.code!==2||!fault.report.alerts.some(a=>a.code==='unavailable'&&a.node==='vps-docker')||!fault.delivery.deliveryFailed||attempts.length!==1)throw Error('Live daemon fault/receiver rejection not observed');
  const pending=JSON.parse(await readFile(queueFile,'utf8'));if(pending.pending.length!==1||pending.pending[0].eventId!==attempts[0].eventId)throw Error('Rejected alert lost');
  const retry=await invoke();if(retry.delivery.deliveryFailed||retry.delivery.pending||attempts.length<2||attempts[1].eventId!==attempts[0].eventId||!accepted.has(attempts[0].eventId))throw Error('Retry identity/acceptance failed');
  await docker(['unpause',remote]);paused=false;await record({event:'alert-fault-daemon-resumed',container:remote});
  const deadline=Date.now()+60000;let recovered;
  while(Date.now()<deadline){recovered=await invoke();if(recovered.code===0&&recovered.report.healthy)break;await delay(1000);}
  if(!recovered?.report.healthy||recovered.delivery.deliveryFailed)throw Error('Live monitor recovery deadline exceeded');
  const notifications=[...accepted.values()];if(!notifications.some(e=>!e.healthy&&e.alerts.some(a=>a.code==='unavailable'&&a.node==='vps-docker'))||notifications.at(-1)?.healthy!==true)throw Error('Fault and recovery not received');
  const result={qualified:true,productionReady:false,fault:'Docker pause of live VPS Nano/P2P process tree',receiver:'Local HTTP fixture with initial 503 and logical idempotency',attempts:attempts.length,accepted:notifications.length,sameRetryId:true,liveFaultReceived:true,liveRecoveryReceived:true,sharedFinalityGate:'required separately after fault'};
  await writeFile(join(directory,'result.json'),JSON.stringify(result,null,2)+'\n');await record({event:'alert-fault-passed',...result});
 }finally{try{if(paused)await docker(['unpause',remote]);}finally{receiver.closeAllConnections();await new Promise(r=>receiver.close(r));}}
}
