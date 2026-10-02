#!/usr/bin/env bash
set -euo pipefail
build=/build/production-evm-c10426b2-20261001
mkdir -p "$build/qualification-scratch"; export TMPDIR="$build/qualification-scratch"
probe=$(mktemp "$TMPDIR/probe.XXXXXX");printf '#!/bin/sh\nexit 0\n' > "$probe";chmod 700 "$probe";"$probe";rm "$probe"
df -Pk "$TMPDIR" > /reports/scratch-filesystem.txt
cmake --version > /reports/cmake-version.txt
c++ --version > /reports/compiler-version.txt
dpkg-query -W > /reports/build-packages.txt
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 2 > /reports/build.log 2>&1
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -P /work/scripts/ci/docker-manifest.cmake
