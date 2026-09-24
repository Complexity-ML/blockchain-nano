import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const directory=process.argv[2];
const sizes=process.argv.slice(3).map(Number);
assert(sizes.length>0 && sizes.every(n=>n===100000||n===1000000));
assert.equal(new Set(sizes).size,sizes.length);
const results=[];
for(const n of sizes){
 const exit=JSON.parse(readFileSync(resolve(directory,`${n}-exit.json`),'utf8'));
 assert.equal(exit.validators,n);assert.equal(exit.exitCode,0);assert(Number.isFinite(exit.elapsedSeconds)&&exit.elapsedSeconds>0&&exit.elapsedSeconds<3660);
 const rows=readFileSync(resolve(directory,`${n}.log`),'utf8').split('\n').filter(l=>l.startsWith('{')).map(JSON.parse);
 const final=rows.filter(r=>r.scenario==='vote-collector-scaling');assert.equal(final.length,1);const r=final[0];assert.equal(r.validators,n);
 assert.equal(r.certificateVotes,n);assert.equal(r.rejected,4);assert.equal(r.conflictEvidence,1);assert.equal(r.newSetAccepted,true);assert.equal(r.participationPolls,5000);
 for(const key of ['constructMs','receiveAndParticipationMs','duplicateMs','participationPollingMs'])assert(Number.isFinite(r[key])&&r[key]>=0);
 assert(Number.isSafeInteger(r.wireBytes)&&r.wireBytes>0);
 for(const stage of ['members','signed','received','duplicates','certificate-verified','complete'])assert.equal(rows.filter(p=>p.stage===stage&&p.completed===n&&p.validators===n).length,1);
 const progress=rows.filter(p=>p.stage);assert(progress.every(p=>Number.isFinite(p.peakRssBytes)&&p.peakRssBytes>0&&p.peakRssBytes<6*1024**3));
 results.push({...r,elapsedSeconds:exit.elapsedSeconds,peakRssBytes:Math.max(...progress.map(p=>p.peakRssBytes))});
}
console.log(JSON.stringify({verified:true,results,scope:'Local individual ECDSA vote collector and independent certificate verification. No distributed node count, throughput guarantee, BLS aggregation or staking-registration capacity is established.'},null,2));
