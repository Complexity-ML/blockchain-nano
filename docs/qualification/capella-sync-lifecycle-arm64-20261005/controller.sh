#!/bin/bash
set -eu
base=/build/finalized-retention-4734f285-arm64
out=/build/capella-sync-http-arm64-20261005
trap 'status=$?; echo "$status" > "$out/lifecycle-exit.txt"' EXIT
cd "$base"
command=$(cat CMakeFiles/nano.dir/link.txt)
command=${command//CMakeFiles\/nano.dir\/src\/main.cpp.o/$out\/main.cpp.o}
command=${command//-o nano /-o $out\/nano }
command=${command// libnano_core.a / $out\/libnano_core.a }
eval "$command" > "$out/link-nano.log" 2>&1
"$out/nano-capella-transition-tests" --cli "$out/nano" > "$out/cli-serve.log" 2>&1
"$out/nano-capella-transition-tests" --managed-cli "$out/nano" > "$out/managed-cli.log" 2>&1
sha256sum "$out/nano" "$out/nano-capella-transition-tests" "$out/libnano_core.a" > "$out/programs.txt"
