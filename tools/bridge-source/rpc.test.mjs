import test from 'node:test';
import assert from 'node:assert/strict';
import {createServer} from 'node:http';
import {createSourceRpc} from './rpc.mjs';
async function fixture(t, handler) {
  const server = createServer(handler);
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  t.after(() => {server.closeAllConnections(); return new Promise(resolve => server.close(resolve));});
  return `http://127.0.0.1:${server.address().port}`;
}
const request = rpc => rpc.request({method: 'eth_chainId', params: []});
test('source transport binds distinct concurrent IDs and permits null result', async t => {
  const ids = [];
  const url = await fixture(t, async (req, res) => {
    let body = ''; for await (const chunk of req) body += chunk;
    const input = JSON.parse(body); ids.push(input.id);
    assert.equal(req.method, 'POST'); assert.equal(input.method, 'eth_chainId');
    res.end(JSON.stringify({jsonrpc: '2.0', id: input.id, result: null}));
  });
  const rpc = createSourceRpc(url);
  assert.deepEqual(await Promise.all([request(rpc), request(rpc)]), [null, null]);
  assert.equal(new Set(ids).size, 2);
});
for (const [name, envelope] of [
  ['wrong ID', {jsonrpc:'2.0', id:2, result:'0x2105'}],
  ['wrong version', {jsonrpc:'1.0', id:1, result:'0x2105'}],
  ['result and error', {jsonrpc:'2.0', id:1, result:null, error:{code:-1}}],
  ['neither result nor error', {jsonrpc:'2.0', id:1}],
  ['batch', [{jsonrpc:'2.0', id:1, result:'0x2105'}]],
  ['null envelope', null]
]) test(`source transport refuses ${name}`, async t => {
  const url = await fixture(t, (_, res) => res.end(JSON.stringify(envelope)));
  await assert.rejects(request(createSourceRpc(url)), /Malformed or mismatched/);
});
test('source transport rejects provider errors without exposing their message', async t => {
  const url = await fixture(t, (_, res) => res.end(JSON.stringify({jsonrpc:'2.0', id:1, error:{message:'private-provider-data'}})));
  await assert.rejects(request(createSourceRpc(url)), error => error.message === 'Source RPC error');
});
test('source transport bounds chunked bytes without content-length', async t => {
  const url = await fixture(t, (_, res) => {res.write('x'.repeat(64)); res.end('x'.repeat(64));});
  await assert.rejects(request(createSourceRpc(url, {maxResponseBytes:80})), /byte budget/);
});
test('source transport rejects excessive declared length', async t => {
  const url = await fixture(t, (_, res) => {res.setHeader('content-length', '5000'); res.end('x');});
  await assert.rejects(request(createSourceRpc(url, {maxResponseBytes:80})), /byte budget/);
});
test('source transport deadline also covers a stalled body', async t => {
  const url = await fixture(t, (_, res) => {res.writeHead(200); res.write('{');});
  await assert.rejects(request(createSourceRpc(url, {timeoutMs:100})), /deadline/);
});
test('source transport never follows a redirect or retries', async t => {
  let count = 0;
  const url = await fixture(t, (_, res) => {count++; res.writeHead(302, {location:'/elsewhere'}); res.end();});
  await assert.rejects(request(createSourceRpc(url)), /transport or decoding/);
  assert.equal(count, 1);
});
test('source transport refuses HTTP failures', async t => {
  const url = await fixture(t, (_, res) => {res.writeHead(503); res.end('private-message');});
  await assert.rejects(request(createSourceRpc(url)), /HTTP failure/);
});
test('source transport refuses invalid JSON and invalid UTF-8', async t => {
  for (const bytes of [Buffer.from('{'), Buffer.from([0xff])]) {
    const url = await fixture(t, (_, res) => res.end(bytes));
    await assert.rejects(request(createSourceRpc(url)), /transport or decoding/);
  }
});
test('source transport rejects unsafe endpoints, budgets, writes and oversized requests', async () => {
  for (const url of ['http://example.com', 'https://user:pass@example.com', 'https://example.com/#secret', 'ftp://localhost', 'not-a-url-secret']) assert.throws(() => createSourceRpc(url));
  for (const options of [{timeoutMs:0}, {timeoutMs:60001}, {maxResponseBytes:0}, {maxResponseBytes:4194305}]) assert.throws(() => createSourceRpc('https://example.com', options));
  const rpc = createSourceRpc('https://example.com');
  await assert.rejects(rpc.request({method:'eth_sendRawTransaction', params:[]}), /Read-only/);
  await assert.rejects(rpc.request({method:'eth_chainId', params:'bad'}), /Read-only/);
  await assert.rejects(rpc.request({method:'eth_getCode', params:['x'.repeat(65537)]}), /request exceeds/);
});
