import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {verifyIsolatedCoreCmake,isolatedCoreCmakePins} from './isolated-core-source.mjs';
// Operations uses a shallow checkout: the reviewed public bytes are packaged.
const baseline=readFileSync(new URL('../../docs/qualification/isolated-recovery-tooling-ci-20261012/cmake/source-snapshots/CMakeLists.core85.txt',import.meta.url));
const insertion=[
 'add_executable(nano-capella-isolated-bootstrap-audit tests/capella/isolated_bootstrap_audit.cpp)',
 'target_link_libraries(nano-capella-isolated-bootstrap-audit PRIVATE nano_core)',
 'add_executable(nano-capella-isolated-stopped-capture tests/capella/isolated_stopped_capture.cpp)',
 'target_link_libraries(nano-capella-isolated-stopped-capture PRIVATE nano_core)',
 'add_test(NAME consensus_isolated_stopped_capture_guards COMMAND nano-capella-isolated-stopped-capture --self-test)',
 'set_tests_properties(consensus_isolated_stopped_capture_guards PROPERTIES TIMEOUT 15 LABELS qualification_consensus_recovery_tooling)'
].join('\n')+'\n';
const marker='add_executable(nano-capella-deposit-audit tests/capella/deposit_audit.cpp)';
const current=Buffer.from(baseline.toString().replace(marker,insertion+marker));
test('exact qualified baseline remains admissible without test-target exception',()=>{
 assert.deepEqual(verifyIsolatedCoreCmake(baseline,baseline),{baselineSha256:isolatedCoreCmakePins.baseline,currentSha256:isolatedCoreCmakePins.baseline,reviewedTestTargetsOnly:false});
});
test('only the six reviewed private test lines are admitted for the reused candidate',()=>{
 const report=verifyIsolatedCoreCmake(baseline,current);assert.equal(report.currentSha256,isolatedCoreCmakePins.reviewed);assert.equal(report.insertionSha256,isolatedCoreCmakePins.insertion);assert.equal(report.reviewedTestTargetsOnly,true);
});
test('changing production flags beside the test block is refused',()=>assert.throws(()=>verifyIsolatedCoreCmake(baseline,Buffer.from(current.toString().replace('-Wpedantic','-Wpedantic -ffast-math'))),/Unreviewed CMake change/));
test('duplicating, moving or weakening the reviewed block is refused',()=>{
 for(const text of [current.toString()+insertion,baseline.toString()+insertion,current.toString().replace('PRIVATE nano_core)\nadd_test(NAME consensus_isolated','PUBLIC nano_core)\nadd_test(NAME consensus_isolated')])assert.throws(()=>verifyIsolatedCoreCmake(baseline,Buffer.from(text)),/Unreviewed CMake change/);
});
test('caller cannot select another baseline or supply an unchecked string',()=>{
 assert.throws(()=>verifyIsolatedCoreCmake(Buffer.from(baseline.toString()+'\n'),current),/Unreviewed core CMake baseline/);assert.throws(()=>verifyIsolatedCoreCmake(baseline,current.toString()),/CMake bytes required/);
});
