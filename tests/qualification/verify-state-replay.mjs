import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {resolve,join} from 'node:path';
import {digest,parseReport} from './audit-evidence.mjs';

// Inventory paths must refer to the files previously checked against the pinned archive.
const directory=resolve(process.argv[2]??'');
assert(process.argv[2],'Usage: node verify-state-replay.mjs <replay directory>');
const inventory=JSON.parse(await readFile(join(directory,'inventory.json'),'utf8'));
const groups=new Map();
for(const entry of inventory){
 const group=entry.archivePath.split('/')[2]?.replace(/^for_/,'');
 assert(group,'Missing archive group');
 assert.equal(await digest(entry.path),entry.sha256,'Fixture changed since archive verification');
 if(!groups.has(group))groups.set(group,new Map());
 const cases=groups.get(group),fixture=JSON.parse(await readFile(entry.path,'utf8'));
 for(const [name,test] of Object.entries(fixture))for(const [fork,variants] of Object.entries(test.post)){
  for(let variant=0;variant<variants.length;variant++){
   const key=JSON.stringify([entry.path,name,fork,variant]);assert(!cases.has(key),'Duplicate input identity');
   cases.set(key,variants[variant]);
  }
 }
}
const results=[];
for(const [group,expected] of groups){
 const report=parseReport(await readFile(join(directory,group+'.jsonl'),'utf8'));
 assert.deepEqual(report.issues,[],group+' report integrity');
 const observed=new Set();
 for(const row of report.rows){
  const key=JSON.stringify([row.file,row.case,row.fork,row.variant]);
  assert(expected.has(key),group+' unexpected case: '+key);assert(!observed.has(key),'Duplicate output');observed.add(key);
  assert.equal(row.status,'matched',group+' failed case: '+key);
  const post=expected.get(key);
  assert.equal(row.stateRoot?.expected,post.hash,'Report state expectation differs from fixture');
  assert.equal(row.stateRoot?.actual,post.hash,'Executed state differs from fixture');
  if(post.expectException){
   assert.equal(row.validation?.expected,post.expectException,'Rejection expectation differs from fixture');
   assert(post.expectException.split('|').includes(row.validation?.actual),'Actual rejection differs from fixture');
   assert.equal(row.validation?.stateUnchanged,true);assert.equal(row.validation?.transactionExecuted,false);
  }else{
   assert.equal(row.validation,undefined,'Unexpected transaction rejection');
   assert.equal(row.logsHash?.expected,post.logs,'Log expectation differs from fixture');
   assert.equal(row.logsHash?.actual,post.logs,'Executed logs differ from fixture');
   if(post.receipt){
    assert.equal(row.transactionHash,post.receipt.transactionHash,'Transaction hash differs from fixture');
    assert.equal(BigInt(row.gasUsed),BigInt(post.receipt.cumulativeGasUsed),'Gas used differs from fixture');
   }
  }
 }
 assert.equal(observed.size,expected.size,group+' omitted cases');
 assert.equal(report.summary.validationMatched,[...expected.values()].filter(post=>post.expectException).length,'Expected rejection count differs from fixtures');
 assert.equal(report.summary.files,inventory.filter(e=>e.archivePath.split('/')[2]==='for_'+group).length);
 results.push({group,files:report.summary.files,expected:expected.size,observed:observed.size,validationMatched:report.summary.validationMatched,coverageVerified:true});
}
console.log(JSON.stringify({scope:'Exact case/fork/variant/file coverage against archive-verified inputs; six supported groups only, not all archive forks or production readiness',results},null,2));
