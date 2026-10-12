import test from 'node:test';
import assert from 'node:assert/strict';
const slots=8n,budget=1024n;
function schedule(head,finalBoundary){
 assert.ok(head>=0n&&finalBoundary>=head&&finalBoundary%slots===0n);
 const chunks=[];let current=head;
 while(current<finalBoundary){
  const remaining=finalBoundary-current,next=current+(remaining<budget?remaining:budget),target=next/slots*slots;
  assert.ok(target>current&&target<=finalBoundary&&target-current<=budget&&target%slots===0n);
  chunks.push({from:current,to:target,work:target-current});current=target;
 }
 return chunks;
}
for(const [name,head,target,expected] of [
 ['aligned first full chunk',0n,1024n,[1024n]],
 ['offset first chunk',3n,1024n,[1021n]],
 ['actual authenticated V37 head',27355n,28376n,[1021n]],
 ['offset head and small final boundary',3n,8n,[5n]],
 ['offset head and short final chunk',3n,1032n,[1021n,8n]],
 ['already complete at boundary',1024n,1024n,[]]
])test(name,()=>assert.deepEqual(schedule(head,target).map(x=>x.work),expected));
test('full seven-day range remains finite with at most99 chunks for aligned and offset heads',()=>{
 const sevenDays=604800000n,stepMs=6144000n;assert.equal((sevenDays-1n)/stepMs+1n,99n);
 for(const head of [0n,3n,27355n]){
  const final=(head+sevenDays/6000n)/slots*slots,chunks=schedule(head,final);
  assert.ok(chunks.length<=99);assert.equal(chunks.at(-1)?.to??head,final);
  assert.equal(chunks.reduce((sum,x)=>sum+x.work,0n),final-head);
  assert.ok(chunks.every(x=>x.work<=1024n&&x.to%8n===0n));
 }
});
test('recorded unaligned V37 counterexample is preserved rather than weakening telemetry',()=>{
 assert.equal((27355n+1024n)%8n,3n);assert.equal(schedule(27355n,28384n)[0].to,28376n);
});
test('final target cannot be backwards or off an epoch boundary',()=>{
 for(const [head,target] of [[3n,0n],[3n,1025n]])assert.throws(()=>schedule(head,target));
});
