import test from 'node:test';
import assert from 'node:assert/strict';
import {createCampaignDeadline} from './campaign-deadline.mjs';

// Synthetic monotone milliseconds only. No real wait, command or campaign.
function clockFixture(at){
 let current=at,next=1,maximumActive=0;
 const timers=new Map(),cleared=[];
 const now=()=>current;
 const setTimer=(callback,delay)=>{
  assert.equal(typeof callback,'function');assert(Number.isFinite(delay)&&delay>0);
  const id=next++;timers.set(id,{callback,due:current+delay});
  maximumActive=Math.max(maximumActive,timers.size);assert(timers.size<=1,'One active campaign timer');return id;
 };
 const clearTimer=id=>{cleared.push(id);timers.delete(id);};
 function advance(target){
  assert(Number.isFinite(target)&&target>=current);let callbacks=0;
  for(;;){
   const first=[...timers].sort((a,b)=>a[1].due-b[1].due||a[0]-b[0])[0];
   if(!first||first[1].due>target)break;
   assert(++callbacks<=16,'Bounded fake timer callbacks');timers.delete(first[0]);current=first[1].due;first[1].callback();
  }
  current=target;
 }
 return {now,setTimer,clearTimer,advance,timers,cleared,get maximumActive(){return maximumActive;}};
}
function budget(clock,options){
 const events=[],deadline=createCampaignDeadline({...options,now:clock.now,setTimer:clock.setTimer,clearTimer:clock.clearTimer,onExpire:event=>events.push(event)});
 return {deadline,events};
}

test('campaign budget includes preparation elapsed since the supplied start',()=>{
 const clock=clockFixture(1250),{deadline,events}=budget(clock,{startedAt:1000,maximumRunMs:1000,closeoutReserveMs:200});
 try{
  assert.equal(deadline.phase,'work');assert.equal(deadline.workEnd,1800);assert.equal(deadline.overallEnd,2000);assert.equal(deadline.deadline,1800);
  assert.equal(deadline.remaining(9999),550);assert.equal(deadline.remaining(23),23);assert.equal(clock.timers.size,1);
  clock.advance(1799);assert.equal(deadline.signal.aborted,false);assert.equal(deadline.remaining(9999),1);
  clock.advance(1800);assert.equal(deadline.signal.aborted,true);assert.match(deadline.signal.reason.message,/^Fresh campaign work deadline$/);
  assert.throws(()=>deadline.check(),/^Error: Fresh campaign work deadline$/);
  assert.deepEqual(events,[{phase:'work',deadline:1800,elapsedMs:800}]);assert.equal(clock.timers.size,0);assert.equal(clock.maximumActive,1);
 }finally{deadline.dispose();}
});

test('expired work still permits a separate closeout signal until the original overall end',()=>{
 const clock=clockFixture(0),{deadline,events}=budget(clock,{startedAt:0,maximumRunMs:100,closeoutReserveMs:20});
 try{
  const workSignal=deadline.signal;clock.advance(80);assert.equal(workSignal.aborted,true);
  assert.throws(()=>deadline.remaining(1),/^Error: Fresh campaign work deadline$/);
  deadline.enterCloseout();const closeoutSignal=deadline.signal;
  assert.notEqual(closeoutSignal,workSignal);assert.equal(closeoutSignal.aborted,false);assert.equal(workSignal.aborted,true);
  assert.equal(deadline.phase,'closeout');assert.equal(deadline.deadline,100);assert.equal(deadline.remaining(999),20);assert.equal(clock.timers.size,1);
  clock.advance(99);assert.equal(closeoutSignal.aborted,false);clock.advance(100);
  assert.equal(closeoutSignal.aborted,true);assert.match(closeoutSignal.reason.message,/^Fresh campaign closeout deadline$/);
  assert.throws(()=>deadline.check(),/^Error: Fresh campaign closeout deadline$/);
  assert.deepEqual(events,[{phase:'work',deadline:80,elapsedMs:80},{phase:'closeout',deadline:100,elapsedMs:100}]);assert.equal(clock.timers.size,0);
 }finally{deadline.dispose();}
});

test('early or repeated closeout never resets the global deadline or replaces its signal twice',()=>{
 const clock=clockFixture(100),{deadline,events}=budget(clock,{startedAt:100,maximumRunMs:100,closeoutReserveMs:20});
 try{
  const workSignal=deadline.signal;clock.advance(125);deadline.enterCloseout();
  const closeoutSignal=deadline.signal,timer=[...clock.timers.keys()][0],clears=clock.cleared.length;
  assert.equal(workSignal.aborted,true);assert.equal(closeoutSignal.aborted,false);assert.equal(deadline.overallEnd,200);assert.equal(deadline.deadline,200);
  clock.advance(160);deadline.enterCloseout();assert.equal(deadline.signal,closeoutSignal);assert.equal(deadline.deadline,200);assert.equal(deadline.remaining(100),40);
  assert.deepEqual([...clock.timers.keys()],[timer]);assert.equal(clock.cleared.length,clears);assert.equal(events.length,0);
  clock.advance(200);assert.equal(closeoutSignal.aborted,true);deadline.enterCloseout();assert.equal(deadline.signal,closeoutSignal);
  assert.throws(()=>deadline.check(),/^Error: Fresh campaign closeout deadline$/);assert.throws(()=>deadline.check(),/^Error: Fresh campaign closeout deadline$/);
  assert.deepEqual(events,[{phase:'closeout',deadline:200,elapsedMs:100}]);assert.equal(clock.timers.size,0);assert.equal(clock.maximumActive,1);
 }finally{deadline.dispose();}
});

test('command budgets are positive integers and refuse before invocation; dispose cancels the unique timer',()=>{
 const clock=clockFixture(0),{deadline,events}=budget(clock,{startedAt:0,maximumRunMs:100,closeoutReserveMs:20}),invocations=[];
 const invoke=requested=>{const timeout=deadline.remaining(requested);invocations.push(timeout);return timeout;};
 try{
  assert.equal(invoke(12),12);assert.equal(invoke(999),80);
  for(const invalid of [0,-1,0.5,NaN,Infinity,Number.MAX_SAFE_INTEGER+1])assert.throws(()=>invoke(invalid),/^Error: Fresh campaign deadline: requested command timeout$/);
  assert.deepEqual(invocations,[12,80]);clock.advance(79);assert.equal(invoke(999),1);
  clock.advance(79.5);assert.throws(()=>invoke(999),/^Error: Fresh campaign deadline: command budget exhausted$/);assert.deepEqual(invocations,[12,80,1]);
  assert.equal(deadline.signal.aborted,false);assert.equal(clock.timers.size,1);deadline.dispose();
  assert.equal(deadline.signal.aborted,true);assert.match(deadline.signal.reason.message,/^Fresh campaign deadline disposed$/);assert.equal(clock.timers.size,0);
  const clears=clock.cleared.length;deadline.dispose();assert.equal(clock.cleared.length,clears);
  assert.throws(()=>invoke(1),/^Error: Fresh campaign deadline: disposed$/);assert.throws(()=>deadline.check(),/^Error: Fresh campaign deadline: disposed$/);assert.throws(()=>deadline.enterCloseout(),/^Error: Fresh campaign deadline: disposed$/);
  clock.advance(1000);assert.deepEqual(events,[]);assert.deepEqual(invocations,[12,80,1]);assert.equal(clock.maximumActive,1);
 }finally{deadline.dispose();}
});
