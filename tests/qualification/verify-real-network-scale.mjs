import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const directory=process.argv[2],results=[];
for(const n of [4,16]){
 assert.equal(readFileSync(resolve(directory,`${n}.exit`),'utf8').trim(),'0');
 const rows=readFileSync(resolve(directory,`${n}.log`),'utf8').split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
 assert.equal(rows.length,1);const r=rows[0];assert.equal(r.scenario,'real-network-scale');
 for(const key of ['nodes','coreProcesses','relayProcesses','matchingCommitments','matchingReceipts'])assert.equal(r[key],n);
 assert.equal(r.validatorCount,1);assert.equal(r.blocks,4);assert.equal(r.transactions,1);assert.equal(r.fork,'Shanghai');assert.equal(r.multiHost,false);assert(r.elapsedSeconds>0);results.push(r);
}
const bft=readFileSync(resolve(directory,'bft-4-full.log'),'utf8');assert(bft.includes('Testing: bft_validators_shanghai_network'));assert(bft.includes('Test Passed.'));
console.log(JSON.stringify({verified:true,results,activeValidatorNetwork:4,scope:'Local 4/16 full nodes with one fixture signer: history, transaction and certified block propagation, exact state and receipt agreement. Separate existing four-active-validator Shanghai BFT test. No 10000-node, multi-host, endurance or production qualification.'},null,2));
