import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='bft-assembly-crash');
const expected=new Set(['Paris','Shanghai'].flatMap(f=>[1,20,40].map(n=>`${f}:${n}`)));
assert.equal(rows.length,6);
for(const row of rows){
 assert(expected.delete(`${row.fork}:${row.crashAfterFragments}`),'unknown or duplicate crash boundary');
 assert.equal(row.totalFragments,40);assert.equal(row.signal,'SIGKILL');assert.equal(row.receipts,10);assert.equal(row.rejectedStaleOrReplay,3);
 for(const field of ['preCommitAudit','noPhantomVotes','redownloadCommitted','reopenedAudit'])assert.equal(row[field],true,field);
 assert.equal(row.libp2pIntegrated,false);assert(/^0x[0-9a-f]{64}$/.test(row.blockHash));
}
assert.equal(expected.size,0);
for(const fork of ['Paris','Shanghai'])assert.equal(new Set(rows.filter(r=>r.fork===fork).map(r=>r.blockHash)).size,1,'same signed proposal across crash boundaries');
for(const fork of ['paris','shanghai'])for(const scenario of ['assembly_crash','proposal_ingress']){
 const name=`bft_${scenario}_${fork}`;
 const begin=text.indexOf(`Testing: ${name}\n`);assert(begin>=0,`missing ${name}`);
 const end=text.indexOf('End testing:',begin);const next=text.indexOf('Testing: ',begin+9);
 const section=text.slice(begin,next<0?(end<0?text.length:end):next);assert(section.includes('Test Passed.'),`failed ${name}`);
}
console.log(JSON.stringify({verified:true,cases:rows.length,acknowledgedFragments:[1,20,40],forks:['Paris','Shanghai'],successfulRecoveredReceipts:60,regressionSuites:2,scope:'SIGKILL of owned fixture processes using core RPC dispatch, before finish/validation. Native assertions verify state, votes, signed quorums, receipts and reopened audits. Not libp2p, nano run supervisor, physical power loss or multi-host qualification.'},null,2));
