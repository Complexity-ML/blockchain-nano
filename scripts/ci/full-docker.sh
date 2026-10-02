#!/usr/bin/env bash
# Host owns audits; the existing builder needs only its current C++/CMake tools.
set -euo pipefail
for tool in git docker node python3; do command -v "$tool" >/dev/null || { echo "Missing host prerequisite: $tool" >&2; exit 1; }; done
: "${NANO_BUILDER_IMAGE:?Provide the existing builder image digest}"
: "${NANO_BUILD_VOLUME:?Provide the existing build volume}"
[[ "$NANO_BUILDER_IMAGE" =~ ^sha256:[a-f0-9]{64}$ ]]
[[ "$NANO_BUILD_VOLUME" =~ ^[A-Za-z0-9][A-Za-z0-9_.-]*$ ]]
docker image inspect "$NANO_BUILDER_IMAGE" >/dev/null
docker volume inspect "$NANO_BUILD_VOLUME" >/dev/null
source_dir=$(pwd -P)
revision=$(git rev-parse HEAD)
test -z "$(git status --porcelain)" || { echo 'Full qualification requires a clean source checkout' >&2; exit 1; }
subdir=${NANO_BUILD_SUBDIR:-full-qualification-${revision:0:12}}
[[ "$subdir" =~ ^[A-Za-z0-9][A-Za-z0-9_.-]*$ ]]
reports=${1:?Provide an empty report directory outside the source checkout}
mkdir -p "$reports"
reports=$(cd "$reports" && pwd)
test -z "$(ls -A "$reports")" || { echo 'Reports must be empty to preserve earlier evidence' >&2; exit 1; }
case "$reports/" in "$source_dir/"*) echo 'Reports must be outside source checkout' >&2; exit 1;; esac
printf '%s\n' "$revision" > "$reports/source-revision.txt"
snapshot_sources(){ git ls-files -z | python3 -c 'import sys,hashlib,json; paths=sys.stdin.buffer.read().split(b"\0"); print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in paths if p},sort_keys=True))'; }
snapshot_sources > "$reports/source-manifest.json"
args=(-i --rm --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount "type=volume,src=$NANO_BUILD_VOLUME,dst=/build" --workdir /work --env "NANO_BUILD_SUBDIR=$subdir")
docker run --name "nano-full-$subdir-prepare" "${args[@]}" "$NANO_BUILDER_IMAGE" bash -s <<'SH' 2>&1 | tee "$reports/prepare.log"
set -euo pipefail
build=/build/$NANO_BUILD_SUBDIR
mkdir -p "$build/qualification-scratch"
export TMPDIR="$build/qualification-scratch"
probe=$(mktemp "$TMPDIR/probe.XXXXXX")
trap 'rm -f "$probe"' EXIT
printf '#!/bin/sh\nexit 0\n' > "$probe"; chmod 700 "$probe"; "$probe"; rm -f "$probe"
df -Pk "$TMPDIR" > /reports/scratch-filesystem.txt
cmake --version > /reports/cmake-version.txt
c++ --version > /reports/compiler-version.txt
dpkg-query -W > /reports/build-packages.txt
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p
cmake --build "$build" --parallel 2
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P scripts/ci/docker-manifest.cmake
SH
node scripts/ci/docker-inventory.mjs "$reports/registry.json" "$reports/executables.json" > "$reports/inventory.json"
snapshot_sources > "$reports/source-manifest-after-build.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-build.json"
test "$(git rev-parse HEAD)" = "$revision"
set +e
docker run --name "nano-full-$subdir-test" "${args[@]}" "$NANO_BUILDER_IMAGE" bash -s <<'SH' 2>&1 | tee "$reports/tests-container.log"
set -euo pipefail
build=/build/$NANO_BUILD_SUBDIR
export TMPDIR="$build/qualification-scratch"
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry-before-tests.json
cmp /reports/registry.json /reports/registry-before-tests.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P scripts/ci/docker-manifest.cmake
cmp /reports/executables.json /reports/executables-before-tests.json
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
status=${PIPESTATUS[0]}
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
printf '%s\n' "$status" > /reports/ctest-exit.txt
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-after-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P scripts/ci/docker-manifest.cmake
manifest_status=$?
printf '%s\n' "$manifest_status" > /reports/post-test-manifest-exit.txt
if (( manifest_status != 0 )); then exit 1; fi
exit "$status"
SH
status=${PIPESTATUS[0]}
set -e
# Preserve all post-run audits even when CTest or an earlier audit failed.
audit_status=0
if ! bash scripts/ci/verify-results.sh "$reports/registry.json" "$reports/results.xml" > "$reports/result-audit.json" 2> "$reports/result-audit.error"; then audit_status=1; fi
if ! node scripts/ci/audit-eest-ctest.mjs "$reports/LastTest.log" > "$reports/eest-counts.json" 2> "$reports/eest-counts.error"; then audit_status=1; fi
snapshot_sources > "$reports/source-manifest-after-tests.json"
if ! cmp "$reports/source-manifest.json" "$reports/source-manifest-after-tests.json" > "$reports/source-compare.log" 2>&1; then audit_status=1; fi
if ! cmp "$reports/executables-before-tests.json" "$reports/executables-after-tests.json" > "$reports/executable-compare.log" 2>&1; then audit_status=1; fi
if ! test "$(git rev-parse HEAD)" = "$revision"; then audit_status=1; fi
printf '%s\n' "$audit_status" > "$reports/audit-exit.txt"
if (( audit_status != 0 )); then exit 1; fi
exit "$status"
