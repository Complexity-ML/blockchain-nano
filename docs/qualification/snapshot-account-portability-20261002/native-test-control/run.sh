#!/usr/bin/env bash
set -euo pipefail
source_dir=/tmp/nano-snapshot-account-portable-candidate-20261002
build_reports=/tmp/nano-coherent-016ab243-native-build-reports-20261002
reports=/tmp/nano-coherent-016ab243-native-test-reports-20261002
control=/tmp/nano-coherent-016ab243-native-test-control-20261002
for file in build-exit.txt source-compare-exit.txt controller-exit.txt; do test "$(cat "$build_reports/$file")" = 0; done
cmp "$build_reports/source-before.json" "$build_reports/source-after.json"
cd "$source_dir"
test -z "$(git status --porcelain)"
test "$(git rev-parse --short=8 HEAD)" = 016ab243
mkdir "$reports"
cp "$control/expected.json" "$reports/expected.json"
sources(){ git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))'; }
sources > "$reports/source-before.json"
inputs(){ python3 - <<'PY'
import hashlib,json,subprocess
from pathlib import Path
build=Path('build-release');files=set(build.rglob('*.a'))|set(build.rglob('*.o'))
programs=['nano','nano-capella-transition-tests','nano-capella-snapshot-hex-tests','nano-capella-audit-ancestry-tests','nano-hex-encoding-tests','nano-tests','nano-capella-rotation-audit','nano-checkpoint-json-tests','nano-capella-snapshot-json-tests','nano-consensus-hash-provider-tests','nano-capella-snapshot-account-tests']
files.update(build/name for name in programs);files.update([Path('/opt/homebrew/bin/node'),Path('/Users/boris/Dev/blockchain-nano/build-network/nano-p2p')])
pending=list(files);seen=set()
while pending:
 f=pending.pop()
 if f in seen:continue
 seen.add(f)
 if f.suffix in ['.a','.o']:continue
 r=subprocess.run(['otool','-L',str(f)],capture_output=True,text=True)
 if r.returncode!=0:continue
 for line in r.stdout.splitlines()[1:]:
  name=line.strip().split(' (',1)[0]
  if name.startswith('/') and Path(name).is_file() and Path(name) not in files:
   dep=Path(name);files.add(dep);pending.append(dep)
print(json.dumps({str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(files)},sort_keys=True))
PY
}
inputs > "$reports/inputs-before.json"
regex=$(cat "$control/regex.txt")
ctest --test-dir build-release --show-only=json-v1 -R "$regex" > "$reports/registry.json"
set +e
ctest --test-dir build-release --no-tests=error --parallel 1 --output-on-failure --output-junit "$reports/results.xml" -R "$regex" > "$reports/tests.log" 2>&1
run_exit=$?
set -e
printf '%s\n' "$run_exit" > "$reports/ctest-exit.txt"
cp build-release/Testing/Temporary/LastTest.log "$reports/LastTest.log"
inputs > "$reports/inputs-after.json"
cmp "$reports/inputs-before.json" "$reports/inputs-after.json"
sources > "$reports/source-after.json"
cmp "$reports/source-before.json" "$reports/source-after.json"
python3 - "$reports" <<'PY'
import json,sys,xml.etree.ElementTree as E
from pathlib import Path
p=Path(sys.argv[1]);expected=json.loads((p/'expected.json').read_text());registry=json.loads((p/'registry.json').read_text())['tests']
assert len(registry)==13 and {t['name'] for t in registry}==set(expected)
for t in registry:assert t.get('command') and next(x['value'] for x in t['properties'] if x['name']=='TIMEOUT')==expected[t['name']]
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'));assert len(cases)==13 and {c.attrib['name'] for c in cases}==set(expected)
assert all(c.find('skipped') is None for c in cases)
(p/'result-audit.json').write_text(json.dumps({'cases':13,'failed':sum(c.find('failure') is not None or c.find('error') is not None for c in cases),'skipped':0,'timeoutsPreserved':True,'sourceAndInputsUnchanged':True,'scope':'native core, helper and controller only; no WAN'},indent=2)+'\n')
PY
exit "$run_exit"
