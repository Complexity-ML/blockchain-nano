import {createHash} from 'node:crypto';
import {open, readFile, mkdir} from 'node:fs/promises';
import {performance} from 'node:perf_hooks';
import {createLaunchRpc} from '/Users/boris/Dev/blockchain-nano/tools/bridge-source/rpc.mjs';

// Fixed public reads only: no launch configuration, wallet or transaction method.
const endpoint = 'https://mainnet.base.org';
const usdc = '0x833589fcd6edb6e08f4c7c32d4f71b54bda02913';
const unknownHash = `0x${'f'.repeat(64)}`;
const output = process.argv[2];
if (process.argv.length !== 3 || !output) throw Error('one new output directory required');
await mkdir(output, {mode: 0o700});
const transportPath = '/Users/boris/Dev/blockchain-nano/tools/bridge-source/rpc.mjs';
const transport = await readFile(transportPath);
const sha = bytes => createHash('sha256').update(bytes).digest('hex');
const rows = [];
let diagnostic;
const rpc = createLaunchRpc(endpoint, {timeoutMs: 10000, maxResponseBytes: 1048576,
  onRpcError: value => {diagnostic = structuredClone(value);}});
const started = performance.now();
const report = {
  format: 'nano-base-public-hash-selector-observation-v1',
  startedAt: new Date().toISOString(), endpoint, sourceRevision: '92b5edf385682b51517ea3a6e008a99831e7016c',
  transport: {sha256: sha(transport), bytes: transport.length},
  budgets: {maximumRequests: 10, perRequestTimeoutMs: 10000, perResponseBytes: 1048576, retries: 0},
  address: usdc, requestedReferenceTag: 'finalized', rows,
  walletUsed: false, launchTermsRead: false, transactionSent: false,
  historicalExecutionProven: false, noncanonicalHashTested: false,
  cryptographicFinalityProven: false, dopplerCreateSimulated: false, productionQualified: false
};
async function request(label, method, params, compact = value => value) {
  if (rows.length >= 10 || performance.now() - started > 110000) throw Error('total probe budget exceeded');
  diagnostic = undefined;
  const row = {index: rows.length + 1, label, method, selector: params.at(-1), startedAt: new Date().toISOString()};
  rows.push(row);
  const start = performance.now();
  try {
    const result = await rpc.request({method, params});
    row.outcome = 'result';
    row.result = compact(result);
    return {ok: true, value: result};
  } catch (error) {
    row.outcome = diagnostic ? 'rpc-error' : 'transport-or-validation-error';
    row.error = error instanceof Error ? error.message : 'unexpected error';
    if (diagnostic) row.diagnostic = diagnostic;
    return {ok: false};
  } finally {row.durationMs = Math.round(performance.now() - start);}
}
const isQuantity = value => typeof value === 'string' && /^0x(?:0|[1-9a-f][0-9a-f]*)$/i.test(value);
const isHash = value => typeof value === 'string' && /^0x[0-9a-f]{64}$/i.test(value);
const headerSummary = value => value && ({number: value.number, hash: value.hash, timestamp: value.timestamp});
try {
  const chain = await request('identity', 'eth_chainId', []);
  if (!chain.ok || chain.value !== '0x2105') throw Error('Base mainnet identity not verified');
  const header = await request('reference', 'eth_getBlockByNumber', ['finalized', false], headerSummary);
  if (!header.ok || !isQuantity(header.value?.number) || !isHash(header.value?.hash) || !isQuantity(header.value?.timestamp)) throw Error('reference header not verified');
  const {number, hash, timestamp} = header.value;
  report.reference = {number, hash, timestamp};
  if (hash.toLowerCase() === unknownHash) throw Error('unknown hash equals reference');
  const selected = {blockHash: hash, requireCanonical: true};
  const absent = {blockHash: unknownHash, requireCanonical: true};
  const call = {to: usdc, data: '0x313ce567', value: '0x0', gas: '0x186a0'};
  const codeSummary = value => {
    if (typeof value !== 'string' || !/^0x(?:[0-9a-f]{2})+$/i.test(value)) throw Error('nonempty bytecode expected');
    const bytes = Buffer.from(value.slice(2), 'hex');
    return {bytes: bytes.length, sha256: sha(bytes)};
  };
  const code = await request('known-hash-code', 'eth_getCode', [usdc, selected], codeSummary);
  const decimals = await request('known-hash-call', 'eth_call', [call, selected]);
  const gas = await request('known-hash-estimate', 'eth_estimateGas', [call, selected]);
  await request('unknown-hash-code', 'eth_getCode', [usdc, absent], codeSummary);
  await request('unknown-hash-call', 'eth_call', [call, absent]);
  await request('unknown-hash-estimate', 'eth_estimateGas', [call, absent]);
  const future = `0x${(BigInt(number) + 0x100000000n).toString(16)}`;
  await request('future-height-call', 'eth_call', [call, {blockNumber: future}]);
  const fenced = await request('reference-fence', 'eth_getBlockByNumber', [number, false], headerSummary);
  report.observed = {
    chainIdentity: true,
    nonemptyCodeAtKnownHash: code.ok,
    usdcDecimalsAtKnownHash: decimals.ok && decimals.value === `0x${'0'.repeat(63)}6`,
    gasQuantityAtKnownHash: gas.ok && isQuantity(gas.value) && BigInt(gas.value) > 0n,
    unknownHashesRejected: rows.slice(5, 8).every(row => row.outcome === 'rpc-error' && Number.isSafeInteger(row.diagnostic?.code) && row.diagnostic.code < 0),
    futureHeightRejected: rows[8].outcome === 'rpc-error' && Number.isSafeInteger(rows[8].diagnostic?.code) && rows[8].diagnostic.code < 0,
    referenceFence: fenced.ok && fenced.value?.number === number && fenced.value?.hash === hash
  };
  report.observed.requiredSelectorSurfaceAccepted = Object.values(report.observed).every(Boolean);
} catch (error) {report.error = error instanceof Error ? error.message : 'unexpected error';}
report.completedAt = new Date().toISOString();
report.durationMs = Math.round(performance.now() - started);
report.requestCount = rows.length;
report.transportUnchanged = sha(await readFile(transportPath)) === report.transport.sha256;
const bytes = Buffer.from(`${JSON.stringify(report, null, 2)}\n`);
const file = await open(`${output}/result-public.json`, 'wx', 0o600);
try {await file.writeFile(bytes);await file.sync();} finally {await file.close();}
console.log(JSON.stringify({result: `${output}/result-public.json`, sha256: sha(bytes), bytes: bytes.length, requestCount: rows.length, observed: report.observed, error: report.error}));
if (!report.observed?.requiredSelectorSurfaceAccepted || !report.transportUnchanged) process.exitCode = 1;
