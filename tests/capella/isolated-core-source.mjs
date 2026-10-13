import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
const sha=bytes=>createHash('sha256').update(bytes).digest('hex');
// These pins are reviewed campaign constants, not values from its checkout or
// a report supplied by the caller. Production source diff remains strict.
export const isolatedCoreCmakePins=Object.freeze({
 baseline:'7d84ecccddd4112578c20af5f4deed0b819c662dbb9571d39f3bfaf14dbcc0bd',
 reviewed:'20bff51087f0772434eac798fec4bf1eda4ca6b57c4b2b036d4ae2a24310f8f5',
 insertion:'a9de91519d74db4a4a09dc664531fb3b4ab36848285001ec1608801b2d9518a0'
});
const approvedInsertion="add_executable(nano-capella-isolated-bootstrap-audit tests/capella/isolated_bootstrap_audit.cpp)\ntarget_link_libraries(nano-capella-isolated-bootstrap-audit PRIVATE nano_core)\nadd_executable(nano-capella-isolated-stopped-capture tests/capella/isolated_stopped_capture.cpp)\ntarget_link_libraries(nano-capella-isolated-stopped-capture PRIVATE nano_core)\nadd_test(NAME consensus_isolated_stopped_capture_guards COMMAND nano-capella-isolated-stopped-capture --self-test)\nset_tests_properties(consensus_isolated_stopped_capture_guards PROPERTIES TIMEOUT 15 LABELS qualification_consensus_recovery_tooling)\n";
export function verifyIsolatedCoreCmake(baselineBytes,currentBytes){
 assert.ok(Buffer.isBuffer(baselineBytes)&&Buffer.isBuffer(currentBytes),'CMake bytes required');
 const baseline=sha(baselineBytes),current=sha(currentBytes);
 assert.equal(baseline,isolatedCoreCmakePins.baseline,'Unreviewed core CMake baseline');
 if(current===baseline)return {baselineSha256:baseline,currentSha256:current,reviewedTestTargetsOnly:false};
 assert.equal(current,isolatedCoreCmakePins.reviewed,'Unreviewed CMake change');
 assert.equal(sha(approvedInsertion),isolatedCoreCmakePins.insertion);
 const text=currentBytes.toString('utf8'),offset=text.indexOf(approvedInsertion);
 assert.ok(offset>=0&&text.indexOf(approvedInsertion,offset+approvedInsertion.length)===-1,'Unique reviewed test block required');
 assert.ok(Buffer.from(text.slice(0,offset)+text.slice(offset+approvedInsertion.length)).equals(baselineBytes),'CMake implementation outside reviewed block changed');
 return {baselineSha256:baseline,currentSha256:current,insertionSha256:isolatedCoreCmakePins.insertion,reviewedTestTargetsOnly:true};
}

// Candidate variant: independent core85 bytes remain the first argument.
export const candidateCoreCmakePins=Object.freeze({
 baseline:isolatedCoreCmakePins.baseline,
 current:'28fe657bbf1a54ad5663f64024d95f90eeb8a5e5fdd25943f0a684042e66f0ec',
 insertion:'29a260604ce5dd2f4b82d54cf0feee65cc3d87043a7a0c715967a5033a5bba11'
});
const candidateInsertion="add_executable(nano-capella-isolated-bootstrap-audit tests/capella/isolated_bootstrap_audit.cpp)\ntarget_link_libraries(nano-capella-isolated-bootstrap-audit PRIVATE nano_core)\nadd_executable(nano-capella-isolated-stopped-capture tests/capella/isolated_stopped_capture.cpp)\ntarget_link_libraries(nano-capella-isolated-stopped-capture PRIVATE nano_core)\nadd_test(NAME consensus_isolated_stopped_capture_guards COMMAND nano-capella-isolated-stopped-capture --self-test)\nset_tests_properties(consensus_isolated_stopped_capture_guards PROPERTIES TIMEOUT 15 LABELS qualification_consensus_recovery_tooling)\nadd_executable(nano-capella-duty-availability-tests tests/test_capella_duty_availability.cpp)\ntarget_link_libraries(nano-capella-duty-availability-tests PRIVATE nano_core)\ntarget_include_directories(nano-capella-duty-availability-tests PRIVATE \"${CMAKE_CURRENT_SOURCE_DIR}/third_party/ckzg/blst/bindings\")\ntarget_compile_definitions(nano-capella-duty-availability-tests PRIVATE NANO_TEST_SOURCE=\"${CMAKE_CURRENT_SOURCE_DIR}\")\nadd_test(NAME consensus_capella_duty_projection_availability COMMAND nano-capella-duty-availability-tests --self-test)\nset_tests_properties(consensus_capella_duty_projection_availability PROPERTIES TIMEOUT 70 LABELS qualification_consensus_duty_availability)\n";
export function verifyCandidateCoreCmake(baselineBytes,currentBytes){
 assert.ok(Buffer.isBuffer(baselineBytes)&&Buffer.isBuffer(currentBytes),'CMake bytes required');
 assert.equal(sha(baselineBytes),candidateCoreCmakePins.baseline,'Unreviewed independent core85 CMake baseline');
 assert.equal(sha(currentBytes),candidateCoreCmakePins.current,'Unreviewed candidate CMake change');
 assert.equal(sha(candidateInsertion),candidateCoreCmakePins.insertion);
 const text=currentBytes.toString('utf8'),offset=text.indexOf(candidateInsertion);
 assert.ok(offset>=0&&text.indexOf(candidateInsertion,offset+candidateInsertion.length)===-1,'Unique three-target reviewed candidate block required');
 assert.ok(Buffer.from(text.slice(0,offset)+text.slice(offset+candidateInsertion.length)).equals(baselineBytes),'Production CMake outside reviewed twelve lines changed');
 return {baselineSha256:sha(baselineBytes),currentSha256:sha(currentBytes),insertionSha256:sha(candidateInsertion),reviewedTestTargetsOnly:true,reviewedTestTargetCount:3};
}
