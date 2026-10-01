#!/usr/bin/env bash
set -euo pipefail
build=/build/rpc-positional-client-20261001
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p 2>&1 | tee /reports/configure.log
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
cmake --build "$build" --parallel 2 --target nano-storage-sync-failure-tests nano-bft-signing-sync-tests nano-snapshot-sync-failure-tests nano-fork-checkpoint-sync-tests nano-execution-checkpoint-sync-tests nano-shanghai-checkpoint-sync-tests nano-capella-transition-tests nano-altair-reactor-tests nano-beacon-recovery-tests nano-bellatrix-transition-tests 2>&1 | tee /reports/build.log
pattern='^(storage_sync_(failure|retry)_(paris|shanghai)|bft_signing_sync_(paris|shanghai)|consensus_phase0_snapshot_sync_failure|consensus_fork_checkpoint_sync_failure|storage_(execution|shanghai)_checkpoint_sync|consensus_capella_execution_owner_(sync|witness)|consensus_capella_pruning_sync|consensus_altair_reactor_publication|consensus_beacon_witness_recovery|consensus_bellatrix_signed_execution_chain)$'
ctest --test-dir "$build" -R "$pattern" --show-only=json-v1 > /reports/selection.json
sha256sum "$build/nano" /build/network/nano-p2p > /reports/runtime.sha256
ctest --test-dir "$build" -R "$pattern" --no-tests=error --output-on-failure --parallel 1 --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
