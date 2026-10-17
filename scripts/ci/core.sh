#!/usr/bin/env bash
set -euo pipefail
if [[ -n "${LD_PRELOAD:-}${LD_LIBRARY_PATH:-}${DYLD_INSERT_LIBRARIES:-}${DYLD_LIBRARY_PATH:-}" ]]; then
 echo 'Unreviewed dynamic-library environment refused for C++ CI' >&2
 exit 1
fi
build_dir=${1:-build-ci}
mkdir -p ci-reports
git -c safe.directory="$PWD" rev-parse HEAD > ci-reports/source-revision.txt
cmake --version > ci-reports/cmake-version.txt
"${CXX:-c++}" --version > ci-reports/compiler-version.txt
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release 2>&1 | tee ci-reports/configure.log
cmake --build "$build_dir" --parallel 2 --target nano-rpc-state-block-id-tests nano-rpc-get-proof-tests nano-rpc-state-read-cost-tests nano-rpc-tests nano-rpc-simulation-context-tests nano-capella-duty-availability-tests nano nano-tests nano-chain-tests nano-rpc-queue-tests nano-consensus-bls-tests nano-consensus-bls-aggregate-tests nano-execution-checkpoint-tests nano-capella-execution-tests nano-capella-transition-tests nano-local-monotonic-time-tests nano-capella-slashing-fixture nano-capella-slashing-audit nano-capella-deposit-audit nano-capella-paused-capture nano-capella-isolated-bootstrap-audit nano-capella-isolated-stopped-capture 2>&1 | tee ci-reports/build.log
# Reviewed scope. Failure or a missing registration must not silently pass.
pattern='^(core|chain|rpc_queue_expiry|rpc_simulation_context|rpc|rpc_state_block_id|rpc_get_proof|rpc_state_read_cost|ethereum_consensus_bls_verify|ethereum_consensus_bls_fast_aggregate|storage_execution_checkpoint|consensus_capella_payload_paired_state|consensus_capella_signed_transition|consensus_capella_clock_diagnostics|consensus_capella_published_signed_inventory|consensus_capella_publication_cancel_boundary|consensus_local_monotonic_time|consensus_capella_duty_projection_availability|consensus_isolated_stopped_capture_guards)$'
ctest --test-dir "$build_dir" -R "$pattern" --show-only=json-v1 > ci-reports/selection.json
cmake -DREGISTRY="$PWD/ci-reports/selection.json" -P scripts/ci/check-core.cmake > ci-reports/selection-audit.log
# The existing transition tests link the fault-injection library. Preserve its
# bytes across CTest too; a binary hash alone does not identify that dependency.
fault_library=$(find "$build_dir" -maxdepth 1 -type f \( -name 'libnano-test-sync-fault.so' -o -name 'libnano-test-sync-fault.dylib' \) -print | sort)
test -f "$fault_library" # empty or multiple candidates must fail
sha256sum "$fault_library" > ci-reports/ctest-fault-library-before.sha256
ctest --test-dir "$build_dir" -R "$pattern" --output-on-failure --no-tests=error --parallel 1 --output-junit "$PWD/ci-reports/core.xml" 2>&1 | tee ci-reports/core-tests.log
bash scripts/ci/verify-results.sh ci-reports/selection.json ci-reports/core.xml > ci-reports/core-result-audit.json
sha256sum "$fault_library" > ci-reports/ctest-fault-library-after.sha256
cmp ci-reports/ctest-fault-library-before.sha256 ci-reports/ctest-fault-library-after.sha256

# New private helper: compile and exercise its real fixture generation separately
# from the nineteen CTest checks; never count this as a live WAN or penalty test.
"$build_dir/nano-capella-transition-tests" --export-network "$PWD/ci-reports/slashing-fixture" > ci-reports/slashing-export.log
python3 - "$build_dir" <<'PYHELPER'
import json,pathlib,subprocess,sys
root=pathlib.Path('ci-reports/slashing-fixture').resolve()
config=json.loads((root/'node-0.json').read_text())
config['capella']['profile']['file']=str(root/'profile.json')
config['capella']['initial']['file']=str(root/'initial.json')
(root/'native.json').write_text(json.dumps(config,indent=2)+'\n')
binary=str(pathlib.Path(sys.argv[1],'nano-capella-slashing-fixture').resolve())
args=[binary,'--generate',str(root/'genesis.json'),str(root/'native.json'),str(root/'proof.json')]
p=subprocess.run(args,capture_output=True,text=True,timeout=30)
pathlib.Path('ci-reports/slashing-generate.log').write_text(p.stdout+p.stderr)
if p.returncode:raise SystemExit(p.returncode)
proof=(root/'proof.json').read_bytes()
refused=subprocess.run(args,capture_output=True,text=True,timeout=30)
pathlib.Path('ci-reports/slashing-existing-output-refusal.log').write_text(refused.stdout+refused.stderr)
if refused.returncode!=1 or 'fresh output required' not in refused.stderr or (root/'proof.json').read_bytes()!=proof:raise SystemExit('private proof output preservation failed')
# A forged proof must be rejected before opening/creating any database.
audit_binary=str(pathlib.Path(sys.argv[1],'nano-capella-slashing-audit').resolve())
invalid=json.loads(proof)
wire=bytearray.fromhex(invalid['ssz'][2:]);wire[-1]^=1;invalid['ssz']='0x'+wire.hex()
(root/'invalid-proof.json').write_text(json.dumps(invalid))
(root/'unused-request.json').write_text('{}')
never_db=root/'must-not-create-db'
audit=subprocess.run([audit_binary,'--audit',str(root/'genesis.json'),str(root/'native.json'),str(never_db),str(root/'unused-request.json'),str(root/'invalid-proof.json')],capture_output=True,text=True,timeout=30)
pathlib.Path('ci-reports/slashing-audit-invalid-proof.log').write_text(audit.stdout+audit.stderr)
if audit.returncode!=1 or 'expected proof signatures/eligibility invalid' not in audit.stderr or never_db.exists():raise SystemExit('forged proof audit or no-store-creation control failed')
pathlib.Path('ci-reports/slashing-audit-result.json').write_text(json.dumps({'forgedProofRefusalExit':1,'databaseNeverCreated':True,'positiveSanctionQualified':False,'productionQualified':False},indent=2)+'\n')
pathlib.Path('ci-reports/slashing-helper-result.json').write_text(json.dumps({'generatorExit':0,'existingOutputRefusalExit':1,'existingProofUnchanged':True,'realNetworkQualified':False,'productionQualified':False},indent=2)+'\n')
PYHELPER

# Capture must refuse a bootstrap configuration before opening any database.
python3 - "$build_dir" <<'PYCAPTURE'
import pathlib,subprocess,sys,json
root=pathlib.Path('ci-reports/slashing-fixture').resolve()
request=root/'capture-unused-request.json';request.write_text('{}')
never=root/'must-not-create-capture-db'
binary=str(pathlib.Path(sys.argv[1],'nano-capella-paused-capture').resolve())
p=subprocess.run([binary,'--capture',str(root/'genesis.json'),str(root/'native.json'),str(never),str(request)],capture_output=True,text=True,timeout=30)
pathlib.Path('ci-reports/capture-bootstrap-refusal.log').write_text(p.stdout+p.stderr)
if p.returncode!=1 or p.stdout or 'strict witnessed source configuration required' not in p.stderr or never.exists():raise SystemExit('capture bootstrap refusal failed')
pathlib.Path('ci-reports/capture-scope.json').write_text(json.dumps({'bootstrapRefusedBeforeDatabaseCreation':True,'livePausedCaptureQualified':False,'productionQualified':False},indent=2)+'\n')
PYCAPTURE

# Deposit auditor controls are separate from its positive stopped-network audit.
"$build_dir/nano-capella-transition-tests" --export-deposit-network "$PWD/ci-reports/deposit-fixture" > ci-reports/deposit-export.log
python3 - "$build_dir" <<'PYDEPOSIT'
import json,pathlib,subprocess,sys
root=pathlib.Path('ci-reports/deposit-fixture').resolve()
base=json.loads((root/'deposit.json').read_text())
(root/'unused-request.json').write_text('{}')
binary=str(pathlib.Path(sys.argv[1],'nano-capella-deposit-audit').resolve())
results=[]
for name,field,value,expected in [('wrong-index','validatorIndex',65,'deposit fixture identity'),('wrong-key','publicKey','0x'+'00'*48,'known public fixture key65 required'),('wrong-amount','amountGwei',1,'deposit fixture identity')]:
    invalid=dict(base);invalid[field]=value
    proof=root/(name+'.json');proof.write_text(json.dumps(invalid))
    never_db=root/(name+'-never-create-db')
    audit=subprocess.run([binary,'--audit',str(root/'genesis.json'),str(root/'node-0.json'),str(never_db),str(root/'unused-request.json'),str(proof)],capture_output=True,text=True,timeout=30)
    pathlib.Path('ci-reports/deposit-audit-'+name+'.log').write_text(audit.stdout+audit.stderr)
    if audit.returncode!=1 or expected not in audit.stderr or never_db.exists():raise SystemExit('deposit metadata refusal/store-creation control failed: '+name)
    results.append({'case':name,'exit':1,'databaseNeverCreated':True})
pathlib.Path('ci-reports/deposit-audit-controls.json').write_text(json.dumps({'cases':results,'positiveNetworkAuditQualified':False,'productionQualified':False},indent=2)+'\n')
PYDEPOSIT

sha256sum "$fault_library" > ci-reports/ctest-fault-library-final.sha256
cmp ci-reports/ctest-fault-library-before.sha256 ci-reports/ctest-fault-library-final.sha256
sha256sum "$build_dir/nano-rpc-state-block-id-tests" "$build_dir/nano-rpc-get-proof-tests" "$build_dir/nano-rpc-state-read-cost-tests" "$build_dir/nano-rpc-tests" "$build_dir/nano-rpc-simulation-context-tests" "$build_dir/nano-capella-duty-availability-tests" "$build_dir/nano" "$build_dir/nano-tests" "$build_dir/nano-chain-tests" "$build_dir/nano-rpc-queue-tests" "$build_dir/nano-consensus-bls-tests" "$build_dir/nano-consensus-bls-aggregate-tests" "$build_dir/nano-execution-checkpoint-tests" "$build_dir/nano-capella-execution-tests" "$build_dir/nano-capella-transition-tests" "$build_dir/nano-local-monotonic-time-tests" "$build_dir/nano-capella-slashing-fixture" "$build_dir/nano-capella-slashing-audit" "$build_dir/nano-capella-deposit-audit" "$build_dir/nano-capella-paused-capture" "$build_dir/nano-capella-isolated-bootstrap-audit" "$build_dir/nano-capella-isolated-stopped-capture" > ci-reports/executables.sha256
if command -v dpkg-query >/dev/null; then dpkg-query -W > ci-reports/build-packages.txt; fi
