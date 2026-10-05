// Head-state checkpoints can lag the durable fork-choice checkpoint after a reorg.
// Never invent an epoch from a block slot: missed boundary slots make that wrong.
export function durableFinalityObservation(headCheckpoint,header,known,previousEpoch){
 if(header?.execution_optimistic!==false||header?.finalized!==true||header?.data?.canonical!==true||!/^0x[a-f0-9]{64}$/.test(header?.data?.root??''))throw Error('Unauthenticated finalized header observation');
 const root=header.data.root;
 let checkpoint,source;
 if(headCheckpoint.root===root){checkpoint=headCheckpoint;source='matching-head-state-and-finalized-header';}
 else{
  const matches=[...known].filter(([,r])=>r===root).sort(([a],[b])=>BigInt(a)<BigInt(b)?1:BigInt(a)>BigInt(b)?-1:0);
  if(!matches.length)return null; // bounded caller retries until a genuine epoch/root pair is observed
  checkpoint={epoch:matches[0][0],root};source='previously-observed-checkpoint-and-current-finalized-header';
 }
 if(BigInt(checkpoint.epoch)<BigInt(previousEpoch))throw Error('Durable finalized header regressed');
 return {checkpoint,source,headStateCheckpoint:headCheckpoint};
}
