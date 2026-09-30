// Bounded evidence collection, not a consensus verifier. Signed SSZ must also
// pass rotation_audit.cpp against each stopped store and its recovery witness.
import {setTimeout as delay} from 'node:timers/promises';
export const rotationNodes=['mac-native','mac-docker','vps-docker'];
const rootPattern=/^0x[0-9a-f]{64}$/;
export function ownership(manifest) {
 if(manifest.fork!=='capella'||manifest.syntheticValidators!==64||manifest.physicalHosts!==2||manifest.nodes?.length!==3)throw Error('Unexpected rotation topology');
 const owners=Array(64).fill(null);
 manifest.nodes.forEach((node,owner)=>{
  if(node.name!==rotationNodes[owner]||!node.indices?.length)throw Error('Unexpected rotation owner');
  for(const raw of node.indices){const index=Number(raw);if(!Number.isInteger(index)||index<0||index>=64||owners[index]!==null)throw Error('Duplicate or invalid validator index');owners[index]=owner;}
 });
 if(owners.some(x=>x===null))throw Error('Missing validator ownership');return owners;
}
export function createRotationCapture({maxBlocks=256,maxBytes=24*1024*1024}={}) {
 const blocks=new Map();let bytes=0;
 return {
  add(name,root,wire){
   if(!rotationNodes.includes(name)||!rootPattern.test(root)||!Buffer.isBuffer(wire)||wire.length<184||wire.length>8*1024*1024)throw Error('Invalid block capture');
   if(wire.readUInt32LE(0)!==100||wire.readUInt32LE(180)!==84)throw Error('Noncanonical captured SSZ');
   const previous=blocks.get(root);
   if(previous){if(!previous.wire.equals(wire))throw Error('Nodes disagree on signed SSZ');previous.observers.add(name);return;}
   if(blocks.size>=maxBlocks||bytes+wire.length>maxBytes)throw Error('Rotation capture budget exceeded');
   blocks.set(root,{wire:Buffer.from(wire),observers:new Set([name])});bytes+=wire.length;
  },
  has(name,root){return blocks.get(root)?.observers.has(name)??false;},
  parent(root){const b=blocks.get(root);if(!b)throw Error('Missing captured ancestor');return '0x'+b.wire.subarray(116,148).toString('hex');},
  proof(finalizedRoot,minimumInclusionSlot){
   if(!rootPattern.test(finalizedRoot)||!Number.isSafeInteger(minimumInclusionSlot)||minimumInclusionSlot<128)throw Error('Invalid rotation cutoff');
   const result={};let tip=finalizedRoot,postRecovery=0,previousSlot=Infinity;
   for(let depth=0;depth<64;++depth){
    const b=blocks.get(tip);if(!b||b.observers.size!==3)throw Error('Missing three-node ancestry capture');
    const slot=Number(b.wire.readBigUInt64LE(100));if(!Number.isSafeInteger(slot)||slot>=previousSlot)throw Error('Nondecreasing captured ancestry');previousSlot=slot;
    result[tip]='0x'+b.wire.toString('hex');
    if(slot<=minimumInclusionSlot){if(postRecovery<3)throw Error('Insufficient post-recovery blocks');return {finalizedRoot,minimumInclusionSlot,blocks:result};}
    ++postRecovery;tip=this.parent(tip);
   }
   throw Error('Rotation ancestry budget exceeded');
  },
  dump(){return Object.fromEntries([...blocks].map(([root,b])=>[root,{ssz:'0x'+b.wire.toString('hex'),observers:[...b.observers]}]));},
  stats(){return {blocks:blocks.size,bytes};},
 };
}
// Never move the fault to a later period when the intended boundary is missed.
export async function waitRotationWindow({sample,record,clock=Date.now,wait=delay,boundary=128,lead=8,timeoutMs=720000}) {
 const deadline=clock()+timeoutMs;
 while(clock()<deadline){
  const rows=await sample();
  if(rows.length!==3||rows.some((r,i)=>r.name!==rotationNodes[i]||!rootPattern.test(r.root)||!Number.isSafeInteger(r.slot)||r.slot<0))throw Error('Invalid rotation heads');
  await record({event:'rotation-wait',rows,boundary});
  if(rows.some(r=>r.slot>=boundary))throw Error('Rotation fault window missed');
  if(rows.every(r=>r.slot>=boundary-lead&&r.root===rows[0].root))return rows;
  await wait(3000);
 }
 throw Error('Rotation start deadline exceeded');
}
