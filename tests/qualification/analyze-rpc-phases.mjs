import {readFile,readdir} from 'node:fs/promises';
import {resolve} from 'node:path';
const reports=[];
for(const directory of process.argv.slice(2)){
 const requests=[],issues=[];
 const files=(await readdir(directory)).filter(name=>/^rpc-\d+\.jsonl$/.test(name));
 if(files.length!==4)issues.push(`Expected four node traces, got ${files.length}`);
 for(const file of files){
  const groups=new Map();
  for(const line of (await readFile(resolve(directory,file),'utf8')).trim().split('\n')){
   const row=JSON.parse(line);
   if(!['request','phase','startNs','durationMs'].every(key=>Object.hasOwn(row,key))||Object.keys(row).length!==4||!Number.isFinite(row.durationMs)||row.durationMs<0)issues.push(`Invalid timing record: ${file}`);
   const group=groups.get(row.request)??{};
   if(group[row.phase])issues.push(`Duplicate phase ${file}/${row.request}/${row.phase}`);
   group[row.phase]=row;groups.set(row.request,group);
  }
  if(groups.size!==144)issues.push(`Expected 144 queued simulations in ${file}, got ${groups.size}`);
  for(const [id,phases] of groups){
   const queue=phases.websocket_queue??phases.http_queue;
   const execution=phases.estimate_execute??phases.call_execute;
   if(!phases.rpc_lock||!phases.prepare||!execution||Object.keys(phases).length!==5)issues.push(`Incomplete or unexpected simulation phases ${file}/${id}`);
   if(!queue||!phases.worker){issues.push(`Incomplete worker trace ${file}/${id}`);continue;}
   requests.push({file,id,method:phases.estimate_execute?'eth_estimateGas':phases.call_execute?'eth_call':'cancelled-before-execution',queueMs:queue.durationMs,lockMs:phases.rpc_lock?.durationMs??null,prepareMs:phases.prepare?.durationMs??null,executeMs:execution?.durationMs??null,workerMs:phases.worker.durationMs,totalMs:queue.durationMs+phases.worker.durationMs});
  }
 }
 if(requests.length!==576)issues.push(`Expected 576 simulations, got ${requests.length}`);
 reports.push({directory,files:files.length,requests:requests.length,issues,verified:issues.length===0,maxima:Object.fromEntries(['queueMs','lockMs','prepareMs','executeMs','workerMs','totalMs'].map(key=>[key,requests.length?Math.max(...requests.map(r=>r[key]??0)):null])),slowest:requests.sort((a,b)=>b.totalMs-a.totalMs).slice(0,8)});
}
console.log(JSON.stringify({reports,verified:reports.length>0&&reports.every(r=>r.verified)},null,2));
if(!reports.length||reports.some(r=>!r.verified))process.exitCode=1;
