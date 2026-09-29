import {setTimeout as delay} from 'node:timers/promises';

// Host tools enter only the labelled campaign container's network namespace.
// No changes to the host qdisc, firewall, routes, or SSH connection.
export async function createRemoteTc({docker, sh, container, network, revision}) {
 if(!/^nano-qualification-vps-[a-z0-9]+$/.test(container)||network!==container+'-net')throw Error('Unexpected delay target');
 let identity;
 const validate=async()=>{
  const s=JSON.parse(await docker(['inspect','--format','{{json .}}',container]));
  const labels=s.Config?.Labels,pid=s.State?.Pid;
  if(!s.State?.Running||s.State.Paused||!Number.isSafeInteger(pid)||pid<=1||
   labels?.['nano.qualification']!=='multihost'||labels?.['org.opencontainers.image.revision']!==revision||
   Object.keys(s.NetworkSettings?.Networks??{}).length!==1||!s.NetworkSettings.Networks[network]||s.HostConfig?.NetworkMode==='host')throw Error('Delay provenance mismatch');
  const ns=(await sh(['sudo','-n','readlink',`/proc/${pid}/ns/net`])).trim();
  const host=(await sh(['sudo','-n','readlink','/proc/1/ns/net'])).trim();
  if(!/^net:\[\d+\]$/.test(ns)||ns===host)throw Error('Unsafe network namespace');
  const current=JSON.stringify([s.Id,pid,ns]);
  if(identity&&identity!==current)throw Error('Delay container identity changed');
  identity=current;return pid;
 };
 await validate();
 return async args=>{
  const pid=await validate();
  return sh(['sudo','-n','nsenter','-t',String(pid),'-n','/usr/sbin/tc',...args]);
 };
}

export async function asymmetricRemote({tc,sample,record,clock=Date.now,wait=delay,holdMs=96000,progressMs=144000}) {
 const qdiscs=async()=>JSON.parse(await tc(['-j','-s','qdisc','show','dev','eth0']));
 const initial=await qdiscs();
 if(initial.some(q=>q.kind==='netem'))throw Error('Unexpected pre-existing netem');
 const roots=new Map(),previous=new Map();
 const read=async()=>{
  const rows=await sample();
  if(rows.length!==3||new Set(rows.map(r=>r.name)).size!==3)throw Error('Expected three delay observers');
  for(const r of rows){
   const epoch=BigInt(r.checkpoint.epoch),root=r.checkpoint.root;
   if(epoch<0n||!/^0x[0-9a-f]{64}$/.test(root))throw Error('Invalid finalized checkpoint');
   if(previous.has(r.name)&&epoch<previous.get(r.name))throw Error('Finality regressed during delay');
   if(roots.has(String(epoch))&&roots.get(String(epoch))!==root)throw Error('Conflicting finalized roots during delay');
   previous.set(r.name,epoch);roots.set(String(epoch),root);
  }
  await record({event:'asymmetric-observation',rows});return rows;
 };
 const common=rows=>rows.every(r=>BigInt(r.peers)>0n&&r.checkpoint.epoch===rows[0].checkpoint.epoch&&r.checkpoint.root===rows[0].checkpoint.root);
 const before=await read();if(!common(before))throw Error('Common checkpoint required before delay');
 const minimum=BigInt(before[0].checkpoint.epoch)+2n;
 let attempted=false;
 try{
  attempted=true; // SSH can lose its response after tc applied the mutation.
  await tc(['qdisc','add','dev','eth0','root','handle','7a00:','netem','delay','500ms']);
  const start=clock(),deadline=start+holdMs+progressMs;
  await record({event:'asymmetric-delay-start',egressMilliseconds:500,holdMs,progressMs});
  while(clock()<=deadline){
   const stats=await qdiscs();
   if(!stats.some(q=>q.kind==='netem'&&q.handle==='7a00:'))throw Error('Delay disappeared');
   const rows=await read();
   if(clock()-start>=holdMs&&common(rows)&&BigInt(rows[0].checkpoint.epoch)>=minimum){
    if(!stats.some(q=>q.kind==='netem'&&Number(q.packets)>0))throw Error('No packets traversed delay');
    await record({event:'asymmetric-delay-finality',rows,qdiscs:stats});return;
   }
   if(clock()>=deadline)break;
   await wait(Math.min(3000,deadline-clock()));
  }
  throw Error('Asymmetric finality deadline exceeded');
 }finally{
  if(attempted){
   const stats=await qdiscs();
   if(stats.some(q=>q.kind==='netem'&&q.handle==='7a00:'))await tc(['qdisc','del','dev','eth0','root','handle','7a00:']);
   if((await qdiscs()).some(q=>q.kind==='netem'))throw Error('Delay restoration failed');
   await record({event:'asymmetric-delay-restored'});
  }
 }
}
