#!/usr/bin/env bash
set -euo pipefail
build=/build/evm-stack-slots-20261001
mkdir -p "$build/scratch"; export TMPDIR="$build/scratch"
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DMCL_PORTABLE=ON -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 2 --target nano nano-tests nano-rpc-tests nano-http-tests nano-rpc-foreign-admission-tests nano-ethereum-state-runner nano-bn-pairing-tests nano-bft-network-tests nano-oversized-block-tests nano-fragment-network-tests > /reports/build.log 2>&1
cp "$build/third_party/mcl/CMakeFiles/mcl_st.dir/flags.make" /reports/mcl-flags.make
sha256sum "$build/nano" "$build/nano-tests" "$build/nano-rpc-tests" "$build/nano-http-tests" "$build/nano-rpc-foreign-admission-tests" "$build/nano-ethereum-state-runner" "$build/nano-bn-pairing-tests" "$build/nano-bft-network-tests" "$build/nano-oversized-block-tests" "$build/nano-fragment-network-tests" /build/network/nano-p2p > /reports/executables.sha256
pattern='^(core|rpc|http|rpc_foreign_chain_admission_isolation|ethereum_official_regressions|bn_pairing_cold_vectors|oversized_block_(paris|shanghai)|libp2p_fragment_(sync|faults)_(paris|shanghai)|bft_rpc_(cost_many|cost_websocket|overload|default_overload)_(paris|shanghai)_network)$'
ctest --test-dir "$build" -R "$pattern" --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$build" -R "$pattern" --parallel 1 --no-tests=error --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
status=$?
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
printf '%s\n' "$status" > /reports/ctest-exit.txt
sha256sum "$build/nano" "$build/nano-tests" "$build/nano-rpc-tests" "$build/nano-http-tests" "$build/nano-rpc-foreign-admission-tests" "$build/nano-ethereum-state-runner" "$build/nano-bn-pairing-tests" "$build/nano-bft-network-tests" "$build/nano-oversized-block-tests" "$build/nano-fragment-network-tests" /build/network/nano-p2p > /reports/executables-after.sha256
cmp /reports/executables.sha256 /reports/executables-after.sha256 || exit 1
exit "$status"
