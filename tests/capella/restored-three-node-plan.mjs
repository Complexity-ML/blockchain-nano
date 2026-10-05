import assert from 'node:assert/strict';
import {pinnedQuiescentRestart} from './deposit-restart-pins.mjs';
const names=['mac-native','mac-docker','vps-docker'];
const identity=k=>{const n=k.index===null?64:Number(BigInt(k.index));assert.ok(Number.isInteger(n)&&n>=0&&n<=64);assert.equal(k.keyEnv,`NANO_TEST_CAPELLA_NETWORK_KEY_${n}`);assert.equal(k.index===null,n===64,'Only admitted fixture key 64 may retain pending identity');return n;};
// Administrative signer ownership changes only; no consensus rule or journal reset.
export function restoredThreeNodePlan({macConfig,vpsConfig,macCapsule,vpsCapsule,pendingPublicKey,genesisHash,finalized,writersStopped}){
 assert.equal(writersStopped,true,'Source writers must be observed stopped before preparing copies');
 const binding={writersSuspended:true,genesisHash,finalized};
 const mac=pinnedQuiescentRestart(macConfig,macCapsule,pendingPublicKey,binding),vps=pinnedQuiescentRestart(vpsConfig,vpsCapsule,pendingPublicKey,binding);
 assert.equal(mac.capella.profile.sha256,vps.capella.profile.sha256,'Consensus profile mismatch');assert.deepEqual(mac.capella.limits,vps.capella.limits,'Recovery resource limits mismatch');
 assert.deepEqual(macCapsule.canonicalReplay.finalized,vpsCapsule.canonicalReplay.finalized,'Stored finalized checkpoints differ');
 const all=[...mac.capella.validator.keys,...vps.capella.validator.keys];assert.equal(all.length,65,'Full admitted fixture inventory required');
 const seen=new Set();for(const key of all){const i=identity(key);assert.ok(!seen.has(i),'Overlapping source signer inventory');seen.add(i);}
 assert.deepEqual([...seen].sort((a,b)=>a-b),Array.from({length:65},(_,i)=>i),'Missing admitted signer');
 const configs=[structuredClone(mac),structuredClone(mac),structuredClone(vps)];
 configs[0].capella.validator.keys=configs[0].capella.validator.keys.filter(k=>identity(k)<16);
 configs[1].capella.validator.keys=configs[1].capella.validator.keys.filter(k=>identity(k)>=16);
 const ownership=Object.fromEntries(configs.map((c,n)=>[names[n],c.capella.validator.keys.map(identity)]));
 const owner=new Map(),paths=new Set();
 for(const [n,c] of configs.entries()){
  assert.equal(c.capella.initial,null);assert.equal(c.capella.automaticRecovery,false);
  for(const k of c.capella.validator.keys){const i=identity(k);assert.ok(!owner.has(i),'Duplicate active signer');owner.set(i,n);assert.notEqual(k.journal,k.recoveryWitness,'Signer journal equals independent witness');for(const p of [k.journal,k.recoveryWitness]){assert.ok(typeof p==='string'&&p.startsWith('/var/lib/nano/'),'Unexpected fixture path');const scoped=names[n]+':'+p;assert.ok(!paths.has(scoped),'Shared local journal/witness path');paths.add(scoped);}}
 }
 assert.equal(owner.size,65);
 return {format:'nano-private-restored-three-node-plan-v1',names,configs,ownership,sourceCheckpoints:{mac:macCapsule.restart,vps:vpsCapsule.restart},baseline:structuredClone(finalized),sourceWritersObservedStopped:true,peerIdentityCopyPermitted:false,sourceDatabaseTransferRequired:false,keysLoaded:false,networkQualified:false,productionQualified:false};
}
