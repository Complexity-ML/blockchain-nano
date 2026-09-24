import {createHash} from 'node:crypto';
import {createReadStream} from 'node:fs';
import {mkdtemp,readFile,readdir,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {resolve,join,relative} from 'node:path';
import {execFileSync} from 'node:child_process';
import assert from 'node:assert/strict';
const archive=resolve(process.argv[2]??''),root=resolve(process.argv[3]??'.');
const expected='b183702a5b447b465873865357ced9eb342315922e52e31b714e2de115dd0bb4';
async function digest(path){const h=createHash('sha256');for await(const c of createReadStream(path))h.update(c);return h.digest('hex');}
assert.equal(await digest(archive),expected,'Archive differs from pinned tests@v20.0.0');
const temp=await mkdtemp(join(tmpdir(),'nano-transaction-corpus-'));
const results=[],issues=[],counts={};let positive=0,negative=0;
try{
 execFileSync('tar',['-xzf',archive,'-C',temp,'fixtures/transaction_tests'],{stdio:'pipe'});
 const source=join(temp,'fixtures/transaction_tests'),bundled=join(root,'tests/fixtures/ethereum-transactions');
 async function walk(dir){const files=[];for(const e of await readdir(dir,{withFileTypes:true})){const p=join(dir,e.name);if(e.isDirectory())files.push(...await walk(p));else if(e.isFile()&&e.name.endsWith('.json'))files.push(p);else issues.push('Unexpected archive entry: '+relative(source,p));}return files.sort();}
 const names=new Set();
 for(const file of await walk(source)){
  const name=relative(source,file).split('/').join('--');assert(!names.has(name),'Flattened filename collision');names.add(name);
  const sha256=await digest(file);let actual=null;try{actual=await digest(join(bundled,name));}catch{issues.push('Missing bundled file: '+name);}
  if(actual!==sha256)issues.push('Content mismatch: '+name);
  const fixture=JSON.parse(await readFile(file,'utf8'));let variants=0;
  for(const test of Object.values(fixture)){assert(test.txbytes&&test.result,'Unrecognized transaction fixture');for(const [fork,result] of Object.entries(test.result)){counts[fork]=(counts[fork]??0)+1;variants++;if(result.exception)negative++;else positive++;}}
  results.push({file:name,archiveSha256:sha256,bundledSha256:actual,variants});
 }
 for(const name of await readdir(bundled))if(name.endsWith('.json')&&!names.has(name))issues.push('Extra bundled fixture: '+name);
 console.log(JSON.stringify({scope:'Exact archive-to-bundled transaction_tests inventory, not execution or general transaction conformance',archive,archiveSha256:expected,files:results.length,counts,positive,negative,issues,results},null,2));
 process.exitCode=issues.length?1:0;
}finally{await rm(temp,{recursive:true,force:true});}
