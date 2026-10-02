set -euo pipefail
parent=/home/ubuntu/nano-capella-serialization-amd64-reports-20261002
control=/home/ubuntu/nano-capella-reindex-profile-control-20261002
reports=/home/ubuntu/nano-capella-reindex-profile-reports-20261002
printf '%s\n' 'Waiting for terminal uninstrumented ten-case Capella campaign and stable manifests.'
while test -n "$(sudo docker ps --filter name=nano-capella-serialization-amd64-test --format '{{.Names}}')";do sleep 10;done
for n in $(seq 1 12);do test -f "$parent/source-manifest-after-tests.json" && break;sleep 5;done
python3 - <<'GATE'
from pathlib import Path
import json,xml.etree.ElementTree as E
p=Path('/home/ubuntu/nano-capella-serialization-amd64-reports-20261002')
assert (p/'container-exit.txt').exists()
assert (p/'post-test-manifest-exit.txt').read_text().strip()=='0'
for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json'),('link-inputs-before.txt','link-inputs-after.txt')]:assert (p/a).read_bytes()==(p/b).read_bytes()
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'))
assert len(cases)==10 and all(c.find('skipped') is None for c in cases)
failed=next(c for c in cases if c.attrib['name']=='consensus_capella_reindexed_validator_activation')
assert failed.find('failure') is not None,'No failed reindex case to diagnose'
assert json.loads((p/'executables-before-tests.json').read_text())['/build/published-bytes-fa390c3b-20261002/nano-capella-transition-tests']=='a125a741e46edff641f2aa8ce03b5984ade96c69c65735752f6b19379dc0baa9'
print('Terminal failed reindex case and unchanged inputs confirmed; diagnostic only.')
GATE
test -z "$(sudo docker ps --filter name=nano-full --format '{{.Names}}')"
mkdir "$reports";chmod 0777 "$reports"
mkdir -m 700 "$reports/perf-private"
find "$control" -type f -exec sha256sum {} \; > "$reports/control.sha256"
sudo docker run -d --name nano-capella-reindex-profile-amd64 --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash /control/container.sh > "$reports/container-id.txt"
for n in $(seq 1 320);do
 if test -f "$reports/runtime.log" && grep -Eq '"scenario":"deposit-reorg-activation-progress".*"slot":(96|104|112|120|128|136|144|152)' "$reports/runtime.log";then break;fi
 test "$(sudo docker inspect nano-capella-reindex-profile-amd64 --format '{{.State.Running}}')" = true
 sleep 1
done
grep -Eq '"scenario":"deposit-reorg-activation-progress".*"slot":(96|104|112|120|128|136|144|152)' "$reports/runtime.log"
sudo docker top nano-capella-reindex-profile-amd64 -eo pid,etime,pcpu,args > "$reports/processes-before-sampling.txt"
profile_pid=$(awk '/nano-capella-transition-tests --deposit-reorg-activation/{print $1;exit}' "$reports/processes-before-sampling.txt")
test -n "$profile_pid"
printf '%s\n' "$profile_pid" > "$reports/sampled-pid.txt"
sudo perf record -F 49 -e cpu-clock:u --call-graph dwarf,8192 -p "$profile_pid" -o "$reports/perf-private/perf.data" -- sleep 30 > "$reports/perf-record.stdout" 2> "$reports/perf-record.stderr"
sudo perf report -i "$reports/perf-private/perf.data" --stdio --no-children --percent-limit 0.5 --sort dso,symbol > "$reports/perf-leaf.txt" 2> "$reports/perf-report.stderr"
sudo perf report -i "$reports/perf-private/perf.data" --stdio --children --percent-limit 2 --call-graph graph,2,caller --sort symbol > "$reports/perf-callers.txt" 2>> "$reports/perf-report.stderr"
sudo sha256sum "$reports/perf-private/perf.data" > "$reports/raw-perf.sha256"
sudo docker wait nano-capella-reindex-profile-amd64 > "$reports/container-exit.txt"
sudo docker logs nano-capella-reindex-profile-amd64 > "$reports/container.log" 2>&1
sudo docker inspect nano-capella-reindex-profile-amd64 --format '{{json .State}}' > "$reports/container-state.json"
test -f "$reports/manifest-exit.txt"
sudo docker rm nano-capella-reindex-profile-amd64 > "$reports/container-removed.txt"
printf '%s\n' 'Diagnostic complete. Instrumented result is not a qualification.'
