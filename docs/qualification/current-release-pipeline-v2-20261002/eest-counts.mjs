import {createReadStream} from 'node:fs';
import {createInterface} from 'node:readline';
import {pathToFileURL} from 'node:url';

// Verify individual reports independently. matched includes validationMatched;
// neither the aggregate alone nor this count proves complete fork compliance.
export function createEestCountsVerifier() {
 let execution=0,validation=0,summary=null;const identities=new Set();
 return {
  consume(row) {
   if(summary)throw Error('Data after terminal EEST summary');
   if(row.summary){summary=row.summary;return;}
   if(row.status!=='matched')throw Error('Non-matching EEST report');
   if(typeof row.file!=='string'||typeof row.case!=='string'||typeof row.fork!=='string'||!Number.isSafeInteger(row.variant)||row.variant<0)throw Error('Invalid EEST case identity');
   const key=JSON.stringify([row.file,row.case,row.fork,row.variant]);if(identities.has(key))throw Error('Duplicate EEST case');identities.add(key);
   if(Object.hasOwn(row,'validation')) {
    if(row.validation?.transactionExecuted!==false||row.validation?.stateUnchanged!==true)throw Error('Matched validation executed or mutated state');
    ++validation;
   }else ++execution;
  },
  finish() {
   if(!summary)throw Error('Missing terminal EEST summary');
   for(const key of ['files','matched','validationMatched','mismatched','unsupported','errors'])if(!Number.isSafeInteger(summary[key])||summary[key]<0)throw Error('Invalid EEST counter: '+key);
   const total=execution+validation;if(!total||!summary.files)throw Error('Empty EEST qualification');
   if(summary.matched!==total||summary.validationMatched!==validation)throw Error('EEST aggregate disagrees with individual reports');
   if(Object.hasOwn(summary,'executionMatched')&&summary.executionMatched!==execution)throw Error('EEST execution counter disagrees with individual reports');
   if(summary.mismatched||summary.unsupported||summary.errors)throw Error('EEST nonzero failure counter');
   return {totalMatched:total,executionMatched:execution,validationMatched:validation,files:summary.files,legacySummary:!Object.hasOwn(summary,'executionMatched'),forkConformanceProven:false};
  },
 };
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href) {
 try {
  if(process.argv.length!==3)throw Error('Usage: node eest-counts.mjs <raw-runner-jsonl>');
  const verifier=createEestCountsVerifier();for await(const line of createInterface({input:createReadStream(process.argv[2]),crlfDelay:Infinity}))if(line.trim())verifier.consume(JSON.parse(line));
  console.log(JSON.stringify(verifier.finish()));
 }catch(error){console.error(error.message);process.exitCode=1;}
}
