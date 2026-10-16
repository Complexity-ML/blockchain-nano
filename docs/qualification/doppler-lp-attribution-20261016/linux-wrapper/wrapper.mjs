import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {performance} from 'node:perf_hooks';
import {setTimeout as delay} from 'node:timers/promises';

const FORMAT='nano-doppler-lp-linux-unit-input-v1';
const RESULT='nano-doppler-lp-linux-unit-result-v1';
const CAUSAL='a pre-existing positive LP balance cannot replace a credit in this migration transaction';
const FILES=Object.freeze([
 'tools/doppler-launch/package.json',
 'tools/doppler-launch/verify-migration.mjs',
 'tools/doppler-launch/migration.test.mjs',
 'docs/qualification/doppler-adversarial-lifecycle-20261004/attempt-b.json',
]);
const MAX_INPUT=1048576,MAX_FILE=262144,MAX_SOURCE=524288,MAX_TAP=131072;
const NINE=['dev','ino','mode','nlink','uid','gid','size','mtimeNs','ctimeNs'];
const C=fs.constants,START=performance.now(),END=START+90000;
const uid=process.getuid?.(),gid=process.getgid?.();
const need=(v,m)=>{if(!v)throw new Error(m);};
const hash=b=>createHash('sha256').update(b).digest('hex');
const equal=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const nine=s=>NINE.map(k=>{need(typeof s[k]==='bigint','BigInt stat field unavailable');return s[k].toString();});
const deadline=()=>need(performance.now()<END,'Wrapper deadline exceeded');
function keys(v,expected){need(v!==null&&typeof v==='object'&&!Array.isArray(v)&&equal(Object.keys(v).sort(),[...expected].sort()),'Input shape refused');}
function diagnostic(e){return {name:typeof e?.name==='string'?e.name:'Error',message:String(e?.message??'Wrapper refused').slice(0,500),...(e?.sourcePath?{sourcePath:e.sourcePath}:{}),...(e?.fence?{fence:e.fence}:{})};}

async function input(){
 const chunks=[];let bytes=0;
 const timer=setTimeout(()=>process.stdin.destroy(new Error('Input deadline exceeded')),5000);
 try{
  for await(const chunk of process.stdin){bytes+=chunk.length;need(bytes<=MAX_INPUT,'Input byte bound exceeded');chunks.push(chunk);}
  let text;try{text=new TextDecoder('utf-8',{fatal:true}).decode(Buffer.concat(chunks));}catch{throw new Error('Input UTF-8 refused');}
  let value;try{value=JSON.parse(text);}catch{throw new Error('Input JSON refused');}return value;
 }finally{clearTimeout(timer);}
}
function validate(value){
 keys(value,['format','phases']);need(value.format===FORMAT,'Input format refused');keys(value.phases,['baseline','candidate']);
 let total=0;
 const phases={};
 for(const name of ['baseline','candidate']){
  keys(value.phases[name],['files']);const files=value.phases[name].files;
  need(Array.isArray(files)&&files.length===FILES.length,'Exactly four source files required per phase');
  const seen=new Set();phases[name]=files.map(row=>{
   keys(row,['path','sha256','bytes','contentBase64']);
   need(FILES.includes(row.path)&&!seen.has(row.path),'Source path refused');seen.add(row.path);
   need(typeof row.sha256==='string'&&/^[0-9a-f]{64}$/.test(row.sha256),'Source SHA shape refused');
   need(Number.isSafeInteger(row.bytes)&&row.bytes>0&&row.bytes<=MAX_FILE,'Source size refused');
   need(typeof row.contentBase64==='string'&&row.contentBase64.length<=4*Math.ceil(MAX_FILE/3),'Source encoding bound exceeded');
   const body=Buffer.from(row.contentBase64,'base64');need(body.toString('base64')===row.contentBase64,'Canonical base64 required');
   need(body.length===row.bytes&&hash(body)===row.sha256,'Source known bytes differ');total+=body.length;
   return {relative:row.path,sha256:row.sha256,bytes:row.bytes,body};
  });
 }
 need(total<=MAX_SOURCE,'Total source bound exceeded');
 for(const relative of FILES.filter(x=>!x.endsWith('verify-migration.mjs'))){
  const a=phases.baseline.find(x=>x.relative===relative),b=phases.candidate.find(x=>x.relative===relative);
  need(a.sha256===b.sha256&&a.bytes===b.bytes,'Only verifier may differ between phases');
 }
 need(phases.baseline.find(x=>x.relative.endsWith('verify-migration.mjs')).sha256!==phases.candidate.find(x=>x.relative.endsWith('verify-migration.mjs')).sha256,'Distinct baseline and candidate verifiers required');
 return phases;
}
function fsyncDir(p){let fd;try{fd=fs.openSync(p,C.O_RDONLY|C.O_DIRECTORY|C.O_NOFOLLOW);fs.fsyncSync(fd);}finally{if(fd!==undefined)fs.closeSync(fd);}}
function project(root,name,files){
 const base=path.join(root,name);fs.mkdirSync(base,{mode:0o700});
 const directories=new Set([base,root]);
 for(const row of files){
  const p=path.join(base,row.relative);fs.mkdirSync(path.dirname(p),{recursive:true,mode:0o700});
  let cursor=path.dirname(p);while(cursor!==root){directories.add(cursor);cursor=path.dirname(cursor);}
  let fd;try{fd=fs.openSync(p,C.O_WRONLY|C.O_CREAT|C.O_EXCL|C.O_NOFOLLOW,0o600);fs.writeFileSync(fd,row.body);fs.fchmodSync(fd,0o400);fs.fsyncSync(fd);}finally{if(fd!==undefined)fs.closeSync(fd);}
  row.path=p;delete row.body;
 }
 const link=path.join(base,'node_modules');fs.symlinkSync('/deps/node_modules',link,'dir');
 for(const p of [...directories].sort((a,b)=>b.length-a.length))fsyncDir(p);
 return {base,files,link,directories:[...directories].sort()};
}
function sampleFile(row){
 deadline();need(fs.realpathSync(row.path)===row.path,'Physical source path required');
 let fd;try{
  const first=fs.lstatSync(row.path,{bigint:true});
  need(first.isFile()&&first.nlink===1n&&first.uid===BigInt(uid)&&first.gid===BigInt(gid)&&(first.mode&0o7777n)===0o400n,'Source owner/type/mode refused');
  fd=fs.openSync(row.path,C.O_RDONLY|C.O_NOFOLLOW|C.O_NONBLOCK);
  const opened=fs.fstatSync(fd,{bigint:true});need(equal(nine(first),nine(opened)),'Source FD/path differs before read');
  need(first.size===BigInt(row.bytes),'Source size differs');
  const h=createHash('sha256'),buffer=Buffer.alloc(32768);let bytes=0;
  for(;;){const n=fs.readSync(fd,buffer,0,buffer.length,null);if(n===0)break;bytes+=n;need(bytes<=row.bytes,'Source read exceeded known size');h.update(buffer.subarray(0,n));}
  const after=fs.fstatSync(fd,{bigint:true}),leaf=fs.lstatSync(row.path,{bigint:true});
  need(equal(nine(first),nine(after))&&equal(nine(first),nine(leaf)),'Source changed during FD read');
  need(bytes===row.bytes&&h.digest('hex')===row.sha256,'Source SHA differs');deadline();
  return {path:row.path,relative:row.relative,materialNine:nine(first),sha256:row.sha256,bytes:row.bytes};
 }catch(e){e.sourcePath=row.path;throw e;}finally{if(fd!==undefined)fs.closeSync(fd);}
}
function snapshot(projects){
 const result={files:[],links:[],directories:[]},seenDirectories=new Set();
 for(const p of Object.values(projects)){
  for(const row of p.files)result.files.push(sampleFile(row));
  const s=fs.lstatSync(p.link,{bigint:true});need(s.isSymbolicLink()&&s.nlink===1n&&s.uid===BigInt(uid)&&s.gid===BigInt(gid)&&fs.readlinkSync(p.link)==='/deps/node_modules','Dependency link differs');
  result.links.push({path:p.link,target:'/deps/node_modules',materialNine:nine(s)});
  for(const dir of p.directories){if(seenDirectories.has(dir))continue;seenDirectories.add(dir);const d=fs.lstatSync(dir,{bigint:true});need(d.isDirectory()&&d.uid===BigInt(uid)&&d.gid===BigInt(gid)&&(d.mode&0o7777n)===0o700n&&fs.realpathSync(dir)===dir,'Projection directory refused');result.directories.push({path:dir,materialNine:nine(d)});}
 }
 return result;
}
async function admit(projects){
 const started=performance.now(),end=Math.min(END,started+30000);let previous,rounds=0,firstEnd;
 for(;;){
  need(performance.now()<end,'New source admission did not settle within 30 seconds');
  const sampleStart=performance.now(),current=snapshot(projects),sampleEnd=performance.now();
  if(previous&&equal(previous,current))rounds++;else{previous=current;rounds=1;firstEnd=sampleEnd;}
  const span=sampleStart-firstEnd;
  need(sampleEnd<end,'New source admission exceeded 30 seconds');
  if(rounds>=3&&span>=2000)return {format:'nano-doppler-lp-linux-source-admission-v1',observations:rounds,stableSpanMs:Math.floor(span),elapsedMs:Math.ceil(sampleEnd-started),firstObservedRows:previous};
  await delay(1050);
 }
}
function fence(projects,admission){
 const current=snapshot(projects);
 if(!equal(current,admission.firstObservedRows)){
  const e=new Error('Post-admission source fence failed');e.fence={passed:false,observedRows:current};throw e;
 }
 return {passed:true,files:current.files.length,links:current.links.length,shaAndNineEqual:true};
}
function summary(tap){
 const out={};
 for(const key of ['tests','suites','pass','fail','cancelled','skipped','todo']){
  const matches=[...tap.matchAll(new RegExp('^# '+key+' ([0-9]+)\\s*$','gm'))];need(matches.length===1,'TAP summary missing or ambiguous');
  out[key]=Number(matches[0][1]);need(Number.isSafeInteger(out[key]),'TAP count refused');
 }
 need(/^TAP version 13\r?$/m.test(tap),'TAP header missing');return out;
}
function run(name,project,projects,admission,records){
 deadline();const preFence=fence(projects,admission);
 const args=['--test','--test-reporter=tap'];
 if(name==='baseline')args.push('--test-name-pattern=^'+CAUSAL+'$');
 args.push('tools/doppler-launch/migration.test.mjs');
 deadline();const timeout=Math.max(1,Math.min(15000,Math.floor(END-performance.now())));
 const child=spawnSync(process.execPath,args,{cwd:project.base,env:{PATH:'/usr/bin:/bin',HOME:'/tmp',TMPDIR:'/tmp',LANG:'C',LC_ALL:'C',TZ:'UTC'},timeout,maxBuffer:MAX_TAP,encoding:'utf8',killSignal:'SIGKILL'});
 const record={phase:name,argv:args,exitCode:child.status,signal:child.signal,error:child.error?diagnostic(child.error):null,stdout:child.stdout??'',stderr:child.stderr??'',preFence};
 records.push(record); // Preserve raw test observations even when the later fence refuses.
 need(Buffer.byteLength(record.stdout)<=MAX_TAP&&Buffer.byteLength(record.stderr)<=MAX_TAP,'Child output exceeded bound');
 record.postFence=fence(projects,admission);deadline();
 if(record.error||child.signal!==null){record.oracleMatched=false;return record;}
 try{
  record.summary=summary(record.stdout);const s=record.summary;
  if(name==='baseline'){
   need(child.status===1&&equal(s,{tests:1,suites:0,pass:0,fail:1,cancelled:0,skipped:0,todo:0}),'Baseline test counters differ');
   need(record.stdout.split('\n').filter(l=>/^not ok [0-9]+ - /.test(l)).length===1&&record.stdout.includes('not ok 1 - '+CAUSAL),'Baseline causal test differs');
   need(/^\s+error: ['"]?Missing expected rejection\.['"]?\s*$/m.test(record.stdout)&&/^\s+code: ['"]?ERR_ASSERTION['"]?\s*$/m.test(record.stdout),'Baseline failure is not missing expected rejection');
   need(/^1\.\.1\r?$/m.test(record.stdout),'Baseline plan differs');record.expectedCausalFailureObserved=true;
  }else{
   need(child.status===0&&equal(s,{tests:29,suites:0,pass:29,fail:0,cancelled:0,skipped:0,todo:0}),'Candidate test counters differ');
   need(record.stdout.split('\n').filter(l=>/^ok [0-9]+ - /.test(l)).length===8&&/^1\.\.8\r?$/m.test(record.stdout)&&!/^\s*not ok /m.test(record.stdout),'Candidate top-level test plan differs');
   record.candidateTestsPassed=true;
  }
  record.oracleMatched=true;
 }catch(e){record.oracleMatched=false;record.oracleFailure=diagnostic(e);}
 return record;
}

const result={format:RESULT,passed:false,sourceAdmission:null,phases:[],runtimeDutiesResumed:false,networkCallsMade:false,publicDeployment:false,productionQualified:false,oldAdmissionRefreshed:false,previousRefusalsRequalified:false,dependencyTreeRequalified:false};
let projects,firstError;
try{
 need(process.platform==='linux'&&['arm64','x64'].includes(process.arch),'Linux 64-bit supported architectures required');
 need(Number.isSafeInteger(uid)&&Number.isSafeInteger(gid),'POSIX owner identity required');
 need(fs.lstatSync('/deps/node_modules').isDirectory(),'Root-held dependency directory required');
 const phases=validate(await input());deadline();
 const root=fs.mkdtempSync('/tmp/nano-doppler-lp-unit-');fs.chmodSync(root,0o700);need(fs.realpathSync(root)===root,'Physical new namespace required');
 result.newNamespace=root;projects={baseline:project(root,'baseline',phases.baseline),candidate:project(root,'candidate',phases.candidate)};fsyncDir(root);
 result.sourceAdmission=await admit(projects);
 for(const name of ['baseline','candidate']){const record=run(name,projects[name],projects,result.sourceAdmission,result.phases);need(record.oracleMatched,'Targeted unit phase refused');}
 result.finalFence=fence(projects,result.sourceAdmission);result.passed=true;
}catch(e){firstError=diagnostic(e);result.failure=firstError;}
finally{
 if(projects&&result.sourceAdmission){try{result.finalFence=fence(projects,result.sourceAdmission);}catch(e){result.passed=false;result.finalFence={passed:false,failure:diagnostic(e)};if(!firstError)result.failure=diagnostic(e);}}
 result.elapsedMs=Math.ceil(performance.now()-START);
 let output=JSON.stringify(result)+'\n';
 if(Buffer.byteLength(output)>786432){result.passed=false;output=JSON.stringify({format:RESULT,passed:false,failure:{message:'Result output exceeded bound'},runtimeDutiesResumed:false,productionQualified:false})+'\n';}
 process.stdout.write(output);process.exitCode=result.passed?0:1;
}
