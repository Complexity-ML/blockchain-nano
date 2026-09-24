// Offline signed-input generator. Compare via compare-geth-sequence.mjs.
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve} from 'node:path';
import {createRequire} from 'node:module';
import assert from 'node:assert/strict';
const {Wallet,keccak256}=createRequire(new URL('../hardhat/package.json',import.meta.url))('ethers');
const [templatePath,output]=process.argv.slice(2);
assert(templatePath&&output,'usage: template-input.json new-output-directory');
const template=JSON.parse(readFileSync(templatePath));
mkdirSync(output,{recursive:false});
const wallet=new Wallet('0x'+'1'.padStart(64,'0'));
const expectations=[];
for(const fork of ['Shanghai','Cancun','Prague','Osaka'])for(const type of [0,1,2]){
 let nonce=0;
 for(const length of [31,32,33,49151,49152]){
  const data='0x5f5ff3'+'00'.repeat(length-3);
  const fees=type===2?{maxFeePerGas:4000000000n,maxPriorityFeePerGas:1000000000n}:{gasPrice:2000000000n};
  const raw=await wallet.signTransaction({chainId:1337,type,nonce,gasLimit:3000000,...fees,data});
  const block={...template.block,number:'0x'+(nonce+1).toString(16),timestamp:'0x'+(1800000000+nonce).toString(16),baseFeePerGas:'0x3b9aca00',excessBlobGas:'0x0'};
  // This is a synthetic execution context, not the template's committed header.
  delete block.hash;
  if(fork==='Shanghai')for(const key of ['blobGasUsed','excessBlobGas','parentBeaconBlockRoot'])delete block[key];
  const input={fork,chainId:'0x539',pre:{[wallet.address.toLowerCase()]:{balance:'0xd3c21bcecceda1000000',nonce:'0x0',code:'0x',storage:{}}},block,rawTransaction:raw};
  const name=`initcode-type${type}-${fork}-${nonce}`;
  writeFileSync(resolve(output,name+'-input.json'),JSON.stringify(input,null,2)+'\n');
  // Three nonzero initcode bytes; two PUSH0 (2 gas each) then RETURN(0,0).
  const intrinsic=21000+32000+3*16+(length-3)*4+2*Math.ceil(length/32);
  const calldataFloor=21000+10*((length-3)+3*4);
  const expectedGas=['Prague','Osaka'].includes(fork)?Math.max(intrinsic+4,calldataFloor):intrinsic+4;
  expectations.push({case:name,fork,type,length,nonce,hash:keccak256(raw),intrinsic,calldataFloor,expectedGas});
  nonce++;
 }
}
writeFileSync(resolve(output,'expectations.json'),JSON.stringify({scope:'Synthetic sequential creations; no RPC admission, network fork activation or public funds. Header fields inherited only as diagnostic context, not validated block commitments.',cases:expectations},null,2)+'\n');
assert.equal(expectations.length,60);
