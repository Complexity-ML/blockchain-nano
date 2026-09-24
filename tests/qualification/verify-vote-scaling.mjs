import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const root=process.argv[2];const read=p=>readFileSync(resolve(root,p),'utf8');
const rows=t=>t.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
function passed(text,name){const begin=text.indexOf(`Testing: ${name}\n`);assert(begin>=0,`missing ${name}`);const next=text.indexOf('Testing: ',begin+9);assert(text.slice(begin,next<0?text.length:next).includes('Test Passed.'),`failed ${name}`);}
const baseline=read('baseline-full.log'),native=read('native-full.log'),network=read('network-full.log');
passed(baseline,'vote_collector_scaling');for(const name of ['vote_collector_scaling','finality','consensus_round'])passed(native,name);
const pick=t=>rows(t).filter(r=>r.scenario==='vote-collector-scaling');const before=pick(baseline),after=pick(native);
for(const table of [before,after]){assert.deepEqual(table.map(r=>r.validators),[4,16,64,256,1000]);for(const r of table){assert.equal(r.certificateVotes,r.validators);assert.equal(r.rejected,4);assert.equal(r.conflictEvidence,1);assert.equal(r.newSetAccepted,true);assert(r.wireBytes>0);for(const name of ['constructMs','receiveAndParticipationMs','duplicateMs'])assert(Number.isFinite(r[name])&&r[name]>=0);}}
const comparison=before.map((b,i)=>{const a=after[i];assert.equal(a.wireBytes,b.wireBytes);return {validators:b.validators,wireBytes:b.wireBytes,beforeReceiveMs:b.receiveAndParticipationMs,afterReceiveMs:a.receiveAndParticipationMs,receiveRatio:b.receiveAndParticipationMs/a.receiveAndParticipationMs,beforeDuplicateMs:b.duplicateMs,afterDuplicateMs:a.duplicateMs};});
const stake=rows(network).filter(r=>r.scenario==='stake-change');assert.equal(stake.length,2);assert.deepEqual(stake.map(r=>r.fork).sort(),['Paris','Shanghai']);
for(const r of stake){passed(network,`bft_stake_change_crash_${r.fork.toLowerCase()}_network`);assert.deepEqual(r.oldWeights,[1,1,1,1]);assert.deepEqual(r.newWeights,[4,1,1,1]);assert.equal(r.restartMode,'SIGKILL');assert.equal(r.confirmedSigkillExits,4);assert.equal(r.certificateSigners,2);assert.equal(r.reopenedPeers,4);assert.equal(r.verifiedJournals,4);assert.equal(r.restartRecipientBalance,'0x3');}
console.log(JSON.stringify({verified:true,comparison,finalSuites:5,limitChanged:false,scope:'One before/after local collector measurement per size, not statistical throughput or network scaling. Every vote still verifies its signature; malformed, foreign-set and outsider messages, conflicts, quorum prefixes and fresh membership checked. Two local four-validator networks verify stake change and crash recovery. Gossip and other decoding paths remain outside this optimization.'},null,2));
