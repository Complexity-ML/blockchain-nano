// Public RPC observation only: no files, keys, writers, clock mutation or signing.
import assert from 'node:assert/strict';
import {durableFinalityObservation} from './finality-observation.mjs';
import {restoredFinalityProgress} from './restored-finality-progress.mjs';
import {verifyRestoredAncestry,pendingBeaconObservation} from './restored-http-observation.mjs';
import {inspectCapellaState} from './isolated-fixture-domain.mjs';
const owners=['mac-native','mac-docker','vps-docker'],U64=(1n<<64n)-1n,ZERO_ROOT='0x'+'00'.repeat(32);
const root=value=>{assert.equal(typeof value,'string');assert.match(value,/^0x[0-9a-f]{64}$/);return value;};
const epoch=value=>{assert.equal(typeof value,'string');assert.match(value,/^(?:0|[1-9][0-9]*)$/);const n=BigInt(value);assert.ok(n<=U64);return n;};
const checkpoint=value=>{assert.ok(value&&typeof value==='object');assert.deepEqual(Object.keys(value).sort(),['epoch','root']);epoch(value.epoch);root(value.root);return value;};
const sameCheckpoint=(a,b)=>a.epoch===b.epoch&&a.root===b.root;
// Caller authenticates the exact initial JSON bytes before parsing. This helper
// binds the checkpoint to its SSZ state/root/profile, not an RPC or caller claim.
export function isolatedBootstrapStateCheckpoint(initial){
 assert.equal(initial?.format,'nano-capella-store-v1');assert.ok(Array.isArray(initial.blocks)&&initial.blocks.length===1,'Bootstrap requires one trusted anchor');
 const row=initial.blocks[0];assert.ok(Array.isArray(row)&&row.length===6);assert.equal(row[0],initial.anchor);root(row[0]);
 assert.ok(Array.isArray(row[1])&&row[1].length===5&&row[1][0]==='0x1','Bootstrap anchor must be slot one');
 assert.deepEqual(initial.finalized,['0x0',initial.anchor]);assert.ok(Array.isArray(row[2])&&row[2].length===2);assert.match(row[2][0],/^0x(?:[0-9a-f]{2})+$/);
 const state=Buffer.from(row[2][0].slice(2),'hex'),inspected=inspectCapellaState(state,initial.profile);assert.equal(inspected.stateRoot,row[1][3],'Bootstrap state root differs from trusted header');assert.equal(inspected.slot,1n);
 const [history,randao,slashings]=initial.profile.common[0].stateLimits.map(BigInt);
 const at=176n+64n*history+96n+32n*randao+8n*slashings+9n+80n;assert.ok(at+40n<=BigInt(state.length));
 const cp=checkpoint({epoch:state.readBigUInt64LE(Number(at)).toString(),root:'0x'+state.subarray(Number(at)+8,Number(at)+40).toString('hex')});
 assert.deepEqual(cp,{epoch:'0',root:ZERO_ROOT},'Only the initial epoch-zero SSZ checkpoint is supported');return cp;
}
export function isolatedRuntimeObservation({baseline,bootstrapStateCheckpoint,requiredAdvances=2,requiredStableSamples=3,maximumAncestry=1024}){
 baseline=structuredClone(checkpoint(baseline));
 if(bootstrapStateCheckpoint!==undefined){bootstrapStateCheckpoint=structuredClone(checkpoint(bootstrapStateCheckpoint));assert.equal(baseline.epoch,'0','Bootstrap state checkpoint requires epoch-zero baseline');assert.deepEqual(bootstrapStateCheckpoint,{epoch:'0',root:ZERO_ROOT},'Only the initial epoch-zero SSZ checkpoint is supported');}
assert.ok(Number.isSafeInteger(maximumAncestry)&&maximumAncestry>0&&maximumAncestry<=1024);
 const known=new Map([[baseline.epoch,baseline.root]]),previous=new Map(owners.map(name=>[name,BigInt(baseline.epoch)])),caches=owners.map(()=>new Map());
 const sampled=new WeakMap(),consumed=new WeakSet();
 const progress=restoredFinalityProgress(baseline,{requiredAdvances,requiredStableSamples});
 return {
  async sample({name,read,peers}){
   assert.ok(owners.includes(name),'Unknown isolated owner');assert.equal(typeof read,'function');assert.equal(typeof peers,'string');assert.match(peers,/^0x(?:0|[1-9a-f][0-9a-f]*)$/);assert.ok(BigInt(peers)<=U64);
   const headState=await read('/eth/v1/beacon/states/head/finality_checkpoints');const state=checkpoint(headState.data.finalized);
   // The trusted Store anchor need not have a retained signature; the initial
   // head SSZ can also retain its original zero checkpoint. Neither is progress.
   // Once this owner advances, both headers must be read even if its head state
   // is stale, so known durable checkpoints remain usable and regressions fatal.
   if(previous.get(name)===epoch(baseline.epoch)&&(sameCheckpoint(state,baseline)||(bootstrapStateCheckpoint&&sameCheckpoint(state,bootstrapStateCheckpoint))))return null;
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
  snapshot(){return {format:'nano-isolated-runtime-observation-v1',...progress.snapshot(),knownCompletedCheckpoints:[...known].map(([epoch,root])=>({epoch,root})),perOwnerPreviousEpoch:Object.fromEntries([...previous].map(([owner,n])=>[owner,n.toString()])),bootstrapStateCheckpoint:bootstrapStateCheckpoint?structuredClone(bootstrapStateCheckpoint):null,networkQualified:false,productionQualified:false};}
 };
}
export function isolatedPendingObservation(error){return pendingBeaconObservation(error);}
