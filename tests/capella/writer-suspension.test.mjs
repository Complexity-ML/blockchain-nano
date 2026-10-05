import test from 'node:test';
import assert from 'node:assert/strict';
import {verifyWriterSuspension} from './writer-suspension.mjs';
const expected={containerId:'a'.repeat(64),executable:'/build/runtime/nano',volume:'nano-private-test-data'};
const state=()=>({Id:expected.containerId,Path:expected.executable,State:{Running:true,Paused:false,Restarting:false,Pid:1234,StartedAt:'2026-10-05T06:00:00Z'},Mounts:[{Name:expected.volume,Destination:'/var/lib/nano'}]});
const sample=()=>({namespacePid:1,executable:expected.executable,startTicks:'100000',tasks:[{tid:1,state:'T'},{tid:2,state:'T'}]});
test('binds every stopped writer thread to unchanged container and process identity',()=>{const a=sample();assert.equal(verifyWriterSuspension(state(),state(),[a,structuredClone(a)],expected).writersSuspended,true);});
test('refuses a runnable thread, vanished thread, reused process or wrong volume',()=>{
 for(const alter of [s=>s.tasks[1].state='R',s=>s.tasks.pop(),s=>s.startTicks='100001',s=>s.executable='/bin/sleep',s=>s.tasks.push({tid:2,state:'T'})]){const a=sample(),b=sample();alter(b);assert.throws(()=>verifyWriterSuspension(state(),state(),[a,b],expected));}
 for(const alter of [s=>s.State.Pid++,s=>s.State.StartedAt='other',s=>s.State.Running=false,s=>s.State.Paused=true,s=>s.Id='b'.repeat(64),s=>s.Mounts=[]]){const b=state();alter(b);assert.throws(()=>verifyWriterSuspension(state(),b,[sample(),sample()],expected));}
});
