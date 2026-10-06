#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/cold-concurrency-$arch-20261006
mkdir "$out"
trap 'echo $? > /probe/build-controller-exit.txt' EXIT
parent=/build/record-leases-$arch-20261006/libnano_core.a
sha256sum "$parent" > /probe/parent-before.sha256
printf '%s\n' consensus_capella_cold_records consensus_capella_record_lease > /probe/abi-consumers.txt
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
 return "$result"
}
pids=();failed=0
while read -r name;do
 compile_one "$name" & pids+=($!)
 if [ ${#pids[@]} -eq 2 ];then for pid in "${pids[@]}";do wait "$pid" || failed=1;done;pids=();fi
done < /probe/abi-consumers.txt
for pid in "${pids[@]}";do wait "$pid" || failed=1;done
[ "$failed" -eq 0 ]
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
NANO_TEST_COLD_CACHE_CONCURRENCY=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_RECORD_CACHE=1 "$out/test" --prepared-block > /probe/test-cold-graph.log 2>&1
echo 0 > /probe/test-cold-graph-exit.txt
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum -c /probe/programs-before.sha256 > /probe/programs-unchanged.log
echo "$arch leases core qualified"
