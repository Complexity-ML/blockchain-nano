#!/bin/bash
set -eu
out=/build/catalog-termination-cmake-targets-amd64-20261006
mkdir -p "$out"
trap 'status=$?; echo "$status" > /output/amd64-cmake-targets-controller-exit.txt' EXIT
cache=/build/coherent-efbeb9bc-amd64/network/CMakeCache.txt
deps=$(sed -n 's/^NANO_DEPS_DIR:PATH=//p' "$cache")
[ -n "$deps" ]
printf '%s\n' "$deps" > /output/amd64-cmake-deps-path.txt
cmake -S /work/network -B "$out" -DNANO_DEPS_DIR="$deps" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ > /output/amd64-cmake-configure.log 2>&1
cmake --build "$out" --target nano-capella-catalog-termination-tests nano-capella-catalog-relay-tests nano-capella-head-catalog-tests nano-capella-head-history-fairness-tests nano-p2p -j2 > /output/amd64-cmake-targets-build.log 2>&1
ctest --test-dir "$out" --output-on-failure -R '^capella_(catalog_termination|catalog_relay|head_catalog|head_history_fairness)$' > /output/amd64-cmake-targets-ctest.log 2>&1
cat /output/amd64-cmake-targets-ctest.log
