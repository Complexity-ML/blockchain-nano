set -euo pipefail
build=/build/snapshot-cache-row-2db1840c-arm64-20261002
mkdir -p /reports/scratch
mkdir -p /build/qualification-capella-serialization-arm-20261002
export TMPDIR=/build/qualification-capella-serialization-arm-20261002
cmake -S /work -B "$build" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DMCL_PORTABLE=ON -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 2 --target nano nano-capella-transition-tests nano-checkpoint-json-tests > /reports/build.log 2>&1
pattern='^(consensus_checkpoint_json_canonical|consensus_capella_local_exits|consensus_capella_queued_exits|consensus_capella_queued_bls_changes|consensus_capella_competing_deposit_reorg|consensus_capella_deposit_activation|consensus_capella_deposit_activation_restart|consensus_capella_deposit_auto_registration|consensus_capella_reindexed_validator_activation|consensus_capella_exit_reindex)$'
ctest --test-dir "$build" -R "$pattern" --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /control/manifest-tool.cmake
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/link-inputs-before.txt
set +e
ctest --test-dir "$build" -R "$pattern" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
test_exit=$?
printf '%s\n' "$test_exit" > /reports/ctest-exit.txt
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-after-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /control/manifest-tool.cmake
manifest_exit=$?
printf '%s\n' "$manifest_exit" > /reports/post-test-manifest-exit.txt
set -e
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/link-inputs-after.txt
cmp /reports/link-inputs-before.txt /reports/link-inputs-after.txt
cmp /reports/executables-before-tests.json /reports/executables-after-tests.json
test "$manifest_exit" = 0
exit "$test_exit"
