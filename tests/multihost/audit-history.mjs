import {readFile} from 'node:fs/promises';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';

// This is a consistency audit of recorded observations, not a new consensus
// proof or a replacement for execution binding and liveness qualifications.
export function auditHistory(names,run,observations){
 if(!Array.isArray(names)||names.length!==3||new Set(names).size!==3)throw Error('Expected three unique node names');
 const roots=new Map(),previous=new Map(),after=new Set();let checked=0,baseline=false;
 const ingest=(name,checkpoint)=>{
  if(!names.includes(name))throw Error('Unknown observer');
  if(!checkpoint||typeof checkpoint.epoch!=='string'||!/^\d+$/.test(checkpoint.epoch)||!/^0x[0-9a-f]{64}$/.test(checkpoint.root))throw Error('Malformed checkpoint');
  const epoch=BigInt(checkpoint.epoch),key=String(epoch);
  if(previous.has(name)&&epoch<previous.get(name))throw Error('Finality regression across phases');
  if(roots.has(key)&&roots.get(key)!==checkpoint.root)throw Error('Conflicting finalized roots across phases');
  roots.set(key,checkpoint.root);previous.set(name,epoch);++checked;
 };
 for(const event of run){
  if(event.event==='startup-finality-bound'){
   for(const name of names)ingest(name,event.checkpoint);baseline=true;
  }
  if(event.event==='partition-observation'){
   if(!baseline||!Array.isArray(event.rows)||event.rows.length!==3||new Set(event.rows.map(r=>r.name)).size!==3)throw Error('Incomplete partition observation');
   for(const row of event.rows)ingest(row.name,row.checkpoint);
  }
 }
 if(!baseline)throw Error('Missing startup baseline');
 for(const event of observations){
  if(!['finality-observation','head-changed-during-finality-read'].includes(event.event))continue;
  const name=new URL(event.endpoint).hostname;ingest(name,event.checkpoint);
  if(event.event==='finality-observation')after.add(name);
 }
 if(after.size!==names.length)throw Error('Missing post-partition observer');
 return {scope:'Recorded finality consistency across startup, partition and subsequent observations, including raced reads. No liveness, ancestor proof, execution binding or production claim.',checked,epochs:roots.size,lastEpochs:Object.fromEntries([...previous].map(([n,e])=>[n,String(e)]))};
}
if(process.argv[1]===fileURLToPath(import.meta.url)){
 const directory=process.argv[2];if(!directory)throw Error('Usage: audit-history.mjs campaign-directory');
 const lines=async path=>(await readFile(path,'utf8')).split('\n').filter(Boolean).map(line=>JSON.parse(line));
 const config=JSON.parse(await readFile(join(directory,'observers.json'),'utf8'));
 console.log(JSON.stringify(auditHistory(config.nodes.map(n=>n.name),await lines(join(directory,'run.jsonl')),await lines(join(directory,'observations','samples.jsonl'))),null,2));
}
