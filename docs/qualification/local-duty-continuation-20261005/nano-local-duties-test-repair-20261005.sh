#!/bin/bash
set -eu
base=$1
runtime=$2
out=$3
core=$4
mkdir "$out"
trap 'nano_startup_exit=$?; echo "$nano_startup_exit" > "$out/controller-exit.txt"' EXIT
sha256sum "$runtime/nano" "$runtime/libnano_core.a" "$runtime/nano-capella-transition-tests" > "$out/parent-before.sha256"
sha256sum /work/include/nano/consensus_capella_reactor.hpp /work/include/nano/consensus_capella_duty.hpp /work/src/consensus_capella_duty.cpp /work/src/consensus_capella_signing.cpp /work/src/consensus_capella_validator.cpp /work/src/consensus_capella_local_body.cpp /work/src/consensus_capella_sync_signing.cpp /work/src/consensus_capella_reactor.cpp /work/tests/test_capella_transition.cpp > "$out/sources.sha256"
flags(){ sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1.dir/flags.make" | tr '\n' ' '; }
# Dependency files list transitive header consumers. Missing dependency files
# conservatively force recompilation; no partial reuse of a changed reactor ABI.
for source in /work/src/*.cpp; do
 unit=$(basename "$source" .cpp)
 [ "$unit" = main ] && continue
 dependency="$base/CMakeFiles/nano_core.dir/src/$unit.cpp.o.d"
 if [ "$unit" = consensus_capella_fork_store ] || [ "$unit" = consensus_capella_duty ] || [ ! -f "$dependency" ] || grep -q consensus_capella_reactor.hpp "$dependency"; then
  echo "$unit" >> "$out/recompiled-units.txt"
  if [ -f "$dependency" ]; then cp "$dependency" "$out/$unit-parent-dependencies.txt"; fi
 fi
done
sha256sum "$core/libnano_core.a" > "$out/preparation-core-before.sha256"
cp -p "$core/libnano_core.a" "$out/libnano_core.a"
for target in nano-capella-transition-tests nano; do
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
for mode in prepared-block configured-runtime; do
 echo "test $mode"
 "$out/nano-capella-transition-tests" "--$mode" > "$out/$mode.log" 2>&1
 echo 0 > "$out/$mode-exit.txt"
 cat "$out/$mode.log"
done
sha256sum -c "$out/preparation-core-before.sha256" > "$out/preparation-core-unchanged.log"
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum "$out/nano" "$out/libnano_core.a" "$out/nano-capella-transition-tests" > "$out/programs.sha256"
