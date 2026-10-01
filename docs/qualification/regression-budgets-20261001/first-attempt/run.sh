#!/usr/bin/env bash
set -euo pipefail
cmake -S /work -B /build -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p 2>&1 | tee /reports/configure.log
cmake --build /build --parallel 2 --target nano-tests nano-chain-tests nano-rpc-tests nano-beacon-http-errors-tests nano-migration-tests nano-uncle-tests nano-bellatrix-transition-tests nano-execution-deposit-tests nano-deposit-history-tests nano-bellatrix-exit-identity-tests nano-capella-withdrawal-tests nano-secp256k1-tests 2>&1 | tee /reports/build.log
ctest --test-dir /build --show-only=json-v1 > /reports/registry.json
pattern='^(core|chain|rpc|beacon_http_error_diagnostics|bootstrap|migration|uncles|consensus_bellatrix_proposal_expiry|consensus_bellatrix_randao_expiry|consensus_bellatrix_attestation_expiry|consensus_execution_deposit_source|consensus_execution_deposit_history|consensus_bellatrix_exit_identity|consensus_capella_withdrawal_execution|secp256k1_ethereum_compatibility)$'
ctest --test-dir /build -R "$pattern" --show-only=json-v1 > /reports/selection.json
cmake -P /reports/check.cmake
ctest --test-dir /build -R "$pattern" --output-on-failure --no-tests=error --parallel 1 --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
sha256sum /build/nano /build/network/nano-p2p > /reports/runtime.sha256
