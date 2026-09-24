import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,5);
for(const name of ['consensus_phase0_snapshot_sync_failure','consensus_phase0_snapshot_recovery','ethereum_official_full_blocks','storage_sync_failure_shanghai','storage_sync_retry_shanghai'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='phase0-snapshot-sync-failure');assert.equal(rows.length,1);const r=rows[0];
assert.deepEqual(r.outcomes.map(o=>o.mode),['retained','lost','torn']);
for(const o of r.outcomes){assert(o.syncFailures>0);assert.equal(o.consistent,true);assert.equal(o.resumedOfficialState,true);assert(o.walAppendedBytes>0);assert.equal(o.reopenedHeight,o.mode==='retained'?1:0);if(o.mode==='retained')assert.equal(o.walSuffixBytesRetained,o.walAppendedBytes);else if(o.mode==='lost')assert.equal(o.walSuffixBytesRetained,0);else assert(o.walSuffixBytesRetained>0&&o.walSuffixBytesRetained<o.walAppendedBytes);}
for(const key of ['physicalPowerLossQualified','executionConsensusBindingQualified','activeInConsensus'])assert.equal(r[key],false);
console.log(JSON.stringify({verified:true,...r,productionQualified:false},null,2));
