import assert from 'node:assert/strict';
// Preserve refusal details; only known read-budget refusals permit a missing sample.
// The caller keeps its original wall-clock deadline and records every refusal.
export function beaconObservationError(status, node, path, raw) {
 let responseBody; try { responseBody=JSON.parse(raw); } catch { responseBody={raw:String(raw).slice(0,4096)}; }
 const error=new Error(`HTTP ${status} ${node} ${path}: ${JSON.stringify(responseBody)}`);
 Object.assign(error,{status,node,path,responseBody});return error;
}
export function pendingBeaconObservation(error) {
 return error.status===503 && error.path?.startsWith('/eth/') &&
  ['Beacon read capacity exceeded','Beacon read cancelled or execution budget expired','Beacon read queue expired'].includes(error.responseBody?.message);
}

// Cached bodies may come from an interrupted read: walk their parents to the
// baseline too. A partial cache must never certify an unverified ancestry.
export async function verifyRestoredAncestry({root,baseline,node,caches,read,maximum=1024}) {
 let tip=root;
 for(let count=0;tip!==baseline;count++){
  assert.ok(count<maximum,'Ancestry work bound exceeded');
  let message=caches[node].get(tip);
  if(!message){const b=await read(tip);assert.equal(b.version,'capella');assert.equal(b.execution_optimistic,false);message=b.data.message;}
  for(let other=0;other<caches.length;other++)if(caches[other].has(tip))assert.deepEqual(message,caches[other].get(tip),'Conflicting canonical block bodies');
  caches[node].set(tip,message);tip=message.parent_root;
 }
}
