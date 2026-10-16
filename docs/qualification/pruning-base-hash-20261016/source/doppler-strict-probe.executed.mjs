import {readFile, mkdir, writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {performance} from 'node:perf_hooks';

const root = '/Users/boris/Dev/blockchain-nano';
const dir = process.argv[2];
if (process.argv.length !== 3 || !dir) throw Error('new output directory required');
await mkdir(dir, {mode: 0o700});
const sha = value => createHash('sha256').update(value).digest('hex');
const save = (leaf, value) => writeFile(`${dir}/${leaf}`, `${JSON.stringify(value, (_, v) => typeof v === 'bigint' ? v.toString() : v, 2)}\n`, {flag: 'wx', mode: 0o600});
const sources = ['tools/doppler-launch/preview.mjs', 'tools/doppler-launch/example.synthetic.json', 'tools/doppler-launch/package-lock.json', 'tools/bridge-source/rpc.mjs'];
const pins = {};
for (const file of sources) {const b = await readFile(`${root}/${file}`); pins[file] = {sha256: sha(b), bytes: b.length};}
const fixture = JSON.parse(await readFile(`${root}/tools/doppler-launch/example.synthetic.json`, 'utf8'));
const priorReferenceTimestamp = fixture.referenceTimestamp;
fixture.referenceTimestamp = Math.floor(Date.now() / 1000);
await save('config.synthetic.json', fixture);
const calls = [], started = performance.now(), controller = new AbortController();
const timer = setTimeout(() => controller.abort(), 120000);
const original = globalThis.fetch;
let selectedHash;
globalThis.fetch = async (url, options) => {
  if (String(url) !== 'https://mainnet.base.org/' || options?.method !== 'POST' || calls.length >= 12 || controller.signal.aborted) throw Error('fixed public simulation budget or endpoint refused');
  const envelope = JSON.parse(options.body);
  if (!['eth_chainId', 'eth_getBlockByNumber', 'eth_getCode', 'eth_call', 'eth_estimateGas'].includes(envelope.method) || !Array.isArray(envelope.params)) throw Error('non-read-only method refused');
  const row = {index: calls.length + 1, method: envelope.method};
  if (['eth_getCode', 'eth_call', 'eth_estimateGas'].includes(envelope.method)) {
    const selector = envelope.params[1];
    if (!/^0x[0-9a-f]{64}$/i.test(selector?.blockHash ?? '') || selector.requireCanonical !== true) throw Error('canonical hash selector required');
    if (selectedHash && selectedHash !== selector.blockHash) throw Error('one reference hash required');
    selectedHash = selector.blockHash;row.selector = selector;
  } else row.params = envelope.params;
  if (envelope.method === 'eth_call' || envelope.method === 'eth_estimateGas') {
    const tx = envelope.params[0];row.call = {from: tx.from, to: tx.to, value: tx.value, dataBytes: (tx.data.length - 2) / 2, dataSha256: sha(Buffer.from(tx.data.slice(2), 'hex'))};
    if (tx.value && tx.value !== '0x0') throw Error('nonzero simulation value refused');
  }
  calls.push(row);
  return original(url, {...options, signal: AbortSignal.any([options.signal, controller.signal])});
};
const report = {format: 'nano-doppler-public-strict-hash-simulation-v1', startedAt: new Date().toISOString(), sourceRevision: '583c58952efde5ccbc033750a06d5a6f7ae056bc', sourcePins: pins, calls, maximumRequests: 12, deadlineMs: 120000, syntheticTermsOnly: true, fixtureChanges: {referenceTimestamp: {before: priorReferenceTimestamp, after: fixture.referenceTimestamp}}, originalFilesAndHistoricalArtifactsModified: false, walletUsed: false, signatureRequested: false, transactionSent: false, realLaunchTermsQualified: false, bridgeQualified: false, productionQualified: false};
try {
  const {preview} = await import(`${root}/tools/doppler-launch/preview.mjs`);
  const review = await preview(fixture, 'https://mainnet.base.org');
  await save('unsigned-review.synthetic.json', review);
  report.status = review.status;report.simulation = review.simulation;
  report.calldataHash = review.calldataHash;report.predicted = review.predicted;
  report.passed = review.status === 'unsigned-preview' && review.simulation.success === true && review.simulation.referenceHashPinned === true && review.simulation.gasEstimateHashExtensionAccepted === true && calls.length === 12 && selectedHash === review.simulation.blockHash;
} catch (error) {report.passed = false;report.error = error instanceof Error ? error.message : 'unexpected failure';}
finally {clearTimeout(timer);globalThis.fetch = original;controller.abort();}
report.durationMs = Math.round(performance.now() - started);report.completedAt = new Date().toISOString();report.requestCount = calls.length;
report.sourceBytesUnchanged = true;
for (const file of sources) {const b = await readFile(`${root}/${file}`);if (sha(b) !== pins[file].sha256 || b.length !== pins[file].bytes) report.sourceBytesUnchanged = false;}
await save('result-public.json', report);
const b = await readFile(`${dir}/result-public.json`);
console.log(JSON.stringify({result: `${dir}/result-public.json`, sha256: sha(b), bytes: b.length, passed: report.passed, requestCount: calls.length, status: report.status, simulation: report.simulation, error: report.error}));
if (!report.passed || !report.sourceBytesUnchanged) process.exitCode = 1;
