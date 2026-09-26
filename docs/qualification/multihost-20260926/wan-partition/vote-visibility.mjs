import {readFile} from 'node:fs/promises';
import {join} from 'node:path';
const dir=process.argv[2];if(!dir)throw Error('Usage: node vote-visibility.mjs campaign-directory');
const names=['native','docker-mac','vps'],nodes=[];
for(const name of names){
 const statuses=(await readFile(join(dir,name+'.log'),'utf8')).split('\n').flatMap(line=>{try{const d=JSON.parse(line);return d.event==='bellatrix-runtime-stopped'?[d]:[];}catch{return [];}});
 if(statuses.length!==1)throw Error('Expected one terminal status: '+name);
 nodes.push(new Map(statuses[0].validator.dutyTrace.map(t=>[t.slot,t])));
}
const rows=[];
for(const slot of [...nodes[0].keys()].sort((a,b)=>a-b)){
 if(slot<48||!nodes.every(n=>n.has(slot)))continue;
 const traces=nodes.map(n=>n.get(slot)),epoch=Math.floor(slot/8);
 rows.push({slot,distinctSignedHeads:new Set(traces.map(t=>t.signedHead)).size,distinctSignedTargets:new Set(traces.map(t=>t.signedTarget.root)).size,nodes:traces.map((t,i)=>({name:names[i],observedTimeMs:t.observedTimeMs,previousEpochMessages:t.latestMessages.filter(m=>m.epoch===epoch-1).length,currentEpochMessages:t.latestMessages.filter(m=>m.epoch===epoch).length}))});
}
console.log(JSON.stringify({scope:'Diagnostic of sampled signing traces only; fixed 8-slot epochs from this 64-validator fixture. Samples are not simultaneous, complete vote counts, weighted quorum, or independent fork-choice verification.',rows},null,2));
