#!/usr/bin/env node
// The relay logs only slow or unsuccessful attempts: never treat this as all traffic.
import {readFile} from 'node:fs/promises';
import {join} from 'node:path';
const [directory]=process.argv.slice(2);
if(!directory)throw Error('Usage: relay-summary.mjs campaign-directory');
const nodes=[];
for(const file of ['native.log','docker-mac.log','vps.log']){
 const groups=new Map();let attempts=0,withoutStageTiming=0;
 for(const line of (await readFile(join(directory,file),'utf8')).split('\n')){
  let entry;try{entry=JSON.parse(line);}catch{continue;}
  if(entry.event!=='test-consensus-relay-attempt')continue;
  ++attempts;
  if(!entry.stageUs){++withoutStageTiming;continue;}
  if(typeof entry.success!=='boolean'||!Number.isFinite(entry.elapsedMs)||entry.elapsedMs<0)throw Error('Invalid relay result');
  const key=`${entry.kind}:${entry.success?'successful':'unsuccessful'}`;
  if(!groups.has(key))groups.set(key,{kind:entry.kind,success:entry.success,count:0,totalMs:0,maxMs:0,stages:{}});
  const group=groups.get(key);++group.count;group.totalMs+=entry.elapsedMs;group.maxMs=Math.max(group.maxMs,entry.elapsedMs);
  for(const [stage,us] of Object.entries(entry.stageUs)){
   if(!['inventory','payload','ack','known'].includes(stage)||!Number.isSafeInteger(us)||us<0)throw Error('Invalid stage timing');
   const value=group.stages[stage]??={count:0,totalUs:0,maxUs:0};++value.count;value.totalUs+=us;value.maxUs=Math.max(value.maxUs,us);
  }
 }
 nodes.push({file,attempts,withoutStageTiming,groups:[...groups.values()]});
}
console.log(JSON.stringify({scope:'Slow (>500ms) or unsuccessful relay attempts only. Stage ack includes stream setup, transfer and remote admission; not network latency. Success may be a cache scan, not a new delivery. No throughput or population percentile claim.',nodes},null,2));
