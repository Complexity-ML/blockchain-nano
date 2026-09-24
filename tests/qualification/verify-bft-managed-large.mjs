import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
const finals=rows.filter(r=>r.scenario==='bft-managed-large-crash');assert.equal(finals.length,2);assert.deepEqual(finals.map(r=>r.fork).sort(),['Paris','Shanghai']);
for(const r of finals){
 assert.equal(r.managedNodes,4);assert.equal(r.rpcSubmissions,40);assert.equal(r.newAdmissions+r.knownDuplicates,40);assert(r.newAdmissions>=10);assert.equal(r.crashAfterFragment,1);assert.equal(r.mempoolReinjected,false);
 for(const f of ['orphanExited','identityPreserved','preCommitAudit'])assert.equal(r[f],true);
 assert.equal(r.reopenedAudits,4);assert.equal(r.receiptCount,10);assert(r.distinctTargetReceivers>=2);assert(r.successfulDownloads>=2);assert(r.compactBytes>16*1024*1024);assert(/^0x[0-9a-f]{64}$/.test(r.blockHash));assert(BigInt(r.blockNumber)>0n);
 const crash=rows.filter(x=>x.stage==='managed-large-crash'&&x.fork===r.fork);assert.equal(crash.length,1);assert.equal(crash[0].parentSignal,'SIGKILL');assert.equal(crash[0].orphanExited,true);assert.equal(crash[0].progress.fragments,1);assert.equal(crash[0].progress.event,'test-bft-fragment-progress');assert(/^0x[0-9a-f]{64}$/.test(crash[0].priorBlockHash));
 const begin=text.indexOf(`Testing: bft_managed_large_crash_${r.fork.toLowerCase()}\n`);assert(begin>=0);const next=text.indexOf('Testing: ',begin+9);assert(text.slice(begin,next<0?text.length:next).includes('Test Passed.'));
}
console.log(JSON.stringify({verified:true,networks:2,profiles:['Paris','Shanghai'],managedNodesPerNetwork:4,preCommitAudits:2,reopenedAudits:8,scope:'Four production nano run processes and real cpp-libp2p per profile; signed transactions admitted by public RPC; one core SIGKILL after first fragment, orphan relay exits, same database/journal/identity restart without manual mempool reinjection. Local scope only; not multi-host endurance, all interruption points, physical power loss or production approval.'},null,2));
