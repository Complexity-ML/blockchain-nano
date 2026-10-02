set -euo pipefail
source_dir=/tmp/nano-snapshot-cache-arm-source-20261002
control=/tmp/nano-arm-witness-volume-retry-control-20261002
reports=/tmp/nano-arm-witness-volume-retry-reports-20261002
test "$(git -C "$source_dir" rev-parse --short=8 HEAD)" = 2db1840c
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir "$reports"
chmod 0777 "$reports"
cp "$control/provenance.json" "$reports/"
find "$control" -type f -exec shasum -a 256 {} \; > "$reports/control-sha256.txt"
cd "$source_dir"
git rev-parse HEAD > "$reports/runtime-source-revision.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest.json"
set +e
docker run -i --rm --name nano-arm-witness-volume-retry-test --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4 bash -s < "$control/container-control.sh" > "$reports/container.log" 2>&1
container_exit=$?
set -e
printf '%s\n' "$container_exit" > "$reports/container-exit.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest-after-tests.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-tests.json"
exit "$container_exit"
