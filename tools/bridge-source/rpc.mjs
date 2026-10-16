// Read-only, bounded transport for source observations. No retries or signing.
const sourceMethods = new Set(['eth_chainId', 'eth_getBlockByNumber', 'eth_getTransactionReceipt', 'eth_getCode']);
const launchMethods = new Set(['eth_chainId', 'eth_getBlockByNumber', 'eth_getCode', 'eth_call', 'eth_estimateGas']);
export function createSourceRpc(endpoint, options) {
  return createReadOnlyRpc(endpoint, options, sourceMethods);
}
export function createLaunchRpc(endpoint, {timeoutMs = 10000, maxResponseBytes = 1048576, onRpcError} = {}) {
  if (onRpcError !== undefined && typeof onRpcError !== 'function') throw Error('Invalid launch diagnostic callback');
  return createReadOnlyRpc(endpoint, {timeoutMs, maxResponseBytes}, launchMethods, onRpcError);
}
function createReadOnlyRpc(endpoint, {timeoutMs = 10000, maxResponseBytes = 1048576} = {}, methods, onRpcError) {
  let url;
  try { url = new URL(endpoint); } catch { throw Error('Invalid source endpoint'); }
  const local = ['localhost', '127.0.0.1', '[::1]'].includes(url.hostname);
  if (url.username || url.password || url.hash || !(url.protocol === 'https:' || (local && url.protocol === 'http:'))) throw Error('HTTPS source endpoint required (HTTP only on loopback); no URL credentials or fragment');
  if (!Number.isSafeInteger(timeoutMs) || timeoutMs < 1 || timeoutMs > 60000 || !Number.isSafeInteger(maxResponseBytes) || maxResponseBytes < 1 || maxResponseBytes > 4194304) throw Error('Invalid source transport budget');
  let sequence = 0;
  return Object.freeze({
    async request({method, params}) {
      if (!methods.has(method) || !Array.isArray(params)) throw Error('Read-only source method and parameter array required');
      const id = ++sequence;
      if (!Number.isSafeInteger(id)) throw Error('Source RPC request ID exhausted');
      const body = JSON.stringify({jsonrpc: '2.0', id, method, params});
      if (Buffer.byteLength(body) > 65536) throw Error('Source request exceeds byte budget');
      const controller = new AbortController();
      const timer = setTimeout(() => controller.abort(), timeoutMs);
      let reader;
      try {
        const response = await fetch(url, {method: 'POST', headers: {'content-type': 'application/json'}, body, signal: controller.signal, redirect: 'error'});
        if (!response.ok) throw Error('Source RPC HTTP failure');
        const length = response.headers.get('content-length');
        if (length !== null && (!/^\d+$/.test(length) || BigInt(length) > BigInt(maxResponseBytes))) throw Error('Source response exceeds byte budget');
        if (!response.body) throw Error('Source response body absent');
        reader = response.body.getReader();
        const chunks = []; let size = 0;
        while (true) {
          const {done, value} = await reader.read();
          if (done) break;
          size += value.byteLength;
          if (size > maxResponseBytes) throw Error('Source response exceeds byte budget');
          chunks.push(value);
        }
        const envelope = JSON.parse(new TextDecoder('utf-8', {fatal: true}).decode(Buffer.concat(chunks, size)));
        const own = key => Object.hasOwn(envelope, key);
        if (!envelope || Array.isArray(envelope) || envelope.jsonrpc !== '2.0' || envelope.id !== id || own('result') === own('error')) throw Error('Malformed or mismatched source RPC envelope');
        if (own('error')) {
          if (onRpcError) {
            const value = envelope.error, diagnostic = {method};
            if (Number.isSafeInteger(value?.code)) diagnostic.code = value.code;
            // Keep only bounded revert bytes, never provider messages or URLs.
            if (typeof value?.data === 'string' && value.data.length <= 8194 && /^0x(?:[0-9a-f]{2})*$/i.test(value.data)) diagnostic.data = value.data;
            onRpcError(diagnostic);
          }
          throw Error('Source RPC error');
        }
        return envelope.result;
      } catch (error) {
        // Do not expose endpoint secrets, RPC payloads or provider messages.
        if (controller.signal.aborted) throw Error('Source RPC deadline exceeded');
        if (error instanceof Error && /^(Source |Malformed or mismatched)/.test(error.message)) throw error;
        throw Error('Source RPC transport or decoding failure');
      } finally {
        clearTimeout(timer);
        controller.abort();
        if (reader) await reader.cancel().catch(() => {});
      }
    }
  });
}
