#!/bin/bash
set -eu
root=/private/tmp/nano-coherent-e33ad89e-20261006
out=$root/native-build
probe=$root/native
trap 'echo $? > "$probe/controller-exit.txt"' EXIT
export CC=/usr/bin/clang CXX=/usr/bin/clang++
run(){ local name=$1; shift; printf '%q ' "$@" > "$probe/$name-command.txt"; printf '\n' >> "$probe/$name-command.txt"; set +e; "$@" > "$probe/$name.log" 2>&1; local rc=$?; echo "$rc" > "$probe/$name-exit.txt"; set -e; echo "$name exit=$rc"; return "$rc"; }
run configure-core /opt/homebrew/bin/cmake -S "$root/source" -B "$out" -DCMAKE_BUILD_TYPE=Release -DMCL_PORTABLE=ON -DNANO_NETWORK_EXECUTABLE=/private/tmp/nano-catalog-busy-integrated-native-20261006/network/nano-p2p
run build-core /opt/homebrew/bin/cmake --build "$out" --target nano nano-capella-transition-tests nano-capella-paused-capture nano-capella-deposit-audit --parallel 2
shasum -a 256 "$out/nano" "$out/libnano_core.a" /private/tmp/nano-catalog-busy-integrated-native-20261006/network/nano-p2p "$out/nano-capella-transition-tests" "$out/nano-capella-paused-capture" "$out/nano-capella-deposit-audit" > "$probe/programs.sha256"
run ctest-inventory /opt/homebrew/bin/ctest --test-dir "$out" --show-only=json-v1
