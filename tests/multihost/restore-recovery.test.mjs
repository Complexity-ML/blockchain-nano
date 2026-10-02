import test from 'node:test';
import assert from 'node:assert/strict';
import {waitRestoredRecovery} from './restore-recovery.mjs';
const rows=(epoch,peers=1)=>['native','docker','vps'].map(name=>({name,peers:String(peers),checkpoint:{epoch:String(epoch),root:'0x'+String(epoch).padStart(64,'0')}}));
const saved={epoch:'0x2',root:'0x'+'2'.padStart(64,'0')};
function fixture(samples){let time=0,index=0;const events=[];return {args:{saved,recoveryMs:12000,clock:()=>time,wait:async n=>{time+=n;},record:async e=>events.push(e),sample:async()=>samples[Math.min(index++,samples.length-1)]},events};}
test('requires reconnect and strictly newer common finality before stability',async()=>{const f=fixture([rows(2),rows(3,0),rows(3)]);const result=await waitRestoredRecovery(f.args);assert.equal(result.elapsedMs,10000);assert.equal(result.checkpoint.epoch,'3');});
test('unchanged baseline cannot pass',async()=>{const f=fixture([rows(2)]);await assert.rejects(waitRestoredRecovery(f.args),/deadline/);assert.equal(f.events.filter(e=>e.event==='backup-restore-recovery-bound').length,0);});
test('disconnected observers cannot pass',async()=>{const f=fixture([rows(3,0)]);await assert.rejects(waitRestoredRecovery(f.args),/deadline/);});
test('conflicting finality is fatal without waiting out deadline',async()=>{const r=rows(2);r[1].checkpoint.root='0x'+'a'.repeat(64);const f=fixture([r]);await assert.rejects(waitRestoredRecovery(f.args),/Conflicting/);assert.equal(f.events.length,0);});
test('regression is fatal',async()=>{const f=fixture([rows(3,0),rows(2)]);await assert.rejects(waitRestoredRecovery(f.args),/regressed/);});
test('duplicate transports do not prove three-node recovery',async()=>{const r=rows(3);r[2].name=r[1].name;await assert.rejects(waitRestoredRecovery(fixture([r]).args),/distinct/);});

for(const [name,alter] of [['missing root',r=>delete r[0].checkpoint.root],['short root',r=>r[0].checkpoint.root='0x1'],['negative epoch',r=>r[0].checkpoint.epoch='-1'],['leading-zero epoch',r=>r[0].checkpoint.epoch='03'],['negative peers',r=>r[0].peers='-1'],['fractional peers',r=>r[0].peers='0.5'],['missing peers',r=>delete r[0].peers]])test('malformed sample fails before credit: '+name,async()=>{const r=rows(3);alter(r);const f=fixture([r]);await assert.rejects(waitRestoredRecovery(f.args),/Malformed/);assert.equal(f.events.length,0);});
test('missing roots on all three observers cannot fabricate shared finality',async()=>{const r=rows(3);for(const row of r)delete row.checkpoint.root;await assert.rejects(waitRestoredRecovery(fixture([r]).args),/Malformed/);});
for(const recoveryMs of [0,-1,Infinity,0.5])test('invalid recovery deadline rejected: '+recoveryMs,async()=>{const f=fixture([rows(3)]);await assert.rejects(waitRestoredRecovery({...f.args,recoveryMs}),/baseline or deadline/);assert.equal(f.events.length,0);});
test('canonical hexadecimal peer count and checkpoint epoch accepted',async()=>{const r=rows(3);for(const row of r){row.peers='0x1';row.checkpoint.epoch='0x3';}const result=await waitRestoredRecovery(fixture([r]).args);assert.equal(result.checkpoint.epoch,'0x3');});
