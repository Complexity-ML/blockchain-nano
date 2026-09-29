import {test} from 'node:test';
import assert from 'node:assert/strict';
import {auditHistory} from './audit-history.mjs';
const names=['mac-native','mac-docker','vps-docker'];
const cp=(epoch,root='a')=>({epoch:String(epoch),root:'0x'+root.repeat(64)});
const run=[{event:'startup-finality-bound',checkpoint:cp(2)},{event:'partition-observation',rows:names.map(name=>({name,checkpoint:cp(3,'b')}))}];
const samples=names.map(name=>({event:'finality-observation',endpoint:'http://'+name,checkpoint:cp(4,'c')}));
test('consistent checkpoints span all phases',()=>assert.equal(auditHistory(names,run,samples).checked,9));
test('new observer baseline cannot hide a regression',()=>assert.throws(()=>auditHistory(names,run,[{...samples[0],checkpoint:cp(2)},...samples.slice(1)]),/regression/));
test('same epoch conflict at phase boundary is fatal',()=>assert.throws(()=>auditHistory(names,run,[{...samples[0],checkpoint:cp(3,'d')},...samples.slice(1)]),/Conflicting/));
test('raced read does not erase a conflicting root',()=>assert.throws(()=>auditHistory(names,run,[{...samples[0],event:'head-changed-during-finality-read',checkpoint:cp(3,'d')},...samples]),/Conflicting/));
test('incomplete or unknown observers do not pass',()=>{
 assert.throws(()=>auditHistory(names,run,samples.slice(1)),/Missing/);
 assert.throws(()=>auditHistory(names,run,[{...samples[0],endpoint:'http://other'},...samples.slice(1)]),/Unknown/);
 assert.throws(()=>auditHistory(names,[run[0],{...run[1],rows:run[1].rows.slice(1)}],samples),/Incomplete/);
});
test('equivalent epoch encodings cannot hide conflicts',()=>assert.throws(()=>auditHistory(names,run,[{...samples[0],checkpoint:{...cp(3,'d'),epoch:'03'}},...samples.slice(1)]),/Conflicting/));
