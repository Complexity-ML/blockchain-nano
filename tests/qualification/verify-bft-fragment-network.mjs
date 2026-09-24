import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
export function verify(text) {
  const rows = text.split('\n').flatMap(line => {
    try { const row = JSON.parse(line); return row && typeof row === 'object' ? [row] : []; }
    catch { return []; }
  });
  const summaries = rows.filter(row => row.scenario === 'bft-fragment-network');
  assert.equal(summaries.length, 1, 'Expected one completed network scenario');
  const summary = summaries[0];
  assert(['Paris', 'Shanghai'].includes(summary.fork));
  assert.equal(summary.validators, 4);
  assert([1, 4].includes(summary.coreProcesses));
  if (summary.coreProcesses === 4) {
    assert.equal(summary.corePids.length, 4);
    assert(summary.corePids.every(pid => Number.isSafeInteger(pid) && pid > 0));
    assert.equal(new Set(summary.corePids).size, 4);
  }
  assert.equal(summary.relayProcesses, 4);
  assert.equal(summary.reopenedAudits, 4);
  assert(Number.isSafeInteger(summary.compactBytes) && summary.compactBytes > 16 * 1024 * 1024);
  assert(Number.isSafeInteger(summary.certificateRound) && summary.certificateRound >= 0);
  assert(/^0x[0-9a-f]{64}$/.test(summary.blockHash));
  // Empty proposals at later heights must not count as evidence of a large transfer.
  const target = rows.filter(row => row.event === 'test-bft-fragments-received' && row.blockHash === summary.blockHash);
  assert(target.length >= 2, 'Missing complete target-proposal downloads');
  for (const row of target) {
    assert.equal(row.proposal, true);
    assert.equal(row.fragments, 40);
    assert(Number.isSafeInteger(row.height) && row.height > 0);
  }
  assert.equal(new Set(target.map(row => row.height)).size, 1);
  if (summary.distinctTargetReceivers !== undefined) {
    for (const row of target) assert(Number.isSafeInteger(row.receiver) && row.receiver >= 0 && row.receiver < 4);
    const distinct = new Set(target.map(row => row.receiver)).size;
    assert(distinct >= 2, 'Target downloads must reach distinct validators');
    assert.equal(summary.distinctTargetReceivers, distinct);
    assert.equal(summary.successfulDownloads, target.length);
  }
  return {...summary, targetProposalDownloads: target.length};
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  assert(process.argv.length > 2, 'Pass full network logs');
  for (const filename of process.argv.slice(2)) console.log(JSON.stringify(verify(fs.readFileSync(filename, 'utf8'))));
}
