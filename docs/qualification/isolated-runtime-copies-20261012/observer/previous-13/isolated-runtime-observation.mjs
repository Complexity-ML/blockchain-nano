// Public RPC observation only: no files, keys, writers, clock mutation or signing.
import assert from 'node:assert/strict';
import {durableFinalityObservation} from './finality-observation.mjs';
import {restoredFinalityProgress} from './restored-finality-progress.mjs';
import {verifyRestoredAncestry,pendingBeaconObservation} from './restored-http-observation.mjs';
const owners=['mac-native','mac-docker','vps-docker'],U64=(1n<<64n)-1n;
const root=value=>{assert.equal(typeof value,'string');assert.match(value,/^0x[0-9a-f]{64}$/);return value;};
const epoch=value=>{assert.equal(typeof value,'string');assert.match(value,/^(?:0|[1-9][0-9]*)$/);const n=BigInt(value);assert.ok(n<=U64);return n;};
const checkpoint=value=>{assert.ok(value&&typeof value==='object');assert.deepEqual(Object.keys(value).sort(),['epoch','root']);epoch(value.epoch);root(value.root);return value;};
export function isolatedRuntimeObservation({baseline,requiredAdvances=2,requiredStableSamples=3,maximumAncestry=1024}){
 checkpoint(baseline);assert.ok(Number.isSafeInteger(maximumAncestry)&&maximumAncestry>0&&maximumAncestry<=1024);
 const known=new Map([[baseline.epoch,baseline.root]]),previous=new Map(owners.map(name=>[name,BigInt(baseline.epoch)])),caches=owners.map(()=>new Map());
 const sampled=new WeakMap(),consumed=new WeakSet();
 const progress=restoredFinalityProgress(baseline,{requiredAdvances,requiredStableSamples});
 return {
  async sample({name,read,peers}){
   assert.ok(owners.includes(name),'Unknown isolated owner');assert.equal(typeof read,'function');assert.equal(typeof peers,'string');assert.match(peers,/^0x(?:0|[1-9a-f][0-9a-f]*)$/);assert.ok(BigInt(peers)<=U64);
   const headState=await read('/eth/v1/beacon/states/head/finality_checkpoints');const state=checkpoint(headState.data.finalized);
   const first=await read('/eth/v1/beacon/headers/finalized'),second=await read('/eth/v1/beacon/headers/finalized');
   // Authenticate both bracket headers, not only their root equality. Genuine
   // state epoch/root pairs or previously completed ancestry are the only epoch source.
   const observation=durableFinalityObservation(state,first,known,previous.get(name));durableFinalityObservation(state,second,known,previous.get(name));
   if(first.data.root!==second.data.root||!observation)return null;
   const cp=checkpoint(observation.checkpoint),node=owners.indexOf(name);if(known.has(cp.epoch))assert.equal(cp.root,known.get(cp.epoch),'Conflicting finalized roots');
   await verifyRestoredAncestry({root:cp.root,baseline:baseline.root,node,caches,maximum:maximumAncestry,read:async tip=>{
    const block=await read('/eth/v2/beacon/blocks/'+tip);root(block.data?.message?.parent_root);assert.notEqual(block.data.message.parent_root,tip,'Canonical ancestry self-cycle');return block;
   }});
   previous.set(name,epoch(cp.epoch));known.set(cp.epoch,cp.root);const row={name,peers,checkpoint:structuredClone(cp),source:observation.source,ancestryVerified:true};sampled.set(row,JSON.stringify(row));return row;
  },
  observe(rows){
   assert.ok(Array.isArray(rows)&&rows.length<=3);const seen=new Set();for(const row of rows){assert.ok(sampled.has(row)&&!consumed.has(row),'Rows must be fresh one-shot samples from this observer');assert.equal(JSON.stringify(row),sampled.get(row),'Sample receipt was mutated');assert.ok(owners.includes(row.name));assert.ok(!seen.has(row.name),'Duplicate isolated owner');seen.add(row.name);assert.equal(row.ancestryVerified,true,'Incomplete ancestry cannot count');checkpoint(row.checkpoint);assert.equal(typeof row.peers,'string');assert.match(row.peers,/^0x(?:0|[1-9a-f][0-9a-f]*)$/);}
   for(const row of rows)consumed.add(row);return progress.observe(rows);
  },
  snapshot(){return {format:'nano-isolated-runtime-observation-v1',...progress.snapshot(),knownCompletedCheckpoints:[...known].map(([epoch,root])=>({epoch,root})),perOwnerPreviousEpoch:Object.fromEntries([...previous].map(([owner,n])=>[owner,n.toString()])),networkQualified:false,productionQualified:false};}
 };
}
export function isolatedPendingObservation(error){return pendingBeaconObservation(error);}
