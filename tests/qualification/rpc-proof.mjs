// Independent qualification verifier. The trusted root is supplied separately;
// verifying this response does not authenticate consensus/finality or its origin.
import assert from 'node:assert/strict';
import {createRequire} from 'node:module';
import {verifyAccountProof} from './account-proof.mjs';
const require=createRequire(new URL('../hardhat/package.json',import.meta.url));
const {keccak256,decodeRlp,encodeRlp}=require('ethers');
const emptyRoot=keccak256('0x80'),emptyCode=keccak256('0x');
function quantity(value){assert.equal(typeof value,'string');assert.match(value,/^0x(?:0|[1-9a-f][0-9a-f]*)$/);const n=BigInt(value);assert(n<(1n<<256n));return n;}
function integer(value){assert.equal(typeof value,'string');assert.match(value,/^0x(?:[0-9a-f]{2})*$/);assert(value==='0x'||!value.startsWith('0x00'),'noncanonical RLP integer');assert(value.length<=66);return value==='0x'?0n:BigInt(value);}
function canonical(raw){assert.equal(typeof raw,'string');assert.match(raw,/^0x(?:[0-9a-f]{2})*$/);const node=decodeRlp(raw);assert.equal(encodeRlp(node),raw,'noncanonical RLP');return node;}
function pathProof(root,path,proof){
 assert.match(root,/^0x[0-9a-f]{64}$/);assert.match(path,/^[0-9a-f]{64}$/);assert(Array.isArray(proof)&&proof.length<=130);let bytes=0;for(const raw of proof){assert.equal(typeof raw,'string');bytes+=(raw.length-2)/2;assert(bytes<=16*1024*1024);canonical(raw);}
 if(root===emptyRoot){assert.equal(proof.length,0);return null;}
 let expected=root,offset=0,index=0;
 const finish=value=>{assert.equal(index,proof.length,'unused proof nodes');return value;};
 for(let steps=0;steps<130;++steps){let node;
  if(Array.isArray(expected)){assert((encodeRlp(expected).length-2)/2<32,'oversized inline reference');node=expected;}
  else{assert.match(expected,/^0x[0-9a-f]{64}$/);assert(index<proof.length,'truncated proof');const raw=proof[index++];assert(index===1||(raw.length-2)/2>=32,'short child must be inline');assert.equal(keccak256(raw),expected,'node hash mismatch');node=canonical(raw);}
  assert(Array.isArray(node));
  if(node.length===17){assert(offset<64,'unexpected fixed-key branch value');expected=node[Number.parseInt(path[offset++],16)];if(expected==='0x')return finish(null);continue;}
  assert.equal(node.length,2);assert.equal(typeof node[0],'string');const compact=node[0].slice(2);assert(compact.length>=2);const flag=Number.parseInt(compact[0],16);assert(flag<=3);if(!(flag&1))assert.equal(compact[1],'0','invalid path padding');const segment=compact.slice(flag&1?1:2);assert(offset+segment.length<=64);if(flag<2)assert(segment.length>0);else assert.equal(offset+segment.length,64,'short secure-trie leaf');
  if(path.slice(offset,offset+segment.length)!==segment)return finish(null);offset+=segment.length;if(flag<2){expected=node[1];continue;}assert.equal(typeof node[1],'string');return finish(node[1]);
 }
 throw Error('proof traversal limit');
}
export function verifyStorageProof(root,key,proof){
 const normalized=BigInt(key);assert(normalized>=0n&&normalized<(1n<<256n));const slot=`0x${normalized.toString(16).padStart(64,'0')}`;const leaf=pathProof(root,keccak256(slot).slice(2),proof);if(leaf===null)return 0n;const value=integer(canonical(leaf));assert(value>0n,'zero storage leaf must be absent');return value;
}
export function verifyRpcProof(root,address,keys,response){
 assert.match(root,/^0x[0-9a-f]{64}$/);assert.match(address,/^0x[0-9a-fA-F]{40}$/);assert(Array.isArray(keys)&&keys.length<=65536);assert(response&&typeof response==='object'&&!Array.isArray(response));assert.deepEqual(Object.keys(response).sort(),['accountProof','address','balance','codeHash','nonce','storageHash','storageProof'].sort());assert.equal(response.address,address.toLowerCase());
 const account=verifyAccountProof(root,address,response.accountProof);const leaf=pathProof(root,keccak256(address).slice(2),response.accountProof);assert.equal(account.exists,leaf!==null);let nonce=0n,balance=0n,storageRoot=emptyRoot,codeHash=emptyCode;
 if(leaf!==null){const values=canonical(leaf);assert(Array.isArray(values)&&values.length===4);nonce=integer(values[0]);balance=integer(values[1]);assert.match(values[2],/^0x[0-9a-f]{64}$/);assert.match(values[3],/^0x[0-9a-f]{64}$/);[storageRoot,codeHash]=values.slice(2);assert.deepEqual(values,account.account);}
 assert.equal(quantity(response.nonce),nonce);assert.equal(quantity(response.balance),balance);assert.equal(response.codeHash,codeHash);assert.equal(response.storageHash,storageRoot);assert(Array.isArray(response.storageProof));assert.equal(response.storageProof.length,keys.length);const values=[];
 for(let i=0;i<keys.length;++i){const entry=response.storageProof[i];assert.deepEqual(Object.keys(entry).sort(),['key','proof','value']);const key=BigInt(keys[i]);assert.equal(quantity(entry.key),key,'storage key order mismatch');const value=verifyStorageProof(storageRoot,key,entry.proof);assert.equal(quantity(entry.value),value,'storage value mismatch');values.push(`0x${value.toString(16)}`);}
 return {exists:account.exists,values};
}
export function verifyProofVector(vector){
 assert.equal(vector.scenario,'proof-vector');const result=verifyRpcProof(vector.root,vector.address,vector.keys,vector.response);assert.equal(result.exists,vector.expectedPresent);assert.deepEqual(result.values,vector.expectedValues);return result;
}
