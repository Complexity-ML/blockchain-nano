import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
const finals=rows.filter(r=>r.scenario==='bft-fragment-download-crash');
assert.equal(finals.length,2);assert.deepEqual(finals.map(r=>r.fork).sort(),['Paris','Shanghai']);
for(const r of finals){
 assert.equal(r.validators,4);assert.equal(r.coreProcesses,4);assert.equal(r.relayProcesses,4);assert.equal(r.reopenedAudits,4);assert(r.compactBytes>16*1024*1024);
 assert(r.successfulDownloads>=2);assert(r.distinctTargetReceivers>=2);assert.equal(new Set(r.corePids).size,4);
 const c=r.crash;assert(Number.isInteger(c.victim)&&c.victim>=0&&c.victim<4);assert.equal(c.progress.event,'test-bft-fragment-progress');assert.equal(c.progress.fragments,1);assert.equal(c.progress.height,1);
 assert(/^0x[0-9a-f]{64}$/.test(c.progress.blockHash));assert.equal(c.observedCoreSignal,'SIGKILL');assert.equal(c.observedRelaySignal,'SIGKILL');assert.equal(c.preCommitAudit,true);assert.equal(c.staleSessionRejected,true);assert(!r.corePids.includes(c.restartedCorePid));
 const restarted=rows.filter(x=>x.stage==='bft-download-crash-restarted'&&x.fork===r.fork);assert.equal(restarted.length,1);assert.deepEqual(restarted[0].crash,c);
 const begin=text.indexOf(`Testing: bft_fragment_download_crash_${r.fork.toLowerCase()}\n`);assert(begin>=0);const next=text.indexOf('Testing: ',begin+9);assert(text.slice(begin,next<0?text.length:next).includes('Test Passed.'));
}
console.log(JSON.stringify({verified:true,profiles:['Paris','Shanghai'],networks:2,crashedCoreProcesses:2,crashedRelayProcesses:2,preCommitAudits:2,postCommitReopenedAudits:8,scope:'Real libp2p downloads with owned fixture core processes and SIGKILL after first fragment acknowledgment. Native assertions verify all receipts, exact balances, block agreement and reopened audits. Not nano run supervisor, multi-host, physical power loss, endurance or production approval.'},null,2));
