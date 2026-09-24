// npm install --prefix .deps/solc --ignore-scripts solc@0.8.28
import fs from 'node:fs';
import crypto from 'node:crypto';
import {createRequire} from 'node:module';
const require = createRequire(import.meta.url);
const solc = require(process.env.NANO_SOLC_MODULE || '../.deps/solc/node_modules/solc');
if (!solc.version().startsWith('0.8.28+')) throw new Error('Expected solc 0.8.28');
const source = fs.readFileSync('contracts/NanoStaking.sol','utf8');
const settings = {evmVersion:'paris',optimizer:{enabled:true,runs:200},
 metadata:{bytecodeHash:'none'},outputSelection:{'*':{'*':['abi','evm.bytecode.object','evm.deployedBytecode.object','evm.methodIdentifiers','storageLayout']}}};
const output = JSON.parse(solc.compile(JSON.stringify({language:'Solidity',sources:{'NanoStaking.sol':{content:source}},settings})));
for(const e of output.errors || []) { if(e.severity==='error') throw new Error(e.formattedMessage); process.stderr.write(e.formattedMessage); }
const c = output.contracts['NanoStaking.sol'].NanoStaking;
fs.writeFileSync('contracts/NanoStaking.json',JSON.stringify({compiler:solc.version(),sourceSha256:crypto.createHash('sha256').update(source).digest('hex'),settings,
 abi:c.abi,bytecode:'0x'+c.evm.bytecode.object,deployedBytecode:'0x'+c.evm.deployedBytecode.object,methodIdentifiers:c.evm.methodIdentifiers,storageLayout:c.storageLayout},null,2)+'\n');

fs.writeFileSync("contracts/NanoStaking.hex","0x"+c.evm.bytecode.object+"\n");
