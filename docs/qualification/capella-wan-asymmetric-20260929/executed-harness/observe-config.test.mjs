import test from 'node:test';
import assert from 'node:assert/strict';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdtemp,writeFile,access,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
const exec=promisify(execFile);
for(const kind of ['native','docker','ssh-docker'])test(`${kind}: aliases cannot count as distinct observers`,async()=>{
 const dir=await mkdtemp(join(tmpdir(),'nano-observer-config-'));
 try{
  const base={kind,url:'http://127.0.0.1:18545',...(kind==='native'?{}:{container:'nano-qualification-same'}),...(kind==='ssh-docker'?{ssh:'ubuntu@192.0.2.1'}:{})};
  const file=join(dir,'observers.json'),out=join(dir,'output');
  await writeFile(file,JSON.stringify({revision:'a'.repeat(40),genesisHash:'0x'+'b'.repeat(64),nodes:[{...base,name:'first'},{...base,name:'second'}]}));
  await assert.rejects(exec(process.execPath,['tests/multihost/observe.mjs',file,out],{timeout:5000}),e=>e.code===1&&e.stderr.includes('Duplicate observer transport'));
  await assert.rejects(access(out)); // Refused before observation or side effects.
 }finally{await rm(dir,{recursive:true,force:true});}
});
