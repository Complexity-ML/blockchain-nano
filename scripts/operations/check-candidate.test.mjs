import test from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {validateManifest,verifyCandidate} from './check-candidate.mjs';
const content=Buffer.from('reviewed fixture'),a='a'.repeat(64),b='b'.repeat(64),revision='c'.repeat(40);
const manifest=()=>({format:'nano-candidate-v1',image:'sha256:'+a,sourceRevision:revision,architecture:'arm64',maximumArtifactBytes:4096,executables:{nano:a,'nano-p2p':b},files:['genesis','node','profile','evidence'].map(kind=>({kind,file:kind+'.json',sha256:createHash('sha256').update(content).digest('hex')}))});
const adapters=()=>({read:async()=>content,daemon:async()=>({os:'linux',architecture:'arm64'}),inspect:async()=>({Id:'sha256:'+a,Architecture:'arm64',Config:{Labels:{'org.opencontainers.image.revision':revision}}}),digests:async()=>({nano:a,'nano-p2p':b}),libraries:async()=> 'libgmp.so => /lib/libgmp.so'});
test('matching identity, artifacts and binaries prove integrity only',async()=>{const r=await verifyCandidate(manifest(),adapters());assert.equal(r.integrityVerified,true);assert.equal(r.productionQualified,false);});
test('mutable image tags cannot be candidates',()=>{const m=manifest();m.image='blockchain-nano:latest';assert.throws(()=>validateManifest(m));});
test('required roles and unique config/artifact references enforced',()=>{const m=manifest();m.files.pop();assert.throws(()=>validateManifest(m));const n=manifest();n.files.push({...n.files[0]});assert.throws(()=>validateManifest(n));});
test('tampered genesis is refused before any container inspection',async()=>{const d=adapters();d.read=async()=>Buffer.from('altered');d.inspect=async()=>{throw Error('must not inspect');};await assert.rejects(verifyCandidate(manifest(),d),/integrity/);});
test('artifact growth exceeds reviewed resource budget',async()=>{const m=manifest();m.maximumArtifactBytes=1;await assert.rejects(verifyCandidate(m,adapters()),/integrity/);});
test('architecture or source mismatch rejected before executable probes',async()=>{for(const field of ['Architecture','source']){const d=adapters(),original=d.inspect;d.inspect=async()=>{const v=await original();if(field==='Architecture')v.Architecture='amd64';else v.Config.Labels['org.opencontainers.image.revision']='d'.repeat(40);return v;};d.digests=async()=>{throw Error('must not probe');};await assert.rejects(verifyCandidate(manifest(),d),/mismatch/);}});
test('image returned under another ID cannot impersonate candidate',async()=>{const d=adapters(),original=d.inspect;d.inspect=async()=>({...await original(),Id:'sha256:'+b});await assert.rejects(verifyCandidate(manifest(),d),/mismatch/);});
test('changed binary rejected before dependency loader invocation',async()=>{const d=adapters();d.digests=async()=>({nano:b,'nano-p2p':b});d.libraries=async()=>{throw Error('must not load');};await assert.rejects(verifyCandidate(manifest(),d),/hash mismatch/);});
test('missing libraries cannot pass integrity gate',async()=>{const d=adapters();d.libraries=async()=> 'librocksdb.so => not found';await assert.rejects(verifyCandidate(manifest(),d),/Unresolved/);});

test('emulated or non-Linux target is refused by deployment gate',async()=>{const d=adapters();d.daemon=async()=>({os:'linux',architecture:'amd64'});await assert.rejects(verifyCandidate(manifest(),d),/platform mismatch/);d.daemon=async()=>({os:'windows',architecture:'arm64'});await assert.rejects(verifyCandidate(manifest(),d),/platform mismatch/);});
test('missing dependency report cannot silently pass',async()=>{const d=adapters();d.libraries=async()=> '';await assert.rejects(verifyCandidate(manifest(),d),/Unresolved/);});
