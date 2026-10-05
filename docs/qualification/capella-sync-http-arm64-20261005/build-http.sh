#!/bin/bash
set -eu
base=/build/finalized-retention-4734f285-arm64
out=/build/capella-sync-http-arm64-20261005
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
flags() { sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1/flags.make" | tr '\n' ' '; }
cp /build/capella-sync-arm64-20261005/libnano_core.a "$out/libnano_core.a"
cp /build/capella-sync-arm64-v3-20261005/main.cpp.o "$out/main.cpp.o"
eval "/usr/bin/clang++ $(flags nano-capella-transition-tests.dir) -c /work/tests/test_capella_transition.cpp -o $out/test.cpp.o" > "$out/compile-tests.log" 2>&1
cd "$base"
command=$(cat CMakeFiles/nano-capella-transition-tests.dir/link.txt)
command=${command//\"CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o\"/$out\/test.cpp.o}
command=${command//-o nano-capella-transition-tests/-o $out\/nano-capella-transition-tests}
command=${command// libnano_core.a / $out\/libnano_core.a }
eval "$command" > "$out/link-tests.log" 2>&1
"$out/nano-capella-transition-tests" --http > "$out/http-runtime.log" 2>&1
