#!/bin/bash
set -eu
arch=$1
if [ "$arch" = native ]; then
 drivers=/private/tmp/nano-catalog-busy-integrated-20261006
 src="$drivers/source"
 sdk=/private/tmp/nano-catalog-busy-integrated-native-20261006/network
 deps=/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/work/blockchain-nano/.deps
else
 drivers=/drivers
 src=/drivers/source
 sdk=/build/catalog-busy-integrated-$arch-20261006/network
 cache=/build/coherent-efbeb9bc-$arch/network/CMakeCache.txt
 deps=$(sed -n 's/^NANO_DEPS_DIR:PATH=//p' "$cache")
fi
out="$drivers/$arch-proof"
mkdir -p "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
(cd "$src"; if command -v sha256sum >/dev/null; then sha256sum -c "$drivers/source.sha256"; else shasum -a 256 -c "$drivers/source.sha256"; fi) > "$out/source-before.log"
cmake -S "$src/network" -B "$sdk" -DNANO_DEPS_DIR="$deps" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ > "$out/configure.log" 2>&1
cmake --build "$sdk" --target nano-p2p nano-altair-core-dispatch-tests nano-altair-core-recovery-tests nano-capella-catalog-relay-tests nano-capella-block-delivery-tests nano-capella-signed-anchor-tests nano-core-keepalive-tests nano-capella-attestation-delivery-tests nano-capella-head-catalog-tests nano-capella-relay-schedule-tests nano-capella-depth-catchup-tests nano-capella-export-resume-tests nano-capella-head-history-fairness-tests nano-capella-catalog-termination-tests nano-capella-catalog-selection-tests nano-core-rpc-diagnostic-tests nano-capella-dependency-preparation-tests nano-capella-catalog-busy-http-tests -j2 > "$out/build.log" 2>&1
ctest --test-dir "$sdk" --output-on-failure -R '^(core_.*|altair_core_.*|capella_.*)$' > "$out/ctest.log" 2>&1
cat "$out/ctest.log"
if command -v sha256sum >/dev/null; then sha256sum "$sdk/nano-p2p" "$sdk/nano-capella-catalog-busy-http-tests"; else shasum -a 256 "$sdk/nano-p2p" "$sdk/nano-capella-catalog-busy-http-tests"; fi > "$out/programs.sha256"
(cd "$src"; if command -v sha256sum >/dev/null; then sha256sum -c "$drivers/source.sha256"; else shasum -a 256 -c "$drivers/source.sha256"; fi) > "$out/source-after.log"
