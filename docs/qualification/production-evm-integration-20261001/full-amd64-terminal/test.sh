#!/usr/bin/env bash
set -euo pipefail
build=/build/production-evm-c10426b2-20261001
export TMPDIR="$build/qualification-scratch"
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry-before-tests.json
cmp /reports/registry.json /reports/registry-before-tests.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -P /work/scripts/ci/docker-manifest.cmake
cmp /reports/executables.json /reports/executables-before-tests.json
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
status=$?
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
printf '%s\n' "$status" > /reports/ctest-exit.txt
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-after-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -P /work/scripts/ci/docker-manifest.cmake
manifest_status=$?
printf '%s\n' "$manifest_status" > /reports/post-test-manifest-exit.txt
if (( manifest_status != 0 ));then exit 1;fi
exit "$status"
