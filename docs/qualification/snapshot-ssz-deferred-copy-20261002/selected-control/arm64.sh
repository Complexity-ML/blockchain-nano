#!/usr/bin/env bash
set -euo pipefail
source_dir=/tmp/nano-snapshot-ssz-view-candidate-20261002
build_reports=/tmp/nano-coherent-80373152-arm64-build-reports-20261002
reports=/tmp/nano-coherent-80373152-arm64-test-reports-20261002
control=/tmp/nano-coherent-80373152-test-control-20261002
for file in build-exit.txt container-exit.txt source-compare-exit.txt; do test "$(cat "$build_reports/$file")" = 0; done
cmp "$build_reports/source-manifest-before.json" "$build_reports/source-manifest-after.json"
cd "$source_dir"
test "$(git rev-parse --short=8 HEAD)" = 80373152
test -z "$(git status --porcelain)"
mkdir "$reports";chmod 0777 "$reports"
sources(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
sources > "$reports/source-manifest-before.json"
cp "$control/expected.json" "$reports/expected.json"
docker run --rm --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 1 --memory 512m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4 ctest --test-dir /build/coherent-80373152-arm64-20261002 --show-only=json-v1 -R "$(cat "$control/regex.txt")" > "$reports/registry-preflight.json"
python3 - "$reports" <<'REGISTRY_HOST'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]);expected=json.loads((p/'expected.json').read_text())
tests=json.loads((p/'registry-preflight.json').read_text())['tests']
assert len(tests)==16 and {t['name'] for t in tests}==set(expected)
for t in tests:
 props={x['name']:x['value'] for x in t['properties']}
 assert t.get('command') and props.get('TIMEOUT')==expected[t['name']]
 assert not props.get('DISABLED',False) and not props.get('SKIP_RETURN_CODE') and not props.get('SKIP_REGULAR_EXPRESSION')
REGISTRY_HOST
set +e
docker run --rm -i --name nano-coherent-80373152-arm64-test --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount "type=bind,src=$control,dst=/control,readonly" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4 bash -s > "$reports/container.log" 2>&1 <<'RUN'
set -euo pipefail
build=/build/coherent-80373152-arm64-20261002
export TMPDIR="$build/scratch"
programs=("$build/nano" "$build/nano-capella-transition-tests" "$build/nano-capella-snapshot-hex-tests" "$build/nano-capella-audit-ancestry-tests" "$build/nano-hex-encoding-tests" "$build/nano-tests" "$build/nano-capella-rotation-audit" "$build/nano-checkpoint-json-tests" "$build/nano-capella-snapshot-json-tests")
inputs=("$build/libnano_core.a" "$build/third_party/mcl/bint64.o" "$build/third_party/mcl/base64.o" "$build/third_party/ckzg/libnano_ckzg.a" "$build/third_party/ckzg/libnano_blst.a" "$build/third_party/secp256k1/lib/libsecp256k1.a" "$build/lib/libmcl.a")
sha256sum "${programs[@]}" > /reports/programs-before.sha256
sha256sum "${inputs[@]}" > /reports/inputs-before.sha256
regex=$(cat /control/regex.txt)
ctest --test-dir "$build" --show-only=json-v1 -R "$regex" > /reports/registry.json
cmp /reports/registry-preflight.json /reports/registry.json
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml -R "$regex" > /reports/tests.log 2>&1
run_exit=$?
set -e
printf '%s\n' "$run_exit" > /reports/ctest-exit.txt
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
sha256sum "${programs[@]}" > /reports/programs-after.sha256
sha256sum "${inputs[@]}" > /reports/inputs-after.sha256
cmp /reports/programs-before.sha256 /reports/programs-after.sha256
cmp /reports/inputs-before.sha256 /reports/inputs-after.sha256
printf '0\n' > /reports/input-compare-exit.txt
exit "$run_exit"
RUN
run_exit=$?
set -e
printf '%s\n' "$run_exit" > "$reports/container-exit.txt"
sources > "$reports/source-manifest-after.json"
cmp "$reports/source-manifest-before.json" "$reports/source-manifest-after.json"
printf '0\n' > "$reports/source-compare-exit.txt"
python3 - "$reports" <<'VERIFY'
import sys,json,xml.etree.ElementTree as E
from pathlib import Path
p=Path(sys.argv[1]);expected=json.loads((p/'expected.json').read_text());registry=json.loads((p/'registry.json').read_text())['tests']
assert len(registry)==16 and {t['name'] for t in registry}==set(expected)
for t in registry:assert t.get('command') and next(x['value'] for x in t['properties'] if x['name']=='TIMEOUT')==expected[t['name']]
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==16 and {c.attrib['name'] for c in cases}==set(expected);assert all(c.find('skipped') is None for c in cases)
summary={'cases':16,'skipped':0,'failed':sum(c.find('failure') is not None or c.find('error') is not None for c in cases),'timeoutsPreserved':True};(p/'result-audit.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary))
VERIFY
exit "$run_exit"
