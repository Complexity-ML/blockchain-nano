import {readFileSync} from 'node:fs';
import assert from 'node:assert/strict';
const text=readFileSync(process.argv[2],'utf8');
const rows=text.split('\n').filter(x=>x.startsWith('{')).map(x=>JSON.parse(x)).filter(x=>x.scenario==='validator-open-admission');
assert.deepEqual(rows.map(x=>x.fork).sort(),['Paris','Shanghai']);
for(const r of rows){
 for(const key of ['signedRegistrations','finalValidators','certificateVotes'])assert.equal(r[key],1001);
 for(const key of ['reopenedAudit','rpcCertificateAccepted','newValidatorDecisive'])assert.equal(r[key],true);
 assert.equal(r.networkProcesses,0);
 const start=text.indexOf(`Testing: validator_open_admission_${r.fork.toLowerCase()}\n`);assert(start>=0);
 const end=text.indexOf('Testing: ',start+9);assert(text.slice(start,end<0?undefined:end).includes('Test Passed.'));
}
console.log(JSON.stringify({verified:true,profiles:['Paris','Shanghai'],validators:1001,decisiveNewValidator:true,rpcCertificate:true,reopenedAudit:true,scope:'Local signed EVM registration and BFT import; not 1001 network processes or production approval.'},null,2));
