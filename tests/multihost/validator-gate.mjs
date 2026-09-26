import {readFile} from 'node:fs/promises';
import {join} from 'node:path';

// Finality alone can pass with one validator loop permanently stopped.
export async function validateStoppedValidators(directory) {
 const results=[];
 for(const file of ['native.log','docker-mac.log','vps.log']) {
  const statuses=[];
  for(const line of (await readFile(join(directory,file),'utf8')).split('\n')) {
   let event;try{event=JSON.parse(line);}catch{continue;}
   if(event.event==='bellatrix-runtime-stopped')statuses.push(event.validator);
  }
  if(statuses.length!==1)throw Error(`${file}: expected one terminal validator status`);
  const v=statuses[0];
  if(v?.healthy!==true||v.failure!==null)throw Error(`${file}: validator failed: ${v?.failure??'missing health'}`);
  if(!Number.isSafeInteger(v.attestationsAdmitted)||v.attestationsAdmitted<=0)
   throw Error(`${file}: no admitted local attestation`);
  results.push({file,healthy:true,attestationsAdmitted:v.attestationsAdmitted});
 }
 return results;
}
