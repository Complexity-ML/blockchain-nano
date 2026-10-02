import {test} from 'node:test';
import assert from 'node:assert/strict';
import {selectBuildPaths} from './build-paths.mjs';
test('legacy read-only volume paths remain available',()=>{
 const p=selectBuildPaths(true,undefined);
 assert.equal(p.nano,'/build/nano');assert.equal(p.revision,'/build/production-source-revision.txt');
});
test('isolated runtime, provenance and rotation auditor share the selected build',()=>{
 const p=selectBuildPaths(true,'production-evm-c10426b2-20261001');
 assert.equal(p.nano,'/build/production-evm-c10426b2-20261001/nano');
 assert.equal(p.revision,p.directory+'/production-source-revision.txt');
 assert.equal(p.rotationAudit,p.directory+'/nano-capella-rotation-audit');
 assert.equal(p.p2p,'/build/network/nano-p2p');
});
test('packaged/default mode cannot silently ignore a requested build',()=>{
 assert.throws(()=>selectBuildPaths(false,'qualified'),/requires build-volume/);
 assert.doesNotThrow(()=>selectBuildPaths(false,undefined));
});
test('escape, whitespace, empty and multiline build selectors are rejected',()=>{
 for(const value of ['', '..', '.hidden', '-option','../old','a/../../old','/build','a\nb','a\rb','a\n','a\r',null,123,'a b','a;id','a$HOME'])assert.throws(()=>selectBuildPaths(true,value),/Invalid/);
});
