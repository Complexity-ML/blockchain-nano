import {setTimeout as delay} from 'node:timers/promises';

export class PartitionRecoveryDeadline extends Error {
 constructor(waitForRecovery){super('Partition recovery deadline exceeded');this.name='PartitionRecoveryDeadline';this.waitForRecovery=waitForRecovery;}
}

// Docker commands operate only on the fresh, labelled test container. No host
// firewall changes; loopback RPC remains reachable via docker exec while cut.
export async function partitionRemote({docker, container, network, revision, sample, record,
 clock=Date.now, wait=delay, cutMs=96000, recoveryMs=180000, duringCut}) {
 if(!/^nano-qualification-vps-[a-z0-9]+$/.test(container)||network!==container+'-net')
  throw Error('Unexpected partition target');
 const inspect=async()=>JSON.parse(await docker(['inspect','--format','{{json .}}',container]));
 const initial=await inspect(), labels=initial.Config?.Labels;
 if(!initial.State?.Running||labels?.['nano.qualification']!=='multihost'||labels?.['org.opencontainers.image.revision']!==revision)
  throw Error('Partition provenance mismatch');
 const entries=Object.entries(initial.NetworkSettings.Networks);
 if(entries.length!==1||entries[0][0]!==network||!entries[0][1].IPAddress)throw Error('Expected dedicated test network');
 const address=entries[0][1].IPAddress;
 const roots=new Map(),previous=new Map();
 const read=async()=>{
  const rows=await sample();
  if(rows.length!==3||new Set(rows.map(r=>r.name)).size!==3)throw Error('Expected three partition observers');
  for(const row of rows){
   const epoch=BigInt(row.checkpoint.epoch),root=row.checkpoint.root;
   if(!/^0x[0-9a-f]{64}$/.test(root)||epoch<0n)throw Error('Invalid checkpoint');
   if(previous.has(row.name)&&epoch<previous.get(row.name))throw Error('Finality regressed across partition');
   if(roots.has(String(epoch))&&roots.get(String(epoch))!==root)throw Error('Conflicting finalized roots across partition');
   previous.set(row.name,epoch);roots.set(String(epoch),root);
  }
  await record({event:'partition-observation',rows});return rows;
 };
 const before=await read();
 if(!before.every(r=>r.checkpoint.epoch===before[0].checkpoint.epoch&&r.checkpoint.root===before[0].checkpoint.root))throw Error('Common checkpoint required before partition');
 const baseline=BigInt(before[0].checkpoint.epoch);
 let disconnected=false;
 try{
  // Mark before dispatch: an SSH response can be lost after Docker applied it.
  disconnected=true;
  await docker(['network','disconnect',network,container]);
  const start=clock();let sawZeroPeers=false;
  await record({event:'partition-disconnected',container,network,address,cutMs});
  // Both branches must settle before reconnecting; no detached submitter may
  // survive a failed observer or submit transactions after test cleanup.
  const monitor=async()=>{
  while(clock()-start<cutMs){
   const state=await inspect();
   if(!state.State.Running||Object.keys(state.NetworkSettings.Networks).length!==0)throw Error('Partition not effective');
   const rows=await read(),remote=rows.find(r=>r.name==='vps-docker');
   if(!remote)throw Error('Missing isolated observer');
   if(BigInt(remote.peers)===0n)sawZeroPeers=true;
   else if(sawZeroPeers||clock()-start>=30000)throw Error('Isolated node still has peers');
   await wait(Math.max(0,Math.min(5000,cutMs-(clock()-start))));
  }
  if(!sawZeroPeers)throw Error('No zero-peer observation during partition');
  };
  const tasks=[monitor()];
  if(duringCut)tasks.push(Promise.resolve().then(async()=>{
   const state=await inspect();
   if(!state.State.Running||Object.keys(state.NetworkSettings.Networks).length!==0)throw Error('Workload requires effective partition');
   await record({event:'partition-workload-start'});
   await duringCut();
   await record({event:'partition-workload-finished'});
  }));
  const results=await Promise.allSettled(tasks);
  const failures=results.filter(r=>r.status==='rejected').map(r=>r.reason);
  if(failures.length)await record({event:'partition-task-failed',errors:failures.map(error=>({name:error.name,message:error.message,stack:error.stack}))});
  if(failures.length===1)throw failures[0];
  if(failures.length>1)throw new AggregateError(failures,'Partition observation and workload failed');
 }finally{
  if(disconnected){
   const state=await inspect();
   if(!state.NetworkSettings.Networks[network])await docker(['network','connect','--ip',address,network,container]);
   const restored=await inspect();
   if(!restored.State.Running||restored.NetworkSettings.Networks[network]?.IPAddress!==address)throw Error('Partition restoration failed');
   await record({event:'partition-reconnected',container,network,address});
  }
 }
 const waitRecovery=async(event)=>{
  const deadline=clock()+recoveryMs;
  while(clock()<deadline){
   const rows=await read(),first=rows[0].checkpoint;
   if(rows.every(r=>BigInt(r.peers)>0n&&r.checkpoint.epoch===first.epoch&&r.checkpoint.root===first.root)&&BigInt(first.epoch)>baseline){
    await record({event,checkpoint:first});return true;
   }
   await wait(5000);
  }
  return false;
 };
 if(await waitRecovery('partition-recovery-bound'))return;
 // Preserve the same roots/previous maps for an optional diagnostic extension.
 // No second partition, baseline reset or relaxation of the original verdict.
 throw new PartitionRecoveryDeadline(async()=>{
  if(!await waitRecovery('late-partition-recovery-bound'))throw Error('Late recovery observation deadline exceeded');
 });
}
