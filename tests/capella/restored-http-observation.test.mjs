import test from 'node:test';
import assert from 'node:assert/strict';
import {beaconObservationError,pendingBeaconObservation,verifyRestoredAncestry} from './restored-http-observation.mjs';
import {restoredFinalityProgress} from './restored-finality-progress.mjs';
test('Known Beacon budget refusals preserve status, phase and elapsed diagnostics',()=>{
 for(const message of ['Beacon read capacity exceeded','Beacon read cancelled or execution budget expired','Beacon read queue expired']){
  const e=beaconObservationError(503,'vps-docker','/eth/v1/beacon/headers/head',JSON.stringify({code:503,message,phase:'snapshot',elapsedMs:501}));
  assert.equal(pendingBeaconObservation(e),true);assert.equal(e.responseBody.phase,'snapshot');assert.equal(e.responseBody.elapsedMs,501);assert.equal(e.node,'vps-docker');assert.match(e.message,/snapshot/);
 }
});
test('Unknown state failures, malformed errors and RPC requests remain fatal',()=>{
 for(const [status,path,raw] of [[503,'/eth/v1/beacon/headers/head','{"message":"Beacon state unavailable"}'],[503,'/eth/v1/beacon/headers/head','broken'],[500,'/eth/v1/beacon/headers/head','{"message":"Beacon read queue expired"}'],[503,'/','{"message":"Beacon read queue expired"}']]){
  const e=beaconObservationError(status,'vps-docker',path,raw);assert.equal(pendingBeaconObservation(e),false);assert.ok(e.responseBody);
 }
});
test('A refused observation cannot preserve consecutive stable finality reads',()=>{
 const baseline={epoch:'1',root:'base'},p=restoredFinalityProgress(baseline,{requiredAdvances:1,requiredStableSamples:3});
 const rows=['mac-native','mac-docker','vps-docker'].map(name=>({name,peers:'0x1',checkpoint:{epoch:'2',root:'next'}}));
 assert.equal(p.observe(rows),null);assert.equal(p.observe(rows),null);
 assert.equal(p.observe(rows.slice(0,2)),null);assert.equal(p.snapshot().stableSamples,0);
 assert.equal(p.observe(rows),null);assert.equal(p.observe(rows),null);assert.deepEqual(p.observe(rows),{epoch:'2',root:'next'});
 assert.equal(p.snapshot().distinctAdvances.length,1);
});

test('A budget refusal during ancestry cannot make a partial cache count as verified',async()=>{
 const caches=[new Map(),new Map(),new Map()],calls=[];let refused=false;
 const read=async root=>{calls.push(root);if(root==='parent'&&!refused){refused=true;throw beaconObservationError(503,'vps-docker','/eth/v2/beacon/blocks/parent','{"message":"Beacon read queue expired"}');}return {version:'capella',execution_optimistic:false,data:{message:{parent_root:root==='tip'?'parent':'base'}}};};
 const args={root:'tip',baseline:'base',node:2,caches,read};
 await assert.rejects(verifyRestoredAncestry(args),e=>pendingBeaconObservation(e));
 assert.equal(caches[2].size,1);await verifyRestoredAncestry(args);
 assert.deepEqual(calls,['tip','parent','parent']);assert.equal(caches[2].get('parent').parent_root,'base');
});
test('Ancestry rejects contradictory cached bodies and cycles',async()=>{
 const caches=[new Map([['tip',{parent_root:'base'}]]),new Map([['tip',{parent_root:'other'}]]),new Map()];
 await assert.rejects(verifyRestoredAncestry({root:'tip',baseline:'base',node:0,caches,read:async()=>{throw Error('unexpected read');}}),/Conflicting canonical/);
 await assert.rejects(verifyRestoredAncestry({root:'loop',baseline:'base',node:0,caches:[new Map([['loop',{parent_root:'loop'}]])],maximum:2,read:async()=>{throw Error('unexpected read');}}),/work bound/);
});
