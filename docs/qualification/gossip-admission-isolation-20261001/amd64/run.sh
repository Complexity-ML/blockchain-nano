#!/usr/bin/env bash
set -euo pipefail
build=/build/rpc-admission-amd64-20261001
mkdir -p "$build/scratch"
export TMPDIR="$build/scratch"
sha256sum /work/src/rpc.cpp /work/include/nano/client.hpp /work/tests/test_rpc_foreign_admission.cpp > /reports/source.sha256
cmake --version > /reports/cmake-version.txt
c++ --version > /reports/compiler-version.txt
dpkg-query -W > /reports/build-packages.txt
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p 2>&1 | tee /reports/configure.log
cmake --build "$build" --parallel 2 --target nano nano-rpc-tests nano-rpc-foreign-admission-tests nano-bft-network-tests nano-supervisor-tests nano-rpc-cli-tests nano-staking_cli-tests nano-signed-validator-cli-tests nano-network-tests 2>&1 | tee /reports/build.log
sha256sum "$build/nano" "$build/nano-rpc-tests" "$build/nano-rpc-foreign-admission-tests" "$build/nano-bft-network-tests" "$build/nano-supervisor-tests" "$build/nano-rpc-cli-tests" "$build/nano-staking_cli-tests" "$build/nano-signed-validator-cli-tests" /build/network/nano-p2p > /reports/executables.sha256
pattern='^(rpc|rpc_foreign_chain_admission_isolation|supervisor|rpc_cli|staking_cli|signed_validator_cli|bft_restart_flood_paris_network|bft_restart_flood_shanghai_network|network|signed_network|bft_network)$'
ctest --test-dir "$build" -R "$pattern" --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$build" -R "$pattern" --no-tests=error --output-on-failure --parallel 1 --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
status=${PIPESTATUS[0]}
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
printf '%s\n' "$status" > /reports/ctest-exit.txt
exit "$status"
