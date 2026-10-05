import test from 'node:test';
import assert from 'node:assert/strict';
import {durableFinalityObservation as read} from './finality-observation.mjs';
const root=n=>'0x'+n.repeat(64),cp=(epoch,n)=>({epoch:String(epoch),root:root(n)});
const header=n=>({execution_optimistic:false,finalized:true,data:{canonical:true,root:root(n)}});
test('head reorg cannot erase a checkpoint confirmed by the finalized header',()=>{
 const r=read(cp(26,'a'),header('b'),new Map([['26',root('a')],['27',root('b')]]),27n);
 assert.deepEqual(r.checkpoint,cp(27,'b'));assert.deepEqual(r.headStateCheckpoint,cp(26,'a'));
});
test('real finalized-header regression is still rejected',()=>assert.throws(()=>read(cp(26,'a'),header('a'),new Map([['27',root('b')]]),27n),/regressed/));
test('unknown header root stays pending rather than inferring an epoch from its slot',()=>assert.equal(read(cp(26,'a'),{...header('c'),data:{...header('c').data,header:{message:{slot:'215'}}}},new Map([['27',root('b')]]),27n),null));
test('matching state checkpoint supplies the real epoch even if the boundary slot was missed',()=>assert.deepEqual(read(cp(28,'c'),header('c'),new Map(),27n).checkpoint,cp(28,'c')));
test('optimistic, noncanonical and unfinalized responses are refused',()=>{
 for(const h of [{...header('b'),execution_optimistic:true},{...header('b'),finalized:false},{...header('b'),data:{...header('b').data,canonical:false}}])assert.throws(()=>read(cp(27,'b'),h,new Map(),27n),/Unauthenticated/);
});
