import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const text=readFileSync(resolve(process.argv[2],'tests-full.log'),'utf8');assert.equal((text.match(/Test Passed\./g)||[]).length,6);
for(const name of ['ethereum_official_full_slots','ethereum_official_beacon_state_root','consensus_phase0_ordered_epoch','ethereum_official_phase0_rewards','ethereum_official_phase0_ffg','ethereum_official_epoch_maintenance'])assert(text.includes(`Testing: ${name}\n`));
const rows=text.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l)).filter(r=>r.scenario==='official-phase0-full-slots');assert.equal(rows.length,1);
for(const [key,value] of Object.entries({cases:7,slots:100,serializedBytesCompared:107223,splitControls:7,replayRejections:7,cancellationPoints:7,localRejections:10,pendingCodecControls:1,historicalAccumulator:true,stateRootsVerified:true,fullBlockTransition:false,activeInConsensus:false}))assert.equal(rows[0][key],value);
console.log(JSON.stringify({verified:true,...rows[0],productionQualified:false},null,2));
