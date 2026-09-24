import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

export function verify(text) {
  const summaries = text.split('\n').flatMap(line => {
    try { const row = JSON.parse(line); return row?.scenario === 'bft-proposer-cycle' ? [row] : []; }
    catch { return []; }
  });
  assert.equal(summaries.length, 2, 'Both completed proposer-cycle profiles are required');
  assert.deepEqual(summaries.map(row => row.fork).sort(), ['Paris', 'Shanghai']);
  for (const row of summaries) {
    assert.equal(row.returnedRound, 4);
    assert.equal(row.canonicalHeight, '0x1');
    assert.equal(row.signatureRefusals, 17);
    assert.equal(row.corruptedSlotsRejected, 5);
    for (const key of ['reactorReopened', 'historyReopened', 'legacyMigrationVerified']) assert.equal(row[key], true, key);
  }
  return summaries;
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  assert.equal(process.argv.length, 3, 'Pass the full combined CTest log');
  console.log(JSON.stringify({verified:true, scenarios:verify(fs.readFileSync(process.argv[2], 'utf8'))}, null, 2));
}
