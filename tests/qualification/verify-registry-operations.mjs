import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');
assert.equal((text.match(/Test Passed\./g)||[]).length,8);
for(const name of ['consensus_phase0_registry_operations','ethereum_official_registry_block_operations','consensus_block_proposer_operations','ethereum_official_block_proposer_operations','ethereum_official_deposit_transition','ethereum_official_voluntary_exit','ethereum_official_deneb_voluntary_exit','consensus_slashing_recovery'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
function verify(scenario,expected){const found=rows.filter(r=>r.scenario===scenario);assert.equal(found.length,1);for(const [k,v] of Object.entries(expected))assert.equal(found[0][k],v);return found[0];}
const local=verify('phase0-registry-block-operations',{positive:3,rejected:10,unsupported:3,cancellations:19,configurationRejections:3,atomicOverflowRejections:1,fullStateTransition:false,activeInConsensus:false});
const official=verify('official-phase0-registry-block-operations',{valid:3,invalid:3,deferredEpochSequences:2,proposerSlashings:1,deposits:2,exits:1,balancesCompared:193,canonicalInputsUnchanged:true,fullStateTransition:false,activeInConsensus:false});
console.log(JSON.stringify({verified:true,local,official,productionQualified:false},null,2));
