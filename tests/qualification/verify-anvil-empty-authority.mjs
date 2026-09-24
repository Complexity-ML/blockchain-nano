// Diagnostic inclusion proof check; deliberately does not claim absence verification.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createRequire} from 'node:module';
import {createHash} from 'node:crypto';
const require=createRequire(new URL('../hardhat/package.json',import.meta.url));
const {keccak256,decodeRlp}=require('ethers');
const files=process.argv.slice(2);
assert.equal(files.length,3,'usage: proof.json parent-input.json transaction-input.json');
const [proof,parentInput,txInput]=files.map(f=>JSON.parse(readFileSync(f)));
const parent=parentInput.block;
assert.equal(txInput.block.parentHash,parent.hash,'proof parent must precede transaction');
const path=keccak256(proof.address).slice(2);
let expected=parent.stateRoot,offset=0,account;
for(const raw of proof.accountProof){
 assert.equal(account,undefined,'unexpected node after leaf');
 assert.equal(keccak256(raw),expected,'proof node hash mismatch');
 const node=decodeRlp(raw);
 if(node.length===17){assert(offset<path.length);expected=node[parseInt(path[offset++],16)];continue;}
 assert.equal(node.length,2);
 const compact=node[0].slice(2),flag=parseInt(compact[0],16);
 assert(flag>=0&&flag<=3);
 if(flag%2===0)assert.equal(compact[1],'0');
 const segment=compact.slice(flag%2?1:2);
 assert.equal(path.slice(offset,offset+segment.length),segment,'address path mismatch');
 offset+=segment.length;
 if(flag>=2){assert.equal(offset,path.length);account=decodeRlp(node[1]);}
 else expected=node[1];
}
assert(account,'no included account leaf');assert.equal(account.length,4);
assert.equal(account[0],'0x');assert.equal(account[1],'0x');
assert.equal(account[2],keccak256('0x80'),'nonempty storage root');
assert.equal(account[3],keccak256('0x'),'nonempty code');
console.log(JSON.stringify({proofVerified:true,address:proof.address,parentHash:parent.hash,stateRoot:parent.stateRoot,account,artifacts:files.map(path=>({path,sha256:createHash('sha256').update(readFileSync(path)).digest('hex')}))},null,2));
