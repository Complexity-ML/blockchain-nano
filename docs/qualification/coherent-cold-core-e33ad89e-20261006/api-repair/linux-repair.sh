#!/bin/bash
set -eu
arch=$1
base=/build/coherent-e33ad89e-$arch
out=/build/coherent-e33ad89e-api-repair-$arch
mkdir "$out"
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum "$base/libnano_core.a" "$base/nano" > /probe/core-before.sha256
cp -as /work /tmp/patched-source
rm /tmp/patched-source/tests/test_capella_transition.cpp
cp /fixed-test.cpp /tmp/patched-source/tests/test_capella_transition.cpp
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
cmd="/usr/bin/clang++ $flags -c /tmp/patched-source/tests/test_capella_transition.cpp -o $out/test.o"
printf '%s\n' "$cmd" > /probe/compile-command.txt
set +e; eval "$cmd" > /probe/compile.log 2>&1; rc=$?; echo "$rc" > /probe/compile-exit.txt; set -e; test "$rc" -eq 0
cmd=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/test.o}
cmd=${cmd//-o nano-capella-transition-tests /-o $out\/test }
printf '%s\n' "$cmd" > /probe/link-command.txt
(cd "$base"; eval "$cmd") > /probe/link.log 2>&1
sha256sum "$out/test" > /probe/program-before.sha256
set +e
env NANO_TEST_REFERENCE_PREFLIGHT=1 NANO_TEST_REFERENCE_VALIDATION_MEMO=1 NANO_TEST_COMPACT_REFERENCE_RESTORE=1 NANO_TEST_COMPACT_REFERENCE_WRITER=1 NANO_TEST_COLD_RECORD_CACHE=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 "$out/test" --prepared-block > /probe/writer-portability.log 2>&1
rc=$?; echo "$rc" > /probe/writer-portability-exit.txt; set -e; test "$rc" -eq 0
sha256sum -c /probe/core-before.sha256 > /probe/core-unchanged.log
sha256sum -c /probe/program-before.sha256 > /probe/program-unchanged.log
