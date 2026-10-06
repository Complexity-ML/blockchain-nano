#!/bin/bash
set -eu
arch=$1
out=/drivers/$arch-proof
mkdir -p "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
(cd /overlay; sha256sum -c /drivers/source.sha256) > "$out/source-before.log"
sdk=/build/catalog-selection-$arch-20261006/network
cmake --build "$sdk" --target nano-capella-catalog-selection-tests nano-capella-catalog-relay-tests nano-capella-head-catalog-tests nano-capella-catalog-termination-tests nano-capella-head-history-fairness-tests nano-p2p -j2 > "$out/sdk-build.log" 2>&1
ctest --test-dir "$sdk" --output-on-failure -R '^capella_(catalog_selection|catalog_relay|head_catalog|catalog_termination|head_history_fairness)$' > "$out/sdk-ctest.log" 2>&1
cat "$out/sdk-ctest.log"
sha256sum "$sdk/nano-p2p" > "$out/p2p-program.sha256"
(cd /overlay; sha256sum -c /drivers/source.sha256) > "$out/source-after.log"
