#!/usr/bin/env node
// Executes the unmodified Assertoor binary against two synthetic local Nano nodes.
import {observeStableFinality} from './stability.mjs';
import { spawn } from 'node:child_process';
import { mkdtemp, writeFile, readFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createServer } from 'node:net';
import { setTimeout as delay } from 'node:timers/promises';
const endpoints = process.argv.slice(2);
if (endpoints.length !== 2 || endpoints.some(x => new URL(x).hostname !== '127.0.0.1')) throw Error('Two loopback Nano endpoints required');
const binary = resolve(process.env.NANO_TEST_ASSERTOOR_BINARY || fileURLToPath(new URL('../../.deps/assertoor-bin', import.meta.url)));
const directory = await mkdtemp(join(tmpdir(), 'nano-assertoor-'));
const listener = createServer();
await new Promise((ok, fail) => { listener.once('error', fail); listener.listen(0, '127.0.0.1', ok); });
const port = listener.address().port;
await new Promise(ok => listener.close(ok));
const finality = process.env.NANO_TEST_ASSERTOOR_FINALITY === '1';
const stabilityEpochs = Number(process.env.NANO_TEST_ASSERTOOR_STABILITY_EPOCHS || 0);
if (stabilityEpochs && (!finality || !Number.isInteger(stabilityEpochs) || stabilityEpochs < 2 || stabilityEpochs > 16)) throw Error('Invalid stability window');
const tests = JSON.parse(await readFile(new URL(finality ? './finality.json' : './smoke.json', import.meta.url), 'utf8'));
const config = {
  coordinator: {maxConcurrentTests: 1},
  web: {server: {host: '127.0.0.1', port: String(port)}, api: {enabled: true}, frontend: {enabled: false}},
  ai: {enabled: false}, playbookLibrary: {enabled: false},
  endpoints: endpoints.map((url, i) => ({name: `nano-${i}`, executionUrl: url, consensusUrl: url})), tests,
};
await writeFile(join(directory, 'config.json'), JSON.stringify(config, null, 2));
const child = spawn(binary, ['--config', join(directory, 'config.json'), '--log-format', 'json', '--metrics-port', '0'], {cwd: directory, stdio: ['ignore', 'pipe', 'pipe']});
let logs = '', terminated = false, spawnError;
const closed = new Promise(ok => {child.once('error', error => {spawnError = error; terminated = true; ok();}); child.once('close', () => {terminated = true; ok();});});
for (const stream of [child.stdout, child.stderr]) stream.on('data', data => { logs += data; if (logs.length > 8 * 1024 * 1024) logs = logs.slice(-8 * 1024 * 1024); });
const stop = () => {if (!terminated) child.kill('SIGTERM');};
process.on('SIGTERM', stop); process.on('SIGINT', stop);
const api = async path => {
  const response = await fetch(`http://127.0.0.1:${port}/api/v1/${path}`, {signal: AbortSignal.timeout(3000)});
  if (!response.ok) throw Error(`Assertoor API ${response.status}`);
  return response.json();
};
let failed;
try {
  const deadline = Date.now() + (finality ? 280000 : 100000);
  let completed;
  while (Date.now() < deadline) {
    if (terminated) throw spawnError || Error('Assertoor exited before reporting results');
    let runs;
    try {runs = (await api('test_runs')).data;} catch {await delay(500); continue;}
    if (runs?.length === tests.length && runs.every(run => !['pending','running'].includes(run.status))) {completed = runs; break;}
    await delay(500);
  }
  if (!completed) throw Error('Assertoor did not complete within bounded deadline');
  await writeFile(join(directory, 'runs.json'), JSON.stringify(completed, null, 2));
  for (const run of completed) {
    const detail = await api(`test_run/${run.run_id}/details`);
    await writeFile(join(directory, `run-${run.run_id}.json`), JSON.stringify(detail, null, 2));
    const expected = tests.find(test => test.id === run.test_id);
    if (!expected || detail.data.tasks.length !== expected.tasks.length || detail.data.tasks.some(task => !task.started || !task.completed || task.result !== 'success')) throw Error('Missing, skipped or failed Assertoor task');
  }
  await writeFile(join(directory, 'clients.json'), JSON.stringify(await api('clients'), null, 2));
  if (completed.some(run => run.status !== 'success')) throw Error(`Assertoor test failure: ${JSON.stringify(completed)}`);
  if (finality) {
    // Assertoor's cache is shared: independently require both native nodes to
    // expose the same nonzero Beacon checkpoint and matching execution block.
    let observations;
    const convergenceDeadline = Date.now() + 30000;
    const attempts = [];
    while (Date.now() < convergenceDeadline) {
      const sample = [];
      for (const endpoint of endpoints) {
        try {
          const read = async path => {
            const r = await fetch(endpoint + path, {headers: {Accept:'application/json'}, signal: AbortSignal.timeout(5000)});
            if (!r.ok) throw Error(`Nano REST ${r.status}: ${path}`);
            return r.json();
          };
          const checkpoint = (await read('/eth/v1/beacon/states/head/finality_checkpoints')).data.finalized;
          if (BigInt(checkpoint.epoch) < 2n) {sample.push({endpoint, checkpoint, pending:true}); continue;}
          const block = (await read('/eth/v2/beacon/blocks/' + checkpoint.root)).data.message;
          const response = await fetch(endpoint, {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({jsonrpc:'2.0', id:1, method:'eth_getBlockByNumber', params:['finalized',false]}), signal:AbortSignal.timeout(5000)});
          if (!response.ok) throw Error(`Nano RPC ${response.status}`);
          const result = await response.json();
          if (result.error) throw Error(`Nano RPC: ${JSON.stringify(result.error)}`);
          const execution = result.result;
          if (!execution || execution.hash !== block.body.execution_payload.block_hash) {sample.push({endpoint, checkpoint, pending:true, reason:'Beacon/execution observation differs'}); continue;}
          sample.push({checkpoint, beaconSlot:block.slot, executionNumber:execution.number, executionHash:execution.hash});
        } catch (error) {sample.push({endpoint, error:String(error)});}
      }
      attempts.push({time:Date.now(), sample});
      if (sample.every(x => !x.pending && !x.error) && JSON.stringify(sample[0]) === JSON.stringify(sample[1])) {observations = sample; break;}
      await delay(500);
    }
    await writeFile(join(directory, 'finality-attempts.json'), JSON.stringify(attempts, null, 2));
    if (!observations) throw Error('Native Beacon/execution checkpoints did not converge within 30 seconds');
    await writeFile(join(directory, 'finality.json'), JSON.stringify(observations, null, 2));
    console.log(JSON.stringify({nativeFinalityMatched:true, observations}));
  }
  if (stabilityEpochs) {
    const samples = [];
    const result = await observeStableFinality(endpoints, stabilityEpochs, async sample => {
      samples.push(sample);
      await writeFile(join(directory, 'stability-samples.json'), JSON.stringify(samples, null, 2));
    });
    await writeFile(join(directory, 'stability-result.json'), JSON.stringify(result, null, 2));
    console.log(JSON.stringify({nativeStableFinality:result}));
  }
  console.log(JSON.stringify({assertoorExecuted: true, runs: completed, evidence: directory, productionQualified: false}));
 } catch (error) {
  failed = error;
  // Capture read-only evidence while the native processes still exist. Never
  // turn a failed test green if diagnostic reads themselves are unavailable.
  const observations = await Promise.all(endpoints.map(async endpoint => {
    const result = {endpoint};
    for (const [key, path] of [['head','/eth/v2/beacon/blocks/head'], ['checkpoints','/eth/v1/beacon/states/head/finality_checkpoints']]) {
      try {const response = await fetch(endpoint + path, {headers:{Accept:'application/json'}, signal: AbortSignal.timeout(5000)}); result[key] = {status:response.status, body:await response.json()};}
      catch (failure) {result[key] = {error:String(failure)};}
    }
    return result;
  }));
  await writeFile(join(directory, 'failure-observations.json'), JSON.stringify(observations, null, 2));
} finally {
  stop(); const kill = setTimeout(() => {if (!terminated) child.kill('SIGKILL');}, 3000);
  await closed; clearTimeout(kill);
  await writeFile(join(directory, 'assertoor.log'), logs);
  console.log(JSON.stringify({assertoorEvidence: directory}));
}
if (failed) {console.error(failed); console.error(logs.slice(-16000)); process.exitCode = 1;}
