// ROOT-only new mode/arity refusals; no real control, seed, exporter or Docker.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,lstat,realpath,open,rm} from 'node:fs/promises';
import {constants} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
const preparer=resolve(fileURLToPath(new URL('./prepare-local-fixture-root.mjs',import.meta.url)));
const preparerSha='df48173b0f146c940050cb1c8742f057eafb4101534b30924ddd9f4022830302';
const fields=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const row=s=>fields.map(k=>String(s[k]));
async function source(){const f=await open(preparer,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);try{const a=await f.stat({bigint:true});assert(a.isFile()&&a.nlink===1n&&a.size>0n&&a.size<131072n);const b=await f.readFile();assert.equal(BigInt(b.length),a.size);assert.deepEqual(row(await f.stat({bigint:true})),row(a));assert.deepEqual(row(await lstat(preparer,{bigint:true})),row(a));assert.equal(createHash('sha256').update(b).digest('hex'),preparerSha);return row(a);}finally{await f.close();}}
const absent=async p=>{try{await lstat(p);}catch(e){if(e.code==='ENOENT')return;throw e;}assert.fail('Forbidden synthetic input/output appeared');};
async function fixture(fn){const before=await source(),parent=await realpath(await mkdtemp(join(tmpdir(),'nano-export-mode-refusal-'))),input=join(parent,'missing-control.json'),workspace=join(parent,'missing-workspace');try{await absent(input);await absent(workspace);await fn({input,workspace});await absent(input);await absent(workspace);assert.deepEqual(await source(),before);}finally{await rm(parent,{recursive:true,force:true});}}
const env=Object.fromEntries(Object.entries(process.env).filter(([k])=>!(/^(NODE_|DYLD_|LD_|NANO_TEST_)/.test(k))));
test('unknown export preparation API mode refuses before reading a missing input or creating output',{timeout:6000},()=>fixture(async({input,workspace})=>{
 const program='import {pathToFileURL} from "node:url";const m=await import(pathToFileURL(process.argv[2]).href);try{await m.prepareLocalFixture(process.argv[3],"0".repeat(64),process.argv[4],"unknown-mode");throw Error("missing enum refusal");}catch(e){if(e.code!=="NANO_FIXTURE_PREP_MODE")throw e;process.stdout.write(JSON.stringify({modeRefused:true}));}';
 // argv[1] is a sentinel, so importing the preparer cannot trigger its CLI.
 const r=spawnSync(process.execPath,['--max-old-space-size=32','--input-type=module','-e',program,'mode-refusal-sentinel',preparer,input,workspace],{env,encoding:'utf8',timeout:4000,maxBuffer:65536});assert.ifError(r.error);assert.equal(r.signal,null);assert.equal(r.status,0);assert.equal(r.stderr,'');assert.deepEqual(JSON.parse(r.stdout),{modeRefused:true});
}));
test('existing strict CLI refuses a missing workspace argument before preparation',{timeout:6000},()=>fixture(async({input})=>{
 const r=spawnSync(process.execPath,['--max-old-space-size=32',preparer,input,'0'.repeat(64)],{env,encoding:'utf8',timeout:4000,maxBuffer:65536});assert.ifError(r.error);assert.equal(r.signal,null);assert.equal(r.status,1);assert.equal(r.stderr,'');assert.deepEqual(JSON.parse(r.stdout),{format:'nano-local-supervisor-fixture-preparation-result-v1',passed:false,refusedStage:'setup',detailsExported:false,actualNanoStarts:0,productionQualified:false});
}));
