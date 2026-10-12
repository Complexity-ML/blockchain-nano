// Public-only auditor. No seed, scalar, environment capsule, ledger or signer reads.
import assert from 'node:assert/strict';
import {constants} from 'node:fs';
import {open,lstat,realpath} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
const MAX_BYTES=16*1024*1024;
const hash=b=>createHash('sha256').update(b).digest('hex');
const VERIFIER_SHA='3e60c538b5eae73ebf8d24af629268d6d74ab29b4c22cfa4b6ceb06e2c5d2dd5';
const EXPECTED_DOMAIN='0x0a08c27fe4ece2483f9e581f78c66379a06f96e9c24cd1390594ff939b26f95b';
const SOURCE_COMMIT='194a91fd9e6f97118156421aac4c80e6e47a9a7f';
const BASELINES=[
 ['officialManifest','tests/fixtures/consensus-capella-slots-official/manifest.json','4ae10a776342808a5b0eb98dedae873bdd06ef71bfd1acb7117fe4407da2ebc3'],
 ['officialPre','tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz','f5a464175565a8ed6a74790a60b84477c16a5b1bf71ece55736f192142cbca3d'],
 ['legacyProfile','docs/qualification/candidate-images-20260930/profile.json','c9d115373e63eea3332aadd86611d1fe6f9d129d61b80d6af883cf16ad176467'],
 ['legacyDeposit','docs/qualification/native-deposit-network-fixture-20261005/deposit.json','159e9122b09d87bbaae3b55c8249e088ccba9745a595f7f0ae5ae8adb3a739f4'],
];
const PUBLIC_FILES=['genesis-validators.ssz','genesis-state.ssz','genesis.json','profile.json','initial.json'];
const binds=[],baselineBinds=[];
let stage='arguments',receiptPath;
async function canonicalDirectory(input){const absolute=path.resolve(input),canonical=await realpath(absolute);assert.equal(canonical,absolute,'Directory aliases are refused');const info=await lstat(absolute);assert.ok(info.isDirectory()&&!info.isSymbolicLink());return absolute;}
async function readPublic(file){
 assert.equal(await realpath(file),file,'Public file aliases are refused');const info=await lstat(file);assert.ok(info.isFile()&&!info.isSymbolicLink()&&info.size<=MAX_BYTES);
 const fd=await open(file,constants.O_RDONLY|constants.O_NOFOLLOW);try{const before=await fd.stat();assert.ok(before.isFile()&&before.size<=MAX_BYTES&&before.dev===info.dev&&before.ino===info.ino);const bytes=await fd.readFile();const after=await fd.stat();assert.ok(bytes.length===before.size&&after.size===before.size&&after.mtimeMs===before.mtimeMs&&after.ctimeMs===before.ctimeMs);return bytes;}finally{await fd.close();}
}
function parseJson(bytes){
 const text=bytes.toString('utf8');assert.ok(Buffer.from(text,'utf8').equals(bytes));const parsed=JSON.parse(text),tokens=text.match(/"(?:\\.|[^"\\])*"|-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?|true|false|null|[{}\[\]:,]/g);let i=0;
 function value(){const t=tokens[i++];if(t==='{'){const seen=new Set();if(tokens[i]==='}'){i++;return;}for(;;){const key=JSON.parse(tokens[i++]);assert.equal(typeof key,'string');assert.ok(!seen.has(key),'Duplicate JSON field');seen.add(key);assert.equal(tokens[i++],':');value();const next=tokens[i++];if(next==='}')break;assert.equal(next,',');}}else if(t==='['){if(tokens[i]===']'){i++;return;}for(;;){value();const next=tokens[i++];if(next===']')break;assert.equal(next,',');}}}
 value();assert.equal(i,tokens.length);return parsed;
}
async function writeReceipt(value){const fd=await open(receiptPath,constants.O_WRONLY|constants.O_CREAT|constants.O_EXCL|constants.O_NOFOLLOW,0o644);try{await fd.writeFile(JSON.stringify(value,null,2)+'\n');await fd.sync();}finally{await fd.close();}}
async function main(){
 const args=process.argv.slice(2);assert.ok(args.length===2||args.length===3,'Usage: node audit-public-export.mjs PUBLIC_EXPORT_DIR NEW_RECEIPT_JSON [PUBLIC_BASELINE_REPO]');
 const directory=await canonicalDirectory(args[0]),repo=await canonicalDirectory(args[2]??'/Users/boris/Dev/blockchain-nano');
 const output=path.resolve(args[1]),outputParent=await canonicalDirectory(path.dirname(output));assert.equal(path.join(outputParent,path.basename(output)),output);assert.ok(output!==directory&&!output.startsWith(directory+path.sep)&&output!==repo&&!output.startsWith(repo+path.sep),'Receipt must be outside input and repository');receiptPath=output;
 stage='frozen-verifier-source';const verifierPath=fileURLToPath(new URL('./balance-final/isolated-fixture-domain.mjs',import.meta.url)),verifierBytes=await readPublic(verifierPath);assert.equal(hash(verifierBytes),VERIFIER_SHA);const {inspectCapellaState,verifyIsolatedFixtureDomain}=await import(pathToFileURL(verifierPath));
 stage='pinned-public-legacy-inputs';const baseline={};for(const [key,relative,expected] of BASELINES){const file=path.join(repo,relative),data=await readPublic(file),sha256=hash(data);assert.equal(sha256,expected);baseline[key]=data;baselineBinds.push({name:key,path:relative,sha256,bytes:data.length});}
 const official=parseJson(baseline.officialManifest),row=official.cases.find(x=>x.name==='slots_1');assert.equal(official.commit,SOURCE_COMMIT);assert.equal(row.files['pre.ssz'].sha256,hash(baseline.officialPre));assert.equal(row.files['pre.ssz'].bytes,baseline.officialPre.length);
 const previous=inspectCapellaState(baseline.officialPre,parseJson(baseline.legacyProfile)),pending=parseJson(baseline.legacyDeposit);assert.equal(previous.registryRoot,EXPECTED_DOMAIN);assert.equal(previous.genesisValidatorsRoot,EXPECTED_DOMAIN);assert.equal(previous.registryCount,64);assert.equal(pending.validatorIndex,64);
 const legacy={genesisValidatorsRoot:EXPECTED_DOMAIN,publicKeys:[...Array.from({length:64},(_,i)=>'0x'+previous.registry.subarray(i*121,i*121+48).toString('hex')),pending.publicKey],transactionSenderAddress:pending.sender.toLowerCase()};
 stage='exact-six-public-export-files';const artifacts=new Map();for(const name of ['isolated-domain.json',...PUBLIC_FILES]){const data=await readPublic(path.join(directory,name));binds.push({name,sha256:hash(data),bytes:data.length});artifacts.set(name,data);}const manifest=parseJson(artifacts.get('isolated-domain.json'));artifacts.delete('isolated-domain.json');
 stage='independent-public-domain-verification';const result=verifyIsolatedFixtureDomain({manifest,artifacts,legacy});
 stage='public-input-stability';for(const binding of binds)assert.equal(hash(await readPublic(path.join(directory,binding.name))),binding.sha256);for(const binding of baselineBinds)assert.equal(hash(await readPublic(path.join(repo,binding.path))),binding.sha256);
 const report={format:'nano-isolated-public-export-audit-v1',verdict:'PASSED_PUBLIC_BINDINGS_ONLY',verifiedAtUtc:new Date().toISOString(),publicExportDirectory:directory,verifierSha256:VERIFIER_SHA,officialPublicSourceCommit:SOURCE_COMMIT,publicExportFiles:binds,legacyPublicInputs:baselineBinds,legacyGenesisValidatorsRoot:EXPECTED_DOMAIN,result,scope:{exporterExecuted:false,signerInvoked:false,privateArtifactsRead:false,ledgerRead:false,networkUsed:false,blsCryptographyVerified:false,actualSigningOrderObserved:false,liveRuntimeVerified:false,productionQualified:false}};
 stage='receipt-write';await writeReceipt(report);console.log(JSON.stringify({verdict:report.verdict,receipt:receiptPath,campaignId:result.campaignId,genesisValidatorsRoot:result.genesisValidatorsRoot,productionQualified:false}));
}
try{await main();}catch(error){
 // Do not print arbitrary assertion actual values, even if a caller substitutes
 // a private document at an expected public path. Diagnostics contain no payload.
 const source=String(error.stack??'').match(/isolated-fixture-domain\.mjs:[0-9]+:[0-9]+/)?.[0]??null;
 const failure={format:'nano-isolated-public-export-audit-v1',verdict:'FAILED_PUBLIC_AUDIT',stage,errorType:error.name,errorCode:error.code??null,verifierSourceLocation:source,verifierSha256:VERIFIER_SHA,publicExportFiles:binds,legacyPublicInputs:baselineBinds,productionQualified:false};
 if(receiptPath&&stage!=='receipt-write'){try{await writeReceipt(failure);}catch{}}
 console.error(JSON.stringify(failure));process.exitCode=1;
}
