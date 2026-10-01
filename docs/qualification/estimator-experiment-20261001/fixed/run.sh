#!/usr/bin/env bash
set -euo pipefail
build=/build/estimator-depth-20261001
mkdir -p "$build/scratch"
export TMPDIR="$build/scratch"
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p 2>&1 | tee /reports/configure.log
cmake --build "$build" --parallel 2 --target nano nano-tests nano-rpc-tests 2>&1 | tee /reports/build.log
sha256sum "$build/nano-rpc-tests" > /reports/executables.sha256
ctest --test-dir "$build" -R '^(core|rpc)$' --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$build" -R '^(core|rpc)$' --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
status=${PIPESTATUS[0]}
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
printf '%s\n' "$status" > /reports/ctest-exit.txt
exit "$status"
