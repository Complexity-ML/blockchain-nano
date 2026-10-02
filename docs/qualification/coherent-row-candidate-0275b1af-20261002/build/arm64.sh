#!/usr/bin/env bash
set -euo pipefail
source_dir=/tmp/nano-checkpoint-row-dump-candidate-20261002
reports=/tmp/nano-coherent-0275b1af-arm64-build-reports-20261002
mkdir "$reports"
chmod 0777 "$reports"
cd "$source_dir"
test "$(git rev-parse --short=8 HEAD)" = 0275b1af
test -z "$(git status --porcelain)"
git rev-parse HEAD > "$reports/source-revision.txt"
sources(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
sources > "$reports/source-manifest-before.json"
set +e
docker run --rm -i --name nano-coherent-0275b1af-arm64-build --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 1 --memory 2g --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4 bash -s > "$reports/build.log" 2>&1 <<'BUILD'
set -euo pipefail
build=/build/coherent-0275b1af-arm64-20261002
test ! -e "$build"
mkdir -p "$build/scratch"
export TMPDIR="$build/scratch"
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p
cmake --build "$build" --parallel 1 --target nano nano-capella-transition-tests nano-capella-snapshot-hex-tests nano-capella-audit-ancestry-tests nano-hex-encoding-tests nano-tests nano-capella-rotation-audit nano-checkpoint-json-tests nano-capella-snapshot-json-tests
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
sha256sum "$build/nano" "$build/nano-capella-transition-tests" "$build/nano-capella-snapshot-hex-tests" "$build/nano-capella-audit-ancestry-tests" "$build/nano-hex-encoding-tests" "$build/nano-tests" "$build/nano-capella-rotation-audit" "$build/nano-checkpoint-json-tests" "$build/nano-capella-snapshot-json-tests" > /reports/programs.sha256
printf '0\n' > /reports/build-exit.txt
BUILD
build_exit=$?
set -e
printf '%s\n' "$build_exit" > "$reports/container-exit.txt"
sources > "$reports/source-manifest-after.json"
cmp "$reports/source-manifest-before.json" "$reports/source-manifest-after.json"
printf '0\n' > "$reports/source-compare-exit.txt"
exit "$build_exit"
