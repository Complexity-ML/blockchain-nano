import test from 'node:test';import assert from 'node:assert/strict';
import {verifyPeerSyncing,capturePeerSyncing} from './peer-syncing.mjs';
const body=(slot='7',distance='0')=>({data:{head_slot:slot,sync_distance:distance,is_syncing:distance!=='0',is_optimistic:false,el_offline:false}});
test('slot projection preserves uint64 precision and lag',()=>{
 assert.equal(verifyPeerSyncing(body('9007199254740993','2'),'9007199254740992','9007199254740994').sync_distance,'2');
});
test('invalid responses cannot become qualification evidence',()=>{
 for(const value of [body('07'),body('-1'),body('7','18446744073709551616'),body('18446744073709551615','1'),{data:{...body().data,is_syncing:true}},{data:{...body().data,el_offline:true}},body('3')])assert.throws(()=>verifyPeerSyncing(value,'6','8'));
});
test('503 is retained and never extends observation deadline',async()=>{
 let time=0;const events=[];
 const unavailable=()=>{const error=Error('503');error.name='HttpReadError';error.status=503;throw error;};
 await assert.rejects(capturePeerSyncing({request:unavailable,record:async event=>events.push(event),phase:'fixture',now:()=>time,delay:async ms=>{time+=ms;},budgetMs:6000}),/deadline/);
 assert.equal(time,6000);assert.equal(events.length,6);assert.ok(events.every(e=>e.event==='peer-syncing-unavailable'));
});
test('all three real observers required; malformed data is fatal',async()=>{
 const events=[];const request=async(kind,path)=>path.includes('/headers/')?{data:{header:{message:{slot:'7'}}}}:body();
 const result=await capturePeerSyncing({request,record:async e=>events.push(e),phase:'fixture'});assert.equal(result.rows.length,3);assert.equal(events.length,1);
 await assert.rejects(capturePeerSyncing({request:async(kind,path)=>path.includes('/headers/')?{data:{header:{message:{slot:'7'}}}}:body('07'),record:async()=>{},phase:'fixture'}),/uint64/);
});
