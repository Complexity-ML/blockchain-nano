import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import assert from 'node:assert/strict';
const dir=process.argv[2],read=n=>readFileSync(resolve(dir,n),'utf8');
const base=read('tests-full.log'),admission=read('admission-full.log');
assert.equal((base.match(/Test Passed\./g)||[]).length,4);assert.equal((admission.match(/Test Passed\./g)||[]).length,2);
const rows=t=>t.split('\n').filter(l=>l.startsWith('{')).map(l=>JSON.parse(l));
const p=rows(base).find(r=>r.scenario==='staking-pagination');assert(p);assert.equal(p.registeredBySignedTransactions,17);assert.deepEqual(p.pageSizes,[1,2,8,17,32]);assert.equal(p.invalidPagesRejected,6);
for(const k of ['identicalRegistry','engineGenesisAndReopen','malformedCountRejected'])assert.equal(p[k],true);
assert.equal(read('page-config.exit').trim(),'0');
const config=rows(read('page-config.log')).find(r=>r.scenario==='staking-pagination');assert(config);assert.equal(config.invalidPageConfigurationsRejected,4);assert.equal(config.invalidPagesRejected,6);
const results=rows(admission).filter(r=>r.scenario==='validator-open-admission');assert.equal(results.length,2);assert.deepEqual(results.map(r=>r.fork).sort(),['Paris','Shanghai']);
for(const r of results){assert.equal(r.pageSize,64);assert.equal(r.signedRegistrations,1001);assert.equal(r.finalValidators,1001);assert.equal(r.certificateVotes,1001);assert.equal(r.networkProcesses,0);for(const k of ['reopenedAudit','rpcCertificateAccepted','newValidatorDecisive'])assert.equal(r[k],true);}
console.log(JSON.stringify({verified:true,pagination:p,admission:results,scope:'Versioned paginated staking sync. Exact legacy equivalence across five page sizes and malformed-page rejection. 1001 signed registrations with BFT certificates, RPC import and database reopen on Paris/Shanghai. Local execution, not 1001 network nodes; complete registry still scanned.'},null,2));
