set -euo pipefail
source_dir=/home/ubuntu/nano-snapshot-ssz-view-candidate-20261002
control=/home/ubuntu/nano-full-coherent-80373152-amd-control-20261002
reports=/home/ubuntu/nano-full-coherent-80373152-amd-reports-20261002
python3 - <<'GATE'
from pathlib import Path
import json
p=Path('/home/ubuntu/nano-full-coherent-80373152-amd-reports-20261002');i=json.loads((p/'host-inventory.json').read_text())
assert i['completeSelection'] and not i['missingExecutables'] and not i['withoutExplicitTimeout'] and i['totalTests']==426
GATE
set +e
sudo docker run -i --rm --name nano-full-coherent-80373152-amd64-test --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s > "$reports/container.log" 2>&1 <<'TESTS'
set -euo pipefail
build=/build/full-coherent-80373152-amd64-20261002
export TMPDIR=/build/qualification-full-amd64-80373152
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry-before-tests.json
cmp /reports/registry.json /reports/registry-before-tests.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /control/manifest-tool.cmake
cmp /reports/executables.json /reports/executables-before-tests.json
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
result=$?;printf '%s\n' "$result" > /reports/ctest-exit.txt
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-after-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /control/manifest-tool.cmake
manifest_result=$?;printf '%s\n' "$manifest_result" > /reports/post-test-manifest-exit.txt
set -e
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/link-inputs-after.txt
cmp /reports/link-inputs-before.txt /reports/link-inputs-after.txt
cmp /reports/executables-before-tests.json /reports/executables-after-tests.json
test "$manifest_result" = 0
exit "$result"
TESTS
result=$?
set -e
printf '%s\n' "$result" > "$reports/container-exit.txt"
cd "$source_dir"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest-after-tests.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-tests.json"
exit "$result"
