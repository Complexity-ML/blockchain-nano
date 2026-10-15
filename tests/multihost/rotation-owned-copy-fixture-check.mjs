#!/usr/bin/env node
// Real, tiny native filesystem fixtures only. Never opens RocksDB or a campaign.
import assert from 'node:assert/strict';
import {open,mkdtemp,mkdir,chmod,writeFile,readFile,lstat,readdir,readlink,realpath,link,symlink,rm} from 'node:fs/promises';
import {constants} from 'node:fs';
import {createHash} from 'node:crypto';
import {join,resolve} from 'node:path';
import {tmpdir} from 'node:os';
import {pathToFileURL} from 'node:url';

const expectedModule={sha256:'845c95c14d46d0c59089f39f053b59b6063618cc52bfe81b3cd9824678bcbd67',bytes:20692};
const fields=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const material=s=>fields.map(k=>String(s[k]));
const id=s=>String(s.dev)+':'+String(s.ino);
const hash=b=>createHash('sha256').update(b).digest('hex');
const started=performance.now(),deadline=started+28000;
const liveFixtures=new Set();
let stage='module-admission',rows=0,createCalls=0,verifyCalls=0,physicalRefusals=0,summaryRefusals=0,fixtureCount=0;
function bounded(){assert.ok(performance.now()<deadline,'Synthetic filesystem test deadline');}
function checkpoint(){bounded();}
function emit(scenario,extra={}){bounded();++rows;console.log(JSON.stringify({scenario,result:'PASS',databaseAccessed:false,newSignaturesCreated:0,childProcessesStarted:0,networkUsed:false,runtimeStarted:false,productionQualified:false,...extra}));}
async function pinnedModule(path){
 const p=resolve(path);assert.equal(p,path,'Absolute module path');assert.equal(await realpath(p),p,'Canonical module path');
 const f=await open(p,constants.O_RDONLY|constants.O_NOFOLLOW|constants.O_NONBLOCK);
 try{const before=await f.stat({bigint:true});assert.ok(before.isFile()&&before.nlink===1n&&before.size===BigInt(expectedModule.bytes));const raw=await f.readFile();assert.equal(hash(raw),expectedModule.sha256);assert.deepEqual(material(before),material(await f.stat({bigint:true})));assert.deepEqual(material(before),material(await lstat(p,{bigint:true})));return material(before);}finally{await f.close();}
}
// Independent observation oracle: record material and bytes; never follows a
// symlink, accepts no authority from the helper's private proof or summary.
async function inventory(root){
 const out={};
 async function walk(p,rel){
  bounded();const s=await lstat(p,{bigint:true});assert.ok(Object.keys(out).length<32,'Tiny fixture entry bound');
  if(s.isDirectory()){out[rel]={kind:'directory',material:material(s)};for(const n of (await readdir(p)).sort())await walk(join(p,n),rel+'/'+n);}
  else if(s.isFile()){assert.ok(s.size<=4096n,'Tiny fixture file bound');out[rel]={kind:'file',material:material(s),sha256:hash(await readFile(p))};}
  else if(s.isSymbolicLink())out[rel]={kind:'symlink',material:material(s),target:await readlink(p)};
  else out[rel]={kind:'special',material:material(s)};
 }
 await walk(root,'');return out;
}
async function sourceInventory(f){return {primary:await inventory(join(f.dataRoot,'chain')),witness:await inventory(join(f.witnessRoot,'chain-witness'))};}
const dataFiles={'CURRENT':Buffer.from('MANIFEST-PUBLIC-0001\n'),'000001.sst':Buffer.from(Array.from({length:128},(_,i)=>i)),'nested/row':Buffer.from('opaque-primary-row\n')};
const witnessFiles={'CURRENT':Buffer.from('MANIFEST-PUBLIC-0002\n'),'000001.log':Buffer.from('opaque-witness-intent\n'),'nested/row':Buffer.from([0x81,0x00,0x7f])};
const expectedPayloadBytes=[...Object.values(dataFiles),...Object.values(witnessFiles)].reduce((n,b)=>n+b.length,0);
async function fixture(fn){
 bounded();const root=await realpath(await mkdtemp(join(tmpdir(),'nano-rotation-copy-public-')));liveFixtures.add(root);++fixtureCount;await chmod(root,0o700);
 const f={root,dataRoot:join(root,'data-owner'),witnessRoot:join(root,'witness-owner'),destination:join(root,'new-owned-copy')};
 try{
  for(const [owner,leaf,files] of [[f.dataRoot,'chain',dataFiles],[f.witnessRoot,'chain-witness',witnessFiles]]){
   await mkdir(join(owner,leaf,'nested'),{recursive:true,mode:0o700});
   for(const p of [owner,join(owner,leaf),join(owner,leaf,'nested')])await chmod(p,0o700);
   if(leaf==='chain')await mkdir(join(owner,leaf,'empty'),{mode:0o700});
   for(const [name,bytes] of Object.entries(files)){const p=join(owner,leaf,name);await writeFile(p,bytes,{flag:'wx',mode:0o600});await chmod(p,0o600);}
  }
  await fn(f);
 }finally{await rm(root,{recursive:true,force:true});liveFixtures.delete(root);assert.equal(await lstat(root).catch(e=>e.code==='ENOENT'?null:Promise.reject(e)),null,'Only the newly owned fixture was removed');}
}
let api;
async function create(f){const p=api.createNativeRotationCopies({...f,checkpoint});++createCalls;return p;}
async function verify(h){const p=h.verify();++verifyCalls;return p;}
async function refused(call){await assert.rejects(call,/^Error: Stopped rotation physical copy refused$/);++physicalRefusals;bounded();}
async function absent(p){await assert.rejects(lstat(p),{code:'ENOENT'});}
async function privateIndependent(f,h){
 const seen=new Set();
 for(const [owner,leaf] of [[f.dataRoot,'chain'],[f.witnessRoot,'chain-witness'],[h.dataRoot,'chain'],[h.witnessRoot,'chain-witness']]){
  const root=join(owner,leaf);
  for(const [rel,z] of Object.entries(await inventory(root))){const p=rel?root+rel:root,s=await lstat(p,{bigint:true});assert.ok(!seen.has(id(s)),'All source/copy entries have independent identities');seen.add(id(s));assert.equal(s.uid,BigInt(process.getuid()));assert.equal(Number(s.mode)&0o7777,z.kind==='directory'?0o700:0o600);if(z.kind==='file')assert.equal(s.nlink,1n);}
 }
 for(const [leaf,files,root] of [['chain',dataFiles,h.dataRoot],['chain-witness',witnessFiles,h.witnessRoot]])for(const [name,bytes] of Object.entries(files))assert.deepEqual(await readFile(join(root,leaf,name)),bytes,'Exact independently known fixture bytes copied');
 const proof=await lstat(join(f.destination,'physical-copy.private.json'),{bigint:true});assert.ok(proof.isFile()&&proof.nlink===1n&&proof.size<65536n);assert.equal(Number(proof.mode)&0o7777,0o600);
}
async function main(){
 assert.equal(process.argv.length,3,'Usage: node rotation-owned-copy.test.mjs ABSOLUTE_FROZEN_MODULE');const modulePath=process.argv[2],moduleMaterial=await pinnedModule(modulePath);api=await import(pathToFileURL(modulePath).href);assert.equal(typeof api.createNativeRotationCopies,'function');assert.equal(typeof api.validateRotationCopySummary,'function');let realSummary;
 stage='valid-private-independent-two-role-copy';
 await fixture(async f=>{
  const before=await sourceInventory(f),h=await create(f);assert.deepEqual(await sourceInventory(f),before,'Original nine fields and bytes unchanged after copy');await privateIndependent(f,h);
  assert.equal(h.summary.sourceFiles,6);assert.equal(h.summary.sourceDirectories,5);assert.equal(h.summary.payloadBytes,expectedPayloadBytes);assert.equal(h.summary.databaseRoles,2);assert.equal(h.summary.copiesUnchangedAfterAudit,false);assert.equal(api.validateRotationCopySummary(h.summary,false),h.summary);
  const copied={data:await inventory(join(h.dataRoot,'chain')),witness:await inventory(join(h.witnessRoot,'chain-witness'))};const after=await verify(h);assert.equal(after.copiesUnchangedAfterAudit,true);assert.equal(after.copyProofSha256,h.summary.copyProofSha256);assert.equal(api.validateRotationCopySummary(after,true),after);assert.deepEqual(await sourceInventory(f),before);assert.deepEqual({data:await inventory(join(h.dataRoot,'chain')),witness:await inventory(join(h.witnessRoot,'chain-witness'))},copied);realSummary=structuredClone(h.summary);
 });emit(stage,{databaseRoles:2,sourceFiles:6,sourceDirectories:5,payloadBytes:expectedPayloadBytes});
 stage='source-mutation-after-admission-refused';
 await fixture(async f=>{const h=await create(f),copied={data:await inventory(join(h.dataRoot,'chain')),witness:await inventory(join(h.witnessRoot,'chain-witness'))};await writeFile(join(f.dataRoot,'chain','000001.sst'),Buffer.from('intentional-source-change'));const changed=await sourceInventory(f);await refused(()=>verify(h));assert.deepEqual(await sourceInventory(f),changed,'Refusal did not rewrite the mutated source');assert.deepEqual({data:await inventory(join(h.dataRoot,'chain')),witness:await inventory(join(h.witnessRoot,'chain-witness'))},copied,'Source refusal left copies unchanged');await writeFile(join(f.dataRoot,'chain','000001.sst'),dataFiles['000001.sst']);assert.deepEqual(await readFile(join(f.dataRoot,'chain','000001.sst')),dataFiles['000001.sst']);await refused(()=>verify(h));assert.deepEqual({data:await inventory(join(h.dataRoot,'chain')),witness:await inventory(join(h.witnessRoot,'chain-witness'))},copied,'Restored bytes cannot refresh the old source material authority');});emit(stage,{restoredSourceBytesStillRefused:true});
 stage='copy-mutation-after-admission-refused';
 await fixture(async f=>{const h=await create(f),before=await sourceInventory(f);await writeFile(join(h.witnessRoot,'chain-witness','000001.log'),Buffer.from('intentional-copy-change'));const changed={data:await inventory(join(h.dataRoot,'chain')),witness:await inventory(join(h.witnessRoot,'chain-witness'))};await refused(()=>verify(h));assert.deepEqual(await sourceInventory(f),before,'Copy refusal left both sources unchanged');assert.deepEqual({data:await inventory(join(h.dataRoot,'chain')),witness:await inventory(join(h.witnessRoot,'chain-witness'))},changed,'Refusal did not repair or rewrite corrupt copy');await writeFile(join(h.witnessRoot,'chain-witness','000001.log'),witnessFiles['000001.log']);assert.deepEqual(await readFile(join(h.witnessRoot,'chain-witness','000001.log')),witnessFiles['000001.log']);await refused(()=>verify(h));assert.deepEqual(await sourceInventory(f),before,'Restored copy bytes cannot refresh its old material authority');});emit(stage,{restoredCopyBytesStillRefused:true});
 stage='source-symlink-refused-before-copy';
 await fixture(async f=>{const canary=join(f.root,'external-public-canary');await writeFile(canary,'opaque-external-canary',{flag:'wx',mode:0o600});await symlink(canary,join(f.dataRoot,'chain','nested','alias'));const before={source:await sourceInventory(f),external:await inventory(canary)};await refused(()=>create(f));await absent(f.destination);assert.deepEqual({source:await sourceInventory(f),external:await inventory(canary)},before);});emit(stage);
 stage='source-external-hardlink-refused-before-copy';
 await fixture(async f=>{const source=join(f.dataRoot,'chain','CURRENT'),external=join(f.root,'external-public-alias');await link(source,external);assert.equal((await lstat(source,{bigint:true})).nlink,2n);const before={source:await sourceInventory(f),external:await inventory(external)};await refused(()=>create(f));await absent(f.destination);assert.deepEqual({source:await sourceInventory(f),external:await inventory(external)},before);});emit(stage);
 stage='source-internal-hardlink-refused-before-copy';
 await fixture(async f=>{await link(join(f.witnessRoot,'chain-witness','CURRENT'),join(f.witnessRoot,'chain-witness','nested','internal-alias'));const before=await sourceInventory(f);await refused(()=>create(f));await absent(f.destination);assert.deepEqual(await sourceInventory(f),before);});emit(stage);
 stage='malformed-real-copy-summary-refused';
 const mutations=[r=>{delete r.format;},r=>{r.format='foreign';},r=>{r.passed=false;},r=>{r.databaseRoles=1;},r=>{r.sourceFiles=-1;},r=>{r.sourceDirectories=0.5;},r=>{r.payloadBytes=1073741825;},r=>{r.copiesUnchangedAfterAudit=true;},r=>{r.sourceNineFieldAndShaUnchanged=false;},r=>{r.independentInodes=false;},r=>{r.copyProofSha256='g'.repeat(64);},r=>{r.stabilityProbeCount=2;},r=>{r.rawInventoryExported=true;},r=>{r.productionQualified=true;}];
 for(const mutate of mutations){const r=structuredClone(realSummary);mutate(r);const before=structuredClone(r);assert.throws(()=>api.validateRotationCopySummary(r,false),/^Error: Stopped rotation physical copy refused$/);++summaryRefusals;assert.deepEqual(r,before,'Validation does not rewrite rejected summaries');bounded();}emit(stage,{expectedRefusals:mutations.length});
 stage='extra-real-copy-summary-field-refused';
 const extra={...realSummary,unexpectedField:true},beforeExtra=structuredClone(extra);assert.throws(()=>api.validateRotationCopySummary(extra,false),/^Error: Stopped rotation physical copy refused$/);++summaryRefusals;assert.deepEqual(extra,beforeExtra);emit(stage,{expectedRefusals:1});
 stage='final-fences';assert.equal(liveFixtures.size,0);assert.deepEqual(await pinnedModule(modulePath),moduleMaterial,'Frozen helper nine fields remain exact');assert.equal(rows,8);assert.equal(fixtureCount,6);assert.equal(createCalls,6);assert.equal(verifyCalls,5);assert.equal(physicalRefusals,7);assert.equal(summaryRefusals,15);bounded();
 console.log(JSON.stringify({scenario:'rotation-owned-copy-native-summary',result:'PASS',scenarioRows:rows,fixtures:fixtureCount,helperCreateCalls:createCalls,helperVerifyCalls:verifyCalls,physicalRefusals,summaryRefusals,sourceFilesPerValidFixture:6,sourceDirectoriesPerValidFixture:5,payloadBytesPerValidFixture:expectedPayloadBytes,ownedFixtureDirectoriesAbsent:true,moduleNineFieldsAndShaUnchanged:true,elapsedMs:Math.ceil(performance.now()-started),databaseAccessed:false,newSignaturesCreated:0,childProcessesStarted:0,networkUsed:false,runtimeStarted:false,productionQualified:false}));
}
try{await main();}catch(error){for(const root of liveFixtures)await rm(root,{recursive:true,force:true}).catch(()=>{});console.log(JSON.stringify({diagnostic:'ROTATION_OWNED_COPY_NATIVE_TEST_FAILED',stage,completedScenarioRows:rows,helperCreateCalls:createCalls,helperVerifyCalls:verifyCalls,physicalRefusals,summaryRefusals,errorType:error?.constructor?.name??'UNKNOWN',databaseAccessed:false,newSignaturesCreated:0,childProcessesStarted:0,runtimeStarted:false,productionQualified:false}));process.exitCode=1;}
