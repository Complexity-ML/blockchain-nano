#!/usr/bin/env node
// Pure regression over the two retained run01 inventories, not a DB audit.
import {readFile,lstat} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';

const started=performance.now(),maximumBytes=16*1024*1024;let peakRssKiB=0;
const sha=b=>createHash('sha256').update(b).digest('hex');
function need(ok,reason){if(!ok)throw Error(reason);}
function checkpoint(){peakRssKiB=Math.max(peakRssKiB,process.resourceUsage().maxRSS);need(performance.now()-started<=180000,'Consumer deadline exceeded');need(Number.isSafeInteger(peakRssKiB)&&peakRssKiB<=131072,'Consumer RSS limit exceeded');}
const pins={
 before:{sha256:'442aa7efcb4adaea6209dc1fb05991b9a2d9e2ca1e7a1f623d582970ac6b1188',bytes:513701},
 after:{sha256:'483acd00e6b75239ff563dc3035bd0bb484ba3698402f3b550b35a1af8ec81e8',bytes:372302}
};
const args=process.argv.slice(2);
need(args.length===5,'Usage: fresh-bootstrap-transport-fixture-check.mjs BEFORE AFTER PREPARE_MODULE MODULE_SHA256 MODULE_BYTES');
const [beforePath,afterPath,modulePath,moduleSha,moduleBytesText]=args;
need(/^[0-9a-f]{64}$/.test(moduleSha)&&/^[1-9][0-9]*$/.test(moduleBytesText),'External module pin required');
const moduleBytes=Number(moduleBytesText);need(Number.isSafeInteger(moduleBytes)&&moduleBytes<=maximumBytes,'Module byte bound');
const retained=[];
async function input(path,pin){
 const p=resolve(path),s=await lstat(p);need(s.isFile()&&!s.isSymbolicLink()&&s.size===pin.bytes&&s.size<=maximumBytes,'Pinned regular input required');
 const b=await readFile(p);need(b.length===pin.bytes&&sha(b)===pin.sha256,'Input bytes differ from external pin');retained.push({p,pin});return b;
}
const before=JSON.parse(await input(beforePath,pins.before)),after=JSON.parse(await input(afterPath,pins.after));
await input(modulePath,{sha256:moduleSha,bytes:moduleBytes});
const {verifyFreshBootstrapTransportInventory:verify}=await import(pathToFileURL(resolve(modulePath)).href);
need(typeof verify==='function','Reviewed pure inventory API required');
const originalBefore=JSON.stringify(before),originalAfter=JSON.stringify(after),clone=v=>structuredClone(v);
const fileIndex=before.entries.findIndex(z=>z.kind==='file'),dirIndex=before.entries.findIndex(z=>z.kind==='directory');
const secondFileIndex=before.entries.findIndex((z,i)=>z.kind==='file'&&i!==fileIndex);
need(fileIndex>=0&&secondFileIndex>=0&&dirIndex>=0&&before.entries.length===1432,'Actual nontrivial run01 inventory required');
let negatives=0,positives=0,groups=0;
const results=[];
function accept(a,b){
 checkpoint();const as=JSON.stringify(a),bs=JSON.stringify(b),v=verify(a,b);
 need(JSON.stringify(a)===as&&JSON.stringify(b)===bs,'Pure API mutated caller inventory');
 const fields=['format','passed','sourceEntriesVerified','filesVerified','directoriesVerified','directoryTimeChanges','rootTimeChanges','allRecordedFileMaterialAndSha256Unchanged','namespaceAndRecordedIdentitiesUnchanged','hardAliasIdentitiesRefused','runtimeStarted','productionQualified'];
 need(v&&Object.keys(v).sort().join('\0')===fields.sort().join('\0')&&v.format==='nano-fresh-bootstrap-transport-inventory-check-v1','Exact transport proof shape required');
 need(v.passed===true&&v.allRecordedFileMaterialAndSha256Unchanged===true&&v.namespaceAndRecordedIdentitiesUnchanged===true&&v.hardAliasIdentitiesRefused===true&&v.runtimeStarted===false&&v.productionQualified===false,'Transport proof flags required');
 for(const k of ['sourceEntriesVerified','filesVerified','directoriesVerified','directoryTimeChanges','rootTimeChanges'])need(Number.isSafeInteger(v[k])&&v[k]>=0,'Nonnegative integer proof counter required');
 positives++;return v;
}
function refuse(label,mutate){
 checkpoint();const a=clone(before),b=clone(before);mutate(a,b);const as=JSON.stringify(a),bs=JSON.stringify(b);let threw=false;
 try{verify(a,b);}catch{threw=true;}
 need(threw,'Mutation accepted: '+label);need(JSON.stringify(a)===as&&JSON.stringify(b)===bs,'Refusal mutated input: '+label);negatives++;
}
function row(scenario,extra={}){groups++;const v={scenario,result:'PASS',...extra,newSignaturesCreated:0,privateKeysRead:false,databaseOpened:false,runtimeStarted:false,productionQualified:false};results.push(v);console.log(JSON.stringify(v));}

// Independent field enumeration: no output from the function under test is
// used to decide which run01 differences are permissible.
const fileFields=['path','kind','device','inode','uid','gid','mode','bytes','mtime','ctime','sha256'];
const stableDirFields=['path','kind','device','inode','uid','gid','mode','bytes','sha256'];
const rootFields=['format','root','rootDevice','rootInode','rootUid','rootGid','rootMode'];
need(before.entries.length===after.entries.length,'Actual namespace count changed');
const afterByPath=new Map(after.entries.map(z=>[z.path,z]));need(afterByPath.size===after.entries.length,'Actual duplicate path');
let files=0,dirs=0,changedDirs=0,rootChanges=0;
for(const old of before.entries){
 const now=afterByPath.get(old.path);need(now&&now.kind===old.kind,'Actual namespace changed');
 for(const k of old.kind==='file'?fileFields:stableDirFields)need(old[k]===now[k],'Unexpected actual material change: '+k);
 if(old.kind==='file')files++;else{dirs++;if(old.mtime!==now.mtime||old.ctime!==now.ctime)changedDirs++;}
}
for(const k of rootFields)need(before[k]===after[k],'Actual root identity changed');
for(const k of ['rootMtime','rootCtime'])if(before[k]!==after[k])rootChanges++;
need(files===1302&&dirs===130&&changedDirs===125&&rootChanges===2,'Actual run01 diagnostic counts differ');
const real=accept(before,after);
need(real.filesVerified===files&&real.directoriesVerified===dirs&&real.sourceEntriesVerified===1432&&real.directoryTimeChanges===changedDirs&&real.rootTimeChanges===rootChanges,'Actual API counters differ');
row('fresh-transport-real-run01-directory-times',{filesVerified:files,directoriesVerified:dirs,directoryTimeChanges:changedDirs,rootTimeChanges:rootChanges});

const same=accept(before,clone(before));need(same.directoryTimeChanges===0&&same.rootTimeChanges===0,'Identity comparison reports changes');
row('fresh-transport-identity');

for(const k of ['mtime','ctime']){const b=clone(before);b.entries[dirIndex][k]=b.entries[dirIndex][k]==='1:0'?'2:0':'1:0';const v=accept(before,b);need(v.directoryTimeChanges===1&&v.rootTimeChanges===0,'Allowed directory-time counters');}
for(const k of ['rootMtime','rootCtime']){const b=clone(before);b[k]=b[k]==='1:0'?'2:0':'1:0';const v=accept(before,b);need(v.directoryTimeChanges===0&&v.rootTimeChanges===1,'Allowed root-time counters');}
row('fresh-transport-only-directory-times-allowed',{additionalPositiveComparisons:4});

function changeValue(z,k){
 if(k==='path')return z.path+'-substituted';if(k==='kind')return z.kind==='file'?'directory':'file';
 if(k==='sha256')return z.sha256===null?'0'.repeat(64):z.sha256[0]==='0'?'1'+z.sha256.slice(1):'0'+z.sha256.slice(1);
 if(['device','inode','rootDevice','rootInode'].includes(k))return String(BigInt(z[k])+1n);
 if(['mtime','ctime','rootMtime','rootCtime'].includes(k))return z[k]==='1:0'?'2:0':'1:0';
 if(k==='root')return z.root+'-substituted';if(k==='format')return z.format+'-other';return z[k]+1;
}
for(const k of fileFields)refuse('file-'+k,(_a,b)=>{b.entries[fileIndex][k]=changeValue(b.entries[fileIndex],k);});
row('fresh-transport-file-material-strict',{negativeComparisons:fileFields.length});

for(const k of stableDirFields)refuse('directory-'+k,(_a,b)=>{b.entries[dirIndex][k]=changeValue(b.entries[dirIndex],k);});
for(const k of rootFields)refuse('root-'+k,(_a,b)=>{b[k]=changeValue(b,k);});
row('fresh-transport-directory-root-identity-strict',{negativeComparisons:stableDirFields.length+rootFields.length});

const malformed=[
 ['missing-entry',(_a,b)=>b.entries.pop()],
 ['extra-entry',(_a,b)=>b.entries.push({...b.entries[fileIndex],path:'unexpected-new-file'})],
 ['duplicate-path',(_a,b)=>b.entries.push(clone(b.entries[fileIndex]))],
 ['extra-root-field',(a,b)=>{a.unrecorded=true;b.unrecorded=true;}],
 ['missing-root-field',(a,b)=>{delete a.rootUid;delete b.rootUid;}],
 ['extra-entry-field',(a,b)=>{a.entries[fileIndex].unrecorded=true;b.entries[fileIndex].unrecorded=true;}],
 ['missing-entry-field',(a,b)=>{delete a.entries[fileIndex].uid;delete b.entries[fileIndex].uid;}],
 ['path-traversal',(a,b)=>{a.entries[fileIndex].path='../escape';b.entries[fileIndex].path='../escape';}],
 ['absolute-entry-path',(a,b)=>{a.entries[fileIndex].path='/escape';b.entries[fileIndex].path='/escape';}],
 ['file-alias',(a,b)=>{for(const v of [a,b]){v.entries[secondFileIndex].device=v.entries[fileIndex].device;v.entries[secondFileIndex].inode=v.entries[fileIndex].inode;}}],
 ['directory-alias',(a,b)=>{for(const v of [a,b]){v.entries[dirIndex].device=v.entries[fileIndex].device;v.entries[dirIndex].inode=v.entries[fileIndex].inode;}}],
 ['root-alias',(a,b)=>{for(const v of [a,b]){v.entries[dirIndex].device=v.rootDevice;v.entries[dirIndex].inode=v.rootInode;}}],
 ['missing-parent',(a,b)=>{const parent=before.entries.find(z=>z.kind==='directory'&&before.entries.some(child=>child.path.startsWith(z.path+'/')));need(parent,'Actual populated parent required');for(const v of [a,b])v.entries=v.entries.filter(z=>z.path!==parent.path);}],
 ['nondirectory-parent',(a,b)=>{const parent=before.entries.find(z=>z.kind==='directory'&&before.entries.some(child=>child.path.startsWith(z.path+'/')));for(const v of [a,b]){const z=v.entries.find(z=>z.path===parent.path);z.kind='file';z.sha256='0'.repeat(64);}}],
 ['file-sha-null',(a,b)=>{a.entries[fileIndex].sha256=null;b.entries[fileIndex].sha256=null;}],
 ['directory-sha-nonnull',(a,b)=>{a.entries[dirIndex].sha256='0'.repeat(64);b.entries[dirIndex].sha256='0'.repeat(64);}],
 ['negative-owner',(a,b)=>{a.entries[fileIndex].uid=-1;b.entries[fileIndex].uid=-1;}],
 ['unsafe-byte-count',(a,b)=>{a.entries[fileIndex].bytes=Number.MAX_SAFE_INTEGER+1;b.entries[fileIndex].bytes=Number.MAX_SAFE_INTEGER+1;}],
 ['invalid-directory-time',(a,b)=>{a.entries[dirIndex].mtime='not-a-time';b.entries[dirIndex].mtime='not-a-time';}],
 ['invalid-root-time',(a,b)=>{a.rootCtime='not-a-time';b.rootCtime='not-a-time';}]
];
for(const [label,mutate] of malformed)refuse(label,mutate);
row('fresh-transport-namespace-schema-and-aliases',{negativeComparisons:malformed.length});

need(groups===6&&positives===6&&negatives===47,'Frozen consumer accounting differs');
need(JSON.stringify(before)===originalBefore&&JSON.stringify(after)===originalAfter,'Actual parsed inputs changed');
for(const {p,pin} of retained){const b=await readFile(p);need(b.length===pin.bytes&&sha(b)===pin.sha256,'Input/module bytes changed during pure checks');}
checkpoint();
console.log(JSON.stringify({scenario:'fresh-transport-inventory-summary',result:'PASS',scenarioGroups:groups,scenarioRows:results.length+1,positiveComparisons:positives,negativeComparisons:negatives,sourceEntriesVerified:1432,filesVerified:1302,directoriesVerified:130,inputBytesUnchanged:true,elapsedMs:Math.floor(performance.now()-started),maxRssKiB:peakRssKiB,newSignaturesCreated:0,privateKeysRead:false,databaseOpened:false,runtimeStarted:false,productionQualified:false}));
