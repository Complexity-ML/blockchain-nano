set -euo pipefail
source_dir=/home/ubuntu/nano-snapshot-ssz-view-candidate-20261002
parent=/home/ubuntu/nano-coherent-80373152-amd64-test-reports-20261002
control=/home/ubuntu/nano-coherent-80373152-reindex-profile-control-20261002
reports=/home/ubuntu/nano-coherent-80373152-reindex-profile-reports-20261002
container=nano-coherent-80373152-reindex-profile-amd64
while sudo docker inspect nano-coherent-80373152-amd64-test >/dev/null 2>&1; do sleep 15; done
for n in $(seq 1 12); do test -f "$parent/result-audit.json" && break; sleep 5; done
python3 - "$parent" <<'GATE'
import json,sys,xml.etree.ElementTree as E
from pathlib import Path
p=Path(sys.argv[1]);assert (p/'input-compare-exit.txt').read_text().strip()=='0' and (p/'source-compare-exit.txt').read_text().strip()=='0'
for a,b in [('source-manifest-before.json','source-manifest-after.json'),('programs-before.sha256','programs-after.sha256'),('inputs-before.sha256','inputs-after.sha256')]:assert (p/a).read_bytes()==(p/b).read_bytes()
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));expected=json.loads((p/'expected.json').read_text());tests=json.loads((p/'registry.json').read_text())['tests']
assert len(cases)==len(tests)==16 and {c.attrib['name'] for c in cases}==set(expected)
assert all(c.find('skipped') is None for c in cases)
for t in tests:assert next(x['value'] for x in t['properties'] if x['name']=='TIMEOUT')==expected[t['name']]
c=next(c for c in cases if c.attrib['name']=='consensus_capella_reindexed_validator_activation');assert c.find('failure') is not None and 'Timeout' in E.tostring(c,encoding='unicode')
assert json.loads((p/'result-audit.json').read_text())['failed']>=1
GATE
# No diagnostic competes with an active complete regression.
test -z "$(sudo docker ps --format '{{.Names}}' | grep '^nano-full-coherent-80373152-' || true)"
test "$(git -C "$source_dir" rev-parse HEAD)" = 803731526c33111ea128bf6fddf5d7323fb5b220
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir "$reports";chmod 0777 "$reports";mkdir -m 700 "$reports/perf-private"
cp "$parent/inputs-after.sha256" "$reports/qualified-inputs.sha256"
awk '$2=="/build/coherent-80373152-amd64-20261002/nano-capella-transition-tests" {print}' "$parent/programs-after.sha256" > "$reports/qualified-program.sha256"
test "$(wc -l < "$reports/qualified-program.sha256")" = 1
sources(){ git -C "$source_dir" ls-files -z | python3 -c 'import sys,json,hashlib;from pathlib import Path;p=Path("/home/ubuntu/nano-snapshot-ssz-view-candidate-20261002");print(json.dumps({n.decode():hashlib.sha256((p/n.decode()).read_bytes()).hexdigest() for n in sys.stdin.buffer.read().split(b"\0") if n},sort_keys=True))'; }
sources > "$reports/source-before.json"
cleanup(){
 code=$?
 trap - EXIT
 set +e
 sudo docker wait "$container" > "$reports/container-exit.txt"
 sudo docker logs "$container" > "$reports/container.log" 2>&1
 sudo docker inspect "$container" --format '{{json .State}}' > "$reports/container-state.json"
 sudo docker rm "$container" > "$reports/container-removed.txt"
 sources > "$reports/source-after.json"
 cmp "$reports/source-before.json" "$reports/source-after.json"
 source_code=$?
 printf '%s\n' "$source_code" > "$reports/source-compare-exit.txt"
 printf '%s\n' "$code" > "$reports/driver-exit.txt"
 exit "$code"
}
sudo docker run -d --name "$container" --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build,readonly --mount type=volume,src=nano-linux-sync-build,dst=/scratch-volume --mount "type=bind,src=$source_dir,dst=/work,readonly" -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash /control/container.sh > "$reports/container-id.txt"
trap cleanup EXIT
for n in $(seq 1 340);do
 if test -f "$reports/runtime.log" && grep -Eq '"scenario":"deposit-reorg-activation-progress".*"slot":(128|136|144|152)' "$reports/runtime.log";then break;fi
 test "$(sudo docker inspect "$container" --format '{{.State.Running}}')" = true
 sleep 1
done
grep -Eq '"scenario":"deposit-reorg-activation-progress".*"slot":(128|136|144|152)' "$reports/runtime.log"
sudo docker top "$container" -eo pid,etime,pcpu,args > "$reports/processes-before-sampling.txt"
# Exact executable field excludes the timeout supervisor (old selector failure).
profile_pid=$(awk '$4=="/build/coherent-80373152-amd64-20261002/nano-capella-transition-tests" && $5=="--deposit-reorg-activation" {print $1}' "$reports/processes-before-sampling.txt")
test "$(printf '%s\n' "$profile_pid" | wc -l)" = 1 && test -n "$profile_pid"
expected=$(awk '{print $1}' "$reports/qualified-program.sha256")
actual=$(sudo sha256sum "/proc/$profile_pid/exe" | awk '{print $1}')
test "$actual" = "$expected"
printf '%s\n' "$profile_pid" > "$reports/sampled-pid.txt"
sudo perf record -F 49 -e cpu-clock:u --call-graph dwarf,8192 -p "$profile_pid" -o "$reports/perf-private/perf.data" -- sleep 30 > "$reports/perf-record.stdout" 2> "$reports/perf-record.stderr"
sudo perf report -i "$reports/perf-private/perf.data" --stdio --no-children --percent-limit 0.5 --sort dso,symbol > "$reports/perf-leaf.txt" 2> "$reports/perf-report.stderr"
sudo perf report -i "$reports/perf-private/perf.data" --stdio --children --percent-limit 2 --call-graph graph,2,caller --sort symbol > "$reports/perf-callers.txt" 2>> "$reports/perf-report.stderr"
sudo sha256sum "$reports/perf-private/perf.data" > "$reports/raw-perf.sha256"
