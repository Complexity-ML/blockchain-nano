#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
out=/build/network-status-coherent-$arch-20261006
trap 'status=$?; echo "$status" > "$out/runtime-controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
for kind in nano capture; do
 source=/work/src/main.cpp;template=nano;program=nano
 if [ "$kind" = capture ];then source=/work/tests/capella/paused_capture.cpp;template=nano-capella-transition-tests;program=nano-capella-paused-capture;fi
 command="/usr/bin/clang++ -I/work/include -I/work/third_party/ckzg/blst/bindings $flags -c $source -o $out/$kind.o"
 printf '%s\n' "$command" > "$out/runtime-$kind-compile-command.txt"
 eval "$command" > "$out/runtime-$kind-compile.log" 2>&1
 command=$(cat "$base/CMakeFiles/$template.dir/link.txt")
 command=${command//CMakeFiles\/$template.dir\/src\/main.cpp.o/$out\/$kind.o}
 command=${command//CMakeFiles\/$template.dir\/tests\/test_capella_transition.cpp.o/$out\/$kind.o}
 command=${command//-o $template /-o $out\/$program }
 command=${command// libnano_core.a / $out\/libnano_core.a }
 printf '%s\n' "$command" > "$out/runtime-$kind-link-command.txt"
 (cd "$base";eval "$command") > "$out/runtime-$kind-link.log" 2>&1
 echo 0 > "$out/runtime-$kind-exit.txt"
done
"$out/nano" --help > "$out/nano-help.log" 2>&1
sha256sum "$out/nano" "$out/nano-capella-paused-capture" "$out/libnano_core.a" > "$out/runtime-programs.sha256"
