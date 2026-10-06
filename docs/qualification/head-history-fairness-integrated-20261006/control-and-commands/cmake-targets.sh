#!/bin/bash
set -eu
out=/build/history-first-cmake-targets-arm64-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > /output/cmake-targets-controller-exit.txt' EXIT
cache=/build/coherent-efbeb9bc-arm64/network/CMakeCache.txt
deps=$(sed -n 's/^NANO_DEPS_DIR:PATH=//p' "$cache")
[ -n "$deps" ]
printf '%s\n' "$deps" > /output/cmake-deps-path.txt
cmake -S /work/network -B "$out" -DNANO_DEPS_DIR="$deps" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ > /output/cmake-configure.log 2>&1
cmake --build "$out" --target nano-capella-head-catalog-tests nano-capella-head-history-fairness-tests nano-capella-relay-schedule-tests -j2 > /output/cmake-targets-build.log 2>&1
ctest --test-dir "$out" --output-on-failure -R '^capella_(head_catalog|head_history_fairness|relay_schedule)$' > /output/cmake-targets-ctest.log 2>&1
cat /output/cmake-targets-ctest.log
