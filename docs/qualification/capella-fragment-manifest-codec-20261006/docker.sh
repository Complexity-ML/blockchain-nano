#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/checkpoint-blobs-$arch-20261006
trap 'echo $? > /probe/controller-exit.txt' EXIT
sha256sum "$out/libnano_core.a" > /probe/parent-before.sha256
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-execution-checkpoint-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
cmd="/usr/bin/clang++ -I/source/include -I/source/network $flags -MMD -MF /probe/manifest.d -c /source/tests/test_checkpoint_fragment_manifest.cpp -o /probe/manifest.o"
printf '%s\n' "$cmd" > /probe/compile-command.txt
eval "$cmd" > /probe/compile.log 2>&1
cmd=$(cat "$base/CMakeFiles/nano-execution-checkpoint-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-execution-checkpoint-tests.dir\/tests\/test_execution_checkpoint.cpp.o/\/probe\/manifest.o}
cmd=${cmd//-o nano-execution-checkpoint-tests /-o \/probe\/test }
cmd=${cmd// libnano_core.a / $out\/libnano_core.a }
printf '%s\n' "$cmd" > /probe/link-command.txt
(cd "$base";eval "$cmd") > /probe/link.log 2>&1
/probe/test > /probe/test.log 2>&1
echo 0 > /probe/test-exit.txt
sha256sum -c /probe/parent-before.sha256 > /probe/parent-unchanged.log
sha256sum /probe/test > /probe/program.sha256
printf '%s manifest codec passed\n' "$arch"
