// Receipt qualification for an isolated finite workload, not a throughput benchmark.
import assert from 'node:assert/strict';

const quantity = (value, label) => {
  assert.match(value ?? '', /^0x(?:0|[1-9a-f][0-9a-f]*)$/, label);
  return BigInt(value);
};
const hash = (value, label) => {
  assert.match(value ?? '', /^0x[0-9a-f]{64}$/, label);
  return value;
};
export const loadNodes = ['native', 'local', 'remote'];

// Missing/pending receipts are incomplete, never successful. Malformed, failed,
// divergent, or noncanonical receipts are evidence of failure, not retry signals.
export function verifyLoadReceipts(expected, observations) {
  assert.ok(expected.length > 0, 'Empty workload');
  expected.forEach(h => hash(h, 'Expected transaction hash'));
  assert.equal(new Set(expected).size, expected.length, 'Duplicate workload hash');
  assert.deepEqual(Object.keys(observations).sort(), [...loadNodes].sort(), 'Three nodes required');
  const pending = [];
  let referenceAnchor;
  for (const node of loadNodes) {
    const observation = observations[node];
    const height = quantity(observation.anchor.number, 'Anchor height');
    hash(observation.anchor.hash, 'Anchor hash');
    if (referenceAnchor) assert.deepEqual(observation.anchor, referenceAnchor, 'Conflicting finalized anchors');
    referenceAnchor = observation.anchor;
    assert.ok(quantity(observation.finalized.number, 'Finalized height') >= height, 'Anchor not finalized');
    hash(observation.finalized.hash, 'Finalized hash');
    assert.deepEqual(Object.keys(observation.receipts).sort(), [...expected].sort(), 'Receipt set mismatch');
    for (const tx of expected) {
      const receipt = observation.receipts[tx];
      if (receipt === null) { pending.push({node, tx, reason: 'missing'}); continue; }
      assert.equal(receipt.transactionHash, tx, 'Wrong transaction receipt');
      assert.equal(receipt.status, '0x1', 'Transaction failed');
      const block = quantity(receipt.blockNumber, 'Receipt block');
      hash(receipt.blockHash, 'Receipt block hash');
      quantity(receipt.transactionIndex, 'Transaction index');
      quantity(receipt.gasUsed, 'Gas used');
      quantity(receipt.cumulativeGasUsed, 'Cumulative gas used');
      assert.ok(Array.isArray(receipt.logs), 'Receipt logs required');
      assert.ok(Object.hasOwn(receipt, 'contractAddress'), 'Contract address required');
      if (block > height) { pending.push({node, tx, reason: 'not-finalized'}); continue; }
      assert.equal(observation.blocks[receipt.blockNumber], receipt.blockHash, 'Noncanonical receipt');
    }
  }
  if (pending.length) return {complete: false, pending};
  // Compare the complete RPC receipt, including logs, bloom, addresses and fees.
  for (const tx of expected) for (const node of loadNodes.slice(1))
    assert.deepEqual(observations[node].receipts[tx], observations.native.receipts[tx], 'Divergent finalized receipts');
  return {complete: true, transactions: expected.length, anchor: referenceAnchor};
}

export async function collectLoadReceipts(expected, rpc) {
  const observations = {};
  for (const node of loadNodes) {
    const finalized = await rpc(node, 'eth_getBlockByNumber', ['finalized', false]);
    quantity(finalized?.number, 'Finalized height');
    hash(finalized?.hash, 'Finalized hash');
    observations[node] = {finalized, receipts: {}, blocks: {}};
  }
  const height = loadNodes.map(n => BigInt(observations[n].finalized.number))
    .reduce((a, b) => a < b ? a : b);
  const tag = '0x' + height.toString(16);
  for (const node of loadNodes) {
    const observation = observations[node];
    const anchor = await rpc(node, 'eth_getBlockByNumber', [tag, false]);
    assert.equal(anchor?.number, tag, 'Wrong anchor height');
    observation.anchor = {number: anchor.number, hash: anchor.hash};
    for (const tx of expected) {
      const receipt = await rpc(node, 'eth_getTransactionReceipt', [tx]);
      observation.receipts[tx] = receipt;
      if (receipt && quantity(receipt.blockNumber, 'Receipt block') <= height &&
          !Object.hasOwn(observation.blocks, receipt.blockNumber)) {
        const block = await rpc(node, 'eth_getBlockByNumber', [receipt.blockNumber, false]);
        assert.equal(block?.number, receipt.blockNumber, 'Wrong canonical block height');
        observation.blocks[receipt.blockNumber] = hash(block.hash, 'Canonical block hash');
      }
    }
    // Finalized anchors cannot legitimately reorganize during a collection pass.
    const after = await rpc(node, 'eth_getBlockByNumber', [tag, false]);
    assert.equal(after?.hash, observation.anchor.hash, 'Finalized anchor changed during read');
  }
  return {observations, verdict: verifyLoadReceipts(expected, observations)};
}
