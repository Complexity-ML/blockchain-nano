#!/bin/bash
set -eu
arch=$1
out=/build/coherent-e33ad89e-$arch
trap 'echo $? > /probe/controller-exit.txt' EXIT
export CC=/usr/bin/clang CXX=/usr/bin/clang++
mkdir "$out"
run(){ local name=$1; shift; printf '%q ' "$@" > /probe/$name-command.txt; printf '\n' >> /probe/$name-command.txt; set +e; "$@" > /probe/$name.log 2>&1; local rc=$?; echo "$rc" > /probe/$name-exit.txt; set -e; echo "$name exit=$rc"; return "$rc"; }
run configure-network cmake -S /work/network -B "$out/network" -DCMAKE_BUILD_TYPE=Release -DNANO_DEPS_DIR=/src/.deps
run build-network cmake --build "$out/network" --target nano-p2p --parallel 2
run configure-core cmake -S /work -B "$out" -DCMAKE_BUILD_TYPE=Release -DMCL_PORTABLE=ON -DNANO_NETWORK_EXECUTABLE="$out/network/nano-p2p"
run build-core cmake --build "$out" --target nano nano-capella-transition-tests nano-capella-paused-capture nano-capella-deposit-audit --parallel 2
sha256sum "$out/nano" "$out/libnano_core.a" "$out/network/nano-p2p" "$out/nano-capella-transition-tests" "$out/nano-capella-paused-capture" "$out/nano-capella-deposit-audit" > /probe/programs.sha256
run ctest-inventory ctest --test-dir "$out" --show-only=json-v1
# Full source/core builds only in this controller. Run the affected qualification
# gates later against these exact binaries, without image rebuilds or relinking.
