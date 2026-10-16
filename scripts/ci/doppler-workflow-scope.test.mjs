import test from 'node:test';
import assert from 'node:assert/strict';
import {coreRequired,networkRequired} from './changes.mjs';
const workflow='.github/workflows/doppler-review.yml';
test('only the exact Doppler read-only workflow is exempt from C++ and network rebuilds',()=>{
 for(const classify of [coreRequired,networkRequired]){
  assert.equal(classify([workflow]),false);
  for(const path of ['.github/workflows/qualification.yml','.github/workflows/other.yml','.github/workflows/doppler-review.yaml','.github/workflows/Doppler-review.yml','.github/workflows/nested/doppler-review.yml',workflow+'.disabled'])assert.equal(classify([path]),true);
  assert.equal(classify([workflow,'src/node.cpp']),true);
  assert.equal(classify([workflow,'include/nano/rpc.hpp']),true);
  assert.throws(()=>classify(['../.github/workflows/doppler-review.yml']));
 }
});
test('Doppler source-only changes do not suppress genuine mixed source and dependency changes',()=>{
 const tools=['tools/doppler-launch/preview.mjs','tools/bridge-source/rpc.mjs',workflow];
 for(const classify of [coreRequired,networkRequired]){
  assert.equal(classify(tools),false);
  for(const path of ['network/main.cpp','dependencies.lock.json','Dockerfile'])assert.equal(classify([...tools,path]),true);
 }
 assert.equal(coreRequired([...tools,'tests/test_core.cpp']),true);
 assert.equal(networkRequired([...tools,'scripts/ci/changes.mjs']),true);
});
