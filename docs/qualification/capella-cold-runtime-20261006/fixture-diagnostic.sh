#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/cold-runtime-$arch-20261006
test -d "$out"
trap 'echo $? > /probe/build-controller-exit.txt' EXIT
parent=/build/cold-reactor-$arch-20261006/libnano_core.a
sha256sum "$parent" > /probe/parent-before.sha256
printf '%s\n' consensus_capella_reactor consensus_capella_runtime > /probe/abi-consumers.txt
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
compile_one(){
 local name=$1
 local cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/secp256k1/include -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/$name.d -c /source/src/$name.cpp -o $out/$name.cpp.o"
 printf '%s\n' "$cmd" > /probe/compile-$name-command.txt
 set +e
 eval "$cmd" > /probe/compile-$name.log 2>&1
 local result=$?
 echo "$result" > /probe/compile-$name-exit.txt
 echo "compiled $name exit=$result"
 set -e
 return "$result"
}
for name in consensus_capella_reactor consensus_capella_runtime;do test "$(cat /initial/compile-$name-exit.txt)" = 0;done
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
sha256sum "$out/test" "$out/libnano_core.a" > /probe/programs-before.sha256
run_case(){
 local name=$1;shift
 set +e
 "$@" > /probe/test-$name.log 2>&1
 local result=$?
 echo "$result" > /probe/test-$name-exit.txt
 set -e
 echo "finished $name exit=$result"
 return "$result"
}
run_case cold-automatic env NANO_TEST_COLD_RUNTIME=1 "$out/test" --automatic-recovery
run_case cold-bls env NANO_TEST_COLD_RUNTIME=1 "$out/test" --queued-bls-rpc
run_case warm-pruning "$out/test" --pruning-automatic
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum -c /probe/programs-before.sha256 > /probe/programs-unchanged.log
