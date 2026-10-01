#!/usr/bin/env bash
set -euo pipefail
build=/build/bn-gcc-arm64-20261001
mkdir -p "$build/scratch"
export TMPDIR="$build/scratch"
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DMCL_PORTABLE=ON -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 1 --target nano-bn-pairing-tests > /reports/build.log 2>&1
cp "$build/third_party/mcl/CMakeFiles/mcl_st.dir/flags.make" /reports/mcl-flags.make
cp "$build/third_party/mcl/CMakeFiles/mcl_st.dir/build.make" /reports/mcl-build.make
g++ --version > /reports/cxx-version.txt
clang --version > /reports/llvm-compiler-version.txt
sha256sum "$build/nano-bn-pairing-tests" "$build/lib/libmcl.a" > /reports/executables-before.sha256
ctest --test-dir "$build" -R '^bn_pairing_cold_vectors$' --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$build" -R '^bn_pairing_cold_vectors$' --no-tests=error --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
status=$?
printf '%s\n' "$status" > /reports/ctest-exit.txt
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
sha256sum "$build/nano-bn-pairing-tests" "$build/lib/libmcl.a" > /reports/executables-after.sha256
cmp /reports/executables-before.sha256 /reports/executables-after.sha256 || exit 1
exit "$status"
