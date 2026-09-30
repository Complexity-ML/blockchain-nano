// Read-only diagnostic snapshots. No deadlines or qualification criteria change.
const root=x=>typeof x==='string'&&/^0x[0-9a-f]{64}$/.test(x);
const number=x=>typeof x==='string'&&/^(0|[1-9][0-9]*)$/.test(x);
function checkpoint(x){if(!x||!number(x.epoch)||!root(x.root))throw Error('Invalid fault progress checkpoint');return {epoch:x.epoch,root:x.root};}
export async function captureFaultProgress(read,clock=Date.now){
 const beforeMs=clock();
 const h=(await read('/eth/v1/beacon/headers/head')).data;
 const header=h?.header?.message;
 if(!root(h?.root)||!root(header?.state_root)||!number(header?.slot))throw Error('Invalid fault progress header');
 // Resolve the immutable state selected by the observed header, even when the
 // head advances between requests. Never label two mutable head reads atomic.
 const cp=(await read('/eth/v1/beacon/states/'+header.state_root+'/finality_checkpoints')).data;
 const result={headRoot:h.root,headSlot:header.slot,stateRoot:header.state_root,
  justified:checkpoint(cp?.current_justified),finalized:checkpoint(cp?.finalized),beforeMs,afterMs:clock()};
 if(!Number.isSafeInteger(result.beforeMs)||!Number.isSafeInteger(result.afterMs)||result.afterMs<result.beforeMs)throw Error('Invalid fault progress sampling clock');
 return result;
}
