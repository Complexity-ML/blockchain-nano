#!/bin/bash
set -eu
base=/build/finalized-retention-4734f285-arm64
out=/build/capella-sync-arm64-20261005
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
cp /build/pruning-schema-59fa6530-arm64/libnano_core.a "$out/libnano_core.a"
flags() { sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1/flags.make" | tr '\n' ' '; }
for name in consensus_capella_runtime node rpc; do
 eval "/usr/bin/clang++ $(flags nano_core.dir) -c /work/src/$name.cpp -o $out/$name.cpp.o" > "$out/compile-$name.log" 2>&1
 ar r "$out/libnano_core.a" "$out/$name.cpp.o"
done
eval "/usr/bin/clang++ $(flags nano-capella-transition-tests.dir) -c /work/tests/test_capella_transition.cpp -o $out/test.cpp.o" > "$out/compile-tests.log" 2>&1
cd "$base"
command=$(cat CMakeFiles/nano-capella-transition-tests.dir/link.txt)
command=${command//\"CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o\"/$out\/test.cpp.o}
command=${command//-o nano-capella-transition-tests/-o $out\/nano-capella-transition-tests}
command=${command// libnano_core.a / $out\/libnano_core.a }
eval "$command" > "$out/link-tests.log" 2>&1
"$out/nano-capella-transition-tests" --configured-runtime > "$out/configured-runtime.log" 2>&1
