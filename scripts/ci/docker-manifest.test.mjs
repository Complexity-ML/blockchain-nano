import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,readFile,chmod,mkdir,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
const exec=promisify(execFile);
const script=fileURLToPath(new URL('./docker-manifest.cmake',import.meta.url));
async function fixture(body){
 const root=await mkdtemp(join(tmpdir(),'nano-manifest-'));
 try{
  const program=join(root,'test-node'),network=join(root,'nano-p2p'),helper=join(root,'nano-altair-probe');
  for(const path of [program,network,helper]){await writeFile(path,'#!/bin/sh\nexit 0\n');await chmod(path,0o755);}
  const registry=join(root,'registry.json'),output=join(root,'manifest.json');
  const run=async(args,options=[])=>{
   await writeFile(registry,JSON.stringify({tests:[{name:'real-p2p',command:[program,...args]}]}));
   await exec('cmake',[`-DREGISTRY=${registry}`,`-DOUTPUT=${output}`,`-DNETWORK_EXECUTABLE=${network}`,...options,'-P',script]);
   return JSON.parse(await readFile(output,'utf8'));
  };
  await body({root,program,network,helper,run});
 }finally{await rm(root,{recursive:true,force:true});}
}
test('manifest records executable helper arguments and ignores data/directories',async()=>fixture(async({root,program,network,helper,run})=>{
 const data=join(root,'genesis.json'),directory=join(root,'state');await writeFile(data,'{}');await chmod(data,0o755);await mkdir(directory);
 const manifest=await run([network,helper,data,directory,'--mode','fixture']);
 assert.deepEqual(Object.keys(manifest).sort(),[program,network,helper].sort());
}));
test('helper-only replacement changes the manifest while primary hashes stay fixed',async()=>fixture(async({program,network,helper,run})=>{
 const before=await run([helper]);await writeFile(helper,'#!/bin/sh\nexit 1\n');
 const after=await run([helper]);assert.notEqual(after[helper],before[helper]);
 assert.equal(after[program],before[program]);assert.equal(after[network],before[network]);
}));
test('missing required Altair helper prevents qualification',async()=>fixture(async({helper,run})=>{
 await rm(helper);await assert.rejects(run([helper]),/Missing executable/);
}));
test('nonexecutable required Altair helper prevents qualification',async()=>fixture(async({helper,run})=>{
 await chmod(helper,0o644);await assert.rejects(run([helper]),/Non-executable CTest command/);
}));
test('explicit node binary is recorded even when absent from command arguments',async()=>fixture(async({root,run})=>{
 const node=join(root,'nano');await writeFile(node,'#!/bin/sh\nexit 0\n');await chmod(node,0o755);
 const manifest=await run([], [`-DNODE_EXECUTABLE=${node}`]);assert.match(manifest[node],/^[a-f0-9]{64}$/);
 await rm(node);await assert.rejects(run([], [`-DNODE_EXECUTABLE=${node}`]),/Missing executable/);
}));
