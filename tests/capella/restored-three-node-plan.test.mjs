import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {restoredThreeNodePlan} from './restored-three-node-plan.mjs';
const fixture=JSON.parse(await readFile(new URL('../../docs/qualification/restored-three-node-prepare-20261005/input.json',import.meta.url),'utf8'));
test('restored 65-member inventory has disjoint 16/16/33 ownership and exact retained pins',()=>{
 const input=structuredClone(fixture),before=structuredClone(input),p=restoredThreeNodePlan(input);
 assert.deepEqual(input,before);assert.deepEqual(Object.values(p.ownership).map(v=>v.length),[16,16,33]);
 assert.deepEqual(p.sourceCheckpoints,{mac:fixture.macCapsule.restart,vps:fixture.vpsCapsule.restart});
 for(const [i,c] of p.configs.entries()){assert.equal(c.capella.initial,null);assert.equal(c.capella.automaticRecovery,false);const capsule=i<2?fixture.macCapsule:fixture.vpsCapsule;assert.deepEqual(c.capella.recoveryCommitment,capsule.restart);for(const key of c.capella.validator.keys){const pin=capsule.validatorKeys.find(k=>k.index===key.index);assert.deepEqual(key.restart,pin.restart);}}
 assert.equal(p.keysLoaded,false);assert.equal(p.peerIdentityCopyPermitted,false);assert.equal(p.networkQualified,false);
});
for(const [name,change] of [
 ['live source',x=>x.writersStopped=false],
 ['retired signer',x=>x.macCapsule.validatorKeys[0].retired=true],
 ['missing signer witness pin',x=>x.macCapsule.validatorKeys.pop()],
 ['overlapping owners',x=>x.vpsConfig.capella.validator.keys[0].index='0x0'],
 ['foreign pending identity',x=>x.pendingPublicKey='0x'+'11'.repeat(48)],
 ['journal and witness alias',x=>x.macConfig.capella.validator.keys[0].recoveryWitness=x.macConfig.capella.validator.keys[0].journal],
 ['foreign consensus profile',x=>x.vpsConfig.capella.profile.sha256='0x'+'11'.repeat(32)],
 ['conflicting finality',x=>x.vpsCapsule.canonicalReplay.finalized.root='0x'+'11'.repeat(32)]
])test('refuses '+name,()=>{const x=structuredClone(fixture);change(x);assert.throws(()=>restoredThreeNodePlan(x));});

test('separate native and Docker audited snapshots retain their independent core and signer pins',()=>{
 const x=structuredClone(fixture);x.macNativeConfig=structuredClone(x.macConfig);x.macNativeCapsule=structuredClone(x.macCapsule);
 x.macNativeCapsule.restart={sequence:'0x2000',digest:'0x'+'a1'.repeat(32)};x.macNativeCapsule.validatorKeys[0].restart={sequence:'0x100',digest:'0x'+'b2'.repeat(32)};
 const p=restoredThreeNodePlan(x);assert.deepEqual(p.configs[0].capella.recoveryCommitment,x.macNativeCapsule.restart);assert.deepEqual(p.configs[1].capella.recoveryCommitment,x.macCapsule.restart);
 assert.deepEqual(p.configs[0].capella.validator.keys[0].restart,x.macNativeCapsule.validatorKeys[0].restart);assert.deepEqual(p.sourceCheckpoints.macNative,x.macNativeCapsule.restart);
});
for(const [name,change] of [
 ['missing independent capsule',x=>delete x.macNativeCapsule],
 ['retired independent signer',x=>x.macNativeCapsule.validatorKeys[0].retired=true],
 ['independent profile mismatch',x=>x.macNativeConfig.capella.profile.sha256='0x'+'ab'.repeat(32)],
 ['independent finalized root mismatch',x=>x.macNativeCapsule.canonicalReplay.finalized.root='0x'+'ab'.repeat(32)]
])test('rejects '+name,()=>{const x=structuredClone(fixture);x.macNativeConfig=structuredClone(x.macConfig);x.macNativeCapsule=structuredClone(x.macCapsule);change(x);assert.throws(()=>restoredThreeNodePlan(x));});


test('explicit retention experiment preserves source evidence and exact pins, changing only local block capacity',()=>{
 const x=structuredClone(fixture);x.retentionExperiment={blocks:'0x400',rationale:'Bounded recovery experiment; old saturation criterion stays open.'};const before=structuredClone(x);
 const base=restoredThreeNodePlan(fixture),p=restoredThreeNodePlan(x);
 assert.deepEqual(x,before);assert.deepEqual(p.sourceCheckpoints,base.sourceCheckpoints);assert.deepEqual(p.baseline,base.baseline);
 for(let n=0;n<3;n++){const c=structuredClone(p.configs[n]);assert.equal(c.capella.limits.blocks,'0x400');c.capella.limits.blocks=base.configs[n].capella.limits.blocks;assert.deepEqual(c,base.configs[n]);}
 assert.deepEqual(p.retentionBudget.sourceLimits,fixture.macConfig.capella.limits);assert.equal(p.retentionBudget.oldBoundaryRecoveryQualified,false);assert.equal(p.productionQualified,false);
});
for(const [name,change] of [
 ['same budget',x=>x.retentionExperiment.blocks=x.macConfig.capella.limits.blocks],
 ['smaller budget',x=>x.retentionExperiment.blocks='0x1'],
 ['beyond capture bound',x=>x.retentionExperiment.blocks='0x401'],
 ['noncanonical budget',x=>x.retentionExperiment.blocks='0x0400'],
 ['unbounded budget',x=>x.retentionExperiment.blocks='0xffffffffffffffffffffffff'],
 ['missing rationale',x=>delete x.retentionExperiment.rationale],
 ['fee or consensus override',x=>x.retentionExperiment.profile='changed'],
 ['unequal source limits',x=>x.vpsConfig.capella.limits.blocks='0x200'],
 ['foreign source profile',x=>x.vpsConfig.capella.profile.sha256='0x'+'ab'.repeat(32)],
 ['live source despite experiment',x=>x.writersStopped=false]
])test('retention experiment refuses '+name,()=>{const x=structuredClone(fixture);x.retentionExperiment={blocks:'0x400',rationale:'Bounded recovery experiment; old saturation criterion stays open.'};change(x);assert.throws(()=>restoredThreeNodePlan(x));});


test('explicit cold startup plan preserves independent source commitments and signer pins',()=>{
 const x=structuredClone(fixture);x.coldCheckpoint={cacheRecords:'0x2',cacheEncodedBytes:'0x10000'};const before=structuredClone(x);
 const baseline=restoredThreeNodePlan(fixture),p=restoredThreeNodePlan(x);
 assert.deepEqual(x,before);assert.deepEqual(p.sourceCheckpoints,baseline.sourceCheckpoints);assert.deepEqual(p.baseline,baseline.baseline);
 for(let n=0;n<3;n++){const config=structuredClone(p.configs[n]);assert.deepEqual(config.capella.coldCheckpoint,x.coldCheckpoint);delete config.capella.coldCheckpoint;assert.deepEqual(config,baseline.configs[n]);}
 assert.equal(p.coldCheckpointPlan.startupMigrationPerformed,false);assert.equal(p.coldCheckpointPlan.physicalRecoveryQualified,false);assert.equal(p.networkQualified,false);
});
for(const [name,options] of [
 ['zero records',{cacheRecords:'0x0',cacheEncodedBytes:'0x10000'}],
 ['noncanonical records',{cacheRecords:'0x02',cacheEncodedBytes:'0x10000'}],
 ['numeric records',{cacheRecords:2,cacheEncodedBytes:'0x10000'}],
 ['zero bytes',{cacheRecords:'0x2',cacheEncodedBytes:'0x0'}],
 ['noncanonical bytes',{cacheRecords:'0x2',cacheEncodedBytes:'0x010000'}],
 ['huge records',{cacheRecords:'0x10000000000000000',cacheEncodedBytes:'0x10000'}],
 ['huge bytes',{cacheRecords:'0x2',cacheEncodedBytes:'0x10000000000000000'}],
 ['extra field',{cacheRecords:'0x2',cacheEncodedBytes:'0x10000',other:true}],
 ['missing field',{cacheRecords:'0x2'}]
])test('rejects cold plan '+name,()=>{const x=structuredClone(fixture);x.coldCheckpoint=options;assert.throws(()=>restoredThreeNodePlan(x));});
test('cold cache is checked against source budgets without implicit expansion',()=>{
 for(const field of ['cacheRecords','cacheEncodedBytes']){const x=structuredClone(fixture);x.coldCheckpoint={cacheRecords:'0x2',cacheEncodedBytes:'0x10000'};const limit=field==='cacheRecords'?'blocks':'envelopeBytes';x.coldCheckpoint[field]='0x'+(BigInt(x.macConfig.capella.limits[limit])+1n).toString(16);assert.throws(()=>restoredThreeNodePlan(x));}
});
test('cold planner refuses changing existing migrated format options',()=>{
 const x=structuredClone(fixture);x.macConfig.capella.coldCheckpoint={cacheRecords:'0x3',cacheEncodedBytes:'0x10000'};x.coldCheckpoint={cacheRecords:'0x2',cacheEncodedBytes:'0x10000'};assert.throws(()=>restoredThreeNodePlan(x));
});
test('cold planner preserves already matching source options',()=>{
 const x=structuredClone(fixture);x.coldCheckpoint={cacheRecords:'0x2',cacheEncodedBytes:'0x10000'};
 for(const c of [x.macConfig,x.vpsConfig])c.capella.coldCheckpoint=structuredClone(x.coldCheckpoint);
 const before=structuredClone(x),p=restoredThreeNodePlan(x);assert.deepEqual(x,before);assert.deepEqual(p.sourceCheckpoints,{mac:x.macCapsule.restart,vps:x.vpsCapsule.restart});for(const c of p.configs)assert.deepEqual(c.capella.coldCheckpoint,x.coldCheckpoint);
});

for(const blocks of ['0x401','0x1000','0xf4240'])test('explicit capture budget permits local retained graph '+blocks+' without changing pins',()=>{
 const x=structuredClone(fixture);x.retentionExperiment={blocks,catalogRootBytes:'0x'+(32n*(BigInt(blocks)>2048n?BigInt(blocks):2048n)).toString(16),rationale:'Explicit bounded cold metadata experiment; saturation recovery remains open.',captureBudget:{blocks,envelopeBytes:x.macConfig.capella.limits.envelopeBytes}};const before=structuredClone(x),p=restoredThreeNodePlan(x),base=restoredThreeNodePlan(fixture);assert.deepEqual(x,before);assert.deepEqual(p.sourceCheckpoints,base.sourceCheckpoints);assert.deepEqual(p.baseline,base.baseline);assert.deepEqual(p.retentionBudget.captureBudget,x.retentionExperiment.captureBudget);for(const c of p.configs)assert.equal(c.capella.limits.blocks,blocks);assert.equal(p.retentionBudget.oldBoundaryRecoveryQualified,false);
});
for(const [name,change] of [
 ['capture count too small',p=>p.blocks='0x400'],
 ['capture bytes too small',p=>p.envelopeBytes='0x1'],
 ['noncanonical count',p=>p.blocks='0x01000'],
 ['witness overflow',p=>p.envelopeBytes='0xffffffffffffffff'],
 ['runtime overflow',p=>p.blocks='0x10000000000000000'],
 ['unknown consensus override',p=>p.consensus=true],
 ['missing byte budget',p=>delete p.envelopeBytes],
 ['numeric count',p=>p.blocks=4096]
])test('explicit capture policy refuses '+name,()=>{const x=structuredClone(fixture);x.retentionExperiment={blocks:'0x1000',rationale:'Explicit bounded local capture experiment; source commitments retained.',captureBudget:{blocks:'0x1000',envelopeBytes:x.macConfig.capella.limits.envelopeBytes}};change(x.retentionExperiment.captureBudget);assert.throws(()=>restoredThreeNodePlan(x));});
