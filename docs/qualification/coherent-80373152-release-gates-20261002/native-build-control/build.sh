#!/usr/bin/env bash
set -euo pipefail
source_dir=/tmp/nano-snapshot-ssz-view-candidate-20261002
reports=/tmp/nano-coherent-80373152-native-build-reports-20261002
cd "$source_dir"
test "$(git rev-parse --short=8 HEAD)" = 80373152
test -z "$(git status --porcelain)"
mkdir "$reports"
sources(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
sources > "$reports/source-before.json"
git diff 0275b1af HEAD -- src include network third_party > "$reports/runtime-diff.txt"
shasum -a 256 /Users/boris/Dev/blockchain-nano/build-network/nano-p2p > "$reports/p2p-before.sha256"
# Mac native is one of the three required owners; Docker only emits Linux binaries.
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/Users/boris/Dev/blockchain-nano/build-network/nano-p2p > "$reports/configure.log" 2>&1
set +e
cmake --build build-release --parallel 2 --target nano nano-capella-transition-tests nano-capella-snapshot-hex-tests nano-capella-audit-ancestry-tests nano-hex-encoding-tests nano-tests nano-capella-rotation-audit nano-checkpoint-json-tests nano-capella-snapshot-json-tests > "$reports/build.log" 2>&1
build_exit=$?
set -e
printf '%s\n' "$build_exit" > "$reports/build-exit.txt"
sources > "$reports/source-after.json"
cmp "$reports/source-before.json" "$reports/source-after.json"
printf '0\n' > "$reports/source-compare-exit.txt"
shasum -a 256 /Users/boris/Dev/blockchain-nano/build-network/nano-p2p > "$reports/p2p-after.sha256"
cmp "$reports/p2p-before.sha256" "$reports/p2p-after.sha256"
test "$build_exit" = 0
for name in nano nano-capella-transition-tests nano-capella-snapshot-hex-tests nano-capella-audit-ancestry-tests nano-hex-encoding-tests nano-tests nano-capella-rotation-audit nano-checkpoint-json-tests nano-capella-snapshot-json-tests; do shasum -a 256 "build-release/$name"; done > "$reports/programs.sha256"
cp build-release/CMakeCache.txt "$reports/CMakeCache.txt"
# Provenance states actual source tree used, not qualification or production readiness.
git rev-parse HEAD > build-release/production-source-revision.txt
printf '0\n' > "$reports/controller-exit.txt"
