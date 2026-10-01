#!/usr/bin/env bash
# Full configured C++ registry, not the smaller daily CI selection.
set -euo pipefail
for tool in git cmake ctest node python3; do command -v "$tool" >/dev/null || { echo "Missing full-run prerequisite: $tool" >&2; exit 1; }; done
python3 -c 'import hashlib; assert hasattr(hashlib,"file_digest"), "Python 3.11+ required"'
build_dir=${1:-build-full}
reports=${2:-full-reports}
: "${NANO_NETWORK_EXECUTABLE:?Provide the existing nano-p2p executable}"
test -x "$NANO_NETWORK_EXECUTABLE"
mkdir -p "$build_dir" "$reports"
build_dir=$(cd "$build_dir" && pwd)
reports=$(cd "$reports" && pwd)
mkdir -p "$build_dir/qualification-scratch"
export TMPDIR="$build_dir/qualification-scratch"
# Installed-binary fixtures need executable scratch, not a noexec tmpfs.
probe=$(mktemp "$TMPDIR/execution-probe.XXXXXX")
trap 'rm -f "$probe"' EXIT
printf '#!/bin/sh\nexit 0\n' > "$probe"
chmod 700 "$probe"
"$probe"
rm -f "$probe"
printf '%s\n' "$TMPDIR" > "$reports/scratch-path.txt"
df -Pk "$TMPDIR" > "$reports/scratch-filesystem.txt"
git -c safe.directory="$PWD" rev-parse HEAD > "$reports/source-revision.txt"
git -c safe.directory="$PWD" diff --binary > "$reports/source.patch"
cmake --version > "$reports/cmake-version.txt"
"${CXX:-c++}" --version > "$reports/compiler-version.txt"
if command -v dpkg-query >/dev/null; then dpkg-query -W > "$reports/build-packages.txt"; fi
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE="$NANO_NETWORK_EXECUTABLE" 2>&1 | tee "$reports/configure.log"
cmake --build "$build_dir" --parallel 2 2>&1 | tee "$reports/build.log"
ctest --test-dir "$build_dir" --show-only=json-v1 > "$reports/registry.json"
node scripts/ci/regression-inventory.mjs "$reports/registry.json" "$reports/registry.json" --require-complete > "$reports/inventory.json"
python3 - "$reports/inventory.json" "$NANO_NETWORK_EXECUTABLE" > "$reports/executables.sha256" <<'PY'
import hashlib,json,sys
paths={t['command'][0] for t in json.load(open(sys.argv[1]))['tests']}
paths.add(sys.argv[2])
for path in sorted(paths):
    with open(path,'rb') as f: digest=hashlib.file_digest(f,'sha256').hexdigest()
    print(digest+'  '+path)
PY
set +e
ctest --test-dir "$build_dir" --no-tests=error --parallel 1 --output-on-failure --output-junit "$reports/results.xml" 2>&1 | tee "$reports/tests.log"
status=${PIPESTATUS[0]}
set -e
printf '%s\n' "$status" > "$reports/ctest-exit.txt"
cp "$build_dir/Testing/Temporary/LastTest.log" "$reports/LastTest.log"
# A successful CTest exit alone must not hide missing/skipped result entries.
bash scripts/ci/verify-results.sh "$reports/registry.json" "$reports/results.xml" > "$reports/result-audit.json"
exit "$status"
