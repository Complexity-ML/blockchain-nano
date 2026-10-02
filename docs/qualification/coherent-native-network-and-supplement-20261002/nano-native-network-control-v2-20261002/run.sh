#!/bin/bash
set -eu
source=/tmp/nano-coherent-network-build-candidate-20261002
build=/tmp/nano-coherent-native-network-build-v2-20261002
reports=/tmp/nano-coherent-native-network-reports-v2-20261002
test ! -e "$build"
test ! -e "$reports"
mkdir "$reports"
trap 'code=$?; printf "%s\n" "$code" > "$reports/controller-exit.txt"' EXIT
cd "$source"
git rev-parse HEAD > "$reports/source-revision.txt"
test -z "$(git status --porcelain)"
git ls-files -z | python3 -c 'import sys,hashlib,json; print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-before.json"
cmake -S network -B "$build" -DNANO_DEPS_DIR=/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/work/blockchain-nano/.deps -DCMAKE_BUILD_TYPE=Release > "$reports/configure.log" 2>&1
cmake --build "$build" --parallel 1 > "$reports/build.log" 2>&1
printf '0\n' > "$reports/build-exit.txt"
ctest --test-dir "$build" --show-only=json-v1 > "$reports/registry.json"
find "$build" -maxdepth 1 -type f -perm /111 -print0 | sort -z | xargs -0 shasum -a 256 > "$reports/programs-before.sha256"
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --timeout 120 --output-on-failure --output-junit "$reports/results.xml" > "$reports/tests.log" 2>&1
code=$?
set -e
printf '%s\n' "$code" > "$reports/ctest-exit.txt"
cp "$build/Testing/Temporary/LastTest.log" "$reports/LastTest.log"
find "$build" -maxdepth 1 -type f -perm /111 -print0 | sort -z | xargs -0 shasum -a 256 > "$reports/programs-after.sha256"
cmp "$reports/programs-before.sha256" "$reports/programs-after.sha256"
git ls-files -z | python3 -c 'import sys,hashlib,json; print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-after.json"
cmp "$reports/source-before.json" "$reports/source-after.json"
bash scripts/ci/verify-results.sh "$reports/registry.json" "$reports/results.xml" > "$reports/result-audit.json" 2> "$reports/result-audit.error"
printf '0\n' > "$reports/audit-exit.txt"
exit "$code"
