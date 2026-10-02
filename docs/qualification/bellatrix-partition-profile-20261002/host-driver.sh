set -euo pipefail
corrective=/home/ubuntu/nano-altair-c104-diagnostic-20261002-reports
control=/home/ubuntu/nano-bellatrix-partition-profile-control-20261002
reports=/home/ubuntu/nano-bellatrix-partition-profile-20261002-reports
source_dir=/home/ubuntu/nano-production-evm-integration-20261001
printf 'Waiting for existing Altair diagnostic driver PID 318471 to finish before Bellatrix profiling.\n'
while test ! -f "$corrective/container-exit.txt"; do
 if ! kill -0 318471 2>/dev/null; then printf 'Altair diagnostic driver ended without terminal report; profiling not launched.\n' >&2; exit 1; fi
 sleep 15
done
while kill -0 318471 2>/dev/null; do sleep 15; done
while sudo docker ps -q --filter 'name=^/nano-altair-c104-diagnostic-amd64-test$' | read -r container_id; do sleep 15; done
python3 - "$corrective" <<'PY'
import json,pathlib,sys,xml.etree.ElementTree as E
r=pathlib.Path(sys.argv[1]);names=[t['name'] for t in json.loads((r/'registry.json').read_text())['tests']];cases=E.parse(r/'results.xml').getroot().findall('testcase')
assert names==['consensus_altair_libp2p'] and len(cases)==1
assert {t.get('name') for t in cases}==set(names)
assert not any(t.find('skipped') is not None or t.get('status') not in ('run','fail') for t in cases)
assert (r/'post-test-manifest-exit.txt').read_text().strip()=='0'
assert (r/'source-manifest.json').read_bytes()==(r/'source-manifest-after-tests.json').read_bytes()
assert (r/'executables-before-tests.json').read_bytes()==(r/'executables-after-tests.json').read_bytes()
print('Altair diagnostic terminal; its result retained and inputs unchanged.')
PY
test "$(git -C "$source_dir" rev-parse HEAD)" = c10426b2eab0aff7eaada3e4b57460f15cf7e90c
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir -p "$reports"
test -z "$(ls -A "$reports")"
cp "$control/provenance.json" "$reports/"
sha256sum "$control"/* > "$reports/control-sha256.txt"
cd "$source_dir"
git rev-parse HEAD > "$reports/runtime-source-revision.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest.json"
set +e
sudo docker run -i --rm --name nano-bellatrix-partition-profile-amd64-test --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build,readonly -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s < "$control/container-control.sh" > "$reports/container.log" 2>&1
container_exit=$?
set -e
printf '%s\n' "$container_exit" > "$reports/container-exit.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest-after-tests.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-tests.json"
exit "$container_exit"
