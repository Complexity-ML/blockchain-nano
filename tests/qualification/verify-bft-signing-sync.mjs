import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
export function verify(text) {
  const rows = text.split('\n').flatMap(line => {
    try { const row = JSON.parse(line); return row?.scenario === 'bft-signing-sync-failure' ? [row] : []; }
    catch { return []; }
  });
  assert.equal(rows.length, 2);
  assert.deepEqual(rows.map(row => row.fork).sort(), ['Paris', 'Shanghai']);
  for (const row of rows) {
    assert.equal(row.records.length, 4);
    assert.deepEqual(row.records.map(r => `${r.layout}/${r.disposition}`).sort(), ['fresh/lost', 'fresh/retained', 'legacy/lost', 'legacy/retained']);
    for (const {failed, recovered, disposition} of row.records) {
      assert(Number.isSafeInteger(failed.syncFailures) && failed.syncFailures > 0);
      assert(Number.isSafeInteger(failed.walAppendedBytes) && failed.walAppendedBytes > 0);
      assert(failed.error.includes('RocksDB:'));
      for (const key of ['signatureEmpty', 'retryRejected', 'conflictRejected']) assert.equal(failed[key], true, key);
      assert.equal(recovered.reservationRetained, disposition === 'retained');
      assert.equal(recovered.blockHash, failed.blockHash);
      assert(/^0x[0-9a-f]{64}$/.test(recovered.blockHash));
      assert.equal(recovered.canonicalHeight, 0);
      for (const key of ['signatureVerified', 'batchAtomic', 'secondReopenVerified', 'downgradeRefused']) assert.equal(recovered[key], true, key);
    }
  }
  return rows;
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  assert.equal(process.argv.length, 3);
  console.log(JSON.stringify({verified:true, scenarios:verify(fs.readFileSync(process.argv[2], 'utf8'))}, null, 2));
}
