import fs from 'node:fs';
import crypto from 'node:crypto';
import {createRequire} from 'node:module';
const solc = createRequire(import.meta.url)('../.deps/solc/node_modules/solc');
if (!solc.version().startsWith('0.8.28+')) throw new Error('Expected solc 0.8.28');
const sources = {};
function collect(dir) {
  for (const entry of fs.readdirSync(dir, {withFileTypes:true})) {
    const path = `${dir}/${entry.name}`;
    if (entry.isDirectory()) collect(path);
    else if (path.endsWith('.sol')) sources[path] = {content:fs.readFileSync(path,'utf8')};
  }
}
collect('tests/contracts/openzeppelin-v5.0.2');
sources['tests/contracts/TestToken.sol'] = {content:fs.readFileSync('tests/contracts/TestToken.sol','utf8')};
sources['tests/contracts/SafeTokenUser.sol'] = {content:fs.readFileSync('tests/contracts/SafeTokenUser.sol','utf8')};
sources['tests/contracts/TestNFT.sol'] = {content:fs.readFileSync('tests/contracts/TestNFT.sol','utf8')};
sources['tests/contracts/TestMarket.sol'] = {content:fs.readFileSync('tests/contracts/TestMarket.sol','utf8')};
const settings = JSON.parse(fs.readFileSync('contracts/NanoStaking.json','utf8')).settings;
const output = JSON.parse(solc.compile(JSON.stringify({language:'Solidity',sources,settings})));
for (const e of output.errors || []) if (e.severity === 'error') throw new Error(e.formattedMessage);
const c = output.contracts['tests/contracts/TestToken.sol'].TestToken;
const sourceSha256 = Object.fromEntries(Object.entries(sources).map(([p,s]) => [p,crypto.createHash('sha256').update(s.content).digest('hex')]));
fs.writeFileSync('tests/fixtures/erc20.json',JSON.stringify({compiler:solc.version(),upstream:'https://github.com/OpenZeppelin/openzeppelin-contracts/tree/v5.0.2',sourceSha256,settings,bytecode:'0x'+c.evm.bytecode.object,deployedBytecode:'0x'+c.evm.deployedBytecode.object,methodIdentifiers:c.evm.methodIdentifiers},null,2)+'\n');

for(const [name,c] of Object.entries(output.contracts['tests/contracts/SafeTokenUser.sol'])) {
 fs.writeFileSync(`tests/fixtures/${name}.json`,JSON.stringify({compiler:solc.version(),sourceSha256,settings,bytecode:'0x'+c.evm.bytecode.object,methodIdentifiers:c.evm.methodIdentifiers},null,2)+'\n');
}

for(const [name,c] of Object.entries(output.contracts['tests/contracts/TestNFT.sol'])) {
 fs.writeFileSync(`tests/fixtures/${name}.json`,JSON.stringify({compiler:solc.version(),sourceSha256,settings,bytecode:'0x'+c.evm.bytecode.object,methodIdentifiers:c.evm.methodIdentifiers},null,2)+'\n');
}

for(const [name,c] of Object.entries(output.contracts['tests/contracts/TestMarket.sol'])) {
 fs.writeFileSync(`tests/fixtures/${name}.json`,JSON.stringify({compiler:solc.version(),sourceSha256,settings,bytecode:'0x'+c.evm.bytecode.object,methodIdentifiers:c.evm.methodIdentifiers},null,2)+'\n');
}
