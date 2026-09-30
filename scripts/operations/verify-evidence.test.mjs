import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,symlink,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {createHash} from 'node:crypto';
import {verifyEvidence} from './verify-evidence.mjs';
const digest=b=>createHash('sha256').update(b).digest('hex');
async function fixture(fn,index){const dir=await mkdtemp(join(tmpdir(),'nano-evidence-'));try{const body=Buffer.from('actual log\n');await writeFile(join(dir,'run.log'),body);const raw=JSON.stringify({filesSha256:index??{'run.log':digest(body)}}),file=join(dir,'manifest.json');await writeFile(file,raw);await fn({dir,file,hash:digest(raw),raw});}finally{await rm(dir,{recursive:true,force:true});}}
test('verify actual files without promoting their semantics or source claims',()=>fixture(async f=>{const r=await verifyEvidence(f.file,f.hash);assert.equal(r.archiveIntegrityVerified,true);assert.equal(r.files,1);assert.equal(r.productionQualified,false);assert.equal(r.qualificationSemanticsVerified,false);assert.equal(r.sourceProvenanceVerified,false);}));
test('tampered child log cannot pass an unchanged manifest',()=>fixture(async f=>{await writeFile(join(f.dir,'run.log'),'forged success');await assert.rejects(verifyEvidence(f.file,f.hash),/file integrity/);}));
test('rewritten manifest requires independent hash review',()=>fixture(async f=>{await writeFile(f.file,JSON.stringify({filesSha256:{'run.log':'f'.repeat(64)}}));await assert.rejects(verifyEvidence(f.file,f.hash),/manifest hash/);}));
test('missing file and symbolic link are refused',()=>fixture(async f=>{await rm(join(f.dir,'run.log'));await assert.rejects(verifyEvidence(f.file,f.hash));await symlink(f.file,join(f.dir,'run.log'));await assert.rejects(verifyEvidence(f.file,f.hash));}));
test('path traversal, absolute entries and malformed hashes are refused',async()=>{for(const index of [{'../run.log':'a'.repeat(64)},{'/etc/passwd':'a'.repeat(64)},{'run.log':'invalid'},{'manifest.json':'a'.repeat(64)}])await fixture(async f=>assert.rejects(verifyEvidence(f.file,f.hash),/Unsafe/),index);});
test('total read budget includes the reviewed manifest itself',()=>fixture(async f=>{await assert.rejects(verifyEvidence(f.file,f.hash,Buffer.byteLength(f.raw)),/oversized/);for(const budget of [0,-1,NaN,Infinity])await assert.rejects(verifyEvidence(f.file,f.hash,budget),/budget/);}));
test('symlink manifest itself cannot become an independently anchored archive',()=>fixture(async f=>{const alias=join(f.dir,'alias.json');await symlink(f.file,alias);await assert.rejects(verifyEvidence(alias,f.hash));}));
test('oversized content returned by a read adapter exceeds the total budget',()=>fixture(async f=>{let calls=0;await assert.rejects(verifyEvidence(f.file,f.hash,Buffer.byteLength(f.raw)+1,{read:async()=>++calls===1?Buffer.from(f.raw):Buffer.alloc(2)}),/file integrity/);}));
