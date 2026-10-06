#!/bin/bash
set -eu
arch=$1
out=/drivers/$arch-proof
mkdir -p "$out"
sdk=/build/catalog-selection-$arch-20261006/network
trap 'status=$?; echo "$status" > "$out/sdk-controller-exit.txt"' EXIT
cache=/build/coherent-efbeb9bc-$arch/network/CMakeCache.txt
deps=$(sed -n 's/^NANO_DEPS_DIR:PATH=//p' "$cache")
[ -n "$deps" ]
cmake -S /work/network -B "$sdk" -DNANO_DEPS_DIR="$deps" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ > "$out/sdk-configure.log" 2>&1
cmake --build "$sdk" --target nano-capella-catalog-selection-tests nano-capella-block-delivery-tests nano-capella-signed-anchor-tests nano-capella-attestation-delivery-tests nano-capella-depth-catchup-tests nano-capella-export-resume-tests nano-capella-head-catalog-tests nano-capella-catalog-termination-tests nano-capella-head-history-fairness-tests nano-altair-core-dispatch-tests nano-altair-core-recovery-tests nano-altair-block-relay-tests nano-core-keepalive-tests nano-core-rpc-diagnostic-tests nano-p2p -j2 > "$out/sdk-build.log" 2>&1
ctest --test-dir "$sdk" --output-on-failure -R '^(altair_(core_dispatch|core_recovery|block_relay)|core_(keepalive|rpc_diagnostic)|capella_(catalog_selection|block_delivery|signed_anchor|attestation_delivery|depth_catchup|export_resume|head_catalog|catalog_termination|head_history_fairness))$' > "$out/sdk-ctest.log" 2>&1
cat "$out/sdk-ctest.log"
sha256sum "$sdk/nano-p2p" > "$out/p2p-program.sha256"
