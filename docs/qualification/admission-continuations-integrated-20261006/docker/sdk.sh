#!/bin/bash
set -eu
arch=$1
out=/build/admission-continuations-$arch-20261006
sdk=$out/network
trap 'status=$?; echo "$status" > "$out/sdk-controller-exit.txt"' EXIT
cache=/build/coherent-efbeb9bc-$arch/network/CMakeCache.txt
deps=$(sed -n 's/^NANO_DEPS_DIR:PATH=//p' "$cache")
[ -n "$deps" ]
cmake -S /work/network -B "$sdk" -DNANO_DEPS_DIR="$deps" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ > "$out/sdk-configure.log" 2>&1
cmake --build "$sdk" --target nano-altair-worker-tests nano-altair-worker-continuation-tests nano-altair-core-dispatch-tests nano-altair-core-recovery-tests nano-p2p -j2 > "$out/sdk-build.log" 2>&1
ctest --test-dir "$sdk" --output-on-failure -R '^altair_(worker|worker_continuations|core_dispatch|core_recovery)$' > "$out/sdk-ctest.log" 2>&1
cat "$out/sdk-ctest.log"
sha256sum "$sdk/nano-p2p" > "$out/p2p-program.sha256"
