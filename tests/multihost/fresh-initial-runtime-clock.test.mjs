// Necessary clock arithmetic and effect ordering only. No process/network,
// fixture export, key, journal, database or real runtime is used by this test.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {checkFreshInitialRuntimeClock,launchFreshInitialRuntime} from './prepare.mjs';
const q=n=>'0x'+BigInt(n).toString(16),g=1000000n,a=g+6000n;
function input({downtime=300000n,steps=2n,step=120000n,replay=64n}={}){
 const approvedClock={maximumStartupElapsedMs:q(120000n),maximumDowntimeMs:q(downtime),maximumSteps:q(steps)};
 return {plan:{anchor:{genesisTimeMs:q(g),timeMs:q(a)}},node:{capella:{clock:{maximumForwardStepMs:q(step),maximumStartupElapsedMs:approvedClock.maximumStartupElapsedMs,startupCatchup:{maximumDowntimeMs:approvedClock.maximumDowntimeMs,maximumSteps:approvedClock.maximumSteps}},limits:{replaySlots:q(replay)}}},profile:{common:[[],q(6000n)]},approvedClock};
}
function checked(x,now){const before=structuredClone(x),r=checkFreshInitialRuntimeClock({...x,nowMs:now});assert.deepEqual(x,before);assert.equal(r.necessaryConditionsPassed,true);assert.equal(r.runtimeStartupVerified,false);assert.equal(r.productionQualified,false);return r;}
function refused(x,now,pattern){const before=structuredClone(x);assert.throws(()=>checkFreshInitialRuntimeClock({...x,nowMs:now}),pattern);assert.deepEqual(x,before);}
test('current durable age accepts equality and refuses the next millisecond of step capacity',()=>{
 const x=input();assert.equal(checked(x,a).requiredCatchupSteps,'0x0');assert.equal(checked(x,a+240000n).requiredCatchupSteps,'0x2');
 refused(x,a+240001n,/current age exceeds catch-up capacity/);
 const one=input({steps:1n});assert.equal(checked(one,a+120000n).requiredCatchupSteps,'0x1');refused(one,a+120001n,/current age exceeds catch-up capacity/);
});
test('the existing conservative genesis-age bound remains distinct from durable age',()=>{
 const x=input({downtime:240000n,steps:3n});const r=checked(x,g+240000n);assert.equal(r.ageSinceGenesisMs,q(240000n));assert.equal(r.ageSinceDurableMs,q(234000n));
 refused(x,g+240001n,/approved age since genesis/);refused(x,a-1n,/precedes durable anchor/);
 const future=structuredClone(x);future.plan.anchor.genesisTimeMs=q(a+1n);refused(future,a,/precedes durable anchor/);
});
test('approved budgets and replay-step capacity cannot be changed by generated config',()=>{
 const x=input();checked(x,a);refused(input({replay:19n}),a,/step exceeds replay-slot capacity/);
 for(const field of ['maximumDowntimeMs','maximumSteps']){const altered=structuredClone(x);altered.node.capella.clock.startupCatchup[field]=q(BigInt(altered.approvedClock[field])+1n);refused(altered,a,/differs from approved startup budgets/);}
 const elapsed=structuredClone(x);elapsed.node.capella.clock.maximumStartupElapsedMs=q(1n);refused(elapsed,a,/differs from approved startup budgets/);
 for(const value of ['0x0','0x01','0x10000000000000000',1]){const altered=structuredClone(x);altered.node.capella.clock.maximumForwardStepMs=value;refused(altered,a,/Initial clock/);}
 for(const now of [-1n,1n<<64n,Number(a)])refused(x,now,/Initial clock/);
});
test('each initial launch samples a new wall time and a peer wait cannot reuse a prior pass',()=>{
 const x=input({steps:1n});let wall=a+120000n,reads=0,spawns=0;
 const now=()=>{++reads;return wall;},launch=proof=>{assert.equal(proof.requiredCatchupSteps,'0x1');++spawns;return 'started';};
 // Preparation and the first owner's check pass. No receipt authorizes a later
 // owner once the peer/tunnel/configuration wait crosses the same fixed budget.
 checked(x,wall);assert.equal(launchFreshInitialRuntime(x,now,launch),'started');assert.equal(spawns,1);
 wall+=1n;assert.throws(()=>launchFreshInitialRuntime(x,now,launch),/current age exceeds catch-up capacity/);assert.equal(spawns,1);assert.equal(reads,2);
 // The three successful owner calls also require three independently sampled
 // clocks; no real spawn is substituted for these counted effect callbacks.
 wall=a+100000n;const owners=[];for(const owner of ['vps-docker','mac-docker','mac-native'])launchFreshInitialRuntime(x,now,()=>owners.push(owner));assert.deepEqual(owners,['vps-docker','mac-docker','mac-native']);assert.equal(reads,5);
 assert.throws(()=>launchFreshInitialRuntime(x,null,launch),/callbacks required/);assert.equal(spawns,1);
});
