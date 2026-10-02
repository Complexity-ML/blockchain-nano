#!/usr/bin/env bash
# Terminal evidence review only. Never starts WAN nodes or promotes runtime code.
set -euo pipefail
control=$(cd "$(dirname "$0")" && pwd -P)
source_dir=${1:?source checkout}; reports=${2:?full reports}; output=${3:?fresh audit output}; prefix=${4:?build path}; image=${5:?frozen builder image}; volume=${6:?existing build volume}; supplement=${7:?network supplement reports}
[[ "$prefix" =~ ^/build/rpc-full-d1597d9a-(arm64|amd64)$ ]] || exit 2
[[ "$image" =~ ^sha256:[a-f0-9]{64}$ ]] || exit 2
[[ "$volume" =~ ^[A-Za-z0-9][A-Za-z0-9_.-]*$ ]] || exit 2
mkdir "$output"
output=$(cd "$output" && pwd -P)
# Each review is immutable and preserves its real failure code.
trap 'code=$?; printf "%s\n" "$code" > "$output/review-exit.txt"' EXIT
reference="$control/source-manifest.json.gz"
python3 "$control/review.py" "$reports" "$reference" "$control/baseline-source.json.gz" "$control/baseline-registry.json.gz" "$prefix" > "$output/full-audit.json" 2> "$output/full-audit.error"
python3 - "$source_dir" "$reference" <<'PY' > "$output/live-source-audit.json"
import gzip,hashlib,json,subprocess,sys
from pathlib import Path
source=Path(sys.argv[1]);reference=json.loads(gzip.decompress(Path(sys.argv[2]).read_bytes()))
if subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()!='d1597d9a6987d0839ed2f0c7ded268aead35d071':raise SystemExit('wrong live revision')
paths=subprocess.check_output(['git','-C',str(source),'ls-files','-z']).split(b'\0')
actual={p.decode():hashlib.sha256((source/p.decode()).read_bytes()).hexdigest() for p in paths if p}
if actual!=reference:raise SystemExit('live source drift')
print(json.dumps({'liveSourceFilesMatched':len(actual)}))
PY
python3 "$control/eest-fixture-audit.py" "$source_dir" "$reference" "$reports/LastTest.log" > "$output/exact-eest-audit.json" 2> "$output/exact-eest-audit.error"
python3 "$control/network-review.py" "$source_dir" "$supplement" "$prefix/network" > "$output/network-audit.json" 2> "$output/network-audit.error"
# Program bytes are re-read from the retained build volume; no compilation.
python3 - "$reports/executables-after-tests.json" <<'PY' > "$output/program-paths.txt"
import json,sys
for p in sorted(json.load(open(sys.argv[1]))):print(p)
PY
docker run --rm -i --network none --read-only --user "$(id -u):$(id -g)" --cap-drop ALL --security-opt no-new-privileges --mount "type=volume,src=$volume,dst=/build,readonly" "$image" sh -c 'set -eu; while IFS= read -r path; do sha256sum "$path"; done' < "$output/program-paths.txt" > "$output/live-programs.sha256"
python3 - "$reports/executables-after-tests.json" "$output/live-programs.sha256" <<'PY' > "$output/live-program-audit.json"
import json,sys
reference=json.load(open(sys.argv[1]));actual={}
for line in open(sys.argv[2]):
 h,p=line.rstrip('\n').split('  ',1)
 if p in actual:raise SystemExit('duplicate live executable')
 actual[p]=h
if actual!=reference:raise SystemExit('live executable drift')
print(json.dumps({'liveProgramHashesMatched':len(actual),'wanStarted':False,'productionQualified':False}))
PY
