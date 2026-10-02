set -euo pipefail
source_dir=/home/ubuntu/nano-snapshot-ssz-view-candidate-20261002
parent=nano-coherent-80373152-amd64-test
parent_reports=/home/ubuntu/nano-coherent-80373152-amd64-test-reports-20261002
control=/home/ubuntu/nano-full-coherent-80373152-amd-control-20261002
reports=/home/ubuntu/nano-full-coherent-80373152-amd-reports-20261002
# Wait for both build and original selected tests; do not infer termination
# from a missing report while a verified container is still live.
while sudo docker inspect nano-coherent-80373152-amd64-build >/dev/null 2>&1 || sudo docker inspect "$parent" >/dev/null 2>&1; do sleep 15; done
# The queued test controller can be between inventory and test containers.
while test ! -f "$parent_reports/result-audit.json" && ps -p 546534 -o args= | grep -F '/tmp/nano-coherent-80373152-test-control-20261002/' >/dev/null; do sleep 15; done
for attempt in $(seq 1 12); do
 if test -f "$parent_reports/result-audit.json" && test -f "$parent_reports/container-exit.txt"; then break; fi
 sleep 5
done
python3 - <<'GATE'
from pathlib import Path
import json,xml.etree.ElementTree as E
p=Path('/home/ubuntu/nano-coherent-80373152-amd64-test-reports-20261002')
for name in ['container-exit.txt','ctest-exit.txt','input-compare-exit.txt','source-compare-exit.txt']:assert (p/name).read_text().strip()=='0'
assert json.loads((p/'result-audit.json').read_text())=={'cases':16,'skipped':0,'failed':0,'timeoutsPreserved':True}
expected=json.loads((p/'expected.json').read_text());tests=json.loads((p/'registry.json').read_text())['tests']
assert len(tests)==16 and {t['name'] for t in tests}==set(expected)
for t in tests:
 props={x['name']:x['value'] for x in t['properties']}
 assert t.get('command') and props.get('TIMEOUT')==expected[t['name']]
 assert not props.get('DISABLED',False) and not props.get('SKIP_RETURN_CODE') and not props.get('SKIP_REGULAR_EXPRESSION')
for a,b in [('source-manifest-before.json','source-manifest-after.json'),('programs-before.sha256','programs-after.sha256'),('inputs-before.sha256','inputs-after.sha256')]:assert (p/a).read_bytes()==(p/b).read_bytes()
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==16 and all(c.find('failure') is None and c.find('error') is None and c.find('skipped') is None for c in cases)
GATE
test "$(git -C "$source_dir" rev-parse --short=8 HEAD)" = 80373152
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir "$reports";chmod 0777 "$reports"
cd "$source_dir"
git rev-parse HEAD > "$reports/source-revision.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest.json"
find "$control" -maxdepth 1 -type f -print0 | sort -z | xargs -0 sha256sum > "$reports/control-sha256.txt"
sudo docker run -i --rm --name nano-full-coherent-80373152-amd64-prepare --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s > "$reports/prepare.log" 2>&1 <<'BUILD'
set -euo pipefail
build=/build/full-coherent-80373152-amd64-20261002
test ! -e "$build"
mkdir -p /build/qualification-full-amd64-80373152
export TMPDIR=/build/qualification-full-amd64-80373152
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p
cmake --build "$build" --parallel 2
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /control/manifest-tool.cmake
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/link-inputs-before.txt
BUILD
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest-after-build.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-build.json"
