import {test} from 'node:test';
import assert from 'node:assert/strict';
import {sampleClock} from './clock-sample.mjs';
test('asymmetric transport remains an interval, not a midpoint offset',async()=>{
 const times=[1000,1100];const r=await sampleClock(async()=>'1070\n',()=>times.shift());
 assert.deepEqual(r,{localBeforeMs:1000,localAfterMs:1100,remoteMs:1070,offsetMinMs:-30,offsetMaxMs:70,roundTripMs:100});
});
test('invalid timestamps and backwards local clocks are rejected',async()=>{
 for(const [text,times] of [['1000',[1100,1000]],['%s%3N',[1,2]],['9007199254740992',[1,2]]])await assert.rejects(sampleClock(async()=>text,()=>times.shift()));
});
test('transport failure is preserved',async()=>{
 const error=Error('SSH failed');await assert.rejects(sampleClock(async()=>{throw error}),e=>e===error);
});
