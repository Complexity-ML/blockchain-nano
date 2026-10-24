// Known text fixtures only. No RocksDB, signer, key generation or network.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,realpath,mkdir,writeFile,readFile,lstat,rm,symlink,link} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {createNativeOwnerCopies} from './rotation-owned-copy.mjs';

async function fixture(){
 const directory=await realpath(await mkdtemp(join(tmpdir(),'nano-owned-copy-fixture-'))),namespace='isolated-capella-'+'a'.repeat(64),owner='mac-native';
 const dataRoot=join(directory,'source-data',namespace,owner),witnessRoot=join(directory,'source-witness',namespace,owner),files=[];
 for(const root of [dataRoot,witnessRoot])await mkdir(root,{recursive:true,mode:0o700});
 const file=async(root,relative,bytes)=>{const path=join(root,relative);await mkdir(join(path,'..'),{recursive:true,mode:0o700});await writeFile(path,bytes,{flag:'wx',mode:0o600});files.push({root,relative,bytes:Buffer.from(bytes),source:path,material:await lstat(path,{bigint:true})});};
 await file(dataRoot,'peer.key','known dummy peer identity bytes, not a private key');
 await file(dataRoot,'chain/CURRENT','known core primary fixture');
 await file(dataRoot,'chain/LOCK',Buffer.alloc(0));
 await file(witnessRoot,'chain-witness/CURRENT','known external core witness fixture');
 await file(witnessRoot,'chain-witness/LOCK',Buffer.alloc(0));
 for(let i=0;i<16;++i){await file(dataRoot,'journal-'+i+'/CURRENT','known signer journal '+i);await file(witnessRoot,'witness-'+i+'/CURRENT','known signer witness '+i);}
 const options={dataRoot,witnessRoot,destination:join(directory,'copy'),namespace,owner,resources:{maximumInventoryFiles:'0x1000',maximumInventoryBytes:'0x400000'}};
 return {directory,files,options,async dispose(){await rm(directory,{recursive:true,force:true});}};
}

test('whole owner filesystem copy retains peer identity and all sixteen signer journals and witnesses, with independent private files',{timeout:15000},async()=>{
 const f=await fixture();try{
  const copy=await createNativeOwnerCopies(f.options);
  assert.equal(copy.summary.roleRoots,2);assert.equal(copy.summary.sourceFiles,37);
  assert.equal(copy.summary.payloadBytes,f.files.reduce((sum,row)=>sum+row.bytes.length,0));
  assert.equal(copy.summary.copiesUnchangedAfterAudit,false);
  assert.equal(copy.summary.productionQualified,false);
  for(const row of f.files){
   const root=row.root===f.options.dataRoot?copy.dataRoot:copy.witnessRoot,path=join(root,row.relative),s=await lstat(path,{bigint:true});
   assert(s.isFile());assert.equal(s.mode&0o7777n,0o600n);assert.equal(s.nlink,1n);
   assert.notEqual(String(s.dev)+':'+String(s.ino),String(row.material.dev)+':'+String(row.material.ino));
   assert.deepEqual(await readFile(path),row.bytes);assert.deepEqual(await readFile(row.source),row.bytes);
  }
  assert.equal((await copy.verify()).copiesUnchangedAfterAudit,true);
  // A resumed COPY may advance; the retained source fence must still hold.
  await writeFile(join(copy.dataRoot,'journal-0/CURRENT'),'new copied journal state');
  assert.equal((await copy.verifySource()).sourceNineFieldAndShaUnchanged,true);
  await assert.rejects(copy.verify(),/Stopped rotation physical copy refused/);
  // The original source is never given a replacement baseline.
  await writeFile(join(f.options.dataRoot,'journal-0/CURRENT'),'changed original journal');
  await assert.rejects(copy.verifySource(),/Stopped rotation physical copy refused/);
 }finally{await f.dispose();}
});

test('whole owner filesystem copy refuses source symlinks and multiply linked files before admitting any replacement',{timeout:10000},async()=>{
 for(const kind of ['symlink','hardlink']){
  const f=await fixture();try{
   const original=join(f.options.dataRoot,'peer.key'),alias=join(f.options.dataRoot,'identity-alias');
   if(kind==='symlink')await symlink(original,alias);else await link(original,alias);
   await assert.rejects(createNativeOwnerCopies(f.options),/Stopped rotation physical copy refused/);
   await assert.rejects(lstat(f.options.destination),{code:'ENOENT'});
   assert.deepEqual(await readFile(original),f.files.find(row=>row.relative==='peer.key').bytes);
  }finally{await f.dispose();}
 }
});
