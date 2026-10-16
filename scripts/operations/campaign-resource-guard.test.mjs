// New pure tests only. No subprocess, host sample, Docker, SSH or launcher.
import test from 'node:test';
import assert from 'node:assert/strict';
import {policy,parseVmStat,parseLinuxCapacity,evaluateResources,ownedStopLatch,supervise,ResourceGuardError} from './campaign-resource-guard.mjs';
const GiB=1024n**3n;
const surface=(memoryMetric,disk=4n*GiB,memory=3n*GiB)=>({diskBytes:String(disk),memoryBytes:String(memory),memoryMetric});
const sample=(now=0)=>({startedMs:now,finishedMs:now,surfaces:{macHost:surface('mac-free-inactive-speculative-estimate'),macDockerVm:surface('linux-MemAvailable'),vpsHost:surface('linux-MemAvailable')}});
const vm=`Mach Virtual Memory Statistics: (page size of 16384 bytes)\nPages free: 4072.\nPages active: 12345.\nPages inactive: 429228.\nPages speculative: 49476.\nPages purgeable: 1200.\n`;
const linux=(rows=1,mem='3145728')=>'NANO_DF_BEGIN\nFilesystem 1024-blocks Used Available Capacity Mounted on\n'+Array.from({length:rows},(_,i)=>`/dev/test 8388608 4194304 4194304 50% /mount${i}\n`).join('')+'NANO_DF_END\nNANO_MEM_KIB:'+mem+'\n';

test('Mac memory is explicitly free+inactive+speculative, not purgeable or os.freemem',()=>{
 const r=parseVmStat(vm,String(24n*GiB));assert.equal(r.memoryBytes,String((4072n+429228n+49476n)*16384n));assert.equal(r.memoryMetric,'mac-free-inactive-speculative-estimate');
 assert.throws(()=>parseVmStat(vm+'Pages free: 1.\n',String(24n*GiB)),/mac-duplicate-counter/);
 assert.throws(()=>parseVmStat(vm.replace('Pages inactive: 429228.\n',''),String(24n*GiB)),/mac-missing-counter/);
 assert.throws(()=>parseVmStat(vm.replace('4072.','18446744073709551616.'),String(24n*GiB)),/mac-page-counter/);
 assert.throws(()=>parseVmStat(vm,String(GiB)),/mac-memory-overflow/);
});

test('Linux df/MemAvailable parse exactly and fail closed on missing/duplicate/overflow',()=>{
 const r=parseLinuxCapacity(linux(3),3);assert.equal(r.diskBytes,String(4n*GiB));assert.equal(r.memoryBytes,String(3n*GiB));
 assert.throws(()=>parseLinuxCapacity(linux(1),3),/linux-df-rows/);
 assert.throws(()=>parseLinuxCapacity(linux(1)+'NANO_MEM_KIB:9\n',1),/linux-memory-rows/);
 assert.throws(()=>parseLinuxCapacity(linux(1,'18446744073709551615'),1),/linux-capacity-overflow/);
 assert.throws(()=>parseLinuxCapacity(linux().replace('NANO_MEM_KIB:3145728','unknown'),1),/linux-memory-row/);
});

test('startup and running boundaries, all three surfaces, freshness and metric identity',()=>{
 assert.equal(evaluateResources(sample(),'startup',0),true);const running=sample();for(const row of Object.values(running.surfaces)){row.diskBytes=String(2n*GiB);row.memoryBytes=String(GiB);}assert.equal(evaluateResources(running,'running',0),true);
 for(const name of Object.keys(running.surfaces)){const low=structuredClone(running);low.surfaces[name].diskBytes=String(2n*GiB-1n);assert.throws(()=>evaluateResources(low,'running',0),/disk-below-running/);const lowMemory=structuredClone(running);lowMemory.surfaces[name].memoryBytes=String(GiB-1n);assert.throws(()=>evaluateResources(lowMemory,'running',0),/memory-below-running/);}
 const missing=sample();delete missing.surfaces.macDockerVm;assert.throws(()=>evaluateResources(missing,'startup',0),/resource-surfaces/);
 assert.throws(()=>evaluateResources(sample(),'startup',5001),/sample-stale-or-invalid/);assert.throws(()=>evaluateResources(sample(1),'startup',0),/sample-stale-or-invalid/);
 const wrong=sample();wrong.surfaces.macHost.memoryMetric='linux-MemAvailable';assert.throws(()=>evaluateResources(wrong,'startup',0),/memory-metric/);
});

test('owned stop latch sends one SIGINT only; never protected/global PID or identity replacement',()=>{
 const signals=[],child={pid:42424,closed:false,kill:s=>{signals.push(s);return true;}},stop=ownedStopLatch(child);assert.equal(stop(),true);assert.equal(stop(),false);assert.deepEqual(signals,['SIGINT']);
 assert.throws(()=>ownedStopLatch({...child,pid:28722}),/owned-child-identity/);assert.throws(()=>ownedStopLatch({...child,pid:-1}),/owned-child-identity/);
 const changed={...child,pid:42425},stopChanged=ownedStopLatch(changed);changed.pid=42426;assert.throws(stopChanged,/owned-child-identity-changed/);
});

test('three probe-only samples never call launcher; failed startup never spawns',async()=>{
 let now=0,launches=0,calls=0;const deps={clock:()=>now,sleep:async ms=>{now+=ms;},sample:async()=>{++calls;return sample(now);},launch:()=>{++launches;throw Error('unexpected');}};
 const result=await supervise({...deps,probeOnly:true});assert.equal(result.passed,true);assert.equal(calls,3);assert.equal(launches,0);assert.equal(result.launcherStarted,false);
 calls=0;const refusal=await supervise({...deps,sample:async()=>{++calls;throw new ResourceGuardError('ssh-sample-timeout');}});assert.equal(refusal.passed,false);assert.equal(refusal.reason,'ssh-sample-timeout');assert.equal(launches,0);assert.equal(calls,1);
});

test('lost SSH while running signals only owned child once and waits for cleanup close',async()=>{
 let now=0,calls=0,signals=0;const child={pid:42424,closed:false,exitCode:null,signalCode:null,kill:s=>{assert.equal(s,'SIGINT');++signals;return true;}};
 const r=await supervise({clock:()=>now,sleep:async ms=>{now+=ms;if(signals){child.closed=true;child.exitCode=1;}},sample:async()=>{if(++calls===4)throw new ResourceGuardError('ssh-sample-timeout');return sample(now);},launch:()=>child});
 assert.equal(r.passed,false);assert.equal(r.reason,'ssh-sample-timeout');assert.equal(signals,1);assert.equal(r.launcherJoined,true);assert.equal(r.sigintSent,true);
});

test('wall timeout and unjoined grace remain failures, with no SIGTERM/SIGKILL escalation',async()=>{
 let now=0,active=false,signals=0;const child={pid:42424,closed:false,exitCode:null,signalCode:null,kill:s=>{assert.equal(s,'SIGINT');++signals;return true;}};
 const r=await supervise({clock:()=>now,sleep:async ms=>{now+=active&&!signals?policy.maximumRunMs:ms;},sample:async()=>sample(now),launch:()=>{active=true;return child;}});
 assert.equal(r.reason,'campaign-wall-timeout');assert.equal(r.passed,false);assert.equal(r.launcherUnjoined,true);assert.equal(r.launcherJoined,false);assert.equal(signals,1);assert.equal(now,2*policy.intervalMs+policy.maximumRunMs+policy.cleanupGraceMs);
});

test('a normal joined exit can pass; a stop request before spawn cannot',async()=>{
 let now=0,launched=0;const child={pid:42424,closed:true,exitCode:0,signalCode:null,kill:()=>{throw Error('unexpected signal');}};
 const deps={clock:()=>now,sleep:async ms=>{now+=ms;},sample:async()=>sample(now),launch:()=>{++launched;return child;}};const r=await supervise(deps);assert.equal(r.passed,true);assert.equal(r.launcherJoined,true);assert.equal(launched,1);
 const stop=await supervise({...deps,stopRequested:()=>true});assert.equal(stop.reason,'wrapper-interrupted');assert.equal(launched,1);
});
