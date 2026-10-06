#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/fragment-runtime-$arch-20261006
mkdir "$out"
trap 'echo $? > /probe/controller-exit.txt' EXIT
parent=/build/checkpoint-blobs-$arch-20261006/libnano_core.a
sha256sum "$parent" > /probe/parent-before.sha256
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
for name in chain_beacon consensus_capella_reactor consensus_capella_runtime;do
 cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/secp256k1/include -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/$name.d -c /source/src/$name.cpp -o $out/$name.cpp.o"
 printf '%s\n' "$cmd" > /probe/compile-$name-command.txt
 eval "$cmd" > /probe/compile-$name.log 2>&1
 echo "compiled $name"
done
cp "$parent" "$out/libnano_core.a"
ar r "$out/libnano_core.a" "$out"/*.cpp.o
ranlib "$out/libnano_core.a"
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/test.d -c /source/tests/test_capella_transition.cpp -o $out/test.o"
printf '%s\n' "$cmd" > /probe/compile-test-command.txt
eval "$cmd" > /probe/compile-test.log 2>&1
cmd=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/test.o}
cmd=${cmd//-o nano-capella-transition-tests /-o $out\/test }
cmd=${cmd// libnano_core.a / $out\/libnano_core.a }
printf '%s\n' "$cmd" > /probe/link-command.txt
(cd "$base";eval "$cmd") > /probe/link.log 2>&1
NANO_TEST_FRAGMENT_CHECKPOINT=1 "$out/test" --prepared-block > /probe/test-fragment.log 2>&1
echo 0 > /probe/test-fragment-exit.txt
for mode in owner-witness configured-runtime automatic-recovery;do
 "$out/test" --"$mode" > /probe/test-$mode.log 2>&1
 echo 0 > /probe/test-$mode-exit.txt
 echo "passed $mode"
done
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum "$out/test" "$out/libnano_core.a" > /probe/programs.sha256
printf '%s fragment runtime integrated tests passed\n' "$arch"
