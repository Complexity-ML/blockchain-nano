import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,readFile,writeFile,realpath,rm,access,symlink} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {preview} from './preview.mjs';

const exec=promisify(execFile),cli=fileURLToPath(new URL('./verify-review.mjs',import.meta.url));
const sha=b=>createHash('sha256').update(b).digest('hex');
const encode=v=>JSON.stringify(v,(_,x)=>typeof x==='bigint'?x.toString():x);
const configuration=JSON.parse(await readFile(new URL('./example.synthetic.json',import.meta.url)));
const reviewBytes=encode(await preview(configuration));
async function fixture(t){
 const root=await realpath(await mkdtemp(join(tmpdir(),'nano-offline-review-cli-')));
 t.after(()=>rm(root,{recursive:true,force:true}));
 const config=join(root,'approved.json'),review=join(root,'review.json'),output=join(root,'verified.json');
 const configBytes=encode(configuration);
 await writeFile(config,configBytes,{flag:'wx',mode:0o600});await writeFile(review,reviewBytes,{flag:'wx',mode:0o600});
 const args=[cli,config,sha(configBytes),review,sha(reviewBytes),output];
 return {root,config,review,output,args};
}
async function run(args){
 // The qualification parent supplies the deny-fetch preload; propagate it to
 // each owned CLI subprocess without inheriting wallet/provider environment.
 const imports=process.execArgv.flatMap((x,i,a)=>x==='--import'?[x,a[i+1]]:[]);
 try{return {...await exec(process.execPath,[...imports,...args],{env:{PATH:'/usr/bin:/bin',HOME:process.env.HOME,LANG:'C',LC_ALL:'C',NODE_OPTIONS:'',NODE_PATH:''},timeout:30000,maxBuffer:65536}),code:0};}
 catch(e){return {stdout:e.stdout??'',stderr:e.stderr??'',code:e.code};}
}
test('offline CLI binds both externally pinned files and emits no readiness claim',async t=>{
 const f=await fixture(t),r=await run(f.args);assert.equal(r.code,0,r.stderr);
 const result=JSON.parse(await readFile(f.output,'utf8'));
 assert.equal(result.reviewIntegrityVerified,true);assert.equal(result.approvedConfigurationSha256,f.args[2]);assert.equal(result.reviewSha256,f.args[4]);
 for(const flag of ['simulationVerified','nativeAssetRelationshipVerified','productionReady','signed','broadcast'])assert.equal(result[flag],false);
});
test('CLI refuses a wrong external pin without creating output',async t=>{
 const f=await fixture(t);f.args[4]='0'.repeat(64);const r=await run(f.args);
 assert.equal(r.code,1);assert.match(r.stderr,/input SHA or identity changed/);await assert.rejects(access(f.output));
});
test('CLI refuses a symlink input even when target content has the approved hash',async t=>{
 const f=await fixture(t),alias=join(f.root,'review-link.json');await symlink(f.review,alias);f.args[3]=alias;
 const r=await run(f.args);assert.equal(r.code,1);assert.match(r.stderr,/physical input path/);await assert.rejects(access(f.output));
});
test('CLI preserves an existing output file instead of overwriting it',async t=>{
 const f=await fixture(t);await writeFile(f.output,'retained-review\n',{flag:'wx',mode:0o600});const r=await run(f.args);
 assert.equal(r.code,1);assert.equal(await readFile(f.output,'utf8'),'retained-review\n');
});
