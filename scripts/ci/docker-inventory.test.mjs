import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync,writeFileSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
const tool=new URL('./docker-inventory.mjs',import.meta.url).pathname;
const entry=(properties=[{name:'TIMEOUT',value:5}])=>({name:'fixture',command:['/build/fixture'],properties});
function audit(registry,manifest){
 const dir=mkdtempSync(join(tmpdir(),'nano-docker-inventory-'));
 try{
  writeFileSync(join(dir,'registry.json'),JSON.stringify(registry));writeFileSync(join(dir,'manifest.json'),JSON.stringify(manifest));
  return spawnSync(process.execPath,[tool,join(dir,'registry.json'),join(dir,'manifest.json')],{encoding:'utf8'});
 }finally{rmSync(dir,{recursive:true,force:true});}
}
test('container-only executable paths are checked against measured manifest',()=>{
 const result=audit({tests:[entry()]},{'/build/fixture':'a'.repeat(64)});
 assert.equal(result.status,0,result.stderr);const report=JSON.parse(result.stdout);
 assert.equal(report.completeSelection,true);assert.deepEqual(report.missingExecutables,[]);assert.equal(report.runtimeTestsExecuted,0);
});
test('missing executable or timeout refuses full qualification before execution',()=>{
 assert.notEqual(audit({tests:[entry()]},{'/build/other':'b'.repeat(64)}).status,0);
 assert.notEqual(audit({tests:[entry([])]},{'/build/fixture':'a'.repeat(64)}).status,0);
});
test('invalid provenance, duplicate registrations and skipped tests fail closed',()=>{
 for(const manifest of [{},[],{'fixture':'a'.repeat(64)},{'/build/fixture':'bad'}])assert.notEqual(audit({tests:[entry()]},manifest).status,0);
 assert.notEqual(audit({tests:[entry(),entry()]},{'/build/fixture':'a'.repeat(64)}).status,0);
 assert.notEqual(audit({tests:[entry([{name:'TIMEOUT',value:5},{name:'SKIP_RETURN_CODE',value:0}])]},{'/build/fixture':'a'.repeat(64)}).status,0);
});
