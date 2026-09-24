import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
const require=createRequire(import.meta.url);

// Keep the published Shanghai artifact unchanged. A Paris network needs
// bytecode compiled for Paris; enabling newer opcodes in the node is incorrect.
export function wrappedArtifact() {
 const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../../..');
 const artifact=JSON.parse(fs.readFileSync(path.join(root,'contracts/WrappedNano.json'),'utf8'));
 if(process.env.NANO_TEST_EVM_FORK!=='Paris')return artifact;
 const solc=require('solc');
 assert.equal(solc.version(),artifact.compiler,'same pinned Solidity compiler');
 const source=fs.readFileSync(path.join(root,'contracts/WrappedNano.sol'),'utf8');
 const input={language:'Solidity',sources:{'WrappedNano.sol':{content:source}},settings:{...artifact.settings,evmVersion:'paris',outputSelection:{'*':{'*':['abi','evm.bytecode.object']}}}};
 const output=JSON.parse(solc.compile(JSON.stringify(input),{import:(name:string)=>{
  assert(name.startsWith('@openzeppelin/contracts/'),'only pinned OpenZeppelin imports');
  return {contents:fs.readFileSync(require.resolve(name),'utf8')};
 }}));
 assert(!(output.errors??[]).some((e:any)=>e.severity==='error'),JSON.stringify(output.errors));
 const contract=output.contracts['WrappedNano.sol'].WrappedNano;
 return {abi:contract.abi,bytecode:'0x'+contract.evm.bytecode.object};
}
