import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {performance} from 'node:perf_hooks';
import {setTimeout as delay} from 'node:timers/promises';

const FORMAT='nano-bridge-source-serial-linux-unit-result-v1';
const INPUT='nano-bridge-source-serial-linux-unit-input-v1';
const FILES=['tools/bridge-source/finality.mjs','tools/bridge-source/finality.test.mjs'];
const GROUP='synchronous RPC cache collisions cannot erase already fulfilled divergences';
const CASES=['canonical-header','finalized-header','second-receipt'];
const COUNTS=['tests','pass','fail','cancelled','skipped','todo'];
const NINE=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const C=fs.constants,MAX_INPUT=1048576,MAX_FILE=131072,MAX_SOURCE=524288,MAX_BUFFER=524288;
const START=performance.now(),END=START+60000,uid=process.getuid?.(),gid=process.getgid?.();
const need=(v,m)=>{if(!v)throw new Error(m);};
const equal=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const sha=b=>createHash('sha256').update(b).digest('hex');
const nine=s=>NINE.map(k=>{need(typeof s[k]==='bigint','BigInt stat field unavailable');return String(s[k]);});
const deadline=()=>need(performance.now()<END,'Wrapper deadline exceeded');
const diagnostic=e=>({name:typeof e?.name==='string'?e.name:'Error',message:String(e?.message??'Wrapper refused').slice(0,400)});
function keys(v,wanted){
 need(v!==null&&typeof v==='object'&&!Array.isArray(v)&&equal(Object.keys(v).sort(),[...wanted].sort()),'Input object shape refused');
}
async function readInput(){
 const chunks=[];let bytes=0;
 const timer=setTimeout(()=>process.stdin.destroy(new Error('Input deadline exceeded')),5000);
 try{
  for await(const b of process.stdin){bytes+=b.length;need(bytes<=MAX_INPUT,'Input byte bound exceeded');chunks.push(b);}
  return JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(Buffer.concat(chunks)));
 }finally{clearTimeout(timer);}
}
function validateCounts(v){
 keys(v,COUNTS);
 for(const k of COUNTS)need(Number.isSafeInteger(v[k])&&v[k]>=0&&v[k]<=1024,'Expected TAP count refused');
 need(v.cancelled===0&&v.todo===0&&v.tests===v.pass+v.fail+v.skipped,'Expected TAP totals inconsistent');
 return Object.fromEntries(COUNTS.map(k=>[k,v[k]]));
}
function validate(v){
 keys(v,['format','phases','newTestPattern','expectations']);need(v.format===INPUT,'Input format refused');keys(v.phases,['baseline','candidate']);
 need(v.newTestPattern==='^'+GROUP+'$','Exact reviewed new test pattern required');
 keys(v.expectations,['baselineCounts','candidateCounts','baselineMissingExpectedRejections']);
 const expectations={
  baselineCounts:validateCounts(v.expectations.baselineCounts),
  candidateCounts:validateCounts(v.expectations.candidateCounts),
  baselineMissingExpectedRejections:v.expectations.baselineMissingExpectedRejections,
 };
 need(expectations.baselineMissingExpectedRejections===CASES.length,'Expected causal leaf count differs from reviewed source');
 const b=expectations.baselineCounts,c=expectations.candidateCounts;
 need(b.pass===0&&b.fail===expectations.baselineMissingExpectedRejections+1,'Baseline must fail only the causal leaves and their parent');
 need(c.tests>0&&c.pass===c.tests&&c.fail===0&&c.skipped===0,'Candidate must pass the full file without skips');
 let total=0;const phases={};
 for(const phase of ['baseline','candidate']){
  keys(v.phases[phase],['files']);const rows=v.phases[phase].files;
  need(Array.isArray(rows)&&rows.length===FILES.length,'Exactly two reviewed files required per phase');
  const seen=new Set();
  phases[phase]=rows.map(r=>{
   keys(r,['path','sha256','bytes','contentBase64']);
   need(FILES.includes(r.path)&&!seen.has(r.path),'Source path refused');seen.add(r.path);
   need(typeof r.sha256==='string'&&/^[0-9a-f]{64}$/.test(r.sha256),'Source SHA refused');
   need(Number.isSafeInteger(r.bytes)&&r.bytes>0&&r.bytes<=MAX_FILE,'Source size refused');
   need(typeof r.contentBase64==='string'&&r.contentBase64.length<=4*Math.ceil(MAX_FILE/3),'Source encoding bound exceeded');
   const body=Buffer.from(r.contentBase64,'base64');
   need(body.toString('base64')===r.contentBase64&&body.length===r.bytes&&sha(body)===r.sha256,'Known source bytes differ');
   total+=body.length;return {relative:r.path,sha256:r.sha256,bytes:r.bytes,body};
  }).sort((a,b)=>a.relative.localeCompare(b.relative));
 }
 need(total<=MAX_SOURCE,'Total source byte bound exceeded');
 const a=phases.baseline,bp=phases.candidate;
 need(a.find(r=>r.relative===FILES[1]).body.equals(bp.find(r=>r.relative===FILES[1]).body),'Both phases must run the identical candidate test');
 need(!a.find(r=>r.relative===FILES[0]).body.equals(bp.find(r=>r.relative===FILES[0]).body),'Observer baseline and candidate must differ');
 return {phases,newTestPattern:v.newTestPattern,expectations};
}
function fsyncDir(p){
 let fd;try{fd=fs.openSync(p,C.O_RDONLY|C.O_DIRECTORY|C.O_NOFOLLOW);fs.fsyncSync(fd);}finally{if(fd!==undefined)fs.closeSync(fd);}
}
function stage(root,name,files){
 const base=path.join(root,name);fs.mkdirSync(base,{mode:0o700});const dirs=new Set([root,base]);
 for(const r of files){
  const p=path.join(base,r.relative);fs.mkdirSync(path.dirname(p),{recursive:true,mode:0o700});
  for(let d=path.dirname(p);d!==root;d=path.dirname(d))dirs.add(d);
  let fd;try{fd=fs.openSync(p,C.O_WRONLY|C.O_CREAT|C.O_EXCL|C.O_NOFOLLOW,0o600);fs.writeFileSync(fd,r.body);fs.fchmodSync(fd,0o400);fs.fsyncSync(fd);}finally{if(fd!==undefined)fs.closeSync(fd);}
  r.path=p;delete r.body;
 }
 for(const d of [...dirs].sort((a,b)=>b.length-a.length))fsyncDir(d);
 return {base,files,directories:[...dirs].sort()};
}
function sampleFile(r){
 deadline();need(fs.realpathSync(r.path)===r.path,'Physical source path required');let fd;
 try{
  const before=fs.lstatSync(r.path,{bigint:true});
  need(before.isFile()&&before.nlink===1n&&before.uid===BigInt(uid)&&before.gid===BigInt(gid)&&(before.mode&0o7777n)===0o400n&&before.size===BigInt(r.bytes),'Source type/owner/mode/size differs');
  fd=fs.openSync(r.path,C.O_RDONLY|C.O_NOFOLLOW|C.O_NONBLOCK);
  need(equal(nine(before),nine(fs.fstatSync(fd,{bigint:true}))),'Source FD/path before differs');
  const h=createHash('sha256'),buffer=Buffer.alloc(32768);let bytes=0;
  for(;;){const n=fs.readSync(fd,buffer,0,buffer.length,null);if(n===0)break;bytes+=n;need(bytes<=r.bytes,'Source read exceeded known size');h.update(buffer.subarray(0,n));}
  need(bytes===r.bytes&&h.digest('hex')===r.sha256&&equal(nine(before),nine(fs.fstatSync(fd,{bigint:true})))&&equal(nine(before),nine(fs.lstatSync(r.path,{bigint:true}))),'Source SHA or nine changed');
  return {path:r.path,relative:r.relative,nine:nine(before),sha256:r.sha256,bytes:r.bytes};
 }finally{if(fd!==undefined)fs.closeSync(fd);}
}
function snapshot(projects){
 const files=[],directories=[],seen=new Set();
 for(const p of Object.values(projects)){
  for(const r of p.files)files.push(sampleFile(r));
  for(const d of p.directories){
   if(seen.has(d))continue;seen.add(d);const s=fs.lstatSync(d,{bigint:true});
   need(s.isDirectory()&&s.uid===BigInt(uid)&&s.gid===BigInt(gid)&&(s.mode&0o7777n)===0o700n&&fs.realpathSync(d)===d,'Owned physical directory differs');
   directories.push({path:d,nine:nine(s)});
  }
 }
 return {files,directories};
}
async function admit(projects){
 const began=performance.now(),end=Math.min(END,began+30000);let previous,count=0,firstEnd;
 for(;;){
  need(performance.now()<end,'New source admission exceeded 30 seconds');
  const sampleStart=performance.now(),rows=snapshot(projects),sampleEnd=performance.now();
  if(previous&&equal(previous,rows))++count;else{previous=rows;count=1;firstEnd=sampleEnd;}
  need(sampleEnd<end,'New source admission exceeded 30 seconds');
  if(count>=3&&sampleStart-firstEnd>=2000)return {observations:count,stableSpanMs:Math.floor(sampleStart-firstEnd),elapsedMs:Math.ceil(sampleEnd-began),firstObservedRows:previous};
  await delay(1050);
 }
}
function fence(projects,admission){
 const rows=snapshot(projects);need(equal(rows,admission.firstObservedRows),'Post-admission source nine/SHA fence failed');
 return {passed:true,sourceFiles:rows.files.length,directories:rows.directories.length,nineAndShaUnchanged:true};
}
function summary(tap){
 need((tap.match(/^TAP version 13\r?$/gm)??[]).length===1,'TAP header missing or ambiguous');
 const result={};
 for(const k of [...COUNTS,'suites']){
  const matches=[...tap.matchAll(new RegExp('^# '+k+' ([0-9]+)[ \\t]*\\r?$','gm'))];
  need(matches.length===1,'TAP count missing or ambiguous');const n=Number(matches[0][1]);
  need(Number.isSafeInteger(n)&&n>=0,'TAP count invalid');if(k==='suites')need(n===0,'Unexpected TAP suites');else result[k]=n;
 }
 return result;
}
function baselineFailures(tap,expectations){
 const matches=[...tap.matchAll(/^([ \t]*)not ok ([0-9]+) - ([^\r\n]+)\r?$/gm)];
 need(matches.length===expectations.baselineCounts.fail,'Unexpected baseline failure records');
 const parent=matches.filter(m=>m[1]===''&&m[3]===GROUP);need(parent.length===1,'Baseline causal parent failure absent');
 const leaves=matches.filter(m=>m[1]!=='');
 need(leaves.length===expectations.baselineMissingExpectedRejections,'Baseline causal leaf cardinality differs');
 for(const name of CASES){
  const leaf=leaves.filter(m=>m[3]==='synchronous collision: '+name);need(leaf.length===1,'Baseline causal leaf name differs');
  const m=leaf[0],next=matches.find(x=>x.index>m.index),part=tap.slice(m.index,next?.index??tap.length);
  need(/^[ \t]+error:[ \t]*['"]?Missing expected rejection\.['"]?[ \t]*\r?$/m.test(part)&&/^[ \t]+code:[ \t]*['"]?ERR_ASSERTION['"]?[ \t]*\r?$/m.test(part),'Baseline leaf is not the expected missing rejection');
 }
 const p=parent[0],tail=tap.slice(p.index);
 need(/^[ \t]+failureType:[ \t]*['"]?subtestsFailed['"]?[ \t]*\r?$/m.test(tail)&&/^[ \t]+code:[ \t]*['"]?ERR_TEST_FAILURE['"]?[ \t]*\r?$/m.test(tail),'Baseline parent is not the aggregate subtest failure');
}
function groupAbsent(pid){
 try{process.kill(-pid,0);return false;}catch(e){if(e.code==='ESRCH')return true;throw e;}
}
async function settleGroup(pid){
 if(!Number.isSafeInteger(pid)||pid<=1)return {absenceConfirmed:false,forcedCleanup:false};
 if(groupAbsent(pid))return {absenceConfirmed:true,forcedCleanup:false};
 try{process.kill(-pid,'SIGKILL');}catch(e){if(e.code==='ESRCH')return {absenceConfirmed:true,forcedCleanup:false};throw e;}
 const end=performance.now()+450;
 while(performance.now()<end){if(groupAbsent(pid))return {absenceConfirmed:true,forcedCleanup:true};await delay(25);}
 return {absenceConfirmed:groupAbsent(pid),forcedCleanup:true};
}
async function run(phase,project,projects,admission,input,records){
 deadline();const preFence=fence(projects,admission),args=['--test','--test-reporter=tap'];
 if(phase==='baseline')args.push('--test-name-pattern='+input.newTestPattern);
 args.push(FILES[1]);
 const timeout=Math.min(14500,Math.floor(END-performance.now()-500));need(timeout>=1,'No child execution budget remains');
 const begin=performance.now();
 const child=spawnSync(process.execPath,args,{cwd:project.base,env:{PATH:'/usr/bin:/bin',HOME:'/tmp',TMPDIR:'/tmp',LANG:'C',LC_ALL:'C',TZ:'UTC'},detached:true,timeout,killSignal:'SIGKILL',maxBuffer:MAX_BUFFER,encoding:'utf8'});
 const record={phase,argv:[process.execPath,...args],exitCode:child.status,signal:child.signal,error:child.error?diagnostic(child.error):null,stdout:child.stdout??'',stderr:child.stderr??'',preFence,elapsedMs:Math.ceil(performance.now()-begin)};
 records.push(record);
 record.childCleanup=await settleGroup(child.pid);
 need(Buffer.byteLength(record.stdout)<=MAX_BUFFER&&Buffer.byteLength(record.stderr)<=MAX_BUFFER,'Child output bound exceeded');
 record.postFence=fence(projects,admission);
 need(!record.error&&record.signal===null&&record.childCleanup.absenceConfirmed&&!record.childCleanup.forcedCleanup,'Child failed or required forced cleanup');
 need(record.stderr==='','Unexpected child stderr');
 record.summary=summary(record.stdout);
 const expected=phase==='baseline'?input.expectations.baselineCounts:input.expectations.candidateCounts;
 need(equal(record.summary,expected),'TAP summary differs from explicit expectations');
 if(phase==='baseline'){
  need(record.exitCode===1,'Baseline must exit one');baselineFailures(record.stdout,input.expectations);record.expectedCausalFailureObserved=true;
 }else{
  need(record.exitCode===0&&!/^[ \t]*not ok /m.test(record.stdout),'Candidate must exit zero without failures');record.fullFilePassed=true;
 }
 record.oracleMatched=true;deadline();
}

const result={format:FORMAT,passed:false,phases:[],sourceAdmission:null,unitTestsOnly:true,oldAuthorityRefreshed:false,previousFailuresRequalified:false,cryptographicFinalityVerified:false,eligibleForMint:false,runtimeDutiesResumed:false,productionQualified:false};
let projects,firstError;
try{
 need(process.platform==='linux'&&['arm64','x64'].includes(process.arch),'Linux ARM64 or AMD64 required');
 need(Number.isSafeInteger(uid)&&Number.isSafeInteger(gid),'POSIX owner identity required');
 const input=validate(await readInput());result.expectations=input.expectations;result.newTestPattern=input.newTestPattern;
 const root=fs.mkdtempSync('/tmp/nano-bridge-serial-unit-');fs.chmodSync(root,0o700);need(fs.realpathSync(root)===root,'Physical new namespace required');result.newNamespace=root;
 projects={baseline:stage(root,'baseline',input.phases.baseline),candidate:stage(root,'candidate',input.phases.candidate)};
 result.sourceAdmission=await admit(projects);
 for(const phase of ['baseline','candidate'])await run(phase,projects[phase],projects,result.sourceAdmission,input,result.phases);
 result.finalFence=fence(projects,result.sourceAdmission);result.passed=true;
}catch(e){firstError=diagnostic(e);result.failure=firstError;}
finally{
 if(projects&&result.sourceAdmission)try{result.finalFence=fence(projects,result.sourceAdmission);}catch(e){result.passed=false;result.finalFence={passed:false,failure:diagnostic(e)};if(!firstError)result.failure=diagnostic(e);}
 result.elapsedMs=Math.ceil(performance.now()-START);
 let text=JSON.stringify(result)+'\n';
 if(Buffer.byteLength(text)>3145728){result.passed=false;text=JSON.stringify({format:FORMAT,passed:false,failure:{message:'Result output byte bound exceeded'},eligibleForMint:false,productionQualified:false})+'\n';}
 process.stdout.write(text);process.exitCode=result.passed?0:1;
}
