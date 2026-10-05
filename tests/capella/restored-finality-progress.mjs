import assert from 'node:assert/strict';
// Count distinct shared finalized epochs, never repeated reads or skipped epoch numbers.
export function restoredFinalityProgress(baseline,{requiredAdvances=3,requiredStableSamples=3}={}){
 assert.ok(Number.isInteger(requiredAdvances)&&requiredAdvances>0);assert.ok(Number.isInteger(requiredStableSamples)&&requiredStableSamples>0);
 const first=BigInt(baseline.epoch);let latest=first,stable=0;const advances=[];
 return {observe(rows){
  const valid=rows.length===3&&new Set(rows.map(r=>r.name)).size===3&&rows.every(r=>BigInt(r.peers)>0n&&r.checkpoint.root===rows[0].checkpoint.root&&BigInt(r.checkpoint.epoch)===BigInt(rows[0].checkpoint.epoch));
  if(!valid){stable=0;return null;}
  const cp=rows[0].checkpoint,epoch=BigInt(cp.epoch);assert.ok(epoch>=latest,'Shared finality regressed');
  if(epoch===first)assert.equal(cp.root,baseline.root,'Baseline shared root changed');
  if(epoch>latest){advances.push(structuredClone(cp));latest=epoch;stable=1;}
  else {if(advances.length)assert.equal(cp.root,advances.at(-1).root,'Shared root changed at same epoch');++stable;}
  return advances.length>=requiredAdvances&&stable>=requiredStableSamples?structuredClone(cp):null;
 },snapshot(){return {baseline:structuredClone(baseline),distinctAdvances:structuredClone(advances),stableSamples:stable,requiredAdvances,requiredStableSamples};}};
}
