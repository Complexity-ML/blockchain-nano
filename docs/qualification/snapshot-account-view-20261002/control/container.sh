set -euo pipefail
build=/build/snapshot-accounts-0f53b42d-amd64-20261002
export TMPDIR=/build/snapshot-accounts-0f53b42d-tmp-20261002
test ! -e "$build"
mkdir "$TMPDIR"
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 2 --target nano-capella-transition-tests nano-consensus-hash-provider-tests nano-capella-snapshot-account-tests nano-capella-snapshot-json-tests > /reports/build.log 2>&1
printf '0\n' > /reports/build-exit.txt
ctest --test-dir "$build" --show-only=json-v1 -R '^(consensus_capella_signed_transition|consensus_capella_reindexed_validator_activation|consensus_hash_provider_parallel|consensus_hash_provider_unavailable|consensus_capella_snapshot_account_primitives|consensus_capella_snapshot_cached_rows)$' > /reports/registry.json
sha256sum "$build/nano-capella-transition-tests" "$build/nano-consensus-hash-provider-tests" "$build/nano-capella-snapshot-account-tests" "$build/nano-capella-snapshot-json-tests" > /reports/programs-before.sha256
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --output-on-failure -R '^(consensus_capella_signed_transition|consensus_capella_reindexed_validator_activation|consensus_hash_provider_parallel|consensus_hash_provider_unavailable|consensus_capella_snapshot_account_primitives|consensus_capella_snapshot_cached_rows)$' --output-junit /reports/results.xml > /reports/tests.log 2>&1
code=$?
set -e
printf '%s\n' "$code" > /reports/ctest-exit.txt
sha256sum "$build/nano-capella-transition-tests" "$build/nano-consensus-hash-provider-tests" "$build/nano-capella-snapshot-account-tests" "$build/nano-capella-snapshot-json-tests" > /reports/programs-after.sha256
cmp /reports/programs-before.sha256 /reports/programs-after.sha256
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
exit "$code"
