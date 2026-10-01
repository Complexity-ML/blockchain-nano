import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,writeFile,readFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
const exec=promisify(execFile);
const repository=process.cwd();
async function exercise(mode){
 const dir=await mkdtemp(join(tmpdir(),'nano-full-postrun-'));
 try{
  const bin=join(dir,'bin'),build=join(dir,'build'),reports=join(dir,'reports'),binary=join(dir,'fixture-executable');
  await mkdir(bin);await mkdir(join(build,'Testing','Temporary'),{recursive:true});
  await writeFile(binary,'#!/bin/sh\nexit 0\n',{mode:0o700});
  await writeFile(join(dir,'tracked-source.cpp'),'// original source\n');
  await mkdir(join(dir,'scripts','ci'),{recursive:true});
  for(const name of ['regression-inventory.mjs','verify-results.sh','audit-eest-ctest.mjs','eest-counts.mjs'])await writeFile(join(dir,'scripts','ci',name),await readFile(join(repository,'scripts','ci',name)));
  const mock=async(name,source)=>writeFile(join(bin,name),'#!'+process.execPath+'\n'+source,{mode:0o700});
  await mock('git',`if(process.argv.includes('rev-parse'))console.log('1'.repeat(40));else if(process.argv.includes('ls-files'))process.stdout.write('tracked-source.cpp'+String.fromCharCode(0));`);
  await mock('cmake',`if(process.argv.includes('--version'))console.log('mock CMake');`);
  await mock('ctest',`const fs=require('node:fs'),p=require('node:path');const a=process.argv.slice(2),b=process.env.TEST_BINARY;if(a.some(x=>x.startsWith('--show-only'))){console.log(JSON.stringify({tests:[{name:'ethereum_official_regressions',command:[b],properties:[{name:'TIMEOUT',value:30}]}]}));}else{const mode=process.env.TEST_MODE,fail=mode==='ctest-failure',xml=a[a.indexOf('--output-junit')+1],build=a[a.indexOf('--test-dir')+1];fs.writeFileSync(xml,'<testsuite tests="1" failures="'+(fail?1:0)+'" skipped="0"><testcase name="ethereum_official_regressions" status="'+(fail?'fail':'run')+'">'+(fail?'<failure message="injected failure"/>':'')+'</testcase></testsuite>');const rows=[{file:'fixture.json',case:'execution',fork:'Berlin',variant:0,status:'matched'},{summary:{files:1,matched:1,executionMatched:1,validationMatched:0,mismatched:0,unsupported:0,errors:0}}];fs.writeFileSync(p.join(build,'Testing','Temporary','LastTest.log'),'1/1 Testing: ethereum_official_regressions\\n'+rows.map(r=>JSON.stringify(r)).join('\\n')+'\\nTest time = 0 sec\\n');if(mode==='tamper')fs.appendFileSync(b,'# injected mutation\\n');if(mode==='source-tamper')fs.appendFileSync('tracked-source.cpp','// injected mutation\\n');process.exitCode=fail?8:0;}`);
  let code=0;
  try{await exec('bash',[join(repository,'scripts/ci/full.sh'),build,reports],{cwd:dir,env:{...process.env,PATH:bin+':'+process.env.PATH,CXX:process.execPath,NANO_NETWORK_EXECUTABLE:binary,TEST_BINARY:binary,TEST_MODE:mode},timeout:30000});}catch(error){code=error.code;try{await readFile(join(reports,'ctest-exit.txt'));}catch{throw Error('Runner stopped before CTest: '+error.stdout+' '+error.stderr);}}
  return {code,ctestExit:(await readFile(join(reports,'ctest-exit.txt'),'utf8')).trim(),auditExit:(await readFile(join(reports,'audit-exit.txt'),'utf8')).trim(),eest:JSON.parse(await readFile(join(reports,'eest-counts.json'),'utf8')),binaryAfter:await readFile(join(reports,'executables-after.sha256'),'utf8'),comparison:await readFile(join(reports,'executable-compare.log'),'utf8'),sourceComparison:await readFile(join(reports,'source-compare.log'),'utf8')};
 }finally{await rm(dir,{recursive:true,force:true});}
}
test('full runner preserves post-run evidence after a CTest failure',async()=>{const r=await exercise('ctest-failure');assert.equal(r.code,1);assert.equal(r.ctestExit,'8');assert.equal(r.auditExit,'1');assert.equal(r.eest.executionMatched,1);assert.ok(r.binaryAfter);assert.equal(r.comparison,'');});
test('successful CTest cannot qualify a binary mutated during tests',async()=>{const r=await exercise('tamper');assert.equal(r.code,1);assert.equal(r.ctestExit,'0');assert.equal(r.auditExit,'1');assert.match(r.comparison,/differ/);});
test('unchanged binaries and complete reports preserve successful exit',async()=>{const r=await exercise('success');assert.equal(r.code,0);assert.equal(r.ctestExit,'0');assert.equal(r.auditExit,'0');assert.equal(r.eest.executionMatched,1);assert.equal(r.comparison,'');});

test('successful CTest cannot qualify sources changed during tests',async()=>{const r=await exercise('source-tamper');assert.equal(r.code,1);assert.equal(r.ctestExit,'0');assert.equal(r.auditExit,'1');assert.match(r.sourceComparison,/differ/);assert.equal(r.comparison,'');});
