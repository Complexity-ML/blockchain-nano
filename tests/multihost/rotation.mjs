// Bounded evidence collection, not a consensus verifier. Signed SSZ must also
// pass rotation_audit.cpp against each stopped store and its recovery witness.
import {setTimeout as delay} from 'node:timers/promises';
import {createHash} from 'node:crypto';
import {durableFinalityObservation} from '../capella/finality-observation.mjs';
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
  digest(root){const b=blocks.get(root);if(!b)throw Error('Missing public signed block');return {sha256:createHash('sha256').update(b.wire).digest('hex'),bytes:b.wire.length};},
  addDigest(name,root,digest,size){
   const b=blocks.get(root);if(name!=='vps-docker'||!b||!/^[0-9a-f]{64}$/.test(digest)||!Number.isSafeInteger(size)||size!==b.wire.length||createHash('sha256').update(b.wire).digest('hex')!==digest)throw Error('Remote in-situ signed-block digest differs');b.observers.add(name);
  },
  freshProof(initialRoot,finalizedRoot,minimumInclusionSlot){
   if(!rootPattern.test(initialRoot)||!rootPattern.test(finalizedRoot)||!Number.isSafeInteger(minimumInclusionSlot)||minimumInclusionSlot<0)throw Error('Invalid fresh rotation roots/cutoff');
   const result={};let tip=finalizedRoot,previousSlot=Infinity,postRecovery=0;
   for(let depth=0;tip!==initialRoot;++depth){
    if(depth>=maxBlocks)throw Error('Fresh rotation ancestry budget exceeded');const b=blocks.get(tip);if(!b||b.observers.size!==3)throw Error('Missing three-owner signed-block observation');
    const slot=Number(b.wire.readBigUInt64LE(100));if(!Number.isSafeInteger(slot)||slot>=previousSlot)throw Error('Nondecreasing fresh rotation ancestry');previousSlot=slot;if(slot>minimumInclusionSlot)postRecovery++;
    result[tip]={ssz:'0x'+b.wire.toString('hex'),observedBy:rotationNodes.filter(name=>b.observers.has(name))};tip=this.parent(tip);
   }
   if(postRecovery<3)throw Error('Insufficient finalized post-reconnection blocks');return result;
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

// Public-profile derivation for the explicit fresh mode. A missed boundary is
// fatal; no caller may slide the fault to a later period or shorten the preset.
export function freshRotationParameters(profileBytes){
 if(!Buffer.isBuffer(profileBytes)||profileBytes.length===0||profileBytes.length>1024*1024)throw Error('Bounded pinned public Capella profile required');
 const p=JSON.parse(profileBytes);if(p?.format!=='nano-capella-profile-v1'||!Array.isArray(p.common)||p.common.length<12)throw Error('Capella profile required');
 const quantity=value=>{if(typeof value!=='string'||!/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(value))throw Error('Canonical profile quantity required');const n=Number(BigInt(value));if(!Number.isSafeInteger(n)||n<=0)throw Error('Finite positive profile quantity required');return n;};
 const slotsPerEpoch=quantity(p.common[0]?.ffg?.[0]),epochsPerPeriod=quantity(p.common[0]?.epochSync?.[1]),slotMs=quantity(p.common[1]),secondsPerSlot=quantity(p.common[7]);
 if(quantity(p.common[0]?.epochSync?.[0])!==slotsPerEpoch||slotMs!==secondsPerSlot*1000)throw Error('Profile clock/epoch fields disagree');
 const periodSlots=slotsPerEpoch*epochsPerPeriod,boundarySlot=2*periodSlots,leadSlots=slotsPerEpoch,cutMs=(2*leadSlots+1)*slotMs;
 if(![periodSlots,boundarySlot,cutMs].every(Number.isSafeInteger))throw Error('Rotation arithmetic exceeds safe range');
 return {slotsPerEpoch,epochsPerPeriod,periodSlots,boundarySlot,leadSlots,slotMs,cutMs};
}
export function freshRotationOwnership(domain,prepared){
 if(domain?.format!=='nano-isolated-capella-fixture-domain-v1'||domain.synthetic!==true||domain.productionQualified!==false||domain.publicKeys?.length!==65||prepared?.nodes?.length!==3)throw Error('Fresh public ownership authority required');
 const seen=new Set(),result=Array(65);
 for(const [ordinal,node] of prepared.nodes.entries()){
  if(node.name!==rotationNodes[ordinal]||node.indices?.length!==node.signerIdentities?.length)throw Error('Ordered owner identities required');
  node.indices.forEach((index,position)=>{const pub=domain.publicKeys[index],key=node.signerIdentities[position];if(!Number.isInteger(index)||index<0||index>=65||seen.has(index)||pub?.index!==index||key?.keyEnv!=='NANO_TEST_CAPELLA_NETWORK_KEY_'+index||key.publicKey!==pub.publicKey||!/^0x[0-9a-f]{96}$/.test(key.publicKey))throw Error('Fresh owner public-key binding differs');seen.add(index);result[index]={publicKey:key.publicKey,keyEnv:key.keyEnv,owner:node.name};});
 }
 if(seen.size!==65)throw Error('Incomplete fresh public ownership');return result;
}
// This monitor validates genuine state/header pairs even when the current
// checkpoint equals the already completed initial baseline. It counts no
// advances; the existing finality observer remains the post-recovery gate.
export function freshPartitionFinalityObserver(initialObservation){
 const cp=x=>{if(!x||typeof x.epoch!=='string'||! /^(?:0|[1-9][0-9]*)$/.test(x.epoch)||!rootPattern.test(x.root))throw Error('Invalid genuine checkpoint');return x;};
 if(!Array.isArray(initialObservation?.knownCompletedCheckpoints)||!initialObservation.knownCompletedCheckpoints.length)throw Error('Completed initial-finality observation required');
 const known=new Map();for(const row of initialObservation.knownCompletedCheckpoints){cp(row);if(known.has(row.epoch)&&known.get(row.epoch)!==row.root)throw Error('Conflicting initial checkpoints');known.set(row.epoch,row.root);}
 const previous=new Map(rotationNodes.map(name=>{const e=initialObservation.perOwnerPreviousEpoch?.[name];if(typeof e!=='string'||!/^(?:0|[1-9][0-9]*)$/.test(e))throw Error('Initial per-owner finality required');return [name,e];}));
 return {async sample(name,read,peers){
  if(!rotationNodes.includes(name)||typeof read!=='function'||typeof peers!=='string'||!/^0x(?:0|[1-9a-f][0-9a-f]*)$/.test(peers))throw Error('Fresh partition observer input differs');
  const state=cp((await read('/eth/v1/beacon/states/head/finality_checkpoints')).data?.finalized),first=await read('/eth/v1/beacon/headers/finalized'),second=await read('/eth/v1/beacon/headers/finalized');
  const a=durableFinalityObservation(state,first,known,previous.get(name)),b=durableFinalityObservation(state,second,known,previous.get(name));
  if(!a||!b||first.data.root!==second.data.root)return null;const checkpoint=cp(a.checkpoint);if(known.has(checkpoint.epoch)&&known.get(checkpoint.epoch)!==checkpoint.root)throw Error('Conflicting finalized roots across fresh fault');
  previous.set(name,checkpoint.epoch);known.set(checkpoint.epoch,checkpoint.root);return {name,peers,checkpoint:{...checkpoint},source:a.source};
 }};
}
