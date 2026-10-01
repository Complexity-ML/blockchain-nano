#!/usr/bin/env bash
# Full configured C++ registry, not the smaller daily CI selection.
set -euo pipefail
for tool in git cmake ctest node python3; do command -v "$tool" >/dev/null || { echo "Missing full-run prerequisite: $tool" >&2; exit 1; }; done
python3 -c 'import hashlib; assert hasattr(hashlib,"file_digest"), "Python 3.11+ required"'
build_dir=${1:-build-full}
reports=${2:-ci-reports/full}
: "${NANO_NETWORK_EXECUTABLE:?Provide the existing nano-p2p executable}"
test -x "$NANO_NETWORK_EXECUTABLE"
revision=$(git -c safe.directory="$PWD" rev-parse HEAD)
test -z "$(git status --porcelain)" || { echo 'Full qualification requires a clean source checkout' >&2; exit 1; }
mkdir -p "$build_dir" "$reports"
test -z "$(ls -A "$reports")" || { echo 'Reports must be empty to preserve earlier evidence' >&2; exit 1; }
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
printf '%s\n' "$revision" > "$reports/source-revision.txt"
snapshot_sources(){ git ls-files -z | python3 -c 'import sys,hashlib,json; paths=sys.stdin.buffer.read().split(b"\0"); print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in paths if p},sort_keys=True))'; }
snapshot_sources > "$reports/source-manifest.json"
git -c safe.directory="$PWD" diff --binary > "$reports/source.patch"
cmake --version > "$reports/cmake-version.txt"
"${CXX:-c++}" --version > "$reports/compiler-version.txt"
if command -v dpkg-query >/dev/null; then dpkg-query -W > "$reports/build-packages.txt"; fi
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE="$NANO_NETWORK_EXECUTABLE" 2>&1 | tee "$reports/configure.log"
cmake --build "$build_dir" --parallel 2 2>&1 | tee "$reports/build.log"
ctest --test-dir "$build_dir" --show-only=json-v1 > "$reports/registry.json"
node scripts/ci/regression-inventory.mjs "$reports/registry.json" "$reports/registry.json" --require-complete > "$reports/inventory.json"
snapshot_executables(){ python3 - "$reports/inventory.json" "$NANO_NETWORK_EXECUTABLE" <<'PY'
import hashlib,json,sys
paths={t['command'][0] for t in json.load(open(sys.argv[1]))['tests']}
paths.add(sys.argv[2])
for path in sorted(paths):
    with open(path,'rb') as f: digest=hashlib.file_digest(f,'sha256').hexdigest()
    print(digest+'  '+path)
PY
}
snapshot_executables > "$reports/executables.sha256"
set +e
ctest --test-dir "$build_dir" --no-tests=error --parallel 1 --output-on-failure --output-junit "$reports/results.xml" 2>&1 | tee "$reports/tests.log"
status=${PIPESTATUS[0]}
set -e
printf '%s\n' "$status" > "$reports/ctest-exit.txt"
cp "$build_dir/Testing/Temporary/LastTest.log" "$reports/LastTest.log"
# A successful CTest exit alone must not hide missing/skipped result entries.
audit_status=0
if ! bash scripts/ci/verify-results.sh "$reports/registry.json" "$reports/results.xml" > "$reports/result-audit.json" 2> "$reports/result-audit.error"; then audit_status=1; fi
if ! node scripts/ci/audit-eest-ctest.mjs "$reports/LastTest.log" > "$reports/eest-counts.json" 2> "$reports/eest-counts.error"; then audit_status=1; fi
snapshot_executables > "$reports/executables-after.sha256"
if ! cmp "$reports/executables.sha256" "$reports/executables-after.sha256" > "$reports/executable-compare.log" 2>&1; then audit_status=1; fi
snapshot_sources > "$reports/source-manifest-after-tests.json"
if ! cmp "$reports/source-manifest.json" "$reports/source-manifest-after-tests.json" > "$reports/source-compare.log" 2>&1; then audit_status=1; fi
if ! test "$(git rev-parse HEAD)" = "$revision"; then audit_status=1; fi
printf '%s\n' "$audit_status" > "$reports/audit-exit.txt"
if (( audit_status != 0 )); then exit 1; fi
exit "$status"
