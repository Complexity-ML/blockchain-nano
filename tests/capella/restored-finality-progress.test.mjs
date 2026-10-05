import test from 'node:test';
import assert from 'node:assert/strict';
import {restoredFinalityProgress} from './restored-finality-progress.mjs';
const baseline={epoch:'75',root:'base'};
const rows=(epoch,root='r'+epoch)=>['mac-native','mac-docker','vps-docker'].map(name=>({name,peers:'0x1',checkpoint:{epoch:String(epoch),root}}));
test('repeated shared checkpoint after a large epoch gap counts once',()=>{const p=restoredFinalityProgress(baseline);for(let n=0;n<20;n++)assert.equal(p.observe(rows(1284)),null);assert.equal(p.snapshot().distinctAdvances.length,1);});
test('requires three distinct advances and three stable reads of latest',()=>{const p=restoredFinalityProgress(baseline);for(const e of [1284,1285,1286,1286])assert.equal(p.observe(rows(e)),null);assert.deepEqual(p.observe(rows(1286)),{epoch:'1286',root:'r1286'});});
test('disagreement, disconnected peer and duplicate node reset stability',()=>{for(const mutate of [r=>r[2].checkpoint=rows(1287)[0].checkpoint,r=>r[2].peers='0x0',r=>r[2].name=r[0].name]){const p=restoredFinalityProgress(baseline);for(const e of [76,77,78,78])p.observe(rows(e));const bad=rows(78);mutate(bad);assert.equal(p.observe(bad),null);assert.equal(p.observe(rows(78)),null);assert.equal(p.observe(rows(78)),null);assert.ok(p.observe(rows(78)));}});
test('same epoch root conflict and shared regression fail',()=>{const p=restoredFinalityProgress(baseline);p.observe(rows(80));assert.throws(()=>p.observe(rows(80,'foreign')));assert.throws(()=>p.observe(rows(79)));});
