import {createReadStream} from 'node:fs';
import {createInterface} from 'node:readline';
import {pathToFileURL} from 'node:url';
import {createEestCountsVerifier} from './eest-counts.mjs';

// LastTest.log preserves full output; JUnit system-out may truncate JSONL.
export async function auditEestCtest(lines) {
 const verifier=createEestCountsVerifier();let active=false,found=false,ended=false;
 for await(const line of lines) {
  if(/^\d+\/\d+ Testing: ethereum_official_regressions$/.test(line)) {
   if(found)throw Error('Duplicate EEST CTest block');
   found=true;active=true;continue;
  }
  if(!active)continue;
  if(line.startsWith('Test time =')) {active=false;ended=true;continue;}
  if(/^\d+\/\d+ Testing:/.test(line))throw Error('Unterminated EEST CTest block');
  if(line.startsWith('{'))verifier.consume(JSON.parse(line));
 }
 if(!found||!ended)throw Error('Missing or incomplete EEST CTest output');
 return verifier.finish();
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href) {
 try {
  if(process.argv.length!==3)throw Error('Usage: node audit-eest-ctest.mjs <LastTest.log>');
  console.log(JSON.stringify(await auditEestCtest(createInterface({input:createReadStream(process.argv[2]),crlfDelay:Infinity}))));
 }catch(error){console.error(error.message);process.exitCode=1;}
}
