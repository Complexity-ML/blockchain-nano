set -euo pipefail
source_dir=/tmp/nano-checkpoint-serialization-candidate-20261002
parent=nano-capella-serialization-arm64-test
parent_reports=/tmp/nano-capella-serialization-arm-reports-20261002
reports=/tmp/nano-full-candidate-3030b799-arm-reports-20261002
network_reports=/tmp/nano-full-candidate-3030b799-arm-network-20261002
while test -n "$(docker ps --filter name="$parent" --format '{{.Names}}' | grep -x "$parent" || true)"; do sleep 15; done
for attempt in $(seq 1 12); do
 if test -f "$parent_reports/container-exit.txt" && test -f "$parent_reports/source-manifest-after-tests.json"; then break; fi
 sleep 5
done
python3 - <<'GATE'
from pathlib import Path
import xml.etree.ElementTree as E
p=Path('/tmp/nano-capella-serialization-arm-reports-20261002')
for name in ['container-exit.txt','ctest-exit.txt','post-test-manifest-exit.txt']:assert (p/name).read_text().strip()=='0'
for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json'),('link-inputs-before.txt','link-inputs-after.txt')]:assert (p/a).read_bytes()==(p/b).read_bytes()
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==10 and all(c.find('failure') is None and c.find('skipped') is None for c in cases)
print('Ten Capella cases terminal and passed; inputs unchanged')
GATE
test "$(git -C "$source_dir" rev-parse --short=8 HEAD)" = 3030b799
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir "$network_reports"
chmod 0777 "$network_reports"
export NANO_BUILDER_IMAGE=sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4
export NANO_BUILD_VOLUME=nano-linux-sync-build
export NANO_BUILD_SUBDIR=snapshot-cache-row-2db1840c-arm64-20261002
cd "$source_dir"
git rev-parse HEAD > "$network_reports/source-revision.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$network_reports/source-manifest.json"
docker run -i --rm --name nano-full-3030b799-arm-network-prepare --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$network_reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work "$NANO_BUILDER_IMAGE" bash -s > "$network_reports/build.log" 2>&1 <<'NET'
set -euo pipefail
mkdir -p /build/qualification-full-network-3030b799
export TMPDIR=/build/qualification-full-network-3030b799
cmake -S /work/network -B /build/network -DCMAKE_BUILD_TYPE=Release -DNANO_DEPS_DIR=/src/.deps
cmake --build /build/network --parallel 2
find /build/network -maxdepth 1 -type f -name 'nano-*' -print0 | sort -z | xargs -0 sha256sum > /reports/executables.json.txt
NET
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$network_reports/source-manifest-after-build.json"
cmp "$network_reports/source-manifest.json" "$network_reports/source-manifest-after-build.json"
mkdir "$reports"
chmod 0777 "$reports"
bash scripts/ci/full-docker.sh "$reports"
