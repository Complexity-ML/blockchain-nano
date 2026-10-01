#!/usr/bin/env bash
set -euo pipefail
journal=/build/assembly-crash-disk-journal-20261001
mkdir -p "$journal/scratch"
export TMPDIR="$journal/scratch"
sha256sum /build/nano /build/nano-bft-large-envelope-tests /build/network/nano-p2p > /reports/executables.sha256
cat > "$journal/CTestTestfile.cmake" <<'CM'
add_test(bft_assembly_crash_paris /build/nano-bft-large-envelope-tests)
set_tests_properties(bft_assembly_crash_paris PROPERTIES ENVIRONMENT "NANO_TEST_EVM_FORK=Paris;NANO_TEST_BFT_ASSEMBLY_CRASH=1" TIMEOUT 240 RUN_SERIAL TRUE)
add_test(bft_assembly_crash_shanghai /build/nano-bft-large-envelope-tests)
set_tests_properties(bft_assembly_crash_shanghai PROPERTIES ENVIRONMENT "NANO_TEST_EVM_FORK=Shanghai;NANO_TEST_BFT_ASSEMBLY_CRASH=1" TIMEOUT 240 RUN_SERIAL TRUE)
CM
cp "$journal/CTestTestfile.cmake" /reports/CTestTestfile.cmake
ctest --test-dir "$journal" --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$journal" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
status=${PIPESTATUS[0]}
cp "$journal/Testing/Temporary/LastTest.log" /reports/LastTest.log
printf '%s\n' "$status" > /reports/ctest-exit.txt
exit "$status"
