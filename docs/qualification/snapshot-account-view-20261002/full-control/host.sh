set -euo pipefail
arch=$1
if test "$arch" = amd64;then
 source_dir=/home/ubuntu/nano-snapshot-account-view-candidate-20261002
 prefix=/home/ubuntu
 docker_command=(sudo -n docker)
 image=sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c
else
 test "$arch" = arm64
 source_dir=/tmp/nano-snapshot-account-view-candidate-20261002
 prefix=/tmp
 docker_command=(docker)
 image=sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4
fi
control=$prefix/nano-full-0f53b42d-control-20261002
reports=$prefix/nano-full-coherent-0f53b42d-$arch-reports-20261002
cd "$source_dir"
test "$(git rev-parse HEAD)" = 0f53b42d3e78da85aa9a2c2f2993472747aabe8a
test -z "$(git status --porcelain)"
mkdir "$reports";chmod 0777 "$reports"
git rev-parse HEAD > "$reports/source-revision.txt"
manifest(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
manifest > "$reports/source-before.json"
set +e
"${docker_command[@]}" run --rm --name "nano-full-coherent-0f53b42d-$arch" --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m -v "$source_dir:/work:ro" -v "$control:/control:ro" -v "$reports:/reports" -v nano-linux-sync-build:/build -w /work "$image" bash /control/container.sh "$arch" > "$reports/container.log" 2>&1
code=$?
set -e
printf '%s\n' "$code" > "$reports/container-exit.txt"
manifest > "$reports/source-after.json"
cmp "$reports/source-before.json" "$reports/source-after.json"
printf '0\n' > "$reports/source-compare-exit.txt"
exit "$code"
