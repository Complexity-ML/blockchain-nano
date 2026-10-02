import {setTimeout as delay} from 'node:timers/promises';
// A bounded reconciliation phase precedes the unchanged strict stability gate.
// Its baseline/deadline never reset, and conflicting/regressing finality fails.
export async function waitRestoredRecovery({sample,record,saved,clock=Date.now,wait=delay,recoveryMs=180000}) {
 const quantity=value=>typeof value==='string'&&/^(?:0|[1-9][0-9]*|0x(?:0|[1-9a-f][0-9a-f]*))$/.test(value);
 const checkpoint=value=>value&&quantity(value.epoch)&&/^0x[0-9a-f]{64}$/.test(value.root);
 if(!checkpoint(saved)||!Number.isSafeInteger(recoveryMs)||recoveryMs<=0)throw Error('Invalid recovery baseline or deadline');
 const baseline=BigInt(saved.epoch),roots=new Map([[String(baseline),saved.root]]),previous=new Map();
 const start=clock(),deadline=start+recoveryMs;
 while(clock()<deadline){
  const rows=await sample();
  if(!Array.isArray(rows)||rows.length!==3||rows.some(r=>!r||typeof r.name!=='string'||!r.name)||new Set(rows.map(r=>r.name)).size!==3)throw Error('Three distinct restore observers required');
  for(const row of rows){
   if(!checkpoint(row.checkpoint)||!quantity(row.peers))throw Error('Malformed restore observer evidence');
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
