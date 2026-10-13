import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {readFile,lstat,open} from 'node:fs/promises';
import {constants} from 'node:fs';
import {createHash} from 'node:crypto';
import {join,resolve,relative,sep} from 'node:path';
import assert from 'node:assert/strict';
import {verifyApprovedCandidateCheckout,sha256} from './candidate-source-authority.mjs';
const exec=promisify(execFile);
const git=async(repo,args)=>(await exec('git',args,{cwd:repo,encoding:'buffer',timeout:10000,maxBuffer:32*1024*1024})).stdout;
const scoped=name=>name.startsWith('src/')||name.startsWith('include/')||name.startsWith('cmake/')||name.startsWith('third_party/')||name.startsWith('network/')||name.startsWith('tests/support/')||['CMakeLists.txt','Dockerfile','dependencies.lock.json','tests/test_capella_duty_availability.cpp'].includes(name)||name.startsWith('tests/capella/')&&!name.endsWith('.test.mjs');
async function regularBytes(repo,name){
 const path=resolve(repo,name);assert.ok(relative(repo,path)!=='..'&&!relative(repo,path).startsWith('..'+sep),'Source path escapes checkout');
 let part=repo;for(const element of relative(repo,path).split(sep)){part=join(part,element);assert.ok(!(await lstat(part)).isSymbolicLink(),'Source symlink refused');}
 const fd=await open(path,constants.O_RDONLY|constants.O_NOFOLLOW);
 try{const before=await fd.stat({bigint:true});assert.ok(before.isFile()&&before.size<=32n*1024n*1024n,'Bounded regular public source required');const bytes=await fd.readFile();const after=await fd.stat({bigint:true});for(const key of ['dev','ino','size','mode','mtimeNs','ctimeNs'])assert.equal(after[key],before[key],'Source changed while read');return {bytes,mode:before.mode};}finally{await fd.close();}
}
// Read-only local adapter. No key/capsule/DB paths, no Git mutation or remote
// command, and no runtime. Returned proof is usable only with pure verifier.
export async function auditCandidateCheckout(repo,{authorityBytes,expectedAuthoritySha256,coreReferenceBytes,compileReferenceBytes,sourceReferenceBytes,baselineCmakeBytes}){
 assert.ok(typeof expectedAuthoritySha256==='string'&&/^[0-9a-f]{64}$/.test(expectedAuthoritySha256),'External root authority pin required before Git/source reads');assert.equal(sha256(authorityBytes),expectedAuthoritySha256,'Root authority changed');
 const authority=JSON.parse(new TextDecoder('utf8',{fatal:true}).decode(authorityBytes));assert.equal(sha256(sourceReferenceBytes),authority.sourceReferenceSha256,'Source reference changed');const reference=JSON.parse(new TextDecoder('utf8',{fatal:true}).decode(sourceReferenceBytes));
 repo=resolve(repo);const head=(await git(repo,['rev-parse','HEAD'])).toString().trim();assert.equal(head,authority.futureRepoRevision,'Candidate approved HEAD differs');
 const status=await git(repo,['status','--porcelain=v1','--untracked-files=all']);assert.equal(status.length,0,'Clean complete worktree required');
 const currentCmakeBytes=(await regularBytes(repo,'CMakeLists.txt')).bytes;
 // Validate reference/overlay authorization before reading any selected source.
 verifyApprovedCandidateCheckout({authorityBytes,expectedAuthoritySha256,coreReferenceBytes,compileReferenceBytes,sourceReferenceBytes,checkout:{head,status:'',files:reference.files,ignoredPaths:[]},baselineCmakeBytes,currentCmakeBytes});
 const approved=new Set(reference.files.map(x=>x.path));const tree=await git(repo,['ls-tree','-r','-z',head]);const files=[];
 for(const row of tree.toString('utf8').split('\0')){if(!row)continue;const tab=row.indexOf('\t');assert.ok(tab>0,'Git tree record shape');const [mode,type,oid]=row.slice(0,tab).split(' '),name=row.slice(tab+1);if(!scoped(name))continue;assert.ok(approved.has(name),'Unapproved scoped source path before read');assert.ok(type==='blob'&&['100644','100755'].includes(mode),'Regular committed source required');
  const {bytes,mode:actual}=await regularBytes(repo,name);assert.equal(Boolean(actual&0o111n),mode==='100755','Source executable mode differs');assert.equal(createHash('sha1').update('blob '+bytes.length+'\0').update(bytes).digest('hex'),oid,'Git object bytes differ');files.push({path:name,mode,gitBlobSha1:oid,bytes:bytes.length,sha256:createHash('sha256').update(bytes).digest('hex')});
 }
 const ignored=(await git(repo,['ls-files','--others','--ignored','--exclude-standard','-z','--','src','include','cmake','third_party','network','tests/support','tests/capella'])).toString().split('\0').filter(Boolean);
 assert.equal((await git(repo,['rev-parse','HEAD'])).toString().trim(),head,'HEAD changed during source audit');assert.equal((await git(repo,['status','--porcelain=v1','--untracked-files=all'])).length,0,'Worktree changed during source audit');
 const checkout={head,status:'',files,ignoredPaths:ignored};
 const verified=verifyApprovedCandidateCheckout({authorityBytes,expectedAuthoritySha256,coreReferenceBytes,compileReferenceBytes,sourceReferenceBytes,checkout,baselineCmakeBytes,currentCmakeBytes});return {checkout,currentCmakeBytes,verified};
}
