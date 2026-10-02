set -euo pipefail
source_dir=/home/ubuntu/nano-production-evm-integration-20261001
control=/home/ubuntu/nano-altair-probe-transport-diagnostic-20261002
reports=/home/ubuntu/nano-altair-probe-transport-diagnostic-20261002-reports
python3 - <<'PROFILE_GATE'
from pathlib import Path
import json,xml.etree.ElementTree as E
p=Path('/home/ubuntu/nano-bellatrix-partition-profile-20261002-reports')
assert (p/'container-exit.txt').exists()
assert [t['name'] for t in json.loads((p/'registry.json').read_text())['tests']]==['consensus_bellatrix_partition_simulation']
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==1 and cases[0].find('skipped') is None
for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json'),('link-inputs-before.txt','link-inputs-after.txt')]:assert (p/a).read_bytes()==(p/b).read_bytes()
assert (p/'post-test-manifest-exit.txt').read_text().strip()=='0'
print('Bellatrix profile terminal; original result retained and inputs unchanged')
PROFILE_GATE
test "$(git -C "$source_dir" rev-parse HEAD)" = c10426b2eab0aff7eaada3e4b57460f15cf7e90c
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir -p "$reports"
chmod 0777 "$reports" # Dedicated output directory for root with all capabilities dropped.
test -z "$(ls -A "$reports")"
cp "$control/provenance.json" "$reports/"
sha256sum "$control"/* > "$reports/control-sha256.txt"
cd "$source_dir"
git rev-parse HEAD > "$reports/runtime-source-revision.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest.json"
set +e
sudo docker run -i --rm --name nano-altair-probe-transport-diagnostic-amd64-test --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build,readonly -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s < "$control/container-control.sh" > "$reports/container.log" 2>&1
container_exit=$?
set -e
printf '%s\n' "$container_exit" > "$reports/container-exit.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest-after-tests.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-tests.json"
exit "$container_exit"
