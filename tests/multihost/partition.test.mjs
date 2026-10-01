import {test} from 'node:test';
import assert from 'node:assert/strict';
import {partitionRemote} from './partition.mjs';
function fixture(mode='pass'){
 let attached=true,time=0,didCut=false;const calls=[],events=[];
 const container='nano-qualification-vps-test',network=container+'-net',revision='a'.repeat(40);
 const docker=async args=>{
  calls.push(args);
  if(args[0]==='inspect')return JSON.stringify({State:{Running:true},Config:{Labels:{'nano.qualification':mode==='wrong-label'?'other':'multihost','org.opencontainers.image.revision':revision}},NetworkSettings:{Networks:attached?{[network]:{IPAddress:'172.30.0.2'}}:{}}});
  if(args[1]==='disconnect'){attached=false;didCut=true;if(mode==='lost-response')throw Error('SSH response lost');}
  if(args[1]==='connect'){if(mode==='restore-fails')throw Error('connect failed');attached=true;}
 };
 const sample=async()=>{
  if(didCut&&!attached&&mode==='rpc-fails')throw Error('RPC unavailable');
  return ['mac-native','mac-docker','vps-docker'].map((name,i)=>({name,peers:!attached&&i===2&&mode!=='peers-remain'?'0x0':'0x1',checkpoint:{epoch:didCut&&attached&&mode==='regression'?'1':didCut&&attached&&mode!=='no-recovery'?'3':'2',root:'0x'+(mode==='conflict'&&didCut&&i===2?'b':'a').repeat(64)}}));
 };
 return {args:{docker,container,network,revision,sample,record:async e=>events.push(e),clock:()=>time,wait:async ms=>{time+=ms;},cutMs:10000,recoveryMs:10000},calls,events,attached:()=>attached,setMode:value=>{mode=value;}};
}
test('cut leaves node queryable, restores original IP, requires later common finality',async()=>{
 const f=fixture();await partitionRemote(f.args);assert.equal(f.attached(),true);
 assert.ok(f.calls.some(c=>c.join(' ')==='network connect --ip 172.30.0.2 nano-qualification-vps-test-net nano-qualification-vps-test'));
 assert.equal(f.events.at(-1).event,'partition-recovery-bound');
});
test('workload runs only while isolated and settles before restoration',async()=>{
 const f=fixture();let completed=false;
 f.args.duringCut=async()=>{assert.equal(f.attached(),false);await Promise.resolve();assert.equal(f.attached(),false);completed=true;};
 await partitionRemote(f.args);assert.equal(completed,true);assert.equal(f.attached(),true);
 const names=f.events.map(e=>e.event);
 assert.ok(names.indexOf('partition-disconnected')<names.indexOf('partition-workload-start'));
 assert.ok(names.indexOf('partition-workload-finished')<names.indexOf('partition-reconnected'));
});
test('failed workload restores the network without reporting recovery success',async()=>{
 const f=fixture();f.args.duringCut=async()=>{throw Error('Transaction rejected');};
 await assert.rejects(partitionRemote(f.args),/Transaction rejected/);
 assert.equal(f.attached(),true);assert.ok(!f.events.some(e=>e.event==='partition-recovery-bound'));
});
test('observer and workload failures are both retained and restored',async()=>{
 const f=fixture('rpc-fails');f.args.duringCut=async()=>{throw Error('Transaction rejected');};
 await assert.rejects(partitionRemote(f.args),e=>e instanceof AggregateError&&e.errors.length===2);
 assert.equal(f.attached(),true);
});
test('lost disconnect response never starts the workload',async()=>{
 const f=fixture('lost-response');let called=false;f.args.duringCut=async()=>{called=true;};
 await assert.rejects(partitionRemote(f.args),/SSH response lost/);
 assert.equal(called,false);assert.equal(f.attached(),true);
});
for(const [mode,error] of [['lost-response',/SSH response lost/],['rpc-fails',/RPC unavailable/],['conflict',/Conflicting finalized/],['no-recovery',/recovery deadline/]])test(mode+' rejects and restores',async()=>{
 const f=fixture(mode);await assert.rejects(partitionRemote(f.args),error);assert.equal(f.attached(),true);
});
test('restoration failure cannot become success',async()=>{
 const f=fixture('restore-fails');await assert.rejects(partitionRemote(f.args),/connect failed/);assert.equal(f.attached(),false);
});
test('wrong provenance refuses mutation',async()=>{
 const f=fixture('wrong-label');await assert.rejects(partitionRemote(f.args),/provenance/);assert.ok(f.calls.every(c=>c[0]==='inspect'));
});

test('peer isolation must be observed, not inferred from Docker success',async()=>{
 const f=fixture('peers-remain');f.args.cutMs=35000;
 await assert.rejects(partitionRemote(f.args),/still has peers/);assert.equal(f.attached(),true);
});
test('finality regression after reconnection is rejected',async()=>{
 const f=fixture('regression');await assert.rejects(partitionRemote(f.args),/Finality regressed/);assert.equal(f.attached(),true);
});

test('late recovery reuses history without another network cut',async()=>{
 const f=fixture('no-recovery');let deadline;
 try{await partitionRemote(f.args);}catch(error){deadline=error;}
 assert.equal(typeof deadline?.waitForRecovery,'function');f.setMode('pass');
 await deadline.waitForRecovery();assert.equal(f.events.at(-1).event,'late-partition-recovery-bound');
 assert.equal(f.calls.filter(c=>c[1]==='disconnect').length,1);
});
for(const mode of ['conflict','regression'])test('late '+mode+' remains fatal',async()=>{
 const f=fixture('no-recovery');let deadline;
 try{await partitionRemote(f.args);}catch(error){deadline=error;}
 f.setMode(mode);await assert.rejects(deadline.waitForRecovery(),/Conflicting finalized|Finality regressed/);
});
test('late recovery is bounded too',async()=>{
 const f=fixture('no-recovery');let deadline;
 try{await partitionRemote(f.args);}catch(error){deadline=error;}
 await assert.rejects(deadline.waitForRecovery(),/Late recovery observation deadline/);
 assert.equal(f.args.clock(),30000);
});

for(const [latency,reject] of [[10000,false],[10001,true]])test(`recovery read completed at ${latency}ms observes the inclusive deadline`,async()=>{
 const f=fixture(),original=f.args.sample;
 f.args.sample=async()=>{const rows=await original();if(f.calls.some(c=>c[1]==='connect'))await f.args.wait(latency);return rows;};
 if(reject){await assert.rejects(partitionRemote(f.args),/recovery deadline/);assert.ok(!f.events.some(e=>e.event==='partition-recovery-bound'));}
 else await partitionRemote(f.args);
 assert.equal(f.attached(),true);
});
test('reconnection record latency cannot extend the recovery budget',async()=>{
 const f=fixture(),original=f.args.record;
 f.args.record=async e=>{await original(e);if(e.event==='partition-reconnected')await f.args.wait(10001);};
 await assert.rejects(partitionRemote(f.args),/recovery deadline/);assert.equal(f.attached(),true);
 assert.ok(!f.events.some(e=>e.event==='partition-recovery-bound'));
});
