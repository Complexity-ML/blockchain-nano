import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');
assert.equal((text.match(/Test Passed\./g)||[]).length,9);
for(const name of ['ethereum_official_registry_epoch_updates','consensus_registry_epoch_scale','consensus_phase0_registry_operations','ethereum_official_registry_block_operations','ethereum_official_deposit_transition','ethereum_official_voluntary_exit','ethereum_official_deneb_voluntary_exit','consensus_slashing_recovery','ethereum_official_proposer_slashing'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));const official=rows.filter(r=>r.scenario==='official-phase0-registry-updates');assert.equal(official.length,1);
for(const [k,v] of Object.entries({valid:14,invalid:1,validatorsCompared:1152,eligible:19,activated:19,ejected:37,snapshotRoundtrips:14,cancellations:68,localRejections:10,fullEpochTransition:false,activeInConsensus:false}))assert.equal(official[0][k],v);
const scale=rows.filter(r=>r.scenario==='phase0-registry-epoch-scale');assert.equal(scale.length,2);
for(const [i,n] of [100000,1000000].entries()){const r=scale[i];assert.equal(r.validators,n);assert.equal(r.activated,Math.floor((n/2)/32));assert.equal(r.ejected,Math.ceil((n/2)/16));assert.equal(r.syntheticRegistry,true);assert.equal(r.distributedNodes,0);assert.equal(r.activeInConsensus,false);assert(r.transitionMicroseconds>0&&r.processPeakRssBytes>0);}
console.log(JSON.stringify({verified:true,official:official[0],scale,productionQualified:false},null,2));
