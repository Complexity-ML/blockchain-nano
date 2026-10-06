#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/checkpoint-blobs-$arch-20261006
mkdir "$out"
trap 'echo $? > /probe/controller-exit.txt' EXIT
parent=/build/catalog-reactor-failfast-$arch-20261006/libnano_core.a
sha256sum "$parent" > /probe/parent-before.sha256
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/third_party/secp256k1/include -I/source/third_party/ckzg/blst/bindings $flags -MMD -MF /probe/storage.d -c /source/src/storage.cpp -o $out/storage.cpp.o"
printf '%s\n' "$cmd" > /probe/compile-storage-command.txt
eval "$cmd" > /probe/compile-storage.log 2>&1
cp "$parent" "$out/libnano_core.a"
ar r "$out/libnano_core.a" "$out/storage.cpp.o"
ranlib "$out/libnano_core.a"
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-execution-checkpoint-sync-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network -I/source/tests $flags -MMD -MF /probe/test.d -c /source/tests/test_checkpoint_blobs.cpp -o $out/test.o"
printf '%s\n' "$cmd" > /probe/compile-test-command.txt
eval "$cmd" > /probe/compile-test.log 2>&1
cmd=$(cat "$base/CMakeFiles/nano-execution-checkpoint-sync-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-execution-checkpoint-sync-tests.dir\/tests\/test_execution_checkpoint.cpp.o/$out\/test.o}
cmd=${cmd//-o nano-execution-checkpoint-sync-tests /-o $out\/test }
cmd=${cmd// libnano_core.a / $out\/libnano_core.a }
printf '%s\n' "$cmd" > /probe/link-command.txt
(cd "$base";eval "$cmd") > /probe/link.log 2>&1
"$out/test" > /probe/test-records.log 2>&1
echo 0 > /probe/test-records-exit.txt
"$out/test" --sync > /probe/test-sync.log 2>&1
echo 0 > /probe/test-sync-exit.txt
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum "$out/test" "$out/libnano_core.a" > /probe/programs.sha256
printf 'immutable checkpoint blobs %s: records and sync passed\n' "$arch"
