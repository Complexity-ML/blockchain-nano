import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const base=process.argv[2];const initial=readFileSync(resolve(base,'initial-full.log'),'utf8');const final=readFileSync(resolve(base,'final-full.log'),'utf8');
assert(initial.includes('timeout: Relay exits when supervisor dies'));assert(initial.includes('Test Failed.'));
const expected=['supervisor','supervisor_parent_crash','libp2p_fragment_faults_paris','libp2p_fragment_faults_shanghai'];
for(const name of expected){const begin=final.indexOf(`Testing: ${name}\n`);assert(begin>=0,`missing ${name}`);const next=final.indexOf('Testing: ',begin+9);assert(final.slice(begin,next<0?final.length:next).includes('Test Passed.'),`failed ${name}`);}
const rows=final.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
const crash=rows.filter(r=>r.scenario==='supervisor-parent-crash');assert.equal(crash.length,1);assert.equal(crash[0].parentSignal,'SIGKILL');for(const field of ['orphanRelayExited','staleParentRejected','identityPreserved','restartedShutdown'])assert.equal(crash[0][field],true,field);
const transfers=rows.filter(r=>r.scenario==='libp2p-fragment-faults');assert.equal(transfers.length,2);assert.deepEqual(transfers.map(r=>r.fork).sort(),['Paris','Shanghai']);
for(const r of transfers){assert.equal(r.faults,true);assert.equal(r.fragments,40);assert.equal(r.receiptCount,10);assert.equal(r.reopenedAudits,2);assert.equal(r.managedNodes,2);assert(r.compactBytes>16*1024*1024);assert.equal(r.faultEvidence.failedAttempt.fragments,1);assert.equal(r.faultEvidence.droppedFragment.offset,'0x40000');}
console.log(JSON.stringify({verified:true,initialFailureReproduced:true,passedSuites:4,managedFragmentProfiles:['Paris','Shanghai'],scope:'Local managed parent SIGKILL and startup-parent mismatch, environment pin override, normal child cleanup, child failure propagation and certified-block transfer fault regressions. Not interrupted distributed BFT proposals under supervisor, multi-host or physical power loss.'},null,2));
