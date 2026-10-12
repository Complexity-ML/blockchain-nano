import assert from 'node:assert/strict';
import {realpath,stat} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {isAbsolute,relative,resolve,sep} from 'node:path';

const contains=(root,path)=>{const rel=relative(root,path);return rel===''||(rel!=='..'&&!rel.startsWith('..'+sep)&&!isAbsolute(rel));};
// Historical temporary sources remain available to separate read-only audits.
// This guard applies before creating a campaign or starting runtime writers.
export async function persistentRuntimeDirectory(directory){
 assert.ok(typeof directory==='string'&&directory.length>0,'Existing runtime directory required');
 const canonical=await realpath(resolve(directory));
 assert.ok((await stat(canonical)).isDirectory(),'Runtime preparation must be an existing directory');
 const roots=new Set();
 for(const root of new Set(['/tmp','/private/tmp','/var/tmp',tmpdir()])){
  try{roots.add(await realpath(root));}
  catch(error){if(error.code!=='ENOENT')throw error;}
 }
 for(const root of roots)assert.ok(!contains(root,canonical),`Runtime preparation is inside system temporary storage: ${canonical}; use a persistent directory before starting writers`);
 return canonical;
}
