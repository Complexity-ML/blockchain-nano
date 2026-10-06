#!/bin/bash
set -eu
root=/private/tmp/nano-capture-resource-budget-20261006
base=/private/tmp/nano-coherent-e33ad89e-20261006/native-build
out=$root/native
trap 'echo $? > "$out/controller-exit.txt"' EXIT
flags=$(sed -En 's/^CXX_(DEFINES|INCLUDES|FLAGS) = / /p' "$base/CMakeFiles/nano-capella-paused-capture.dir/flags.make" | tr '\n' ' ')
test -n "$flags"
cmd="/usr/bin/clang++ $flags -c $root/source/tests/capella/paused_capture.cpp -o $out/capture.o"
printf '%s\n' "$cmd" > "$out/compile-command.txt"
set +e;eval "$cmd" > "$out/compile.log" 2>&1;rc=$?;echo "$rc" > "$out/compile-exit.txt";set -e;test "$rc" -eq 0
cmd=$(cat "$base/CMakeFiles/nano-capella-paused-capture.dir/link.txt")
cmd=${cmd//\"CMakeFiles\/nano-capella-paused-capture.dir\/tests\/capella\/paused_capture.cpp.o\"/$out\/capture.o}
cmd=${cmd//-o nano-capella-paused-capture /-o $out\/capture }
printf '%s\n' "$cmd" > "$out/link-command.txt"
(cd "$base";eval "$cmd") > "$out/link.log" 2>&1
shasum -a 256 "$out/capture" "$root/policy-test" "$base/libnano_core.a" > "$out/programs.sha256"
