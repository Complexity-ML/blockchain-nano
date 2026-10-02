set -euo pipefail
source_dir=/home/ubuntu/nano-capella-serialization-candidate-20261002
parent=nano-capella-serialization-amd64-test
parent_reports=/home/ubuntu/nano-capella-serialization-amd64-reports-20261002
control=/home/ubuntu/nano-full-candidate-3030b799-amd-control-20261002
reports=/home/ubuntu/nano-full-candidate-3030b799-amd-reports-20261002
while test -n "$(sudo docker ps --filter name="$parent" --format '{{.Names}}' | grep -x "$parent" || true)"; do sleep 15; done
for attempt in $(seq 1 12); do
 if test -f "$parent_reports/container-exit.txt" && test -f "$parent_reports/source-manifest-after-tests.json"; then break; fi
 sleep 5
done
python3 - <<'GATE'
from pathlib import Path
import xml.etree.ElementTree as E
p=Path('/home/ubuntu/nano-capella-serialization-amd64-reports-20261002')
for name in ['container-exit.txt','ctest-exit.txt','post-test-manifest-exit.txt']:assert (p/name).read_text().strip()=='0'
for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json'),('link-inputs-before.txt','link-inputs-after.txt')]:assert (p/a).read_bytes()==(p/b).read_bytes()
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==10 and all(c.find('failure') is None and c.find('skipped') is None for c in cases)
GATE
test "$(git -C "$source_dir" rev-parse --short=8 HEAD)" = 3030b799
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir "$reports";chmod 0777 "$reports"
cd "$source_dir"
git rev-parse HEAD > "$reports/source-revision.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest.json"
find "$control" -maxdepth 1 -type f -print0 | sort -z | xargs -0 sha256sum > "$reports/control-sha256.txt"
sudo docker run -i --rm --name nano-full-3030b799-amd64-prepare --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s > "$reports/prepare.log" 2>&1 <<'BUILD'
set -euo pipefail
build=/build/published-bytes-fa390c3b-20261002
mkdir -p /build/qualification-full-amd64-3030b799
export TMPDIR=/build/qualification-full-amd64-3030b799
cmake -S /work/network -B /build/network -DCMAKE_BUILD_TYPE=Release -DNANO_DEPS_DIR=/src/.deps
cmake --build /build/network --parallel 2
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DMCL_PORTABLE=ON -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p
cmake --build "$build" --parallel 2
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /control/manifest-tool.cmake
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/link-inputs-before.txt
BUILD
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest-after-build.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-build.json"
