#!/usr/bin/env bash
set -euo pipefail
git -c safe.directory=/work -C /work rev-parse HEAD > /reports/source-revision.txt
sha256sum /work/CMakeLists.txt /work/tests/test_rpc.cpp > /reports/changed-source.sha256
cmake --build /build --parallel 2 2>&1 | tee /reports/build.log
ctest --test-dir /build --show-only=json-v1 > /reports/registry.json
cmake -P /reports/check.cmake 2>&1 | tee /reports/registry-check.log
sha256sum /build/nano /build/network/nano-p2p > /reports/runtime.sha256
ctest --test-dir /build --output-on-failure --no-tests=error --parallel 1 --output-junit /reports/results.xml 2>&1 | tee /reports/tests.log
