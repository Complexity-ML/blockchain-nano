import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {validateStoppedValidators} from './validator-gate.mjs';
const healthy={event:'bellatrix-runtime-stopped',validator:{healthy:true,failure:null,attestationsAdmitted:1}};
for(const [name,records,accepted] of [
 ['all validators healthy',[healthy],true],
 ['missing terminal status',[],false],
 ['ambiguous repeated status',[healthy,healthy],false],
 ['follower without local votes',[{...healthy,validator:{...healthy.validator,attestationsAdmitted:0}}],false],
 ['validator failed but follower finalized',[{...healthy,validator:{healthy:false,failure:'Proposal expired before provider',attestationsAdmitted:0}}],false],
 ['missing health fields',[{event:'bellatrix-runtime-stopped'}],false]
])test(name,async()=>{
 const directory=await mkdtemp(join(tmpdir(),'nano-validator-gate-'));
 try{
  for(const file of ['native.log','docker-mac.log','vps.log']){
   const rows=file==='docker-mac.log'?records:[healthy];
   await writeFile(join(directory,file),'non-json diagnostic\n'+rows.map(x=>JSON.stringify(x)).join('\n'));
  }
  if(accepted)assert.equal((await validateStoppedValidators(directory)).length,3);
  else await assert.rejects(validateStoppedValidators(directory));
 }finally{await rm(directory,{recursive:true,force:true});}
});
test('Capella status is required when Capella is selected',async()=>{
 const directory=await mkdtemp(join(tmpdir(),'nano-capella-validator-gate-'));
 try{
  const record={...healthy,event:'capella-runtime-stopped'};
  for(const file of ['native.log','docker-mac.log','vps.log'])await writeFile(join(directory,file),JSON.stringify(record)+'\n');
  assert.equal((await validateStoppedValidators(directory,'capella')).length,3);
  await assert.rejects(validateStoppedValidators(directory,'bellatrix'));
  await writeFile(join(directory,'vps.log'),JSON.stringify({...record,validator:{...record.validator,healthy:false,failure:'stopped'}}));
  await assert.rejects(validateStoppedValidators(directory,'capella'));
 }finally{await rm(directory,{recursive:true,force:true});}
});
