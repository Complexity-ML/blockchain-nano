import test from 'node:test';
import assert from 'node:assert/strict';
import {pruningRestoreMode,pruningRestoreConfiguration,verifyPrunedRestoreAudit,validateBackupRestoreMode} from './pruning-restore.mjs';
test('pruning restore requires explicit standalone Capella selection',()=>{
 const valid={profile:'capella',backupRestore:true};
 assert.equal(pruningRestoreMode(undefined,valid),false);assert.equal(pruningRestoreMode('0',valid),false);assert.equal(pruningRestoreMode('1',valid),true);
 for(const value of ['',true,1,'true'])assert.throws(()=>pruningRestoreMode(value,valid));
 for(const options of [{profile:'bellatrix',backupRestore:true},{profile:'capella',backupRestore:false},{profile:'capella'}])assert.throws(()=>pruningRestoreMode('1',options));
});
test('pruning restore changes only a cloned opt-in configuration',()=>{
 const config={capella:{automaticPruning:false,historicalStatePruning:false,validator:{keys:[{index:'0x1'}]}},bootnodes:['peer']};
 assert.equal(pruningRestoreConfiguration(config,false),config);
 const selected=pruningRestoreConfiguration(config,true);assert.deepEqual(selected,{...config,capella:{...config.capella,automaticPruning:true,historicalStatePruning:true}});
 selected.capella.validator.keys[0].index='0x2';assert.equal(config.capella.validator.keys[0].index,'0x1');assert.equal(config.capella.historicalStatePruning,false);
 for(const invalid of [{},{bellatrix:{}},{capella:{},bellatrix:{}}])assert.throws(()=>pruningRestoreConfiguration(invalid,true));
});
test('pruning restore audit requires an actual nonempty signed replay',()=>{
 const good={canonicalHistoryVerified:true,allHistoricalStatesMaterialized:false,prunedExecutionStatesReplayed:55};assert.equal(verifyPrunedRestoreAudit(good),good);
 for(const patch of [{canonicalHistoryVerified:false},{allHistoricalStatesMaterialized:true},{prunedExecutionStatesReplayed:0},{prunedExecutionStatesReplayed:-1},{prunedExecutionStatesReplayed:1.5},{prunedExecutionStatesReplayed:'55'},{prunedExecutionStatesReplayed:Number.MAX_SAFE_INTEGER+1}])assert.throws(()=>verifyPrunedRestoreAudit({...good,...patch}));
 assert.throws(()=>verifyPrunedRestoreAudit({canonicalHistoryVerified:true}));
});

test('pruned restore and bounded load retain standalone artifact and fault guards',()=>{
 const valid={backupRestore:true,profile:'capella',reviewedArtifacts:true,partition:false,asymmetric:false,rotation:false,boundedLoad:true,historicalPruning:true};
 assert.doesNotThrow(()=>validateBackupRestoreMode(valid));
 assert.doesNotThrow(()=>validateBackupRestoreMode({...valid,boundedLoad:false,historicalPruning:false}));
 for(const patch of [{profile:'bellatrix'},{reviewedArtifacts:false},{partition:true},{asymmetric:true},{rotation:true},{historicalPruning:false}])assert.throws(()=>validateBackupRestoreMode({...valid,...patch}));
 assert.doesNotThrow(()=>validateBackupRestoreMode({backupRestore:false}));
});
