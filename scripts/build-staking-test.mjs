// Rebuild adversarial fixture with the same pinned compiler/settings as NanoStaking.
import fs from 'node:fs';
import crypto from 'node:crypto';
import {createRequire} from 'node:module';
const require = createRequire(import.meta.url);
const solc = require('../.deps/solc/node_modules/solc');
if (!solc.version().startsWith('0.8.28+')) throw new Error('Expected solc 0.8.28');
const source = fs.readFileSync('tests/contracts/StakingReceiver.sol','utf8');
const settings = JSON.parse(fs.readFileSync('contracts/NanoStaking.json','utf8')).settings;
const output = JSON.parse(solc.compile(JSON.stringify({language:'Solidity',sources:{'StakingReceiver.sol':{content:source}},settings})));
for (const e of output.errors || []) if (e.severity==='error') throw new Error(e.formattedMessage);
const c = output.contracts['StakingReceiver.sol'].StakingReceiver;
fs.writeFileSync('tests/fixtures/staking-receiver.json',JSON.stringify({compiler:solc.version(),sourceSha256:crypto.createHash('sha256').update(source).digest('hex'),settings,bytecode:'0x'+c.evm.bytecode.object,methodIdentifiers:c.evm.methodIdentifiers},null,2)+'\n');
