#!/bin/bash
set -eu
base=/build/finalized-retention-4734f285-amd64
out=/build/capella-sync-http-amd64-20261005
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
flags() { sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1/flags.make" | tr '\n' ' '; }
cp /build/capella-sync-amd64-20261005/libnano_core.a "$out/libnano_core.a"
eval "/usr/bin/clang++ $(flags nano.dir) -c /work/src/main.cpp -o $out/main.cpp.o" > "$out/compile-main.log" 2>&1
eval "/usr/bin/clang++ $(flags nano-capella-transition-tests.dir) -c /work/tests/test_capella_transition.cpp -o $out/test.cpp.o" > "$out/compile-tests.log" 2>&1
cd "$base"
command=$(cat CMakeFiles/nano-capella-transition-tests.dir/link.txt)
command=${command//\"CMakeFiles\/nano-capella-transition-tests.dir\/tests\/test_capella_transition.cpp.o\"/$out\/test.cpp.o}
command=${command//-o nano-capella-transition-tests/-o $out\/nano-capella-transition-tests}
command=${command// libnano_core.a / $out\/libnano_core.a }
eval "$command" > "$out/link-tests.log" 2>&1
"$out/nano-capella-transition-tests" --http > "$out/http-runtime.log" 2>&1

cd "$base"
command=$(cat CMakeFiles/nano.dir/link.txt)
command=${command//CMakeFiles\/nano.dir\/src\/main.cpp.o/$out\/main.cpp.o}
command=${command//-o nano /-o $out\/nano }
command=${command// libnano_core.a / $out\/libnano_core.a }
eval "$command" > "$out/link-nano.log" 2>&1
"$out/nano-capella-transition-tests" --cli "$out/nano" > "$out/cli-serve.log" 2>&1
"$out/nano-capella-transition-tests" --managed-cli "$out/nano" > "$out/managed-cli.log" 2>&1
sha256sum "$out/nano" "$out/nano-capella-transition-tests" "$out/libnano_core.a" > "$out/programs.txt"
