#!/usr/bin/env bash
set -euo pipefail
source_dir=/home/ubuntu/nano-production-candidate-20261002
build_reports=/home/ubuntu/nano-coherent-12ad9eb5-amd64-build-reports-20261002
reports=/home/ubuntu/nano-coherent-12ad9eb5-amd64-reindex-reports-20261002
while sudo docker ps --filter name=nano-coherent-12ad9eb5-amd64-build --format '{{.Names}}' | grep -qx nano-coherent-12ad9eb5-amd64-build; do sleep 15; done
for attempt in $(seq 1 12); do test ! -f "$build_reports/source-compare-exit.txt" || break; sleep 5; done
for file in build-exit.txt container-exit.txt source-compare-exit.txt; do test "$(cat "$build_reports/$file")" = 0; done
cmp "$build_reports/source-manifest-before.json" "$build_reports/source-manifest-after.json"
cd "$source_dir"
test "$(git rev-parse --short=8 HEAD)" = 12ad9eb5
test -z "$(git status --porcelain)"
mkdir "$reports";chmod 0777 "$reports"
sources(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
sources > "$reports/source-manifest-before.json"
set +e
sudo docker run --rm -i --name nano-coherent-12ad9eb5-amd64-reindex-test --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s > "$reports/container.log" 2>&1 <<'RUN'
set -euo pipefail
build=/build/coherent-12ad9eb5-amd64-20261002
export TMPDIR="$build/scratch"
programs=("$build/nano" "$build/nano-capella-transition-tests" "$build/nano-capella-snapshot-hex-tests" "$build/nano-capella-audit-ancestry-tests")
inputs=("$build/libnano_core.a" "$build/third_party/mcl/bint64.o" "$build/third_party/mcl/base64.o" "$build/third_party/ckzg/libnano_ckzg.a" "$build/third_party/ckzg/libnano_blst.a" "$build/third_party/secp256k1/lib/libsecp256k1.a" "$build/lib/libmcl.a")
sha256sum "${programs[@]}" > /reports/programs-before.sha256
sha256sum "${inputs[@]}" > /reports/inputs-before.sha256
ctest --test-dir "$build" --show-only=json-v1 -R '^(consensus_capella_reindexed_validator_activation|consensus_capella_snapshot_canonical_hex|consensus_capella_audit_finalized_ancestry)$' > /reports/registry.json
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml -R '^(consensus_capella_reindexed_validator_activation|consensus_capella_snapshot_canonical_hex|consensus_capella_audit_finalized_ancestry)$' > /reports/tests.log 2>&1
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
python3 - "$reports" <<'VERIFY'
import sys,json,xml.etree.ElementTree as E
from pathlib import Path
p=Path(sys.argv[1]);expected={'consensus_capella_reindexed_validator_activation':360,'consensus_capella_snapshot_canonical_hex':60,'consensus_capella_audit_finalized_ancestry':60}
registry=json.loads((p/'registry.json').read_text())['tests'];assert len(registry)==3 and {t['name'] for t in registry}==set(expected)
for t in registry:assert next(x['value'] for x in t['properties'] if x['name']=='TIMEOUT')==expected[t['name']]
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==3 and {c.attrib['name'] for c in cases}==set(expected)
assert all(c.find('skipped') is None for c in cases)
summary={'cases':3,'skipped':0,'failed':sum(c.find('failure') is not None or c.find('error') is not None for c in cases),'timeoutsPreserved':True}
(p/'result-audit.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary))
VERIFY
exit "$run_exit"
