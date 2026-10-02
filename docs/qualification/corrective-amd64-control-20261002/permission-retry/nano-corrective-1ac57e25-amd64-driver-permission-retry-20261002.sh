set -euo pipefail
old=nano-full-production-evm-c10426b2-amd64-test
old_reports=/home/ubuntu/nano-production-evm-full-amd64-20261001-reports
source_dir=/home/ubuntu/nano-bellatrix-pruning-clock-barrier-20261002
reports=/home/ubuntu/nano-corrective-1ac57e25-amd64-20261002-reports
printf 'Waiting for original container %s to finish; no compilation during its tests.\n' "$old"
while sudo docker ps -q --filter "name=^/${old}$" | read -r container_id; do sleep 15; done
python3 - "$old_reports" <<'PY'
import sys,json,pathlib,xml.etree.ElementTree as E
r=pathlib.Path(sys.argv[1]);root=E.parse(r/'results.xml').getroot();cases=root.findall('testcase')
names=[t['name'] for t in json.loads((r/'registry.json').read_text())['tests']]
assert len(names)==417 and len(set(names))==417
assert len(cases)==417 and {t.get('name') for t in cases}==set(names)
assert not any(t.find('skipped') is not None or t.get('status') not in ('run','fail') for t in cases)
assert (r/'post-test-manifest-exit.txt').read_text().strip()=='0'
assert (r/'executables-before-tests.json').read_bytes()==(r/'executables-after-tests.json').read_bytes()
print('Original campaign terminal; all 417 results retained; executable hashes unchanged.')
PY
test "$(git -C "$source_dir" rev-parse HEAD)" = 1ac57e25ff04f1d79b5c605acc2ae8ff1d73dcc9
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir -p "$reports"
chmod 0777 "$reports" # Root container drops DAC_OVERRIDE; this dedicated output directory must be writable.
test -z "$(ls -A "$reports")"
cd "$source_dir"
git rev-parse HEAD > "$reports/source-revision.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest.json"
set +e
sudo docker run -i --rm --name nano-corrective-1ac57e25-amd64-test --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s < /home/ubuntu/nano-corrective-1ac57e25-amd64-20261002.sh > "$reports/container.log" 2>&1
container_exit=$?
set -e
printf '%s\n' "$container_exit" > "$reports/container-exit.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest-after-tests.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-tests.json"
set +e
bash scripts/ci/verify-results.sh "$reports/registry.json" "$reports/results.xml" > "$reports/result-audit.json" 2> "$reports/result-audit.error"
audit_exit=$?
set -e
printf '%s\n' "$audit_exit" > "$reports/result-audit-exit.txt"
printf 'Corrective container exit=%s, result audit=%s.\n' "$container_exit" "$audit_exit"
exit "$container_exit"
