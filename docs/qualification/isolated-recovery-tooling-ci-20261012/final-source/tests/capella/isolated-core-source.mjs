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
