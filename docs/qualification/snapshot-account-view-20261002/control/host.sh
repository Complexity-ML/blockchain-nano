set -euo pipefail
source_dir=/home/ubuntu/nano-snapshot-account-view-candidate-20261002
reports=/home/ubuntu/nano-snapshot-accounts-reindex-reports-20261002
control=/home/ubuntu/nano-snapshot-accounts-control-20261002
mkdir "$reports";chmod 0777 "$reports"
cd "$source_dir"
git rev-parse HEAD > "$reports/source-revision.txt"
test "$(git rev-parse --short=8 HEAD)" = 0f53b42d
test -z "$(git status --porcelain)"
manifest(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
manifest > "$reports/source-before.json"
set +e
sudo docker run --rm --name nano-snapshot-accounts-0f53b42d-amd-reindex --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m -v "$source_dir:/work:ro" -v "$control:/control:ro" -v "$reports:/reports" -v nano-linux-sync-build:/build -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash /control/container.sh > "$reports/container.log" 2>&1
code=$?
set -e
printf '%s\n' "$code" > "$reports/container-exit.txt"
manifest > "$reports/source-after.json"
cmp "$reports/source-before.json" "$reports/source-after.json"
printf '0\n' > "$reports/source-compare-exit.txt"
exit "$code"
