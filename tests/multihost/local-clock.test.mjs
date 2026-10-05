import test from 'node:test';
import assert from 'node:assert/strict';
import {monotonicClockConfiguration} from './local-clock.mjs';
const config={capella:{clock:{intervalMs:'0x64',maximumForwardStepMs:'0x1d4c0'},validator:{keys:[{index:'0x1'}]}}};
const profile={format:'nano-capella-profile-v1',common:[{},'0x1770','0x7d0']};
test('clock selection is explicit and changes only a cloned clock configuration',()=>{
 assert.equal(monotonicClockConfiguration(config,undefined,{}),config);
 const result=monotonicClockConfiguration(config,'250',profile);
 assert.deepEqual(result,{...config,capella:{...config.capella,clock:{...config.capella.clock,maximumWallClockDriftMs:'0xfa'}}});
 result.capella.validator.keys[0].index='0x2';assert.equal(config.capella.validator.keys[0].index,'0x1');
});
test('invalid and unsafe drift cannot create qualification configuration',()=>{
 for(const v of ['0','0250','-1','250.0','0xfa','',true,250,'18446744073709551616','2000'])assert.throws(()=>monotonicClockConfiguration(config,v,profile));
 for(const p of [{},{...profile,format:'other'},{...profile,common:[{},'0x1770','0x01']}])assert.throws(()=>monotonicClockConfiguration(config,'250',p));
 assert.throws(()=>monotonicClockConfiguration({...config,bellatrix:{}},'250',profile));
 assert.throws(()=>monotonicClockConfiguration({capella:{clock:{maximumForwardStepMs:'0xfa'}}},'250',profile));
});
