import assert from 'node:assert/strict';
import {pinnedQuiescentRestart} from './deposit-restart-pins.mjs';
const names=['mac-native','mac-docker','vps-docker'];
const identity=k=>{const n=k.index===null?64:Number(BigInt(k.index));assert.ok(Number.isInteger(n)&&n>=0&&n<=64);assert.equal(k.keyEnv,`NANO_TEST_CAPELLA_NETWORK_KEY_${n}`);assert.equal(k.index===null,n===64,'Only admitted fixture key 64 may retain pending identity');return n;};
// Administrative signer ownership changes only; no consensus rule or journal reset.
export function restoredThreeNodePlan({macConfig,vpsConfig,macCapsule,vpsCapsule,macNativeConfig,macNativeCapsule,pendingPublicKey,genesisHash,finalized,writersStopped,retentionExperiment,coldCheckpoint}){
 assert.equal(writersStopped,true,'Source writers must be observed stopped before preparing copies');
 const binding={writersSuspended:true,genesisHash,finalized};
 assert.equal(macNativeConfig===undefined,macNativeCapsule===undefined,'Separate native source requires configuration and audited capsule');
 const mac=pinnedQuiescentRestart(macConfig,macCapsule,pendingPublicKey,binding),vps=pinnedQuiescentRestart(vpsConfig,vpsCapsule,pendingPublicKey,binding);
 const native=macNativeConfig===undefined?structuredClone(mac):pinnedQuiescentRestart(macNativeConfig,macNativeCapsule,pendingPublicKey,binding);
 assert.deepEqual(native.capella.validator.keys.map(identity),mac.capella.validator.keys.map(identity),'Native and Docker source inventories differ');
 assert.equal(native.capella.profile.sha256,mac.capella.profile.sha256,'Native source profile mismatch');assert.deepEqual(native.capella.limits,mac.capella.limits,'Native source limits mismatch');
 assert.deepEqual((macNativeCapsule??macCapsule).canonicalReplay.finalized,macCapsule.canonicalReplay.finalized,'Native source finalized checkpoint differs');
 assert.equal(mac.capella.profile.sha256,vps.capella.profile.sha256,'Consensus profile mismatch');assert.deepEqual(mac.capella.limits,vps.capella.limits,'Recovery resource limits mismatch');
 assert.deepEqual(macCapsule.canonicalReplay.finalized,vpsCapsule.canonicalReplay.finalized,'Stored finalized checkpoints differ');
 const all=[...mac.capella.validator.keys,...vps.capella.validator.keys];assert.equal(all.length,65,'Full admitted fixture inventory required');
 const seen=new Set();for(const key of all){const i=identity(key);assert.ok(!seen.has(i),'Overlapping source signer inventory');seen.add(i);}
 assert.deepEqual([...seen].sort((a,b)=>a-b),Array.from({length:65},(_,i)=>i),'Missing admitted signer');
 const configs=[native,structuredClone(mac),structuredClone(vps)];
 configs[0].capella.validator.keys=configs[0].capella.validator.keys.filter(k=>identity(k)<16);
 configs[1].capella.validator.keys=configs[1].capella.validator.keys.filter(k=>identity(k)>=16);
 // Explicit fixture resource experiment, applied only AFTER source validation.
 // Source capsules, consensus profile and exact recovery/signer pins stay intact.
 // Increasing this local budget does not qualify recovery at the old boundary.
 let retentionBudget;
 if(retentionExperiment!==undefined){
  assert.ok(retentionExperiment&&typeof retentionExperiment==='object'&&!Array.isArray(retentionExperiment));
  const explicit=Object.hasOwn(retentionExperiment,'captureBudget');
  const catalogExplicit=Object.hasOwn(retentionExperiment,'catalogRootBytes');
  assert.deepEqual(Object.keys(retentionExperiment).sort(),['blocks','rationale',...(explicit?['captureBudget']:[]),...(catalogExplicit?['catalogRootBytes']:[])].sort());
  assert.match(retentionExperiment.blocks,/^0x[1-9a-f][0-9a-f]*$/);
  assert.ok(typeof retentionExperiment.rationale==='string'&&retentionExperiment.rationale.trim().length>=20&&retentionExperiment.rationale.length<=2048);
  const source=structuredClone(mac.capella.limits),target=BigInt(retentionExperiment.blocks);
  assert.ok(target>BigInt(source.blocks),'Experiment must increase the retained-block budget');
  const captureBudget=explicit?retentionExperiment.captureBudget:{blocks:'0x400',envelopeBytes:'0x8000000'};
  assert.ok(captureBudget&&typeof captureBudget==='object'&&!Array.isArray(captureBudget));
  assert.deepEqual(Object.keys(captureBudget).sort(),['blocks','envelopeBytes']);
  for(const field of ['blocks','envelopeBytes']){assert.equal(typeof captureBudget[field],'string');assert.match(captureBudget[field],/^0x[1-9a-f][0-9a-f]*$/);assert.ok(BigInt(captureBudget[field])<=0xffffffffffffffffn);}
  const captureBlocks=BigInt(captureBudget.blocks),captureBytes=BigInt(captureBudget.envelopeBytes);
  assert.ok(captureBytes<=(0xffffffffffffffffn-2048n)/8n&&captureBlocks<=captureBytes,'Capture witness/resource budget overflow');
  assert.ok(target<=captureBlocks&&BigInt(source.envelopeBytes)<=captureBytes,'Experiment exceeds explicit capture resource budget');
  if(catalogExplicit){assert.equal(typeof retentionExperiment.catalogRootBytes,'string');assert.match(retentionExperiment.catalogRootBytes,/^0x[1-9a-f][0-9a-f]*$/);assert.ok(BigInt(retentionExperiment.catalogRootBytes)<=captureBytes,'Catalog exceeds explicit local capture resource budget');}
  for(const c of configs){
   const roots=catalogExplicit?BigInt(retentionExperiment.catalogRootBytes):BigInt(c.capella.catalogs.rootBytes);
   const required=[target,BigInt(c.capella.limits.messages),BigInt(c.capella.limits.equivocations)].reduce((a,b)=>a>b?a:b);
   assert.ok(roots>=BigInt(c.capella.catalogs.rootBytes)&&required<=roots/32n,'Catalog budget below candidate graph/inventory capacity');
   if(catalogExplicit)c.capella.catalogs.rootBytes=retentionExperiment.catalogRootBytes;
   c.capella.limits.blocks=retentionExperiment.blocks;
  }
  retentionBudget={...(catalogExplicit?{catalogRootBytes:retentionExperiment.catalogRootBytes}:{}),...(explicit?{captureBudget:structuredClone(captureBudget)}:{}),sourceLimits:source,candidateLimits:structuredClone(configs[0].capella.limits),rationale:retentionExperiment.rationale,oldBoundaryRecoveryQualified:false,productionQualified:false};
 }
 // Explicit startup migration; no source files, pins or consensus profile change.
 let coldCheckpointPlan;
 if(coldCheckpoint!==undefined){
  assert.ok(coldCheckpoint&&typeof coldCheckpoint==='object'&&!Array.isArray(coldCheckpoint));
  assert.deepEqual(Object.keys(coldCheckpoint).sort(),['cacheEncodedBytes','cacheRecords']);
  for(const [field,limit] of [['cacheRecords','blocks'],['cacheEncodedBytes','envelopeBytes']]){
   assert.equal(typeof coldCheckpoint[field],'string');assert.match(coldCheckpoint[field],/^0x[1-9a-f][0-9a-f]*$/);
   const value=BigInt(coldCheckpoint[field]);assert.ok(value<=0xffffffffffffffffn,'Cold cache exceeds runtime uint64 range');
   for(const c of configs)assert.ok(value<=BigInt(c.capella.limits[limit]),'Cold cache exceeds configured local budget');
  }
  for(const c of configs){
   if(c.capella.coldCheckpoint!==undefined)assert.deepEqual(c.capella.coldCheckpoint,coldCheckpoint,'Existing cold format options cannot be changed by the planner');
   c.capella.coldCheckpoint=structuredClone(coldCheckpoint);
  }
  coldCheckpointPlan={options:structuredClone(coldCheckpoint),sourceCommitmentsUnchanged:true,startupMigrationPerformed:false,physicalRecoveryQualified:false,networkQualified:false};
 }
 const ownership=Object.fromEntries(configs.map((c,n)=>[names[n],c.capella.validator.keys.map(identity)]));
 const owner=new Map(),paths=new Set();
 for(const [n,c] of configs.entries()){
  assert.equal(c.capella.initial,null);assert.equal(c.capella.automaticRecovery,false);
  for(const k of c.capella.validator.keys){const i=identity(k);assert.ok(!owner.has(i),'Duplicate active signer');owner.set(i,n);assert.notEqual(k.journal,k.recoveryWitness,'Signer journal equals independent witness');for(const p of [k.journal,k.recoveryWitness]){assert.ok(typeof p==='string'&&p.startsWith('/var/lib/nano/'),'Unexpected fixture path');const scoped=names[n]+':'+p;assert.ok(!paths.has(scoped),'Shared local journal/witness path');paths.add(scoped);}}
 }
 assert.equal(owner.size,65);
 return {format:'nano-private-restored-three-node-plan-v1',...(retentionBudget?{retentionBudget}:{}),...(coldCheckpointPlan?{coldCheckpointPlan}:{}),names,configs,ownership,sourceCheckpoints:{mac:macCapsule.restart,vps:vpsCapsule.restart,...(macNativeCapsule?{macNative:macNativeCapsule.restart}: {})},baseline:structuredClone(finalized),sourceWritersObservedStopped:true,peerIdentityCopyPermitted:false,sourceDatabaseTransferRequired:false,keysLoaded:false,networkQualified:false,productionQualified:false};
}
