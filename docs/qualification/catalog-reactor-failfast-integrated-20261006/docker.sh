#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/catalog-reactor-failfast-$arch-20261006
mkdir "$out"
trap 'echo $? > "$out/controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/work\/source\/}
for name in consensus_capella_reactor rpc consensus_capella_runtime; do
 cmd="/usr/bin/clang++ -I/work/source/include -I/work/source/network -I/work/source/third_party/secp256k1/include -I/work/source/third_party/ckzg/blst/bindings $flags -MMD -MF $out/$name.d -c /work/source/src/$name.cpp -o $out/$name.cpp.o"
 printf '%s\n' "$cmd" > "$out/compile-$name-command.txt"
 eval "$cmd" > "$out/compile-$name.log" 2>&1
 echo "compiled $name"
done
parent=/build/catalog-reservation-integrated-$arch-20261006/libnano_core.a
sha256sum "$parent" > "$out/parent-before.sha256"
cp "$parent" "$out/libnano_core.a"
ar r "$out/libnano_core.a" "$out"/*.cpp.o
ranlib "$out/libnano_core.a"
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/work\/source\/}
cmd="/usr/bin/clang++ -I/work/source/include -I/work/source/network -I/work/source/third_party/ckzg/blst/bindings $flags -MMD -MF $out/candidate.d -c /work/source/tests/test_capella_transition.cpp -o $out/candidate.o"
printf '%s\n' "$cmd" > "$out/compile-test-command.txt"
eval "$cmd" > "$out/compile-test.log" 2>&1
cmd=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/candidate.o}
cmd=${cmd//-o nano-capella-transition-tests /-o $out\/candidate }
cmd=${cmd// libnano_core.a / $out\/libnano_core.a }
printf '%s\n' "$cmd" > "$out/link-command.txt"
(cd "$base";eval "$cmd") > "$out/link.log" 2>&1
for mode in reactor-failfast generation-reservation capture-control worker-control worker-http; do
 envvar=NANO_TEST_CATALOG_REACTOR_FAILFAST;arg=--beacon-reads
 [ "$mode" != generation-reservation ] || envvar=NANO_TEST_CATALOG_GENERATION_RACE
 [ "$mode" != capture-control ] || envvar=NANO_TEST_CATALOG_CAPTURE_UNLOCKED
 [ "$mode" != worker-control ] || envvar=NANO_TEST_CATALOG_WORKER_FAILFAST
 if [ "$mode" = worker-http ];then envvar=NANO_TEST_CATALOG_WORKER_FAILFAST;arg=--queued-bls-rpc;fi
 env "$envvar=1" "$out/candidate" "$arg" > "$out/test-$mode.log" 2>&1
 echo 0 > "$out/test-$mode-exit.txt"
 echo "passed $mode"
done
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum "$out/candidate" "$out/libnano_core.a" > "$out/programs.sha256"
mkdir /work/$arch
cp "$out"/*.log "$out"/*.txt "$out"/*.d "$out"/*.sha256 /work/$arch/
