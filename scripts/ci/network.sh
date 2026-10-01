#!/usr/bin/env bash
set -euo pipefail
build_dir=${1:-build-ci-network}
mkdir -p ci-reports
git -c safe.directory="$PWD" rev-parse HEAD > ci-reports/source-revision.txt
cmake --version > ci-reports/cmake-version.txt
"${CXX:-c++}" --version > ci-reports/compiler-version.txt
# Local qualification reuses an existing dependency builder; fresh CI builds once.
# The dependency script pins cpp-libp2p and disables Hunter uploads.
deps=${NANO_DEPS_DIR:-"$PWD/.deps"}
case "${NANO_CI_REUSE_DEPS:-0}" in
 0) JOBS=2 sh scripts/build-libp2p.sh 2>&1 | tee ci-reports/network-dependencies.log ;;
 1) test -f "$deps/install/lib/cmake/libp2p/libp2pConfig.cmake"
    test -f "$deps/libp2p-build/CMakeCache.txt"
    printf 'Reused prebuilt dependency prefix; no dependency build performed.\n' | tee ci-reports/network-dependencies.log ;;
 *) echo 'Invalid dependency reuse mode' >&2; exit 1 ;;
esac
git -c safe.directory="$deps/cpp-libp2p" -C "$deps/cpp-libp2p" rev-parse HEAD > ci-reports/libp2p-revision.txt
cmake -DNANO_CI_CHECK=pin -P scripts/ci/check-network.cmake
cmake -S network -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DNANO_DEPS_DIR="$deps" 2>&1 | tee ci-reports/network-configure.log
cmake --build "$build_dir" --parallel 2 2>&1 | tee ci-reports/network-build.log
ctest --test-dir "$build_dir" --show-only=json-v1 > ci-reports/network-selection.json
cmake -DNANO_CI_CHECK=registry -P scripts/ci/check-network.cmake
ctest --test-dir "$build_dir" --output-on-failure --no-tests=error --parallel 1 --timeout 60 --output-junit "$PWD/ci-reports/network.xml" 2>&1 | tee ci-reports/network-tests.log
sha256sum "$build_dir/nano-p2p" > ci-reports/network-executable.sha256
if command -v dpkg-query >/dev/null; then dpkg-query -W > ci-reports/build-packages.txt; fi
