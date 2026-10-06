#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/cold-record-$arch-20261006
mkdir /probe/initial-fixture-shape-failure
cp /probe/controller-exit.txt /probe/test-cold.log /probe/initial-fixture-shape-failure/
trap 'echo $? > /probe/repair-controller-exit.txt' EXIT
sha256sum "$out/libnano_core.a" > /probe/cold-core-before.sha256
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
NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_RECORD_CACHE=1 "$out/test" --prepared-block > /probe/test-cold.log 2>&1
echo 0 > /probe/test-cold-exit.txt
sha256sum -c /probe/cold-core-before.sha256 > /probe/cold-core-unchanged.log
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum "$out/test" "$out/libnano_core.a" > /probe/programs.sha256
printf '%s fragment runtime integrated tests passed\n' "$arch"
