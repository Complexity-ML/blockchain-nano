import {test} from 'node:test';
import assert from 'node:assert/strict';
import {advanceRestartConfiguration} from './restart-pins.mjs';
const pin=n=>({sequence:'0x'+n.toString(16),digest:'0x'+String(n).repeat(64)});
const receipt=(n=2)=>({event:'capella-runtime-stopped',restart:pin(n),clockHealthy:true,finalized:{epoch:'0x'+n,root:'0x'+'a'.repeat(64)},validator:{healthy:true,failure:null,localKeys:2,attestationsAdmitted:4,proposalsAdmitted:2},validatorKeys:[{index:'0x0',restart:pin(n)},{index:'0x1',restart:pin(1)}]});
const original=()=>({capella:{initial:{file:'/config/initial.json'},recoveryWitness:'/data/core',validator:{keys:[{index:'0x0',journal:'/data/k0',recoveryWitness:'/data/w0'},{index:'0x1',journal:'/data/k1',recoveryWitness:'/data/w1'}]}}});
const run=(a,b)=>advanceRestartConfiguration(original(),JSON.stringify(a),JSON.stringify(b));
test('advance independent pins by identity while preserving current paths',()=>{const a=receipt(),b=receipt(3);b.validatorKeys.reverse();const r=run(a,b);assert.equal(r.advancedSignerPins,1);assert.deepEqual(r.current,pin(3));assert.equal(r.configuration.capella.validator.keys[0].journal,'/data/k0');assert.equal(r.configuration.capella.automaticRecovery,false);});
test('refuse reused or regressed chain commitment',()=>{const a=receipt();for(const n of [1,2])assert.throws(()=>run(a,receipt(n)),/commitment/);});
test('refuse signer regression and same-sequence digest substitution',()=>{for(const replace of [pin(1),{...pin(2),digest:'0x'+'b'.repeat(64)}]){const b=receipt(3);b.validatorKeys[0].restart=replace;assert.throws(()=>run(receipt(),b),/commitment/);}});
test('refuse absent signer advancement, foreign inventory or unhealthy terminal',()=>{const a=receipt();for(const alter of [b=>b.validatorKeys=a.validatorKeys,b=>b.validatorKeys[0].index='0x2',b=>b.validator.failure='stopped',b=>b.clockHealthy=false]){const b=receipt(3);alter(b);assert.throws(()=>run(a,b));}});
test('refuse conflicting or regressed finality even with newer core and signer pins',()=>{for(const finalized of [{epoch:'0x1',root:'0x'+'a'.repeat(64)},{epoch:'0x2',root:'0x'+'b'.repeat(64)}]){const b=receipt(3);b.finalized=finalized;assert.throws(()=>run(receipt(),b),/checkpoint/);}});

test('automatic signer restart advancement remains distinct by independent environment identity',()=>{
 const n=original(),a=receipt(),b=receipt(3),pub=n=>'0x'+n.repeat(96);
 const identities=[{keyEnv:'AUTO_A',publicKey:pub('a')},{keyEnv:'AUTO_B',publicKey:pub('b')}];
 n.capella.validator.keys.forEach((k,i)=>{k.index=null;k.keyEnv=identities[i].keyEnv;});
 for(const r of [a,b])r.validatorKeys.forEach((k,i)=>{k.index=null;k.publicKey=identities[i].publicKey;});
 b.validatorKeys.reverse();
 const result=advanceRestartConfiguration(n,JSON.stringify(a),JSON.stringify(b),identities);
 assert.equal(result.advancedSignerPins,1);assert.deepEqual(result.configuration.capella.validator.keys.map(k=>k.index),[null,null]);
 b.validatorKeys[0].publicKey=pub('c');
 assert.throws(()=>advanceRestartConfiguration(n,JSON.stringify(a),JSON.stringify(b),identities));
});
