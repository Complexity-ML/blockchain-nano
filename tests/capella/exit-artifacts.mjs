import assert from 'node:assert/strict';
const digest=x=>assert.match(x??'',/^[0-9a-f]{64}$/,'Exact executable SHA256 required');
const shape=(x,keys)=>{assert.ok(x&&typeof x==='object'&&!Array.isArray(x),'Artifact object required');assert.deepEqual(Object.keys(x).sort(),[...keys].sort(),'Incomplete or unknown artifact fields');};
const absolute=p=>{assert.ok(typeof p==='string'&&/^\/[a-zA-Z0-9_./-]+$/.test(p)&&p!=='/'&&!p.split('/').some(s=>s==='.'||s==='..')&&!p.includes('//'),'Unsafe executable path');return p;};
const names=['nano','p2p','exporter'];
// Synthetic two-node exit qualification only: this manifest cannot deploy a sale
// or authorize automatic recovery on a public network.
export function selectExitArtifacts(input){
 shape(input,['runtimeRevision','native','vps',...(Object.hasOwn(input,'coreRevision')?['coreRevision']:[])]);assert.match(input.runtimeRevision??'',/^[0-9a-f]{40}$/,'Exact runtime revision required');
 if(Object.hasOwn(input,'coreRevision'))assert.match(input.coreRevision??'',/^[0-9a-f]{40}$/,'Exact core revision required');
 shape(input.native,names);shape(input.vps,['image','volume','directory','executables']);shape(input.vps.executables,names);
 assert.match(input.vps.image??'',/^sha256:[0-9a-f]{64}$/,'Immutable builder image required');
 assert.match(input.vps.volume??'',/^[a-zA-Z0-9][a-zA-Z0-9_.-]*$/,'Explicit build volume required');
 const directory=absolute(input.vps.directory);assert.ok(directory.startsWith('/build/')&&directory.length>7,'Explicit build subdirectory required');
 for(const set of [input.native,input.vps.executables]){
  const paths=[];for(const name of names){shape(set[name],['path','sha256']);paths.push(absolute(set[name].path));digest(set[name].sha256);}
  assert.equal(new Set(paths).size,names.length,'Duplicate executable paths');
 }
 for(const entry of Object.values(input.vps.executables))assert.ok(entry.path.startsWith(directory+'/'),'Remote executable escapes selected build directory');
 return {...structuredClone(input),mount:`type=volume,src=${input.vps.volume},dst=/build,readonly`,revisionFile:directory+'/production-source-revision.txt'};
}
export function verifyExitExecutableHashes(expected,text){
 assert.equal(typeof text,'string','Hash output required');const found=new Map();
 for(const line of text.trim().split('\n')){const m=/^([0-9a-f]{64})\s+\*?(.+)$/.exec(line);assert.ok(m,'Malformed executable hash');assert.ok(!found.has(m[2]),'Duplicate executable hash');found.set(m[2],m[1]);}
 assert.equal(found.size,names.length,'Incomplete or extraneous executable inventory');
 for(const entry of Object.values(expected))assert.equal(found.get(entry.path),entry.sha256,'Executable hash mismatch');return Object.fromEntries(found);
}
export function verifyExitVpsArtifacts(selection,info,revision,hashes){
 assert.equal(info?.Id,selection.vps.image,'Selected image changed');assert.equal(info.Architecture,'amd64','Wrong VPS architecture');
 assert.equal(revision.trim(),selection.runtimeRevision,'Build/runtime revision mismatch');
 return verifyExitExecutableHashes(selection.vps.executables,hashes);
}

// Cold data can be reused with a new relay only when the exact core and fixture
// exporter binaries still match the source campaign. The caller must ALSO prove
// that src/include/third_party have no diff against this explicit core revision.
export function verifyExitCoreCompatibility(selection,source){
 const coreRevision=selection.coreRevision??selection.runtimeRevision;
 assert.equal(source.runtimeRevision,coreRevision,'Stopped source/core revision mismatch');
 for(const side of ['native','vps'])for(const name of ['nano','exporter']){
  const a=side==='native'?selection.native:selection.vps.executables;
  const b=side==='native'?source.native:source.vps.executables;
  digest(b?.[name]?.sha256);assert.equal(a[name].sha256,b[name].sha256,'Stopped source core or exporter changed');
 }
 return coreRevision;
}
