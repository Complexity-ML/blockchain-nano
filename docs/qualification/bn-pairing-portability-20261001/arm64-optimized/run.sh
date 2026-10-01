set -euo pipefail
build=/build/bn-pairing-cold-20261001
mkdir -p "$build/scratch";export TMPDIR="$build/scratch"
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 2 --target nano-bn-pairing-tests > /reports/build.log 2>&1
sha256sum "$build/nano-bn-pairing-tests" /work/src/bn_pairing.cpp /work/tests/fixtures/bn256Pairing.json > /reports/hashes.sha256
ctest --test-dir "$build" -R '^bn_pairing_cold_vectors$' --show-only=json-v1 > /reports/selection.json
set +e
ctest --test-dir "$build" -R '^bn_pairing_cold_vectors$' --no-tests=error --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
status=$?
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
printf '%s\n' "$status" > /reports/ctest-exit.txt
exit "$status"