#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/checkpoint-blobs-$arch-20261006
mkdir /probe/regression-initial-old-object
for f in /probe/regression-*.txt /probe/regression-*.log;do [ ! -f "$f" ] || cp "$f" /probe/regression-initial-old-object/;done
trap 'echo $? > /probe/regression-controller-exit.txt' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-execution-checkpoint-tests.dir/flags.make" | tr '\n' ' ')
flags=${flags//\/work\//\/source\/}
compile="/usr/bin/clang++ -I/source/include -I/source/network $flags -MMD -MF /probe/regression.d -c /source/tests/test_execution_checkpoint.cpp -o $out/regression.o"
printf '%s\n' "$compile" > /probe/regression-compile-command.txt
eval "$compile" > /probe/regression-compile.log 2>&1
cmd=$(cat "$base/CMakeFiles/nano-execution-checkpoint-tests.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-execution-checkpoint-tests.dir\/tests\/test_execution_checkpoint.cpp.o/$out\/regression.o}
cmd=${cmd//-o nano-execution-checkpoint-tests /-o $out\/legacy-execution-test }
cmd=${cmd// libnano_core.a / $out\/libnano_core.a }
printf '%s\n' "$cmd" > /probe/regression-link-command.txt
(cd "$base";eval "$cmd") > /probe/regression-link.log 2>&1
"$out/legacy-execution-test" > /probe/regression-execution.log 2>&1
echo 0 > /probe/regression-execution-exit.txt
sha256sum "$out/legacy-execution-test" "$base/CMakeFiles/nano-execution-checkpoint-tests.dir/tests/test_execution_checkpoint.cpp.o" > /probe/regression-programs.sha256
printf '%s execution checkpoint regression passed\n' "$arch"
