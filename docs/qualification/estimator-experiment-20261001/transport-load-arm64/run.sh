#!/usr/bin/env bash
set -euo pipefail
build=/build/estimator-depth-20261001
mkdir -p "$build/scratch"
export TMPDIR="$build/scratch"


sha256sum "$build/nano" "$build/nano-bft-network-tests" /build/network/nano-p2p > /reports/executables.sha256
ctest --test-dir "$build" -R '^(bft_rpc_cost_websocket_paris_network|bft_rpc_cost_websocket_shanghai_network|bft_rpc_overload_paris_network|bft_rpc_overload_shanghai_network|bft_rpc_default_overload_paris_network|bft_rpc_default_overload_shanghai_network)$' --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$build" -R '^(bft_rpc_cost_websocket_paris_network|bft_rpc_cost_websocket_shanghai_network|bft_rpc_overload_paris_network|bft_rpc_overload_shanghai_network|bft_rpc_default_overload_paris_network|bft_rpc_default_overload_shanghai_network)$' --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
status=${PIPESTATUS[0]}
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
printf '%s\n' "$status" > /reports/ctest-exit.txt
sha256sum "$build/nano" "$build/nano-bft-network-tests" /build/network/nano-p2p > /reports/executables-after.sha256
cmp /reports/executables.sha256 /reports/executables-after.sha256
exit "$status"
