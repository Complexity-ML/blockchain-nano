#!/bin/bash
set -eu
base=$1
runtime=$2
out=$3
mkdir "$out"
trap 'nano_startup_exit=$?; echo "$nano_startup_exit" > "$out/controller-exit.txt"' EXIT
sha256sum "$runtime/nano" "$runtime/libnano_core.a" "$runtime/nano-capella-transition-tests" > "$out/parent-before.sha256"
sha256sum /work/include/nano/consensus_capella_fork_state.hpp /work/src/consensus_capella_fork_state.cpp /work/tests/test_capella_transition.cpp > "$out/sources.sha256"
flags(){ sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1.dir/flags.make" | tr '\n' ' '; }
cp -p /build/prepared-duty-context-arm64-20261005/libnano_core.a "$out/libnano_core.a"
sha256sum /build/prepared-duty-context-arm64-20261005/libnano_core.a "$out/libnano_core.a" > "$out/reused-core.sha256"
for target in nano-capella-transition-tests; do
 source=/work/tests/test_capella_transition.cpp
 object="CMakeFiles/$target.dir/tests/test_capella_transition.cpp.o"
 if [ "$target" = nano ]; then source=/work/src/main.cpp; object="CMakeFiles/nano.dir/src/main.cpp.o"; fi
 echo "compile $target"
 eval "/usr/bin/clang++ $(flags "$target") -I/work/third_party/ckzg/blst/bindings -c $source -o $out/$target.o" > "$out/$target-compile.log" 2>&1
 echo 0 > "$out/$target-compile-exit.txt"
 command=$(cat "$base/CMakeFiles/$target.dir/link.txt")
 command=${command//$object/$out\/$target.o}
 command=${command//-o $target /-o $out\/$target }
 command=${command// libnano_core.a / $out\/libnano_core.a }
 printf '%s\n' "$command" > "$out/$target-link-command.txt"
 (cd "$base"; eval "$command") > "$out/$target-link.log" 2>&1
 echo 0 > "$out/$target-link-exit.txt"
done
for mode in slot-continuation configured-runtime; do
 echo "test $mode"
 "$out/nano-capella-transition-tests" "--$mode" > "$out/$mode.log" 2>&1
 echo 0 > "$out/$mode-exit.txt"
 cat "$out/$mode.log"
done
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum "$out/libnano_core.a" "$out/nano-capella-transition-tests" > "$out/programs.sha256"
