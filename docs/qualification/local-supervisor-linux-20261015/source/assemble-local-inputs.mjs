// ROOT preparation glue only. Pure buffers/objects; no filesystem, seed,
// signing, process, Docker, clock read, ledger reservation or runtime operation.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
const sha=b=>createHash('sha256').update(b).digest('hex');
const bytes=x=>{assert.ok(Buffer.isBuffer(x)&&x.length>0&&x.length<=16*1024*1024);return x;};
const parse=x=>JSON.parse(bytes(x).toString('utf8'));
const encode=x=>Buffer.from(JSON.stringify(x)+'\n');
const hex=x=>'0x'+BigInt(x).toString(16);
const names=['mac-native','mac-docker','vps-docker'];

// The caller separately pins these two existing public modules before import.
// Public audit scope concerns the pure verifier, not the surrounding exporter.
export function assembleLocalInputs(input,{verifyIsolatedFixtureDomain,createIsolatedBootstrapPlan}){
 const {manifestBytes,artifacts,legacy,legacyDescriptor,verifierDescriptor,
  importAuditBytes,journalBindingsBytes,bootstrapInterchangeBytes,
  sourceInventoryBytes,bootstrapWholeInventoryBytes,exportedNodeBytes,exportedEnvironmentBytes,
  voluntaryExitPublicKey,verifiedAtUtc,exportDirectory}=input;
 assert.equal(typeof verifyIsolatedFixtureDomain,'function');assert.equal(typeof createIsolatedBootstrapPlan,'function');
 const m=parse(manifestBytes);assert.equal(m.synthetic,true);assert.equal(m.productionQualified,false);
 assert.match(m.campaignId,/^[0-9a-f]{64}$/);assert.equal(m.registryCount,64);assert.equal(m.publicKeys.length,65);
 assert.ok(artifacts instanceof Map);assert.deepEqual([...artifacts.keys()].sort(),Object.values(m.files).map(d=>d.path).sort());
 for(const d of Object.values(m.files)){const b=bytes(artifacts.get(d.path));assert.equal(b.length,d.bytes);assert.equal(sha(b),d.sha256);}
 const publicResult=verifyIsolatedFixtureDomain({manifest:m,artifacts,legacy});
 assert.equal(typeof verifiedAtUtc,'string');assert.equal(new Date(verifiedAtUtc).toISOString(),verifiedAtUtc);
 assert.equal(typeof exportDirectory,'string');assert.ok(exportDirectory.startsWith('/'));
 assert.equal(verifierDescriptor.sha256,'3e60c538b5eae73ebf8d24af629268d6d74ab29b4c22cfa4b6ceb06e2c5d2dd5');
 const publicAuditBytes=encode({format:'nano-isolated-public-export-audit-v1',verdict:'PASSED_PUBLIC_BINDINGS_ONLY',verifiedAtUtc,
  publicExportDirectory:exportDirectory,verifierSha256:verifierDescriptor.sha256,officialPublicSourceCommit:'194a91fd9e6f97118156421aac4c80e6e47a9a7f',
  publicExportFiles:[{name:'isolated-domain.json',sha256:sha(manifestBytes),bytes:manifestBytes.length},...Object.values(m.files).map(d=>({name:d.path,sha256:d.sha256,bytes:d.bytes}))],
  legacyPublicInputs:[legacyDescriptor],legacyGenesisValidatorsRoot:legacy.genesisValidatorsRoot,result:publicResult,
  scope:Object.fromEntries(['exporterExecuted','signerInvoked','privateArtifactsRead','ledgerRead','networkUsed','blsCryptographyVerified','actualSigningOrderObserved','liveRuntimeVerified','productionQualified'].map(k=>[k,false]))});
 const documents={manifest:manifestBytes,publicAudit:publicAuditBytes,privateAudit:bytes(importAuditBytes),journalBindings:bytes(journalBindingsBytes),initial:artifacts.get('initial.json'),bootstrapInterchange:bytes(bootstrapInterchangeBytes)};
 const evidencePins=Object.fromEntries(Object.entries(documents).map(([k,b])=>[k,sha(b)]));
 const ns='isolated-capella-'+m.campaignId;
 const plan=createIsolatedBootstrapPlan({campaignNamespace:ns,documents,evidencePins,
  assignments:names.map((owner,n)=>({owner,publicKeyIndices:Array.from({length:n===2?33:16},(_,i)=>i+(n===0?0:n===1?16:32))}))});
 const planBytes=encode(plan),inventory=parse(sourceInventoryBytes),whole=parse(bootstrapWholeInventoryBytes);assert.equal(inventory.format,'nano-isolated-bootstrap-file-inventory-v1');assert.equal(inventory.root,'/bootstrap/import');assert.equal(whole.format,inventory.format);assert.equal(whole.root,'/bootstrap');
 const anchorsBytes=encode({format:'nano-isolated-bootstrap-physical-audit-anchors-v1',fixtureManifestSha256:sha(manifestBytes),planDocumentSha256:sha(planBytes),planSha256:plan.planSha256,sourceInventorySha256:sha(sourceInventoryBytes)});
 const node=structuredClone(parse(exportedNodeBytes)),env=parse(exportedEnvironmentBytes),selected=plan.owners[0];
 assert.ok(node.capella&&!node.bellatrix&&!node.altair&&!node.bft);assert.equal(selected.owner,'mac-native');assert.equal(selected.keys.length,16);
 const data='/data/'+ns+'/mac-native',witness='/witness/'+ns+'/mac-native';
 node.rpc_host='127.0.0.1';node.rpc_port=8545;node.listen_addr='/ip4/0.0.0.0/tcp/30303';node.p2p_binary='/network-build/nano-p2p';node.p2p_identity=data+'/peer.key';node.bootnodes=[];
 node.capella.profile.file='/config/profile.json';node.capella.initial.file='/config/initial.json';node.capella.recoveryWitness=witness+'/chain-witness';
 assert.notEqual(node.capella.automaticRecovery,true);assert.equal(node.capella.restart,null);
 const originalKeys=node.capella.validator.keys;assert.equal(originalKeys.length,32);
 const selectedEnvironment={},selectedIdentities=[];
 node.capella.validator.keys=selected.keys.map(b=>{
  const old=originalKeys.filter(k=>k.keyEnv===b.runtimeKey.keyEnv);assert.equal(old.length,1);assert.equal(old[0].index,hex(b.publicKeyIndex));assert.equal(old[0].restart,null);
  assert.equal(b.publicKey,m.publicKeys[b.publicKeyIndex].publicKey);const scalar=env[b.runtimeKey.keyEnv];assert.match(scalar,/^0x[0-9a-f]{64}$/);
  selectedEnvironment[b.runtimeKey.keyEnv]=scalar;selectedIdentities.push({keyEnv:b.runtimeKey.keyEnv,publicKey:b.publicKey});
  return {...b.runtimeKey,journal:data+'/journal-'+b.publicKeyIndex,recoveryWitness:witness+'/witness-'+b.publicKeyIndex};
 });
 assert.equal(Object.keys(selectedEnvironment).length,16);
 const converted=m.bootstrapSigningHistory.blsToExecutionChange.validatorIndex;assert.ok(Number.isSafeInteger(converted)&&converted>=0&&converted<m.publicKeys.length);
 const slashingPublicKey=m.publicKeys[converted].publicKey;
 const find=publicKey=>{const all=selectedIdentities.filter(x=>x.publicKey===publicKey);assert.equal(all.length,1);return {owner:'mac-native',...all[0]};};
 assert.notEqual(voluntaryExitPublicKey,slashingPublicKey);const identities={voluntaryExit:find(voluntaryExitPublicKey),slashing:find(slashingPublicKey)};
 return {publicAuditBytes,planBytes,anchorsBytes,configurationBytes:encode(node),environmentBytes:Buffer.from(Object.entries(selectedEnvironment).map(([k,v])=>k+'='+v).join('\n')+'\n'),
  identities,campaignId:m.campaignId,campaignNamespace:ns,dataRoot:data,witnessRoot:witness,bootstrapInventorySha256:sha(bootstrapWholeInventoryBytes),
  sourcePlannerOwners:3,selectedRuntimeOwnerConfigurations:1,pendingKeyConfigured:false,newSignaturesCreated:0,runtimeStarted:false,productionQualified:false};
}

// Necessary early refusal, not a substitute for the actual runtime clock.
// Invoke in a reviewed successor driver immediately before first Nano spawn.
// Supply initial.clock[1], exact unchanged config/profile values, and a fresh
// injected local wall time. 900 driver seconds never appear in this arithmetic.
export function checkFreshStartupAge({durableTimeMs,capella,slotDurationMs,nowMs}){
 const u64=(1n<<64n)-1n;
 const quantity=s=>{assert.equal(typeof s,'string');assert.match(s,/^0x(?:0|[1-9a-f][0-9a-f]*)$/);const n=BigInt(s);assert.ok(n<=u64);return n;};
 assert.equal(typeof nowMs,'bigint');assert.ok(nowMs>=0n&&nowMs<=u64);
 const from=quantity(durableTimeMs),step=quantity(capella.clock.maximumForwardStepMs),slot=quantity(slotDurationMs);assert.ok(step>0n&&slot>0n&&nowMs>=from);
 const age=nowMs-from;let maximum=step;
 if(capella.clock.startupCatchup!==undefined){
  const c=capella.clock.startupCatchup;assert.deepEqual(Object.keys(c).sort(),['maximumDowntimeMs','maximumSteps']);
  const downtime=quantity(c.maximumDowntimeMs),steps=quantity(c.maximumSteps);assert.ok(downtime>0n&&steps>0n);
  assert.ok((step+slot-1n)/slot<=quantity(capella.limits.replaySlots));maximum=downtime<step*steps?downtime:step*steps;
 }
 assert.ok(age<=maximum,'Fresh startup age exceeds the existing configured clock authority');
 return {format:'nano-local-supervisor-startup-age-preflight-v1',necessaryConditionsPassed:true,
  ageMs:hex(age),configuredMaximumAgeMs:hex(maximum),remainingMs:hex(maximum-age),
  runtimeStartupVerified:false,genesisOrProfileChanged:false,productionQualified:false};
}
