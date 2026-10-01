#!/usr/bin/env bash
set -euo pipefail
build=/build/rpc-positional-client-20261001
mkdir -p "$build/cli-fixed-scratch"
export TMPDIR="$build/cli-fixed-scratch"
git -c safe.directory=/work -C /work rev-parse HEAD > /reports/source-revision.txt
sha256sum /work/src/evm.cpp /work/include/nano/client.hpp /work/tests/support/process.hpp > /reports/source.sha256
cmake --build "$build" --parallel 2 --target nano nano-supervisor-tests nano-rpc-cli-tests nano-staking_cli-tests nano-signed-validator-cli-tests 2>&1 | tee /reports/build.log
sha256sum "$build/nano" "$build/nano-supervisor-tests" "$build/nano-rpc-cli-tests" "$build/nano-staking_cli-tests" "$build/nano-signed-validator-cli-tests" /build/network/nano-p2p > /reports/executables.sha256
pattern='^(supervisor|rpc_cli|staking_cli|signed_validator_cli)$'
ctest --test-dir "$build" -R "$pattern" --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$build" -R "$pattern" --output-on-failure --no-tests=error --parallel 1 --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
status=${PIPESTATUS[0]}
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
exit "$status"
