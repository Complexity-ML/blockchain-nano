#!/usr/bin/env node
// Read-only archive integrity; neither test semantics nor production readiness.
import {open} from 'node:fs/promises';
import {constants} from 'node:fs';
import {createHash} from 'node:crypto';
import {dirname,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
const hash=x=>typeof x==='string'&&/^[0-9a-f]{64}$/.test(x);
async function boundedRead(path,budget){
 const f=await open(path,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
 try{
  const stat=await f.stat();if(!stat.isFile()||stat.size>budget)throw Error('Nonregular or oversized evidence file');
  const chunks=[];let size=0;
  while(true){const b=Buffer.alloc(Math.min(65536,budget-size+1));const {bytesRead}=await f.read(b,0,b.length,null);if(!bytesRead)break;size+=bytesRead;if(size>budget)throw Error('Evidence grew beyond byte budget');chunks.push(b.subarray(0,bytesRead));}
  return Buffer.concat(chunks,size);
 }finally{await f.close();}
}
export async function verifyEvidence(file,reviewedManifestSha256,maximumBytes=33554432,{read=boundedRead}={}){
 if(!hash(reviewedManifestSha256)||!Number.isSafeInteger(maximumBytes)||maximumBytes<=0)throw Error('Reviewed manifest hash and positive byte budget required');
 const manifestPath=resolve(file),raw=await read(manifestPath,maximumBytes);
 if(createHash('sha256').update(raw).digest('hex')!==reviewedManifestSha256)throw Error('Reviewed evidence manifest hash mismatch');
 const m=JSON.parse(raw),entries=Object.entries(m.filesSha256??{});
 if(!entries.length||entries.length>256||Array.isArray(m.filesSha256)||typeof m.filesSha256!=='object')throw Error('Bounded evidence file index required');
 let bytes=raw.length;
 for(const [name,digest] of entries){
  if(!/^[A-Za-z0-9][A-Za-z0-9_.-]*$/.test(name)||name==='.'||name==='..'||name===manifestPath.slice(manifestPath.lastIndexOf('/')+1)||!hash(digest))throw Error('Unsafe evidence index entry');
  const content=await read(resolve(dirname(manifestPath),name),maximumBytes-bytes);bytes+=content.length;
  if(bytes>maximumBytes||createHash('sha256').update(content).digest('hex')!==digest)throw Error('Evidence file integrity mismatch: '+name);
 }
 return {archiveIntegrityVerified:true,manifestSha256:reviewedManifestSha256,files:entries.length,bytes,qualificationSemanticsVerified:false,sourceProvenanceVerified:false,productionQualified:false};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 try{if(process.argv.length<4||process.argv.length>5)throw Error('Usage: verify-evidence.mjs manifest.json reviewed-sha256 [total-byte-budget]');console.log(JSON.stringify(await verifyEvidence(process.argv[2],process.argv[3],process.argv[4]===undefined?33554432:Number(process.argv[4]))));}
 catch(e){console.error(e.message);process.exitCode=1;}
}
