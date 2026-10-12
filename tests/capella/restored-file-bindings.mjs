import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {lstat,readFile,readdir} from 'node:fs/promises';
import {join,isAbsolute} from 'node:path';
const sha=x=>createHash('sha256').update(x).digest('hex');
const relative=p=>typeof p==='string'&&p.length>0&&!isAbsolute(p)&&!p.includes('\\')&&!p.includes('\0')&&!p.includes('\n')&&p.split('/').every(x=>x!==''&&x!=='.'&&x!=='..');
export function validateFileManifest(manifest){
 assert.ok(manifest&&typeof manifest==='object'&&!Array.isArray(manifest));
 const rows=Object.entries(manifest);assert.ok(rows.length>0,'Empty data manifest cannot qualify restoration');
 for(const [path,digest] of rows){assert.ok(relative(path),'Unsafe relative manifest path');assert.match(digest,/^[a-f0-9]{64}$/);}
 return manifest;
}
export function parseVolumeFileHashes(text){
 assert.equal(typeof text,'string');const rows=[];const seen=new Set();
 for(const line of text.trimEnd().split('\n')){const m=/^([a-f0-9]{64})  \/var\/lib\/nano\/(.+)$/.exec(line);assert.ok(m,'Unexpected volume hash row');assert.ok(relative(m[2]));assert.ok(!seen.has(m[2]),'Duplicate volume file');seen.add(m[2]);rows.push([m[2],m[1]]);}
 return validateFileManifest(Object.fromEntries(rows));
}
export function verifyDataManifest(actual,expected){
 validateFileManifest(actual);validateFileManifest(expected);
 assert.deepEqual(Object.keys(actual).sort(),Object.keys(expected).sort(),'Data file inventory differs');
 for(const [p,h] of Object.entries(expected))assert.equal(actual[p],h,'Data file hash differs: '+p);
 assert.ok(!Object.hasOwn(actual,'peer.key'),'Prepared data must omit only the recorded ephemeral peer identity');
 return true;
}
export async function readDataManifest(root){
 assert.ok((await lstat(root)).isDirectory());const rows=[];
 async function visit(path,base=''){
  for(const e of await readdir(path,{withFileTypes:true})){const rel=base?base+'/'+e.name:e.name;assert.ok(relative(rel));const next=join(path,e.name);const info=await lstat(next);assert.ok(!info.isSymbolicLink(),'Data symlink is not an independent copy');
   if(info.isDirectory())await visit(next,rel);else{assert.ok(info.isFile(),'Unexpected data file kind');rows.push([rel,sha(await readFile(next))]);}
  }
 }
 await visit(root);return validateFileManifest(Object.fromEntries(rows));
}
export async function verifyFixtureFiles(files,profileSha){
 assert.ok(files&&typeof files==='object'&&!Array.isArray(files));assert.deepEqual(Object.keys(files).sort(),['genesis.json','initial.json','profile.json']);
 for(const name of Object.keys(files)){const f=files[name];assert.deepEqual(Object.keys(f).sort(),['path','sha256']);assert.equal(typeof f.path,'string');assert.ok(isAbsolute(f.path));assert.match(f.sha256,/^[a-f0-9]{64}$/);assert.ok((await lstat(f.path)).isFile(),'Fixture must be a regular file');assert.equal(sha(await readFile(f.path)),f.sha256,'Fixture bytes differ: '+name);}
 assert.equal(files['profile.json'].sha256,profileSha.replace(/^0x/,''),'Fixture profile differs from plan');return true;
}

export function verifyRuntimeVolumeReceipt(expected,receipt,volume){
 validateFileManifest(expected);assert.ok(receipt&&typeof receipt==='object');assert.equal(receipt.format,'nano-runtime-recovered-volume-copy-v1');assert.equal(receipt.volume,volume);assert.ok(!Object.hasOwn(expected,'peer.key'));
 assert.match(receipt.sourceVolume,/^nano-restored-three-node-vps-[a-z0-9-]+-data$/);assert.notEqual(receipt.sourceVolume,volume,'Original failed volume cannot be used as recovered destination');
 for(const field of ['sourceManifestSha256','recoveredManifestSha256','captureSHA256','removedPeerKeySha256'])assert.match(receipt[field],/^[a-f0-9]{64}$/);
 assert.equal(receipt.files,Object.keys(expected).length);assert.equal(receipt.physicallyIndependentFiles,true);assert.equal(receipt.onlyPeerIdentityOmitted,true);
 const source={...expected,'peer.key':receipt.removedPeerKeySha256};const raw=Object.entries(source).map(([name,h])=>h+'  /recovery/data/'+name+'\n').sort().join('');assert.equal(sha(raw),receipt.recoveredManifestSha256,'Volume copy differs from sealed recovered source');return true;
}

export function manifestDigest(manifest,prefix){
 validateFileManifest(manifest);assert.ok(['/source','/recovery/data'].includes(prefix));return sha(Object.entries(manifest).map(([name,h])=>h+'  '+prefix+'/'+name+'\n').sort().join(''));
}
