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
