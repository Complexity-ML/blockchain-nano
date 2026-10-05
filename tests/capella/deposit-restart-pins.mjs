import assert from 'node:assert/strict';
const pin=t=>{assert.ok(t&&typeof t==='object'&&Object.keys(t).length===2);assert.match(t.sequence,/^0x[1-9a-f][0-9a-f]*$/);assert.match(t.digest,/^0x[0-9a-f]{64}$/);return structuredClone(t);};
export function pinnedDepositRestart(configuration,receipt,pendingPublicKey){
 assert.match(pendingPublicKey,/^0x[0-9a-f]{96}$/);
 assert.equal(receipt.clockHealthy,true);assert.equal(receipt.validator?.healthy,true);assert.equal(receipt.validator.failure,null);
 const c=structuredClone(configuration),keys=c.capella.validator.keys,pins=receipt.validatorKeys;
 assert.equal(receipt.validator.localKeys,keys.length);assert.equal(pins.length,keys.length);
 const identity=x=>x.index===null?(assert.equal(x.publicKey,pendingPublicKey),'key:'+x.publicKey):'index:'+BigInt(x.index).toString();
 const inventory=new Map();for(const p of pins){const id=identity(p);assert.ok(!inventory.has(id),'Duplicate signer receipt');inventory.set(id,pin(p.restart));}
 for(const k of keys){const id=k.index===null?'key:'+pendingPublicKey:'index:'+BigInt(k.index).toString();assert.ok(inventory.has(id),'Missing signer commitment');k.restart=inventory.get(id);inventory.delete(id);}
 assert.equal(inventory.size,0);c.capella.restart=pin(receipt.restart);c.capella.automaticRecovery=false;
 return c;
}
