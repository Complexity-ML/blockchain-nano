#!/usr/bin/env bash
set -euo pipefail
source_dir=/home/ubuntu/nano-production-hex-candidate-20261002
reports=/home/ubuntu/nano-coherent-2d2764de-amd64-build-reports-20261002
mkdir "$reports"
chmod 0777 "$reports"
cd "$source_dir"
test "$(git rev-parse --short=8 HEAD)" = 2d2764de
test -z "$(git status --porcelain)"
git rev-parse HEAD > "$reports/source-revision.txt"
sources(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
sources > "$reports/source-manifest-before.json"
set +e
sudo docker run --rm -i --name nano-coherent-2d2764de-amd64-build --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s > "$reports/build.log" 2>&1 <<'BUILD'
set -euo pipefail
build=/build/coherent-2d2764de-amd64-20261002
mkdir -p "$build/scratch"
export TMPDIR="$build/scratch"
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p
cmake --build "$build" --parallel 2 --target nano nano-capella-transition-tests nano-capella-snapshot-hex-tests nano-capella-audit-ancestry-tests nano-hex-encoding-tests nano-tests nano-capella-rotation-audit nano-checkpoint-json-tests
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
sha256sum "$build/nano" "$build/nano-capella-transition-tests" "$build/nano-capella-snapshot-hex-tests" "$build/nano-capella-audit-ancestry-tests" "$build/nano-hex-encoding-tests" "$build/nano-tests" "$build/nano-capella-rotation-audit" "$build/nano-checkpoint-json-tests" > /reports/programs.sha256
printf '0\n' > /reports/build-exit.txt
BUILD
build_exit=$?
set -e
printf '%s\n' "$build_exit" > "$reports/container-exit.txt"
sources > "$reports/source-manifest-after.json"
cmp "$reports/source-manifest-before.json" "$reports/source-manifest-after.json"
printf '0\n' > "$reports/source-compare-exit.txt"
exit "$build_exit"
