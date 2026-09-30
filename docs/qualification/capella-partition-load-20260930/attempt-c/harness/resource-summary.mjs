#!/usr/bin/env node
// Descriptive sampled measurements only; no production capacity verdict.
import {readFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
const hosts=['mac-native','mac-docker','vps-docker'];
const units={B:1,kB:1e3,KB:1e3,MB:1e6,GB:1e9,TB:1e12,KiB:1024,MiB:1024**2,GiB:1024**3,TiB:1024**4};
export function bytes(text){
 const m=/^(\d+(?:\.\d+)?)\s*(B|kB|KB|MB|GB|TB|KiB|MiB|GiB|TiB)$/.exec(text);
 if(!m)throw Error('Unknown resource byte quantity');const n=Number(m[1])*units[m[2]];
 if(!Number.isFinite(n)||n>Number.MAX_SAFE_INTEGER)throw Error('Resource quantity out of range');return Math.round(n);
}
function pair(text){const parts=text.split(' / ');if(parts.length!==2)throw Error('Invalid resource pair');return parts.map(bytes);}
function percent(text){if(!/^\d+(?:\.\d+)?%$/.test(text))throw Error('Invalid CPU percentage');const n=Number(text.slice(0,-1));if(!Number.isFinite(n))throw Error('CPU percentage out of range');return n;}
export function summarize(records,binding){
 if(!records.length||!Number.isSafeInteger(binding.nativePid)||!binding.directory?.startsWith('/'))throw Error('Missing resource provenance');
 const nodes=Object.fromEntries(hosts.map(host=>[host,{samples:0,unavailable:[],identity:null,firstTimeMs:null,lastTimeMs:null,maximumSampleGapMs:0,maxObservedMemoryBytes:0,maxObservedCpuPercent:0}]));
 let previous=-1;
 for(const row of records){
  if(!Number.isSafeInteger(row.timeMs)||row.timeMs<=previous||!Array.isArray(row.samples)||row.samples.length!==3||new Set(row.samples.map(s=>s.host)).size!==3)throw Error('Invalid resource timeline');previous=row.timeMs;
  for(const s of row.samples){
   const n=nodes[s.host];if(!n)throw Error('Unknown resource host');
   if(s.exitCode!==0||s.error){n.unavailable.push({timeMs:row.timeMs,error:s.error??s.stderr??'command failed',exitCode:s.exitCode??null});continue;}
   let memory,cpu,id;
   if(s.host==='mac-native'){
    const m=/^\s*(\d+)\s+(\d+)\s+([\d.]+)\s+(\S+)\s+(.+)$/.exec(s.stdout.trimEnd());
    if(!m||Number(m[1])!==binding.nativePid||!m[5].includes(binding.directory+'/prepared/mac-native/'))throw Error('Native resource identity mismatch');
    id=String(binding.nativePid);memory=Number(m[2])*1024;cpu=Number(m[3]);
    if(!Number.isSafeInteger(memory)||!Number.isFinite(cpu))throw Error('Invalid native resource quantity');
    n.measurement='ps RSS and %CPU for the main process only, excluding its P2P child; sampling differs from Docker';
   }else{
    const d=JSON.parse(s.stdout),expected=s.host==='mac-docker'?binding.localContainer:binding.remoteContainer;
    if(!/^nano-qualification-(mac|vps)-[a-z0-9]+$/.test(expected??'')||d.Name!==expected||!d.ID)throw Error('Container resource identity mismatch');
    id=d.ID;const [used,limit]=pair(d.MemUsage);memory=used;cpu=percent(d.CPUPerc);
    if(limit===0){n.unavailable.push({timeMs:row.timeMs,error:'Docker stats inactive or unavailable (zero limit)'});continue;}
    if(n.memoryLimitBytes!==undefined&&n.memoryLimitBytes!==limit)throw Error('Container memory limit changed');n.memoryLimitBytes=limit;
    const [read,written]=pair(d.BlockIO);n.lastBlockIoBytes={read,written};
    n.measurement='docker stats memory accounting and sampled CPU; BlockIO is cumulative I/O, not disk usage';
   }
   if(n.identity!==null&&n.identity!==id)throw Error('Resource identity changed');n.identity=id;
   if(n.lastTimeMs!==null)n.maximumSampleGapMs=Math.max(n.maximumSampleGapMs,row.timeMs-n.lastTimeMs);
   n.firstTimeMs??=row.timeMs;n.lastTimeMs=row.timeMs;n.samples++;
   n.maxObservedMemoryBytes=Math.max(n.maxObservedMemoryBytes,memory);n.maxObservedCpuPercent=Math.max(n.maxObservedCpuPercent,cpu);
  }
 }
 if(Object.values(nodes).some(n=>!n.samples))throw Error('A host has no usable resource measurement');
 return {scope:'Sampled observations of this campaign only. Peaks between samples are unknown. No throughput, disk-occupancy, endurance or production-readiness claim.',collectionStartDelayMs:Number.isSafeInteger(binding.time)?records[0].timeMs-binding.time:null,firstTimeMs:records[0].timeMs,lastTimeMs:records.at(-1).timeMs,records:records.length,nodes};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(resolve(process.argv[1])).href){
 const [directory]=process.argv.slice(2);if(!directory)throw Error('Usage: resource-summary.mjs campaign-directory');
 const lines=async name=>(await readFile(resolve(directory,name),'utf8')).trim().split('\n').map(line=>JSON.parse(line));
 const start=(await lines('run.jsonl')).filter(r=>r.event==='started');if(start.length!==1)throw Error('Expected one campaign start');
 console.log(JSON.stringify(summarize(await lines('resources.jsonl'),{...start[0],directory:resolve(directory)}),null,2));
}
