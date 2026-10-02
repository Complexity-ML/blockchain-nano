#!/bin/bash
set -euo pipefail
source=/tmp/nano-coherent-network-build-candidate-20261002
build=/tmp/nano-coherent-native-network-build-v2-20261002
reports=/tmp/nano-coherent-native-network-reports-v3-20261002
test -f "$build/nano-p2p"
test ! -e "$reports"
mkdir "$reports"
trap 'code=$?; printf "%s\n" "$code" > "$reports/controller-exit.txt"' EXIT
cd "$source"
git rev-parse HEAD > "$reports/source-revision.txt"
test -z "$(git status --porcelain)"
git ls-files -z | python3 -c 'import sys,hashlib,json; print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-before.json"
test "$(cat /tmp/nano-coherent-native-network-reports-v2-20261002/build-exit.txt)" = 0
cp /tmp/nano-coherent-native-network-reports-v2-20261002/configure.log "$reports/reused-build-configure.log"
printf '%s\n' '/tmp/nano-coherent-native-network-reports-v2-20261002' > "$reports/reused-build-origin.txt"
printf '0\n' > "$reports/build-exit.txt"
ctest --test-dir "$build" --show-only=json-v1 > "$reports/registry.json"
python3 - "$build" <<'HASH' > "$reports/programs-before.sha256"
import sys,os,hashlib
from pathlib import Path
for p in sorted(Path(sys.argv[1]).iterdir()):
 if p.is_file() and os.access(p,os.X_OK):print(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p))
HASH
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --timeout 120 --output-on-failure --output-junit "$reports/results.xml" > "$reports/tests.log" 2>&1
code=$?
set -e
printf '%s\n' "$code" > "$reports/ctest-exit.txt"
cp "$build/Testing/Temporary/LastTest.log" "$reports/LastTest.log"
python3 - "$build" <<'HASH' > "$reports/programs-after.sha256"
import sys,os,hashlib
from pathlib import Path
for p in sorted(Path(sys.argv[1]).iterdir()):
 if p.is_file() and os.access(p,os.X_OK):print(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p))
HASH
cmp "$reports/programs-before.sha256" "$reports/programs-after.sha256"
git ls-files -z | python3 -c 'import sys,hashlib,json; print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-after.json"
cmp "$reports/source-before.json" "$reports/source-after.json"
bash scripts/ci/verify-results.sh "$reports/registry.json" "$reports/results.xml" > "$reports/result-audit.json" 2> "$reports/result-audit.error"
printf '0\n' > "$reports/audit-exit.txt"
exit "$code"
