#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/cold-concurrency-$arch-20261006
trap 'echo $? > /probe/fixture-v2-controller-exit.txt' EXIT
sha256sum "$out/libnano_core.a" > /probe/core-before-v2.sha256
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/test-v2.d -c /source/tests/test_capella_transition.cpp -o $out/test-v2.o"
printf '%s\n' "$cmd" > /probe/compile-fixture-v2-command.txt
eval "$cmd" > /probe/compile-fixture-v2.log 2>&1
cmd=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/test-v2.o}
cmd=${cmd//-o nano-capella-transition-tests /-o $out\/test-v2 }
cmd=${cmd// libnano_core.a / $out\/libnano_core.a }
(cd "$base";eval "$cmd") > /probe/link-v2.log 2>&1
NANO_TEST_COLD_CACHE_CONCURRENCY=1 NANO_TEST_FRAGMENT_CHECKPOINT=1 NANO_TEST_COLD_RECORD_CACHE=1 "$out/test-v2" --prepared-block > /probe/test-cold-graph-v2.log 2>&1
sha256sum "$out/test-v2" > /probe/program-v2.sha256
sha256sum -c /probe/core-before-v2.sha256 > /probe/core-unchanged-v2.log
