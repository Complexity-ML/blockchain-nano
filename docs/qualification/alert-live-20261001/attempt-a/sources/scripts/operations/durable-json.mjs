import {open,rename,unlink} from 'node:fs/promises';
import {dirname} from 'node:path';
// Persist before acknowledging an external effect. Callers hold their own lock.
export async function durableJson(file,value){
 const temp=file+'.'+process.pid+'.tmp';let handle,created=false;
 try{handle=await open(temp,'wx',0o600);created=true;await handle.writeFile(JSON.stringify(value)+'\n');await handle.sync();await handle.close();handle=undefined;await rename(temp,file);created=false;const directory=await open(dirname(file),'r');try{await directory.sync();}finally{await directory.close();}}
 finally{if(handle)await handle.close();if(created)await unlink(temp).catch(()=>{});}
}
