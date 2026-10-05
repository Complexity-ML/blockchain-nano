import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
// Await genuine live process + local protocol readiness before first RPC read.
// No RPC refusal is silently accepted as a consensus qualification.
export async function waitRestoredReadiness({sample,record,clock=Date.now,wait=delay,maximumMs=180000}){
 assert.ok(Number.isSafeInteger(maximumMs)&&maximumMs>0);const start=clock(),deadline=start+maximumMs;
 while(clock()<deadline){const rows=await sample();assert.equal(rows.length,3);assert.deepEqual([...new Set(rows.map(x=>x.name))].sort(),['mac-docker','mac-native','vps-docker']);
  for(const row of rows){assert.equal(row.running,true,row.name+' stopped during startup');assert.equal(typeof row.networkReady,'boolean');}
  await record({event:'restored-readiness',elapsedMs:clock()-start,rows});
  if(rows.every(x=>x.networkReady))return {elapsedMs:clock()-start,rows};
  await wait(Math.min(1000,Math.max(0,deadline-clock())));
 }
 throw Error('Three restored nodes did not establish local IPC readiness before fixed startup deadline');
}
