#!/usr/bin/env node
// Local qualification only: never point failure-injection harnesses at live funds.
import {mkdtemp, appendFile, writeFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {observeStableFinality} from './stability.mjs';
const endpoints = process.argv.slice(2);
if (endpoints.length !== 2 || endpoints.some(value => {
  const u = new URL(value);
  return u.protocol !== 'http:' || u.hostname !== '127.0.0.1' || u.username || u.password;
})) throw Error('Two loopback node endpoints required');
const directory = await mkdtemp(join(tmpdir(), 'nano-recovery-stability-'));
console.log(JSON.stringify({event:'stability-evidence', directory}));
try {
  const result = await observeStableFinality(endpoints, 3, async sample => {
    await appendFile(join(directory, 'samples.jsonl'), JSON.stringify(sample)+'\n');
    if (sample.advances) console.log(JSON.stringify(sample));
  });
  await writeFile(join(directory, 'result.json'), JSON.stringify(result,null,2)+'\n');
  console.log(JSON.stringify({event:'stability-passed', ...result}));
} catch (error) {
  await writeFile(join(directory, 'failure.txt'), error.stack+'\n');
  console.error(error); process.exitCode=1;
}
