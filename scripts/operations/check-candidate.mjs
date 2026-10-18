#!/usr/bin/env node
// Integrity preflight only. Never promotes an image, opens chain data or signs.
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {readFile,lstat} from 'node:fs/promises';
import {resolve,dirname} from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash,randomUUID} from 'node:crypto';
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

// Argument-only installed-entrypoint oracles from tests/test_install.cpp.
// No genesis, request, identity, state path, key or network endpoint is supplied.
const RUNTIME_NAMES=['nano','nano-p2p','nano-storage-maintenance','nano-bootstrap-archive-maintenance','nano-capella-duty-readiness'];
const need=(ok,why)=>{if(!ok)throw Error(why);};
export function verifyRuntimeArgumentResult(name,r){
 need(RUNTIME_NAMES.includes(name)&&r&&Number.isInteger(r.code)&&typeof r.stdout==='string'&&typeof r.stderr==='string'&&Buffer.byteLength(r.stdout)<=65536&&Buffer.byteLength(r.stderr)<=65536,'Bounded runtime terminal/output required');
 if(name==='nano')need(r.code===0&&r.stdout.includes('Usage: nano <'),'Installed nano help failed');
 else if(name==='nano-p2p')need(r.code===1&&r.stderr.includes('Usage: nano-p2p '),'Transport did not refuse before startup');
 else if(name==='nano-storage-maintenance')need(r.code===1&&r.stderr.includes('Usage: compact-owned-copy '),'Storage tool did not refuse before opening a copy');
 else{
  need(r.code===1,'Maintenance tool must refuse empty arguments');const v=JSON.parse(r.stdout);need(v&&typeof v==='object'&&!Array.isArray(v),'One maintenance JSON object required');
  const fixed=name==='nano-bootstrap-archive-maintenance'?{completed:false,refusedStage:'arguments',runtimeStarted:false,productionQualified:false}:{status:'REFUSED_NO_AUTOMATIC_RETRY',refusedStage:'request',reason:'validation_or_io_refused',probeCompleted:false,keysLoaded:false,runtimeStarted:false};
  for(const [k,w]of Object.entries(fixed))need(typeof v[k]===typeof w&&v[k]===w,'Typed maintenance refusal field differs: '+k);
 }
 return {name,exitCode:r.code,argumentOnlyOracleVerified:true};
}
async function runtimeDocker(argv){
 try{const r=await exec('docker',argv,{encoding:'utf8',timeout:10000,killSignal:'SIGKILL',maxBuffer:65536});return {code:0,stdout:r.stdout,stderr:r.stderr};}
 catch(e){if(Number.isInteger(e.code)&&typeof e.stdout==='string'&&typeof e.stderr==='string')return {code:e.code,stdout:e.stdout,stderr:e.stderr};throw e;}
}
export async function smokeCandidateRuntime(image,{invoke=runtimeDocker}={}){
 need(/^sha256:[0-9a-f]{64}$/.test(image),'Immutable candidate image ID required');
 const call=async argv=>{const r=await invoke([...argv]);need(r&&Number.isInteger(r.code)&&typeof r.stdout==='string'&&typeof r.stderr==='string'&&Buffer.byteLength(r.stdout)<=65536&&Buffer.byteLength(r.stderr)<=65536,'Bounded Docker terminal required');return r;};
 const inspected=await call(['image','inspect',image,'--format','{{json .}}']);need(inspected.code===0,'Candidate image inspection refused');const metadata=JSON.parse(inspected.stdout);
 need(metadata.Id===image&&metadata.Os==='linux'&&['arm64','amd64'].includes(metadata.Architecture)&&metadata.Config?.User==='10001:10001','Candidate Linux image/user identity differs');
 // A declared VOLUME would create an anonymous RW mount despite --read-only.
 need(!metadata.Config.Volumes||Object.keys(metadata.Config.Volumes).length===0,'Argument-only smoke refuses automatic image volumes');
 const records=[],closures=[],secondary=[];const owner=randomUUID().replaceAll('-',''),label='org.nanobees.argument-smoke.owner';let first;
 for(let i=0;i<RUNTIME_NAMES.length&&!first;i++){
  const executable=RUNTIME_NAMES[i],name='nano-argument-smoke-'+owner+'-'+i;let attempted=false;
  const inspectOwned=async()=>{
   const r=await call(['container','inspect',name,'--format','{{json .}}']);
   if(r.code===1&&r.stdout===''&&/no such (?:object|container)/i.test(r.stderr)&&r.stderr.includes(name))return null;
   need(r.code===0,'Owned smoke container inspection refused');const c=JSON.parse(r.stdout);
   need(/^[0-9a-f]{64}$/.test(c.Id??'')&&c.Name==='/'+name&&c.Image===image&&c.Config?.Labels?.[label]===owner,'Smoke container ownership differs');return c;
  };
  try{
   need(await inspectOwned()===null,'Smoke name was not absent');attempted=true;
   const args=['run','--rm','--pull=never','--no-healthcheck','--name',name,'--label',label+'='+owner,'--network','none','--read-only','--user','10001:10001','--cap-drop','ALL','--security-opt','no-new-privileges','--memory','512m','--cpus','1','--pids-limit','32','--workdir','/','--entrypoint','/usr/bin/env',image,'-u','NANO_SUPERVISOR_PID','/usr/local/bin/'+executable,...(executable==='nano'?['--help']:[])];
   const r=await call(args);records.push(verifyRuntimeArgumentResult(executable,r));
  }catch(e){first=e;}
  finally{
   if(attempted)try{
    const c=await inspectOwned();if(c){const r=await call(['container','rm','--force',c.Id]);need(r.code===0,'Owned smoke container cleanup refused');}
    need(await inspectOwned()===null,'Owned smoke container remains');closures.push({name,ownedContainerAbsent:true});
   }catch(e){if(first)secondary.push({stage:'owned-cleanup',message:e.message});else first=e;}
  }
 }
 if(first){first.secondaryFailures=secondary;throw first;}
 return {format:'nano-candidate-runtime-arguments-v1',argumentOnlyVerified:true,image,architecture:metadata.Architecture,executablesChecked:records.length,records,closures,supervisorEnvironmentUnset:true,dataArgumentsProvided:false,dataMountsProvided:false,readOnlyRootfsConfigured:true,networkMode:'none',runtimeDutiesAuthorized:false,productionQualified:false};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 if(process.argv.length===4&&process.argv[2]==='--runtime-smoke'){
  try{console.log(JSON.stringify(await smokeCandidateRuntime(process.argv[3])));}catch(e){console.error(JSON.stringify({format:'nano-candidate-runtime-arguments-failure-v1',argumentOnlyVerified:false,firstFailure:{name:e.name,message:e.message},secondaryFailures:e.secondaryFailures??[],productionQualified:false}));process.exitCode=1;}
 }else if(process.argv.length!==3){console.error('Usage: check-candidate.mjs reviewed-manifest.json OR --runtime-smoke sha256:IMAGE_ID');process.exitCode=1;}
 else try{console.log(JSON.stringify(await checkCandidate(process.argv[2])));}catch(e){console.error(e.message);process.exitCode=1;}
}
