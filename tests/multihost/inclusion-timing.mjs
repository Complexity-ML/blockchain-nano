#!/usr/bin/env node
// Offline diagnostic over an admitted Bellatrix archive, not a consensus verifier.
import {readFile} from 'node:fs/promises';
import {gunzipSync} from 'node:zlib';
import {createHash} from 'node:crypto';
import {join} from 'node:path';
const [checkpointPath,directory]=process.argv.slice(2);
if(!checkpointPath||!directory)throw Error('Usage: inclusion-timing.mjs checkpoint.json[.gz] campaign-directory');
const raw=await readFile(checkpointPath),p=JSON.parse(checkpointPath.endsWith('.gz')?gunzipSync(raw):raw);
const blocks=new Map(p.store.blocks.map(r=>[r[0],r])),ancestors=new Set();let root=p.store.finalized[1];
while(blocks.has(root)&&!ancestors.has(root)){ancestors.add(root);root=blocks.get(root)[1][2];}
const genesisMs=BigInt(p.store.clock[0]),slotMs=BigInt(p.store.profile[1]);
if(slotMs<=0n)throw Error('Invalid slot duration');
const relays=new Map();let undated=0,dated=0;
for(const file of ['native.log','docker-mac.log','vps.log']){
 for(const line of (await readFile(join(directory,file),'utf8')).split('\n')){
  let e;try{e=JSON.parse(line);}catch{continue;}
  // "known" may be a cached scan, and is not evidence of a new transfer.
  if(e.event!=='test-altair-vote-relayed')continue;
  if(e.timeMs===undefined){++undated;continue;}
  if(!Number.isSafeInteger(e.timeMs)||e.timeMs<0||!/^0x[0-9a-f]{64}$/.test(e.root)||typeof e.peerId!=='string')throw Error('Malformed timed relay');
  ++dated;const list=relays.get(e.root)??[];list.push({file,peerId:e.peerId,timeMs:e.timeMs});relays.set(e.root,list);
 }
}
const rows=[];
for(const [root,wire] of p.signedBlocks){
 if(!ancestors.has(root))continue;
 const record=blocks.get(root),slot=BigInt(record[1][0]);
 const b=Buffer.from(wire.slice(2),'hex');
 const u32=at=>b.readUInt32LE(at),message=u32(0),body=message+u32(message+80);
 const start=body+u32(body+208),end=body+u32(body+212);
 if(start<body||end<start||end>b.length)throw Error('Invalid attestation range');
 if(start===end)continue;
 const first=u32(start);if(!first||first%4||first>end-start)throw Error('Invalid attestation list');
 for(let i=0;i<first/4;i++){
  const from=start+u32(start+i*4),to=i+1<first/4?start+u32(start+(i+1)*4):end;
  if(from<start+first||to<from||to>end||to-from<228)throw Error('Invalid attestation bounds');
  const vote=b.subarray(from,to),voteSlot=vote.readBigUInt64LE(4);
  if(voteSlot>=slot)throw Error('Invalid inclusion delay');
  const id='0x'+createHash('sha256').update(vote).digest('hex');
  const opening=genesisMs+slot*slotMs;
  rows.push({blockRoot:root,slot:String(slot),voteSlot:String(voteSlot),delaySlots:String(slot-voteSlot),voteRoot:id,
   relayCompletions:(relays.get(id)??[]).map(e=>({...e,msRelativeToInclusionSlotOpening:String(BigInt(e.timeMs)-opening)}))});
 }
}
rows.sort((a,b)=>Number(BigInt(a.slot)-BigInt(b.slot))||a.voteRoot.localeCompare(b.voteRoot));
console.log(JSON.stringify({scope:'Recorded relay completion times matched by exact attestation bytes on the native finalized ancestry. Not arrival or proposer availability proof; no signature/state-root revalidation. Wall clocks may differ. Missing records do not prove missing delivery. Known/cache scans excluded.',datedRelayEvents:dated,undatedRelayEvents:undated,inclusions:rows.length,matchedInclusions:rows.filter(r=>r.relayCompletions.length).length,rows},null,2));
