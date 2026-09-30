import test from 'node:test';
import assert from 'node:assert/strict';
import {waitRestoredRecovery} from './restore-recovery.mjs';
const rows=(epoch,peers=1)=>['native','docker','vps'].map(name=>({name,peers:String(peers),checkpoint:{epoch:String(epoch),root:'root'+epoch}}));
const saved={epoch:'0x2',root:'root2'};
function fixture(samples){let time=0,index=0;const events=[];return {args:{saved,recoveryMs:12000,clock:()=>time,wait:async n=>{time+=n;},record:async e=>events.push(e),sample:async()=>samples[Math.min(index++,samples.length-1)]},events};}
test('requires reconnect and strictly newer common finality before stability',async()=>{const f=fixture([rows(2),rows(3,0),rows(3)]);const result=await waitRestoredRecovery(f.args);assert.equal(result.elapsedMs,10000);assert.equal(result.checkpoint.epoch,'3');});
test('unchanged baseline cannot pass',async()=>{const f=fixture([rows(2)]);await assert.rejects(waitRestoredRecovery(f.args),/deadline/);assert.equal(f.events.filter(e=>e.event==='backup-restore-recovery-bound').length,0);});
test('disconnected observers cannot pass',async()=>{const f=fixture([rows(3,0)]);await assert.rejects(waitRestoredRecovery(f.args),/deadline/);});
test('conflicting finality is fatal without waiting out deadline',async()=>{const r=rows(2);r[1].checkpoint.root='different';const f=fixture([r]);await assert.rejects(waitRestoredRecovery(f.args),/Conflicting/);assert.equal(f.events.length,0);});
test('regression is fatal',async()=>{const f=fixture([rows(3,0),rows(2)]);await assert.rejects(waitRestoredRecovery(f.args),/regressed/);});
test('duplicate transports do not prove three-node recovery',async()=>{const r=rows(3);r[2].name=r[1].name;await assert.rejects(waitRestoredRecovery(fixture([r]).args),/distinct/);});
