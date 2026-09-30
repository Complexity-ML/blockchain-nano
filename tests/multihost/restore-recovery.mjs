import {setTimeout as delay} from 'node:timers/promises';
// A bounded reconciliation phase precedes the unchanged strict stability gate.
// Its baseline/deadline never reset, and conflicting/regressing finality fails.
export async function waitRestoredRecovery({sample,record,saved,clock=Date.now,wait=delay,recoveryMs=180000}) {
 const baseline=BigInt(saved.epoch),roots=new Map([[String(baseline),saved.root]]),previous=new Map();
 const start=clock(),deadline=start+recoveryMs;
 while(clock()<deadline){
  const rows=await sample();
  if(!Array.isArray(rows)||rows.length!==3||new Set(rows.map(r=>r.name)).size!==3)throw Error('Three distinct restore observers required');
  for(const row of rows){
   const epoch=BigInt(row.checkpoint.epoch),key=String(epoch),root=row.checkpoint.root;
   if(previous.has(row.name)&&epoch<previous.get(row.name))throw Error('Restore finality regressed');
   if(roots.has(key)&&roots.get(key)!==root)throw Error('Conflicting restored finality');
   previous.set(row.name,epoch);roots.set(key,root);
  }
  await record({event:'backup-restore-recovery-sample',elapsedMs:clock()-start,rows});
  const first=rows[0].checkpoint;
  if(rows.every(r=>BigInt(r.peers)>0n&&r.checkpoint.root===first.root&&BigInt(r.checkpoint.epoch)===BigInt(first.epoch))&&BigInt(first.epoch)>baseline){
   const result={event:'backup-restore-recovery-bound',elapsedMs:clock()-start,recoveryMs,checkpoint:first};await record(result);return result;
  }
  await wait(Math.min(5000,Math.max(0,deadline-clock())));
 }
 throw Error('Restored shared finality recovery deadline exceeded');
}
