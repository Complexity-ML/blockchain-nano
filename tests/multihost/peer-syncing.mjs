// Read-only monitoring evidence; peer claims never prove consensus finality.
const uint64=value=>typeof value==='string'&&/^(0|[1-9][0-9]*)$/.test(value)&&BigInt(value)<=0xffffffffffffffffn;
export function verifyPeerSyncing(body,before,after){
 const data=body?.data;
 if(!data||!uint64(data.head_slot)||!uint64(data.sync_distance)||!uint64(before)||!uint64(after))throw Error('Invalid Beacon syncing decimal uint64');
 if(typeof data.is_syncing!=='boolean'||data.is_optimistic!==false||data.el_offline!==false)throw Error('Invalid integrated Beacon syncing flags');
 if(data.is_syncing!==(BigInt(data.sync_distance)>0n))throw Error('Beacon syncing distance/flag disagreement');
 if(BigInt(data.head_slot)<BigInt(before)||BigInt(data.head_slot)>BigInt(after))throw Error('Beacon syncing head outside surrounding observations');
 if(BigInt(data.head_slot)+BigInt(data.sync_distance)>0xffffffffffffffffn)throw Error('Beacon syncing target overflow');
 return {...data};
}
export async function capturePeerSyncing({request,record,phase,now=Date.now,delay=ms=>new Promise(resolve=>setTimeout(resolve,ms)),budgetMs=60000}){
 const started=now(),deadline=started+budgetMs;let attempts=0;
 while(now()<deadline){
  ++attempts;const rows=[];let unavailable=false;
  for(const [kind,name] of [['native','mac-native'],['local','mac-docker'],['remote','vps-docker']]){
   try{
    const before=(await request(kind,'/eth/v1/beacon/headers/head')).data.header.message.slot;
    const body=await request(kind,'/eth/v1/node/syncing');
    const after=(await request(kind,'/eth/v1/beacon/headers/head')).data.header.message.slot;
    rows.push({name,...verifyPeerSyncing(body,before,after)});
   }catch(error){
    if(error.name!=='HttpReadError'||error.status!==503)throw error;
    unavailable=true;await record({event:'peer-syncing-unavailable',phase,name,attempt:attempts,message:error.message});
   }
  }
  if(!unavailable&&now()<deadline){const result={event:'peer-syncing-window',phase,rows,attempts,elapsedMs:now()-started};await record(result);return result;}
  const remaining=deadline-now();if(remaining>0)await delay(Math.min(3000,remaining));
 }
 throw Error(`Peer syncing observation deadline exceeded (${phase})`);
}
