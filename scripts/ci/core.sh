#!/usr/bin/env bash
set -euo pipefail
build_dir=${1:-build-ci}
mkdir -p ci-reports
git -c safe.directory="$PWD" rev-parse HEAD > ci-reports/source-revision.txt
cmake --version > ci-reports/cmake-version.txt
"${CXX:-c++}" --version > ci-reports/compiler-version.txt
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release 2>&1 | tee ci-reports/configure.log
cmake --build "$build_dir" --parallel 2 --target nano nano-tests nano-chain-tests nano-rpc-queue-tests nano-consensus-bls-tests nano-consensus-bls-aggregate-tests nano-execution-checkpoint-tests nano-capella-execution-tests nano-capella-transition-tests 2>&1 | tee ci-reports/build.log
# Reviewed scope. Failure or a missing registration must not silently pass.
pattern='^(core|chain|rpc_queue_expiry|ethereum_consensus_bls_verify|ethereum_consensus_bls_fast_aggregate|storage_execution_checkpoint|consensus_capella_payload_paired_state|consensus_capella_signed_transition|consensus_capella_clock_diagnostics)$'
ctest --test-dir "$build_dir" -R "$pattern" --show-only=json-v1 > ci-reports/selection.json
python3 - <<'PY'
import json
expected={'core','chain','rpc_queue_expiry','ethereum_consensus_bls_verify','ethereum_consensus_bls_fast_aggregate','storage_execution_checkpoint','consensus_capella_payload_paired_state','consensus_capella_signed_transition','consensus_capella_clock_diagnostics'}
actual={t['name'] for t in json.load(open('ci-reports/selection.json'))['tests']}
if actual != expected: raise SystemExit('C++ CI registrations mismatch: '+str(actual ^ expected))
PY
ctest --test-dir "$build_dir" -R "$pattern" --output-on-failure --no-tests=error --parallel 1 --output-junit "$PWD/ci-reports/core.xml" 2>&1 | tee ci-reports/core-tests.log

sha256sum "$build_dir/nano" "$build_dir/nano-tests" "$build_dir/nano-chain-tests" "$build_dir/nano-rpc-queue-tests" "$build_dir/nano-consensus-bls-tests" "$build_dir/nano-consensus-bls-aggregate-tests" "$build_dir/nano-execution-checkpoint-tests" "$build_dir/nano-capella-execution-tests" "$build_dir/nano-capella-transition-tests" > ci-reports/executables.sha256
if command -v dpkg-query >/dev/null; then dpkg-query -W > ci-reports/build-packages.txt; fi
