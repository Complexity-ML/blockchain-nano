#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
parent=/build/reference-memo-runtime-$arch-20261006
shim=/build/cold-memo-sigkill-$arch-20261006/libnano-test-sync-stop.so
out=/build/reference-preflight-$arch-20261006
mkdir "$out"
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum "$parent/libnano_core.a" "$parent/test" "$shim" > /probe/parent-before.sha256
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
compile_one(){
 local name=$1
 local cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/secp256k1/include -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/$name.d -c /source/src/$name.cpp -o $out/$name.cpp.o"
 printf '%s\n' "$cmd" > /probe/compile-$name-command.txt
 set +e; eval "$cmd" > /probe/compile-$name.log 2>&1; local result=$?;echo "$result" > /probe/compile-$name-exit.txt;set -e
 echo "compiled $name exit=$result";return "$result"
}
compile_one consensus_capella_store_snapshot & first=$!
compile_one consensus_capella_reactor & second=$!
failed=0;wait "$first" || failed=1;wait "$second" || failed=1;test "$failed" -eq 0
cp "$parent/libnano_core.a" "$out/libnano_core.a";ar r "$out/libnano_core.a" "$out"/*.cpp.o;ranlib "$out/libnano_core.a"
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/test.d -c /source/tests/test_capella_transition.cpp -o $out/test.o"
printf '%s\n' "$cmd" > /probe/compile-test-command.txt
set +e;eval "$cmd" > /probe/compile-test.log 2>&1;result=$?;echo "$result" > /probe/compile-test-exit.txt;set -e;test "$result" -eq 0
cmd=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/test.o}
cmd=${cmd//-o nano-capella-transition-tests /-o $out\/test }
cmd=${cmd// libnano_core.a / $out\/libnano_core.a }
cmd=${cmd// libnano-test-sync-fault.so / $shim }
printf '%s\n' "$cmd" > /probe/link-command.txt
(cd "$base";eval "$cmd") > /probe/link.log 2>&1
sha256sum "$out/test" "$out/libnano_core.a" > /probe/program-before.sha256
run_case(){local name=$1;shift;set +e;"$@" > /probe/test-$name.log 2>&1;local result=$?;echo "$result" > /probe/test-$name-exit.txt;set -e;echo "finished $name exit=$result";return "$result";}
run_case writer-preflight env NANO_TEST_REFERENCE_PREFLIGHT=1 NANO_TEST_REFERENCE_VALIDATION_MEMO=1 NANO_TEST_COMPACT_REFERENCE_RESTORE=1 NANO_TEST_COMPACT_REFERENCE_WRITER=1 NANO_TEST_COLD_RECORD_CACHE=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 "$out/test" --prepared-block
run_case cold-faults env NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_REACTOR=1 NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_MEMO_SYNC=1 NANO_TEST_COLD_MEMO_SIGKILL=1 NANO_TEST_DUPLICATE_GUARDS=1 "$out/test" --prepared-block
run_case cold-automatic env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 "$out/test" --automatic-recovery
run_case cold-bls env NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_RUNTIME=1 "$out/test" --queued-bls-rpc
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum -c /probe/program-before.sha256 > /probe/program-unchanged.log
