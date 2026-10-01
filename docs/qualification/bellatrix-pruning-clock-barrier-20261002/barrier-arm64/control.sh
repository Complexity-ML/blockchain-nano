set -euo pipefail
build=/build/bellatrix-barrier-1ac57e25-20261002
mkdir -p "$build/qualification-scratch"
export TMPDIR="$build/qualification-scratch"
selection='^(consensus_bellatrix_voluntary_exit(_pruning|_network|_reorg)?|consensus_bellatrix_clock_coalescing)$'
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 2 --target nano-bellatrix-transition-tests > /reports/build.log 2>&1
ctest --test-dir "$build" --show-only=json-v1 -R "$selection" > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -P /work/scripts/ci/docker-manifest.cmake
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 -R "$selection" --output-on-failure --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
test_exit=${PIPESTATUS[0]}
printf '%s\n' "$test_exit" > /reports/ctest-exit.txt
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-after-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -P /work/scripts/ci/docker-manifest.cmake
manifest_exit=$?
printf '%s\n' "$manifest_exit" > /reports/post-test-manifest-exit.txt
if (( manifest_exit != 0 )); then exit 1; fi
cmp /reports/executables-before-tests.json /reports/executables-after-tests.json
exit "$test_exit"
