#!/usr/bin/env node
// Integrity preflight only. Never promotes an image, opens chain data or signs.
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {readFile,lstat} from 'node:fs/promises';
import {resolve,dirname} from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const exec=promisify(execFile),hash=v=>typeof v==='string'&&/^[0-9a-f]{64}$/.test(v);
export function validateManifest(m){
 if(m?.format!=='nano-candidate-v1'||!/^sha256:[0-9a-f]{64}$/.test(m.image??'')||! /^[0-9a-f]{40}$/.test(m.sourceRevision??'')||!['arm64','amd64'].includes(m.architecture))throw Error('Immutable image, revision and architecture required');
 if(!hash(m.executables?.nano)||!hash(m.executables?.['nano-p2p']))throw Error('Two executable hashes required');
 if(!Number.isSafeInteger(m.maximumArtifactBytes)||m.maximumArtifactBytes<=0||!Array.isArray(m.files))throw Error('Bounded artifact list required');
 const kinds=new Set(),paths=new Set();
 for(const f of m.files){if(!['genesis','node','profile','evidence'].includes(f.kind)||typeof f.file!=='string'||!f.file||!hash(f.sha256)||paths.has(f.file))throw Error('Invalid/duplicate artifact');paths.add(f.file);if(f.kind!=='evidence'&&kinds.has(f.kind))throw Error('Duplicate configuration role');kinds.add(f.kind);}
 if(['genesis','node','profile','evidence'].some(k=>!kinds.has(k)))throw Error('Configuration and evidence roles required');return m;
}
export async function verifyCandidate(manifest,{read,daemon,inspect,digests,libraries}){
 const m=validateManifest(manifest);let bytes=0;
 for(const f of m.files){const content=await read(f.file,m.maximumArtifactBytes-bytes);bytes+=content.length;if(bytes>m.maximumArtifactBytes||createHash('sha256').update(content).digest('hex')!==f.sha256)throw Error('Artifact integrity rejected: '+f.kind);}
 const host=await daemon();if(host.os!=='linux'||host.architecture!==m.architecture)throw Error('Docker daemon platform mismatch');
 const image=await inspect(m.image);
 if(image.Id!==m.image||image.Architecture!==m.architecture||image.Config?.Labels?.['org.opencontainers.image.revision']!==m.sourceRevision)throw Error('Image identity/architecture/source label mismatch');
 const observed=await digests(m.image);if(observed.nano!==m.executables.nano||observed['nano-p2p']!==m.executables['nano-p2p'])throw Error('Executable hash mismatch');
 const output=await libraries(m.image);if(typeof output!=='string'||!output.trim()||/not found/.test(output))throw Error('Unresolved executable libraries');
 return {integrityVerified:true,image:m.image,sourceRevision:m.sourceRevision,architecture:m.architecture,artifactBytes:bytes,executables:observed,productionQualified:false};
}
export async function checkCandidate(file){
 const manifest=JSON.parse(await readFile(file,'utf8')),base=dirname(resolve(file));
 const run=async(args)=>(await exec('docker',args,{timeout:30000,maxBuffer:1024*1024})).stdout;
 const sandbox=['run','--rm','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','512m','--cpus','1'];
 return verifyCandidate(manifest,{
  read:async(name,budget)=>{const path=resolve(base,name),stat=await lstat(path);if(!stat.isFile()||stat.size>budget)throw Error('Artifact file/budget rejected');const b=await readFile(path);if(b.length>budget)throw Error('Artifact grew beyond budget');return b;},
  daemon:async()=>{const text=(await run(['info','--format','{{.OSType}} {{.Architecture}}'])).trim().split(/\s+/);return {os:text[0],architecture:({aarch64:'arm64',x86_64:'amd64',arm64:'arm64',amd64:'amd64'})[text[1]]};},
  inspect:async id=>JSON.parse(await run(['image','inspect',id,'--format','{{json .}}'])),
  digests:async id=>{
   const text=await run([...sandbox,'--entrypoint','/usr/bin/sha256sum',id,'/usr/local/bin/nano','/usr/local/bin/nano-p2p']);
   const result={};for(const line of text.trim().split('\n')){const match=/^([0-9a-f]{64})\s+\/usr\/local\/bin\/(nano|nano-p2p)$/.exec(line);if(!match||result[match[2]])throw Error('Invalid executable checksum output');result[match[2]]=match[1];}return result;
  },
  libraries:async id=>run([...sandbox,'--entrypoint','/usr/bin/ldd',id,'/usr/local/bin/nano','/usr/local/bin/nano-p2p'])
 });
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 if(process.argv.length!==3){console.error('Usage: check-candidate.mjs reviewed-manifest.json');process.exitCode=1;}
 else try{console.log(JSON.stringify(await checkCandidate(process.argv[2])));}catch(e){console.error(e.message);process.exitCode=1;}
}
