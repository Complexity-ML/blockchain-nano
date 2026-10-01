// Finite fault schedule only; these budgets are test limits, not network limits.
export function partitionSchedule(text){
 if(text===undefined)return null;
 if(typeof text!=='string'||!/^\d+(,\d+)*$/.test(text))throw Error('Invalid finite partition schedule');
 const cuts=text.split(',').map(Number);
 if(cuts.length<2||cuts.length>4||cuts.some(n=>!Number.isSafeInteger(n)||n<1000||n>300000)||cuts.reduce((a,b)=>a+b,0)>600000)throw Error('Partition schedule exceeds finite qualification budget');
 return cuts;
}
export async function runPartitionCycles({cuts,partition,sample,record}){
 partitionSchedule(cuts.join(','));
 const roots=new Map(),previous=new Map();let identities;
 const guarded=async()=>{
  const rows=await sample();
  if(!Array.isArray(rows)||rows.length!==3||new Set(rows.map(r=>r.name)).size!==3)throw Error('Three distinct partition-cycle observers required');
  const names=rows.map(r=>r.name).sort().join(',');if(identities&&names!==identities)throw Error('Partition-cycle observer identities changed');identities=names;
  for(const r of rows){
   const epoch=BigInt(r.checkpoint.epoch),root=r.checkpoint.root,key=String(epoch);
   if(epoch<0n||!/^0x[0-9a-f]{64}$/.test(root))throw Error('Invalid cycle checkpoint');
   if(previous.has(r.name)&&epoch<previous.get(r.name))throw Error('Finality regressed between partition cycles');
   if(roots.has(key)&&roots.get(key)!==root)throw Error('Conflicting finalized roots between partition cycles');
   roots.set(key,root);previous.set(r.name,epoch);
  }
  return rows;
 };
 const common=rows=>rows.every(r=>BigInt(r.peers)>0n&&r.checkpoint.epoch===rows[0].checkpoint.epoch&&r.checkpoint.root===rows[0].checkpoint.root);
 for(const [i,cutMs] of cuts.entries()){
  const before=await guarded();if(!common(before))throw Error('Connected common checkpoint required before next partition cycle');
  const cycle=i+1;await record({event:'partition-cycle-start',cycle,cutMs,baseline:before[0].checkpoint});
  // A deadline/safety/restoration error ends the schedule; never start another
  // cut to conceal it. The underlying partition function restores its network.
  await partition({cutMs,sample:guarded,record:e=>record({...e,cycle})});
  const after=await guarded();
  if(!common(after)||BigInt(after[0].checkpoint.epoch)<=BigInt(before[0].checkpoint.epoch))throw Error('Partition cycle lacked newer connected common finality');
  await record({event:'partition-cycle-passed',cycle,cutMs,checkpoint:after[0].checkpoint});
 }
 return {cycles:cuts.length,cutsMs:cuts,securityHistoryPreserved:true};
}
