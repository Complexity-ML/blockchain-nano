#!/bin/bash
set -eu
base=$1
runtime=$2
out=$3
mkdir "$out"
trap 'code=$?; echo "$code" > "$out/controller-exit.txt"' EXIT
sha256sum "$runtime/nano" "$runtime/libnano_core.a" > "$out/parent-before.sha256"
sha256sum /work/tests/capella/paused_capture.cpp > "$out/tool-source.sha256"
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano-capella-transition-tests.dir/flags.make" | tr '\n' ' ')
eval "/usr/bin/clang++ $flags -I/work/third_party/ckzg/blst/bindings -c /work/tests/capella/paused_capture.cpp -o $out/paused-capture.o" > "$out/compile.log" 2>&1
echo 0 > "$out/compile-exit.txt"
command=$(cat "$base/CMakeFiles/nano-capella-transition-tests.dir/link.txt")
command=${command//CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o/$out\/paused-capture.o}
command=${command//-o nano-capella-transition-tests /-o $out\/nano-capella-paused-capture }
command=${command// libnano_core.a / $runtime\/libnano_core.a }
printf '%s\n' "$command" > "$out/link-command.txt"
(cd "$base"; eval "$command") > "$out/link.log" 2>&1
echo 0 > "$out/link-exit.txt"
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum "$out/nano-capella-paused-capture" > "$out/program.sha256"
