// One read and one immutable byte copy for every public JSON proof binding.
// An omitted pin returns provenance only; the caller must bind its SHA externally.
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {TextDecoder} from 'node:util';
export const maximumPinnedJsonBytes=16*1024*1024;
export async function readPinnedJson(path,expectedSha256,read=readFile){
 assert.ok(typeof path==='string'&&path.length>0,'Pinned JSON path must be a nonempty string');
 assert.equal(typeof read,'function','Pinned JSON reader must be a function');
 if(expectedSha256!==undefined)assert.ok(typeof expectedSha256==='string'&&expectedSha256.length===64&&/^[0-9a-f]{64}$/.test(expectedSha256),'Expected SHA-256 must be canonical lowercase 64-character hex');
 const input=await read(path);
 assert.ok(Buffer.isBuffer(input),'Pinned JSON reader must return a Buffer');
 assert.ok(input.length<=maximumPinnedJsonBytes,'Pinned JSON byte limit exceeded');
 // Copy once so the reader cannot later mutate the bytes hashed and parsed.
 const bytes=Buffer.from(input),sha256=createHash('sha256').update(bytes).digest('hex');
 if(expectedSha256!==undefined)assert.equal(sha256,expectedSha256,'Pinned JSON SHA-256 mismatch');
 let text;
 try{text=new TextDecoder('utf-8',{fatal:true,ignoreBOM:true}).decode(bytes);}
 catch{throw Error('Pinned JSON is not valid UTF-8');}
 let value;
 try{value=JSON.parse(text);}
 catch{throw Error('Pinned JSON is invalid JSON');}
 return {value,sha256};
}
