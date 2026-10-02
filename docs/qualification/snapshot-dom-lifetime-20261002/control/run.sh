set -euo pipefail
source=/tmp/nano-snapshot-dom-skeleton-candidate-20261002
control=/tmp/nano-snapshot-dom-skeleton-control-20261002
reports=/tmp/nano-snapshot-dom-skeleton-arm-reports-20261002
mkdir "$reports";chmod 0777 "$reports"
cd "$source"
test "$(git rev-parse HEAD)" = a0e1869305a2c56024e2897ac6f1776ccf022af5
test -z "$(git status --porcelain)"
sources(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
sources > "$reports/source-manifest-before.json"
set +e
docker run --rm -i --name nano-snapshot-dom-skeleton-arm-integration --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 1 --memory 2g --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source,dst=/work,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/base,readonly --mount type=volume,src=nano-linux-sync-build,dst=/writable -w /work sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4 bash -s > "$reports/container.log" 2>&1 <<'RUN'
set -euo pipefail
# New serializer differential first; then the actual signed store integration.
scratch=/writable/snapshot-dom-skeleton-helper-arm-20261002
mkdir "$scratch"
export TMPDIR="$scratch"
g++ -std=c++20 -O2 -I/work/third_party /work/tests/test_capella_snapshot_json.cpp -o "$scratch/helper"
sha256sum "$scratch/helper" > /reports/helper-before.sha256
set +e
timeout 60 "$scratch/helper" > /reports/helper.log 2>&1
result=$?
set -e
printf '%s\n' "$result" > /reports/helper-exit.txt
sha256sum "$scratch/helper" > /reports/helper-after.sha256
cmp /reports/helper-before.sha256 /reports/helper-after.sha256
test "$result" = 0
bash /control/compile.sh
RUN
code=$?
set -e
printf '%s\n' "$code" > "$reports/container-exit.txt"
sources > "$reports/source-manifest-after.json"
cmp "$reports/source-manifest-before.json" "$reports/source-manifest-after.json"
printf '0\n' > "$reports/source-compare-exit.txt"
exit "$code"
