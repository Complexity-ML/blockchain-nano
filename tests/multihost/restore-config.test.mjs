import test from 'node:test';
import assert from 'node:assert/strict';
import {stoppedReceipt,pinnedRestoreConfiguration,restoreAuditRequest,validateRestoredFinality} from './restore-config.mjs';
const pin=n=>({sequence:'0x'+n.toString(16),digest:'0x'+String(n).repeat(64)});
const receipt=()=>({event:'capella-runtime-stopped',restart:pin(3),clockHealthy:true,finalized:{epoch:'0x2',root:'0x'+'a'.repeat(64)},validator:{healthy:true,failure:null,localKeys:2,attestationsAdmitted:4,proposalsAdmitted:2},validatorKeys:[{index:'0x1',restart:pin(2)},{index:'0x0',restart:pin(1)}]});
const node=()=>({capella:{initial:{file:'/config/initial.json',sha256:'0x'+'b'.repeat(64)},profile:{file:'/config/profile.json',sha256:'0x'+'c'.repeat(64)},limits:{blocks:'0x100'},recoveryWitness:'/data/witness',validator:{keys:[{index:'0x0',journal:'/data/key0',recoveryWitness:'/data/w0',restart:null},{index:'0x1',journal:'/data/key1',recoveryWitness:'/data/w1',restart:null}]}}});
test('independent pins bind by signer identity despite receipt permutation',()=>{const n=node(),r=receipt(),cfg=pinnedRestoreConfiguration(n,r);assert.deepEqual(cfg.capella.validator.keys.map(k=>k.restart),[pin(1),pin(2)]);assert.deepEqual(cfg.capella.recoveryCommitment,pin(3));assert.equal(cfg.capella.initial,null);assert.equal(cfg.capella.automaticRecovery,false);assert.equal(n.capella.validator.keys[0].restart,null);assert.deepEqual(restoreAuditRequest(n,r).anchor,n.capella.initial);});
for(const [name,alter] of [['missing',r=>r.validatorKeys.pop()],['duplicate',r=>r.validatorKeys[1].index='0x1'],['foreign',r=>r.validatorKeys[1].index='0x2'],['malformed',r=>r.validatorKeys[1].restart.sequence='0x01']])test('refuses '+name+' signer pin',()=>{const r=receipt();alter(r);assert.throws(()=>pinnedRestoreConfiguration(node(),r));});
for(const [name,alter] of [['duplicate receipt',text=>text+'\n'+text],['inactive signer',()=>JSON.stringify({...receipt(),validator:{...receipt().validator,attestationsAdmitted:0}})],['unhealthy clock',()=>JSON.stringify({...receipt(),clockHealthy:false})]])test('refuses '+name,()=>assert.throws(()=>stoppedReceipt(alter(JSON.stringify(receipt())))));
test('three common advances plus fresh duties required',()=>{const r=receipt(),terminal={...receipt(),finalized:{epoch:'0x5',root:r.finalized.root}},result={epochs:3,firstEpoch:'2',lastEpoch:'5',lastCheckpoint:{epoch:'5',root:r.finalized.root}};validateRestoredFinality(r,result,terminal);assert.throws(()=>validateRestoredFinality(r,{...result,lastEpoch:'4'},terminal));assert.throws(()=>validateRestoredFinality(r,result,{...terminal,validator:{...terminal.validator,proposalsAdmitted:0}}));assert.throws(()=>validateRestoredFinality(r,{...result,firstEpoch:'1'},terminal));});

const completion=()=>({saved:receipt(),result:{epochs:3,firstEpoch:'2',lastEpoch:'5',lastCheckpoint:{epoch:'5',root:'0x'+'d'.repeat(64)}},terminal:{...receipt(),finalized:{epoch:'0x5',root:'0x'+'d'.repeat(64)}}});
for(const value of [undefined,null,NaN,Infinity,0.5,-1,0,Number.MAX_SAFE_INTEGER+1,'1'])test('terminal attestations must be a positive safe integer: '+String(value),()=>{const f=completion();f.terminal.validator.attestationsAdmitted=value;assert.throws(()=>validateRestoredFinality(f.saved,f.result,f.terminal),/resume healthy/);});
test('same-epoch terminal root must match the independently observed checkpoint',()=>{const f=completion();f.terminal.finalized.root='0x'+'e'.repeat(64);assert.throws(()=>validateRestoredFinality(f.saved,f.result,f.terminal),/conflicts/);});
for(const [name,alter] of [['missing checkpoint',f=>delete f.result.lastCheckpoint],['missing terminal root',f=>delete f.terminal.finalized.root],['different checkpoint epoch',f=>f.result.lastCheckpoint.epoch='6'],['noncanonical result epoch',f=>f.result.lastEpoch='05']])test('refuses '+name,()=>{const f=completion();alter(f);assert.throws(()=>validateRestoredFinality(f.saved,f.result,f.terminal),/malformed restored finality/);});
test('matching terminal checkpoint accepts safe fresh duties; later epoch retains audit requirement',()=>{const f=completion();validateRestoredFinality(f.saved,f.result,f.terminal);f.terminal.finalized.epoch='0x6';validateRestoredFinality(f.saved,f.result,f.terminal);});

// Initial offline restore must use the independently stopped finality before duties.
test('restore audit binds the stopped finalized checkpoint without aliasing it',()=>{
 const r=receipt(),request=restoreAuditRequest(node(),r);
 assert.deepEqual(request.requiredFinalized,r.finalized);
 r.finalized.root='0x'+'e'.repeat(64);
 assert.equal(request.requiredFinalized.root,'0x'+'a'.repeat(64));
});
for(const [name,alter] of [
 ['absent finality',r=>delete r.finalized],
 ['leading-zero epoch',r=>r.finalized.epoch='0x02'],
 ['decimal epoch',r=>r.finalized.epoch='2'],
 ['short root',r=>r.finalized.root='0xaa'],
 ['absent root',r=>delete r.finalized.root]
])test('restore audit refuses '+name,()=>{const r=receipt();alter(r);assert.throws(()=>restoreAuditRequest(node(),r),/stopped finalized checkpoint/);});

const publicKey=n=>'0x'+n.repeat(96);
function automaticFixture(){
 const n=node(),r=receipt();
 n.capella.validator.keys=[
  {...n.capella.validator.keys[0],index:null,keyEnv:'FRESH_PENDING_ALPHA'},
  {...n.capella.validator.keys[1],index:null,keyEnv:'FRESH_PENDING_BETA'}
 ];
 const identities=[{keyEnv:'FRESH_PENDING_ALPHA',publicKey:publicKey('a')},{keyEnv:'FRESH_PENDING_BETA',publicKey:publicKey('b')}];
 r.validatorKeys=[{index:null,publicKey:publicKey('b'),restart:pin(2)},{index:null,publicKey:publicKey('a'),restart:pin(1)}];
 return {n,r,identities};
}
test('automatic signers bind independent public keys without resolving or inventing indices',()=>{
 const {n,r,identities}=automaticFixture(),before=structuredClone({n,r,identities});
 const restored=pinnedRestoreConfiguration(n,r,identities.slice().reverse());
 assert.deepEqual(restored.capella.validator.keys.map(k=>k.index),[null,null]);
 assert.deepEqual(restored.capella.validator.keys.map(k=>k.restart),[pin(1),pin(2)]);
 assert.equal(restored.capella.validator.keys[0].publicKey,undefined);
 assert.deepEqual({n,r,identities},before);
 restored.capella.validator.keys[0].restart.digest='0x'+'f'.repeat(64);
 assert.deepEqual(r,before.r);
});
test('mixed indexed and automatic receipt preserves each configured identity and paths',()=>{
 const {n,r,identities}=automaticFixture();
 n.capella.validator.keys[0].index='0x0';r.validatorKeys[1]={index:'0x0',restart:pin(1)};
 const restored=pinnedRestoreConfiguration(n,r,identities);
 assert.deepEqual(restored.capella.validator.keys.map(k=>k.index),['0x0',null]);
 assert.deepEqual(restored.capella.validator.keys.map(k=>k.restart),[pin(1),pin(2)]);
 assert.equal(restored.capella.validator.keys[1].recoveryWitness,n.capella.validator.keys[1].recoveryWitness);
});
for(const [name,alter] of [
 ['missing independent authority',f=>delete f.identities],
 ['missing identity row',f=>f.identities.pop()],
 ['wrong independent public key',f=>f.identities[0].publicKey=publicKey('c')],
 ['duplicate independent public key',f=>f.identities[1].publicKey=f.identities[0].publicKey],
 ['duplicate independent environment',f=>f.identities[1].keyEnv=f.identities[0].keyEnv],
 ['foreign environment',f=>f.identities[0].keyEnv='UNKNOWN'],
 ['malformed public key',f=>f.r.validatorKeys[0].publicKey='0x01'],
 ['duplicate automatic receipt',f=>f.r.validatorKeys[0].publicKey=f.r.validatorKeys[1].publicKey],
 ['substituted receipt public key',f=>f.r.validatorKeys[0].publicKey=publicKey('d')],
 ['guessed resolved index',f=>f.r.validatorKeys[0]={index:'0x40',restart:pin(2)}],
 ['missing receipt public key',f=>delete f.r.validatorKeys[0].publicKey],
 ['null substituted into indexed identity',f=>{f.n.capella.validator.keys[0].index='0x0';}],
 ['extra authority field',f=>f.identities[0].index='0x40'],
 ['duplicate configured identity',f=>f.n.capella.validator.keys[1].keyEnv=f.n.capella.validator.keys[0].keyEnv]
])test('automatic restore refuses '+name,()=>{
 const f=automaticFixture();alter(f);const before=structuredClone(f);
 assert.throws(()=>pinnedRestoreConfiguration(f.n,f.r,f.identities));assert.deepEqual(f,before);
});
