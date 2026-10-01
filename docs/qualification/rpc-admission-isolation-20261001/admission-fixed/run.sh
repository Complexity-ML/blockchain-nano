#!/usr/bin/env bash
set -euo pipefail
build=/build/rpc-positional-client-20261001
mkdir -p "$build/foreign-admission-scratch"
export TMPDIR="$build/foreign-admission-scratch"
cmake --build "$build" --parallel 2 --target nano nano-rpc-tests nano-rpc-foreign-admission-tests nano-bft-network-tests 2>&1 | tee /reports/build.log
sha256sum "$build/nano" "$build/nano-rpc-tests" "$build/nano-rpc-foreign-admission-tests" "$build/nano-bft-network-tests" /build/network/nano-p2p > /reports/executables.sha256
pattern='^(rpc|rpc_foreign_chain_admission_isolation|bft_restart_flood_network)$'
ctest --test-dir "$build" -R "$pattern" --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$build" -R "$pattern" --output-on-failure --no-tests=error --parallel 1 --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
status=${PIPESTATUS[0]}
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
exit "$status"
