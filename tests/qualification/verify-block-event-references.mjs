import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

export function verify(text) {
  const summaries = text.split('\n').flatMap(line => {
    try { const row = JSON.parse(line); return row?.scenario === 'block-event-references' ? [row] : []; }
    catch { return []; }
  });
  assert.equal(summaries.length, 2, 'Expected both completed reference scenarios');
  assert.deepEqual(summaries.map(row => row.fork).sort(), ['Paris', 'Shanghai']);
  for (const row of summaries) {
    assert(Number.isSafeInteger(row.compactBlockBytes) && row.compactBlockBytes > 16 * 1024 * 1024);
    assert(Number.isSafeInteger(row.maximumPageBytes) && row.maximumPageBytes > 0 && row.maximumPageBytes < 16384);
    for (const [key, value] of Object.entries({fragments:40, receiptCount:10, paginatedBlocks:52, expiredHistoryEvents:10000, invalidReferencesRejected:7, reopenedAudits:2, coreProcesses:1, relayProcesses:2, legacyOversizedEventRefused:true})) assert.equal(row[key], value, key);
    assert.equal(row.referenceSent.event, 'test-block-reference-sent');
    assert.equal(row.referenceReceived.event, 'test-block-reference-received');
    assert.deepEqual(row.referenceSent.reference, row.referenceReceived.reference);
    const reference = row.referenceSent.reference;
    assert.deepEqual(Object.keys(reference).sort(), ['hash', 'number', 'version']);
    assert.equal(reference.version, 1);
    assert.equal(reference.number, '0x1');
    assert(/^0x[0-9a-f]{64}$/.test(reference.hash));
  }
  return summaries;
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  assert.equal(process.argv.length, 3, 'Pass the full combined CTest log');
  console.log(JSON.stringify({verified:true, scenarios:verify(fs.readFileSync(process.argv[2], 'utf8'))}, null, 2));
}
