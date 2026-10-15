#!/usr/bin/env node
// Pure models over a real, separately qualified ROOT-exported NEW-domain fixture.
// This consumer never opens a DB, starts a child, generates keys or signs.
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {join,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
const [fixtureDirectory,bootstrapPath,topologyPath,specPath,admissionsPath,modulePath,legacyPath]=process.argv.slice(2);
if(process.argv.length!==9)throw Error('Usage: fresh-model-fixture-check.mjs exported-fixture bootstrap-plan.json topology.json campaign-spec.json owner-admissions.json prepare.mjs legacy-prepare.mjs');
const sources=new Map(),sha=b=>createHash('sha256').update(b).digest('hex');
async function bytes(path){const name=resolve(path),raw=await readFile(name);assert.ok(raw.length>0&&raw.length<=16*1024*1024,'bounded real fixture input');sources.set(name,sha(raw));return raw;}
const json=async path=>JSON.parse(await bytes(path));
const {buildFreshNodePlan,validateFreshCampaignSpec,validateFreshOwnerAdmission}=await import(pathToFileURL(resolve(modulePath)).href);
const input={domain:await json(join(fixtureDirectory,'isolated-domain.json')),nodes:await Promise.all([0,1].map(i=>json(join(fixtureDirectory,`node-${i}.json`)))),envs:await Promise.all([0,1].map(i=>json(join(fixtureDirectory,`env-${i}.json`)))),bootstrapPlan:await json(bootstrapPath),topology:await json(topologyPath)};
const spec=await json(specPath),admissions=await json(admissionsPath),names=['mac-native','mac-docker','vps-docker'];
const options={target:spec.owners['vps-docker'].host,sourceBaseRevision:spec.sourceBaseRevision};
const inputBefore=structuredClone(input),specBefore=structuredClone(spec),admissionsBefore=structuredClone(admissions);
const started=process.hrtime.bigint();
let groups=0,rejections=0;
function bounded(){assert.ok(process.hrtime.bigint()-started<180000000000n,'pure model duration bound');assert.ok(process.resourceUsage().maxRSS<=131072,'pure model RSS bound');}
function emit(scenario,extra={}){bounded();++groups;console.log(JSON.stringify({scenario,result:'PASS',newSignaturesCreated:0,operationalPrivateInputsRead:false,productionQualified:false,...extra}));}
function rejected(value,alter,call){const modified=structuredClone(value);alter(modified);const before=structuredClone(modified);assert.throws(()=>call(modified));assert.deepEqual(modified,before,'rejected pure input must remain unchanged');++rejections;bounded();}
function key(i,x=input){return x.nodes.flatMap(n=>n.capella.validator.keys).find(k=>k.keyEnv==='NANO_TEST_CAPELLA_NETWORK_KEY_'+i);}
const model=buildFreshNodePlan(input);assert.deepEqual(input,inputBefore);assert.equal(model.nodes.length,3);
assert.deepEqual(model.nodes.map(n=>n.indices.length),[16,16,33]);
const assigned=model.nodes.flatMap(n=>n.indices);assert.equal(assigned.length,65);assert.equal(new Set(assigned).size,65);
for(const n of model.nodes){
 assert.equal(n.node.capella.validator.keys.length,n.signerIdentities.length);
 for(let j=0;j<n.indices.length;j++){
  const i=n.indices[j],k=n.node.capella.validator.keys[j],bound=input.bootstrapPlan.owners.find(x=>x.owner===n.name).keys.find(x=>x.publicKeyIndex===i);
  assert.equal(k.index,i===64?null:'0x'+i.toString(16));assert.deepEqual(k.restart,bound.runtimeKey.restart);
  assert.equal(k.journal,join(n.dataRoot,'journal-'+i));assert.equal(k.recoveryWitness,join(n.witnessRoot,'witness-'+i));
  assert.notEqual(n.dataRoot,n.witnessRoot);assert.deepEqual(n.signerIdentities[j],{keyEnv:k.keyEnv,publicKey:input.domain.publicKeys[i].publicKey});
  const env=input.envs.find(e=>Object.hasOwn(e,k.keyEnv));assert.equal(n.environment[k.keyEnv],env[k.keyEnv]);
 }
}
emit('fresh-model-real-export-and-independent-identities',{owners:3,physicalHosts:2,registeredKeys:64,pendingKeys:1,indicesGuessed:0});
for(const alter of [
 x=>x.domain.format='legacy',x=>x.domain.synthetic=false,x=>x.domain.productionQualified=true,
 x=>x.domain.campaignId='0'.repeat(64),x=>x.bootstrapPlan.genesisValidatorsRoot='0x'+'f'.repeat(64),
 x=>x.bootstrapPlan.executionGenesisHash='0x'+'e'.repeat(64),x=>x.bootstrapPlan.modelOnly=false,
 x=>x.bootstrapPlan.runtimeStarted=true,x=>x.bootstrapPlan.bootstrapDutyPolicy.minimumNewProposalSlot=1,
 x=>x.bootstrapPlan.externalAuditReportsAccepted.blsCryptography=false,
 x=>x.bootstrapPlan.independentWitnessCommitmentsVerified--,
 x=>x.domain.publicKeys[0].publicKey=x.domain.publicKeys[1].publicKey
])rejected(input,alter,buildFreshNodePlan);
emit('fresh-model-domain-and-import-authority-refusals');
for(const alter of [
 x=>{key(64,x).index='0x40';},x=>{key(0,x).index=null;},
 x=>{x.bootstrapPlan.owners[2].keys.at(-1).runtimeKey.index='0x40';},
 x=>{x.bootstrapPlan.owners[2].keys.at(-1).status='registered';},
 x=>{x.bootstrapPlan.owners[0].keys[0].publicKeyIndex=1;},
 x=>{x.bootstrapPlan.owners[0].keys[0].runtimeKey.keyEnv='FOREIGN';},
 x=>{x.bootstrapPlan.owners[0].keys[0].runtimeKey.restart.sequence='0x01';},
 x=>{const k=x.bootstrapPlan.owners[0].keys[0];k.expectedWitnessCommitment=structuredClone(k.runtimeKey.restart);},
 x=>{key(1,x).keyEnv=key(0,x).keyEnv;},x=>{x.envs[0].UNEXPECTED='0x'+'0'.repeat(64);},
 x=>{const k=key(0,x);delete x.envs.find(e=>Object.hasOwn(e,k.keyEnv))[k.keyEnv];},
 x=>{x.topology[0].runtimeDirectory='/fresh/../alias';},
 x=>{x.topology[1].name=x.topology[0].name;},x=>{x.topology[2].host=x.topology[0].host;}
])rejected(input,alter,buildFreshNodePlan);
emit('fresh-model-owner-pending-namespace-and-environment-refusals');
assert.deepEqual(validateFreshCampaignSpec(spec,options),spec);assert.deepEqual(spec,specBefore);
for(const alter of [
 x=>x.extra=true,x=>x.productionQualified=true,x=>x.sourceBaseRevision='0'.repeat(40),
 x=>x.owners['mac-native'].coreBuildRoot=x.owners['mac-native'].networkBuildRoot,
 x=>x.owners['mac-native'].networkBuildRoot=x.owners['mac-native'].coreBuildRoot+'/nested',
 x=>x.owners['mac-docker'].image='latest',x=>x.owners['mac-docker'].programs.nano.path='/outputs/../stale/nano',
 x=>x.owners['vps-docker'].host+='-substituted',x=>x.owners['vps-docker'].uid=0,
 x=>x.owners['mac-native'].programs.nano.bytes=0,x=>x.owners['mac-native'].programs.nano.sha256='a',
 x=>x.fixture.seedFile=x.fixture.ledgerDirectory+'/seed',x=>x.clock.maximumSteps='0x0',x=>x.clock.maximumStartupElapsedMs='0x01'
])rejected(spec,alter,x=>validateFreshCampaignSpec(x,options));
emit('fresh-model-campaign-shape-physical-host-and-build-alias-refusals');
for(const name of names){
 assert.deepEqual(validateFreshOwnerAdmission(admissions[name],spec,name),admissions[name]);
 for(const alter of [
  x=>x.extra=true,x=>x.owner='FOREIGN',x=>x.sourceBaseRevision='0'.repeat(40),x=>x.overlayManifestSha256='f'.repeat(64),
  x=>x.coreObjects=139,x=>x.canonicalCmake=false,x=>x.currentCompilerMmdArchiveClosureVerified=false,
  x=>x.sdkAuthority='unqualified',x=>x.actualLinksExact=false,x=>x.actualBuildExitCode=1,x=>x.productionQualified=true,
  x=>x.coreArchive.sha256='0'.repeat(64),x=>x.programs.nano.bytes++
 ])rejected(admissions[name],alter,x=>validateFreshOwnerAdmission(x,spec,name));
}
assert.deepEqual(admissions,admissionsBefore);
emit('fresh-model-current-root-admission-and-artifact-substitution-refusals',{ownersChecked:3});
const actual=(await bytes(modulePath)).toString(),legacy=(await bytes(legacyPath)).toString(),marker='const [input,output,topologyFile]=process.argv.slice(2);';
const body=legacy.slice(legacy.indexOf(marker));assert.ok(legacy.includes(marker));
const start=actual.indexOf(marker),end=actual.indexOf('\n}\n\nconst ownerNames',start);assert.ok(start>=0&&end>start);
assert.equal(actual.slice(start,start+body.length),body,'legacy executable body must remain byte exact');
assert.equal(actual.slice(start+body.length,end).trim(),'','only wrapper whitespace may follow legacy body');
assert.deepEqual(input,inputBefore);assert.deepEqual(spec,specBefore);
for(const [path,expected] of sources)assert.equal(sha(await readFile(path)),expected,'all real fixture/control input bytes unchanged');
emit('fresh-model-legacy-body-and-all-inputs-conservation',{inputsChecked:sources.size});
assert.equal(groups,6);assert.equal(rejections,79);
console.log(JSON.stringify({scenario:'fresh-model-summary',result:'PASS',groups,rows:7,rejections,maxRssKiB:process.resourceUsage().maxRSS,elapsedMs:Number((process.hrtime.bigint()-started)/1000000n),newSignaturesCreated:0,dbOpens:0,childrenStarted:0,operationalPrivateInputsRead:false,productionQualified:false}));
