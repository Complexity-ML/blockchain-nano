import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync,writeFileSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
const script=new URL('./verify-results.sh',import.meta.url).pathname;
function audit(xml){
 const dir=mkdtempSync(join(tmpdir(),'nano-result-audit-'));
 try{
  writeFileSync(join(dir,'registry.json'),JSON.stringify({tests:[{name:'one'},{name:'two'}]}));
  writeFileSync(join(dir,'results.xml'),xml);
  return spawnSync('bash',[script,join(dir,'registry.json'),join(dir,'results.xml')],{encoding:'utf8'});
 }finally{rmSync(dir,{recursive:true,force:true});}
}
const one='<testcase name="one" status="run"/>',two='<testcase name="two" status="run"/>';
test('exact successful registry produces a terminal complete audit',()=>{
 const result=audit(`<testsuite tests="2" failures="0" skipped="0">${one}${two}</testsuite>`);
 assert.equal(result.status,0,result.stderr);
 assert.deepEqual(JSON.parse(result.stdout),{registered:2,reported:2,failed:[],skipped:0,complete:true});
});
test('missing, duplicate and foreign results cannot produce a green gate',()=>{
 for(const cases of [one,one+one,one+'<testcase name="other" status="run"/>'])assert.notEqual(audit(`<testsuite tests="2">${cases}</testsuite>`).status,0);
});
test('skipped, disabled and non-executed cases fail even with green summary',()=>{
 for(const cases of [one+'<testcase name="two" status="notrun"/>',one+'<testcase name="two" status="run"><skipped/></testcase>'])assert.notEqual(audit(`<testsuite tests="2" failures="0">${cases}</testsuite>`).status,0);
 for(const attr of ['skipped="1"','disabled="1"'])assert.notEqual(audit(`<testsuite tests="2" ${attr}>${one}${two}</testsuite>`).status,0);
});
test('real failures remain visible and malformed/inconsistent reports fail closed',()=>{
 const failed=audit(`<testsuite tests="2" failures="1">${one}<testcase name="two" status="fail"><failure>broken</failure></testcase></testsuite>`);
 assert.notEqual(failed.status,0);assert.deepEqual(JSON.parse(failed.stdout).failed,['two']);
 for(const xml of ['',`<testsuite tests="3">${one}${two}</testsuite>`,`<testsuite tests="2" failures="1">${one}${two}</testsuite>`])assert.notEqual(audit(xml).status,0);
});
