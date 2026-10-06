#!/bin/bash
set -eu
arch=$1
out=/build/private-capture-registration-$arch-20261006
trap 'echo $? > /probe/controller-exit.txt' EXIT
cp -as /work /tmp/capture-config-source
rm /tmp/capture-config-source/CMakeLists.txt
cp /patch/CMakeLists.candidate.txt /tmp/capture-config-source/CMakeLists.txt
cp /patch/source/tests/capella/private_capture_budget.hpp /patch/source/tests/capella/test_private_capture_budget.cpp /tmp/capture-config-source/tests/capella/
CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -S /tmp/capture-config-source -B "$out" -DCMAKE_BUILD_TYPE=Release -DMCL_PORTABLE=ON -DNANO_NETWORK_EXECUTABLE=/build/coherent-e33ad89e-$arch/network/nano-p2p > /probe/configure.log 2>&1
cmake --build "$out" --target nano-private-capture-budget-tests --parallel 2 > /probe/build.log 2>&1
ctest --test-dir "$out" --show-only=json-v1 -R '^consensus_private_capture_resource_budget$' > /probe/inventory.json
ctest --test-dir "$out" --output-on-failure -R '^consensus_private_capture_resource_budget$' > /probe/ctest.log 2>&1
cp "$out/Testing/Temporary/LastTest.log" /probe/LastTest.log
