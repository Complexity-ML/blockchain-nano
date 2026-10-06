#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
parent=/build/reference-memo-runtime-$arch-20261006
out=/build/cold-memo-sigkill-$arch-20261006
mkdir -p "$out"
trap 'echo $? > /probe/controller-exit.txt' EXIT
/usr/bin/clang++ -std=c++20 -fPIC -shared /faultsource/tests/support/sync_fault_linux.cpp -ldl -o "$out/libnano-test-sync-stop.so" > /probe/compile-shim.log 2>&1
sha256sum "$out/libnano-test-sync-stop.so" > /probe/shim-before.sha256
sha256sum "$parent/libnano_core.a" "$parent/test" > /probe/parent-before.sha256
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/test.d -c /faultsource/tests/test_capella_transition.cpp -o $out/test.o"
printf '%s\n' "$cmd" > /probe/compile-command.txt
set +e
eval "$cmd" > /probe/compile.log 2>&1
result=$?; echo "$result" > /probe/compile-exit.txt
set -e
[ "$result" -eq 0 ]
cmd=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/test.o}
cmd=${cmd//-o nano-capella-transition-tests /-o $out\/test }
cmd=${cmd// libnano_core.a / $parent\/libnano_core.a }
cmd=${cmd// libnano-test-sync-fault.so / $out\/libnano-test-sync-stop.so }
printf '%s\n' "$cmd" > /probe/link-command.txt
(cd "$base";eval "$cmd") > /probe/link.log 2>&1
sha256sum "$out/test" > /probe/program-before.sha256
set +e
env NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_REACTOR=1 NANO_TEST_RUNTIME_REFERENCE_MEMO=1 NANO_TEST_COLD_MEMO_SYNC=1 NANO_TEST_COLD_MEMO_SIGKILL=1 "$out/test" --prepared-block > /probe/test.log 2>&1
result=$?;echo "$result" > /probe/test-exit.txt
set -e
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum -c /probe/program-before.sha256 > /probe/program-unchanged.log
sha256sum -c /probe/shim-before.sha256 > /probe/shim-unchanged.log
exit "$result"
