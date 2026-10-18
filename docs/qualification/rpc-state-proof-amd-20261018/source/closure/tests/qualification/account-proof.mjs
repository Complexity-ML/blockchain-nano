// Diagnostic account trie proof verifier. Roots must come from the intended parent header.
import assert from 'node:assert/strict';
import {createRequire} from 'node:module';
const require=createRequire(new URL('../hardhat/package.json',import.meta.url));
const {keccak256,decodeRlp,encodeRlp}=require('ethers');
export function verifyAccountProof(root,address,proof){
 assert.match(root,/^0x[0-9a-fA-F]{64}$/);assert.match(address,/^0x[0-9a-fA-F]{40}$/);
 assert(Array.isArray(proof));
 if(root.toLowerCase()===keccak256('0x80')){assert.equal(proof.length,0);return {exists:false,reason:'empty-trie'};}
 const path=keccak256(address).slice(2);let offset=0,index=0,expected=root.toLowerCase();
 function finish(value){assert.equal(index,proof.length,'unused proof nodes');return value;}
 for(let steps=0;steps<130;++steps){
  let node;
  if(Array.isArray(expected)){assert((encodeRlp(expected).length-2)/2<32,'oversized embedded node');node=expected;}
  else{assert.match(expected,/^0x[0-9a-f]{64}$/,'invalid child reference');assert(index<proof.length,'truncated proof');const raw=proof[index++];assert.equal(keccak256(raw),expected,'node hash mismatch');node=decodeRlp(raw);}
  assert(Array.isArray(node));
  if(node.length===17){
   assert(offset<path.length,'unexpected account branch value');
   const child=node[parseInt(path[offset++],16)];
   if(child==='0x')return finish({exists:false,reason:'empty-branch'});
   expected=child;continue;
  }
  assert.equal(node.length,2,'invalid node arity');
  assert.equal(typeof node[0],'string');const compact=node[0].slice(2);
  assert(compact.length>=2);const flag=parseInt(compact[0],16);assert(flag>=0&&flag<=3);
  if(flag%2===0)assert.equal(compact[1],'0','noncanonical path padding');
  const segment=compact.slice(flag%2?1:2);assert(offset+segment.length<=path.length,'path too long');
  if(flag<2)assert(segment.length>0,'empty extension');
  if(flag>=2)assert.equal(offset+segment.length,path.length,'short account leaf');
  if(path.slice(offset,offset+segment.length)!==segment)return finish({exists:false,reason:'different-path'});
  offset+=segment.length;
  if(flag<2){expected=node[1];continue;}
  const account=decodeRlp(node[1]);assert(Array.isArray(account)&&account.length===4,'invalid account');
  assert.match(account[2],/^0x[0-9a-f]{64}$/);assert.match(account[3],/^0x[0-9a-f]{64}$/);
  return finish({exists:true,account});
 }
 throw Error('proof traversal limit');
}
