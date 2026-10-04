import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile,mkdtemp,mkdir,writeFile,symlink,rm} from 'node:fs/promises';
import {join} from 'node:path';
import {tmpdir} from 'node:os';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
const exec=promisify(execFile);
// Exercise the actual exporter shell body, with private local fixtures instead
// of Docker. Image assembly, ELF linkage and runtime behavior remain separate.
const source=await readFile(new URL('./package-candidate.sh',import.meta.url),'utf8');
const match=/ -c '\n([\s\S]*?)\n  ' sh/.exec(source);
assert.ok(match,'exporter body must remain identifiable');
async function fixture(action){
 const root=await mkdtemp(join(tmpdir(),'nano-real-export-'));
 try{
  const core=join(root,'core'),parent=join(root,'coherent'),network=join(parent,'network'),legacy=join(root,'network');
  await mkdir(core);await mkdir(network,{recursive:true});await mkdir(legacy);
  await writeFile(join(core,'nano'),'qualified-core');await writeFile(join(core,'production-source-revision.txt'),'c'.repeat(40));
  await writeFile(join(network,'nano-p2p'),'qualified-p2p');await writeFile(join(legacy,'nano-p2p'),'stale-p2p');
  return await action({root,core,parent,network,legacy,run:()=>exec('sh',['-c',match[1],'sh',core,network],{encoding:'buffer',timeout:10000,maxBuffer:1024*1024})});
 }finally{await rm(root,{recursive:true,force:true});}
}
test('real export selects isolated P2P bytes despite a stale root P2P',async()=>fixture(async({root,run})=>{
 const result=await run(),archive=join(root,'export.tar'),output=join(root,'output');await writeFile(archive,result.stdout);await mkdir(output);
 await exec('tar',['-xf',archive,'-C',output]);
 assert.equal(await readFile(join(output,'nano'),'utf8'),'qualified-core');
 assert.equal(await readFile(join(output,'network','nano-p2p'),'utf8'),'qualified-p2p');
 assert.equal(await readFile(join(output,'production-source-revision.txt'),'utf8'),'c'.repeat(40));
}));
test('real export refuses a missing selected P2P without using the legacy binary',async()=>fixture(async({network,run})=>{
 await rm(join(network,'nano-p2p'));await assert.rejects(run(),error=>error.code===1);
}));
test('real export refuses symlinks in selected P2P binary, directory or parent',async()=>{
 for(const target of ['binary','directory','parent'])await fixture(async({root,parent,network,legacy,run})=>{
  if(target==='binary'){await rm(join(network,'nano-p2p'));await symlink(join(legacy,'nano-p2p'),join(network,'nano-p2p'));}
  if(target==='directory'){await rm(network,{recursive:true});await symlink(legacy,network);}
  if(target==='parent'){await rm(parent,{recursive:true});await mkdir(join(root,'replacement','network'),{recursive:true});await writeFile(join(root,'replacement','network','nano-p2p'),'qualified-p2p');await symlink(join(root,'replacement'),parent);}
  await assert.rejects(run(),error=>error.code===1);
 });
});
