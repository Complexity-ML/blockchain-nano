import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {relocateOwnerRuntimeConfiguration,compareOwnerRecoveryCaptures,runMacOwnerRecoveries,minimumMacOwnerRecoveryMs} from './owner-recovery.mjs';

// Public synthetic contracts only: no FS, process, DB, exporter or private key.
const hash=n=>'0x'+n.toString(16).padStart(64,'0'),publicKey=n=>'0x'+n.toString(16).padStart(96,'0');
const token=n=>({sequence:'0x1',digest:hash(n)}),clone=structuredClone;
function configured(owner='mac-native'){
 const suffix='/isolated-capella-'+'a'.repeat(64)+'/'+owner;
 const roots={oldDataRoot:'/synthetic/source/data'+suffix,oldWitnessRoot:'/synthetic/source/witness'+suffix,dataRoot:'/synthetic/copy/data'+suffix,witnessRoot:'/synthetic/copy/witness'+suffix};
 const first=owner==='mac-native'?0:16;
 const node={rpc_host:'127.0.0.1',rpc_port:18545,listen_addr:'/ip4/127.0.0.1/tcp/30305',p2p_identity:roots.oldDataRoot+'/peer.key',bootnodes:['/ip4/127.0.0.1/tcp/30307/p2p/PUBLIC'],capella:{profile:{file:'/synthetic/config/profile.json',sha256:hash(1)},initial:{file:'/synthetic/config/initial.json'},clock:{maximumForwardStepMs:'0x1'},recoveryWitness:roots.oldWitnessRoot+'/chain-witness',validator:{keys:Array.from({length:16},(_,i)=>({index:'0x'+(first+i).toString(16),keyEnv:'PUBLIC_SYNTHETIC_'+(first+i),feeRecipient:'0x'+'11'.repeat(20),restart:token(first+i+10),journal:roots.oldDataRoot+'/journal-'+(first+i),recoveryWitness:roots.oldWitnessRoot+'/witness-'+(first+i)}))}}};
 return {node,roots};
}
test('owner relocation remaps peer, core witness and all16 journal pairs without changing consensus or floors',()=>{
 for(const owner of ['mac-native','mac-docker']){
  const {node,roots}=configured(owner),before=clone(node),expected=clone(node);
  expected.p2p_identity=roots.dataRoot+'/peer.key';expected.capella.recoveryWitness=roots.witnessRoot+'/chain-witness';
  for(const k of expected.capella.validator.keys){k.journal=roots.dataRoot+k.journal.slice(roots.oldDataRoot.length);k.recoveryWitness=roots.witnessRoot+k.recoveryWitness.slice(roots.oldWitnessRoot.length);}
  const moved=relocateOwnerRuntimeConfiguration(node,roots);assert.deepEqual(moved,expected);assert.deepEqual(node,before);assert.notEqual(moved,node);
  moved.capella.validator.keys[0].restart.sequence='0x2';assert.deepEqual(node,before);
 }
});
test('relocation refuses aliases, overlapping roots, changed domain/owner or an incomplete original journal mapping',()=>{
 const cases=[
  ({roots})=>{roots.dataRoot=roots.oldDataRoot;},
  ({roots})=>{roots.witnessRoot=roots.dataRoot+'/nested';},
  ({roots})=>{roots.dataRoot=roots.dataRoot.replace('isolated-capella-','other-domain-');},
  ({roots})=>{roots.witnessRoot=roots.witnessRoot.replace('/mac-native','/mac-docker');},
  ({roots})=>{roots.oldDataRoot+='/../alias';},
  ({node})=>{node.p2p_identity='/synthetic/foreign/peer.key';},
  ({node})=>{node.capella.recoveryWitness='/synthetic/foreign/chain-witness';},
  ({node})=>{node.capella.validator.keys[0].journal+='/extra';},
  ({node})=>{node.capella.validator.keys[0].recoveryWitness=node.capella.validator.keys[1].recoveryWitness;},
  ({node})=>{node.bellatrix={};}
 ];
 for(const mutate of cases){const f=configured();mutate(f);const before=clone(f);assert.throws(()=>relocateOwnerRuntimeConfiguration(f.node,f.roots),/^Error: Owner recovery: /);assert.deepEqual(f,before);}
});

function capture(owner='mac-native'){
 const first=owner==='mac-native'?0:16;
 return {format:'nano-isolated-stopped-recovery-pins-v1',passed:true,owner,campaignId:'a'.repeat(64),chainId:'0x539',genesisHash:hash(1),genesisValidatorsRoot:hash(2),manifestSha256:'b'.repeat(64),profileSha256:'c'.repeat(64),consensusIdentity:hash(3),forkDigest:'0x00000003',externalStopReceiptSha256:'d'.repeat(64),restart:token(4),recoveryWitnessCommitment:token(5),core:{primary:token(4),witness:token(5),witnessPrevious:null,witnessNext:token(4),snapshot:'PUBLIC synthetic core',witnessSnapshot:'PUBLIC synthetic core intent',restart:token(4),archive:[[hash(6),1,'0x01']]},validatorKeys:Array.from({length:16},(_,i)=>({publicKeyIndex:first+i,index:'0x'+(first+i).toString(16),publicKey:publicKey(first+i+1),primary:token(first+i+20),witness:token(first+i+40),witnessPrevious:null,witnessNext:token(first+i+20),snapshot:'PUBLIC synthetic journal '+(first+i),witnessSnapshot:'PUBLIC synthetic intent '+(first+i),restart:token(first+i+20),retired:false,proposal:null,attestation:null,registeredInCurrentHead:true})),finalized:{epoch:'0x3',root:hash(7)},justifiedEpoch:'0x3',headSlot:25,registryCount:64,primaryWitnessPairs:16,databasesReadOnly:34,externalStopTokensMatched:true,externalWitnessTokensMatched:true,externalWitnessPinOriginExternallyRequired:true,canonicalStoreSnapshotVerified:true,primaryAndWitnessBlobClosuresVerified:true,allObservedFileBytesAndInodesUnchanged:true,newSignaturesCreated:0,journalWritesRequested:false,runtimeStarted:false,productionQualified:false,configurationSha256:'e'.repeat(64),fullFileInodeInventories:[{path:'/synthetic/source/db',inventorySha256:'f'.repeat(64)}],explicitSourcePathMapping:{dataObserved:'/synthetic/source/data',witnessObserved:'/synthetic/source/witness'}};
}
test('capture comparison preserves complete semantic commitments while physical copies have different inventories and config paths',()=>{
 for(const owner of ['mac-native','mac-docker']){
  const source=capture(owner),restored=clone(source),original=clone(source);restored.configurationSha256='1'.repeat(64);restored.fullFileInodeInventories=[{path:'/synthetic/copy/db',inventorySha256:'2'.repeat(64)}];restored.explicitSourcePathMapping={dataObserved:'/synthetic/copy/data',witnessObserved:'/synthetic/copy/witness'};
  assert.deepEqual(compareOwnerRecoveryCaptures(source,restored),{owner,primaryWitnessPairs:16,readOnlyDatabaseRoles:34,sourceAndRestoredCommitmentsEqual:true,externalWitnessTokensMatched:true,newSignaturesCreated:0,productionQualified:false});assert.deepEqual(source,original);
 }
});
test('capture comparison refuses changed tokens, archives, signing history, owner and incomplete or writable role claims',()=>{
 const changes=[x=>{x.core.archive[0][2]='0x02';},x=>{x.core.snapshot+=' changed';},x=>{x.recoveryWitnessCommitment.digest=hash(200);},x=>{x.restart.sequence='0x2';},x=>{x.validatorKeys[0].snapshot+=' changed';},x=>{x.validatorKeys[0].witnessPrevious=token(200);},x=>{x.validatorKeys[0].publicKey=publicKey(200);},x=>{x.validatorKeys.pop();},x=>{x.finalized.root=hash(200);},x=>{x.externalStopReceiptSha256='2'.repeat(64);},x=>{x.owner='mac-docker';},x=>{x.primaryWitnessPairs=15;},x=>{x.databasesReadOnly=33;},x=>{x.externalWitnessTokensMatched=false;},x=>{x.newSignaturesCreated=1;},x=>{x.journalWritesRequested=true;},x=>{x.runtimeStarted=true;},x=>{x.productionQualified=true;}];
 for(const mutate of changes){const source=capture(),restored=clone(source);mutate(restored);assert.throws(()=>compareOwnerRecoveryCaptures(source,restored),/^Error: Owner recovery: /);}
 for(const field of ['core','validatorKeys','campaignId']){const source=capture(),restored=clone(source);delete source[field];delete restored[field];assert.throws(()=>compareOwnerRecoveryCaptures(source,restored),/^Error: Owner recovery: /,'Matching omissions must not count as equal commitments');}
 const correlated=[x=>{x.core=null;},x=>{x.core={};},x=>{x.validatorKeys=[];},x=>{x.validatorKeys[1]=clone(x.validatorKeys[0]);},x=>{delete x.validatorKeys[0].witness;},x=>{delete x.core.snapshot;},x=>{x.core.primary={};},x=>{x.validatorKeys[0].witnessPrevious={};}];
 for(const mutate of correlated){const source=capture();mutate(source);assert.throws(()=>compareOwnerRecoveryCaptures(source,clone(source)),/^Error: Owner recovery: /,'Matching incomplete role bodies must not authenticate the advertised counts');}
 const foreign=capture('vps-docker');assert.throws(()=>compareOwnerRecoveryCaptures(foreign,clone(foreign)),/^Error: Owner recovery: /);
});

function coordinator(){
 const initialFinality={result:{lastCheckpoint:{epoch:'0x1',root:hash(10)}}},events=[],records=[],selected=[],restores=[],finalities=[],proposals=[];
 const duties=['mac-native','mac-docker'].map((owner,i)=>({owner,index:i===0?'0x0':'0x10',publicKey:publicKey(i+1),beforeProposalRoot:hash(100+i)}));
 let checks=0;
 const ops={
  check(){++checks;},
  async selectDuty(owner,cp){events.push('select:'+owner);selected.push({owner,checkpoint:clone(cp)});return duties[selected.length-1];},
  async restore(owner,duty){events.push('restore:'+owner);restores.push({owner,duty});const i=restores.length-1;return {owner,sourceCapture:{path:'/synthetic/source-'+owner+'.json',sha256:'a'.repeat(64),bytes:1},restoredCapture:{path:'/synthetic/restored-'+owner+'.json',sha256:'b'.repeat(64),bytes:1},copiesVerifiedBeforeResume:true,peerIdentityUnchanged:true,finalized:{epoch:i===0?'0x3':'0x8',root:hash(20+i)},cutoffSlot:i===0?'0x20':'0x30'};},
  async finality(cp,label){events.push('finality:'+label);finalities.push({checkpoint:clone(cp),label});const epoch=BigInt(cp.epoch)+3n;return {result:{epochs:3,lastEpoch:'0x'+epoch.toString(16),lastCheckpoint:{epoch:'0x'+epoch.toString(16),root:hash(50+finalities.length)}}};},
  async finalizedProposal(owner,duty,cp,cutoff){events.push('proposal:'+owner);proposals.push({owner,duty,checkpoint:clone(cp),cutoff});assert.equal(typeof cutoff,'bigint');return {root:hash(200+proposals.length),slot:'0x'+(cutoff+1n).toString(16)};},
  async record(row){events.push('record:'+row.owner);records.push(clone(row));}
 };
 return {initialFinality,ops,events,records,selected,restores,finalities,proposals,duties,get checks(){return checks;}};
}
test('coordinator restores Native then DockerMac and binds each new proposal to the same duty beyond its cutoff',async()=>{
 const f=coordinator(),before=clone(f.initialFinality),result=await runMacOwnerRecoveries(f);
 assert.deepEqual(f.events,['select:mac-native','restore:mac-native','finality:owner-recovery-mac-native','proposal:mac-native','record:mac-native','select:mac-docker','restore:mac-docker','finality:owner-recovery-mac-docker','proposal:mac-docker','record:mac-docker']);
 assert.deepEqual(f.selected[0].checkpoint,before.result.lastCheckpoint);assert.deepEqual(f.selected[1].checkpoint,f.proposals[0].checkpoint);
 for(let i=0;i<2;i++){assert.equal(f.restores[i].duty,f.duties[i]);assert.equal(f.proposals[i].duty,f.duties[i]);assert.equal(f.proposals[i].owner,f.duties[i].owner);assert.equal(f.proposals[i].cutoff,i===0?32n:48n);assert.equal(result.records[i].dutyPublicKey,f.duties[i].publicKey);assert.notEqual(result.records[i].beforeProposalRoot,result.records[i].afterProposalRoot);}
 assert.equal(result.passed,true);assert.deepEqual(result.owners,['mac-native','mac-docker']);assert.equal(result.restoresExecuted,2);assert.deepEqual(result.records,f.records);assert.deepEqual(result.finality.result.lastCheckpoint,f.proposals[1].checkpoint);assert.equal(result.runtimeRestorationInferredFromOfflineAudit,false);assert.equal(result.productionQualified,false);assert.deepEqual(f.initialFinality,before);assert.equal(f.checks,12);
});
test('coordinator refuses before later effects when a copy, finality, postcutoff proposal or required capability fails',async()=>{
 const cases=[
  f=>{f.ops.selectDuty=async()=>({...f.duties[0],owner:'vps-docker'});},
  f=>{const restore=f.ops.restore;f.ops.restore=async(...args)=>({...await restore(...args),copiesVerifiedBeforeResume:false});},
  f=>{const restore=f.ops.restore;f.ops.restore=async(...args)=>({...await restore(...args),peerIdentityUnchanged:false});},
  f=>{const finality=f.ops.finality;f.ops.finality=async(...args)=>{const r=await finality(...args);r.result.epochs=2;return r;};},
  f=>{const finality=f.ops.finality;f.ops.finality=async(...args)=>{const r=await finality(...args);r.result.lastEpoch=args[0].epoch;return r;};},
  f=>{f.ops.finalizedProposal=async(owner,duty)=>({root:duty.beforeProposalRoot,slot:'0x21'});},
  f=>{f.ops.finalizedProposal=async(owner,duty,cp,cutoff)=>({root:hash(200),slot:'0x'+cutoff.toString(16)});},
  f=>{delete f.ops.record;}
 ];
 for(const mutate of cases){const f=coordinator();mutate(f);await assert.rejects(runMacOwnerRecoveries(f),/^Error: Owner recovery: /);assert.equal(f.records.length,0);assert(!f.events.some(x=>x.endsWith(':mac-docker')));}
 for(const stage of ['check','restore','record']){const f=coordinator(),failure=Error('injected '+stage);f.ops[stage]=async()=>{throw failure;};if(stage==='check')f.ops.check=()=>{throw failure;};await assert.rejects(runMacOwnerRecoveries(f),error=>error===failure);assert.equal(f.selected.length,stage==='check'?0:1);assert.equal(f.records.length,0);}
});

test('additional Mac recovery budget includes two restores and six full finality phases without changing existing audits',()=>{
 const input={restoreMs:180000,finalityMs:480000,auditMs:180000},before=structuredClone(input);
 assert.equal(minimumMacOwnerRecoveryMs(input),3240000n);assert.deepEqual(input,before);
 assert.equal(minimumMacOwnerRecoveryMs({restoreMs:1,finalityMs:1}),8n);
 for(const value of [undefined,null,{}, {restoreMs:0,finalityMs:1},{restoreMs:1,finalityMs:-1},{restoreMs:1,finalityMs:0.5},{restoreMs:1,finalityMs:Infinity}])assert.throws(()=>minimumMacOwnerRecoveryMs(value),/^Error: Owner recovery: /);
 assert.equal(minimumMacOwnerRecoveryMs({restoreMs:Number.MAX_SAFE_INTEGER,finalityMs:Number.MAX_SAFE_INTEGER}),8n*BigInt(Number.MAX_SAFE_INTEGER));
});

// Exercise the exact new Mac adapter expression from the driver, not a copy of
// its intended implementation. Only a public source file and injected time.
test('actual Mac post-recovery proposal adapter refuses exact or late phase completion even while the global budget remains valid',async()=>{
 const source=await readFile(new URL('./run.mjs',import.meta.url),'utf8'),marker='   finalizedProposal:async(name,duty,checkpoint,cutoff)=>',legacy='   finalizedProposal:(name,duty,checkpoint,cutoff)=>';
 // The legacy spelling makes this test causally reject the uncorrected source.
 const selected=source.includes(marker)?marker:legacy,at=source.indexOf(selected);assert(at>=0&&source.indexOf(selected,at+1)===-1,'One exact new Mac proposal adapter');
 const stop=source.indexOf(',\n   record:',at);assert(stop>at,'Exact following recorder boundary');const expression=source.slice(at+'   finalizedProposal:'.length,stop).trim();
 // These small fixtures exercise the caller, not validateLifecycleMode(). The
 // actual window resolver is covered by lifecycle-proposer-budget.test.mjs.
 // Keep its dependency explicit and require the caller to use its result.
 const windows=[
  {label:'legacy P=F without a campaign budget',format:'nano-fresh-validator-lifecycle-v1',proposerMs:100,campaignBudget:null,phaseEnd:100},
  {label:'P exceeds F',format:'nano-fresh-validator-lifecycle-v2',proposerMs:300,campaignBudget:{workEnd:1000},phaseEnd:300},
  {label:'global work end is shorter than P',format:'nano-fresh-validator-lifecycle-v2',proposerMs:300,campaignBudget:{workEnd:200},phaseEnd:200}
 ];
 for(const window of windows)for(const completion of [window.phaseEnd-1,window.phaseEnd,window.phaseEnd+1]){
  let clock=0,checks=0,calls=0,windowCalls=0;const expected={root:hash(800),slot:'0x21'},duty={index:'0x0'},checkpoint={epoch:'0x1',root:hash(700)},lifecycle={format:window.format,deadlines:{finalityMs:100,...(window.format.endsWith('-v2')?{proposerMs:window.proposerMs}:{})}};
  // The injected check counts both boundaries while the independent overall
  // test limit remains open; the real adapter must enforce its own work cap.
  const dependencies={performance:{now:()=>clock},lifecycleCheck:()=>{++checks;assert(clock<1000,'Independent outer test limit stays valid');},need:(ok,why)=>{if(!ok)throw Error(why);},spec:{lifecycle},campaignBudget:window.campaignBudget,lifecycleProposalMs:value=>{++windowCalls;assert.equal(value,lifecycle);return window.proposerMs;},lifecycleFindFinalized:async(name,cp,cutoff,find,milliseconds,phaseEnd,phaseKind)=>{++calls;assert.equal(name,'mac-native');assert.equal(cp,checkpoint);assert.equal(cutoff,32n);assert.equal(typeof find,'function');assert.equal(milliseconds,window.proposerMs,window.label);assert.equal(phaseEnd,window.phaseEnd,window.label);assert.equal(phaseKind,'proposal');clock=completion;return expected;}};
  const adapter=Function(...Object.keys(dependencies),'return ('+expression+');')(...Object.values(dependencies));
  if(completion<window.phaseEnd)assert.deepEqual(await adapter('mac-native',duty,checkpoint,32n),expected);else await assert.rejects(adapter('mac-native',duty,checkpoint,32n),/Whole owner post-recovery proposal deadline/,window.label);
  assert.equal(windowCalls,1,window.label);assert.equal(calls,1,window.label);assert.equal(checks,2,window.label);
 }
});
