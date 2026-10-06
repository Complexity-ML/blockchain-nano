#!/bin/bash
set -eu
arch=$1
base=/build/coherent-e33ad89e-$arch
out=/build/private-capture-budget-$arch-20261006
mkdir "$out"
trap 'echo $? > /probe/controller-exit.txt' EXIT
/usr/bin/clang++ -std=c++20 -O2 -I/work/third_party /patch/test_private_capture_budget.cpp -o "$out/policy-test" > /probe/compile-policy.log 2>&1
"$out/policy-test" > /probe/policy-test.log 2>&1
cp -as /work /tmp/patched-source
rm /tmp/patched-source/tests/capella/paused_capture.cpp
cp /patch/paused_capture.cpp /patch/private_capture_budget.hpp /tmp/patched-source/tests/capella/
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-paused-capture.dir/flags.make" | tr '\n' ' ')
cmd="/usr/bin/clang++ $flags -c /tmp/patched-source/tests/capella/paused_capture.cpp -o $out/capture.o"
printf '%s\n' "$cmd" > /probe/compile-capture-command.txt
set +e;eval "$cmd" > /probe/compile-capture.log 2>&1;rc=$?;echo "$rc" > /probe/compile-capture-exit.txt;set -e;test "$rc" -eq 0
cmd=$(cat "$base/CMakeFiles/nano-capella-paused-capture.dir/link.txt")
cmd=${cmd//CMakeFiles\/nano-capella-paused-capture.dir\/tests\/capella\/paused_capture.cpp.o/$out\/capture.o}
cmd=${cmd//-o nano-capella-paused-capture /-o $out\/capture }
printf '%s\n' "$cmd" > /probe/link-command.txt
(cd "$base";eval "$cmd") > /probe/link.log 2>&1
sha256sum "$out/policy-test" "$out/capture" "$base/libnano_core.a" > /probe/programs.sha256
